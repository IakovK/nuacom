#include <Windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <tapi.h>
#include <intrin.h>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>
#include "resource.h"

#pragma comment(lib, "tapi32")

void TestInstall(int ac, char *av[])
{
	printf("TestInstall\n");
	if (ac < 3)
	{
		printf("usage: testapp.exe install <file name>\n");
		return;
	}
	std::string fileName = av[2];
	printf("installing %s\n", fileName.c_str());
	DWORD providerId = 0;
	printf("calling lineAddProvider\n");
	LONG retVal = lineAddProvider("c:\\windows\\system32\\nuacom.tsp", NULL, &providerId);
	printf("lineAddProvider returned: retVal = %08x, providerId = %d\n", retVal, providerId);
}

void TestUninstall(int ac, char *av[])
{
	printf("TestUninstall\n");
	if (ac < 3)
	{
		printf("usage: testapp.exe uninstall <provider id>\n");
		return;
	}
	std::string idStr = av[2];
	std::stringstream s(idStr);
	int providerId;
	s >> providerId;
	printf("uninstalling %d\n", providerId);
	printf("calling lineRemoveProvider\n");
	LONG retVal = lineRemoveProvider(providerId, NULL);
	printf("lineRemoveProvider returned: retVal = %08x\n", retVal);
}

