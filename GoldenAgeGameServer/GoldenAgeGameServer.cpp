#include "stdafx.h"
#include <GoldenAge/debug.h>
#include <GoldenAge/cryptinfo.h>
#include <GoldenAge/udp_com.h>
#include <GoldenAge/toongraphics.h>
#include <GoldenAge/array_packet.h>
#include <GoldenAge/secretkey.h>

#define MAXCLIENTS 1000
#define MAXCHANNELS 2

struct toon {
	std::string name;
	ga::toongraphics tg;

	toon(std::string& name)
	{
		this->name = name;
	}
};

struct account {
	std::vector<toon> toons;
	ENetAddress address;
	std::string name;

	account() {}

	account(ENetAddress& address, std::string name)
	{
		this->name = name;
		this->address = address;
		setupAccountToons();
	}

	void setupAccountToons()
	{
		std::ifstream intoons("./Accounts/" + name + ".txt");
		std::string line = " ";
		while (std::getline(intoons, line)) {
			toon loadedtoon(line);
			intoons >> loadedtoon.tg;
			toons.push_back(loadedtoon);
		}
		intoons.close();
	}
};

struct client {
	ga::cryptinfo ci;
	account acc;
	ga::secretkey secret_key;
	bool authed = false;

	client(ENetAddress& address, std::string name)
	{
		this->acc = account(address, name);
	}
};

unsigned int runloop = 1;
std::unordered_map<unsigned int, client> clientsmap;

int main()
{
	ga::udp_com com(MAXCLIENTS, MAXCHANNELS, 3001, true);

	com.register_receive([](ENetEvent& e) {
		debug("My receive handler");
		if (e.packet->data[0] == 'c')
		{
			debug("received a login preparation packet");
			std::string host_port_string = (char*)(e.packet->data + 49);
			ENetAddress address;
			int index = host_port_string.find_first_of(':', 0);
			enet_address_set_host(&address, host_port_string.substr(0, index - 1).c_str());
			address.port = std::atoi(host_port_string.substr(index + 1).c_str());
			client cli(address, (char*)(e.packet->data + 49 + host_port_string.size()));
			(char*)(e.packet->data + 1) >> cli.ci;
			debug("got name, ci, and address");
			debug("making an entry in the clientsmap");
			clientsmap.insert(std::make_pair(cli.acc.address.host, cli));
		}
		else if (e.packet->data[0] == 'a')
		{
			debug("receive a login request from client, checking their ip and seeing if they are awaiting login, and checking their secret login key");
			if (clientsmap.find(e.peer->address.host) != clientsmap.end())
			{
				debug("their ip didn't change, checking await status");
				client& cli = clientsmap.at(e.peer->address.host);
				if (!cli.authed)
				{
					debug("they were awaiting, finally checking the secret key");
					std::string secret_key;
					secret_key = (const char*)(e.packet->data + 1);
					if (strcmp((const char*)cli.secret_key.buffer, secret_key.c_str()) == 0)
					{
						debug("authorization successful, hope it's secure lol");
						cli.authed = true;
						debug("telling the client");

					}
					else
					{
						debug("they gave the wrong secret key, what's going on?");
					}
				}
				else
				{
					debug("they were not awaiting a login, what is going on?");
				}
			}
			else
			{
				debug("the login server hasn't even told me about this dude");
			}
		}
		else
		{
			debug("received unkown packet type");
		}
	});

	com.register_connect([](ENetPeer* peer) {
		debug("My connect handler");
		//check if their ip is the same as last time
		client cli();
				
	});

	com.register_disconnect([](ENetPeer* e) {
		debug("My disconnect handler");
	});

	com.register_delta([](unsigned int last_time)->unsigned int {
		unsigned int current_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		return current_time - last_time;
	});

	com.register_tick([](unsigned int delta_time) {
		//update toons
		//do tasks
	});

	unsigned int current_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	com.listenLoop(runloop, current_time);
	debug("exiting");
	system("pause");
	return 0;
}