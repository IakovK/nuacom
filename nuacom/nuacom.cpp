// nuacom.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "nuacom.h"
#include "config.h"
#include <strsafe.h>
#include "nuacomAPI.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#define MAX_BUF_SIZE 256

#include "..\common\structs.h"

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
	DBGOUT((3, "DRVLINE::ProcessObjectMessage: msg.size() = %d", msg.size()));
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
	if ((htCall == NULL || bIncomingCall) && gbHandleIncomingCalls)
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

bool DRVLINE::ConnectToServer()
{
	if (bConnected)
		return true;
	unsigned char * pszProtocolSequence = (unsigned char *)"ncalrpc";
	unsigned char * pszEndpoint = (unsigned char *)"NuacomService";
	unsigned char * pszCallbackEndpoint = (unsigned char *)"NuacomServiceCallback";
	DBGOUT((3, "DRVLINE::ConnectToServer: calling InitRpc"));
	RPC_STATUS status = rpcw.Init(pszProtocolSequence, pszEndpoint);
	if (status != RPC_S_OK)
	{
		DBGOUT((3, "DRVLINE::ConnectToServer: rpcw.Init failed. status = %d", status));
		return false;
	}
	DBGOUT((3, "DRVLINE::ConnectToServer: calling rpcw.ConnectToServer. this = %p", this));
	status = rpcw.ConnectToServer(pszCallbackEndpoint, this, dwDeviceID);
	if (status != RPC_S_OK)
	{
		DBGOUT((3, "DRVLINE::ConnectToServer: rpcw.ConnectToServer failed. status = %d", status));
		return false;
	}
	bConnected = true;
	return true;
}

void DRVLINE::ProcessCallMessage(const std::string &msgId, const std::map<std::string, IMessageHolder*> &msg)
{
	DBGOUT((3, "DRVLINE::ProcessCallMessage: msg.size() = %d", msg.size()));
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
				RPC_STATUS status = rpcw.Hangup((char*)localChannel.c_str());
				return;
			}
			bIncomingCall = TRUE;
		}

		DBGOUT((3, "DRVLINE::ProcessCallMessage: calling SetCallState"));
		auto status = msgData["call_status"];
		if (status == "ringing")
			SetCallState(this, LINECALLSTATE_RINGBACK, 0);
		else if (status == "connected")
			SetCallState(this, LINECALLSTATE_CONNECTED, 0);
		else if (status == "completed")
		{
			SetCallState(this, LINECALLSTATE_DISCONNECTED, 0);
			DropActiveCall(this);
			SetCallState(this, LINECALLSTATE_IDLE, 0);
		}
	}
	catch (const std::exception &ex)
	{
		DBGOUT((3, "DRVLINE::ProcessCallMessage: exception: %s", ex.what()));
	}
}

