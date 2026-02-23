#include <Windows.h>
#include <Shellapi.h>

#pragma comment(lib, "Shell32.lib")

constexpr UINT WM_TRAY_MSG = WM_USER + 1;
constexpr UINT HOTKEY_ID = 100;
constexpr UINT ID_SELECT_WINDOW = 1001;
constexpr UINT ID_UNSET_TOPMOST = 1002;
constexpr UINT ID_EXIT = 1003;

HWND g_hwndMsg = nullptr;
HWND g_hwndTopmost = nullptr;
NOTIFYICONDATAW g_nid = { sizeof(NOTIFYICONDATAW) };

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitTrayIcon(HWND);
void DestroyTrayIcon();
bool SetWindowTopmost(HWND, bool);
HWND SelectWindowByClick(HWND);
void ToggleForegroundWindowTopmost();
void RegisterHotkey(HWND);
void UnregisterHotkey(HWND);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"TopmostTrayClass";
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"窗口类注册失败", L"错误", MB_ICONERROR);
        return 1;
    }

    g_hwndMsg = CreateWindowExW(0, wc.lpszClassName, L"MsgWindow", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInst, nullptr);
    if (!g_hwndMsg) {
        MessageBoxW(nullptr, L"消息窗口创建失败", L"错误", MB_ICONERROR);
        return 1;
    }

    InitTrayIcon(g_hwndMsg);
    RegisterHotkey(g_hwndMsg);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_TRAY_MSG:
        if (lp == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, ID_SELECT_WINDOW, L"鼠标选择窗口置顶\tCtrl+T");
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(menu, MF_STRING, ID_UNSET_TOPMOST, L"取消置顶");
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(menu, MF_STRING, ID_EXIT, L"退出");

            SetForegroundWindow(hwnd);
            TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(menu);
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case ID_SELECT_WINDOW: {
            HWND hwndSel = SelectWindowByClick(hwnd);
            if (hwndSel) {
                // 取消之前的置顶窗口
                if (g_hwndTopmost && IsWindow(g_hwndTopmost)) {
                    SetWindowTopmost(g_hwndTopmost, false);
                    g_hwndTopmost = nullptr;
                }
                if (SetWindowTopmost(hwndSel, true))
                    g_hwndTopmost = hwndSel;
                else
                    MessageBoxW(hwnd, L"无法设置窗口置顶", L"提示", MB_ICONWARNING);
            }
            return 0;
        }
        case ID_UNSET_TOPMOST:
            if (g_hwndTopmost && IsWindow(g_hwndTopmost)) {
                SetWindowTopmost(g_hwndTopmost, false);
                g_hwndTopmost = nullptr;
            }
            else {
                MessageBoxW(hwnd, L"当前无置顶窗口", L"提示", MB_ICONWARNING);
            }
            return 0;
        case ID_EXIT:
            DestroyTrayIcon();
            UnregisterHotkey(hwnd);
            DestroyWindow(hwnd);
            PostQuitMessage(0);
            return 0;
        }
        return 0;

    case WM_HOTKEY:
        if (wp == HOTKEY_ID) {
            ToggleForegroundWindowTopmost();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
}

void InitTrayIcon(HWND hwnd) {
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAY_MSG;
    g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, L"窗口置顶工具");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void DestroyTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    if (g_nid.hIcon) DestroyIcon(g_nid.hIcon);
}

bool SetWindowTopmost(HWND hwnd, bool topmost) {
    if (!hwnd || !IsWindow(hwnd)) return false;
    return SetWindowPos(hwnd, topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
        0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE) != 0;
}

HWND SelectWindowByClick(HWND parent) {
    SetCursor(LoadCursorW(nullptr, IDC_CROSS));
    SetCapture(parent);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_LBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            ReleaseCapture();
            SetCursor(LoadCursorW(nullptr, IDC_ARROW));
            return WindowFromPoint(pt);
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            ReleaseCapture();
            SetCursor(LoadCursorW(nullptr, IDC_ARROW));
            return nullptr; // 用户取消
        }
        if (msg.message == WM_QUIT) {
            PostQuitMessage((int)msg.wParam);
            ReleaseCapture();
            SetCursor(LoadCursorW(nullptr, IDC_ARROW));
            return nullptr;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return nullptr;
}

void ToggleForegroundWindowTopmost() {
    HWND hwndFore = GetForegroundWindow();
    if (!hwndFore || !IsWindow(hwndFore) || hwndFore == g_hwndMsg)
        return;

    if (hwndFore == g_hwndTopmost) {
        if (SetWindowTopmost(g_hwndTopmost, false))
            g_hwndTopmost = nullptr;
    }
    else {
        if (g_hwndTopmost && IsWindow(g_hwndTopmost))
            SetWindowTopmost(g_hwndTopmost, false);
        if (SetWindowTopmost(hwndFore, true))
            g_hwndTopmost = hwndFore;
    }
}

void RegisterHotkey(HWND hwnd) {
    RegisterHotKey(hwnd, HOTKEY_ID, MOD_CONTROL | MOD_NOREPEAT, 'T');
}

void UnregisterHotkey(HWND hwnd) {
    UnregisterHotKey(hwnd, HOTKEY_ID);
}