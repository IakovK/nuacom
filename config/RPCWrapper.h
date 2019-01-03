#pragma once

#if !defined(_M_IA64) && !defined(_M_AMD64)
#include "..\common\Win32\nuacom.h"
#endif
#if defined(_M_AMD64)
#include "..\common\x64\nuacom.h"
#endif

class CRPCWrapper
{
	unsigned char * m_pszStringBinding;
	RPC_BINDING_HANDLE m_binding;
public:
	RPC_STATUS Init (unsigned char * pszProtocolSequence, unsigned char * pszEndpoint);
	CRPCWrapper();
	~CRPCWrapper();
	RPC_STATUS GetConfigInfo(char *name, ULONG nameSize, char *password, ULONG passwordSize, char *extension, ULONG extensionSize, BOOL *bHandleIncoming, BOOL *bConfigured);
};
