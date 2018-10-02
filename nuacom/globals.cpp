#include "stdafx.h"
#include "nuacom.h"

DWORD gdwDebugLevel = 0;
BOOL gbHandleIncomingCalls;
HMODULE hInst = NULL;
DWORD gdwLineDeviceIDBase = 0;
ASYNC_COMPLETION gpfnCompletionProc = NULL;
