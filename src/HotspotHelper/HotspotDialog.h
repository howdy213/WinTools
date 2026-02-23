#pragma once
#include <Windows.h>
#include "HotspotHelper.h"
class HotspotDialog {
public:
	explicit HotspotDialog(HINSTANCE hInstance);
	int DoModal();

private:
	HINSTANCE m_hInstance;
	HWND      m_hDlg = nullptr;
	HotspotConfig m_config;

	void Refresh();
	void Apply();
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
};