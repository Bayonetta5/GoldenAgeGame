#include "stdafx.h"
#include "LoginEventReceiver.h"
#include "clientloops.h"
#include <GoldenAge/Debug.h>
#include <GoldenAge/winsockwrapper.h>
#include <GoldenAge/cryptinfo.h>

irr::IrrlichtDevice* device = irr::createDevice(irr::video::E_DRIVER_TYPE::EDT_DIRECT3D9, irr::core::dimension2d<irr::u32>(800, 900), 16, false);
irr::video::IVideoDriver* driver = device->getVideoDriver();;
irr::gui::IGUIEnvironment* env = device->getGUIEnvironment();
irr::u32 millis = device->getTimer()->getTime();

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

	{
		ga::loginScreenLoop();
	}

	ga::gameLoop();

	debug("exiting program");
	system("pause");
	return 0;
}