class DialTester
{
	bool m_bInited;
	HLINEAPP m_appHandle;
	DWORD m_numDevs;
	LINEINITIALIZEEXPARAMS m_params;
public:
	DialTester()
		:m_bInited(false)
		, m_params{ 0 }
		, m_numDevs{ 0 }
	{
		DWORD apiVersion = TAPI_CURRENT_VERSION;
		m_params.dwTotalSize = sizeof(LINEINITIALIZEEXPARAMS);
		m_params.dwOptions = LINEINITIALIZEEXOPTION_USEEVENT;
		auto n = lineInitializeEx(&m_appHandle, NULL, NULL, "nuacom test app", &m_numDevs, &apiVersion, &m_params);
		if (n == 0)
		{
			m_bInited = true;
		}
	}
	~DialTester()
	{
		if (m_bInited)
		{
			auto n = lineShutdown(m_appHandle);
		}
	}
	struct provDesc
	{
		DWORD providerId;
		std::string providerName;
	};
	struct lineDesc
	{
		DWORD lineId;
		DWORD dwAPIVersion;
		std::string lineName;
	};
	using ProvidersList = std::vector<provDesc>;
	using LinesList = std::vector<lineDesc>;
	ProvidersList GetProvidersList()
	{
		ProvidersList retVal;
		LINEPROVIDERLIST lpl;
		lpl.dwTotalSize = sizeof lpl;
		auto n = lineGetProviderList(TAPI_CURRENT_VERSION, &lpl);
		if (n != 0)
		{
			printf("GetProvidersList: lineGetProviderList(1) failed: %08x\n", n);
			return retVal;
		}
		std::vector<unsigned char> v(lpl.dwNeededSize);
		LPLINEPROVIDERLIST lpl2 = (LPLINEPROVIDERLIST)v.data();
		lpl2->dwTotalSize = lpl.dwNeededSize;
		n = lineGetProviderList(TAPI_CURRENT_VERSION, lpl2);
		if (n != 0)
		{
			printf("GetProvidersList: lineGetProviderList(2) failed: %08x\n", n);
			return retVal;
		}
		LPLINEPROVIDERENTRY lpeptr = (LPLINEPROVIDERENTRY)(v.data() + lpl2->dwProviderListOffset);
		for (int j = 0; j < lpl2->dwNumProviders; j++)
		{
			provDesc pd;
			pd.providerId = lpeptr[j].dwPermanentProviderID;
			pd.providerName = std::string((char*)(v.data() + lpeptr[j].dwProviderFilenameOffset), lpeptr[j].dwProviderFilenameSize);
			retVal.push_back(pd);
		}
		return retVal;
	}
	bool GetLineInfo(int lineNum, lineDesc &ld)
	{
		LINEEXTENSIONID extId;
		auto n = lineNegotiateAPIVersion(m_appHandle, lineNum,
			TAPI_CURRENT_VERSION, TAPI_CURRENT_VERSION,
			&(ld.dwAPIVersion), &extId);
		if (n != 0)
		{
			printf("GetLineInfo: lineNum = %d, lineNegotiateAPIVersion failed: %08x\n", lineNum, n);
			return false;
		}
		LINEDEVCAPS ldc{ 0 };
		ldc.dwTotalSize = sizeof(LINEDEVCAPS);
		n = lineGetDevCaps(m_appHandle, lineNum, ld.dwAPIVersion, 0, &ldc);
		if (n != 0)
		{
			printf("GetLineInfo: lineNum = %d, lineGetDevCaps(1) failed: %08x\n", lineNum, n);
			return false;
		}
		std::vector<unsigned char> v(ldc.dwNeededSize);
		LPLINEDEVCAPS ldc2 = (LPLINEDEVCAPS)v.data();
		ldc2->dwTotalSize = ldc.dwNeededSize;
		n = lineGetDevCaps(m_appHandle, lineNum, ld.dwAPIVersion, 0, ldc2);
		if (n != 0)
		{
			printf("GetLineInfo: lineNum = %d, lineGetDevCaps(2) failed: %08x\n", lineNum, n);
			return false;
		}
		std::string lineName((char*)v.data() + ldc2->dwLineNameOffset, ldc2->dwLineNameSize);
		ld.lineId = lineNum;
		ld.lineName = lineName;
		return true;
	}
	LinesList GetLinesList()
	{
		LinesList retVal;
		for (int j = 0; j < m_numDevs; j++)
		{
			lineDesc ld;
			if (GetLineInfo(j, ld))
				retVal.push_back(ld);
		}
		return retVal;
	}
	void dial(int deviceId, const char *number)
	{
		printf("dial: deviceId = %d, number = %s\n", deviceId, number);
		LinesList l = GetLinesList();
		auto it = std::find_if(std::begin(l), std::end(l), [=](const lineDesc& ld)
		{
			return ld.lineId == deviceId;
		});
		if (it == std::end(l))
		{
			printf("dial: linedesc not found\n");
			return;
		}
		HLINE lineHandle;
		auto n = lineOpen(m_appHandle, deviceId, &lineHandle, it->dwAPIVersion, 0, (DWORD_PTR)this,
			LINECALLPRIVILEGE_OWNER, LINEMEDIAMODE_UNKNOWN, NULL);
		printf("dial: lineOpen returned: %08x\n", n);
		if (n != 0)
			return;

		HCALL callHandle;
		n = lineMakeCall(lineHandle, &callHandle, number, 0, NULL);
		printf("dial: lineMakeCall returned: %08x\n", n);
		if (n > 0)
			RunMessageLoop(300);
		n = lineClose(lineHandle);
		printf("dial: lineClose returned: %08x\n", n);
	}
	void answer(int deviceId)
	{
		printf("answer: deviceId = %d\n", deviceId);
		LinesList l = GetLinesList();
		auto it = std::find_if(std::begin(l), std::end(l), [=](const lineDesc& ld)
		{
			return ld.lineId == deviceId;
		});
		if (it == std::end(l))
		{
			printf("answer: linedesc not found\n");
			return;
		}
		HLINE lineHandle;
		auto n = lineOpen(m_appHandle, deviceId, &lineHandle, it->dwAPIVersion, 0, (DWORD_PTR)this,
			LINECALLPRIVILEGE_OWNER, LINEMEDIAMODE_UNKNOWN, NULL);
		printf("answer: lineOpen returned: %08x\n", n);
		if (n != 0)
			return;

		RunMessageLoop(300);
		n = lineClose(lineHandle);
		printf("answer: lineClose returned: %08x\n", n);
	}
	LONG GetCallInfo(HCALL hCall, std::vector<std::uint8_t> &v)
	{
		printf("GetCallInfo: entry\n");
		LINECALLINFO lci{ 0 };
		lci.dwTotalSize = lci.dwUsedSize = lci.dwNeededSize = sizeof(LINECALLINFO);
		long l = lineGetCallInfo(hCall, &lci);
		printf("GetCallInfo: lineGetCallInfo returned: lci.dwNeededSize = %d, l = %08x\n", lci.dwNeededSize, l);
		if (lci.dwNeededSize == 0)
			return LINEERR_OPERATIONFAILED;
		v.assign(lci.dwNeededSize, 0);
		printf("GetCallInfo: after v.assign: v.size() = %d\n", v.size());
		LPLINECALLINFO plci = (LPLINECALLINFO)v.data();
		plci->dwTotalSize = plci->dwUsedSize = plci->dwNeededSize = v.size();
		printf("GetCallInfo: plci = %p\n", plci);
		l = lineGetCallInfo(hCall, plci);
		return l;
	}

