// GoldenAgeClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define _IRR_STATIC_LIB_
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <C:/Users/cool_/Documents/cpp-httplib/httplib.h>
#include <irrlicht.h>
#include <iostream>
#include <string>

using namespace std;
using namespace httplib;
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

IrrlichtDevice* device = createDevice(EDT_DIRECT3D9, dimension2d<u32>(600, 800), 16, false);
IVideoDriver* driver = device->getVideoDriver();;
IGUIEnvironment* env = device->getGUIEnvironment();

class LoginEventReceiver : public IEventReceiver
{
	IGUIButton* login = env->addButton(rect<s32>(275, 500, 325, 530), 0, 1, L"Login", L"Login");
	IGUIButton* createaccount = env->addButton(rect<s32>(275, 540, 325, 570), 0, 2, L"Make Account", L"Make Account");
	IGUIEditBox* email = env->addEditBox(L"", rect<s32>(225, 330, 375, 390), true, 0, 1);
	IGUIEditBox* password = env->addEditBox(L"", rect<s32>(225, 410, 375, 470), true, 0, 2);
	bool bdrawemailtext = true;
	bool bdrawpasstext = true;
	bool bsendlogin = false;
	bool bsendcreateaccount = false;


public:
	virtual bool OnEvent(const SEvent& event)
	{
		if (event.EventType == EET_GUI_EVENT)
		{
			s32 id = event.GUIEvent.Caller->getID();
			switch (event.GUIEvent.EventType)
			{
			case EGET_EDITBOX_CHANGED:
				if (id == 1)
				{
					if (strcmp((char*)email->getText(), (char*)L"") == 0)
					{
						bdrawemailtext = true;
					}
					else
					{
						bdrawemailtext = false;
					}
				}
				else if (id == 2)
				{
					if (strcmp((char*)password->getText(), (char*)L"") == 0)
					{
						bdrawpasstext = true;
					}
					else
					{
						bdrawpasstext = false;
					}
				}
				break;

			case EGET_BUTTON_CLICKED:
				if (id == 1)
				{
					//login button clicked
					bsendlogin = true;
				}
				else if (id == 2)
				{
					//create account button clicked
					bsendcreateaccount = true;
				}
				break;
			}
		}
		return false;
	}

	IGUIEditBox* getEmail()
	{
		return email;
	}

	IGUIEditBox* getPass()
	{
		return password;
	}

	bool shouldDrawEmailText()
	{
		return bdrawemailtext;
	}

	bool shouldDrawPasswordText()
	{
		return bdrawpasstext;
	}

	bool shouldSendLogin()
	{
		return bsendlogin;
	}

	bool shouldSendCreateAccount()
	{
		return bsendcreateaccount;
	}

	void resetSendLogin()
	{
		bsendlogin = false;
	}

	void resetSendCreateAccount()
	{
		bsendcreateaccount = false;
	}
};

shared_ptr<httplib::Response> makeLoginPost(const char* location, SSLClient& client, LoginEventReceiver& login_event_receiver)
{
	char emailspacepassword[200];
	wcstombs(emailspacepassword, login_event_receiver.getEmail()->getText(), 100);
	int len = strlen(emailspacepassword);
	*(emailspacepassword + len++) = ' ';
	wcstombs(emailspacepassword + len, login_event_receiver.getPass()->getText(), 100);
	return client.Post(location, emailspacepassword, "text/plain");
}

