#include "stdafx.h"
#include "LoginEventReceiver.h"
#include "CharacterSelectionEventReceiver.h"
#include "login_screen_loop.h"
#include <GoldenAge/debug.h>
#include <GoldenAge/udp_com.h>
#include <GoldenAge/cryptinfo.h>
#include <GoldenAge/toongraphics.h>
#include <GoldenAge/array_packet.h>
#include <GoldenAge/secretkey.h>
#include <GoldenAge/toon.h>

#define MAXCLIENTS 1
#define MAXCHANNELS 2

std::string selected_account;
ga::cryptinfo ci;
ga::secretkey sk;
irr::IrrlichtDevice* device = irr::createDevice(irr::video::E_DRIVER_TYPE::EDT_DIRECT3D9, irr::core::dimension2d<irr::u32>(800, 900), 16, false);
irr::video::IVideoDriver* driver = device->getVideoDriver();
irr::gui::IGUIEnvironment* env = device->getGUIEnvironment();
irr::video::ITexture* bg = driver->getTexture("./system/resources/textures/kewlbg.jpg");
irr::core::dimension2du screen_size = driver->getScreenSize();
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

	std::vector<ga::toon> mytoons;
	ga::CharacterSelectEventReceiver receiver;
	ga::udp_com com(MAXCLIENTS, MAXCHANNELS);
	com.connect("localhost", 3001, 5000);

	com.register_receive([&mytoons, &receiver](ENetEvent& e) {
		debug("My receive handler");
		ga::array_packet packet;
		packet.from_buf((const char*)e.packet->data, e.packet->dataLength);
		std::string type = packet.get();
		if (type == "f")
		{
			debug("received all my toongraphics for character select");
			std::string size = packet.get();
			int s = atoi(size.c_str());
			for (int i = 0; i < s; ++i)
			{
				std::string name = packet.get();
				ga::toon t(name);
				std::string tgraph = packet.get();
				t.tg.from_string(tgraph);
				mytoons.push_back(t);
			}
			receiver = ga::CharacterSelectEventReceiver(mytoons);
			device->setEventReceiver(&receiver);
		}
		else
		{
			debug("received unkown packet type");
		}

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
		device->run();
		driver->beginScene(true, true, irr::video::SColor(0, 120, 102, 136));
		//draw the scene (a background with the toon over it, toon selector, toon select button, options button, appropriate texts)
		driver->draw2DImage(bg, irr::core::position2d<irr::s32>(0, 0),
			irr::core::rect<irr::s32>(0, 0, 800, 900), 0,
			irr::video::SColor(255, 255, 255, 255), false);

		env->drawAll();

		driver->endScene();
	});
	
	{
		ga::loginScreenLoop(bg, runloop);
	}


	//full screen mode!
	debug("trying to go full screen");
	//device->maximizeWindow();

	
	//start the game loop
	irr::u32 millis = device->getTimer()->getTime();
	//get the screen size data
	screen_size = driver->getScreenSize();

	std::string type("a");
	ga::array_packet packet(
		ga::array_packet::get_args_size(
			type,
			selected_account,
			sk.to_string()
		));
	packet.fill(type, selected_account, sk.to_string());
	Sleep(75);
	com.send(com.getPeer(), packet(), packet.size(), ENET_PACKET_FLAG_RELIABLE);
	runloop = true;
	com.listenLoop(runloop, millis);

	debug("exiting program");
	system("pause");
	return 0;
}