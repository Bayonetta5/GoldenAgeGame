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

#ifdef WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

#define SERVER_CERT_FILE "./cert.pem"
#define SERVER_PRIVATE_KEY_FILE "./key.pem"
#define ACCOUNTMANIFESTLOCATION "./accountmanifest.txt"

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
	ifstream accountmanifest("./accountmanifest.txt");
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
	manifest << email;
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
			cout << newaccount.password << " == " << accounts.at(newaccount.email.c_str()) << endl;
			if (newaccount.password == accounts.at(newaccount.email.c_str()))
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