	void PrintString(const char *name, int size, int offset, LINECALLINFO *lci)
	{
		if (offset == 0)
			return;
		char *p = (char*)lci + offset;
		std::string s(p, size);
		printf("%s = \"%s\"\n", name, s.c_str());
	}

	void DumpCallInfo(LINECALLINFO *lci)
	{
#define PRINT_INT(x) printf(#x " = %d\n", lci->x)
#define PRINT_HEX(x) printf(#x " = %08x\n", lci->x)
		printf("DumpCallInfo\n");
		PRINT_INT(dwLineDeviceID);
		PRINT_INT(dwAddressID);
		PRINT_INT(dwBearerMode);
		PRINT_INT(dwRate);
		PRINT_HEX(dwMediaMode);
		PRINT_HEX(dwAppSpecific);
		PRINT_HEX(dwCallID);
		PRINT_HEX(dwRelatedCallID);
		PRINT_HEX(dwCallParamFlags);
		PRINT_HEX(dwCallStates);
		PRINT_HEX(dwMonitorDigitModes);
		PRINT_HEX(dwMonitorMediaModes);
		PRINT_INT(dwOrigin);
		PRINT_INT(dwReason);
		PRINT_INT(dwCompletionID);
		PRINT_INT(dwNumOwners);
		PRINT_INT(dwNumMonitors);
		PRINT_INT(dwCountryCode);
		PRINT_INT(dwTrunk);
		PRINT_HEX(dwCallerIDFlags);
		PrintString("dwCallerID", lci->dwCallerIDSize, lci->dwCallerIDOffset, lci);
		PrintString("dwCallerIDName", lci->dwCallerIDNameSize, lci->dwCallerIDNameOffset, lci);
		PRINT_HEX(dwCalledIDFlags);
		PrintString("dwCalledID", lci->dwCalledIDSize, lci->dwCalledIDOffset, lci);
		PrintString("dwCalledIDName", lci->dwCalledIDNameSize, lci->dwCalledIDNameOffset, lci);
		PrintString("dwAppName", lci->dwAppNameSize, lci->dwAppNameOffset, lci);
		PrintString("dwDisplayableAddress", lci->dwDisplayableAddressSize, lci->dwDisplayableAddressOffset, lci);
		PrintString("dwCalledParty", lci->dwCalledPartySize, lci->dwCalledPartyOffset, lci);
		PrintString("dwCalledParty", lci->dwCalledPartySize, lci->dwCalledPartyOffset, lci);
	}
	void RunMessageLoop(int timeoutSecs)
	{
		printf("RunMessageLoop: entry\n");
		LINEMESSAGE msg;
		while (true)
		{
			int n = lineGetMessage(m_appHandle, &msg, timeoutSecs * 1000);
			printf("RunMessageLoop: lineGetMessage returned: %08x\n", n);
			if (n < 0)
				return;
			printf("RunMessageLoop: msg.dwMessageID = %s\n", MessageIdToText(msg.dwMessageID));
			switch (msg.dwMessageID)
			{
			case LINE_REPLY:
				printf("LINE_REPLY: request id = %08x, result code = %08x\n", msg.dwParam1, msg.dwParam2);
				break;
			case LINE_CALLSTATE:
				printf("LINE_CALLSTATE: LineCallState = %08x, StateData = %08x, MediaMode = %08x\n", msg.dwParam1, msg.dwParam2, msg.dwParam3);
				if (msg.dwParam1 == LINECALLSTATE_RINGBACK)
				{
					std::vector<std::uint8_t> v;
					auto l = GetCallInfo(msg.hDevice, v);
					if (l < 0)
					{
						printf("RunMessageLoop: GetCallInfo returned %08x\n", l);
					}
					else
					{
						LPLINECALLINFO lci = (LPLINECALLINFO)v.data();
						printf("RunMessageLoop: lpLineInfo = %p, lpLineInfo->dwCallerIDOffset = %d, lpLineInfo->dwCallerIDSize = %d\n",
							lci, lci->dwCallerIDOffset, lci->dwCallerIDSize);
						DumpCallInfo(lci);
					}
				}
				break;
			case LINE_APPNEWCALL:
				printf("LINE_APPNEWCALL: addressId = %08x, callHandle = %08x, privilege = %08x\n", msg.dwParam1, msg.dwParam2, msg.dwParam3);
				break;
			default:
				printf("RunMessageLoop: msg.dwMessageID = %s\n", MessageIdToText(msg.dwMessageID));
				break;
			}
		}
		printf("RunMessageLoop: exit\n");
	}
	const char *MessageIdToText(int dwMessageID)
	{
		static char s[100];
#define CASE_TEXT(n) case n: return #n;
		switch (dwMessageID)
		{
		default:
			sprintf(s, "Unknown: %d", dwMessageID);
			return s;

CASE_TEXT(LINE_CALLINFO)
CASE_TEXT(LINE_CALLSTATE)
CASE_TEXT(LINE_CLOSE)
CASE_TEXT(LINE_DEVSPECIFIC)
CASE_TEXT(LINE_DEVSPECIFICFEATURE)
CASE_TEXT(LINE_GATHERDIGITS)
CASE_TEXT(LINE_GENERATE)
CASE_TEXT(LINE_LINEDEVSTATE)
CASE_TEXT(LINE_MONITORDIGITS)
CASE_TEXT(LINE_MONITORMEDIA)
CASE_TEXT(LINE_MONITORTONE)
CASE_TEXT(LINE_REPLY)
CASE_TEXT(LINE_REQUEST)
CASE_TEXT(PHONE_BUTTON)
CASE_TEXT(PHONE_CLOSE)
CASE_TEXT(PHONE_DEVSPECIFIC)
CASE_TEXT(PHONE_REPLY)
CASE_TEXT(PHONE_STATE)
CASE_TEXT(LINE_CREATE)
CASE_TEXT(PHONE_CREATE)
CASE_TEXT(LINE_AGENTSPECIFIC)
CASE_TEXT(LINE_AGENTSTATUS)
CASE_TEXT(LINE_APPNEWCALL)
CASE_TEXT(LINE_PROXYREQUEST)
CASE_TEXT(LINE_REMOVE)
CASE_TEXT(PHONE_REMOVE)
CASE_TEXT(LINE_AGENTSESSIONSTATUS)
CASE_TEXT(LINE_QUEUESTATUS)
CASE_TEXT(LINE_AGENTSTATUSEX)
CASE_TEXT(LINE_GROUPSTATUS)
CASE_TEXT(LINE_PROXYSTATUS)
		}
#undef CASE_TEXT
	}
};

