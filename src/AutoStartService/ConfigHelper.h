#pragma once
#include <string>

// 读取配置文件中 [Config] 节的指定键值
std::wstring ReadConfigString(const std::wstring& section, const std::wstring& key, const std::wstring& defaultValue = L"");

// 获取当前进程所在目录下的配置文件完整路径
std::wstring GetConfigFilePath();