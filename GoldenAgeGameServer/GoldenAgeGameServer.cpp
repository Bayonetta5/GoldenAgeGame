#include "stdafx.h"
#include <GoldenAge/Debug.h>
#include <GoldenAge/cryptinfo.h>
#include <GoldenAge/winsockwrapper.h>

struct account {
	//tons of shit
};

struct client {
	cryptinfo ci;
	account acc;
	bool awaitinghello = false;
};

std::unordered_map<std::string, client> clientsmap;

void serverLoop()
{
	debug("init data for server loop");
	initENet();
	ENetAddress address;
	ENetHost* server = makeServer(address, 3001, 1000, 2);
	debug("initializing data for server loop good");
	debug("entering server loop");
	while (true)
	{
		ENetEvent event;
		while (enet_host_service(server, &event, 0) > 0)
		{
			switch (event.type)
			{
			case _ENetEventType::ENET_EVENT_TYPE_CONNECT:
				debug("A new client connected from ", event.peer->address.host, ":", event.peer->address.port);
				/* Store any relevant client information here. */
				event.peer->data = (void*)"Client information";
				break;
			case _ENetEventType::ENET_EVENT_TYPE_RECEIVE:
				debug("A packet of length ", event.packet->dataLength, " containing ", event.packet->data, " was received from ", event.peer->data, " on channel ", event.peer->data, ".\n");
				debug("handling packet");

				debug("packet handled");

				debug("destroying packet");
				enet_packet_destroy(event.packet);
				debug("destroying packet good");
				break;

			case _ENetEventType::ENET_EVENT_TYPE_DISCONNECT:
				debug(event.peer->data, " disconnected");
				debug("null their data");
				event.peer->data = NULL;
			}
		}
	}
	debug("leaving server loop");
}

int main()
{
	serverLoop();
	debug("exiting");
	system("pause");
	return 0;
}