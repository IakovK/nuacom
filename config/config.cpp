// config.cpp : Defines the entry point for the application.
//

#include "header.h"
#include "config.h"
#include "SystemTraySDK.h"
#include "../RPCClient/RPCWrapper.h"

#define MAX_LOADSTRING 100
#define	WM_ICON_NOTIFY WM_APP+10

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ConfigDlgProc(HWND, UINT, WPARAM, LPARAM);
void InitConfigDialog(HWND);
void ConfigureProvider(HWND);
extern "C" VOID
cdecl
SetErrorMessage(HWND hDlg, int id,
	LPCSTR   pszFormat,
	...
);

HINSTANCE hInst;
TCHAR szWindowClass[MAX_LOADSTRING] = "ConfigAppClass";
CSystemTray TrayIcon;

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	// TODO: Place code here.
	MSG msg;

	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex{ 0 };

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = szWindowClass;

	return RegisterClassEx(&wcex);
}

VOID CALLBACK
TimerFunc(
	HWND Arg1,
	UINT Arg2,
	UINT_PTR Arg3,
	DWORD Arg4
)
{
	unsigned char * pszProtocolSequence = (unsigned char *)"ncalrpc";
	unsigned char * pszEndpoint = (unsigned char *)"NuacomService";

	CRPCWrapper rpcw;
	RPC_STATUS status = rpcw.Init(pszProtocolSequence, pszEndpoint);
	if (status != RPC_S_OK)
	{
		return;
	}

	static BOOL bConnectedOrig = FALSE;

	BOOL bChanged = FALSE, bConnected = FALSE;
	status = rpcw.GetConnectionStatus(&bConnected);
	if (status != RPC_S_OK)
	{
		return;
	}
	if (bConnected != bConnectedOrig)
	{
		bChanged = TRUE;
		bConnectedOrig = bConnected;
	}
	if (bConnected)
		TrayIcon.SetIcon(IDI_ICON2);
	else
		TrayIcon.SetIcon(IDI_ICON1);
	if (bChanged)
	{
		if (bConnected)
			TrayIcon.ShowBalloon("Nuacom TAPI status has changed to connected");
		else
			TrayIcon.ShowBalloon("Nuacom TAPI status has changed to disconnected");
	}
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance;

	hWnd = CreateWindow(szWindowClass, "", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	// Create the tray icon
	if (!TrayIcon.Create(hInstance,
		hWnd,                            // Parent window
		WM_ICON_NOTIFY,                  // Icon notify message to use
		_T("Nuacom TSP config utility"), // tooltip
		::LoadIcon(hInstance, (LPCTSTR)IDI_ICON1),
		IDR_POPUP_MENU))
		return FALSE;

	TrayIcon.SetMenuDefaultItem(IDM_CONFIG, FALSE);
	SetTimer(NULL, 0, 1000, TimerFunc);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_ICON_NOTIFY:
		return TrayIcon.OnTrayNotification(wParam, lParam);

	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_CONFIG:
			DialogBox(hInst, (LPCTSTR)IDD_CONFIG, hWnd, (DLGPROC)ConfigDlgProc);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK ConfigDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		InitConfigDialog(hDlg);
		return TRUE;

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
		case IDCLOSE:
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		case IDC_CONFIGURE:
			ConfigureProvider(hDlg);
			return TRUE;
		default:
			break;
		}
	}
		break;
	}
	return FALSE;
}

static void SetControlsState(HWND hDlg, const char *name, const char *password, const char *extension, BOOL bHandleIncoming, BOOL bIncomingLocal, BOOL bConfigured)
{
	if (!bConfigured)
		return;
	ShowWindow(GetDlgItem(hDlg, IDC_NOTICE), SW_HIDE);
	SetDlgItemText(hDlg, IDC_USERNAME, name);
	SetDlgItemText(hDlg, IDC_PASSWORD, password);
	SetDlgItemText(hDlg, IDC_EXTENSION, extension);
	CheckDlgButton(hDlg, IDC_INCOMING, bHandleIncoming ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_INCOMINGLOCAL, bIncomingLocal ? BST_CHECKED : BST_UNCHECKED);
}

