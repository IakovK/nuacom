#include <windows.h>
#include <rpc.h>
#include "RPCWrapper.h"
#include <AtlBase.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CRPCWrapper::CRPCWrapper()
{
#if 0
	m_pszStringBinding = NULL;
	m_binding = NULL;
	m_hEnum = NULL;
#endif
}

CRPCWrapper::~CRPCWrapper()
{
#if 0
	RpcStringFreeA(&m_pszStringBinding);
	RpcBindingFree(&m_binding);
#endif
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

RPC_STATUS CRPCWrapper::GetConfigInfo(char *name, ULONG nameSize, char *password, ULONG passwordSize, char *extension, ULONG extensionSize, BOOL *bHandleIncoming, BOOL *bConfigured)
{
	RpcTryExcept
	{
	::GetConfigInfo(m_binding, (unsigned char*)name, nameSize, (unsigned char*)password, passwordSize, (unsigned char*)extension, extensionSize, bHandleIncoming, bConfigured);
	}RpcExcept(EXCEPTION_EXECUTE_HANDLER)
	{
		return RpcExceptionCode();
	}
	RpcEndExcept
		return RPC_S_OK;
}

#if 0
RPC_STATUS CRPCWrapper::EnumCustomers()
{
	RpcTryExcept
	{
	::EnumCustomers (m_binding, &m_hEnum);
	}RpcExcept(EXCEPTION_EXECUTE_HANDLER)
	{
		return RpcExceptionCode();
	}
	RpcEndExcept
	return RPC_S_OK;
}

RPC_STATUS CRPCWrapper::GetNextCustomer (WCHAR *name, ULONG bufSize, bool *bEnabled)
{
	RpcTryExcept
	{
		BOOL b;
		::GetNextCustomer (m_binding, m_hEnum, &b, bufSize, name);
		*bEnabled = b;
	}RpcExcept(EXCEPTION_EXECUTE_HANDLER)
	{
		return RpcExceptionCode();
	}
	RpcEndExcept
	return RPC_S_OK;
}

RPC_STATUS CRPCWrapper::CloseEnum()
{
	RpcTryExcept
	{
	::CloseEnum (m_binding, m_hEnum);
	}RpcExcept(EXCEPTION_EXECUTE_HANDLER)
	{
		return RpcExceptionCode();
	}
	RpcEndExcept
	return RPC_S_OK;
}

RPC_STATUS CRPCWrapper::TransferFilesBegin (const WCHAR *transferName, ULONG options, ULONG nCount)
{
	RpcTryExcept
	{
		::TransferFilesBegin (m_binding, transferName, options, nCount);
	}RpcExcept(EXCEPTION_EXECUTE_HANDLER)
	{
		return RpcExceptionCode();
	}
	RpcEndExcept
	return RPC_S_OK;
}

RPC_STATUS CRPCWrapper::TransferFile (const WCHAR *fileName)
{
	RpcTryExcept
	{
		::TransferFile (m_binding, fileName);
	}RpcExcept(EXCEPTION_EXECUTE_HANDLER)
	{
		return RpcExceptionCode();
	}
	RpcEndExcept
	return RPC_S_OK;
}
#endif

extern "C" void __RPC_FAR * __RPC_API MIDL_user_allocate(size_t cBytes) 
{ 
    return(malloc(cBytes)); 
}

extern "C" void __RPC_API MIDL_user_free(void __RPC_FAR * p) 
{ 
    free(p); 
}
