// nuacom.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "nuacom.h"
#include "config.h"
#include <strsafe.h>
#include "nuacomAPI.h"

#define MAX_BUF_SIZE 256

void DRVLINE::OnOpenListener()
{
	DBGOUT((3, "DRVLINE::OnOpenListener: open_listener callback"));
}

void DRVLINE::OnFailListener()
{
	DBGOUT((3, "DRVLINE::OnFailListener: fail_listener callback"));
}

void DRVLINE::ProcessObjectMessage(const std::map<std::string, IMessageHolder*> &msg)
{
	for (auto &v : msg)
	{
		auto msgId = v.first;
		if (v.second->get_flag() != IMessageHolder::flag::flag_object)
			continue;
		auto pmh = v.second;
		auto l = pmh->get_object_length();
		std::map<std::string, IMessageHolder*> data;
		for (int j = 0; j < l; j++)
		{
			char *ptr;
			int length;
			pmh->get_key(j, (void**)&ptr, &length);
			std::string s(ptr, length);
			auto obj = pmh->get_data(j);
			data[s] = obj;
		}
		ProcessCallMessage(msgId, data);
	}
}

bool DRVLINE::shouldProcessIncomingCalls(const std::string &callDirection)
{
	if (callDirection != "inbound")
		return true;
	if ((htCall == NULL || incomingCall) && gbHandleIncomingCalls)
		return true;
	else
		return false;
}

DWORD DRVLINE::CalculateCallInfoSize()
{
	DWORD retVal = sizeof(LINECALLINFO);

	std::string idx = "call_caller_number";	// CallerID
	if (msgData.find(idx) != msgData.end())
		retVal += (msgData[idx].size() + 1) * 2;

	idx = "call_caller_name";	// CallerIDName
	if (msgData.find(idx) != msgData.end())
		retVal += (msgData[idx].size() + 1) * 2;

	idx = "call_callee_number";	// CalledID
	if (msgData.find(idx) != msgData.end())
		retVal += (msgData[idx].size() + 1) * 2;

	idx = "call_callee_name";	// CalledIDName
	if (msgData.find(idx) != msgData.end())
	{
		if (msgData[idx].empty())
			msgData[idx] = "Test name #1";
		retVal += (msgData[idx].size() + 1) * 2;
	}

	DBGOUT((3, "DRVLINE::CalculateCallInfoSize: retVal = %d", retVal));
	return retVal;
}

void DRVLINE::ProcessCallMessage(const std::string &msgId, const std::map<std::string, IMessageHolder*> &msg)
{
	{
		std::lock_guard<std::mutex> guard(mtx);
		msgData.clear();
		for (auto &v : msg)
		{
			auto f = v.second->get_flag();
			if (f == IMessageHolder::flag::flag_string)
			{
				auto pmh = v.second;
				char *ptr;
				int length;
				pmh->get_string((void**)&ptr, &length);
				std::string s(ptr, length);
				msgData[v.first] = s;
			}
		}
	}

	for (auto &v : msgData)
	{
		DBGOUT((3, "DRVLINE::ProcessCallMessage: %s: %s", v.first.c_str(), v.second.c_str()));
	}

	try
	{
		if (!shouldProcessIncomingCalls(msgData["call_direction"]))
			return;
		auto channel = msgData["local_channel"];
		if (channel.size() > 4)
		{
			if (channel.substr(0, 4) == "SIP/")
				localChannel = channel.substr(4);
		}

		if (htCall == NULL)
		{
			(*pfnEventProc)(
				htLine,
				NULL,
				LINE_NEWCALL,
				(DWORD_PTR)this,
				(DWORD_PTR)&htCall,
				0
				);
			DBGOUT((3, "DRVLINE::ProcessCallMessage: after LINE_NEWCALL: htCall = %p", htCall));
			if (htCall == 0)
			{
				// hang up. we can't process now...
				std::string msg;
				EndCall(session_token, localChannel, msg);
				return;
			}
			incomingCall = TRUE;
		}

		DBGOUT((3, "DRVLINE::ProcessCallMessage: calling SetCallState"));
		auto status = msgData["call_status"];
		if (status == "ringing")
			SetCallState(this, LINECALLSTATE_RINGBACK, 0);
		else if (status == "connected")
			SetCallState(this, LINECALLSTATE_CONNECTED, 0);
		else if (status == "completed")
			SetCallState(this, LINECALLSTATE_DISCONNECTED, 0);
	}
	catch (const std::exception&)
	{
	}
}

void DRVLINE::ProcessMessage(IMessageHolder *pmh)
{
	if (pmh->get_flag() != IMessageHolder::flag::flag_object)
		return;
	auto l = pmh->get_object_length();
	std::map<std::string, IMessageHolder*> data;
	for (int j = 0; j < l; j++)
	{
		char *ptr;
		int length;
		pmh->get_key(j, (void**)&ptr, &length);
		std::string s(ptr, length);
		auto obj = pmh->get_data(j);
		data[s] = obj;
	}
	ProcessObjectMessage(data);
}

///////////////////////////////////////////////////////////////////////////
// TSPI_lineAnswer
//
// This function answers the specified offering call.
//
extern "C"
LONG TSPIAPI TSPI_lineAnswer(DRV_REQUESTID dwRequestId, HDRVCALL hdCall,
	LPCSTR lpsUserUserInfo, DWORD dwSize)
{
	DBGOUT((3, "TSPI_lineAnswer"));
	LONG        lResult = LINEERR_INVALCALLHANDLE;

	return lResult;
}

///////////////////////////////////////////////////////////////////////////
// TSPI_lineDial
//
// This function dials the specified dialable number on the specified
// call device.
//
extern "C"
LONG TSPIAPI TSPI_lineDial(DRV_REQUESTID dwRequestID, HDRVCALL hdCall,
	LPCWSTR lpszDestAddress, DWORD dwCountryCode)
{
	DBGOUT((3, "TSPI_lineDial"));
	LONG        lResult = LINEERR_INVALCALLHANDLE;

	return lResult;
}

///////////////////////////////////////////////////////////////////////////
// TSPI_lineGetAddressID
//
// This function returns the specified address associated with the
// specified line in a specified format.
//
extern "C"
LONG TSPIAPI TSPI_lineGetAddressID(HDRVLINE hdLine, LPDWORD lpdwAddressID,
	DWORD dwAddressMode, LPCWSTR lpszAddress, DWORD dwSize)
{
	DBGOUT((3, "TSPI_lineGetAddressID"));
	LONG lResult = LINEERR_INVALLINEHANDLE;

	return lResult;
}

//////////////////////////////////////////////////////////////////////////
// TSPI_lineGetDevConfig
//
// This function returns a data structure object, the contents of which
// are specific to the line (SP) and device class, giving the current
// configuration of a device associated one-to-one with the line device.
//
extern "C"
LONG TSPIAPI TSPI_lineGetDevConfig(DWORD dwDeviceID, LPVARSTRING lpDeviceConfig,
	LPCWSTR lpszDeviceClass)
{
	DBGOUT((3, "TSPI_lineGetDevConfig"));
	LONG lResult = LINEERR_BADDEVICEID;

	return lResult;
}

//////////////////////////////////////////////////////////////////////////
// TSPI_lineGetIcon
//
// This function retreives a service line device-specific icon for
// display to the user
//
extern "C"
LONG TSPIAPI TSPI_lineGetIcon(DWORD dwDeviceID, LPCWSTR lpszDeviceClass,
	LPHICON lphIcon)
{
	DBGOUT((3, "TSPI_lineGetIcon"));
	LONG lResult = LINEERR_BADDEVICEID;

	return lResult;
}

//////////////////////////////////////////////////////////////////////////////
// TSPI_lineSetAppSpecific
//
// This function sets the application specific portion of the 
// LINECALLINFO structure.  This is returned by the TSPI_lineGetCallInfo
// function.
//
extern "C"
LONG TSPIAPI TSPI_lineSetAppSpecific(HDRVCALL hdCall, DWORD dwAppSpecific)
{
	DBGOUT((3, "TSPI_lineSetAppSpecific"));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	return lResult;
}

/////////////////////////////////////////////////////////////////////////////
// TSPI_lineSetDevConfig
//
// This function restores the configuration of a device associated one-to-one
// with the line device from a data structure obtained through TSPI_lineGetDevConfig.
// The contents of the data structure are specific to the service provider.
//
extern "C"
LONG TSPIAPI TSPI_lineSetDevConfig(DWORD dwDeviceID, LPVOID const lpDevConfig,
	DWORD dwSize, LPCWSTR lpszDeviceClass)
{
	DBGOUT((3, "TSPI_lineSetDevConfig"));
	LONG lResult = LINEERR_BADDEVICEID;

	return lResult;
}