int main()
{
	device->setWindowCaption(L"Golden Age Login");

	if (device == 0)
	{
		cout << "Failed to create device." << endl;
		system("pause");
		return 1; // could not create selected driver.
	}

	if (driver == 0)
	{
		cout << "Failed to create video driver." << endl;
		system("pause");
		return 1;
	}

	if (env == 0)
	{
		cout << "Failed to create gui environment." << endl;
		system("pause");
		return 1;
	}

	{
		ITexture* images = driver->getTexture("./system/resources/textures/loginscreen.png");
		IGUIFont* font = device->getGUIEnvironment()->getBuiltInFont();
		SSLClient client("localhost", 1234, 10);
		LoginEventReceiver login_event_receiver;
		device->setEventReceiver(&login_event_receiver);
		
		while (device->run() && driver)
		{
			//u32 time = device->getTimer()->getTime();
			driver->beginScene(true, true, video::SColor(0, 120, 102, 136));

			//draw bg image
			driver->draw2DImage(images, core::position2d<s32>(0, 0),
				core::rect<s32>(0, 0, 600, 800), 0,
				video::SColor(255, 255, 255, 255), false);

			//draw gui
			env->drawAll();

			//draw 2d text
			// draw email text
			if (login_event_receiver.shouldDrawEmailText())
			{
				font->draw(L"email",
					core::rect<s32>(275, 340, 325, 380),
					video::SColor(255, 0, 0, 0));
			}

			// draw password text
			if (login_event_receiver.shouldDrawPasswordText())
			{
				font->draw(L"password",
					core::rect<s32>(275, 420, 325, 460),
					video::SColor(255, 0, 0, 0));
			}

			if (login_event_receiver.shouldSendLogin())
			{
				auto res = makeLoginPost("/login", client, login_event_receiver);
				cout << "response body: " << res->body << endl;
				if (res->body[0] == '\0')
				{
					cout << "login failure" << endl;
				}
				else {
					cout << "login success" << endl;
				}
				//this body needs to let me send packets at my game server
				login_event_receiver.resetSendLogin();
			}
			
			if (login_event_receiver.shouldSendCreateAccount())
			{
				auto res = makeLoginPost("/createaccount", client, login_event_receiver);
				cout << "response body: " << res->body << endl;
				if (res->body[0] == '\0')
				{
					cout << "create account failure" << endl;
				}
				else
				{
					cout << "create account success" << endl;
				}
				login_event_receiver.resetSendCreateAccount();
			}

			driver->endScene();
		}
	}

	device->drop();
	return 0;
}



















	/*
	video::IVideoDriver* driver = device->getVideoDriver();
	scene::ISceneManager* smgr = device->getSceneManager();

	scene::ITriangleSelector* selector = 0;

	scene::ICameraSceneNode* camera =
		camera = smgr->addCameraSceneNodeFPS(0, 100.0f, 300.0f);
	camera->setPosition(core::vector3df(-100, 50, -150));

	scene::ISceneNodeAnimator* anim =
		smgr->createCollisionResponseAnimator(
			selector, camera, core::vector3df(30, 50, 30),
			core::vector3df(0, -3, 0),
			core::vector3df(0, 50, 0));

	selector->drop();

	camera->addAnimator(anim);
	anim->drop();

	// disable mouse cursor

	//device->getCursorControl()->setVisible(false);

	// add billboard

	scene::IBillboardSceneNode * bill = smgr->addBillboardSceneNode();
	bill->setMaterialType(video::EMT_TRANSPARENT_ADD_COLOR);
	bill->setMaterialTexture(0, driver->getTexture(
		"../../media/particle.bmp"));
	bill->setMaterialFlag(video::EMF_LIGHTING, false);
	bill->setSize(core::dimension2d<f32>(20.0f, 20.0f));

	// add 3 animated faeries.

	video::SMaterial material;
	material.setTexture(1, driver->getTexture(
		"../../media/faerie2.bmp"));
	material.Lighting = true;

	scene::IAnimatedMeshSceneNode* node = 0;
	scene::IAnimatedMesh* faerie = smgr->getMesh(
		"../../media/faerie.md2");

	if (faerie)
	{
		node = smgr->addAnimatedMeshSceneNode(faerie);
		node->setPosition(core::vector3df(-70, 0, -90));
		node->setMD2Animation(scene::EMAT_RUN);
		node->getMaterial(0) = material;

		node = smgr->addAnimatedMeshSceneNode(faerie);
		node->setPosition(core::vector3df(-70, 0, -30));
		node->setMD2Animation(scene::EMAT_SALUTE);
		node->getMaterial(0) = material;

		node = smgr->addAnimatedMeshSceneNode(faerie);
		node->setPosition(core::vector3df(-70, 0, -60));
		node->setMD2Animation(scene::EMAT_JUMP);
		node->getMaterial(0) = material;
	}

	material.setTexture(1, 0);
	material.Lighting = false;

	// Add a light

	smgr->addLightSceneNode(0, core::vector3df(-60, 100, 400),
		video::SColorf(1.0f, 1.0f, 1.0f, 1.0f),
		600.0f);*/


