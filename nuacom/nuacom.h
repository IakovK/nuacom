#pragma once
/*++

Copyright 1995 - 2000 Microsoft Corporation

Module Name:

    atsp.h

Notes:

--*/

#include "resource.h"
#include <initguid.h> 


//                                                                      
// Line device GUID of MSP                                                    
//                                                  
// CLSID = s '// 23F7C678- 24E1 -48db- AE AC- 54 A6 F2 A0 10 A0


DEFINE_GUID(CLSID_SAMPMSP, 
0x23F7C678, 0x24E1, 0x48db, 0xAE, 0xAC, 0x54, 0xA6, 0xF2, 0xA0, 0x10, 0xA0);



#define  MAX_DEV_NAME_LENGTH    63
#define  ATSP_TIMEOUT           60000   // milliseconds


typedef struct _DRVLINE
{
    HTAPILINE               htLine;

    LINEEVENT               pfnEventProc;

	DWORD                   dwDeviceID;

	std::string session_token;
	std::string extension;
#if 0
	char                    szComm[8];
#endif

	HTAPICALL               htCall;

	DWORD                   dwCallState;

    DWORD                   dwCallStateMode;

    DWORD                   dwMediaMode;

#if 0
	HANDLE                  hComm;
#endif

    BOOL                    bDropInProgress;

#if 0
	OVERLAPPED              Overlapped;
#endif

} DRVLINE, FAR *PDRVLINE;


typedef struct _DRVLINECONFIG
{
    char                    szPort[8];

    char                    szCommands[64];

} DRVLINECONFIG, FAR *PDRVLINECONFIG;


typedef struct _ASYNC_REQUEST
{
    DWORD                   dwRequestID;

    DWORD                   dwCommand;

    char                    szCommand[32];

    struct _ASYNC_REQUEST  *pNext;

} ASYNC_REQUEST, *PASYNC_REQUEST;


extern DWORD gdwLineDeviceIDBase;
extern ASYNC_COMPLETION gpfnCompletionProc;

#if DBG

extern DWORD gdwDebugLevel;
extern HMODULE hInst;

void
CDECL
DebugOutput(
    DWORD   dwLevel,
    LPCSTR  lpszFormat,
    ...
    );

#define DBGOUT(arg) DebugOutput arg

#else

#define DBGOUT(arg)

#endif



LPVOID
PASCAL
DrvAlloc(
    DWORD dwSize
    );

VOID
PASCAL
DrvFree(
    LPVOID lp
    );

void
PASCAL
SetCallState(
    PDRVLINE    pLine,
    DWORD       dwCallState,
    DWORD       dwCallStateMode
    );

BOOL
CALLBACK
ConfigDlgProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    );

LONG
PASCAL
ProviderInstall(
    char   *pszProviderName,
    BOOL    bNoMultipleInstance
    );

void
PASCAL
DropActiveCall(
    PDRVLINE    pLine
    );
