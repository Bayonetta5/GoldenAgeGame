/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*                                                                                                                    *
*   stdafx.h : include file for standard system include files,														 *
*	or project specific include files that are used frequently, but													 *
*	are changed infrequently																						 *
*																													 *
*	e.g. project wide includes you won't touch, or you have to recompile it all (precompiled headers, stdafx == pch) *
*	so our project code we will include in our cpp directly instead of through stdafx.h	(you include stdafx.h in all *
*	your cpp, before other includes)													                             *
*                                                                                                                    *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once //faster compile times than a header guard

//project wide defines (there are some set in the project settings as well)
//these come first because they affect the preprocessing of everything
//that comes after them [you could end up having to weave these in]
#define CPPHTTPLIB_OPENSSL_SUPPORT
#define _ITERATOR_DEBUG_LEVEL 0

#include "targetver.h" //target version info

//project wide external code includes
#include <cpp-httplib/httplib.h>
#include <irrlicht.h>

//project wide system includes
#include <fstream>
#include <string>
#include <unordered_map>
#include <thread>
#include <direct.h>

//project wide defines
//these come after because they affect the preprocessing of everything
//that comes before them
#define GetCurrentDir _getcwd
#define GAMESERVERINILOCATION "./init/gameserver.ini"
#define LOGINBUFFERMAX 16
