// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.


//  MODULE:   server.c
//
//  PURPOSE:  Implements the body of the RPC service sample.
//
//  FUNCTIONS:
//            Called by service.c:
//            ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv);
//            ServiceStop( );
//
//            Called by RPC:
//            error_status_t Ping(handle_t)
//
//  COMMENTS: The ServerStart and ServerStop functions implemented here are
//            prototyped in service.h.  The other functions are RPC manager
//            functions prototypes in rpcsvc.h.
//              



#include "header.h"
#include "service.h"
#include "nuacomAPI.h"
#include "websocketapi.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <curl/curl.h>

#include "../RPCClient/RPCWrapper1.h"

#if !defined(_M_IA64) && !defined(_M_AMD64)
#include "..\common\Win32\Nuacom.h"
#endif
#if defined(_M_AMD64)
#include "..\common\x64\Nuacom.h"
#endif

#include "..\common\structs.h"

#define EP_NAME "NuacomService"
char  szRegKey[] = "Software\\Nuacom\\TSP";
char  szIncomingFlag[] = "HandleIncomingCalls";
char  szIncomingLocalFlag[] = "IncomingCallsLocal";
char  szName[] = "username";
char  szPassword[] = "passwordEncrypted";
extern BOOL bDebug;
BOOL g_bConnected = FALSE;

void
CDECL
DebugPrint(
	LPCSTR  lpszFormat,
	...
)
{
	char    buf[512] = "NUACOMSRV: ";
	DWORD pid = GetCurrentProcessId();
	DWORD tid = GetCurrentThreadId();
	va_list ap;

	sprintf(buf, "NUACOMSRV(%d:%d): ", pid, tid);
	char *newBuf = buf + strlen(buf);
	int newbufSize = 512 - strlen(buf);

	va_start(ap, lpszFormat);

	_vsnprintf_s(newBuf, newbufSize, newbufSize - 1, lpszFormat, ap);


	strncat_s(buf, sizeof(buf), "\n", 511 - strlen(buf));
	buf[511] = '\0';

	if (bDebug)
		printf(buf);
	else
		OutputDebugString(buf);

	va_end(ap);
}
//
// RPC configuration.
//

//
//  FUNCTION: ServiceStart
//
//  PURPOSE: Actual code of the service
//           that does the work.
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    Starts the service listening for RPC requests.
//
VOID ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv)
{
    RPC_BINDING_VECTOR *pbindingVector = 0;
    RPC_STATUS status;
    BOOL fListening = FALSE;
	unsigned char * pszProtocolSequence = (unsigned char *)"ncalrpc";
	unsigned char * pszEndpoint = (unsigned char *)EP_NAME;

    ///////////////////////////////////////////////////
    //
    // Service initialization
    //

	status = RpcServerUseProtseqEp(pszProtocolSequence, RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
		pszEndpoint, NULL);
	if (status)
	{
		SetLastError(status);
		return;
	}

	status = RpcServerRegisterIf(Nuacom_v1_0_s_ifspec, NULL, NULL);
	if (status)
	{
		SetLastError(status);
		return;
	}

	status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
	if (status)
	{
		SetLastError(status);
		return;
	}


    if (status != RPC_S_OK)
        {
        return;
        }

    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_RUNNING,       // service state
        NO_ERROR,              // exit code
        0))                    // wait hint
        return;

    //
    // End of initialization
    //
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    //
    // Cleanup
    //

    // RpcMgmtWaitServerListen() will block until the server has
    // stopped listening.  If this service had something better to
    // do with this thread, it would delay this call until
    // ServiceStop() had been called. (Set an event in ServiceStop()).
    //
    status = RpcMgmtWaitServerListen();

    // ASSERT(status == RPC_S_OK)

    //
    ////////////////////////////////////////////////////////////
    return;
}