///////////////////////////////////////////////////////////////////////////
// TSPI_lineSetStatusMessages
//
// This function tells us which events to notify TAPI about when
// address or status changes about the specified line.
//
extern "C"
LONG TSPIAPI TSPI_lineSetStatusMessages(HDRVLINE hdLine, DWORD dwLineStates,
	DWORD dwAddressStates)
{
	DBGOUT((3, "TSPI_lineSetStatusMessages: hdLine = %p, dwLineStates = %08x, dwAddressStates = %08x",
		hdLine, dwLineStates, dwAddressStates));
	LONG lResult = 0;

	return lResult;
}

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneNegotiateTSPIVersion
//
// This function returns the highest SP version number the
// service provider is willing to operate under for this device,
// given the range of possible values.
//
extern "C"
LONG TSPIAPI TSPI_phoneNegotiateTSPIVersion(DWORD dwDeviceID,
	DWORD dwLowVersion, DWORD dwHighVersion,
	LPDWORD lpdwVersion)
{
	DBGOUT((3, "TSPI_phoneNegotiateTSPIVersion"));
	LONG lResult = LINEERR_BADDEVICEID;

	return lResult;
}

#if 0
void
CommThread(
	PDRVLINE    pLine
)
{
	char            buf[4];
	DWORD           dwThreadID = GetCurrentThreadId(), dwNumBytes;
	HANDLE          hComm = pLine->hComm, hEvent;
	LPOVERLAPPED    pOverlapped = &pLine->Overlapped;


	DBGOUT((
		3,
		"CommThread (id=%d): enter, port=%s",
		dwThreadID,
		pLine->szComm
		));

	hEvent = pOverlapped->hEvent;
	buf[0] = buf[1] = '.';


	//
	// Loop waiting for i/o to complete (either the Write done in
	// TSPI_lineMakeCall or the Reads done to retrieve status info).
	// Note that TSPI_lineDrop or TSPI_lineCloseCall may set the
	// event to alert us that they're tearing down the call, in
	// which case we just exit.
	//

	for (;;)
	{
		if (WaitForSingleObject(hEvent, ATSP_TIMEOUT) == WAIT_OBJECT_0)
		{
			if (pLine->bDropInProgress == TRUE)
			{
				DBGOUT((2, "CommThread (id=%d): drop in progress"));
				goto CommThread_exit;
			}

			GetOverlappedResult(hComm, pOverlapped, &dwNumBytes, FALSE);
			ResetEvent(hEvent);
		}
		else
		{
			DBGOUT((2, "CommThread (id=%d): wait timeout"));
			SetCallState(pLine, LINECALLSTATE_IDLE, 0);
			goto CommThread_exit;
		}

		buf[1] &= 0x7f; // remove the parity bit

		DBGOUT((
			3,
			"CommThread (id=%d): read '%c'",
			dwThreadID,
			(buf[1] == '\r' ? '.' : buf[1])
			));

		switch ((buf[0] << 8) + buf[1])
		{
		case 'CT':  // "CONNECT"
		case 'OK':  // "OK"

			SetCallState(pLine, LINECALLSTATE_CONNECTED, 0);
			goto CommThread_exit;

		case 'SY':  // "BUSY"
		case 'OR':  // "ERROR"
		case 'NO':  // "NO ANSWER", "NO DIALTONE", "NO CARRIER"

			SetCallState(pLine, LINECALLSTATE_IDLE, 0);
			goto CommThread_exit;

		default:

			break;
		}

		buf[0] = buf[1];

		ZeroMemory(pOverlapped, sizeof(OVERLAPPED) - sizeof(HANDLE));
		ReadFile(hComm, &buf[1], 1, &dwNumBytes, pOverlapped);
	}

CommThread_exit:

	CloseHandle(hEvent);
	DBGOUT((3, "CommThread (id=%d): exit", dwThreadID));
	ExitThread(0);
}


//
// We get a slough of C4047 (different levels of indrection) warnings down
// below in the initialization of FUNC_PARAM structs as a result of the
// real func prototypes having params that are types other than DWORDs,
// so since these are known non-interesting warnings just turn them off
//

#pragma warning (disable:4047)


//
// --------------------------- TAPI_lineXxx funcs -----------------------------
//


LONG
TSPIAPI TSPI_lineMSPIdentify(
	DWORD               dwDeviceID,
	GUID *              pCLSID
)
{

	LONG  lResult = 0;

#if DBG
	FUNC_PARAM  params[] =
	{
		{ "dwDeviceID", dwDeviceID }
	};

	FUNC_INFO   info =
	{
		"TSPI_lineMSPIdentify",
		1,
		params
	};

#endif


	Prolog(&info);

	//
	// Identify the CLSID of the our MSP
	// this is the CLSID of Sample MSP in the 
	// TAPI3 SDK
	//

	*pCLSID = CLSID_SAMPMSP;

	return (Epilog(&info, lResult));

}


LONG
TSPIAPI TSPI_lineReceiveMSPData(
	HDRVLINE        hdLine,
	HDRVCALL        hdCall,
	HDRVMSPLINE     hdMSPLine,
	LPBYTE          pBuffer,
	DWORD           dwSize
)
{
	LONG lResult = 0;

#if DBG
	FUNC_PARAM  params[] =
	{
		{  gszhdLine, hdLine },
		{  gszhdCall, hdCall},
		{ "hdMSPLine", hdMSPLine},
		{ "pBuffer"  , pBuffer},
		{ "dwSize" , dwSize}

	};

	FUNC_INFO   info =
	{
		"TSPI_lineReceiveMSPData",
		5,
		params
	};

#endif

	Prolog(&info);

	//
	// Place code to Process Data received from the MSP
	//

	return (Epilog(&info, lResult));
}

LONG
TSPIAPI
TSPI_lineCreateMSPInstance(
	HDRVLINE        hdLine,
	DWORD           dwAddressID,
	HTAPIMSPLINE    htMSPLine,
	LPHDRVMSPLINE   phdMSPLine
)
{
	LONG		lResult = 0;
	PDRVLINE    pLine = (PDRVLINE)hdLine;

#if DBG
	FUNC_PARAM  params[] =
	{
		{ gszhdLine,      hdLine},
		{ "dwAddressID",  dwAddressID },
		{ "htMSPLine",    htMSPLine  }
	};
	FUNC_INFO   info =
	{
		"TSPI_lineCreateMSPInstance",
		3,
		params
	};

#endif


	Prolog(&info);

	//
	// we save the TAPI MSP handle as a member value
	// in the line in order to be able to send MSP messages
	// with the line/client identification to TAPISRV.
	// tapisrv identifies the client with the call handle and or the
	// MSP handle.. both cannot be ambiguous.
	//

	pLine->htMSPLineHandle = htMSPLine;


	//
	// we fake a TTSP handle for TAPI we are not keeping track of this
	//

	*phdMSPLine = 0;

	return (Epilog(&info, lResult));

}

LONG
TSPIAPI TSPI_lineCloseMSPInstance(
	HDRVMSPLINE         hdMSPLine
)
{
	LONG  lResult = 0;

#if DBG
	FUNC_PARAM  params[] =
	{
		{ (char*)"hdMSPLine", (DWORD)hdMSPLine}
	};

	FUNC_INFO   info =
	{
		(char*)"TSPI_lineCloseMSPInstance",
		1,
		params
	};

#endif


	Prolog(&info);

	//
	// no - op there is no information stored with the 
	// msp so we dont need to free anything or remove anything 
	//

	return (Epilog(&info, lResult));

}
#endif



LONG
TSPIAPI
TSPI_lineClose(
	HDRVLINE    hdLine
)
{
	DBGOUT((3, "TSPI_lineClose"));
	LONG        lResult = 0;
	try
	{
		PDRVLINE pd = (PDRVLINE)hdLine;
		DBGOUT((3, "TSPI_lineClose: calling CloseWebsocket"));
		CloseWebsocket(pd->socketHandle);
		DBGOUT((3, "TSPI_lineClose: calling CloseWebsocket done"));
		delete (PDRVLINE)hdLine;
	}
	catch (const std::exception&)
	{
	}
	return lResult;
}


LONG
TSPIAPI
TSPI_lineCloseCall(
	HDRVCALL    hdCall
)
{
	DBGOUT((3, "TSPI_lineCloseCall"));
	PDRVLINE    pLine = (PDRVLINE)hdCall;

	//
	// Note that in TAPI 2.0 TSPI_lineCloseCall can get called
	// without TSPI_lineDrop ever being called, so we need to
	// be prepared for either case.
	//

	DropActiveCall(pLine);
	pLine->htCall = NULL;
	pLine->incomingCall = FALSE;
	pLine->msgData.clear();
	return 0;
}


