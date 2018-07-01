#include "stdafx.h"
#include <GoldenAge/Debug.h>
#include <GoldenAge/cryptinfo.h>
#include <GoldenAge/udp_com.h>

#define MAXCLIENTS 1000
#define MAXCHANNELS 2


struct account {
	//tons of shit
};

struct client {
	cryptinfo ci;
	account acc;
	bool awaitinghello = false;
};

unsigned int runloop = 1;
std::unordered_map<std::string, client> clientsmap;

int main()
{
	ga::udp_com com(MAXCLIENTS, MAXCHANNELS, 3001, true);

	com.register_receive([](ENetEvent& e) {
		debug("My receive handler");
	});

	com.register_connect([](ENetPeer* e) {
		debug("My connect handler");
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