//
//  FUNCTION: ServiceStop
//
//  PURPOSE: Stops the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    If a ServiceStop procedure is going to
//    take longer than 3 seconds to execute,
//    it should spawn a thread to execute the
//    stop code, and return.  Otherwise, the
//    ServiceControlManager will believe that
//    the service has stopped responding.
//    
VOID ServiceStop()
{
    // Stop's the server, wakes the main thread.

    RpcMgmtStopServerListening(0);
}

static void DecryptData(std::vector<BYTE> input, BYTE *output, ULONG outputSize)
{
	DATA_BLOB  db1 = { 0 }, db2 = { 0 };
	db1.cbData = input.size();
	db1.pbData = (BYTE*)input.data();
	BOOL b = CryptUnprotectData(&db1, NULL, NULL, NULL, NULL, 0, &db2);
	if (!b)
	{
		DWORD dwErr = GetLastError();
		RpcRaiseException(dwErr);
	}
	memcpy(output, db2.pbData, min(db2.cbData, outputSize));
	LocalFree(db2.pbData);
}

static void EncryptData(const char *data, std::vector<unsigned char> &encryptedData)
{
	DATA_BLOB  db1 = { 0 }, db2 = { 0 };
	db1.cbData = strlen(data) + 1;
	db1.pbData = (BYTE*)data;
	BOOL b = CryptProtectData(&db1, L"nuacom password", NULL, NULL, NULL, 0, &db2);
	if (!b)
	{
		DWORD dwErr = GetLastError();
		RpcRaiseException(dwErr);
	}
	encryptedData.assign(db2.pbData, db2.pbData + db2.cbData);
	LocalFree(db2.pbData);
}

static int GetUserExtension(const char *name, const char *password, char *extension, ULONG extensionSize)
{
	std::string userName(name);
	std::string Password(password);
	std::string Extension;
	std::string session_token;
	bool b = nuacom::GetSessionToken(userName, password, session_token);
	if (!b)
		return E_FAIL;

	b = nuacom::GetExtension(session_token, Extension);
	if (!b)
		return E_FAIL;
	strncpy(extension, Extension.c_str(), extensionSize);
	return 0;
}

void GetConfigInfo(
	/* [in] */ handle_t IDL_handle,
	/* [size_is][string][out] */ unsigned char *name,
	ULONG nameSize,
	/* [size_is][string][out] */ unsigned char *password,
	ULONG passwordSize,
	/* [size_is][string][out] */ unsigned char *extension,
	ULONG extensionSize,
	/* [out] */ BOOL *bHandleIncoming,
	/* [out] */ BOOL *bIncomingLocal,
	/* [out] */ BOOL *bConfigured)
{
	strcpy((char*)name, "");
	strcpy((char*)password, "");
	strcpy((char*)extension, "");
	*bHandleIncoming = FALSE;
	*bIncomingLocal = FALSE;
	*bConfigured = FALSE;

	struct locVars
	{
		HKEY  hKey{ 0 };
		~locVars()
		{
			RegCloseKey(hKey);
		}
	} lv;

	HKEY rootKey = bDebug ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;

	auto n = RegOpenKeyEx(
		rootKey,
		szRegKey,
		0,
		KEY_READ,
		&lv.hKey
	);
	if (n != 0)
	{
		RpcRaiseException(n);
	}

	// RegQueryValue HandleIncomingCalls
	DWORD dataSize = 4;
	n = RegQueryValueEx(
		lv.hKey,
		szIncomingFlag,
		0,
		NULL,
		(LPBYTE)bHandleIncoming,
		&dataSize
	);

	// RegQueryValue IncomingCallsLocal
	dataSize = 4;
	n = RegQueryValueEx(
		lv.hKey,
		szIncomingLocalFlag,
		0,
		NULL,
		(LPBYTE)bIncomingLocal,
		&dataSize
	);

	// RegQueryValue username
	dataSize = nameSize;
	n = RegQueryValueEx(
		lv.hKey,
		szName,
		0,
		NULL,
		(LPBYTE)name,
		&dataSize
	);
	if (n != 0)
	{
		RpcRaiseException(n);
	}

	// RegQueryValue passwordEncrypted
	n = RegQueryValueEx(
		lv.hKey,
		szPassword,
		0,
		NULL,
		NULL,
		&dataSize
	);
	if (n != 0)
	{
		RpcRaiseException(n);
	}

	std::vector<BYTE> d1(dataSize);
	n = RegQueryValueEx(
		lv.hKey,
		szPassword,
		0,
		NULL,
		d1.data(),
		&dataSize
	);
	if (n != 0)
	{
		RpcRaiseException(n);
	}
	DecryptData(d1, password, passwordSize);

	n = GetUserExtension((const char*)name, (const char*)password, (char*)extension, extensionSize);
	if (n != 0)
	{
		RpcRaiseException(n);
	}
	*bConfigured = TRUE;
}

