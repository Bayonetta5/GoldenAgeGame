// GoldenAgeClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "../GoldenAgeGameServer/winsockwrapper.hpp"
#include "../GoldenAgeLoginServer/Debug.hpp"
#include "../GoldenAgeLoginServer/cryptinfo.hpp"
#include <enet/enet.h>
#include <enet/win32.h>

using namespace std;
using namespace httplib;
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

IrrlichtDevice* device = createDevice(EDT_DIRECT3D9, dimension2d<u32>(800, 900), 16, false);
IVideoDriver* driver = device->getVideoDriver();;
IGUIEnvironment* env = device->getGUIEnvironment();
u32 millis = device->getTimer()->getTime();

class LoginEventReceiver : public IEventReceiver
{
	IGUIStaticText* statusmsg = env->addStaticText(L"Hello!", rect<s32>(150, 290, 650, 320), false, false, 0, -1, false);
	IGUIListBox* gameserver = env->addListBox(rect<s32>(300, 210, 500, 290), 0, 1);
	IGUIEditBox* email = env->addEditBox(L"", rect<s32>(300, 320, 500, 390), true, 0, 1);
	IGUIEditBox* password = env->addEditBox(L"", rect<s32>(300, 410, 500, 480), true, 0, 2);
	IGUIButton* login = env->addButton(rect<s32>(350, 500, 450, 530), 0, 1, L"Login", L"Login");
	IGUIButton* createaccount = env->addButton(rect<s32>(350, 540, 450, 570), 0, 2, L"Make Account", L"Make Account");
	unordered_map<u32, std::string> gsitemmap;
	SSLClient* client;
	vector<thread> threads;

public:
	LoginEventReceiver(SSLClient& client)
	{
		this->client = &client;
		statusmsg->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
		email->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
		password->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
	}

	~LoginEventReceiver()
	{
		jointhreads();
	}

	virtual bool OnEvent(const SEvent& event)
	{
		//gui events
		if (event.EventType == EET_GUI_EVENT)
		{
			//get id of gui element
			s32 id = event.GUIEvent.Caller->getID();
			if (event.GUIEvent.EventType == EGET_EDITBOX_CHANGED)
			{
				if (id == 1)
				{
					unsigned int lenemail = lstrlenW(email->getText());
					if (lenemail == 17)
					{
						debug("email string too long");
						wchar_t textminusone[17];
						memcpy(textminusone, email->getText(), sizeof(textminusone) - 2);
						textminusone[16] = L'\0';
						email->setText(textminusone);
					}
				}
				else if (id == 2)
				{
					unsigned int lenpass = lstrlenW(password->getText());
					if (lenpass == 17)
					{
						debug("password string too long");
						wchar_t textminusone[17];
						memcpy(textminusone, password->getText(), sizeof(textminusone) - 2);
						textminusone[16] = L'\0';
						password->setText(textminusone);
					}
				}
			}
			else if (event.GUIEvent.EventType == EGET_BUTTON_CLICKED)
			{
				if (id == 1)
				{
					sendLoginPost();
				}
				else if (id == 2)
				{
					sendCreateAccountPost();
					sendLoginPost();
				}
			}
		}
		return false;
	}

	std::string getGameServerStringFromSelected()
	{
		return gsitemmap.at(gameserver->getSelected());
	}

	void setupGameServerMenu()
	{
		//read them from game server.ini
		debug("opening file ", GAMESERVERINILOCATION);
		ifstream gameserverini(GAMESERVERINILOCATION);
		bool firsttime = true;
		for (std::string line; getline(gameserverini, line);)
		{
			debug(line);
			wchar_t buf[LOGINBUFFERMAX];
			mbstowcs(buf, line.c_str(), line.size());
			wcout << buf << endl;
			if (firsttime)
			{
				gsitemmap.insert(make_pair(gameserver->addItem(buf), line));
				//set focus
				gameserver->setSelected(buf);
				firsttime = false;
			}
			else
			{
				gsitemmap.insert(make_pair(gameserver->addItem(buf), line));
			}
		}
	}

