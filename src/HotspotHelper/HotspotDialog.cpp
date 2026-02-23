#include"pch.h"

#include "HotspotDialog.h"
#include "resource.h"
HotspotDialog::HotspotDialog(HINSTANCE hInstance) : m_hInstance(hInstance) {}

int HotspotDialog::DoModal() {
	return (int)DialogBoxParam(m_hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN),
		nullptr, DlgProc, reinterpret_cast<LPARAM>(this));
}

void HotspotDialog::Refresh() {
	m_config = GetCurrentConfig();
	SetDlgItemText(m_hDlg, IDC_EDIT_SSID, m_config.Ssid().c_str());
	SetDlgItemText(m_hDlg, IDC_EDIT_PASSPHRASE, m_config.Passphrase().c_str());

	int state = GetOperationalState();
	// 0=未知,1=开启,2=关闭,3=开启中,4=关闭中
	// 只当状态明确为 1 时才勾选
	CheckDlgButton(m_hDlg, IDC_CHECK_STATE, state == 1);
}

void HotspotDialog::Apply() {
	WCHAR ssid[64] = {}, pass[64] = {};
	GetDlgItemText(m_hDlg, IDC_EDIT_SSID, ssid, 64);
	GetDlgItemText(m_hDlg, IDC_EDIT_PASSPHRASE, pass, 64);

	auto newConfig = GetCurrentConfig();
	newConfig.Ssid(ssid);
	if (IsPassphraseValid(pass)) newConfig.Passphrase(pass);

	int currentState = GetOperationalState();
	bool wasRunning = (currentState == 1); // 1 表示正在运行

	if (wasRunning) {
		if (!StopHotspot()) {
			MessageBox(m_hDlg, L"无法停止正在运行的热点，请稍后重试。", L"错误", MB_ICONERROR);
			return;
		}
	}

	if (!SetCurrentConfig(newConfig)) {
		MessageBox(m_hDlg, L"配置更新失败，请检查输入或权限。", L"错误", MB_ICONERROR);
		return;
	}

	bool shouldStart = IsDlgButtonChecked(m_hDlg, IDC_CHECK_STATE) == BST_CHECKED;
	if (shouldStart) {
		if (!StartHotspot()) {
			MessageBox(m_hDlg, L"热点启动失败。", L"错误", MB_ICONERROR);
		}
	}

	Refresh();
}

INT_PTR CALLBACK HotspotDialog::DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_INITDIALOG) {
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		auto pThis = reinterpret_cast<HotspotDialog*>(lParam);
		pThis->m_hDlg = hDlg;
		HICON hIcon = LoadIcon(pThis->m_hInstance, MAKEINTRESOURCE(IDI_HOTSPOTHELPER));
		if (hIcon) {
			SendMessage(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
		}
		pThis->Refresh();
		return TRUE;
	}
	auto pThis = reinterpret_cast<HotspotDialog*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
	if (pThis) return pThis->HandleMessage(msg, wParam, lParam);
	return FALSE;
}

INT_PTR HotspotDialog::HandleMessage(UINT msg, WPARAM wParam, LPARAM /*lParam*/) {
	switch (msg) {
	case WM_COMMAND: {
		UINT id = LOWORD(wParam);
		switch (id) {
		case IDC_BUTTON_REFRESH:
			Refresh();
			return TRUE;
		case IDC_BUTTON_SET:
			Apply();
			return TRUE;
		}
		break;
	}
	case WM_CLOSE:
		DestroyWindow(m_hDlg);
		return TRUE;
	}
	return FALSE;
}