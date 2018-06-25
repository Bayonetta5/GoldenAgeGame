// GoldenAge.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "C:/Users/cool_/Documents/cpp-httplib/httplib.h" //<path to your httplib.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <inttypes.h>
#include <algorithm>
#include <openssl/aes.h>
#include <openssl/rand.h>

#ifdef WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

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
		cout << "account structure email: " << email << ", password: " << password << endl;
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
	cout << "init key and IV" << endl;
	RAND_bytes(Key, sizeof(Key));
	RAND_bytes(IV, sizeof(IV));
}

// Make a copy of the IV to IVd as it seems to get destroyed when used
void initIVd()
{
	cout << "init IVd" << endl;
	for (int i = 0; i < AES_BLOCK_SIZE; i++) {
		IVd[i] = IV[i];
	}
}

/** Setup the AES Key structure required for use in the OpenSSL APIs **/
void setAESKey()
{
	cout << "seAESKey to " << Key << " " << 256 << " " << AesKey << endl;
	AES_set_encrypt_key(Key, 256, AesKey);
}

/** Setup an AES Key structure for the decrypt operation **/
void setAESDecryptKey()
{
	cout << "setting AES decrypt key to " << Key << " " << 256 << " " << AesDecryptKey << endl;
	AES_set_decrypt_key(Key, 256, AesDecryptKey);
}

/** take an input string and pad it so it fits into 16 bytes (AES Block Size) **/
vector<unsigned char> padstr(string txt)
{
	cout << "padding string " << txt << " to 16 byte alignment" << endl;
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
	cout << "encrypting ";
	for (int i = 0; i < pad.size(); ++i)
	{
		if (pad[i] == '\0') cout << "0";
		else cout << pad[i];
	}
	cout << endl;
	AES_cbc_encrypt((unsigned char*)&pad[0], encrypteddata, pad.size(), (const AES_KEY*)AesKey, IV, AES_ENCRYPT);
	cout << "encrypted data " << encrypteddata << endl;
}

/** Decrypt the data **/
void decrypt(unsigned char* encrypteddata, unsigned char* decrypteddata, size_t length)
{
	cout << "decrypting " << encrypteddata << endl;
	AES_cbc_encrypt(encrypteddata, decrypteddata, length, (const AES_KEY*)AesDecryptKey, IVd, AES_DECRYPT);
	cout << "decrypted data " << decrypteddata << endl;
}

unordered_map<string, string> accounts;
unordered_map<string, gameserver> gameservermap;

//takes char email[100] and char pass[100], never overwrites the buffer
//but seems huge. at least it handles large emails and passwords
account getEmailAndPassword(char* email, char* pass, const httplib::Request& req)
{
	cout << "reading email and password from request" << endl;
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
	cout << "making email and password structure" << endl;
	char email[100];
	char pass[100];
	return getEmailAndPassword(email, pass, req);
}

void initAccountsMap()
{
	cout << "initializing accounts map" << endl;
	ifstream accountmanifest(ACCOUNTMANIFESTLOCATION);
	for (string line = ""; getline(accountmanifest, line);)
	{
		//I have the email
		cout << "opening " << "./Accounts/" + line + ".txt" << endl;
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
	cout << "appending email to manifest" << endl;
	ofstream manifest(ACCOUNTMANIFESTLOCATION, ios::app);
	manifest << email << "\n";
	manifest.close();
}

void writePasswordToAccountFile(string email, string password)
{
	cout << "writing password [" +
		password +
		"] to file ./Accounts/" +
		email +
		".txt" << endl;
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

	cout << "Testing encryption and decryption" << endl;
	string test = "I am the encrypted string";
	cout << "init test " << test << endl;
	unsigned char encrypteddata[512] = { 0 };
	unsigned int bufferlen;
	encrypt(test, encrypteddata, bufferlen);
	unsigned char decrypteddata[512] = { 0 };
	decrypt(encrypteddata, decrypteddata, bufferlen);


	cout << "Creating the server, passing it the cert and private key." << endl;	
	httplib::SSLServer server(SERVER_CERT_FILE, SERVER_PRIVATE_KEY_FILE);

	if (!server.is_valid()) {
		cout << "Error creating server, shutting down...\n\n\t";
		system("pause");
		return -1;
	}

	//REST API - AUTH A USER, ESTABLISH CRYPTO SHIT, TELL GAME SERVER,
	//GET TOLD BY GAME SERVER WHEN TO TELL CLIENT, TELL CLIENT
	//ALSO - CREATE ACCOUNT (MAKE FIRST)
	server.Post("/login", [](const httplib::Request& req, httplib::Response& res) {
		//login
		cout << "got a login post" << endl;
		cout << req.body << endl;
		account newaccount = getAccountFromReq(req);

		cout << "search for email " << newaccount.email << " in account map" << endl;
		//if file exists
		if (accounts.find(newaccount.email) != accounts.end())
		{
			//open the file and compare the passwords
			cout << "account exists" << endl;
			cout << "comparing passwords" << endl;
			cout << newaccount.password << " == " << accounts.at(newaccount.email) << endl;
			if (newaccount.password == accounts.at(newaccount.email))
			{
				//calculate their crypto shit and, tell the game server
				//wait for it to tell you back, and then tell the client lol
				cout << "password matched" << endl;

				//inform the client of success
				res.set_content(" ", "text/plain");
			}
			else {
				//they got the password wrong
				cout << "password did not match" << endl;

				//inform the client of failure
				res.set_content("", "text/plain");
			}
		}
		else
		{
			cout << "There is no account with that email" << endl;
			
			//inform the client of failure
			res.set_content("", "text/plain");
		}
	});
	
	server.Post("/createaccount", [](const httplib::Request& req, httplib::Response& res) {
		//just create an account, button to login will be different
		cout << "got a createaccount post" << endl;
		cout << req.body << endl;
		
		//get the name and the password out
		account newaccount = getAccountFromReq(req);

		//if account is not in accounts
		if (accounts.find(newaccount.email) == accounts.end())
		{
			cout << "account was not already in accounts map" << endl;
			appendEmailToManifest(newaccount.email);
			writePasswordToAccountFile(newaccount.email, newaccount.password);
			accounts.insert(make_pair(newaccount.email, newaccount.password));

			//inform the client of success
			res.set_content(" ", "text/plain");
		}
		else
		{
			cout << "account was in accounts map already" << endl;

			//inform the client of failure
			res.set_content("", "text/plain");
		}
	});

	cout << "Https server listening at https://localhost:1234" << endl;
	server.listen("localhost", 1234);
    return 0;
}