void TestDial(int ac, char *av[])
{
	printf("TestDial\n");
	if (ac < 4)
	{
		printf("usage: testapp.exe dial <device id> <number>\n");
		return;
	}
	std::string idStr = av[2];
	std::string numToDial = av[3];
	std::stringstream s(idStr);
	int providerId;
	s >> providerId;
	DialTester dt;
	dt.dial(providerId, numToDial.c_str());
	printf("TestDial exit\n");
}

void TestAnswer(int ac, char *av[])
{
	printf("TestAnswer\n");
	if (ac < 3)
	{
		printf("usage: testapp.exe answer <device id>\n");
		return;
	}
	std::string idStr = av[2];
	std::stringstream s(idStr);
	int providerId;
	s >> providerId;
	DialTester dt;
	dt.answer(providerId);
	printf("TestAnswer exit\n");
}

void TestGetProvidersList(int ac, char *av[])
{
	printf("TestGetProvidersList\n");
	DialTester dt;
	DialTester::ProvidersList p = dt.GetProvidersList();
	printf("Id\tName\n");
	for (int j = 0; j < p.size(); j++)
	{
		printf("%d\t%s\n", p[j].providerId, p[j].providerName.c_str());
	}
}

void TestGetLinesList(int ac, char *av[])
{
	printf("TestGetLinesList\n");
	DialTester dt;
	DialTester::LinesList l = dt.GetLinesList();
	printf("l.size() = %zd\n", l.size());
	printf("lineId\tAPIVersion\tName\n");
	for (int j = 0; j < l.size(); j++)
	{
		printf("%d\t%08x\t%s\n", l[j].lineId, l[j].dwAPIVersion, l[j].lineName.c_str());
	}
}