	bool makeLoginPost(const char* location, shared_ptr<httplib::Response>& res)
	{
		bool isinvalid = false;
		char emailspacepassword[LOGINBUFFERMAX + LOGINBUFFERMAX + LOGINBUFFERMAX + 3];
		wcstombs(emailspacepassword, email->getText(), LOGINBUFFERMAX);
		debug(emailspacepassword, " logging in");
		int len = strlen(emailspacepassword);
		//validate input (alpha numeric or digit)
		if (len > 17)
		{
			debug("email too long");
			isinvalid = true;
		}
		for (int i = 0; i < len; ++i)
		{
			if (!irr::core::isdigit(emailspacepassword[i]) && !isalpha(emailspacepassword[i]))
			{
				debug("email input was invalid");
				isinvalid = true;
			}
		}
		*(emailspacepassword + len++) = ' ';
		wcstombs(emailspacepassword + len, password->getText(), LOGINBUFFERMAX);
		int oldlen = len;
		len = strlen(emailspacepassword);
		//validate input (alpha numeric or digit)
		if ((len - oldlen) > 17)
		{
			debug("password too long");
			isinvalid = true;
		}
		for (int i = oldlen; i < len; ++i)
		{
			if (!irr::core::isdigit(emailspacepassword[i]) && !isalpha(emailspacepassword[i]))
			{
				debug("password input was invalid");
				isinvalid = true;
			}
		}
		bool hasdif = false;
		for (int i = oldlen+1; i < len; ++i)
		{
			if (emailspacepassword[i] != emailspacepassword[i - 1])
			{
				hasdif = true;
				break;
			}
		}
		if (!hasdif)
		{
			isinvalid = true;
		}
		if (isinvalid)
		{
			debug("Account and password must be 16 or less characters, password two different characters, and only digits or letters");
			statusmsg->setText(L"Account and password must be 16 or less characters, password two different characters, and only digits or letters");
			resetStatusMessageTimeout();
			return false;
		}
		*(emailspacepassword + len++) = ' ';
		std::string str = getGameServerStringFromSelected().c_str();
		memcpy(emailspacepassword + len, str.c_str(), str.size() + 1);
		res = client->Post(location, emailspacepassword, "text/plain");
		return true;
	}

	void sendLoginPost()
	{
		shared_ptr<httplib::Response> res;
		if (makeLoginPost("/login", res))
		{
			debug("response body: ", res->body);
			if (res->body[0] == '\0')
			{
				debug("login failure");
				statusmsg->setText(L"Login failure!");
				resetStatusMessageTimeout();
			}
			else {
				debug("login success");
				statusmsg->setText(L"Login Success!");
			}
			//this body needs to let me send packets at my game server
		}
	}

	void sendCreateAccountPost()
	{
		shared_ptr<httplib::Response> res;
		if (makeLoginPost("/createaccount", res)) //response isn't filled on failure, so don't derefrence it or anything!
		{
			debug("response body: ", res->body);

			if (res->body[0] == '\0')
			{
				debug("create account failure");
				statusmsg->setText(L"Create Account Failure!");
				resetStatusMessageTimeout();
			}
			else
			{
				debug("create account success");
				statusmsg->setText(L"Create Account Success!");
				resetStatusMessageTimeout();
			}
		}
	}

	void resetStatusMessageTimeout()
	{
		threads.push_back(thread([this]() {
			Sleep(5000);
			statusmsg->setText(L"");
		}));
	}

	void jointhreads()
	{
		for (thread& t : threads)
		{
			t.join();
		}
	}
};

void loginScreenLoop()
{
	//ITexture* bg = driver->getTexture("./system/resources/textures/loginscreen.png");
	ITexture* bg = driver->getTexture("./system/resources/textures/kewlbg.jpg");
	ITexture* title = driver->getTexture("./system/resources/textures/GoldenAgeTitle.png");
	IGUIFont* font = device->getGUIEnvironment()->getBuiltInFont();
	SSLClient client("localhost", 1234, 10);
	LoginEventReceiver receiver(client);
	device->setEventReceiver(&receiver);
	receiver.setupGameServerMenu();
	receiver.resetStatusMessageTimeout();
	while (device->run() && driver)
	{
		driver->beginScene(true, true, video::SColor(0, 120, 102, 136));

		driver->draw2DImage(bg, core::position2d<s32>(0, 0),
			rect<s32>(0, 0, 800, 900), 0,
			SColor(255, 255, 255, 255), false);

		driver->draw2DImage(title, core::position2d<s32>(50, 10),
			rect<s32>(0, 0, 700, 230), 0,
			SColor(255, 255, 255, 255), true);

		env->drawAll();

		font->draw(L"email",
			rect<s32>(350, 330, 450, 370),
			SColor(255, 0, 0, 0), true);

		font->draw(L"password",
			rect<s32>(350, 420, 450, 460),
			SColor(255, 0, 0, 0), true);

		driver->endScene();
	}
}

void gameLoop()
{
	initENet();
	ENetHost* client = makeClient();
	while (true)
	{
		ENetEvent event;
		/* Wait up to 1000 milliseconds for an event. */
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
	ERR_load_crypto_strings();
	device->setWindowCaption(L"Golden Age Login");

	if (device == 0)
	{
		debug("Failed to create device.");
		system("pause");
		return 1; // could not create selected driver.
	}

	if (driver == 0)
	{
		debug("Failed to create video driver.");
		system("pause");
		return 1;
	}

	if (env == 0)
	{
		debug("Failed to create gui environment.");
		system("pause");
		return 1;
	}

	//ensure creation of a stackframe, we don't want this stuff cloggin ram
	//when it's time to run the gameloop just because we opted for all the compiler
	//speed options over size
	{
		loginScreenLoop();
	}

	debug("client listening at localhost:3003");
	gameLoop();

	debug("exiting program\n\n\t ");
	system("pause");
	return 0;
}