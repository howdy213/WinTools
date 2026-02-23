#include "resource.h"
#include <windows.h>

#include <shellapi.h>
#include <sstream>
#include <string>
#include <strsafe.h>
#include <tchar.h>
#include <time.h>
using namespace std;

// 常量定义
const int DEFAULT_DELAY_SECONDS = 60;
const int TIMER_ID = 1;
const wstring COUNTDOWN_TEXT = L"系统将在 %d 秒后关机";
const wstring FONT_NAME = L"黑体";
const int FONT_SIZE = 13;

// 全局变量
HANDLE g_hEvent = NULL;
HFONT g_hFont = NULL;
BOOL g_bShowDialog = TRUE;
int g_nCountdown = DEFAULT_DELAY_SECONDS;

// 链接器清单依赖
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// 函数声明
INT_PTR CALLBACK ShutdownDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void StartCountdownTimer(HWND hwnd);
BOOL ShutdownSystem();
void BackgroundCountdown(int nDelay);
HFONT CreateLargeFont(HWND hwndCtrl);
void SetWindowTopMost(HWND hwnd, BOOL bTopMost);
BOOL ParseCommandLine(LPCWSTR lpCmdLine, int& nDelay, BOOL& bShowDialog);
void ShowLastError(LPCWSTR lpOperation);

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	if (!ParseCommandLine(lpCmdLine, g_nCountdown, g_bShowDialog))
	{
		MessageBoxW(NULL,
			L"ShutdownEx [秒数] [-n]",
			L"参数错误",
			MB_ICONERROR);
		return 1;
	}

	if (g_bShowDialog)
	{
		g_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (g_hEvent == NULL)
		{
			ShowLastError(L"创建事件对象失败");
			return 1;
		}
		INT_PTR nResult = DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_SHUTDOWN), NULL, ShutdownDialogProc);
		if (g_hFont != NULL)
		{
			DeleteObject(g_hFont);
			g_hFont = NULL;
		}
		if (g_hEvent != NULL)
		{
			CloseHandle(g_hEvent);
			g_hEvent = NULL;
		}
		return (int)nResult;
	}
	else
	{
		BackgroundCountdown(g_nCountdown);
		return 0;
	}
}

void SetWindowTopMost(HWND hwnd, BOOL bTopMost)
{
	if (hwnd == NULL)return;
	SetWindowPos(hwnd,
		bTopMost ? HWND_TOPMOST : HWND_NOTOPMOST,
		0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

HFONT CreateLargeFont(HWND hwndCtrl)
{
	if (hwndCtrl == NULL)return NULL;
	HDC hdc = GetDC(hwndCtrl);
	if (hdc == NULL)return NULL;
	int nFontHeight = -MulDiv(FONT_SIZE, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(hwndCtrl, hdc);
	LOGFONT logFont = { 0 };
	logFont.lfHeight = nFontHeight;
	logFont.lfWeight = FW_NORMAL;
	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	logFont.lfQuality = DEFAULT_QUALITY;
	logFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	StringCchCopyW(logFont.lfFaceName, LF_FACESIZE, FONT_NAME.c_str());
	return CreateFontIndirect(&logFont);
}

INT_PTR CALLBACK ShutdownDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND s_hwndCountdown;

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		s_hwndCountdown = GetDlgItem(hwnd, IDC_STATIC_COUNTDOWN);
		if (s_hwndCountdown == NULL)
		{
			EndDialog(hwnd, -1);
			return (INT_PTR)FALSE;
		}

		g_hFont = CreateLargeFont(s_hwndCountdown);
		if (g_hFont != NULL)SendMessageW(s_hwndCountdown, WM_SETFONT, (WPARAM)g_hFont, TRUE);

		SetWindowTopMost(hwnd, TRUE);
		HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
		HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SHUTDOWNEX));
		if (hIcon != NULL)
		{
			SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		}

		wchar_t szText[256];
		swprintf_s(szText, COUNTDOWN_TEXT.c_str(), g_nCountdown);
		SetWindowTextW(s_hwndCountdown, szText);

		StartCountdownTimer(hwnd);
		return (INT_PTR)TRUE;
	}

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			if (g_hEvent != NULL)
				SetEvent(g_hEvent);
			ShutdownSystem();
			EndDialog(hwnd, IDOK);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			if (g_hEvent != NULL)
				SetEvent(g_hEvent);
			EndDialog(hwnd, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		break;

	case WM_TIMER:
		SetWindowTopMost(hwnd, TRUE);

		g_nCountdown--;
		if (g_nCountdown <= 0)
		{
			KillTimer(hwnd, TIMER_ID);
			ShutdownSystem();
			EndDialog(hwnd, IDOK);
		}
		else
		{
			wchar_t szText[256];
			swprintf_s(szText, COUNTDOWN_TEXT.c_str(), g_nCountdown);
			SetWindowTextW(s_hwndCountdown, szText);
		}
		return (INT_PTR)TRUE;

	case WM_DESTROY:
		KillTimer(hwnd, TIMER_ID);
		break;
	}
	return (INT_PTR)FALSE;
}

