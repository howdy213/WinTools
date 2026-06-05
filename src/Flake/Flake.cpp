#include "WinUtils/WinPch.h"

#include <windows.h>
#include <vector>
#include <chrono>
#include "WinUtils/WinUtils.h"

using namespace WinUtils;
using namespace std::chrono;

constexpr int kRequiredClicks = 5;
constexpr int kTimeLimitMs = 2000;
std::vector<steady_clock::time_point> g_clickTimes;


LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

bool CreateFullscreenWindow(HINSTANCE hInstance) {
	constexpr wchar_t CLASS_NAME[] = L"BlackScreenClass";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	if (!RegisterClass(&wc)) {
		MessageBoxW(nullptr, L"窗口类注册失败！", L"错误", MB_OK | MB_ICONERROR);
		return false;
	}

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	HWND hwnd = CreateWindowExW(
		WS_EX_TOPMOST,
		CLASS_NAME,
		L"全屏黑色窗口",
		WS_POPUP | WS_VISIBLE,
		0, 0, screenWidth, screenHeight,
		nullptr, nullptr, hInstance, nullptr
	);

	if (!hwnd) {
		MessageBoxW(nullptr, L"窗口创建失败！", L"错误", MB_OK | MB_ICONERROR);
		return false;
	}

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	return true;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_LBUTTONDOWN: {
		auto now = steady_clock::now();
		g_clickTimes.push_back(now);

		while (!g_clickTimes.empty() &&
			now - g_clickTimes.front() > milliseconds(kTimeLimitMs)) {
			g_clickTimes.erase(g_clickTimes.begin());
		}

		if (g_clickTimes.size() >= kRequiredClicks)
			PostQuitMessage(0);
		return 0;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		RECT rc;
		GetClientRect(hwnd, &rc);
		FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
		EndPaint(hwnd, &ps);
		return 0;
	}

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
	EnsureSingleInstance(true);
	if (!CreateFullscreenWindow(hInstance))
		return 1;

	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return static_cast<int>(msg.wParam);
}