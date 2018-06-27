// GoldenAge.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;

struct gameserver {
	char localhost[11];
	int port;
};

struct cryptinfo {
	uint8_t key[32];
	uint8_t iv[AES_BLOCK_SIZE];
	EVP_CIPHER_CTX *ectx;
	EVP_CIPHER_CTX *dctx;

private:
	friend std::ostream& operator<<(std::ostream &os, const  cryptinfo  &e);
	friend std::istream& operator>>(std::istream &os, cryptinfo  &e);
};

std::ostream& operator<<(std::ostream &os, const  cryptinfo  &e)
{
	for (int i = 0; i < 32; i++)
	{
		os << e.key[i];
	}
	os << "\n";
	for (int i = 0; i < AES_BLOCK_SIZE; i++)
	{
		os << e.iv[i];
	}
	os << "\n";

	return os;
}

std::istream& operator>>(std::istream &os, cryptinfo  &e)
{
	for (int i = 0; i < 32; i++)
	{
		os >> e.key[i];
	}
	for (int i = 0; i < AES_BLOCK_SIZE; i++)
	{
		os >> e.iv[i];
	}
	return os;
}

struct sessiondata {
	string email;
	string password;
	string gameservername;
	cryptinfo ci;

	bool loggedin = false;

	void fillinsd(string email, string password, string gameservername)
	{
		this->email = email;
		this->password = password;
		this->gameservername = gameservername;
	}
};


void createNewRandKeyIV(cryptinfo& ci)
{
	debug("creating new key and iv for a cryptinfo");
	RAND_bytes(ci.key, sizeof(ci.key));
	RAND_bytes(ci.iv, sizeof(ci.iv));
}

void handleErrors(void)
{
	unsigned long errCode;
	debug("An error has occured in encryption/decryption");
	while (errCode = ERR_get_error())
		debug(ERR_error_string(errCode, NULL));		
	abort();
}

void getNewEncryptParams(cryptinfo& ci)
{
	debug("getting new encryption params");
	debug("making new ctx, pass me a unfilled one");
	if (!(ci.ectx = EVP_CIPHER_CTX_new())) handleErrors();
	debug("initializing the encryption operation");
	if (1 != EVP_EncryptInit_ex(ci.ectx, EVP_aes_256_cbc(), NULL, NULL, NULL)) handleErrors();
	debug("initialize key and IV", "\nbefore: ", ci.key, " ", ci.iv);
	if (1 != EVP_EncryptInit_ex(ci.ectx, NULL, NULL, ci.key, ci.iv)) handleErrors();
	debug("read key and IV", "\nafter: ", ci.key, " ", ci.iv);
}

int encrypt(cryptinfo& ci, unsigned char* plaintext, int plaintext_len,
	unsigned char* ciphertext, unsigned char* aad = 0,	int aad_len = 0)
{
	debug("encrypting string ", plaintext);
	int len;
	int ciphertext_len;
	debug("writing encrypted bytes to ciphertext");
	if (1 != EVP_EncryptUpdate(ci.ectx, ciphertext, &len, plaintext, plaintext_len)) handleErrors();
	ciphertext_len = len;
	debug("writing final bytes to ciphertext");
	if (1 != EVP_EncryptFinal_ex(ci.ectx, ciphertext + len, &len)) handleErrors();
	ciphertext_len += len;
	debug("encrypted string ", ciphertext);
	debug("free ctx obj");
	EVP_CIPHER_CTX_free(ci.ectx);
	debug("return ciphertext_len");
	return ciphertext_len;
}

