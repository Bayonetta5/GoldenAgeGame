#pragma once
#include "stdafx.h"
#include "Sleeper.h"
#include "CharacterSelectionEventReceiver.h"
#include <GoldenAge/debug.h>
#include <GoldenAge/cryptinfo.h>
#include <GoldenAge/array_packet.h>
#include <GoldenAge/wcstrtostdstr.h>
#include <GoldenAge/toongraphics.h>
#include <GoldenAge/secretkey.h>

extern irr::gui::IGUIEnvironment* env;
extern irr::IrrlichtDevice* device;
extern ga::cryptinfo ci;
extern ga::secretkey sk;
extern std::string selected_account;

namespace ga {
	class LoginEventReceiver : public irr::IEventReceiver
	{
		irr::gui::IGUIStaticText* statusmsg = env->addStaticText(L"Hello!", irr::core::rect<irr::s32>(150, 290, 650, 320), false, false, 0, -1, false);
		irr::gui::IGUIListBox* gameserver = env->addListBox(irr::core::rect<irr::s32>(300, 210, 500, 290), 0, 1);
		irr::gui::IGUIEditBox* email = env->addEditBox(L"", irr::core::rect<irr::s32>(300, 320, 500, 390), true, 0, 1);
		irr::gui::IGUIEditBox* password = env->addEditBox(L"", irr::core::rect<irr::s32>(300, 410, 500, 480), true, 0, 2);
		irr::gui::IGUIButton* login = env->addButton(irr::core::rect<irr::s32>(350, 500, 450, 530), 0, 1, L"Login", L"Login");
		irr::gui::IGUIButton* createaccount = env->addButton(irr::core::rect<irr::s32>(350, 540, 450, 570), 0, 2, L"Make Account", L"Make Account");
		std::unordered_map<irr::u32, std::string> gsitemmap;
		httplib::SSLClient* client;
		std::vector<std::thread> threads;
		std::vector<ga::sleeper*> sleepers; //dynamically allocated because of copy constructor issues (new + delete)
		CharacterSelectEventReceiver* rec;
		unsigned int* runloop;

	public:
		LoginEventReceiver(httplib::SSLClient& client, unsigned int* runloop, CharacterSelectEventReceiver& rec)
		{
			debug("constructing LoginEventReceiver");
			this->client = &client;
			debug("setting alignment of statusmsg, email, and password");
			statusmsg->setTextAlignment(irr::gui::EGUI_ALIGNMENT::EGUIA_CENTER, irr::gui::EGUI_ALIGNMENT::EGUIA_CENTER);
			email->setTextAlignment(irr::gui::EGUI_ALIGNMENT::EGUIA_CENTER, irr::gui::EGUI_ALIGNMENT::EGUIA_CENTER);
			password->setTextAlignment(irr::gui::EGUI_ALIGNMENT::EGUIA_CENTER, irr::gui::EGUI_ALIGNMENT::EGUIA_CENTER);
			debug("setting alignment good");
			this->rec = &rec;
			this->runloop = runloop;
			debug("constructing LoginEventReceiver good");
		}

		virtual bool OnEvent(const irr::SEvent& event)
		{
			if (event.EventType == irr::EET_GUI_EVENT)
			{
				debug("got a gui event");
				debug("getting id of gui element");
				irr::s32 id = event.GUIEvent.Caller->getID();
				debug("gui element id: ", id);
				debug("checking type of gui event");
				if (event.GUIEvent.EventType == irr::gui::EGET_EDITBOX_CHANGED)
				{
					debug("irr::gui::EGET_EDITBOX_CHANGED");
					if (id == 1)
					{
						debug("checking if email string is too long");
						unsigned int lenemail = lstrlenW(email->getText());
						if (lenemail == 17)
						{
							debug("email string too long");
							wchar_t textminusone[17];
							std::memcpy(textminusone, email->getText(), sizeof(textminusone) - 2);
							textminusone[16] = L'\0';
							email->setText(textminusone);
							debug("email string set back a letter");
						}
						debug("email string good");
					}
					else if (id == 2)
					{
						debug("checking if password string is too long");
						unsigned int lenpass = lstrlenW(password->getText());
						if (lenpass == 17)
						{
							debug("password string too long");
							wchar_t textminusone[17];
							std::memcpy(textminusone, password->getText(), sizeof(textminusone) - 2);
							textminusone[16] = L'\0';
							password->setText(textminusone);
							debug("password string set back a letter");
						}
						debug("password string good");
					}
					else
					{
						debug("did you forget to add to the event receiver?");
					}
				}
				else if (event.GUIEvent.EventType == irr::gui::EGET_BUTTON_CLICKED)
				{
					debug("irr::gui::EGET_BUTTON_CLICKED");
					if (id == 1)
					{
						sendLoginPost();
					}
					else if (id == 2)
					{
						sendCreateAccountPost();
						sendLoginPost();
					}
					else
					{
						debug("did you forget to add to the event receiver?");
					}
				}
			}
			return false;
		}