#if 0
LONG
TSPIAPI
TSPI_lineConditionalMediaDetection(
	HDRVLINE            hdLine,
	DWORD               dwMediaModes,
	LPLINECALLPARAMS    const lpCallParams
)
{
#if DBG
	FUNC_PARAM  params[] =
	{
		{ gszhdLine,        (DWORD)hdLine       },
		{ (char*)"dwMediaModes",   dwMediaModes },
		{ gszlpCallParams,  (DWORD)lpCallParams }
	};
	FUNC_INFO   info =
	{
		(char*)"TSPI_lineConditionalMediaDetection",
		3,
		params
	};
#endif


	//
	// This func is really a no-op for us, since we don't look
	// for incoming calls (though we do say we support them to
	// make apps happy)
	//

	Prolog(&info);
	return (Epilog(&info, 0));
}
#endif


LONG
TSPIAPI
TSPI_lineDrop(
	DRV_REQUESTID   dwRequestID,
	HDRVCALL        hdCall,
	LPCSTR          lpsUserUserInfo,
	DWORD           dwSize
)
{
	DBGOUT((3, "TSPI_lineDrop: dwRequestID = %08x, dwSize = %d", dwRequestID, dwSize));

	PDRVLINE    pLine = (PDRVLINE)hdCall;
	DropActiveCall(pLine);
	SetCallState(pLine, LINECALLSTATE_IDLE, 0);
	(*gpfnCompletionProc)(dwRequestID, 0);

	return dwRequestID;
}


LONG
TSPIAPI
TSPI_lineGetAddressCaps(
	DWORD              dwDeviceID,
	DWORD              dwAddressID,
	DWORD              dwTSPIVersion,
	DWORD              dwExtVersion,
	LPLINEADDRESSCAPS  lpAddressCaps
)
{
	DBGOUT((3, "TSPI_lineGetAddressCaps: dwDeviceID = %d, dwAddressID = %d, dwTSPIVersion = %08x, dwExtVersion = %08x",
		dwDeviceID, dwAddressID, dwTSPIVersion, dwExtVersion));

	LONG        lResult = 0;


	if (dwAddressID != 0)
	{
		lResult = LINEERR_INVALADDRESSID;
	}

	lpAddressCaps->dwNeededSize =
		lpAddressCaps->dwUsedSize = sizeof(LINEADDRESSCAPS);

	lpAddressCaps->dwLineDeviceID = dwDeviceID;
	lpAddressCaps->dwAddressSharing = LINEADDRESSSHARING_PRIVATE;
	lpAddressCaps->dwCallInfoStates = LINECALLINFOSTATE_MEDIAMODE |
		LINECALLINFOSTATE_APPSPECIFIC;
	lpAddressCaps->dwCallerIDFlags =
		lpAddressCaps->dwCalledIDFlags =
		lpAddressCaps->dwRedirectionIDFlags =
		lpAddressCaps->dwRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;
	lpAddressCaps->dwCallStates = LINECALLSTATE_IDLE |
		LINECALLSTATE_OFFERING |
		LINECALLSTATE_ACCEPTED |
		LINECALLSTATE_DIALTONE |
		LINECALLSTATE_DIALING |
		LINECALLSTATE_CONNECTED |
		LINECALLSTATE_PROCEEDING |
		LINECALLSTATE_DISCONNECTED |
		LINECALLSTATE_UNKNOWN;
	lpAddressCaps->dwDialToneModes = LINEDIALTONEMODE_UNAVAIL;
	lpAddressCaps->dwBusyModes = LINEBUSYMODE_UNAVAIL;
	lpAddressCaps->dwSpecialInfo = LINESPECIALINFO_UNAVAIL;
	lpAddressCaps->dwDisconnectModes = LINEDISCONNECTMODE_NORMAL |
		LINEDISCONNECTMODE_BUSY |
		LINEDISCONNECTMODE_NOANSWER |
		LINEDISCONNECTMODE_UNAVAIL |
		LINEDISCONNECTMODE_NODIALTONE;
	lpAddressCaps->dwMaxNumActiveCalls = 1;
	lpAddressCaps->dwAddrCapFlags = LINEADDRCAPFLAGS_DIALED;
	lpAddressCaps->dwCallFeatures = LINECALLFEATURE_ACCEPT |
		LINECALLFEATURE_ANSWER |
		LINECALLFEATURE_DROP |
		LINECALLFEATURE_SETCALLPARAMS;
	lpAddressCaps->dwAddressFeatures = LINEADDRFEATURE_MAKECALL;

	return lResult;
}


LONG
TSPIAPI
TSPI_lineGetAddressStatus(
	HDRVLINE            hdLine,
	DWORD               dwAddressID,
	LPLINEADDRESSSTATUS lpAddressStatus
)
{
	DBGOUT((3, "TSPI_lineGetAddressStatus"));

	LONG        lResult = 0;
	PDRVLINE    pLine = (PDRVLINE)hdLine;


	lpAddressStatus->dwNeededSize =
		lpAddressStatus->dwUsedSize = sizeof(LINEADDRESSSTATUS);

	lpAddressStatus->dwNumActiveCalls = (pLine->htCall ? 1 : 0);
	lpAddressStatus->dwAddressFeatures = LINEADDRFEATURE_MAKECALL;

	return lResult;
}


#if 0
LONG
TSPIAPI
TSPI_lineGetCallAddressID(
	HDRVCALL            hdCall,
	LPDWORD             lpdwAddressID
)
{
#if DBG
	FUNC_PARAM  params[] =
	{
		{ gszhdCall,        hdCall          },
		{ "lpdwAddressID",  lpdwAddressID   }
	};
	FUNC_INFO   info =
	{
		"TSPI_lineGetCallAddressID",
		2,
		params
	};
#endif


	//
	// We only support 1 address (id=0)
	//

	Prolog(&info);
	*lpdwAddressID = 0;
	return (Epilog(&info, 0));
}
#endif


LONG
TSPIAPI
TSPI_lineGetCallInfo(
	HDRVCALL        hdCall,
	LPLINECALLINFO  lpLineInfo
)
{
	DBGOUT((3, "TSPI_lineGetCallInfo: hdCall = %p, lpLineInfo = %p", hdCall, lpLineInfo));
	LONG        lResult = 0;
	PDRVLINE    pLine = (PDRVLINE)hdCall;

	lpLineInfo->dwNeededSize = pLine->CalculateCallInfoSize();
	lpLineInfo->dwUsedSize = sizeof(LINECALLINFO);

	DBGOUT((3, "TSPI_lineGetCallInfo: lpLineInfo->dwNeededSize = %d, lpLineInfo->dwTotalSize = %d, lpLineInfo->dwUsedSize = %d, msgData.size() = %d", lpLineInfo->dwNeededSize, lpLineInfo->dwTotalSize, lpLineInfo->dwUsedSize, pLine->msgData.size()));

	lpLineInfo->dwBearerMode = LINEBEARERMODE_VOICE;
	lpLineInfo->dwMediaMode = pLine->dwMediaMode;
	lpLineInfo->dwCallStates = LINECALLSTATE_IDLE |
		LINECALLSTATE_DIALTONE |
		LINECALLSTATE_DIALING |
		LINECALLSTATE_CONNECTED |
		LINECALLSTATE_PROCEEDING |
		LINECALLSTATE_DISCONNECTED |
		LINECALLSTATE_UNKNOWN;
	lpLineInfo->dwOrigin = LINECALLORIGIN_OUTBOUND;
	lpLineInfo->dwReason = LINECALLREASON_DIRECT;
	lpLineInfo->dwCallerIDFlags = 0;
	lpLineInfo->dwCalledIDFlags = 0;
	lpLineInfo->dwConnectedIDFlags =
		lpLineInfo->dwRedirectionIDFlags =
		lpLineInfo->dwRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;

	if (lpLineInfo->dwNeededSize > lpLineInfo->dwTotalSize)
		return lResult;

	// fill variable data
	char *ptr = (char*)(lpLineInfo + 1);
	char *base = (char*)(lpLineInfo);

	// fill caller id
	std::string idx = "call_caller_number";
	try
	{
		std::lock_guard<std::mutex> guard(pLine->mtx);
		if (pLine->msgData.find(idx) != pLine->msgData.end())
		{
			std::string s = pLine->msgData[idx];
			stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
			auto a = myconv.from_bytes(s);
			memcpy(ptr, a.data(), a.size() * 2);
			lpLineInfo->dwCallerIDSize = (s.size() + 1) * 2;
			lpLineInfo->dwCallerIDFlags |= LINECALLPARTYID_ADDRESS;
			lpLineInfo->dwCallerIDOffset = ptr - base;
			ptr += s.size() * 2;
			*((wchar_t*)ptr) = 0;
			ptr += 2;
			lpLineInfo->dwUsedSize += (s.size() + 1) * 2;
		}
	}
	catch (const std::exception &ex)
	{
		DBGOUT((3, "TSPI_lineGetCallInfo: exception. %s", ex.what()));
	}

	// fill caller name
	idx = "call_caller_name";
	try
	{
		std::lock_guard<std::mutex> guard(pLine->mtx);
		if (pLine->msgData.find(idx) != pLine->msgData.end())
		{
			std::string s = pLine->msgData[idx];
			stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
			auto a = myconv.from_bytes(s);
			if (a.empty())
				a = L"Test name #1";
			memcpy(ptr, a.data(), a.size() * 2);
			lpLineInfo->dwCallerIDNameSize = (s.size() + 1) * 2;
			lpLineInfo->dwCallerIDFlags |= LINECALLPARTYID_NAME;
			lpLineInfo->dwCallerIDNameOffset = ptr - base;
			ptr += s.size() * 2;
			*((wchar_t*)ptr) = 0;
			ptr += 2;
			lpLineInfo->dwUsedSize += (s.size() + 1) * 2;
		}
	}
	catch (const std::exception &ex)
	{
		DBGOUT((3, "TSPI_lineGetCallInfo: exception. %s", ex.what()));
	}
	if (lpLineInfo->dwCallerIDFlags == 0)
		lpLineInfo->dwCallerIDFlags = LINECALLPARTYID_UNKNOWN;

	// fill callee id
	idx = "call_callee_number";
	try
	{
		std::lock_guard<std::mutex> guard(pLine->mtx);
		if (pLine->msgData.find(idx) != pLine->msgData.end())
		{
			std::string s = pLine->msgData[idx];
			stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
			auto a = myconv.from_bytes(s);
			memcpy(ptr, a.data(), a.size() * 2);
			lpLineInfo->dwCalledIDSize = (s.size() + 1) * 2;
			lpLineInfo->dwCalledIDFlags |= LINECALLPARTYID_ADDRESS;
			lpLineInfo->dwCalledIDOffset = ptr - base;
			ptr += s.size() * 2;
			*((wchar_t*)ptr) = 0;
			ptr += 2;
			lpLineInfo->dwUsedSize += (s.size() + 1) * 2;
		}
	}
	catch (const std::exception &ex)
	{
		DBGOUT((3, "TSPI_lineGetCallInfo: exception. %s", ex.what()));
	}

	// fill callee name
	idx = "call_caller_name";
	try
	{
		std::lock_guard<std::mutex> guard(pLine->mtx);
		if (pLine->msgData.find(idx) != pLine->msgData.end())
		{
			std::string s = pLine->msgData[idx];
			stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
			auto a = myconv.from_bytes(s);
			if (a.empty())
				a = L"Test name #1";
			memcpy(ptr, a.data(), a.size() * 2);
			lpLineInfo->dwCalledIDNameSize = (s.size() + 1) * 2;
			lpLineInfo->dwCalledIDFlags |= LINECALLPARTYID_NAME;
			lpLineInfo->dwCalledIDNameOffset = ptr - base;
			ptr += s.size() * 2;
			*((wchar_t*)ptr) = 0;
			ptr += 2;
			lpLineInfo->dwUsedSize += (s.size() + 1) * 2;
		}
	}
	catch (const std::exception &ex)
	{
		DBGOUT((3, "TSPI_lineGetCallInfo: exception. %s", ex.what()));
	}
	if (lpLineInfo->dwCalledIDFlags == 0)
		lpLineInfo->dwCalledIDFlags = LINECALLPARTYID_UNKNOWN;

	return lResult;
}