void getNewDecryptParams(cryptinfo& ci)
{
	debug("getting new decryption params");
	debug("making new ctx, pass me a unfilled one");
	if (!(ci.dctx = EVP_CIPHER_CTX_new())) handleErrors();
	debug("initializing the decryption operation");
	if (!EVP_DecryptInit_ex(ci.dctx, EVP_aes_256_cbc(), NULL, NULL, NULL)) handleErrors();
	debug("setting dctx key and iv \nbefore ", ci.key, " ", ci.iv);
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
	debug("decrypting string ", ciphertext);
	int len;
	int plaintext_len;
	int ret;
	debug("writing unencrypted bytes to plaintext");
	if (!EVP_DecryptUpdate(ci.dctx, plaintext, &len, ciphertext, ciphertext_len))
		handleErrors();
	plaintext_len = len;
	debug("writing final bytes to plaintext");
	ret = EVP_DecryptFinal_ex(ci.dctx, plaintext + len, &len);
	debug("decrypted data: ", plaintext);
	debug("freeing ctx obj");
	EVP_CIPHER_CTX_free(ci.dctx);
	if (ret == 1)
	{
		debug("decrypt success\ndecrypted string: ", plaintext);
		debug("returning plain text len");
		plaintext_len += len;
		return plaintext_len;
	}
	else
	{
		debug("there may have been an error check != -1");
		return -1;
	}
}

unordered_map<string, sessiondata> accounts;
unordered_map<string, gameserver> gameservermap;

//takes char email[100] and char pass[100], never overwrites the buffer
//but seems huge. at least it handles large emails and passwords
void getEmailPasswordGameServer(char* email, char* pass, char* gameserver, const httplib::Request& req)
{
	debug("reading email and password from request");
	int i = 0;
	for (; i < 21; ++i)
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
	debug(email);
	for (int j = 0; i < 42; ++i, ++j)
	{
		if (req.body[i] == ' ')
		{
			pass[j] = '\0';
			++i;
			break;
		}
		else {
			pass[j] = req.body[i];
		}
	}
	debug(pass);
	for (int k = 0; i < 63; ++k, ++i)
	{
		if (req.body[i] == '\0')
		{
			gameserver[k] = '\0';
			//++i;
			break;
		}
		else {
			gameserver[k] = req.body[i];
		}
	}
	debug(gameserver);
}

void getAccountFromReq(sessiondata& sd, const httplib::Request& req)
{
	debug("making email and password structure");
	char email[LOGINBUFFERMAX];
	char pass[LOGINBUFFERMAX];
	char gameserver[LOGINBUFFERMAX];
	getEmailPasswordGameServer(email, pass, gameserver, req);
	sd.fillinsd(email, pass, gameserver);
}

void initAccountsMap()
{
	debug("initializing accounts map");
	ifstream accountmanifest(ACCOUNTMANIFESTLOCATION);
	for (string line = ""; getline(accountmanifest, line);)
	{
		debug("opening ", "./Accounts/" + line + ".txt");
		ifstream acc("./Accounts/" + line + ".txt");
		string p, gsn;
		acc >> p;
		acc >> gsn;
		sessiondata newsession;
		newsession.fillinsd(line, p, gsn);
		accounts.insert(make_pair(line, newsession));
		acc.close();
	}
	accountmanifest.close();
}

void initGameServerMap()
{

}

void appendEmailToManifest(string email)
{
	debug("appending email to manifest");
	ofstream manifest(ACCOUNTMANIFESTLOCATION, ios::app);
	manifest << email << "\n";
	manifest.close();
}

void writeAccountFile(cryptinfo& ci, sessiondata& data)
{
	unsigned char encryptedpassword[64];
	fillcryptinfo(ci);
	unsigned int len = encrypt(ci, (unsigned char*)data.password.c_str(), data.password.size(), encryptedpassword);
	debug("writing password [",
		encryptedpassword,
		"] to file ./Accounts/",
		data.email,
		".txt");
	ofstream accountfile("./Accounts/" + data.email + ".txt");
	for (int i = 0; i < len; ++i)
	{
		accountfile << encryptedpassword[i];
	}
	accountfile << "\n";
	accountfile << data.gameservername << "\n";

	accountfile.close();
}

void readInPassCryptInfo(cryptinfo& ci)
{
	debug("reading in file", PASSWORDKEYLOCATION);
	ifstream inpass(PASSWORDKEYLOCATION);
	inpass >> ci;
	inpass.close();
	debug("on in: ", ci);
}

