#include <windows.h>
#include <rpc.h>
#include "RPCWrapper1.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CRPCWrapperCallback::CRPCWrapperCallback()
{
	m_pszStringBinding = NULL;
	m_binding = NULL;
}

CRPCWrapperCallback::~CRPCWrapperCallback()
{
	RpcStringFreeA(&m_pszStringBinding);
	RpcBindingFree(&m_binding);
}

RPC_STATUS CRPCWrapperCallback::Init(unsigned char * pszProtocolSequence, unsigned char * pszEndpoint)
{
	RPC_STATUS status;
	status = RpcStringBindingComposeA(
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
	status = RpcBindingFromStringBindingA(
		m_pszStringBinding,
		&m_binding);
	if (status != RPC_S_OK)
		return status;
	status = RpcMgmtIsServerListening(m_binding);
	return status;
}

RPC_STATUS CRPCWrapperCallback::SendString(char *msg, PCALLBACK_HANDLE_TYPE hCallback)
{
	RpcTryExcept
	{
	::SendString(m_binding, hCallback, (unsigned char*)msg);
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
