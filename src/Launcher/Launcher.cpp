#include "framework.h"
#include "WinUtils/WinUtils.h"
#include "WinUtils/INI.h"
#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>
#include <WinUtils/StrConvert.h>

using namespace std;
using namespace WinUtils;

static void CreateSampleIni(const wstring& path) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"w, ccs=UTF-8") == 0 && f) {
        wstring sample = L"\n[Pro1]\nProgram=notepad.exe\n\n[Pro2]\nProgram=.\\example.exe\nParams=arg1 arg2\n\n[Pro3]\nProgram=..\\script.bat\nRunAsAdmin=1\nShowWnd=7\n";
        fwrite(sample.c_str(), sizeof(wchar_t), sample.length(), f);
        fclose(f);
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    wstring iniPath = GetCurrentProcessPath();
    iniPath = iniPath.substr(0, iniPath.find_last_of(L'.')) + L".ini";

    if (!filesystem::exists(iniPath)) {
        CreateSampleIni(iniPath);
        return 0;
    }

    INIFile iniFile(iniPath);
    INIStructure data;
    if (!iniFile.read(data)) {
        return 1;  // 读取失败
    }

    for (const auto& sectionPair : data) {
        const wstring& sectionName = sectionPair.first;
        const INIMap<wstring>& kvMap = sectionPair.second;

        wstring program = kvMap.get(L"Program");
        if (program.empty()) continue;

        wstring params = kvMap.get(L"Params");
        wstring workDir = kvMap.get(L"WorkDir");

        bool runAsAdmin = kvMap.get_as<bool>(L"RunAsAdmin", false);
        int showWnd = kvMap.get_as<int>(L"ShowWnd", SW_NORMAL);

        RunExternalProgram(ResolvePath(program),
            runAsAdmin ? L"runas" : L"open",
            params, workDir, showWnd);
        Sleep(100);
    }

    return 0;
}