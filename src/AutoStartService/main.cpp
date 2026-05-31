#include <WinUtils/WinPch.h>

#include <filesystem>
#include <iostream>

#include <WinUtils/WinSvcMgr.h>
#include <WinUtils/Console.h>
#include <WinUtils/WinUtils.h>

#include "ServiceUtils.h"
#include "Common.h"
#include "ConfigHelper.h"

using namespace WinUtils;
using namespace std;

static void CreateSampleIni(const wstring& path) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"w, ccs=UTF-8") == 0 && f) {
        wstring sample =
            L"[Config]\n"
            L"Program=..\\program.exe\n"
            L"Params=arg1 arg2\n"
            L"WorkDir=C:\\\n"
            L"ShowWnd=7\n"
            L"KeepRunning=0\n"
            L"ServiceName=AutoStartService\n"
            L"ServiceDescription=Auto start user program in active session\n";
        fwrite(sample.c_str(), sizeof(wchar_t), sample.length(), f);
        fclose(f);
    }
}

// 检查服务是否已安装
static bool IsServiceInstalled(const std::wstring& serviceName) {
    UniqueScHandle scm(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!scm) return false;
    UniqueScHandle svc(OpenServiceW(scm.get(), serviceName.c_str(), SERVICE_QUERY_STATUS));
    return svc != nullptr;
}

// 设置服务描述（在安装成功后调用）
static bool SetServiceDescription(const std::wstring& serviceName, const std::wstring& description) {
    if (description.empty()) return true;
    UniqueScHandle scm(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
    if (!scm) return false;
    UniqueScHandle svc(OpenServiceW(scm.get(), serviceName.c_str(), SERVICE_CHANGE_CONFIG));
    if (!svc) return false;
    SERVICE_DESCRIPTIONW sd = { const_cast<LPWSTR>(description.c_str()) };
    return ChangeServiceConfig2W(svc.get(), SERVICE_CONFIG_DESCRIPTION, &sd) != 0;
}

int wmain(int argc, wchar_t* argv[]) {
    Console console;
    console.setLocale();

    // 确保配置文件存在，不存在则创建示例
    std::wstring configPath = GetConfigFilePath();
    if (!filesystem::exists(configPath)) {
        CreateSampleIni(configPath);
        // 刚创建示例配置，此时应提示用户编辑配置后再操作
        if (argc == 1) {
            wcout << L"配置文件已创建，请根据需要编辑 " << configPath << L" 后重新运行。\n";
        }
        return 0;
    }

    // 读取动态服务名
    std::wstring serviceName = GetConfiguredServiceName();
    std::wstring serviceDesc = GetConfiguredServiceDescription();

    // 命令行参数处理
    if (argc == 2) {
        wstring arg = argv[1];

        if (arg == L"/install") {
            RequireAdminPrivilege();
            WinSvcMgr svcMgr(serviceName);
            wstring modulePath = GetCurrentProcessPath();
            if (svcMgr.Install(modulePath)) {
                // 设置服务描述
                if (!SetServiceDescription(serviceName, serviceDesc)) {
                    wcout << L"警告：设置服务描述失败（错误码：" << GetLastError() << L"）\n";
                }
                wcout << L"服务安装成功。服务名：" << serviceName << L"\n";
                return 0;
            }
            else {
                wcout << L"服务安装失败，错误码: " << svcMgr.GetLastError() << endl;
                return 1;
            }
        }

        if (arg == L"/uninstall") {
            RequireAdminPrivilege();
            WinSvcMgr svcMgr(serviceName);
            if (svcMgr.Uninstall()) {
                wcout << L"服务卸载成功。服务名：" << serviceName << L"\n";
                return 0;
            }
            else {
                wcout << L"服务卸载失败，错误码: " << svcMgr.GetLastError() << endl;
                return 1;
            }
        }

        if (arg == L"/query") {
            bool installed = IsServiceInstalled(serviceName);
            wcout << (installed ? 1 : 0) << endl;
            return installed ? 1 : 0;
        }

        wcout << L"无效参数！支持：/install 或 /uninstall 或 /query\n";
        return 1;
    }

    // 服务模式启动：动态构建服务分发表
    // 需要保证服务名字符串在分发表生命周期内有效（使用静态或全局）
    static std::wstring dynamicServiceName = serviceName;
    SERVICE_TABLE_ENTRYW table[] = {
        {dynamicServiceName.data(), ServiceMain},
        {nullptr, nullptr}
    };

    if (!StartServiceCtrlDispatcherW(table)) {
        wcout << L"启动服务调度器失败，错误码:" << GetLastError() << endl;
        return 1;
    }
    return 0;
}