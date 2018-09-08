// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "nuacom.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		hInst = hModule;
#if DBG
		//OutputDebugString("NUACOM: dllmain\n");
		//__debugbreak();
			{
				HKEY    hKey;
				DWORD   dwDataSize, dwDataType;
				char    szAtsp32DebugLevel[] = "DebugLevel";


				auto n = RegOpenKeyEx(
					HKEY_LOCAL_MACHINE,
					gszRegKey,
					0,
					KEY_READ,
					&hKey
				);

				dwDataSize = sizeof(DWORD);
				gdwDebugLevel = 0;

				n = RegQueryValueEx(
					hKey,
					szAtsp32DebugLevel,
					0,
					&dwDataType,
					(LPBYTE)&gdwDebugLevel,
					&dwDataSize
				);

				RegCloseKey(hKey);
			}
#endif
		return TRUE;
	case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

