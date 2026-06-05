// HotspotHelper.cpp
#include "pch.h"
#include "WinUtils/WinPch.h"

#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Networking.Connectivity.h>
#include <winrt/Windows.Networking.NetworkOperators.h>
#include <shellapi.h>
#include <iostream>
#include <string>
#include "resource.h"
#include "HotspotHelper.h"
#include "HotspotDialog.h"
#include <WinUtils/StrConvert.h>
#include <WinUtils/WinUtils.h>
using namespace WinUtils;

using namespace winrt;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking::NetworkOperators;

// 尝试获取 TetheringManager，如果失败则返回空 optional
std::optional<NetworkOperatorTetheringManager> TryGetManager() noexcept {
	try {
		auto profile = NetworkInformation::GetInternetConnectionProfile();
		if (!profile) {
			return std::nullopt;
		}
		return NetworkOperatorTetheringManager::CreateFromConnectionProfile(profile);
	}
	catch (...) {
		return std::nullopt;
	}
}

// 安全获取 TetheringManager
NetworkOperatorTetheringManager GetManagerOrThrow() {
	auto mgr = TryGetManager();
	if (!mgr) {
		throw std::runtime_error("No active Internet connection or unable to access tethering manager.");
	}
	return *mgr;
}

void PrintError(std::wstring_view context, std::wstring_view detail = L"") {
	std::wstring msg(context);
	if (!detail.empty()) {
		msg += L": ";
		msg += detail;
	}
	auto err = GetWindowsErrorMsg();
	if (!err.empty()) {
		msg += L" (";
		msg += err;
		msg += L')';
	}
	std::wcerr << msg << std::endl;
}

void PrintError(std::wstring_view context, const winrt::hresult_error& e) {
	wchar_t buffer[512];
	swprintf_s(buffer, L"%s (HRESULT: 0x%08lX): %s",
		context.data(), static_cast<HRESULT>(e.code()), e.message().c_str());
	PrintError(context, buffer);
}

void PrintError(std::wstring_view context, const std::exception& e) {
	PrintError(context, AnsiToWide(e.what()));
}


bool IsPassphraseValid(std::wstring_view passphrase) noexcept {
	return passphrase.length() >= 8 && passphrase.length() <= 63;
}

HotspotConfig GetCurrentConfig() {
	try {
		winrt::init_apartment();
		return GetManagerOrThrow().GetCurrentAccessPointConfiguration();
	}
	catch (const winrt::hresult_error& e) {
		PrintError(L"GetCurrentConfig", e);
	}
	catch (const std::exception& e) {
		PrintError(L"GetCurrentConfig", e);
	}
	return HotspotConfig{};
}

bool SetCurrentConfig(const HotspotConfig& config) {
	try {
		winrt::init_apartment();
		GetManagerOrThrow().ConfigureAccessPointAsync(config).get();
		return true;
	}
	catch (const winrt::hresult_error& e) {
		PrintError(L"SetCurrentConfig", e);
	}
	catch (const std::exception& e) {
		PrintError(L"SetCurrentConfig", e);
	}
	return false;
}

bool StartHotspot() {
	try {
		winrt::init_apartment();
		auto mgr = GetManagerOrThrow();
		auto result = mgr.StartTetheringAsync().get();
		return result.Status() == TetheringOperationStatus::Success;
	}
	catch (const winrt::hresult_error& e) {
		PrintError(L"StartHotspot", e);
	}
	catch (const std::exception& e) {
		PrintError(L"StartHotspot", e);
	}
	return false;
}

bool StopHotspot() {
	try {
		winrt::init_apartment();
		auto mgr = TryGetManager();
		if (!mgr) {
			// 没有连接，视为已停止
			return true;
		}
		// 检查当前状态，如果已经停止则直接返回成功
		auto state = mgr->TetheringOperationalState();
		if (state == TetheringOperationalState::Off) {
			return true;
		}
		auto result = mgr->StopTetheringAsync().get();
		return result.Status() == TetheringOperationStatus::Success;
	}
	catch (const winrt::hresult_error& e) {
		PrintError(L"StopHotspot", e);
	}
	catch (const std::exception& e) {
		PrintError(L"StopHotspot", e);
	}
	return false;
}

int GetOperationalState() {
	try {
		winrt::init_apartment();
		auto mgr = TryGetManager();
		if (!mgr) {
			return -1;   // 表示无法获取状态
		}
		return static_cast<int>(mgr->TetheringOperationalState());
	}
	catch (const winrt::hresult_error& e) {
		PrintError(L"GetOperationalState", e);
	}
	catch (const std::exception& e) {
		PrintError(L"GetOperationalState", e);
	}
	return -1;
}

bool EasyStart(std::wstring_view ssid, std::wstring_view passphrase) {
	try {
		winrt::init_apartment();
		auto mgr = GetManagerOrThrow();
		if (!ssid.empty() || !passphrase.empty()) {
			auto config = mgr.GetCurrentAccessPointConfiguration();
			if (!ssid.empty()) config.Ssid(ssid);
			if (!passphrase.empty() && IsPassphraseValid(passphrase))
				config.Passphrase(passphrase);
			mgr.ConfigureAccessPointAsync(config).get();
		}
		auto result = mgr.StartTetheringAsync().get();
		if (result.Status() == TetheringOperationStatus::Success) {
			std::wcout << L"Hotspot started successfully." << std::endl;
			return true;
		}
		else {
			std::wcout << L"Failed to start hotspot." << std::endl;
		}
	}
	catch (const winrt::hresult_error& e) {
		PrintError(L"EasyStart", e);
	}
	catch (const std::exception& e) {
		PrintError(L"EasyStart", e);
	}
	return false;
}

