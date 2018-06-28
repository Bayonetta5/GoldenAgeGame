// GoldenAgeGameServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
//winsock server only! hooray!
#include "./winsockwrapper.hpp" //why can't I put this in stdafx.h?
#include "../GoldenAgeLoginServer/Debug.hpp"
#include "../GoldenAgeLoginServer/cryptinfo.hpp"

using namespace std;

struct account {
	//tons of shit
};

struct client {
	cryptinfo ci;
	account acc;
	bool awaitinghello = false;
};

unordered_map<string, client> clientsmap;

void serverLoop()
{
	debug("entering server loop");
	initENet();
	ENetAddress address;
	ENetHost* server = makeServer(address, 3001);
	while (true)
	{
		ENetEvent event;
		/* Wait up to 1000 milliseconds for an event. */
		while (enet_host_service(server, &event, 0) > 0)
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
	
	debug("leaving server loop");
}

int main()
{
	serverLoop();
	debug("exiting");
	system("pause");
	return 0;
}