int main()
{
	debug("size of cryptinfo ", sizeof(cryptinfo));
	ERR_load_crypto_strings();

	initAccountsMap();

	debug("setting up password cryptinfo");
	cryptinfo passci;
	readInPassCryptInfo(passci);
	fillcryptinfo(passci);


	debug("Creating the server, passing it the cert and private key.");	
	httplib::SSLServer server(SERVER_CERT_FILE, SERVER_PRIVATE_KEY_FILE);

	if (!server.is_valid()) {
		debug("Error creating server, shutting down...\n\n\t");
		system("pause");
		return -1;
	}

	//REST API - AUTH A USER, ESTABLISH CRYPTO SHIT, TELL GAME SERVER,
	//GET TOLD BY GAME SERVER WHEN TO TELL CLIENT, TELL CLIENT
	//ALSO - CREATE ACCOUNT (MAKE FIRST)
	server.Post("/login", [&passci](const httplib::Request& req, httplib::Response& res) {
		//login
		debug("got a login post");
		debug(req.body);
		sessiondata sessdata;
		getAccountFromReq(sessdata, req);

		debug("search for email ", sessdata.email, " in account map");
		//if file exists
		if (accounts.find(sessdata.email) != accounts.end())
		{
			//open the file and compare the passwords
			debug("account exists");
			debug("comparing passwords and checking bloggedin");
			sessiondata& sessd = accounts.at(sessdata.email);
			fillcryptinfo(passci);
			unsigned char decryptdata[64];
			unsigned int len = decrypt(passci, (unsigned char*)sessd.password.c_str(), sessd.password.size(),  decryptdata);
			decryptdata[len] = '\0';
			string dpass = (const char*)decryptdata;
			if ((sessdata.password == dpass) && (!sessd.loggedin))
			{
				sessd.loggedin = true;
				debug("password matched");
				cryptinfo ci;
				createNewRandKeyIV(ci);
				fillcryptinfo(ci);
				debug("filled crypt info ", ci);
				//unsigned char encrypteddata[128] = { 0 };
				//unsigned int len = encrypt(ci, (unsigned char*)"lololol", 8, encrypteddata);
				//unsigned char decrypteddata[128]; //decrypted data doesn't have to be nulled
				//decrypt(ci, encrypteddata, len, decrypteddata);
				//send cryptinfo over in body
				//send 127.0.0.1:3000 - this is game server info 

				//inform the client of success
				res.set_content(" ", "text/plain");
			}
			else {
				//they got the password wrong
				debug("password did not match or they are already logged in");
					
				//inform the client of failure
				res.set_content("", "text/plain");
			}
		}
		else
		{
			debug("There is no account with that email");
			
			//inform the client of failure
			res.set_content("", "text/plain");
		}
	});
	
	server.Post("/createaccount", [&passci](const httplib::Request& req, httplib::Response& res) {
		//just create an account, button to login will be different
		debug("got a createaccount post");
		debug(req.body);
		
		//get the name and the password out
		sessiondata sessdata;
		getAccountFromReq(sessdata, req);

		//if account is not in accounts
		debug("finding ", sessdata.email);
		if (accounts.find(sessdata.email) == accounts.end())
		{
			debug("account was not already in accounts map");
			appendEmailToManifest(sessdata.email);
			writeAccountFile(passci, sessdata);
			accounts.insert(make_pair(sessdata.email, sessdata));

			//inform the client of success
			res.set_content(" ", "text/plain");
		}
		else
		{
			debug("account was in accounts map already");

			//inform the client of failure
			res.set_content("", "text/plain");
		}
	});

	debug("Https server listening at https://localhost:1234");
	thread loginserverthread([&server]() {
		server.listen("localhost", 1234);
	});

	//TO-DO listen on the udp socket in a loop

	debug("something something thread something");
	loginserverthread.join();
	debug("program exiting\n");
	system("pause");
    return 0;
}

//make it forkable!