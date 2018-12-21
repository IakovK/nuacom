#include "stdafx.h"
#include <tapi.h>
#pragma comment(lib, "tapi32")


UINT __stdcall InstallProvider(
	MSIHANDLE hInstall
)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;

	hr = WcaInitialize(hInstall, "InstallProvider");
	ExitOnFailure(hr, "Failed to initialize");

	WcaLog(LOGMSG_STANDARD, "Initialized.");

	WcaLog(LOGMSG_STANDARD, "entry");

	WCHAR *CustomActionData = NULL;
	hr = WcaGetProperty(L"CustomActionData", &CustomActionData);

	if (CustomActionData == NULL)
	{
		hr = E_FAIL;
		ExitOnFailure(hr, "CustomActionData is NULL");
	}

	WcaLog(LOGMSG_STANDARD, "CustomActionData = %S", CustomActionData);

	DWORD providerId = 0;
	WcaLog(LOGMSG_STANDARD, "calling lineAddProvider");
	LONG retVal = lineAddProviderW(CustomActionData, NULL, &providerId);
	WcaLog(LOGMSG_STANDARD, "lineAddProvider returned: retVal = %08x, providerId = %d", retVal, providerId);
#if 0
	if (retVal != 0)
	{
		hr = E_FAIL;
		ExitOnFailure(hr, "lineAddProvider failed: %08x", retVal);
	}
#endif

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

UINT __stdcall UninstallProvider(
	MSIHANDLE hInstall
)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;

	hr = WcaInitialize(hInstall, "UninstallProvider");
	ExitOnFailure(hr, "Failed to initialize");

	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// TODO: Add your custom action code here.


LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}


// DllMain - Initialize and cleanup WiX custom action utils.
extern "C" BOOL WINAPI DllMain(
	__in HINSTANCE hInst,
	__in ULONG ulReason,
	__in LPVOID
	)
{
	switch(ulReason)
	{
	case DLL_PROCESS_ATTACH:
		WcaGlobalInitialize(hInst);
		break;

	case DLL_PROCESS_DETACH:
		WcaGlobalFinalize();
		break;
	}

	return TRUE;
}