LONG
TSPIAPI
TSPI_lineGetCallStatus(
	HDRVCALL            hdCall,
	LPLINECALLSTATUS    lpLineStatus
)
{
	LONG        lResult = 0;
	PDRVLINE    pLine = (PDRVLINE)hdCall;

	lpLineStatus->dwNeededSize =
		lpLineStatus->dwUsedSize = sizeof(LINECALLSTATUS);

	DBGOUT((3, "TSPI_lineGetCallStatus. pLine->dwCallState = %08x", pLine->dwCallState));
	lpLineStatus->dwCallState = pLine->dwCallState;

	if (pLine->dwCallState != LINECALLSTATE_IDLE)
	{
		lpLineStatus->dwCallFeatures = LINECALLFEATURE_DROP;
	}

	return lResult;
}


LONG
TSPIAPI
TSPI_lineGetDevCaps(
	DWORD           dwDeviceID,
	DWORD           dwTSPIVersion,
	DWORD           dwExtVersion,
	LPLINEDEVCAPS   lpLineDevCaps
)
{
	DBGOUT((3, "TSPI_lineGetDevCaps: dwDeviceID = %d, dwTSPIVersion = %08x, dwExtVersion = %08x, gdwLineDeviceIDBase = %d", dwDeviceID, dwTSPIVersion, dwExtVersion, gdwLineDeviceIDBase));
	LONG            lResult = 0;
	static WCHAR    szProviderInfo[] = L"nuacom service provider";
	static WCHAR    szLineName[100];// = L"nuacom service provider line 1";
	auto lines = GetLinesInfo();
	int localIndex = dwDeviceID - gdwLineDeviceIDBase;
	DBGOUT((3, "TSPI_lineGetDevCaps: after GetLinesInfo(): lines.size() = %d, localIndex = %d", lines.size(), localIndex));

	swprintf(szLineName, L"nuacom service provider line %d", localIndex + 1);

#define PROVIDER_INFO_SIZE (sizeof szProviderInfo)

	lpLineDevCaps->dwNeededSize = sizeof(LINEDEVCAPS) + PROVIDER_INFO_SIZE +
		(MAX_DEV_NAME_LENGTH + 1) * sizeof(WCHAR);

	if (lpLineDevCaps->dwTotalSize >= lpLineDevCaps->dwNeededSize)
	{
		lpLineDevCaps->dwUsedSize = lpLineDevCaps->dwNeededSize;

		lpLineDevCaps->dwProviderInfoSize = PROVIDER_INFO_SIZE;
		lpLineDevCaps->dwProviderInfoOffset = sizeof(LINEDEVCAPS);

		StringCbCopyW((WCHAR *)(lpLineDevCaps + 1), lpLineDevCaps->dwTotalSize, szProviderInfo);

		lpLineDevCaps->dwLineNameSize = sizeof szLineName;
		lpLineDevCaps->dwLineNameOffset = sizeof(LINEDEVCAPS) +
			PROVIDER_INFO_SIZE;
		char *ptr = (char*)lpLineDevCaps + lpLineDevCaps->dwLineNameOffset;

		StringCbCopyW((WCHAR *)(ptr), lpLineDevCaps->dwTotalSize, szLineName);
	}
	else
	{
		lpLineDevCaps->dwUsedSize = sizeof(LINEDEVCAPS);
	}

	lpLineDevCaps->dwStringFormat = STRINGFORMAT_ASCII;
	lpLineDevCaps->dwAddressModes = LINEADDRESSMODE_ADDRESSID;
	lpLineDevCaps->dwNumAddresses = 1;
	lpLineDevCaps->dwBearerModes = LINEBEARERMODE_VOICE;
	lpLineDevCaps->dwMaxRate = 9600;
	lpLineDevCaps->dwMediaModes = LINEMEDIAMODE_INTERACTIVEVOICE |
		LINEMEDIAMODE_DATAMODEM;


	//
	// Specify  LINEDEVCAPFLAGS_MSP so tapi knows to call lineMSPIdentify for
	// the MSP CLSID. Note: version must be 3.0 or higher
	//

	lpLineDevCaps->dwDevCapFlags = LINEDEVCAPFLAGS_CLOSEDROP |
		LINEDEVCAPFLAGS_DIALBILLING |
		LINEDEVCAPFLAGS_DIALQUIET |
		LINEDEVCAPFLAGS_DIALDIALTONE;

	lpLineDevCaps->dwMaxNumActiveCalls = 1;
	lpLineDevCaps->dwRingModes = 1;
	lpLineDevCaps->dwLineFeatures = LINEFEATURE_MAKECALL;

	return lResult;
}


LONG
TSPIAPI
TSPI_lineGetID(
	HDRVLINE    hdLine,
	DWORD       dwAddressID,
	HDRVCALL    hdCall,
	DWORD       dwSelect,
	LPVARSTRING lpDeviceID,
	LPCWSTR     lpszDeviceClass,
	HANDLE      hTargetProcess
)
{
	DBGOUT((3, "TSPI_lineGetID: hdLine = %p, dwAddressID = %d, hdCall = %p, dwSelect = %d, lpszDeviceClass = %S",
		hdLine, dwAddressID, hdCall, dwSelect, lpszDeviceClass));

	DWORD       dwNeededSize = sizeof(VARSTRING) + sizeof(DWORD);
	LONG        lResult = 0;
	PDRVLINE    pLine = (dwSelect == LINECALLSELECT_CALL ?
		(PDRVLINE)hdCall : (PDRVLINE)hdLine);


	if (lstrcmpiW(lpszDeviceClass, L"tapi/line") == 0)
	{
		if (lpDeviceID->dwTotalSize < dwNeededSize)
		{
			lpDeviceID->dwUsedSize = 3 * sizeof(DWORD);
		}
		else
		{
			lpDeviceID->dwUsedSize = dwNeededSize;

			lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;
			lpDeviceID->dwStringSize = sizeof(DWORD);
			lpDeviceID->dwStringOffset = sizeof(VARSTRING);

			*((LPDWORD)(lpDeviceID + 1)) = pLine->dwDeviceID;
		}

		lpDeviceID->dwNeededSize = dwNeededSize;
	}
	else
	{
		lResult = LINEERR_NODEVICE;
	}

	return lResult;
}


