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

uint8_t Key[32];
uint8_t IV[AES_BLOCK_SIZE];
uint8_t IVd[AES_BLOCK_SIZE];
AES_KEY* AesKey = new AES_KEY();
AES_KEY* AesDecryptKey = new AES_KEY();

// Generate an AES Key
void initKeyAndIV()
{
	debug("init key and IV")
	RAND_bytes(Key, sizeof(Key));
	RAND_bytes(IV, sizeof(IV));
}

// Make a copy of the IV to IVd as it seems to get destroyed when used
void initIVd()
{
	debug("init IVd")
	for (int i = 0; i < AES_BLOCK_SIZE; i++) {
		IVd[i] = IV[i];
	}
}

/** Setup the AES Key structure required for use in the OpenSSL APIs **/
void setAESKey()
{
	debug("setting AES encrypt key to ", Key, " ", 256, " ", AesKey)
	AES_set_encrypt_key(Key, 256, AesKey);
}

/** Setup an AES Key structure for the decrypt operation **/
void setAESDecryptKey()
{
	debug("setting AES decrypt key to ", Key, " ", 256, " ", AesDecryptKey)
	AES_set_decrypt_key(Key, 256, AesDecryptKey);
}

/** take an input string and pad it so it fits into 16 bytes (AES Block Size) **/
vector<unsigned char> padstr(string txt)
{
	debug("padding string ", txt, " to 16 byte alignment")
	const int UserDataSize = (const int)txt.length();
	int RequiredPadding = (AES_BLOCK_SIZE - (txt.length() % AES_BLOCK_SIZE));
	std::vector<unsigned char> PaddedTxt(txt.begin(), txt.end());
	for (int i = 0; i < RequiredPadding; i++)
		PaddedTxt.push_back(0);
	return PaddedTxt;
}

void encrypt(string txt, unsigned char* encrypteddata, unsigned int& bufferlen)
{						
	vector<unsigned char> pad = padstr(txt);
	bufferlen = pad.size();
	debug("encrypting ")
	for (int i = 0; i < pad.size(); ++i)
	{
		if (pad[i] == '\0') debug("0")
		else debug(pad[i])
	}
	AES_cbc_encrypt((unsigned char*)&pad[0], encrypteddata, pad.size(), (const AES_KEY*)AesKey, IV, AES_ENCRYPT);
	debug("encrypted data ", encrypteddata)
}

/** Decrypt the data **/
void decrypt(unsigned char* encrypteddata, unsigned char* decrypteddata, size_t length)
{
	debug("decrypting ", encrypteddata)
	AES_cbc_encrypt(encrypteddata, decrypteddata, length, (const AES_KEY*)AesDecryptKey, IVd, AES_DECRYPT);
	debug("decrypted data ", decrypteddata)
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
		//I have the email
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
	initAccountsMap();

	initKeyAndIV();
	initIVd();
	setAESKey();
	setAESDecryptKey();

	debug("Testing encryption and decryption")
	string test = "I am the encrypted string";
	debug("init test ", test)
	unsigned char encrypteddata[512] = { 0 };
	unsigned int bufferlen;
	encrypt(test, encrypteddata, bufferlen);
	unsigned char decrypteddata[512] = { 0 };
	decrypt(encrypteddata, decrypteddata, bufferlen);


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
	server.listen("localhost", 1234);
    return 0;
}

