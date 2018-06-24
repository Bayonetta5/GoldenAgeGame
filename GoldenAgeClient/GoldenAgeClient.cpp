// GoldenAgeClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define _IRR_STATIC_LIB_
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <C:/Users/cool_/Documents/cpp-httplib/httplib.h>
#include <irrlicht.h>
#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;
using namespace httplib;
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

class LoginEventReceiver;

typedef void(*lpvoidfunc)();
typedef void(*lplerfunc)(const LoginEventReceiver* receiver);

IrrlichtDevice* device = createDevice(EDT_DIRECT3D9, dimension2d<u32>(600, 800), 16, false);
IVideoDriver* driver = device->getVideoDriver();;
IGUIEnvironment* env = device->getGUIEnvironment();
u32 millis = device->getTimer()->getTime();
u32 granularEventTimer = millis + 128;

struct login_event {
	lplerfunc e;
	u32 timer;
	unsigned int index;
	bool hasexecuted = false;

	login_event(lplerfunc e, unsigned int index, int timer)
	{
		this->index = index;
		this->e = e;
		this->timer = millis + timer;
		cout << "function " << e << " will execute in " << timer << " milliseconds" << endl;
	}

	bool shouldExecute(const LoginEventReceiver* receiver)
	{
		if (!hasexecuted)
		{
			if (timer <= granularEventTimer)
			{
				cout << "executing event " << e << endl;
				hasexecuted = true;
				e(receiver);
				return true;
			}
			else
			{
				cout << "not executing event " << e << endl;
				return false;
			}
		}
	}
};

class LoginEventReceiver : public IEventReceiver
{
	IGUIButton* login = env->addButton(rect<s32>(275, 500, 325, 530), 0, 1, L"Login", L"Login");
	IGUIButton* createaccount = env->addButton(rect<s32>(275, 540, 325, 570), 0, 2, L"Make Account", L"Make Account");
	IGUIEditBox* email = env->addEditBox(L"", rect<s32>(225, 330, 375, 390), true, 0, 1);
	IGUIEditBox* password = env->addEditBox(L"", rect<s32>(225, 410, 375, 470), true, 0, 2);

	unordered_map<int, login_event> receiverevents;
	u32 receivereventid = 0;

	bool bdrawemailtext = true;
	bool bdrawpasstext = true;
	bool bsendlogin = false;
	bool bsendcreateaccount = false;
	bool bshowstatus = true;

public:
	IGUIStaticText* statusmsg = env->addStaticText(L"Hello!", rect<s32>(275, 300, 325, 330), false, false, 0, -1, false);

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

	void tickReceiverEvents(u32 granularEventTimer)
	{
		cout << "ticking receiver events" << endl;
		for (unsigned int i = 0; i < receivereventid; ++i)
		{
			login_event& e = receiverevents.at(i);
			if (e.shouldExecute(this))
			{
				cout << "skipping erasing the event" << endl;
			}
		}
		cout << "done ticking receiver events" << endl;
	}

	void startLoginEvent(lplerfunc func, int timer)
	{
		receiverevents.insert(make_pair(receivereventid, login_event(func, receivereventid, timer)));
		++receivereventid;
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

void resetStatusTimeout(LoginEventReceiver& receiver)
{
	receiver.startLoginEvent([](const LoginEventReceiver* rec) { 
		cout << "reset status timeout called" << endl; 
		rec->statusmsg->setText(L"");
	}, 5000);
}

shared_ptr<httplib::Response> makeLoginPost(const char* location, SSLClient& client, LoginEventReceiver& receiver)
{
	char emailspacepassword[200];
	wcstombs(emailspacepassword, receiver.getEmail()->getText(), 100);
	int len = strlen(emailspacepassword);
	*(emailspacepassword + len++) = ' ';
	wcstombs(emailspacepassword + len, receiver.getPass()->getText(), 100);
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
		LoginEventReceiver receiver;
		device->setEventReceiver(&receiver);

		resetStatusTimeout(receiver);
		cout << "successful reset status timeout" << endl;
		while (device->run() && driver)
		{
			//cout << "looping" << endl;
			millis = device->getTimer()->getTime();
			if (granularEventTimer <= millis) //granular event timer has already ticked on iteration
			{
				cout << "GRANULAR EVENT TIMER TICKED" << endl;
				granularEventTimer = millis + 1024;
				receiver.tickReceiverEvents(granularEventTimer);
			}

			driver->beginScene(true, true, video::SColor(0, 120, 102, 136));

			//draw bg image
			driver->draw2DImage(images, core::position2d<s32>(0, 0),
				core::rect<s32>(0, 0, 600, 800), 0,
				video::SColor(255, 255, 255, 255), false);

			//draw gui
			env->drawAll();

			//draw 2d text
			// draw email text
			if (receiver.shouldDrawEmailText())
			{
				font->draw(L"email",
					core::rect<s32>(275, 340, 325, 380),
					video::SColor(255, 0, 0, 0));
			}

			// draw password text
			if (receiver.shouldDrawPasswordText())
			{
				font->draw(L"password",
					core::rect<s32>(275, 420, 325, 460),
					video::SColor(255, 0, 0, 0));
			}

			if (receiver.shouldSendLogin())
			{
				auto res = makeLoginPost("/login", client, receiver);
				cout << "response body: " << res->body << endl;
				if (res->body[0] == '\0')
				{
					cout << "login failure" << endl;
					receiver.statusmsg->setText(L"Login failure!");
					resetStatusTimeout(receiver);
				}
				else {
					cout << "login success" << endl;
					receiver.statusmsg->setText(L"Login Success!");
					resetStatusTimeout(receiver);
				}
				//this body needs to let me send packets at my game server
				receiver.resetSendLogin();
			}
			
			if (receiver.shouldSendCreateAccount())
			{
				auto res = makeLoginPost("/createaccount", client, receiver);
				cout << "response body: " << res->body << endl;
				if (res->body[0] == '\0')
				{
					cout << "create account failure" << endl;
					receiver.statusmsg->setText(L"Create Account Failure!");
					resetStatusTimeout(receiver);
				}
				else
				{
					receiver.statusmsg->setText(L"Create Account Success!");
					resetStatusTimeout(receiver);
				}
				receiver.resetSendCreateAccount();
			}

			driver->endScene();
		}
	}

	cout << "exiting program\n\n\t ";
	system("pause");
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