void SetConfigInfo( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ unsigned char *name,
    /* [string][in] */ unsigned char *password,
    /* [size_is][string][out] */ unsigned char *extension,
    ULONG extensionSize,
    /* [in] */ BOOL bHandleIncoming,
	/* [in] */ BOOL bIncomingLocal,
	/* [out] */ BOOL *bConfigured)
{
	struct locVars
	{
		HKEY  hKey{ 0 };
		~locVars()
		{
			RegCloseKey(hKey);
		}
	} lv;

	*bConfigured = FALSE;

	HKEY rootKey = bDebug ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;

	auto n = RegCreateKeyEx(
		rootKey,
		szRegKey,
		0,
		NULL,
		0,
		KEY_READ | KEY_WRITE, NULL,
		&lv.hKey, NULL
	);

	// RegSetValue IncomingCallsLocal
	n = RegSetValueEx(
		lv.hKey,
		szIncomingLocalFlag,
		0,
		REG_DWORD,
		(LPBYTE)&bIncomingLocal,
		4
	);

	// RegSetValue HandleIncomingCalls
	n = RegSetValueEx(
		lv.hKey,
		szIncomingFlag,
		0,
		REG_DWORD,
		(LPBYTE)&bHandleIncoming,
		4
	);

	// RegSetValue username
	n = RegSetValueEx(
		lv.hKey,
		szName,
		0,
		REG_SZ,
		(LPBYTE)name,
		strlen((const char*)name) + 1
	);

	std::vector<unsigned char> passwordEncrypted;
	EncryptData((const char*)password, passwordEncrypted);

	// RegSetValue passwordEncrypted
	n = RegSetValueEx(
		lv.hKey,
		szPassword,
		0,
		REG_BINARY,
		(LPBYTE)passwordEncrypted.data(),
		passwordEncrypted.size()
	);

	n = GetUserExtension((const char*)name, (const char*)password, (char*)extension, extensionSize);
	if (n != 0)
	{
		RpcRaiseException(n);
	}
	*bConfigured = TRUE;
}

struct ConnectionContext : ICallbacks
{
	std::string session_token;
	std::string extension;
	void *socketHandle;
	PCALLBACK_HANDLE_TYPE hCallback;
	ULONG providerId;

	bool m_bInited;
	HLINEAPP m_appHandle;
	DWORD m_apiVersion;
	LINEINITIALIZEEXPARAMS m_params;

	ConnectionContext();
	~ConnectionContext();
	void SendString(const std::string &msg);
	void WaitForReply(LONG requestId, int timeoutSecs);
	virtual void OnOpenListener();
	virtual void OnFailListener();
	virtual void ProcessMessage(IMessageHolder *pmh);
};

ConnectionContext::ConnectionContext()
	:m_bInited{ false }
	, socketHandle{ NULL }
	, hCallback{ NULL }
	, providerId{ 0 }
	, m_appHandle{ NULL }
	, m_apiVersion{ TAPI_CURRENT_VERSION }
{
	DWORD numDevs;
	m_params.dwTotalSize = sizeof(LINEINITIALIZEEXPARAMS);
	m_params.dwOptions = LINEINITIALIZEEXOPTION_USEEVENT;
	auto n = lineInitializeEx(&m_appHandle, NULL, NULL, "nuacom test app", &numDevs, &m_apiVersion, &m_params);
	if (n == 0)
	{
		m_bInited = true;
	}
}

