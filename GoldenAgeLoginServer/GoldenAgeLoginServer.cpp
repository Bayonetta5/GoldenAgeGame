// GoldenAge.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "../GoldenAgeGameServer/winsockwrapper.hpp"
#include "./cryptinfo.hpp"
#include <enet/enet.h>
#include <enet/win32.h>

using namespace std;

struct gameserver {
	string host;
	unsigned int port;

	gameserver() {}

	gameserver(string host, unsigned int port)
	{
		this->host = host;
		this->port = port;
	}

private:
	friend std::ostream& operator<<(std::ostream &os, const  gameserver  &e);
	friend std::istream& operator>>(std::istream &os, gameserver  &e);
};

std::ostream& operator<<(std::ostream &os, const  gameserver  &e)
{
	os << e.host << "\n";
	os << e.port;
	return os;
}

std::istream& operator>>(std::istream &os, gameserver  &e)
{
	os >> e.host;
	os >> e.port;
	return os;
}

struct sessiondata {
	string email;
	string password;
	string gameservername;
	cryptinfo ci;
	httplib::Response* res;

	bool loggedin = false;
	bool awaitinggamelogin = false;

	void fillinsd(string email, string password, string gameservername)
	{
		this->email = email;
		this->password = password;
		this->gameservername = gameservername;
	}
};

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
	debug("initializing game server map");
	ifstream gameservermanifest(GAMESERVERMANIFESTLOCATION);
	for (string line = ""; getline(gameservermanifest, line);)
	{
		debug("opening ", "./GameServers/" + line + ".txt");
		ifstream gs("./GameServers/" + line + ".txt");
		gameserver newgs;
		gs >> newgs;
		debug(newgs);
		gameservermap.insert(make_pair(line, newgs));
		gs.close();
	}
	gameservermanifest.close();
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
	string encryptedpassword;
	ci.encrypt(data.password, encryptedpassword);
	debug("writing password [",
		encryptedpassword,
		"] to file ./Accounts/",
		data.email,
		".txt");
	ofstream accountfile("./Accounts/" + data.email + ".txt");
	
	accountfile << encryptedpassword << "\n";
	accountfile << data.gameservername << "\n";

	data.password = encryptedpassword;

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

void serverLoop(ENetHost* client)
{
	while (true)
	{
		ENetEvent event;
		/* Wait up to 0 milliseconds for an event. */
		while (enet_host_service(client, &event, 0) > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				debug("A new client connected from %x:%u.\n",
					event.peer->address.host,
					event.peer->address.port);
				/* Store any relevant client information here. */
				event.peer->data = (void*)"Client information";
				break;
			case ENET_EVENT_TYPE_RECEIVE:
				debug("A packet of length %u containing %s was received from %s on channel %u.\n",
					event.packet->dataLength,
					event.packet->data,
					event.peer->data,
					event.channelID);
				/* Clean up the packet now that we're done using it. */
				enet_packet_destroy(event.packet);

				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				debug("%s disconnected.\n", event.peer->data);
				/* Reset the peer's client information. */
				event.peer->data = NULL;
			}
		}
	}
}

int main()
{
	debug("size of cryptinfo ", sizeof(cryptinfo));
	ERR_load_crypto_strings();

	initAccountsMap();
	initGameServerMap();
	
	initENet();
	ENetEvent event;
	ENetHost* client = makeClient();
	ENetAddress address;
	ENetPeer *peer;
	enet_address_set_host(&address, "localhost");
	address.port = 3001;
	/* Initiate the connection, allocating the two channels 0 and 1. */
	peer = enet_host_connect(client, &address, 2, 0);
	if (peer == NULL)
	{
		debug("No available peers for initiating an ENet connection.\n");
		system("pause");
		exit(EXIT_FAILURE);
	}
	/* Wait up to 5 seconds for the connection attempt to succeed. */
	if (enet_host_service(client, &event, 5000) > 0 &&
		event.type == ENET_EVENT_TYPE_CONNECT)
	{
		debug("Connection to some.server.net:1234 succeeded.");
	}
	else
	{
		/* Either the 5 seconds are up or a disconnect event was */
		/* received. Reset the peer in the event the 5 seconds   */
		/* had run out without any significant event.            */
		enet_peer_reset(peer);
		debug("Connection to localhost:1234 failed.");
	}
	debug("setting up password cryptinfo");
	cryptinfo passci;
	readInPassCryptInfo(passci);


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
	server.Post("/login", [&passci, client](const httplib::Request& req, httplib::Response& res) {
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
			string decryptedpass;
			passci.decrypt(sessd.password, decryptedpass);
			if ((sessdata.password == decryptedpass) && (!sessd.loggedin))
			{
				sessd.loggedin = true;
				debug("password matched");
				cryptinfo ci;
				ci.fillkeyiv();
				debug("filled crypt info ", ci);
				char buffer[70];
				int i = 0;
				buffer[i] = 'c'; // c - client login
				++i;
				(buffer + i) << ci;
				i += 48;
				for (int j = 0; j < sessd.email.size(); ++i, ++j)
				{
					buffer[i] = sessd.email[j];
				}
				buffer[i] = '\0';
				
				gameserver& gs = gameservermap.at(sessd.gameservername);
				sessd.awaitinggamelogin = true;
				//loop and read the socket right here, these posts should spawn a thread that does dis
				//client is awaiting res success
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

	debug("login server listening at localhost:3002");
	serverLoop(client);

	loginserverthread.join();
	
	debug("program exiting\n");
	system("pause");
    return 0;
}

//make it forkable!