#include "stdafx.h"
#include "nuacom.h"

char gszRegKey[] = "Software\\Nuacom\\TSP";
DWORD gdwDebugLevel = 0;
HMODULE hInst = NULL;
DWORD gdwLineDeviceIDBase = 0;
ASYNC_COMPLETION gpfnCompletionProc = NULL;
