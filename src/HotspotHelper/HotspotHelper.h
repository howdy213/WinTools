// HotspotHelper.h
#pragma once
#include <string>
#include <winrt/Windows.Networking.NetworkOperators.h>
using HotspotConfig = winrt::Windows::Networking::NetworkOperators::NetworkOperatorTetheringAccessPointConfiguration;
using HotspotBand = winrt::Windows::Networking::NetworkOperators::TetheringWiFiBand;

bool IsPassphraseValid(std::wstring_view passphrase) noexcept;
HotspotConfig GetCurrentConfig();
bool SetCurrentConfig(const HotspotConfig& config);
bool StartHotspot();
bool StopHotspot();
int GetOperationalState();
bool EasyStart(std::wstring_view ssid = L"", std::wstring_view passphrase = L"");
