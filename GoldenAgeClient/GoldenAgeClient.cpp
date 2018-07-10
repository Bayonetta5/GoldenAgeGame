#include "stdafx.h"
#include "LoginEventReceiver.h"
#include "CharacterSelectionEventReceiver.h"
#include "login_screen_loop.h"
#include "GameEventReceiver.h"
#include "client_toon.h"
#include <GoldenAge/debug.h>
#include <GoldenAge/udp_com.h>
#include <GoldenAge/cryptinfo.h>
#include <GoldenAge/array_packet.h>
#include <GoldenAge/secretkey.h>
#include <GoldenAge/toon.h>

#define MAXCLIENTS 1
#define MAXCHANNELS 2

ga::udp_com com(MAXCLIENTS, MAXCHANNELS);
irr::scene::IMeshSceneNode* current_area = 0;
irr::scene::IMeshSceneNode* last_area = 0;
irr::scene::ITriangleSelector* selector = 0;
std::string areamap[12][12] = {
	{ "./system/resources/3dart/terrain-heightmap.bmp" }
};
std::vector<client_toon> client_toons;
ga::GameEventReceiver gamereceiver;
ga::cryptinfo ci;
ga::secretkey sk;
irr::IrrlichtDevice* device = irr::createDevice(irr::video::E_DRIVER_TYPE::EDT_DIRECT3D9, irr::core::dimension2d<irr::u32>(800, 900), 16, false);
irr::video::IVideoDriver* driver = device->getVideoDriver();
irr::gui::IGUIEnvironment* env = device->getGUIEnvironment();
irr::scene::ISceneManager* smgr = device->getSceneManager();
irr::scene::ICameraSceneNode* camera = smgr->addCameraSceneNode(0, irr::core::vector3df(0, 30, -40), irr::core::vector3df(0, 5, 0));
irr::video::ITexture* bg = driver->getTexture("./system/resources/textures/kewlbg.jpg");
irr::core::dimension2du screen_size = driver->getScreenSize();
unsigned int runloop = 1;

enum
{
	// I use this ISceneNode ID to indicate a scene node that is
	// not pickable by getSceneNodeAndCollisionPointFromRay()
	ID_IsNotPickable = 0,

	// I use this flag in ISceneNode IDs to indicate that the
	// scene node can be picked by ray selection.
	IDFlag_IsPickable = 1 << 0,

	// I use this flag in ISceneNode IDs to indicate that the
	// scene node can be highlighted.  In this example, the
	// homonids can be highlighted, but the level mesh can't.
	IDFlag_IsHighlightable = 1 << 1
};


void load_area(ga::world_position pos)
{
	// add terrain scene node
	irr::scene::ITerrainSceneNode* terrain = smgr->addTerrainSceneNode(areamap[pos.row][pos.col].c_str());
	// create triangle selector for the terrain 
	selector = smgr->createTerrainTriangleSelector(terrain, 0);
	terrain->setTriangleSelector(selector);
	terrain->setMaterialFlag(irr::video::EMF_LIGHTING, false);

	terrain->setMaterialTexture(0,
	driver->getTexture("./system/resources/3dart/terrain-texture.jpg"));
	terrain->setMaterialTexture(1,
	driver->getTexture("./system/resources/3dart/detailmap3.jpg"));

	terrain->setMaterialType(irr::video::EMT_DETAIL_MAP);

	terrain->scaleTexture(1.0f, 20.0f);
}

void add_ground_collision(irr::scene::IAnimatedMeshSceneNode* node)
{
	if (selector)
	{
		irr::scene::ISceneNodeAnimator* anim = smgr->createCollisionResponseAnimator(
			selector, node, irr::core::vector3df(30, 50, 30),
			irr::core::vector3df(0, -10, 0), irr::core::vector3df(0, 30, 0));
		node->addAnimator(anim);
		anim->drop();  // And likewise, drop the animator when we're done referring to it.
	}
}

void init_toon_models(std::vector<ga::toon>& toons)
{
	bool firsttime = true;
	for (ga::toon& t : toons)
	{
		if (firsttime)
		{
			client_toon ct(t.name);
			ct.t.tg = t.tg;
			ct.setup_graphics("./system/resources/3dart/human_male.blend.ms3d");
			client_toons.push_back(ct);
			firsttime = false;
		}
		client_toon ct(t.name);
		ct.t.tg = t.tg;
		ct.setup_graphics("./system/resources/3dart/human_male.blend.ms3d");
		ct.node->setVisible(false);
		client_toons.push_back(ct);
	}
}

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

	debug("checking smgr");
	if (smgr == 0)
	{
		debug("Failed to create scene manager!");
		system("pause");
		return 1;
	}

	std::vector<ga::toon> mytoons;
	com.connect("localhost", 3001, 60*1000*15); //timeout 15 minutes
	ga::CharacterSelectEventReceiver receiver;

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
			init_toon_models(mytoons);
			receiver.init_toon_select_box(mytoons);
		}
		else if (type == "p")
		{
			debug("received a toon created confirmation");
			//take care of the gui after we confirm that creation is correct, or display terrible ERROR
			receiver.finishCreateToon();
		}
		else if (type == "s")
		{
			debug("received my current toon data, fill him in and start the game!");
			smgr->clear();
			ga::toon t(receiver.getSelectedToon()->name);
			t.from_string(packet.get());
			client_toon mytoon(std::move(t));
			mytoon.setup_graphics("./system/resources/3dart/human_male.blend.ms3d");
			device->setEventReceiver(&gamereceiver);
			camera = smgr->addCameraSceneNode();
			client_toons = { mytoon };

			runloop = false;
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

		smgr->drawAll();

		env->drawAll();

		driver->endScene();
	});
	
	{
		ga::loginScreenLoop(bg, runloop, receiver);
	}
	env->clear();
	receiver.setup();
	
	//start the game loop
	irr::u32 millis = device->getTimer()->getTime();
	//get the screen size data
	screen_size = driver->getScreenSize();

	runloop = true;
	com.listenLoop(runloop, millis);
	runloop = true;
	env->clear();

	//register game packet handlers
	com.register_receive([](ENetEvent& e) {
		debug("My game receive handler");
		ga::array_packet packet;
		packet.from_buf((const char*)e.packet->data, e.packet->dataLength);
		std::string type = packet.get();
		if (type == "d")
		{

		}
		else
		{

		}
	});

	com.register_tick([](unsigned int delta_time) {
		//update toons
		device->run();
		driver->beginScene(true, true, irr::video::SColor(0, 120, 102, 136));

		smgr->drawAll();

		env->drawAll();

		driver->endScene();
	});

	//load area from toon world_position
	load_area(client_toons.front().t.pos);
	debug(client_toons.front().node->getPosition().X, " ", client_toons.front().node->getPosition().Y, " ", client_toons.front().node->getPosition().Z);
	client_toons.front().init_position();
	debug(client_toons.front().node->getPosition().X, " ", client_toons.front().node->getPosition().Y, " ", client_toons.front().node->getPosition().Z);
	add_ground_collision(client_toons.front().node);

	//setup camera to follow client 0
	client_toons.front().node->addChild(camera);
	camera->setPosition(client_toons.front().node->getPosition() + irr::core::vector3df(0, 25, 0));
	camera->setTarget(client_toons.front().node->getPosition());

	//start the gameloop, probs get a bunch of packets about stuff spawning
	millis = device->getTimer()->getTime();
	com.listenLoop(runloop, millis);


	debug("exiting program");
	system("pause");
	return 0;
}