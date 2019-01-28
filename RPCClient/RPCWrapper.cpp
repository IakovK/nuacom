#include <windows.h>
#include <rpc.h>
#include "RPCWrapper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CRPCWrapper::CRPCWrapper()
{
	m_pszStringBinding = NULL;
	m_binding = NULL;
}

CRPCWrapper::~CRPCWrapper()
{
	RpcStringFreeA(&m_pszStringBinding);
	RpcBindingFree(&m_binding);
}

RPC_STATUS CRPCWrapper::Init (unsigned char * pszProtocolSequence, unsigned char * pszEndpoint)
{
	RPC_STATUS status;
	status = RpcStringBindingComposeA (
		NULL,
		pszProtocolSequence,
		NULL,
		pszEndpoint,
		NULL,
		&m_pszStringBinding);
	if (status)
	{
		return status;
	}
	status = RpcBindingFromStringBindingA (
		m_pszStringBinding,
		&m_binding);
	if (status != RPC_S_OK)
		return status;
	status = RpcMgmtIsServerListening (m_binding);
	return status;
}

RPC_STATUS CRPCWrapper::GetConfigInfo(char *name, ULONG nameSize, char *password, ULONG passwordSize, char *extension, ULONG extensionSize, BOOL *bHandleIncoming, BOOL *bIncomingLocal, BOOL *bConfigured)
{
	RpcTryExcept
	{
	::GetConfigInfo(m_binding, (unsigned char*)name, nameSize, (unsigned char*)password, passwordSize, (unsigned char*)extension, extensionSize, bHandleIncoming, bIncomingLocal, bConfigured);
	}RpcExcept(EXCEPTION_EXECUTE_HANDLER)
	{
		return RpcExceptionCode();
	}
	RpcEndExcept
		return RPC_S_OK;
}

RPC_STATUS CRPCWrapper::SetConfigInfo(char *name, char *password, char *extension, ULONG extensionSize, BOOL bHandleIncoming, BOOL bIncomingLocal, BOOL *bConfigured)
{
	RpcTryExcept
	{
	::SetConfigInfo(m_binding, (unsigned char*)name, (unsigned char*)password, (unsigned char*)extension, extensionSize, bHandleIncoming, bIncomingLocal, bConfigured);
	}RpcExcept(EXCEPTION_EXECUTE_HANDLER)
	{
		return RpcExceptionCode();
	}
	RpcEndExcept
		return RPC_S_OK;
}

RPC_STATUS CRPCWrapper::ConnectToServer(unsigned char *pszCallbackEndpoint, PVOID callback, ULONG providerId, BOOL ConnectToWS)
{
	RpcTryExcept
	{
	::ConnectToServer(m_binding, pszCallbackEndpoint, (PCALLBACK_HANDLE_TYPE)callback, providerId,
	ConnectToWS,
	&m_hConn);
	}RpcExcept(EXCEPTION_EXECUTE_HANDLER)
	{
		return RpcExceptionCode();
	}
	RpcEndExcept
		return RPC_S_OK;
}

RPC_STATUS CRPCWrapper::MakeCall(char *address, char *statusMessage, ULONG statusMessageSize, BOOL *bSuccess)
{
	RpcTryExcept
	{
	::MakeCall(m_binding, m_hConn, (unsigned char*)address, (unsigned char*)statusMessage, statusMessageSize, bSuccess);
	}RpcExcept(EXCEPTION_EXECUTE_HANDLER)
	{
		return RpcExceptionCode();
	}
	RpcEndExcept
		return RPC_S_OK;
}

RPC_STATUS CRPCWrapper::Hangup(char *localChannel)
{
	RpcTryExcept
	{
	::Hangup(m_binding, m_hConn, (unsigned char*)localChannel);
	}RpcExcept(EXCEPTION_EXECUTE_HANDLER)
	{
		return RpcExceptionCode();
	}
	RpcEndExcept
		return RPC_S_OK;
}

RPC_STATUS CRPCWrapper::Disconnect()
{
	RpcTryExcept
	{
	::Disconnect(m_binding, &m_hConn);
	}RpcExcept(EXCEPTION_EXECUTE_HANDLER)
	{
		return RpcExceptionCode();
	}
	RpcEndExcept
		return RPC_S_OK;
}

RPC_STATUS CRPCWrapper::GetConnectionStatus(BOOL *bConnected)
{
	RpcTryExcept
	{
	::GetConnectionStatus(m_binding, bConnected);
	}RpcExcept(EXCEPTION_EXECUTE_HANDLER)
	{
		return RpcExceptionCode();
	}
	RpcEndExcept
		return RPC_S_OK;
}

extern "C" void __RPC_FAR * __RPC_API MIDL_user_allocate(size_t cBytes)
{ 
    return(malloc(cBytes)); 
}

extern "C" void __RPC_API MIDL_user_free(void __RPC_FAR * p) 
{ 
    free(p); 
}
