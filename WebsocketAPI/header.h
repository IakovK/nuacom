// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#include "targetver.h"

#ifdef WEBSOCKETAPI_EXPORTS
#define WSAPI __declspec(dllexport)
#else
#define WSAPI __declspec(dllimport)
#endif

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>



// reference additional headers your program requires here
#include <stdint.h>
