// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

// TODO: reference additional headers your program requires here
#define CPPHTTPLIB_OPENSSL_SUPPORT
#define LOGINBUFFERMAX 20
#define SERVER_CERT_FILE "./Auth/cert.pem"
#define SERVER_PRIVATE_KEY_FILE "./Auth/key.pem"
#define ACCOUNTMANIFESTLOCATION "./Manifests/accountmanifest.txt"
#define GAMESERVERMANIFESTLOCATION "./Manifests/gameservermanifest.txt"
#define PASSWORDKEYLOCATION "./Auth/passwordkey.txt"

#include <cpp-httplib/httplib.h>
#include <fstream>
#include <string>
#include <unordered_map>
#include <stdarg.h>
#include "Debug.hpp"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <thread>
#include <sstream>

#ifdef WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif
