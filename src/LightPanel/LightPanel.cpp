#include "pch.h"
#include <tchar.h>
#include <shellapi.h>
#include "uiaccess.h"
#include "Resource.h"
#include <string>

using namespace std;

#define WM_CUSTOM_1     (WM_USER + 66)
#define WM_TIME_CLEAR   (WM_USER + 67)

static HINSTANCE g_hInst = nullptr;
static HWND      g_hDlg = nullptr;
static BOOL      g_hasUIAccess = FALSE;
static BOOL      g_alwaysTop = TRUE;

static HANDLE    g_timerQueue = nullptr;
static HANDLE    g_timerKeepTop = nullptr;      // 周期置顶定时器
static HANDLE    g_timerReset = nullptr;        // 重置点击次数的定时器
static bool      g_isTimerResetActive = false;  // 重置定时器是否有效
static int       g_clickCount = 3;              // 剩余点击次数

static POINT     g_dragOffset = { 0 };          // 拖拽时鼠标相对于窗口左上角的偏移
static BOOL      g_isDragging = FALSE;          // 是否正在拖拽

wstring GetModulePath()
{
	WCHAR buf[MAX_PATH] = {};
	GetModuleFileName(nullptr, buf, MAX_PATH);
	return buf;
}

wstring GetFolderPath()
{
	wstring path = GetModulePath();
	size_t pos = path.find_last_of(L"\\/");
	return path.substr(0, pos + 1);
}

void SetTopmost(BOOL topmost)
{
	HWND insertAfter = topmost ? HWND_TOPMOST : HWND_NOTOPMOST;
	SetWindowPos(g_hDlg, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

VOID CALLBACK TimerProcKeepTop(PVOID, BOOLEAN)
{
	SetTopmost(g_alwaysTop);
}

VOID CALLBACK TimerProcReset(PVOID, BOOLEAN)
{
	PostMessage(g_hDlg, WM_TIME_CLEAR, 0, 0);
}

bool CreateKeepTopTimer()
{
	g_timerQueue = CreateTimerQueue();
	if (!g_timerQueue)
		return false;

	if (!CreateTimerQueueTimer(&g_timerKeepTop, g_timerQueue,
		TimerProcKeepTop, nullptr, 0, 1000, 0))
		return false;

	return true;
}

static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		// 隐藏任务栏图标
		SetWindowLongPtr(hDlg, GWL_EXSTYLE, GetWindowLongPtr(hDlg, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
		g_hDlg = hDlg;
		SetTopmost(g_alwaysTop);
		return TRUE;
	}

	case WM_COMMAND:
	{
		UINT id = LOWORD(wParam);
		switch (id)
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, id);
			break;

		case IDC_MAIN_OPEN:
			ShellExecute(nullptr, nullptr,
				L"notepad.exe", nullptr, nullptr, SW_SHOWDEFAULT);
			break;

		case IDC_BUTTON_ESC:
		{
			// 减少计数，更新按钮文字
			g_clickCount--;
			WCHAR text[10] = {};
			_itow_s(g_clickCount, text, 10);
			SetWindowText(GetDlgItem(hDlg, IDC_BUTTON_ESC), text);

			if (g_clickCount <= 0)
			{
				PostQuitMessage(0);
				break;
			}

			// 如果重置定时器尚未启动，则启动它（2秒后重置计数）
			if (!g_isTimerResetActive)
			{
				if (CreateTimerQueueTimer(&g_timerReset, g_timerQueue,
					TimerProcReset, nullptr, 2000, 0, 0))
				{
					g_isTimerResetActive = true;
				}
			}
			break;
		}
		}
		break;
	}

	case WM_TIME_CLEAR:
	{
		// 重置计数和按钮文字
		g_clickCount = 3;
		SetWindowText(GetDlgItem(hDlg, IDC_BUTTON_ESC), L"Esc");

		// 删除重置定时器
		if (g_isTimerResetActive)
		{
			(void)DeleteTimerQueueTimer(g_timerQueue, g_timerReset, nullptr);
			g_isTimerResetActive = false;
		}
		break;
	}

	case WM_MOVING:
		// 移动过程中强制置顶（防止Z序变化）
		PostMessage(hDlg, WM_CUSTOM_1, 0, 0);
		break;

	case WM_CUSTOM_1:
		SetTopmost(g_alwaysTop);
		break;

	case WM_LBUTTONDOWN:
	{
		// 开始拖拽
		g_isDragging = TRUE;
		SetCapture(hDlg);

		POINT curPos;
		GetCursorPos(&curPos);
		g_dragOffset = curPos;
		ScreenToClient(hDlg, &g_dragOffset);   // 鼠标相对于窗口客户区的位置

		SetCursor(LoadCursor(nullptr, IDC_HAND));
		break;
	}

	case WM_MOUSEMOVE:
		if (g_isDragging)
		{
			SetCursor(LoadCursor(nullptr, IDC_HAND));

			POINT curPos;
			GetCursorPos(&curPos);
			int newX = curPos.x - g_dragOffset.x;
			int newY = curPos.y - g_dragOffset.y;
			SetWindowPos(hDlg, nullptr, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		}
		break;

	case WM_LBUTTONUP:
		if (g_isDragging)
		{
			g_isDragging = FALSE;
			ReleaseCapture();
			SetCursor(LoadCursor(nullptr, IDC_ARROW));
		}
		break;
	}

	return FALSE;
}

static int InitInstance(HINSTANCE hInst)
{
	(void)CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	DWORD dwErr = PrepareForUIAccess();
	g_hasUIAccess = (dwErr == ERROR_SUCCESS);
	g_hInst = hInst;

	// 创建定时器队列和置顶定时器
	if (!CreateKeepTopTimer())
		return -1;

	INT_PTR ret = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_MAIN), nullptr, DialogProc);
	g_hDlg = nullptr;

	// 清理定时器队列
	if (g_timerQueue)
		(void)DeleteTimerQueue(g_timerQueue);

	CoUninitialize();
	return (int)ret;
}

int APIENTRY _tWinMain(HINSTANCE hInst, HINSTANCE, LPTSTR, int)
{
	return InitInstance(hInst);
}