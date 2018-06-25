// GoldenAge.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define SERVER_CERT_FILE "./Auth/cert.pem"
#define SERVER_PRIVATE_KEY_FILE "./Auth/key.pem"
#define ACCOUNTMANIFESTLOCATION "./Manifests/accountmanifest.txt"

using namespace std;

struct gameserver {
	int details;
};

struct account {
	string email;
	string password;

	account(string email, string password)
	{
		debug("account structure email: ", email, ", password: ", password)
		this->email = email;
		this->password = password;
	}
};

struct cryptinfo {
	uint8_t key[32];
	uint8_t iv[AES_BLOCK_SIZE];
	unsigned char tag[AES_BLOCK_SIZE];
	EVP_CIPHER_CTX *ectx;
	EVP_CIPHER_CTX *dctx;
};

void createNewRandKeyIV(cryptinfo& ci)
{
	RAND_bytes(ci.key, sizeof(ci.key));
	RAND_bytes(ci.iv, sizeof(ci.iv));
}

void handleErrors(void)
{
	unsigned long errCode;
	debug("An error has occured in encryption/decryption")
	while (errCode = ERR_get_error())
		debug(ERR_error_string(errCode, NULL))		
	abort();
}

void getNewEncryptParams(cryptinfo& ci)
{
	debug("getting new encryption params")
	debug("making new ctx, pass me a unfilled one")
	if (!(ci.ectx = EVP_CIPHER_CTX_new())) handleErrors();
	debug("initializing the encryption operation")
	if (1 != EVP_EncryptInit_ex(ci.ectx, EVP_aes_256_gcm(), NULL, NULL, NULL)) handleErrors();
	debug("setting iv_len if default 12 bytes (96 bits) is not appropriate")
	if (1 != EVP_CIPHER_CTX_ctrl(ci.ectx, EVP_CTRL_GCM_SET_IVLEN, AES_BLOCK_SIZE, NULL)) handleErrors();
	debug("initialize key and IV", "\nbefore: ", ci.key, " ", ci.iv)
	if (1 != EVP_EncryptInit_ex(ci.ectx, NULL, NULL, ci.key, ci.iv)) handleErrors();
	debug("read key and IV", "\nafter: ", ci.key, " ", ci.iv)
}

int encrypt(cryptinfo& ci, unsigned char* plaintext, int plaintext_len,
	unsigned char* ciphertext, unsigned char* aad = 0,	int aad_len = 0)
{
	debug("padding plaintext to 16 byte offset")
	plaintext_len += AES_BLOCK_SIZE - (plaintext_len % AES_BLOCK_SIZE);
	debug(plaintext_len)
	debug("encrypting string ", plaintext)
	int len;
	int ciphertext_len;
	debug("writing encrypted bytes to ciphertext")
	if (1 != EVP_EncryptUpdate(ci.ectx, ciphertext, &len, plaintext, plaintext_len)) handleErrors();
	ciphertext_len = len;
	debug("writing final bytes to ciphertext")
	if (1 != EVP_EncryptFinal_ex(ci.ectx, ciphertext + len, &len)) handleErrors();
	ciphertext_len += len;
	debug("getting tag")
	if (1 != EVP_CIPHER_CTX_ctrl(ci.ectx, EVP_CTRL_GCM_GET_TAG, AES_BLOCK_SIZE, ci.tag)) handleErrors();
	debug("encrypted string ", ciphertext)
	debug("free ctx obj")
	EVP_CIPHER_CTX_free(ci.ectx);
	debug("return ciphertext_len")
	return ciphertext_len;
}