LONG
TSPIAPI
TSPI_lineGetLineDevStatus(
	HDRVLINE        hdLine,
	LPLINEDEVSTATUS lpLineDevStatus
)
{
	DBGOUT((3, "TSPI_lineGetLineDevStatus"));

	LONG        lResult = 0;
	PDRVLINE    pLine = (PDRVLINE)hdLine;


	lpLineDevStatus->dwUsedSize =
		lpLineDevStatus->dwNeededSize = sizeof(LINEDEVSTATUS);

	lpLineDevStatus->dwNumActiveCalls = (pLine->htCall ? 1 : 0);
	//lpLineDevStatus->dwLineFeatures =
	lpLineDevStatus->dwDevStatusFlags = LINEDEVSTATUSFLAGS_CONNECTED |
		LINEDEVSTATUSFLAGS_INSERVICE;
	return lResult;
}


LONG
TSPIAPI
TSPI_lineGetNumAddressIDs(
	HDRVLINE    hdLine,
	LPDWORD     lpdwNumAddressIDs
)
{
	DBGOUT((3, "TSPI_lineGetNumAddressIDs"));
	LONG        lResult = 0;
	PDRVLINE    pLine = (PDRVLINE)hdLine;

	//
	// We only support 1 address (id=0)
	//

	*lpdwNumAddressIDs = 1;
	return lResult;
}


LONG
TSPIAPI
TSPI_lineMakeCall(
	DRV_REQUESTID       dwRequestID,
	HDRVLINE            hdLine,
	HTAPICALL           htCall,
	LPHDRVCALL          lphdCall,
	LPCWSTR             lpszDestAddress,
	DWORD               dwCountryCode,
	LPLINECALLPARAMS    const lpCallParams
)
{
	DBGOUT((3, "TSPI_lineMakeCall: dwRequestID = %08x, lpszDestAddress = %S, dwCountryCode = %d",
		dwRequestID, lpszDestAddress, dwCountryCode));

	PDRVLINE pLine = (PDRVLINE)hdLine;

	//
	// Check to see if there's already another call
	//

	if (pLine->htCall)
	{
		DBGOUT((3, "TSPI_lineMakeCall: pLine->htCall = %p, aborting", pLine->htCall));
		(*gpfnCompletionProc)(dwRequestID, LINEERR_CALLUNAVAIL);
		return dwRequestID;
	}

	//
	// Init the data structure & tell tapi our handle to the call
	//

	pLine->htCall = htCall;
	pLine->bDropInProgress = FALSE;
	pLine->dwMediaMode = (lpCallParams ? lpCallParams->dwMediaMode :
		LINEMEDIAMODE_INTERACTIVEVOICE);

	*lphdCall = (HDRVCALL)pLine;

	std::string session_token = pLine->session_token;
	std::string src_number = pLine->extension;
	std::wstring dst_number;
	std::string status_message;

	if (lpszDestAddress[0] == L'P' || lpszDestAddress[0] == L'T')
		dst_number = lpszDestAddress + 1;
	else
		dst_number = lpszDestAddress;

	stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	pLine->destAddress = myconv.to_bytes(dst_number);

	DBGOUT((3, "TSPI_lineMakeCall: src_number = %S, dst_number = %S", src_number.c_str(), dst_number.c_str()));
	bool b = MakeCall(session_token, src_number, dst_number, status_message);
	if (b)
	{
		DBGOUT((3, "TSPI_lineMakeCall: MakeCall returned %s", status_message.c_str()));
		(*gpfnCompletionProc)(dwRequestID, 0);
		if (status_message == "success")
		{
			SetCallState(pLine, LINECALLSTATE_DIALING, 0);
		}
		else
		{
			SetCallState(pLine, LINECALLSTATE_IDLE, 0);
		}
	}
	else
	{
		DBGOUT((1, "TSPI_lineMakeCall: MakeCall failed: %s", status_message.c_str()));
		(*gpfnCompletionProc)(dwRequestID, LINEERR_OPERATIONFAILED);
	}
	return dwRequestID;
}


LONG
TSPIAPI
TSPI_lineNegotiateTSPIVersion(
	DWORD   dwDeviceID,
	DWORD   dwLowVersion,
	DWORD   dwHighVersion,
	LPDWORD lpdwTSPIVersion
)
{
	DBGOUT((3, "TSPI_lineNegotiateTSPIVersion, dwDeviceID = %d, dwLowVersion = %08x, dwHighVersion = %08x",
		dwDeviceID, dwLowVersion, dwHighVersion));

	*lpdwTSPIVersion = TAPI_CURRENT_VERSION;
	return 0;
}


LONG
TSPIAPI
TSPI_lineOpen(
	DWORD       dwDeviceID,
	HTAPILINE   htLine,
	LPHDRVLINE  lphdLine,
	DWORD       dwTSPIVersion,
	LINEEVENT   lpfnEventProc
)
{
	int localIndex = dwDeviceID - gdwLineDeviceIDBase;
	DBGOUT((3, "TSPI_lineOpen, dwDeviceID = %d, dwTSPIVersion = %08x, gdwLineDeviceIDBase = %d, localIndex = %d",
		dwDeviceID, dwTSPIVersion, gdwLineDeviceIDBase, localIndex));
	LONG        lResult = 0;
	PDRVLINE    pLine;
	try
	{
		pLine = new DRVLINE();
		pLine->htLine = htLine;
		pLine->pfnEventProc = lpfnEventProc;
		pLine->dwDeviceID = dwDeviceID;
		// Get line info from registry
		auto lines = GetLinesInfo();
		// Do the login
		std::string userName = lines[localIndex].userName;
		std::string password = lines[localIndex].password;
		std::string extension = lines[localIndex].extension;
		std::string session_token;
		DBGOUT((3, "TSPI_lineOpen, userName = %s, password = %s, extension = %s",
			userName.c_str(), password.c_str(), extension.c_str()));
		bool b = GetSessionToken(userName, password, session_token);
		if (!b)
		{
			DBGOUT((3, "TSPI_lineOpen: error in GetSessionToken: %s", session_token.c_str()));
			delete pLine;
			return LINEERR_OPERATIONFAILED;
		}
		pLine->session_token = session_token;
		pLine->extension = extension;
		DBGOUT((3, "TSPI_lineOpen: calling ConnectToWebsocket: session_token = %s", session_token.c_str()));
		ConnectToWebsocket(pLine->session_token.c_str(), pLine, &pLine->socketHandle);
		DBGOUT((3, "TSPI_lineOpen: calling ConnectToWebsocket done: pLine->socketHandle = %p", pLine->socketHandle));

		*lphdLine = (HDRVLINE)pLine;

		lResult = 0;
	}
	catch (const std::exception&)
	{
		lResult = LINEERR_NOMEM;
	}

	return lResult;
}


LONG
TSPIAPI
TSPI_lineSetDefaultMediaDetection(
	HDRVLINE    hdLine,
	DWORD       dwMediaModes
)
{
	DBGOUT((3, "TSPI_lineSetDefaultMediaDetection, dwMediaModes = %08x", dwMediaModes));
	return 0;
}


#if 0
//
// ------------------------- TSPI_providerXxx funcs ---------------------------
//

LONG
TSPIAPI
TSPI_providerConfig(
	HWND    hwndOwner,
	DWORD   dwPermanentProviderID
)
{
	//
	// Although this func is never called by TAPI v2.0, we export
	// it so that the Telephony Control Panel Applet knows that it
	// can configure this provider via lineConfigProvider(),
	// otherwise Telephon.cpl will not consider it configurable
	//

	return 0;
}


LONG
TSPIAPI
TSPI_providerGenericDialogData(
	DWORD_PTR           dwObjectID,
	DWORD               dwObjectType,
	LPVOID              lpParams,
	DWORD               dwSize
)
{
	LONG        lResult = 0;
#if DBG
	FUNC_PARAM  params[] =
	{
		{ "dwObjectID",     dwObjectID      },
		{ "dwObjectType",   dwObjectType    },
		{ "lpParams",       lpParams        },
		{ "dwSize",         dwSize          }
	};
	FUNC_INFO   info =
	{
		"TSPI_providerGenericDialogData",
		4,
		params
	};
#endif


	Prolog(&info);
	return (Epilog(&info, lResult));
}
#endif