#define HEADING_ID "Line id"
#define HEADING_EXTENSION "Extension"
#define HEADING_USERNAME "Username"
#define HEADING_PASSWORD "Password"
enum columnsEnum {
	COL_ID,
	COL_EXTENSION,
	COL_USERNAME,
	COL_PASSWORD
};
#define NUM_COLUMNS 3
char  szLinesRegKey[] = "Software\\Nuacom\\TSP\\Lines";
char  szIncomingFlag[] = "HandleIncomingCalls";
char  szRegKey[] = "Software\\Nuacom\\TSP";
#define  MAX_DEV_NAME_LENGTH    63
BOOL gbHandleIncomingCalls;

struct LineInfo
{
	std::string extension;
	std::string userName;
	std::string password;
};

void DeleteItem(const std::string &index)
{
	struct locVars
	{
		HKEY  hKeyLines{ 0 };
		~locVars()
		{
			RegCloseKey(hKeyLines);
		}
	} lv;

	auto n = RegOpenKeyEx(
		HKEY_CURRENT_USER,
		szLinesRegKey,
		0,
		KEY_READ,
		&lv.hKeyLines
	);

	RegDeleteKey(lv.hKeyLines, index.c_str());
}

void SaveItem(const std::string &index, const LineInfo &li)
{
	DeleteItem(index);
	struct locVars
	{
		HKEY  hKeyLines{ 0 };
		HKEY  hKeyLine{ 0 };
		~locVars()
		{
			RegCloseKey(hKeyLines);
			RegCloseKey(hKeyLine);
		}
	} lv;

	auto n = RegCreateKeyEx(
		HKEY_CURRENT_USER,
		szLinesRegKey,
		0,
		NULL,
		0,
		KEY_READ | KEY_WRITE, NULL,
		&lv.hKeyLines, NULL
	);

	n = RegCreateKeyEx(
		lv.hKeyLines,
		index.c_str(),
		0,
		NULL,
		0,
		KEY_READ | KEY_WRITE, NULL,
		&lv.hKeyLine, NULL
	);

	// RegSetValue extension number
	DWORD dataType = REG_SZ;
	n = RegSetValueEx(
		lv.hKeyLine,
		"extension",
		0,
		REG_SZ,
		(LPBYTE)li.extension.c_str(),
		li.extension.size()
	);

	// RegSetValue username
	dataType = REG_SZ;
	n = RegSetValueEx(
		lv.hKeyLine,
		"username",
		0,
		REG_SZ,
		(LPBYTE)li.userName.c_str(),
		li.userName.size()
	);

	// RegSetValue password
	dataType = REG_SZ;
	n = RegSetValueEx(
		lv.hKeyLine,
		"password",
		0,
		REG_SZ,
		(LPBYTE)li.password.c_str(),
		li.password.size()
	);
}

void GetIncomingFlag()
{
	HKEY  hKey;
	DWORD dwDataSize, dwDataType;

	auto n = RegOpenKeyEx(
		HKEY_CURRENT_USER,
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

void SaveIncomingFlag()
{
	HKEY  hKey;
	DWORD dwDataSize, dwDataType;

	auto n = RegOpenKeyEx(
		HKEY_CURRENT_USER,
		szRegKey,
		0,
		KEY_READ|KEY_WRITE,
		&hKey
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

using linesCollection = std::map<int, LineInfo>;
linesCollection GetLinesInfo();

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
		HKEY_CURRENT_USER,
		szLinesRegKey,
		0,
		KEY_READ,
		&lv.hKeyLines
	);
	if (n != 0)
	{
		printf("GetLinesInfo: failed to open %s key: %d\n", szLinesRegKey, n);
		return retVal;
	}

	char name[MAX_PATH];
	DWORD nameSize = MAX_PATH;
	for (DWORD index = 0;; index++)
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
			printf("GetLinesInfo: failed to open %s key: %d\n", name, n);
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
			printf("GetLinesInfo: failed to query %s key: %d\n", name, n);
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
			printf("GetLinesInfo: failed to query %s key: %d\n", name, n);
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
			printf("GetLinesInfo: failed to query %s key: %d\n", name, n);
			return retVal;
		}
		li.password = data;

		// convert key name into line number
		int lineNumber = atol(name);
		retVal[lineNumber] = li;
	}

	return retVal;
}

void FillList(HWND hWndList)
{
	ListView_DeleteAllItems(hWndList);
	linesCollection lc = GetLinesInfo();
	int lvIndex = 0;
	for (auto &ll:lc)
	{
		LVITEM Item;
		char s[20];
		_ltoa(ll.first, s, 10);
		Item.mask = LVIF_TEXT;
		Item.iItem = lvIndex++;
		Item.iSubItem = COL_ID;
		Item.pszText = (LPSTR)s;
		ListView_InsertItem(hWndList, &Item);

		Item.iSubItem = COL_EXTENSION;
		Item.pszText = (LPSTR)ll.second.extension.c_str();
		ListView_SetItem(hWndList, &Item);

		Item.iSubItem = COL_USERNAME;
		Item.pszText = (LPSTR)ll.second.userName.c_str();
		ListView_SetItem(hWndList, &Item);

		Item.iSubItem = COL_PASSWORD;
		Item.pszText = (LPSTR)ll.second.password.c_str();
		ListView_SetItem(hWndList, &Item);
	}
	ListView_SetColumnWidth(hWndList, COL_ID, LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hWndList, COL_EXTENSION, LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hWndList, COL_USERNAME, LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hWndList, COL_PASSWORD, 0);
}

void GetLineInfo(HWND hWndList, int index, LineInfo &li, std::string &id)
{
	char s[100];
	LVITEM lv;
	lv.iItem = index;
	lv.iSubItem = COL_ID;
	lv.mask = LVIF_TEXT;
	lv.pszText = s;
	lv.cchTextMax = 100;
	ListView_GetItem(hWndList, &lv);
	id = lv.pszText;
	lv.iSubItem = COL_EXTENSION;
	lv.pszText = s;
	lv.cchTextMax = 100;
	ListView_GetItem(hWndList, &lv);
	li.extension = lv.pszText;
	lv.iSubItem = COL_USERNAME;
	lv.pszText = s;
	lv.cchTextMax = 100;
	ListView_GetItem(hWndList, &lv);
	li.userName = lv.pszText;
	lv.iSubItem = COL_PASSWORD;
	lv.pszText = s;
	lv.cchTextMax = 100;
	ListView_GetItem(hWndList, &lv);
	li.password = lv.pszText;
}

std::string g_index;
LineInfo g_li;
BOOL CALLBACK
EditDialogProc(HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		// Fill edit controls
		auto p = (std::pair<std::string, LineInfo> *)lParam;
		SetDlgItemText(hwnd, IDC_INDEX, p->first.c_str());
		SetDlgItemText(hwnd, IDC_EXTENSION, p->second.extension.c_str());
		SetDlgItemText(hwnd, IDC_USERNAME, p->second.userName.c_str());
		SetDlgItemText(hwnd, IDC_PASSWORD, p->second.password.c_str());
		return TRUE;
	}
	case WM_COMMAND:
	{
		switch (LOWORD((DWORD)wParam))
		{
			char s[100];
		case IDOK:
		case IDCLOSE:
			GetDlgItemText(hwnd, IDC_INDEX, s, 100);
			g_index = s;
			GetDlgItemText(hwnd, IDC_EXTENSION, s, 100);
			g_li.extension = s;
			GetDlgItemText(hwnd, IDC_USERNAME, s, 100);
			g_li.userName = s;
			GetDlgItemText(hwnd, IDC_PASSWORD, s, 100);
			g_li.password = s;
			EndDialog(hwnd, IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		default:
			break;
		}
		break;
	}
	default:
		break;
	}
	return FALSE;
}

