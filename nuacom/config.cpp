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

void SaveIncomingFlag()
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
#define NUM_COLUMNS 4

void FillList(HWND hWndList);
void SaveIncomingFlag();
void AddLine(HWND hwnd, HWND hWndList);
void RemoveLine(HWND hWndList);
void EditLine(HWND hwnd, HWND hWndList);
void EditItem(HWND hwnd, const std::string &index, const LineInfo &li);
void GetLineInfo(HWND hWndList, int index, LineInfo &li, std::string &id);
void DeleteItem(const std::string &index);
BOOL RunEditDialog(HWND hwnd, const std::string &index, const LineInfo &li, std::string &index1, LineInfo &li1);
void SaveItem(const std::string &index, const LineInfo &li);

std::pair<std::string, LineInfo> g_in;
std::pair<std::string, LineInfo> g_out;

TUISPIDLLCALLBACK lpfnUIDLLCallback;

BOOL CALLBACK
EditDialogProc(HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam);

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
		lpfnUIDLLCallback = (TUISPIDLLCALLBACK)lParam;
#if 0
		DBGOUT((3, "DialogProc: lpfnUIDLLCallback = %p, gdwPermanentProviderID = %d", lpfnUIDLLCallback, gdwPermanentProviderID));
		DBGOUT((3, "DialogProc: calling lpfnUIDLLCallback"));
		std::vector<unsigned char> v(sizeof(RPCHeader) + 5);
		auto ph = (RPCHeader*)v.data();
		ph->dwTotalSize = 3 * 4 + 5;
		char * ph1 = (char*)(ph + 1);
		memcpy(ph1, "1234", 5);
		auto n = lpfnUIDLLCallback(gdwPermanentProviderID, TUISPIDLL_OBJECT_PROVIDERID, v.data(), v.size());
		DBGOUT((3, "DialogProc: calling lpfnUIDLLCallback done. n = %08x", n));
#endif
		// Set up columns: id, extension, username
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

void FillList(HWND hWndList)
{
	ListView_DeleteAllItems(hWndList);
	linesCollection lc = GetLinesInfo();
	int lvIndex = 0;
	for (auto &ll : lc)
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
	_ltoa(newIndex, s, 10);
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
		HKEY_LOCAL_MACHINE,
		szLinesRegKey,
		0,
		KEY_READ,
		&lv.hKeyLines
	);

	RegDeleteKey(lv.hKeyLines, index.c_str());
}

BOOL RunEditDialog(HWND hwnd, const std::string &index, const LineInfo &li, std::string &index1, LineInfo &li1)
{
	DBGOUT((3, "RunEditDialog"));
	g_in.first = index;
	g_in.second = li;
	int retVal = DialogBox(hInst,
		MAKEINTRESOURCE(IDD_DIALOG2),
		hwnd,
		(DLGPROC)EditDialogProc);
	if (retVal == IDOK)
	{
		index1 = g_out.first;
		li1 = g_out.second;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
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
		HKEY_LOCAL_MACHINE,
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
		SetDlgItemText(hwnd, IDC_INDEX, g_in.first.c_str());
		SetDlgItemText(hwnd, IDC_EXTENSION, g_in.second.extension.c_str());
		SetDlgItemText(hwnd, IDC_USERNAME, g_in.second.userName.c_str());
		SetDlgItemText(hwnd, IDC_PASSWORD, g_in.second.password.c_str());
		return TRUE;
	}
	case WM_COMMAND:
	{
		switch (LOWORD((DWORD)wParam))
		{
			char s[MAX_DEV_NAME_LENGTH];
		case IDOK:
		case IDCLOSE:
			GetDlgItemText(hwnd, IDC_INDEX, s, MAX_DEV_NAME_LENGTH);
			g_out.first = s;
			GetDlgItemText(hwnd, IDC_EXTENSION, s, MAX_DEV_NAME_LENGTH);
			g_out.second.extension = s;
			GetDlgItemText(hwnd, IDC_USERNAME, s, MAX_DEV_NAME_LENGTH);
			g_out.second.userName = s;
			GetDlgItemText(hwnd, IDC_PASSWORD, s, MAX_DEV_NAME_LENGTH);
			g_out.second.password = s;
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