void InitConfigDialog(HWND hDlg)
{
	unsigned char * pszProtocolSequence = (unsigned char *)"ncalrpc";
	unsigned char * pszEndpoint = (unsigned char *)"NuacomService";

	CRPCWrapper rpcw;
	SetErrorMessage(hDlg, IDC_STATUSMESSAGE, "");
	RPC_STATUS status = rpcw.Init(pszProtocolSequence, pszEndpoint);
	if (status)
	{
		SetErrorMessage(hDlg, IDC_STATUSMESSAGE, "Service not available. Try again later. Status = %d", status);
		return;
	}

	TCHAR name[255];
	TCHAR password[255];
	TCHAR extension[255];
	BOOL bHandleIncoming;
	BOOL bIncomingLocal;
	BOOL bConfigured;
	RPC_STATUS n = rpcw.GetConfigInfo(name, 255, password, 255, extension, 255, &bHandleIncoming, &bIncomingLocal, &bConfigured);
	if (n != RPC_S_OK)
		SetErrorMessage(hDlg, IDC_STATUSMESSAGE, "rpcw.GetConfigInfo returned: %d", n);
	else
		SetControlsState(hDlg, name, password, extension, bHandleIncoming, bIncomingLocal, bConfigured);
}

void ConfigureProvider(HWND hDlg)
{
	unsigned char * pszProtocolSequence = (unsigned char *)"ncalrpc";
	unsigned char * pszEndpoint = (unsigned char *)"NuacomService";

	CRPCWrapper rpcw;
	SetErrorMessage(hDlg, IDC_STATUSMESSAGE, "");
	RPC_STATUS status = rpcw.Init(pszProtocolSequence, pszEndpoint);
	if (status)
	{
		SetErrorMessage(hDlg, IDC_STATUSMESSAGE, "Service not available. Try again later. Status = %d", status);
		return;
	}

	TCHAR name[255];
	TCHAR password[255];
	TCHAR extension[255];
	BOOL bHandleIncoming;
	BOOL bIncomingLocal;
	BOOL bConfigured = FALSE;

	GetDlgItemText(hDlg, IDC_USERNAME, name, 255);
	GetDlgItemText(hDlg, IDC_PASSWORD, password, 255);
	bHandleIncoming = SendDlgItemMessage(hDlg, IDC_INCOMING, BM_GETCHECK, 0, 0) == BST_CHECKED;
	bIncomingLocal = SendDlgItemMessage(hDlg, IDC_INCOMINGLOCAL, BM_GETCHECK, 0, 0) == BST_CHECKED;
	SetErrorMessage(hDlg, IDC_STATUSMESSAGE, "");
	RPC_STATUS n = rpcw.SetConfigInfo(name, password, extension, 255, bHandleIncoming, bIncomingLocal, &bConfigured);
	if (n != RPC_S_OK)
		SetErrorMessage(hDlg, IDC_STATUSMESSAGE, "rpcw.GetConfigInfo returned: %d", n);
	else
	{
		SetControlsState(hDlg, name, password, extension, bHandleIncoming, bIncomingLocal, bConfigured);
		if (!bConfigured)
		{
			SetErrorMessage(hDlg, IDC_STATUSMESSAGE, "Failed to set config. Either server is unavailable or username/password is incorrect");
			TrayIcon.ShowBalloon("Nuacom TAPI authentication failed, please check username/password");
		}
	}
}

#define CCHOF(x)                    (sizeof(x)/sizeof(*(x)))
extern "C" VOID
cdecl
SetErrorMessage(HWND hDlg, int id,
	LPCSTR   pszFormat,
	...
)
{
	va_list         vaList;

	static CHAR    OutBuf[1000];
	va_start(vaList, pszFormat);
	if (!SUCCEEDED(StringCchVPrintfA(OutBuf, CCHOF(OutBuf), pszFormat, vaList)))
	{
		return;
	}
	va_end(vaList);

	SetDlgItemText(hDlg, id, OutBuf);
}

