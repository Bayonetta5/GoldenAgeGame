// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

// TODO: reference additional headers your program requires here
//#define _IRR_STATIC_LIB_
//#define CPPHTTPLIB_OPENSSL_SUPPORT
#define GAMESERVERINILOCATION "./init/gameserver.ini"
#define LOGINBUFFERMAX 20
#define _ITERATOR_DEBUG_LEVEL 0

#include <cpp-httplib/httplib.h>
#include <irrlicht.h>
#include <fstream>
#include <string>
#include <unordered_map>
#include <thread>

#ifdef WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif