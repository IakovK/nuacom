#pragma once
/*++

Copyright 1995 - 2000 Microsoft Corporation

Module Name:

    atsp.h

Notes:

--*/

#include "resource.h"
#include <initguid.h> 
#include "../RPCClient/RPCWrapper.h"
#include "..\nuacomsrv\websocketapi.h"


//                                                                      
// Line device GUID of MSP                                                    
//                                                  
// CLSID = s '// 23F7C678- 24E1 -48db- AE AC- 54 A6 F2 A0 10 A0


DEFINE_GUID(CLSID_SAMPMSP, 
0x23F7C678, 0x24E1, 0x48db, 0xAE, 0xAC, 0x54, 0xA6, 0xF2, 0xA0, 0x10, 0xA0);



#define  MAX_DEV_NAME_LENGTH    63
#define  ATSP_TIMEOUT           60000   // milliseconds


class DRVLINE : public ICallbacks
{
public:
	virtual void OnOpenListener();
	virtual void OnFailListener();
	virtual void ProcessMessage(IMessageHolder *pmh);
	void ProcessObjectMessage(const std::map<std::string, IMessageHolder*> &msg);
	void ProcessCallMessage(const std::string &msgId, const std::map<std::string, IMessageHolder*> &msg);
	bool shouldProcessIncomingCalls(const std::string &callDirection);
	DWORD CalculateCallInfoSize();
	bool ConnectToServer();

	HTAPILINE               htLine;

    LINEEVENT               pfnEventProc;

	DWORD                   dwDeviceID;

	CRPCWrapper rpcw;
	bool bConnected;

	//std::string destAddress;
	std::string localChannel;

	HTAPICALL               htCall;

	DWORD                   dwCallState;

    DWORD                   dwCallStateMode;

    DWORD                   dwMediaMode;

    BOOL                    bDropInProgress;
	BOOL bIncomingCall;
	std::mutex mtx;
	std::map<std::string, std::string> msgData;

	DRVLINE()
		:htLine(NULL)
		, pfnEventProc(NULL)
		, dwDeviceID(0)
		, htCall(NULL)
		, dwCallState(0)
		, dwCallStateMode(0)
		, dwMediaMode(0)
		, bDropInProgress(FALSE)
		, bIncomingCall(FALSE)
		, bConnected(false)
	{
	}
};
typedef DRVLINE FAR *PDRVLINE;

extern DWORD gdwLineDeviceIDBase;
extern DWORD gdwPermanentProviderID;
extern ASYNC_COMPLETION gpfnCompletionProc;
extern HMODULE hInst;
extern BOOL gbHandleIncomingCalls;
extern BOOL gbIncomingCallsLocal;

#if DBG

extern DWORD gdwDebugLevel;

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

void
PASCAL
DropActiveCall(
    PDRVLINE    pLine
    );