		void getGameServerStringFromSelected(std::string& str)
		{
			debug("getting id of selected game server");
			irr::u32 id = gameserver->getSelected();
			debug("id: ", id);
			debug("looking in game server map with id");
			str = gsitemmap.at(id);
			debug("found string: ", str);
		}

		void setupGameServerMenu()
		{
			debug("setting up game server menu");
			debug("opening file ", GAMESERVERINILOCATION);
			std::ifstream gameserverini(GAMESERVERINILOCATION);
			bool firsttime = true;
			for (std::string line; getline(gameserverini, line);)
			{
				debug("making entry in map for ", line);
				wchar_t buf[LOGINBUFFERMAX];
				mbstowcs(buf, line.c_str(), line.size());
				if (firsttime)
				{
					debug("getting entry id");
					irr::u32 id = gameserver->addItem(buf);
					debug("entry id ", id);
					debug("insert ", line, " at ", id);
					gsitemmap.insert(make_pair(id, line));
					debug("insert good");
					debug("set the first item selected");
					gameserver->setSelected(buf);
					debug("set first item selected good");
					firsttime = false;
				}
				else
				{
					debug("getting entry id");
					irr::u32 id = gameserver->addItem(buf);
					debug("entry id ", id);
					debug("insert ", line, " at ", id);
					gsitemmap.insert(make_pair(id, line));
					debug("insert good");
				}
				debug("make entry good");
			}
			debug("close file ", GAMESERVERINILOCATION);
			gameserverini.close();
			debug("game server menu setup good");
		}

		void makeLoginPost(const char* location, std::shared_ptr<httplib::Response>& res)
		{
			debug("login post at ", location);
			//fill the packet args
			std::string email_string = wcstrtostdstr16(email->getText());
			std::string password_string = wcstrtostdstr16(password->getText());
			std::string selected_server; getGameServerStringFromSelected(selected_server);
			ga::array_packet packet(
				ga::array_packet::get_args_size(
					email_string,
					password_string,
					selected_server
				));
			packet.fill(email_string, password_string, selected_server);
			debug("the packet: ", packet.get(), "\n", packet.get(), "\n", packet.get());
			res = client->Post(location, packet(), "text/plain");
			selected_account = email_string;
		}

		void sendLoginPost()
		{
			std::shared_ptr<httplib::Response> res;
			makeLoginPost("/login", res);
			debug("response body: ", res->body.c_str());
			if (res->body[0] == '\0')
			{
				debug("login failure");
				statusmsg->setText(L"Login failure!");
				resetStatusMessageTimeout();
			}
			else {
				debug("login success");
				statusmsg->setText(L"Login Success!");
				resetStatusMessageTimeout();
				ga::array_packet packet;
				packet.from_buf(res->body.c_str(), res->body.size());
				std::string type = packet.get();
				std::string key = packet.get();
				std::string iv = packet.get();
				std::string skstr = packet.get();
				ci.keyFromString(key);
				ci.ivFromString(iv);
				sk.from_string(skstr);
				debug("established session!");
				debug("client ci for this session: ", ci);
				debug("client secret key for this session: ", sk.to_string());
				debug("connecting to the game server...");
				device->setEventReceiver(rec);
				//*rec = CharacterSelectEventReceiver();
				*runloop = false;
				jointhreads(); //this will be destructed. join any threads.
			}
		}

		void sendCreateAccountPost()
		{
			std::shared_ptr<httplib::Response> res;
			makeLoginPost("/createaccount", res);
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

		void resetStatusMessageTimeout()
		{
			debug("pushing status timeout thread onto thread array");
			ga::sleeper* sleep = new ga::sleeper();
			threads.push_back(std::thread([this, sleep]() {
				sleep->sleepFor(std::chrono::milliseconds(3000));
				statusmsg->setText(L"");
			}));
			sleepers.push_back(sleep);
			debug("pushing status timeout thread good");
		}

		void jointhreads()
		{
			debug("joining threads");
			for (int i = 0; i < threads.size(); ++i)
			{
				sleepers[i]->wake();
				if (threads[i].joinable())
				{
					threads[i].join();
				}
				delete sleepers[i];
			}
			debug("joining threads good");
		}
	};
}