ConnectionContext::~ConnectionContext()
{
	if (m_bInited)
	{
		auto n = lineShutdown(m_appHandle);
	}
}

void ConnectionContext::OnOpenListener()
{
}

void ConnectionContext::OnFailListener()
{
}

#include "sioclient\src\secure-socketio\internal\ssio_packet.h"
namespace ssio
{
	void accept_message(message const& msg, rapidjson::Value& val, rapidjson::Document& doc, std::vector<std::shared_ptr<const std::string>>& buffers);
}

void ConnectionContext::WaitForReply(LONG requestId, int timeoutSecs)
{
	DebugPrint("ConnectionContext::WaitForReply: entry. requestId = %d\n", requestId);
	LINEMESSAGE msg;
	while (true)
	{
		int n = lineGetMessage(m_appHandle, &msg, timeoutSecs * 1000);
		DebugPrint("ConnectionContext::WaitForReply: lineGetMessage returned: %08x\n", n);
		if (n < 0)
			break;
		switch (msg.dwMessageID)
		{
		case LINE_REPLY:
			DebugPrint("LINE_REPLY: request id = %d, result code = %08x\n", msg.dwParam1, msg.dwParam2);
			break;
		default:
			break;
		}
		if (msg.dwParam1 == requestId)
			break;
	}
	DebugPrint("RunMessageLoop: exit\n");
}

void ConnectionContext::SendString(const std::string &msg)
{
	DebugPrint("ConnectionContext::SendString: msg.size = %d, providerId = %d, hCallback = %p\n", msg.size(), providerId, hCallback);
	HLINE lineHandle;
	auto n = lineOpen(m_appHandle, providerId, &lineHandle, m_apiVersion, 0, (DWORD_PTR)this,
		LINECALLPRIVILEGE_OWNER, LINEMEDIAMODE_UNKNOWN, NULL);
	DebugPrint("ConnectionContext::SendString: lineOpen returned: %08x\n", n);
	if (n != 0)
		return;
	DWORD dwTotalSize = sizeof(RPCHeader) + sizeof(stringData) + msg.size();
	RPCHeader *h = (RPCHeader *)malloc(dwTotalSize);
	if (h == NULL)
		return;
	memset(h, 0, dwTotalSize);
	h->dwTotalSize = dwTotalSize;
	h->dwCommand = SEND_STRING;
	h->dwDataSize = sizeof(stringData) + msg.size();
	stringData *sd = (stringData *)h->data;
	sd->callback = hCallback;
	sd->dwDataSize = msg.size();
	memcpy(sd->data, msg.data(), sd->dwDataSize);
	n = lineDevSpecific(lineHandle, 0, NULL, h, h->dwTotalSize);
	DebugPrint("ConnectionContext::SendString: lineDevSpecific returned: %08x\n", n);
	if (n > 0)
		WaitForReply(n, 30);
	free(h);
	DebugPrint("ConnectionContext::SendString: calling lineClose(lineHandle)\n");
	lineClose(lineHandle);
	DebugPrint("ConnectionContext::SendString: exit\n");
}

void ConnectionContext::ProcessMessage(IMessageHolder *pmh)
{
	ssio::message::ptr _m = pmh->get_message();
	std::vector<std::shared_ptr<const std::string>>buffers;
	rapidjson::Document doc;
	ssio::accept_message(*_m, doc, doc, buffers);
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	std::string msgStr = buffer.GetString();
	SendString(msgStr);
}