void getNewDecryptParams(cryptinfo& ci)
{
	debug("getting new decryption params")
	debug("making new ctx, pass me a unfilled one")
	if (!(ci.dctx = EVP_CIPHER_CTX_new())) handleErrors();
	debug("initializing the decryption operation")
	if (!EVP_DecryptInit_ex(ci.dctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) handleErrors();
	debug("setting iv_len if default 12 bytes (96 bits) is not appropriate")
	if (!EVP_CIPHER_CTX_ctrl(ci.dctx, EVP_CTRL_GCM_SET_IVLEN, AES_BLOCK_SIZE, NULL)) handleErrors();
	debug("setting dctx key and iv \nbefore ", ci.key, " ", ci.iv)
	if (!EVP_DecryptInit_ex(ci.dctx, NULL, NULL, ci.key, ci.iv)) handleErrors();
}

void fillcryptinfo(cryptinfo& ci)
{
	getNewEncryptParams(ci);
	getNewDecryptParams(ci);
}

int decrypt(cryptinfo& ci, unsigned char *ciphertext, int ciphertext_len,
	unsigned char *plaintext, unsigned char *aad = 0, int aad_len = 0)
{
	debug("decrypting string ", ciphertext)
	int len;
	int plaintext_len;
	int ret;
	debug("writing unencrypted bytes to plaintext")
	if (!EVP_DecryptUpdate(ci.dctx, plaintext, &len, ciphertext, ciphertext_len))
		handleErrors();
	plaintext_len = len;
	debug("setting tag")
	if (1 != EVP_CIPHER_CTX_ctrl(ci.dctx, EVP_CTRL_GCM_SET_TAG, AES_BLOCK_SIZE, ci.tag)) handleErrors();
	debug("writing final bytes to plaintext")
	ret = EVP_DecryptFinal_ex(ci.dctx, plaintext + len, &len);
	debug("decrypted data: ", plaintext)
	debug("freeing ctx obj")
	EVP_CIPHER_CTX_free(ci.dctx);
	if (ret == 1)
	{
		debug("decrypt success\ndecrypted string: ", plaintext)
		debug("returning plain text len")
		plaintext_len += len;
		return plaintext_len;
	}
	else
	{
		debug("there may have been an error check != -1")
		debug("we'll treat these as warnings for now and see how to get rid of them later")
		debug("returning plaintextlen")
		plaintext_len += len;
		return plaintext_len;
		//return -1;
	}
}

unordered_map<string, string> accounts;
unordered_map<string, gameserver> gameservermap;

//takes char email[100] and char pass[100], never overwrites the buffer
//but seems huge. at least it handles large emails and passwords
account getEmailAndPassword(char* email, char* pass, const httplib::Request& req)
{
	debug("reading email and password from request")
	int i = 0;
	for (; i < 100; ++i)
	{
		if (req.body[i] == ' ')
		{
			email[i] = '\0';
			++i;
			break;
		}
		else {
			email[i] = req.body[i];
		}
	}
	for (int j = 0; i < 200; ++i, ++j)
	{
		if (req.body[i] == '\0')
		{
			pass[j] = '\0';
			break;
		}
		else {
			pass[j] = req.body[i];
		}
	}

	return account(email, pass);
}

account getAccountFromReq(const httplib::Request& req)
{
	debug("making email and password structure")
	char email[100];
	char pass[100];
	return getEmailAndPassword(email, pass, req);
}

void initAccountsMap()
{
	debug("initializing accounts map")
	ifstream accountmanifest(ACCOUNTMANIFESTLOCATION);
	for (string line = ""; getline(accountmanifest, line);)
	{
		debug("opening ", "./Accounts/" + line + ".txt")
		ifstream acc("./Accounts/" + line + ".txt");
		string p;
		acc >> p;
		accounts.insert(make_pair(line, p));
		acc.close();
	}
	accountmanifest.close();
}

void initGameServerMap()
{

}

void appendEmailToManifest(string email)
{
	debug("appending email to manifest")
	ofstream manifest(ACCOUNTMANIFESTLOCATION, ios::app);
	manifest << email << "\n";
	manifest.close();
}

void writePasswordToAccountFile(string email, string password)
{
	debug("writing password [" +
		password +
		"] to file ./Accounts/" +
		email +
		".txt")
	ofstream accountfile("./Accounts/" + email + ".txt");
	accountfile << password;
	accountfile.close();
}

int main()
{
	debug("size of cryptinfo ", sizeof(cryptinfo)) //120 - 18 for packet
	ERR_load_crypto_strings();

	initAccountsMap();

	debug("Creating the server, passing it the cert and private key.");	
	httplib::SSLServer server(SERVER_CERT_FILE, SERVER_PRIVATE_KEY_FILE);

	if (!server.is_valid()) {
		debug("Error creating server, shutting down...\n\n\t")
		system("pause");
		return -1;
	}

	//REST API - AUTH A USER, ESTABLISH CRYPTO SHIT, TELL GAME SERVER,
	//GET TOLD BY GAME SERVER WHEN TO TELL CLIENT, TELL CLIENT
	//ALSO - CREATE ACCOUNT (MAKE FIRST)
	server.Post("/login", [](const httplib::Request& req, httplib::Response& res) {
		//login
		debug("got a login post")
		debug(req.body)
		account newaccount = getAccountFromReq(req);

		debug("search for email ", newaccount.email, " in account map")
		//if file exists
		if (accounts.find(newaccount.email) != accounts.end())
		{
			//open the file and compare the passwords
			debug("account exists")
			debug("comparing passwords")
			debug(newaccount.password, " == ", accounts.at(newaccount.email))
			if (newaccount.password == accounts.at(newaccount.email))
			{
				//calculate their crypto shit and, tell the game server
				//wait for it to tell you back, and then tell the client lol
				debug("password matched")
				cryptinfo ci;
				createNewRandKeyIV(ci);
				fillcryptinfo(ci);
				debug("filled decrypt info ", ci.dctx, " ",
					ci.iv, " ", ci.key)
				debug("filled encrypt info ", ci.ectx, " ",
					ci.iv, " ", ci.key)
				unsigned char encrypteddata[128] = { 0 };
				unsigned int len = encrypt(ci, (unsigned char*)"lololol", 8, encrypteddata);
				unsigned char decrypteddata[128] = { 0 };
				decrypt(ci, encrypteddata, len, decrypteddata);
				//send cryptinfo over in body
				//send 127.0.0.1:3000 

				//inform the client of success
				res.set_content(" ", "text/plain");
			}
			else {
				//they got the password wrong
				debug("password did not match")

				//inform the client of failure
				res.set_content("", "text/plain");
			}
		}
		else
		{
			debug("There is no account with that email")
			
			//inform the client of failure
			res.set_content("", "text/plain");
		}
	});
	
	server.Post("/createaccount", [](const httplib::Request& req, httplib::Response& res) {
		//just create an account, button to login will be different
		debug("got a createaccount post")
		debug(req.body)
		
		//get the name and the password out
		account newaccount = getAccountFromReq(req);

		//if account is not in accounts
		if (accounts.find(newaccount.email) == accounts.end())
		{
			debug("account was not already in accounts map")
			appendEmailToManifest(newaccount.email);
			writePasswordToAccountFile(newaccount.email, newaccount.password);
			accounts.insert(make_pair(newaccount.email, newaccount.password));

			//inform the client of success
			res.set_content(" ", "text/plain");
		}
		else
		{
			debug("account was in accounts map already")

			//inform the client of failure
			res.set_content("", "text/plain");
		}
	});

	debug("Https server listening at https://localhost:1234")
	thread loginserverthread([&server]() {
		server.listen("localhost", 1234);
	});

	loginserverthread.join();
	debug("program exiting\n")
	system("pause");
    return 0;
}