LONG
TSPIAPI
TSPI_providerInit(
	DWORD               dwTSPIVersion,
	DWORD               dwPermanentProviderID,
	DWORD               dwLineDeviceIDBase,
	DWORD               dwPhoneDeviceIDBase,
	DWORD_PTR           dwNumLines,
	DWORD_PTR           dwNumPhones,
	ASYNC_COMPLETION    lpfnCompletionProc,
	LPDWORD             lpdwTSPIOptions
)
{
	DBGOUT((3, "TSPI_providerInit. dwTSPIVersion = %08x, dwPermanentProviderID = %d, dwLineDeviceIDBase = %d, dwPhoneDeviceIDBase = %d, dwNumLines = %d, dwNumPhones = %d",
		dwTSPIVersion, dwPermanentProviderID, dwLineDeviceIDBase, dwPhoneDeviceIDBase, dwNumLines, dwNumPhones));
	gdwLineDeviceIDBase = dwLineDeviceIDBase;
	gpfnCompletionProc = lpfnCompletionProc;
	*lpdwTSPIOptions = LINETSPIOPTION_NONREENTRANT;
	return 0;
}


LONG
TSPIAPI
TSPI_providerInstall(
	HWND    hwndOwner,
	DWORD   dwPermanentProviderID
)
{
	DBGOUT((3, "TSPI_providerInstall. TODO: Do something to install provider"));
	return 0;
}


LONG
TSPIAPI
TSPI_providerRemove(
	HWND    hwndOwner,
	DWORD   dwPermanentProviderID
)
{
	DBGOUT((3, "TSPI_providerRemove. TODO: Do something to uninstall provider"));
	return 0;
}


LONG
TSPIAPI
TSPI_providerShutdown(
	DWORD   dwTSPIVersion,
	DWORD   dwPermanentProviderID
)
{
	DBGOUT((3, "TSPI_providerShutdown. TODO: Do something to shut provider down"));
	return 0;
}


LONG
TSPIAPI
TSPI_providerEnumDevices(
	DWORD       dwPermanentProviderID,
	LPDWORD     lpdwNumLines,
	LPDWORD     lpdwNumPhones,
	HPROVIDER   hProvider,
	LINEEVENT   lpfnLineCreateProc,
	PHONEEVENT  lpfnPhoneCreateProc
)
{
	DBGOUT((3, "TSPI_providerEnumDevices: dwPermanentProviderID = %d", dwPermanentProviderID));
	auto lines = GetLinesInfo();
	DBGOUT((3, "TSPI_providerEnumDevices: after GetLinesInfo(): lines.size() = %d", lines.size()));
	*lpdwNumLines = lines.size();
	//*lpdwNumLines = 1;
	*lpdwNumPhones = 0;
	return 0;
}


LONG
TSPIAPI
TSPI_providerUIIdentify(
	LPWSTR   lpszUIDLLName
)
{
	LONG lResult = 0;
	auto n = GetModuleFileNameW(hInst, lpszUIDLLName, MAX_PATH);
	if (n == 0)
	{
		auto dwErr = GetLastError();
		DBGOUT((1, "TSPI_providerUIIdentify: error in GetModuleFileName. dwErr = %d", dwErr));
		lResult = LINEERR_OPERATIONFAILED;
	}

	return lResult;
}


//
// ---------------------------- TUISPI_xxx funcs ------------------------------
//

LONG
TSPIAPI
TUISPI_lineConfigDialog(
	TUISPIDLLCALLBACK   lpfnUIDLLCallback,
	DWORD               dwDeviceID,
	HWND                hwndOwner,
	LPCWSTR             lpszDeviceClass
)
{
	DBGOUT((3, "TUISPI_lineConfigDialog"));
	LONG        lResult = LINEERR_BADDEVICEID;

	return lResult;
}


///////////////////////////////////////////////////////////////////////////
// TUISPI_lineConfigDialogEdit
//
// This function causes the provider of the specified line device to
// display a modal dialog to allow the user to configure parameters
// related to the line device.  The parameters editted are NOT the
// current device parameters, rather the set retrieved from the
// 'TSPI_lineGetDevConfig' function (lpDeviceConfigIn), and are returned
// in the lpDeviceConfigOut parameter.
//
extern "C"
LONG TSPIAPI TUISPI_lineConfigDialogEdit(TUISPIDLLCALLBACK lpfnDLLCallback,
	DWORD dwDeviceID, HWND hwndOwner,
	LPCWSTR lpszDeviceClass, LPVOID const lpDeviceConfigIn, DWORD dwSize,
	LPVARSTRING lpDeviceConfigOut)
{
	DBGOUT((3, "TUISPI_lineConfigDialogEdit"));
	LONG        lResult = LINEERR_BADDEVICEID;

	return lResult;
}

#if 0
LONG
TSPIAPI
TUISPI_providerConfig(
	TUISPIDLLCALLBACK   lpfnUIDLLCallback,
	HWND                hwndOwner,
	DWORD               dwPermanentProviderID
)
{
	LONG        lResult = 0;
#if DBG
	FUNC_PARAM  params[] =
	{
		{ "lpfnUIDLLCallback",      lpfnUIDLLCallback },
		{ gszhwndOwner,             hwndOwner    },
		{ gszdwPermanentProviderID, dwPermanentProviderID   }
	};
	FUNC_INFO   info =
	{
		"TUISPI_providerConfig",
		3,
		params
	};
#endif


	Prolog(&info);

	DialogBoxParam(
		ghInst,
		MAKEINTRESOURCE(IDD_DIALOG1),
		hwndOwner,
		(DLGPROC)ConfigDlgProc,
		0
	);

	return (Epilog(&info, lResult));
}
#endif


LONG
TSPIAPI
TUISPI_providerInstall(
	TUISPIDLLCALLBACK   lpfnUIDLLCallback,
	HWND                hwndOwner,
	DWORD               dwPermanentProviderID
)
{
	DBGOUT((3, "TUISPI_providerInstall. TODO: Do something to install provider"));
	return 0;
}


LONG
TSPIAPI
TUISPI_providerRemove(
	TUISPIDLLCALLBACK   lpfnUIDLLCallback,
	HWND                hwndOwner,
	DWORD               dwPermanentProviderID
)
{
	DBGOUT((3, "TUISPI_providerRemove. TODO: Do something to uninstall provider"));
	return 0;
}


#if 0
#pragma warning (default:4047)


//
// ---------------------- Misc private support routines -----------------------
//



void
PASCAL
EnableChildren(
	HWND    hwnd,
	BOOL    bEnable
)
{
	int i;
	static int aiControlIDs[] =
	{
		IDC_DEVICES,
		IDC_NAME,
		IDC_PORT,
		IDC_COMMANDS,
		IDC_REMOVE,
		0
	};


	for (i = 0; aiControlIDs[i]; i++)
	{
		EnableWindow(GetDlgItem(hwnd, aiControlIDs[i]), bEnable);
	}
}


void
PASCAL
SelectDevice(
	HWND    hwnd,
	LRESULT     iDevice
)
{
	SendDlgItemMessage(hwnd, IDC_DEVICES, LB_SETCURSEL, iDevice, 0);
	PostMessage(hwnd, WM_COMMAND, IDC_DEVICES | (LBN_SELCHANGE << 16), 0);
}


