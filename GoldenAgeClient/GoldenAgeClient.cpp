#include "stdafx.h"
#include "LoginEventReceiver.h"
#include "clientloops.h"
#include <GoldenAge/Debug.h>
#include <GoldenAge/udp_com.h>
#include <GoldenAge/cryptinfo.h>

#define MAXCLIENTS 1
#define MAXCHANNELS 2

irr::IrrlichtDevice* device = irr::createDevice(irr::video::E_DRIVER_TYPE::EDT_DIRECT3D9, irr::core::dimension2d<irr::u32>(800, 900), 16, false);
irr::video::IVideoDriver* driver = device->getVideoDriver();;
irr::gui::IGUIEnvironment* env = device->getGUIEnvironment();
cryptinfo ci;
unsigned int runloop = 1;

int main()
{
	debug("load crypto strings");
	ERR_load_crypto_strings();
	debug("set window caption to ");
	device->setWindowCaption(L"Golden Age Login");

	debug("checking device");
	if (device == 0)
	{
		debug("Failed to create device.");
		system("pause");
		return 1; // could not create selected driver.
	}

	debug("checking driver");
	if (driver == 0)
	{
		debug("Failed to create video driver.");
		system("pause");
		return 1;
	}

	debug("checking environment");
	if (env == 0)
	{
		debug("Failed to create gui environment.");
		system("pause");
		return 1;
	}

	ga::udp_com com(MAXCLIENTS, MAXCHANNELS);

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
		unsigned int current_time = device->getTimer()->getTime();
		return current_time - last_time;
	});

	com.register_tick([](unsigned int delta_time) {
		//update toons
		driver->beginScene(true, true, irr::video::SColor(0, 120, 102, 136));
		//draw the scene
		driver->endScene();
	});
	
	{
		ga::loginScreenLoop(runloop);
	}


	Sleep(500); //give the game server time to know how to respond
	com.connect("localhost", 3001, 5000);
	irr::u32 millis = device->getTimer()->getTime();
	com.listenLoop(runloop, millis);

	debug("exiting program");
	system("pause");
	return 0;
}