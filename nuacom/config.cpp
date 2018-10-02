#include "stdafx.h"
#include "nuacom.h"
#include "config.h"

char  szAtsp32DebugLevel[] = "DebugLevel";
char  szIncomingFlag[] = "HandleIncomingCalls";
char  szRegKey[] = "Software\\Nuacom\\TSP";
char  szLinesRegKey[] = "Software\\Nuacom\\TSP\\Lines";

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

	n = RegQueryValueEx(
		hKey,
		szIncomingFlag,
		0,
		&dwDataType,
		(LPBYTE)&gbHandleIncomingCalls,
		&dwDataSize
	);

	RegCloseKey(hKey);
}

void GetConfig()
{
#if DBG
	GetDebugLevel();
#endif
	GetIncomingFlag();
	DBGOUT((3, "GetConfig: gbHandleIncomingCalls = %d", gbHandleIncomingCalls));
}

linesCollection GetLinesInfo()
{
	struct locVars
	{
		HKEY  hKeyLines{ 0 };
		~locVars()
		{
			RegCloseKey(hKeyLines);
		}
	} lv;
	linesCollection retVal;

	auto n = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		szLinesRegKey,
		0,
		KEY_READ,
		&lv.hKeyLines
	);
	if (n != 0)
	{
		DBGOUT((3, "GetLinesInfo: failed to open %s key: %d", szLinesRegKey, n));
		return retVal;
	}

	char name[MAX_PATH];
	DWORD nameSize = MAX_PATH;
	for (DWORD index = 0;;index++)
	{
		struct locVars
		{
			HKEY  hKeyLine{ 0 };
			~locVars()
			{
				RegCloseKey(hKeyLine);
			}
		} lv1;
		name[0] = 0;
		nameSize = MAX_PATH;
		auto n = RegEnumKeyEx(
			lv.hKeyLines,
			index,
			name,
			&nameSize,
			NULL, NULL, NULL, NULL
		);
		if (n != 0)
			break;
		LineInfo li;
		n = RegOpenKeyEx(
			lv.hKeyLines,
			name,
			0,
			KEY_READ,
			&lv1.hKeyLine
		);
		if (n != 0)
		{
			DBGOUT((3, "GetLinesInfo: failed to open %s key: %d", name, n));
			return retVal;
		}

		// RegQueryValue extension number
		DWORD dataType = REG_SZ;
		DWORD dataSize = MAX_DEV_NAME_LENGTH;
		char data[MAX_DEV_NAME_LENGTH];
		n = RegQueryValueEx(
			lv1.hKeyLine,
			"extension",
			0,
			&dataType,
			(LPBYTE)data,
			&dataSize
		);
		if (n != 0)
		{
			DBGOUT((3, "GetLinesInfo: failed to query %s key: %d", name, n));
			return retVal;
		}
		li.extension = data;

		// RegQueryValue username
		dataType = REG_SZ;
		dataSize = MAX_DEV_NAME_LENGTH;
		n = RegQueryValueEx(
			lv1.hKeyLine,
			"username",
			0,
			&dataType,
			(LPBYTE)data,
			&dataSize
		);
		if (n != 0)
		{
			DBGOUT((3, "GetLinesInfo: failed to query %s key: %d", name, n));
			return retVal;
		}
		li.userName = data;

		// RegQueryValue password
		dataType = REG_SZ;
		dataSize = MAX_DEV_NAME_LENGTH;
		n = RegQueryValueEx(
			lv1.hKeyLine,
			"password",
			0,
			&dataType,
			(LPBYTE)data,
			&dataSize
		);
		if (n != 0)
		{
			DBGOUT((3, "GetLinesInfo: failed to query %s key: %d", name, n));
			return retVal;
		}
		li.password = data;

		// convert key name into line number
		int lineNumber = atol(name);
		retVal[lineNumber] = li;
	}

	return retVal;
}

