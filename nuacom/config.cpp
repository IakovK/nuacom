#include "stdafx.h"
#include "nuacom.h"
#include "config.h"

char  szAtsp32DebugLevel[] = "DebugLevel";
char  szIncomingFlag[] = "HandleIncomingCalls";
char  szIncomingLocalFlag[] = "IncomingCallsLocal";
char  szRegKey[] = "Software\\Nuacom\\TSP";

#if DBG
void GetDebugLevel()
{
	HKEY  hKey;
	DWORD dwDataSize, dwDataType;

	auto n = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		szRegKey,
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

void GetIncomingFlag()
{
	HKEY  hKey;
	DWORD dwDataSize, dwDataType;

	auto n = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		szRegKey,
		0,
		KEY_READ,
		&hKey
	);

	dwDataSize = sizeof(DWORD);
	gbHandleIncomingCalls = FALSE;
	gbIncomingCallsLocal = FALSE;

	n = RegQueryValueEx(
		hKey,
		szIncomingFlag,
		0,
		&dwDataType,
		(LPBYTE)&gbHandleIncomingCalls,
		&dwDataSize
	);

	n = RegQueryValueEx(
		hKey,
		szIncomingLocalFlag,
		0,
		&dwDataType,
		(LPBYTE)&gbIncomingCallsLocal,
		&dwDataSize
	);

	RegCloseKey(hKey);
}

static void SaveIncomingFlag()
{
	HKEY  hKey;

	auto n = RegCreateKeyEx(
		HKEY_LOCAL_MACHINE,
		szRegKey,
		0,
		NULL,
		0,
		KEY_READ | KEY_WRITE, NULL,
		&hKey, NULL
	);

	n = RegSetValueEx(
		hKey,
		szIncomingFlag,
		0,
		REG_DWORD,
		(LPBYTE)&gbHandleIncomingCalls,
		4
	);

	RegCloseKey(hKey);
}

void GetConfig()
{
#if DBG
	GetDebugLevel();
#endif
	GetIncomingFlag();
	DBGOUT((3, "GetConfig: gbHandleIncomingCalls = %d, gbIncomingCallsLocal = %d", gbHandleIncomingCalls, gbIncomingCallsLocal));
}
