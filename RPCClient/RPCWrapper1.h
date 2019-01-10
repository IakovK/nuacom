#pragma once

#if !defined(_M_IA64) && !defined(_M_AMD64)
#include "..\common\Win32\NuacomCallback.h"
#endif
#if defined(_M_AMD64)
#include "..\common\x64\NuacomCallback.h"
#endif

class CRPCWrapperCallback
{
	unsigned char * m_pszStringBinding;
	RPC_BINDING_HANDLE m_binding;
public:
	RPC_STATUS Init(unsigned char * pszProtocolSequence, unsigned char * pszEndpoint);
	CRPCWrapperCallback();
	~CRPCWrapperCallback();
	RPC_STATUS SendString(char *msg, PCALLBACK_HANDLE_TYPE hCallback);
};
