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
//#define IMAKETHINGSWORK 777

#define _ITERATOR_DEBUG_LEVEL 0

#include "targetver.h" //target version information

//project wide external code includes
#include <irrlicht.h>

//project wide system includes
#include <sstream> //get this out
#include <unordered_map>
#include <chrono>
#include <vector>
#include <fstream>

//project wide defines
//these come after because they affect the preprocessing of everything
//that comes before them
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 3000