BOOL RunEditDialog(HWND hwnd, const std::string &index, const LineInfo &li, std::string &index1, LineInfo &li1)
{
	std::pair<std::string, LineInfo> p(index, li);
	int retVal = DialogBoxParam(NULL,
		MAKEINTRESOURCE(IDD_DIALOG2),
		hwnd,
		(DLGPROC)EditDialogProc,
		(LONG_PTR)&p);
	if (retVal == IDOK)
	{
		index1 = g_index;
		li1 = g_li;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void EditItem(HWND hwnd, const std::string &index, const LineInfo &li)
{
	LineInfo li1;
	std::string index1;
	if (RunEditDialog(hwnd, index, li, index1, li1))
	{
		DeleteItem(index);
		SaveItem(index1, li1);
	}
}

void EditLine(HWND hwnd, HWND hWndList)
{
	int iSelect = ListView_GetNextItem(hWndList, -1, LVNI_SELECTED);
	if (iSelect != -1)
	{
		LineInfo li;
		std::string index;
		GetLineInfo(hWndList, iSelect, li, index);
		EditItem(hwnd, index, li);
	}
}

void AddLine(HWND hwnd, HWND hWndList)
{
	auto lines = GetLinesInfo();
	int newIndex = 0;
	for (;; newIndex++)
	{
		if (lines.find(newIndex) == lines.end())
			break;
	}
	char s[20];
	ltoa(newIndex, s, 10);
	LineInfo li;
	EditItem(hwnd, s, li);
}

void RemoveLine(HWND hWndList)
{
	int iSelect = ListView_GetNextItem(hWndList, -1, LVNI_SELECTED);
	if (iSelect != -1)
	{
		LineInfo li;
		std::string index;
		GetLineInfo(hWndList, iSelect, li, index);
		DeleteItem(index);
	}
}

BOOL CALLBACK
DialogProc(HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		// Set up columns: extension, username
		LVCOLUMN Column;
		HWND hWndList = GetDlgItem(hwnd, IDC_LINESLIST);
		HWND hWndCheck = GetDlgItem(hwnd, IDC_INCOMING);

		Column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_ORDER | LVCF_SUBITEM;
		Column.fmt = LVCFMT_LEFT;

		// id
		Column.pszText = (LPSTR)HEADING_ID;
		Column.cx = 140;
		Column.iSubItem = COL_ID;
		Column.iOrder = 0;
		Column.fmt = LVCFMT_LEFT;
		ListView_InsertColumn(hWndList, COL_ID, &Column);

		// extension
		Column.pszText = (LPSTR)HEADING_EXTENSION;
		Column.cx = 140;
		Column.iSubItem = COL_EXTENSION;
		Column.iOrder = 1;
		Column.fmt = LVCFMT_LEFT;
		ListView_InsertColumn(hWndList, COL_EXTENSION, &Column);

		// username
		Column.pszText = (LPSTR)HEADING_USERNAME;
		Column.cx = 140;
		Column.iSubItem = COL_USERNAME;
		Column.iOrder = 2;
		ListView_InsertColumn(hWndList, COL_USERNAME, &Column);

		// password
		Column.pszText = (LPSTR)HEADING_PASSWORD;
		Column.cx = 140;
		Column.iSubItem = COL_PASSWORD;
		Column.iOrder = 3;
		ListView_InsertColumn(hWndList, COL_PASSWORD, &Column);

		ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

		FillList(hWndList);
		GetIncomingFlag();
		Button_SetCheck(hWndCheck, gbHandleIncomingCalls ? BST_CHECKED : BST_UNCHECKED);
		return TRUE;
	}
	case WM_COMMAND:
	{
		HWND hWndList = GetDlgItem(hwnd, IDC_LINESLIST);
		HWND hWndCheck = GetDlgItem(hwnd, IDC_INCOMING);
		switch (LOWORD((DWORD)wParam))
		{
		case IDOK:
		case IDCANCEL:
		case IDCLOSE:
			EndDialog(hwnd, 0);
			return TRUE;
		case IDC_INCOMING:
		{
			auto a = Button_GetCheck(hWndCheck);
			gbHandleIncomingCalls = (a == BST_CHECKED);
			SaveIncomingFlag();
			return TRUE;
		}
		case IDC_ADD:
		{
			AddLine(hwnd, hWndList);
			FillList(hWndList);
			return TRUE;
		}
		case IDC_REMOVE:
		{
			RemoveLine(hWndList);
			FillList(hWndList);
			return TRUE;
		}
		case IDC_EDIT:
		{
			EditLine(hwnd, hWndList);
			FillList(hWndList);
			return TRUE;
		}
		default:
			break;
		}
		break;
	}
	default:
		break;
	}
	return FALSE;
}

void TestRunConfDialog(int ac, char *av[])
{
	printf("TestRunConfDialog\n");
	DialogBox(NULL,
		MAKEINTRESOURCE(IDD_DIALOG1),
		NULL,
		(DLGPROC)DialogProc);
}

int main(int ac, char *av[])
{
	printf("sizeof(LINECALLINFO) = %d\n", sizeof(LINECALLINFO));
	DWORD retVal = sizeof(LINECALLINFO);
	if (ac < 2)
	{
		printf("usage: testapp.exe <command>\n");
		printf("commands: install, uninstall, dial, answer, getprovlist, getlineslist, runconfdialog\n");
		return 0;
	}
	std::string command = av[1];
	if (command == "install")
		TestInstall(ac, av);
	else if(command == "uninstall")
		TestUninstall(ac, av);
	else if (command == "dial")
		TestDial(ac, av);
	else if (command == "answer")
		TestAnswer(ac, av);
	else if (command == "getprovlist")
		TestGetProvidersList(ac, av);
	else if (command == "getlineslist")
		TestGetLinesList(ac, av);
	else if (command == "runconfdialog")
		TestRunConfDialog(ac, av);
	else
	{
		printf("invalid command\n");
		printf("commands: install, uninstall, dial, answer, getprovlist, getlineslist\n");
	}
	return 0;
}