void ConnectToServer(
	/* [in] */ handle_t Binding,
	/* [string][in] */ unsigned char *pszCallbackEndpoint,
	/* [in] */ PCALLBACK_HANDLE_TYPE hCallback,
	/* [in] */ ULONG providerId,
	/* [out] */ PPCONTEXT_HANDLE_TYPE hConn)
{
	DebugPrint("ConnectToServer: pszCallbackEndpoint = %s, hCallback = %p, providerId = %d\n", pszCallbackEndpoint, hCallback, providerId);
	ConnectionContext *pcc = new ConnectionContext();
	pcc->hCallback = hCallback;
	pcc->providerId = providerId;

	char name[255];
	char password[255];
	char extension[255];
	BOOL b1, b2, c;
	GetConfigInfo(NULL, (unsigned char*)name, 255, (unsigned char*)password, 255, (unsigned char*)extension, 255, &b1, &b2, &c);
	if (!c)
	{
		RpcRaiseException(RPC_S_CALL_FAILED);
	}

	bool b = nuacom::GetSessionToken(name, password, pcc->session_token);
	if (!b)
	{
		DebugPrint("ConnectToServer: error in GetSessionToken: %s\n", pcc->session_token.c_str());
		delete pcc;
		RpcRaiseException(RPC_S_CALL_FAILED);
	}
	pcc->extension = extension;

	DebugPrint("ConnectToServer: calling ConnectToWebsocket: pcc = %p, session_token = %s\n", pcc, pcc->session_token.c_str());
	ConnectToWebsocket(pcc->session_token.c_str(), pcc, &pcc->socketHandle);
	DebugPrint("ConnectToServer: calling ConnectToWebsocket done: pcc->socketHandle = %p, pcc->hCallback = %p\n", pcc->socketHandle, pcc->hCallback);
	g_bConnected = TRUE;
	*hConn = pcc;
}

void MakeCall(
	/* [in] */ handle_t Binding,
	/* [in] */ PCONTEXT_HANDLE_TYPE hConn,
	/* [string][in] */ unsigned char *address,
	/* [size_is][string][out] */ unsigned char *statusMessage,
	/* [in] */ ULONG statusMessageSize,
	/* [out] */ BOOL *bSuccess)
{
	ConnectionContext *pcc = (ConnectionContext *)hConn;
	DebugPrint("MakeCall: hConn = %p, address = %s, statusMessageSize = %d, pcc->hCallback = %p\n", hConn, address, statusMessageSize, pcc->hCallback);
	std::string status_message;
	*bSuccess = nuacom::MakeCall(pcc->session_token, pcc->extension, (char*)address, status_message);
	strncpy((char*)statusMessage, status_message.c_str(), statusMessageSize);
	DebugPrint("MakeCall: after nuacom::MakeCall: *bSuccess = %d, status_message = %s\n", *bSuccess, status_message.c_str());
}

void Hangup(
	/* [in] */ handle_t Binding,
	/* [in] */ PCONTEXT_HANDLE_TYPE hConn,
	/* [string][in] */ unsigned char *localChannel)
{
	ConnectionContext *pcc = (ConnectionContext *)hConn;
	std::string status_message;
	bool b = nuacom::EndCall(pcc->session_token, (char*)localChannel, status_message);
	DebugPrint("Hangup: hConn = %p, localChannel = %s, status_message = %s, b = %d\n", hConn, localChannel, status_message.c_str(), b);
}

void Disconnect(
	/* [in] */ handle_t Binding,
	/* [out][in] */ PPCONTEXT_HANDLE_TYPE hConn)
{
	ConnectionContext *pcc = (ConnectionContext *)(*hConn);
	CloseWebsocketAsync(pcc->socketHandle);
	DebugPrint("Disconnect: hConn = %p, pcc->socketHandle = %p\n", hConn, pcc->socketHandle);
	delete pcc;
	*hConn = NULL;
	g_bConnected = FALSE;
}

void GetConnectionStatus(
	/* [in] */ handle_t IDL_handle,
	/* [out] */ BOOL *bConnected)
{
	*bConnected = g_bConnected;
}

void __RPC_USER PCONTEXT_HANDLE_TYPE_rundown(PCONTEXT_HANDLE_TYPE hConn)
{
	DebugPrint("PCONTEXT_HANDLE_TYPE_rundown: hConn = %p\n", hConn);
	Disconnect(NULL, &hConn);
}

