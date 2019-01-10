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

#define EP_NAME "NuacomService"
char  szRegKey[] = "Software\\Nuacom\\TSP";
char  szIncomingFlag[] = "HandleIncomingCalls";
char  szIncomingLocalFlag[] = "IncomingCallsLocal";
char  szName[] = "username";
char  szPassword[] = "passwordEncrypted";
extern BOOL bDebug;

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
	CRPCWrapperCallback rpcw;
	virtual void OnOpenListener();
	virtual void OnFailListener();
	virtual void ProcessMessage(IMessageHolder *pmh);
};

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
	RPC_STATUS status = rpcw.SendString((char*)msgStr.c_str(), hCallback);
}

void ConnectToServer(
	/* [in] */ handle_t Binding,
	/* [string][in] */ unsigned char *pszCallbackEndpoint,
	/* [in] */ PCALLBACK_HANDLE_TYPE hCallback,
	/* [out] */ PPCONTEXT_HANDLE_TYPE hConn)
{
	printf("ConnectToServer: pszCallbackEndpoint = %s, hCallback = %p\n", pszCallbackEndpoint, hCallback);
	ConnectionContext *pcc = new ConnectionContext();
	pcc->hCallback = hCallback;
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
		printf("ConnectToServer: error in GetSessionToken: %s\n", pcc->session_token.c_str());
		delete pcc;
		RpcRaiseException(RPC_S_CALL_FAILED);
	}
	pcc->extension = extension;
	unsigned char * pszProtocolSequence = (unsigned char *)"ncalrpc";
	RPC_STATUS status = pcc->rpcw.Init(pszProtocolSequence, pszCallbackEndpoint);
	if (status != RPC_S_OK)
	{
		printf("ConnectToServer: pcc->rpcw.Init failed. status = %d", status);
		RpcRaiseException(status);
	}
	printf("ConnectToServer: calling ConnectToWebsocket: pcc = %p, session_token = %s\n", pcc, pcc->session_token.c_str());
	ConnectToWebsocket(pcc->session_token.c_str(), pcc, &pcc->socketHandle);
	printf("ConnectToServer: calling ConnectToWebsocket done: pcc->socketHandle = %p, pcc->hCallback = %p\n", pcc->socketHandle, pcc->hCallback);
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
	printf("MakeCall: hConn = %p, address = %s, statusMessageSize = %d, pcc->hCallback = %p\n", hConn, address, statusMessageSize, pcc->hCallback);
	std::string status_message;
	*bSuccess = nuacom::MakeCall(pcc->session_token, pcc->extension, (char*)address, status_message);
	strncpy((char*)statusMessage, status_message.c_str(), statusMessageSize);
	printf("MakeCall: after nuacom::MakeCall: *bSuccess = %d, status_message = %s\n", *bSuccess, status_message.c_str());
}

void Hangup(
	/* [in] */ handle_t Binding,
	/* [in] */ PCONTEXT_HANDLE_TYPE hConn,
	/* [string][in] */ unsigned char *localChannel)
{
	ConnectionContext *pcc = (ConnectionContext *)hConn;
	std::string status_message;
	bool b = nuacom::EndCall(pcc->session_token, (char*)localChannel, status_message);
	printf("Hangup: hConn = %p, localChannel = %s, status_message = %s, b = %d\n", hConn, localChannel, status_message.c_str(), b);
}

void Disconnect(
	/* [in] */ handle_t Binding,
	/* [out][in] */ PPCONTEXT_HANDLE_TYPE hConn)
{
	ConnectionContext *pcc = (ConnectionContext *)(*hConn);
	CloseWebsocketAsync(pcc->socketHandle);
	printf("Disconnect: hConn = %p, pcc->socketHandle = %p\n", hConn, pcc->socketHandle);
	delete pcc;
	*hConn = NULL;
}

void __RPC_USER PCONTEXT_HANDLE_TYPE_rundown(PCONTEXT_HANDLE_TYPE hConn)
{
	printf("PCONTEXT_HANDLE_TYPE_rundown: hConn = %p\n", hConn);
	Disconnect(NULL, &hConn);
}

