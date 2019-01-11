#pragma once

#if !defined(_M_IA64) && !defined(_M_AMD64)
#include "..\common\Win32\Nuacom.h"
#endif
#if defined(_M_AMD64)
#include "..\common\x64\Nuacom.h"
#endif

class CRPCWrapper
{
	unsigned char * m_pszStringBinding;
	RPC_BINDING_HANDLE m_binding;
	PCONTEXT_HANDLE_TYPE m_hConn;
public:
	RPC_STATUS Init(unsigned char * pszProtocolSequence, unsigned char * pszEndpoint);
	CRPCWrapper();
	~CRPCWrapper();
	RPC_STATUS GetConfigInfo(char *name, ULONG nameSize, char *password, ULONG passwordSize, char *extension, ULONG extensionSize, BOOL *bHandleIncoming, BOOL *bIncomingLocal, BOOL *bConfigured);
	RPC_STATUS SetConfigInfo(char *name, char *password, char *extension, ULONG extensionSize, BOOL bHandleIncoming, BOOL bIncomingLocal, BOOL *bConfigured);
	RPC_STATUS ConnectToServer(unsigned char *pszCallbackEndpoint, PVOID callback, ULONG providerId);
	RPC_STATUS MakeCall(char *address, char *statusMessage, ULONG statusMessageSize, BOOL *bSuccess);
	RPC_STATUS Hangup(char *localChannel);
	RPC_STATUS Disconnect();
};
