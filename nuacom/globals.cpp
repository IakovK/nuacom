#include "stdafx.h"
#include "nuacom.h"

DWORD gdwDebugLevel = 0;
BOOL gbHandleIncomingCalls = FALSE;
BOOL gbIncomingCallsLocal = FALSE;
HMODULE hInst = NULL;
DWORD gdwLineDeviceIDBase = 0;
DWORD gdwPermanentProviderID = 0;
ASYNC_COMPLETION gpfnCompletionProc = NULL;
TUISPIDLLCALLBACK glpfnUIDLLCallback = NULL;
