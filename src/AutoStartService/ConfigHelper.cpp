#include "ConfigHelper.h"
#include "Common.h"
#include <WinUtils/INI.h>
#include <WinUtils/WinUtils.h>

std::wstring ReadConfigString(const std::wstring& section, const std::wstring& key, const std::wstring& defaultValue) {
    std::wstring configPath = WinUtils::GetCurrentProcessDir() + CONFIG_FILE_NAME;
    WinUtils::INIFile iniFile(configPath);
    WinUtils::INIStructure data;
    if (!iniFile.read(data)) {
        return defaultValue;
    }
    if (!data.has(section)) {
        return defaultValue;
    }
    const auto& kv = data.get(section);
    return kv.has(key) ? kv.get(key) : defaultValue;
}

std::wstring GetConfigFilePath() {
    return WinUtils::GetCurrentProcessDir() + CONFIG_FILE_NAME;
}

std::wstring GetConfiguredServiceName() {
    std::wstring name = ReadConfigString(L"Config", L"ServiceName");
    return name.empty() ? DEFAULT_SERVICE_NAME : name;
}

std::wstring GetConfiguredServiceDescription() {
    return ReadConfigString(L"Config", L"ServiceDescription");
}