void StartCountdownTimer(HWND hwnd)
{
	if (hwnd == NULL)return;
	SetTimer(hwnd, TIMER_ID, 1000, NULL);
}

void BackgroundCountdown(int nDelay)
{
	if (nDelay <= 0)nDelay = DEFAULT_DELAY_SECONDS;
	DWORD64 dwStart = GetTickCount64();
	DWORD64 dwEnd = dwStart + (DWORD64)nDelay * 1000;

	while (GetTickCount64() < dwEnd)
	{
		Sleep(500);
		if (g_hEvent != NULL && WaitForSingleObject(g_hEvent, 0) == WAIT_OBJECT_0) return;
	}
	ShutdownSystem();
}

BOOL ShutdownSystem()
{
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES tkp = { 0 };
	BOOL bResult = FALSE;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		ShowLastError(L"获取进程令牌失败");
		if (hToken != NULL)CloseHandle(hToken);
		return FALSE;
	}

	if (!LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid))
	{
		ShowLastError(L"获取关机权限值失败");
		if (hToken != NULL)CloseHandle(hToken);
		return FALSE;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0))
	{
		ShowLastError(L"调整令牌权限失败");
		if (hToken != NULL)CloseHandle(hToken);
		return FALSE;
	}

	bResult = ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE,
		SHTDN_REASON_MAJOR_OPERATINGSYSTEM |
		SHTDN_REASON_MINOR_UPGRADE |
		SHTDN_REASON_FLAG_PLANNED);

	if (!bResult)ShowLastError(L"执行关机命令失败");
	if (hToken != NULL)CloseHandle(hToken);
	return bResult;
}

// 解析命令行参数
BOOL ParseCommandLine(LPCWSTR lpCmdLine, int& nDelay, BOOL& bShowDialog)
{
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);
	if (argv == NULL)return FALSE;
	bShowDialog = TRUE;
	if (argc >= 1)
	{
		try {
			int delay = _wtoi(argv[0]);
			if (delay > 0||(wstring)argv[0] == L"0")nDelay = delay;
		}
		catch (...) {
			nDelay = DEFAULT_DELAY_SECONDS;
		}
	}
	if (argc >= 2)
	{
		wstring param = argv[1];
		if (param == L"-n" || param == L"/n")
			bShowDialog = FALSE;
	}

	LocalFree(argv);
	return TRUE;
}

// 显示错误信息
void ShowLastError(LPCWSTR lpOperation)
{
	if (lpOperation == NULL)return;
	DWORD dwError = GetLastError();
	if (dwError == 0)return;
	LPWSTR lpMsgBuf = NULL;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf, 0, NULL);

	if (lpMsgBuf != NULL)
	{
		wstring errorMsg = lpOperation;
		errorMsg += L":\n";
		errorMsg += lpMsgBuf;

		MessageBoxW(NULL, errorMsg.c_str(), L"错误", MB_ICONERROR);
		LocalFree(lpMsgBuf);
	}
}