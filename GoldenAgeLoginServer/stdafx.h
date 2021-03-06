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

#include "targetver.h" //target version information

//project wide external code includes
#include <cpp-httplib/httplib.h>

//project wide system includes
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdarg.h>
#include <thread>
#include <sstream> //get this out
#include <direct.h>
#include <chrono>

//project wide defines
//these come after because they affect the preprocessing of everything
//that comes before them
#define GetCurrentDir _getcwd
#define LOGINBUFFERMAX 16
#define SERVER_CERT_FILE "./Auth/cert.pem"
#define SERVER_PRIVATE_KEY_FILE "./Auth/key.pem"
#define ACCOUNTMANIFESTLOCATION "./Manifests/accountmanifest.txt"
#define GAMESERVERMANIFESTLOCATION "./Manifests/gameservermanifest.txt"
#define PASSWORDKEYLOCATION "./Auth/passwordkey.txt"
#define MAXCONNECTIONS 1
#define MAXCHANNELS 2