BOOL
CALLBACK
ConfigDlgProc(
	HWND    hwnd,
	UINT    msg,
	WPARAM  wParam,
	LPARAM  lParam
)
{
	static  HKEY    hAtspKey;

	DWORD   dwDataSize;
	DWORD   dwDataType;
	HRESULT hr;

	switch (msg)
	{
	case WM_INITDIALOG:
	{
		char   *pBuf;
		DWORD   i, iNumLines;


		//
		// Create or open our configuration key in the registry.  If the
		// create fails it may well be that the current user does not
		// have write access to this portion of the registry, so we'll
		// just show a "read only" dialog and not allow user to make any
		// changes
		//

		{
			LONG    lResult;
			DWORD   dwDisposition;


			if ((lResult = RegCreateKeyEx(
				HKEY_LOCAL_MACHINE,
				gszAtspKey,
				0,
				"",
				REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS,
				(LPSECURITY_ATTRIBUTES)NULL,
				&hAtspKey,
				&dwDisposition

			)) != ERROR_SUCCESS)
			{
				DBGOUT((
					3,
					"RegCreateKeyEx(%s,ALL_ACCESS) failed, err=%d",
					gszAtspKey,
					lResult
					));

				if ((lResult = RegOpenKeyEx(
					HKEY_LOCAL_MACHINE,
					gszAtspKey,
					0,
					KEY_QUERY_VALUE,
					&hAtspKey

				)) != ERROR_SUCCESS)
				{
					DBGOUT((
						3,
						"RegOpenKeyEx(%s,ALL_ACCESS) failed, err=%d",
						gszAtspKey,
						lResult
						));

					EndDialog(hwnd, 0);
					return FALSE;
				}

				{
					int i;
					static int aiControlIDs[] =
					{
						IDC_NAME,
						IDC_PORT,
						IDC_COMMANDS,
						IDC_ADD,
						IDC_REMOVE,
						IDOK,
						0
					};


					for (i = 0; aiControlIDs[i]; i++)
					{
						EnableWindow(
							GetDlgItem(hwnd, aiControlIDs[i]),
							FALSE
						);
					}
				}
			}
		}


		//
		// Retrieve our configuration info from the registry
		//

		dwDataSize = sizeof(iNumLines);
		iNumLines = 0;

		RegQueryValueEx(
			hAtspKey,
			gszNumLines,
			0,
			&dwDataType,
			(LPBYTE)&iNumLines,
			&dwDataSize
		);

		SendDlgItemMessage(
			hwnd,
			IDC_NAME,
			EM_LIMITTEXT,
			MAX_DEV_NAME_LENGTH,
			0
		);

		SendDlgItemMessage(
			hwnd,
			IDC_COMMANDS,
			EM_LIMITTEXT,
			MAX_DEV_NAME_LENGTH,
			0
		);

		if (!(pBuf = DrvAlloc(MAX_BUF_SIZE)))
			break;

		for (i = 0; i < iNumLines; i++)
		{
			char           *p, *p2, szLineN[8];
			PDRVLINECONFIG  pLineConfig;
			LONG            lResult;

			if (!(pLineConfig = DrvAlloc(sizeof(DRVLINECONFIG))))
				break;

			hr = StringCbPrintfA(szLineN, sizeof(szLineN), "Line%d", i);
			assert_str(hr);

			dwDataSize = 256;

			lResult = RegQueryValueEx(
				hAtspKey,
				szLineN,
				0,
				&dwDataType,
				(LPBYTE)pBuf,
				&dwDataSize
			);


			//
			// If there was a problem, use the default config
			//

			if (0 != lResult)
			{
				hr = StringCbCopyA(pBuf, MAX_BUF_SIZE, gszDefLineConfigParams);
				assert_str(hr);
			}

			for (p = pBuf; *p != ','; p++);
			*p = 0;

			SendDlgItemMessage(
				hwnd,
				IDC_DEVICES,
				LB_ADDSTRING,
				0,
				(LPARAM)pBuf
			);

			SendDlgItemMessage(
				hwnd,
				IDC_DEVICES,
				LB_SETITEMDATA,
				i,
				(LPARAM)pLineConfig
			);

			p++;
			for (p2 = p; *p2 != ','; p2++);
			*p2 = 0;

			hr = StringCbCopyA(pLineConfig->szPort, sizeof(pLineConfig->szPort), p);
			assert_str(hr);


			p = p2 + 1;

			hr = StringCbCopyA(pLineConfig->szCommands, sizeof(pLineConfig->szCommands), p);
			assert_str(hr);

		}

		DrvFree(pBuf);


		//
		// Fill up the various controls with configuration options
		//

		{
			static char *aszPorts[] = { "COM1","COM2","COM3",NULL };

			for (i = 0; aszPorts[i]; i++)
			{
				SendDlgItemMessage(
					hwnd,
					IDC_PORT,
					LB_ADDSTRING,
					0,
					(LPARAM)aszPorts[i]
				);
			}
		}

		if (iNumLines == 0)
		{
			EnableChildren(hwnd, FALSE);
		}
		else
		{
			SelectDevice(hwnd, 0);
		}

		break;
	}
	case WM_COMMAND:
	{
		LRESULT             iSelection;
		PDRVLINECONFIG  pLineConfig;


		iSelection = SendDlgItemMessage(
			hwnd,
			IDC_DEVICES,
			LB_GETCURSEL,
			0,
			0
		);

		pLineConfig = (PDRVLINECONFIG)SendDlgItemMessage(
			hwnd,
			IDC_DEVICES,
			LB_GETITEMDATA,
			(WPARAM)iSelection,
			0
		);

		switch (LOWORD((DWORD)wParam))
		{
		case IDC_DEVICES:

			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				char buf[MAX_DEV_NAME_LENGTH + 1];


				SendDlgItemMessage(
					hwnd,
					IDC_DEVICES,
					LB_GETTEXT,
					iSelection,
					(LPARAM)buf
				);

				SetDlgItemText(hwnd, IDC_NAME, buf);

				SendDlgItemMessage(
					hwnd,
					IDC_PORT,
					LB_SELECTSTRING,
					(WPARAM)-1,
					(LPARAM)pLineConfig->szPort
				);

				SetDlgItemText(hwnd, IDC_COMMANDS, pLineConfig->szCommands);
			}

			break;

		case IDC_NAME:

			if ((HIWORD(wParam) == EN_CHANGE) && (iSelection != LB_ERR))
			{
				char    buf[MAX_DEV_NAME_LENGTH + 1];


				GetDlgItemText(hwnd, IDC_NAME, buf, MAX_DEV_NAME_LENGTH);

				SendDlgItemMessage(
					hwnd,
					IDC_DEVICES,
					LB_DELETESTRING,
					iSelection,
					0
				);

				SendDlgItemMessage(
					hwnd,
					IDC_DEVICES,
					LB_INSERTSTRING,
					iSelection,
					(LPARAM)buf
				);

				SendDlgItemMessage(
					hwnd,
					IDC_DEVICES,
					LB_SETCURSEL,
					iSelection,
					0
				);

				SendDlgItemMessage(
					hwnd,
					IDC_DEVICES,
					LB_SETITEMDATA,
					iSelection,
					(LPARAM)pLineConfig
				);
			}

			break;

		case IDC_PORT:

			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				iSelection = SendDlgItemMessage(
					hwnd,
					IDC_PORT,
					LB_GETCURSEL,
					0,
					0
				);

				SendDlgItemMessage(
					hwnd,
					IDC_PORT,
					LB_GETTEXT,
					iSelection,
					(LPARAM)pLineConfig->szPort
				);
			}

			break;

		case IDC_COMMANDS:

			if ((HIWORD(wParam) == EN_CHANGE) && (iSelection != LB_ERR))
			{
				GetDlgItemText(
					hwnd,
					IDC_COMMANDS,
					pLineConfig->szCommands,
					63
				);
			}

			break;

		case IDC_ADD:
		{
			LRESULT             iNumLines;
			int				i = 2;
			char            szLineName[32];
			PDRVLINECONFIG  pLineConfig;

			if (!(pLineConfig = DrvAlloc(sizeof(DRVLINECONFIG))))
				break;


			iNumLines = SendDlgItemMessage(
				hwnd,
				IDC_DEVICES,
				LB_GETCOUNT,
				0,
				0
			);

			hr = StringCbCopyA(pLineConfig->szPort, sizeof(pLineConfig->szPort), "COM1");
			assert_str(hr);


			hr = StringCbCopyA(szLineName, sizeof(szLineName), "my new line");
			assert_str(hr);


		find_unique_line_name:

			if (SendDlgItemMessage(
				hwnd,
				IDC_DEVICES,
				LB_FINDSTRING,
				(WPARAM)-1,
				(LPARAM)szLineName

			) != LB_ERR)
			{
				hr = StringCbPrintfA(szLineName, sizeof(szLineName), "my new line%d", i++);
				assert_str(hr);
				goto find_unique_line_name;
			}

			SendDlgItemMessage(
				hwnd,
				IDC_DEVICES,
				LB_ADDSTRING,
				0,
				(LPARAM)szLineName
			);

			SendDlgItemMessage(
				hwnd,
				IDC_DEVICES,
				LB_SETITEMDATA,
				iNumLines,
				(LPARAM)pLineConfig
			);

			EnableChildren(hwnd, TRUE);

			SelectDevice(hwnd, iNumLines);

			SetFocus(GetDlgItem(hwnd, IDC_NAME));

			SendDlgItemMessage(
				hwnd,
				IDC_NAME,
				EM_SETSEL,
				0,
				(LPARAM)-1
			);

			break;
		}
		case IDC_REMOVE:
		{
			LRESULT iNumLines;


			DrvFree(pLineConfig);

			iNumLines = SendDlgItemMessage(
				hwnd,
				IDC_DEVICES,
				LB_DELETESTRING,
				iSelection,
				0
			);

			if (iNumLines == 0)
			{
				SetDlgItemText(hwnd, IDC_NAME, "");
				SetDlgItemText(hwnd, IDC_COMMANDS, "");

				EnableChildren(hwnd, FALSE);
			}
			else
			{
				SelectDevice(hwnd, 0);
			}

			break;
		}
		case IDOK:
		{
			int     i;
			LRESULT	iNumLines;
			char   *pBuf;


			//
			// Update the num lines & num phones values
			//

			if (!(pBuf = DrvAlloc(256)))
				break;

			iNumLines = SendDlgItemMessage(
				hwnd,
				IDC_DEVICES,
				LB_GETCOUNT,
				0,
				0
			);

			RegSetValueEx(
				hAtspKey,
				gszNumLines,
				0,
				REG_DWORD,
				(LPBYTE)&iNumLines,
				sizeof(DWORD)
			);


			//
			// For each installed device save it's config info
			//

			for (i = 0; i < iNumLines; i++)
			{
				char szLineN[8];
				PDRVLINECONFIG pLineConfig;


				SendDlgItemMessage(
					hwnd,
					IDC_DEVICES,
					LB_GETTEXT,
					i,
					(LPARAM)pBuf
				);

				pLineConfig = (PDRVLINECONFIG)SendDlgItemMessage(
					hwnd,
					IDC_DEVICES,
					LB_GETITEMDATA,
					i,
					0
				);

				hr = StringCbPrintfA(
					pBuf + strlen(pBuf),
					MAX_BUF_SIZE - strlen(pBuf),
					",%s,%s",
					pLineConfig->szPort,
					pLineConfig->szCommands
				);
				assert_str(hr);

				hr = StringCbPrintfA(szLineN, sizeof(szLineN), "Line%d", i);
				assert_str(hr);

				RegSetValueEx(
					hAtspKey,
					szLineN,
					0,
					REG_SZ,
					(LPBYTE)pBuf,
					lstrlen(pBuf) + 1
				);

				DrvFree(pLineConfig);
			}

			DrvFree(pBuf);

			// fall thru to EndDialog...
		}
		case IDCANCEL:

			RegCloseKey(hAtspKey);
			EndDialog(hwnd, 0);
			break;

		} // switch (LOWORD((DWORD)wParam))

		break;
	}
	} // switch (msg)

	return FALSE;
}