void DRVLINE::ProcessMessage(IMessageHolder *pmh)
{
	DBGOUT((3, "DRVLINE::ProcessMessage: pmh = %p", pmh));
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

LONG
TSPIAPI
TSPI_lineClose(
	HDRVLINE    hdLine
)
{
	DBGOUT((3, "TSPI_lineClose: hdLine = %p", hdLine));
	LONG        lResult = 0;
	try
	{
		PDRVLINE pd = (PDRVLINE)hdLine;
		if (pd->bConnected)
		{
			DBGOUT((3, "TSPI_lineClose: calling rpcw.Disconnect"));
			RPC_STATUS status = pd->rpcw.Disconnect();
			DBGOUT((3, "TSPI_lineClose: calling rpcw.Disconnect done. status = %d", status));
		}
		delete pd;
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
	DBGOUT((3, "TSPI_lineCloseCall: hdCall = %p", hdCall));
	PDRVLINE    pLine = (PDRVLINE)hdCall;

	//
	// Note that in TAPI 2.0 TSPI_lineCloseCall can get called
	// without TSPI_lineDrop ever being called, so we need to
	// be prepared for either case.
	//

	DropActiveCall(pLine);
	pLine->htCall = NULL;
	pLine->bIncomingCall = FALSE;
	pLine->msgData.clear();
	return 0;
}


LONG
TSPIAPI
TSPI_lineDrop(
	DRV_REQUESTID   dwRequestID,
	HDRVCALL        hdCall,
	LPCSTR          lpsUserUserInfo,
	DWORD           dwSize
)
{
	DBGOUT((3, "TSPI_lineDrop: dwRequestID = %08x, dwSize = %d, hdCall = %p", dwRequestID, dwSize, hdCall));

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
	DBGOUT((3, "TSPI_lineGetAddressStatus, hdLine = %p", hdLine));

	LONG        lResult = 0;
	PDRVLINE    pLine = (PDRVLINE)hdLine;


	lpAddressStatus->dwNeededSize =
		lpAddressStatus->dwUsedSize = sizeof(LINEADDRESSSTATUS);

	lpAddressStatus->dwNumActiveCalls = (pLine->htCall ? 1 : 0);
	lpAddressStatus->dwAddressFeatures = LINEADDRFEATURE_MAKECALL;

	return lResult;
}


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
	if (pLine->bIncomingCall)
		lpLineInfo->dwOrigin = LINECALLORIGIN_INBOUND;
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

	{
		std::lock_guard<std::mutex> guard(pLine->mtx);
		std::string idx;
		// fill caller id
		if (gbIncomingCallsLocal)
			idx = "call_caller_number_local";
		else
			idx = "call_caller_number";
		try
		{
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

	DBGOUT((3, "TSPI_lineGetCallStatus: pLine = %p, pLine->dwCallState = %08x", pLine, pLine->dwCallState));
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
	DBGOUT((3, "TSPI_lineGetDevCaps: dwDeviceID = %d, dwTSPIVersion = %08x, dwExtVersion = %08x, gdwLineDeviceIDBase = %d",
		dwDeviceID, dwTSPIVersion, dwExtVersion, gdwLineDeviceIDBase));
	LONG            lResult = 0;
	static WCHAR    szProviderInfo[] = L"nuacom service provider";
	static WCHAR    szLineName[100];// = L"nuacom service provider line 1";
	//auto lines = GetLinesInfo();
	int localIndex = dwDeviceID - gdwLineDeviceIDBase;
	//DBGOUT((3, "TSPI_lineGetDevCaps: after GetLinesInfo(): lines.size() = %d, localIndex = %d", lines.size(), localIndex));

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
	lpLineDevCaps->dwPermanentLineID = MAKELONG((WORD)gdwPermanentProviderID, localIndex + 1);

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
	DBGOUT((3, "TSPI_lineGetLineDevStatus: hdLine = %p", hdLine));

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
	DBGOUT((3, "TSPI_lineGetNumAddressIDs: hdLine = %p", hdLine));
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
	DBGOUT((3, "TSPI_lineMakeCall: dwRequestID = %08x, hdLine = %p, lpszDestAddress = %S, dwCountryCode = %d",
		dwRequestID, hdLine, lpszDestAddress, dwCountryCode));

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

	std::wstring dst_number;

	if (lpszDestAddress[0] == L'P' || lpszDestAddress[0] == L'T')
		dst_number = lpszDestAddress + 1;
	else
		dst_number = lpszDestAddress;

	stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	std::string destAddress = myconv.to_bytes(dst_number);

	BOOL bSuccess;
	char statusMessage[255];
	RPC_STATUS status = pLine->rpcw.MakeCall((char*)destAddress.c_str(), (char*)statusMessage, 255, &bSuccess);
	std::string status_message = statusMessage;
	if (status != RPC_S_OK)
	{
		DBGOUT((1, "TSPI_lineMakeCall: rpcw.MakeCall failed: status = %d", status));
		(*gpfnCompletionProc)(dwRequestID, LINEERR_OPERATIONFAILED);
		return dwRequestID;
	}
	if (bSuccess)
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

		DBGOUT((3, "TSPI_lineOpen: calling pLine->ConnectToServer, pLine = %p", pLine));
		bool b = pLine->ConnectToServer();
		if (!b)
		{
			DBGOUT((3, "TSPI_lineOpen: error in pLine->ConnectToServer"));
			return LINEERR_OPERATIONFAILED;
		}
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
	return lResult;
}


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
	gdwPermanentProviderID = dwPermanentProviderID;
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
TSPI_lineNegotiateExtVersion(
	DWORD               dwDeviceID,
	DWORD               dwTSPIVersion,
	DWORD               dwLowVersion,
	DWORD               dwHighVersion,
	LPDWORD             lpdwExtVersion
)
{
	DBGOUT((3, "TSPI_lineNegotiateExtVersion: dwDeviceID = %d, dwTSPIVersion = %08x, dwLowVersion = %08x, dwHighVersion = %08x",
		dwDeviceID, dwTSPIVersion, dwLowVersion, dwHighVersion));
	*lpdwExtVersion = dwHighVersion;
	return 0;
}

namespace ssio
{
	message::ptr from_json(rapidjson::Value const& value, std::vector<std::shared_ptr<const std::string>> const& buffers);
}

class MessageHolder : public IMessageHolder
{
	ssio::message::ptr _m;
	mutable std::vector<MessageHolder> children;
	mutable std::vector<std::string> keys;
	mutable std::vector<MessageHolder> values;
	mutable bool keys_retrieved;
public:
	MessageHolder(ssio::message::ptr m)
		:_m(m)
		, keys_retrieved(false)
	{
	}
	virtual flag get_flag() const
	{
		return (flag)(int)_m->get_flag();
	}
	virtual int64_t get_int() const
	{
		return _m->get_int();
	}
	virtual double get_double() const
	{
		return _m->get_double();
	}
	virtual void get_string(void **ptr, int *length)
	{
		auto &s = _m->get_string();
		*((const char**)ptr) = s.data();
		*length = s.length();
	}
	virtual void get_binary(void **ptr, int *length)
	{
		auto &s = _m->get_binary();
		*((const char**)ptr) = s->data();
		*length = s->length();
	}
	virtual int get_array_length() const
	{
		auto v = _m->get_vector();
		return v.size();
	}
	virtual IMessageHolder *get_array_elt(int index) const
	{
		auto v = _m->get_vector();
		MessageHolder mh(v[index]);
		children.push_back(mh);
		auto &v1 = *children.rbegin();
		return &v1;
	}
	virtual int get_object_length() const
	{
		auto v = _m->get_map();
		return v.size();
	}
	virtual void get_key(int index, void **ptr, int *length) const
	{
		if (!keys_retrieved)
		{
			auto v = _m->get_map();
			for (auto &e : v)
			{
				keys.push_back(e.first);
				values.push_back(MessageHolder(e.second));
			}
		}
		keys_retrieved = true;
		auto &s = keys[index];
		*((const char**)ptr) = s.data();
		*length = s.length();
	}
	IMessageHolder *get_data(int index) const
	{
		if (!keys_retrieved)
		{
			auto v = _m->get_map();
			for (auto &e : v)
			{
				keys.push_back(e.first);
				values.push_back(MessageHolder(e.second));
			}
		}
		keys_retrieved = true;
		auto &h = values[index];
		return &h;
	}
	virtual bool get_bool() const
	{
		return _m->get_bool();
	}
	virtual ssio::message::ptr get_message()
	{
		return _m;
	}
};

LONG
TSPIAPI
TSPI_lineDevSpecific(
	DRV_REQUESTID       dwRequestID,
	HDRVLINE            hdLine,
	DWORD               dwAddressID,
	HDRVCALL            hdCall,
	LPVOID              lpParams,
	DWORD               dwSize
)
{
	DBGOUT((3, "TSPI_lineDevSpecific: dwRequestID = %d, hdLine = %p, lpParams = %p, dwSize = %d", dwRequestID, hdLine, lpParams, dwSize));
	RPCHeader *h = (RPCHeader *)lpParams;
	DBGOUT((3, "TSPI_lineDevSpecific: h->dwTotalSize = %d, h->dwNeededSize = %d, h->dwUsedSize = %d, h->data = %p", h->dwTotalSize, h->dwNeededSize, h->dwUsedSize, h->data));
	if (h->dwCommand == SEND_STRING)
	{
		DBGOUT((3, "TSPI_lineDevSpecific: h->dwCommand == SEND_STRING"));
		stringData *sd = (stringData *)h->data;
		DBGOUT((3, "TSPI_lineDevSpecific: sd = %p, sd->callback = %p, sd->dwDataSize = %d", sd, sd->callback, sd->dwDataSize));
		rapidjson::Document doc;
		DBGOUT((3, "TSPI_lineDevSpecific: calling doc.Parse"));
		doc.Parse<0>(sd->data, sd->dwDataSize);
		DBGOUT((3, "TSPI_lineDevSpecific: calling doc.Parse done"));
		DBGOUT((3, "TSPI_lineDevSpecific: calling ssio::from_json"));
		ssio::message::ptr m = ssio::from_json(doc, std::vector<std::shared_ptr<const std::string> >());
		DBGOUT((3, "TSPI_lineDevSpecific: calling ssio::from_json done"));
		MessageHolder mh(m);
		PDRVLINE pLine = (PDRVLINE)sd->callback;
		DBGOUT((3, "TSPI_lineDevSpecific: calling pLine->ProcessMessage"));
		pLine->ProcessMessage(&mh);
		DBGOUT((3, "TSPI_lineDevSpecific: calling pLine->ProcessMessage done"));
	}
	DBGOUT((3, "TSPI_lineDevSpecific: calling gpfnCompletionProc"));
	gpfnCompletionProc(dwRequestID, 0);
	DBGOUT((3, "TSPI_lineDevSpecific: calling gpfnCompletionProc done"));
	return dwRequestID;
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
	*lpdwNumLines = 1;
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

LONG
TSPIAPI
TUISPI_providerConfig(
	TUISPIDLLCALLBACK   lpfnUIDLLCallback,
	HWND                hwndOwner,
	DWORD               dwPermanentProviderID
)
{
	DBGOUT((3, "TUISPI_providerConfig"));
	LONG        lResult = 0;
	gdwPermanentProviderID = dwPermanentProviderID;
	return lResult;
}


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


void
PASCAL
DropActiveCall(
	PDRVLINE    pLine
)
{
	// hangup call if any
	if (pLine->htCall == NULL)
		return;
	DBGOUT((3, "DropActiveCall: calling rpcw.Hangup. localChannel = %s", pLine->localChannel.c_str()));
	RPC_STATUS status = pLine->rpcw.Hangup((char*)pLine->localChannel.c_str());
	DBGOUT((3, "DropActiveCall: calling rpcw.Hangup done. status = %08x", status));
}