LPVOID
PASCAL
DrvAlloc(
	DWORD dwSize
)
{
	return (LocalAlloc(LPTR, dwSize));
}


VOID
PASCAL
DrvFree(
	LPVOID lp
)
{
	LocalFree(lp);
}
#endif


void
PASCAL
SetCallState(
	PDRVLINE    pLine,
	DWORD       dwCallState,
	DWORD       dwCallStateMode
)
{
	if (dwCallState != pLine->dwCallState)
	{
		pLine->dwCallState = dwCallState;
		pLine->dwCallStateMode = dwCallStateMode;

		(*pLine->pfnEventProc)(
			pLine->htLine,
			pLine->htCall,
			LINE_CALLSTATE,
			dwCallState,
			dwCallStateMode,
			pLine->dwMediaMode
			);
	}
}


#if DBG

#if 0
void
PASCAL
Prolog(
	PFUNC_INFO  pInfo
)
{
	DWORD i;


	DBGOUT((3, "%s: enter", pInfo->lpszFuncName));

	for (i = 0; i < pInfo->dwNumParams; i++)
	{
		if (pInfo->aParams[i].dwVal &&
			pInfo->aParams[i].lpszVal[3] == 'z') // lpszVal = "lpsz..."
		{
			DBGOUT((
				3,
				"%s%s=x%lx, '%s'",
				gszTab,
				pInfo->aParams[i].lpszVal,
				pInfo->aParams[i].dwVal,
				pInfo->aParams[i].dwVal
				));
		}
		else
		{
			DBGOUT((
				3,
				"%s%s=x%lx",
				gszTab,
				pInfo->aParams[i].lpszVal,
				pInfo->aParams[i].dwVal
				));
		}
	}
}


LONG
PASCAL
Epilog(
	PFUNC_INFO  pInfo,
	LONG        lResult
)
{
	DBGOUT((3, "%s: returning x%x", pInfo->lpszFuncName, lResult));

	return lResult;
}
#endif


void
CDECL
DebugOutput(
	DWORD   dwDbgLevel,
	LPCSTR  lpszFormat,
	...
)
{
	if (dwDbgLevel <= gdwDebugLevel)
	{
		char    buf[512] = "NUACOM: ";
		DWORD pid = GetCurrentProcessId();
		DWORD tid = GetCurrentThreadId();
		va_list ap;

		sprintf(buf, "NUACOM(%d:%d): ", pid, tid);
		char *newBuf = buf + strlen(buf);
		int newbufSize = 512 - strlen(buf);

		va_start(ap, lpszFormat);

		_vsnprintf_s(newBuf, newbufSize, newbufSize - 1, lpszFormat, ap);


		strncat_s(buf, sizeof(buf), "\n", 511 - strlen(buf));
		buf[511] = '\0';

		OutputDebugString(buf);

		va_end(ap);
	}
}

#endif


#if 0
LONG
PASCAL
ProviderInstall(
	char   *pszProviderName,
	BOOL    bNoMultipleInstance
)
{
	LONG    lResult;


	//
	// If only one installation instance of this provider is
	// allowed then we want to check the provider list to see
	// if the provider is already installed
	//

	if (bNoMultipleInstance)
	{
		LONG(WINAPI *pfnGetProviderList)();
		DWORD               dwTotalSize, i;
		HINSTANCE           hTapi32;
		LPLINEPROVIDERLIST  pProviderList;
		LPLINEPROVIDERENTRY pProviderEntry;


		//
		// Load Tapi32.dll & get a pointer to the lineGetProviderList
		// func.  We don't want to statically link because this module
		// plays the part of both core SP & UI DLL, and we don't want
		// to incur the performance hit of automatically loading
		// Tapi32.dll when running as a core SP within Tapisrv.exe's
		// context.
		//

		if (!(hTapi32 = LoadLibrary("tapi32.dll")))
		{
			DBGOUT((
				1,
				"LoadLibrary(tapi32.dll) failed, err=%d",
				GetLastError()
				));

			lResult = LINEERR_OPERATIONFAILED;
			goto ProviderInstall_return;
		}

		if (!(pfnGetProviderList = (LONG(WINAPI *)())GetProcAddress(
			hTapi32,
			(LPCSTR) "lineGetProviderList"
		)))
		{
			DBGOUT((
				1,
				"GetProcAddr(lineGetProviderList) failed, err=%d",
				GetLastError()
				));

			lResult = LINEERR_OPERATIONFAILED;
			goto ProviderInstall_unloadTapi32;
		}


		//
		// Loop until we get the full provider list
		//

		dwTotalSize = sizeof(LINEPROVIDERLIST);

		goto ProviderInstall_allocProviderList;

	ProviderInstall_getProviderList:

		if ((lResult = (*pfnGetProviderList)(0x00020000, pProviderList)) != 0)
		{
			goto ProviderInstall_freeProviderList;
		}

		if (pProviderList->dwNeededSize > pProviderList->dwTotalSize)
		{
			dwTotalSize = pProviderList->dwNeededSize;

			LocalFree(pProviderList);

		ProviderInstall_allocProviderList:

			if (!(pProviderList = LocalAlloc(LPTR, dwTotalSize)))
			{
				lResult = LINEERR_NOMEM;
				goto ProviderInstall_unloadTapi32;
			}

			pProviderList->dwTotalSize = dwTotalSize;

			goto ProviderInstall_getProviderList;
		}


		//
		// Inspect the provider list entries to see if this provider
		// is already installed
		//

		pProviderEntry = (LPLINEPROVIDERENTRY)(((LPBYTE)pProviderList) +
			pProviderList->dwProviderListOffset);

		for (i = 0; i < pProviderList->dwNumProviders; i++)
		{
			char   *pszInstalledProviderName = ((char *)pProviderList) +
				pProviderEntry->dwProviderFilenameOffset,
				*p;


			//
			// Make sure pszInstalledProviderName points at <filename>
			// and not <path>\filename by walking backeards thru the
			// string searching for last '\\'
			//

			p = pszInstalledProviderName +
				lstrlen(pszInstalledProviderName) - 1;

			for (; *p != '\\' && p != pszInstalledProviderName; p--);

			pszInstalledProviderName =
				(p == pszInstalledProviderName ? p : p + 1);

			if (lstrcmpiA(pszInstalledProviderName, pszProviderName) == 0)
			{
				lResult = LINEERR_NOMULTIPLEINSTANCE;
				goto ProviderInstall_freeProviderList;
			}

			pProviderEntry++;
		}


		//
		// If here then the provider isn't currently installed,
		// so do whatever configuration stuff is necessary and
		// indicate SUCCESS
		//

		lResult = 0;


	ProviderInstall_freeProviderList:

		LocalFree(pProviderList);

	ProviderInstall_unloadTapi32:

		FreeLibrary(hTapi32);
	}
	else
	{
		//
		// Do whatever configuration stuff is necessary and return SUCCESS
		//

		lResult = 0;
	}

ProviderInstall_return:

	return lResult;
}
#endif


void
PASCAL
DropActiveCall(
	PDRVLINE    pLine
)
{
	if (pLine->htCall == NULL)
		return;
	// hangup call if any
	DBGOUT((3, "DropActiveCall: calling EndCall. localChannel = %s", pLine->localChannel.c_str()));
	std::string msg;
	bool b = EndCall(pLine->session_token, pLine->localChannel, msg);
	DBGOUT((3, "DropActiveCall: calling EndCall done. b = %d, msg = %s", b, msg.c_str()));
}
