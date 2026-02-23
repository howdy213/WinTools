#define INI_CASE_SENSITIVE
#include "WinUtils/WinUtils.h"
#include "WinUtils/Console.h"
#include "WinUtils/INI.h"
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <filesystem>

using namespace std;
using namespace WinUtils;

struct TimedTask {
    wstring name;
    int hour = 0, minute = 0, second = 0;
    LaunchItem launch;
};

// 解析时间字符串 "HH:MM:SS"
static bool ParseTime(const wstring& timeStr, int& hour, int& minute, int& second) {
    wstringstream wss(timeStr);
    wstring part;
    int parts[3] = {};
    int idx = 0;
    while (getline(wss, part, L':') && idx < 3)
        parts[idx++] = _wtoi(part.c_str());
    if (idx == 3) {
        hour = parts[0]; minute = parts[1]; second = parts[2];
        return true;
    }
    return false;
}

// 从 INI 结构中读取所有任务
static vector<TimedTask> ReadTasks(const wstring& path) {
    vector<TimedTask> tasks;
    INIFile iniFile(path);
    INIStructure data;

    if (!iniFile.read(data)) {
        wcout << L"无法读取配置文件：" << path << endl;
        return tasks;
    }

    for (const auto& sectionPair : data) {
        const wstring& sectionName = sectionPair.first;
        const INIMap<wstring>& kvMap = sectionPair.second;

        TimedTask task;
        task.name = sectionName;
        wstring timeStr = kvMap.get(L"Time");
        if (!ParseTime(timeStr, task.hour, task.minute, task.second)) {
            wcout << L"跳过 [" << sectionName << L"]：时间格式无效" << endl;
            continue;
        }
        task.launch.program = kvMap.get(L"Program");
        if (task.launch.program.empty()) {
            wcout << L"跳过 [" << sectionName << L"]：缺少 Program 字段" << endl;
            continue;
        }
        task.launch.params = kvMap.get(L"Params");
        task.launch.workDir = kvMap.get(L"WorkDir");
        task.launch.runAsAdmin = kvMap.get_as<bool>(L"RunAsAdmin",0);
        task.launch.showWnd = kvMap.get_as<int>(L"ShowWnd", SW_NORMAL);

        tasks.push_back(move(task));
    }
    return tasks;
}

// 创建示例配置文件（与原实现一致）
static void CreateSampleIni(const wstring& path) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"w, ccs=UTF-8") == 0 && f) {
        const wchar_t* sample =
            L"[Task1]\n"
            L"Time=09:00:00\n"
            L"Program=notepad.exe\n"
            L"Params=C:\\logs\\daily.txt\n"
            L"WorkDir=C:\\logs\n"
            L"ShowWnd=1\n"
            L"\n"
            L"[Task2]\n"
            L"Time=12:30:00\n"
            L"Program=backup.bat\n"
            L"Params=D:\\data D:\\backup\n"
            L"RunAsAdmin=1\n"
            L"ShowWnd=6\n"
            L"\n"
            L"[Task3]\n"
            L"Time=18:00:00\n"
            L"Program=C:\\Tools\\report.exe\n"
            L"Params=-auto\n"
            L"WorkDir=C:\\Reports\n"
            L"ShowWnd=7\n"
            L"\n"
            L"[Task4]\n"
            L"Time=22:00:00\n"
            L"Program=..\\scripts\\cleanup.cmd\n"
            L"Params=/silent\n"
            L"RunAsAdmin=0\n"
            L"ShowWnd=1\n";
        fwrite(sample, sizeof(wchar_t), wcslen(sample), f);
        fclose(f);
    }
}

// 判断当前时间是否接近任务设定时间（误差≤3秒）
static bool IsTimeToExecute(const TimedTask& task, const SYSTEMTIME& now) {
    int taskTotal = task.hour * 3600 + task.minute * 60 + task.second;
    int nowTotal = now.wHour * 3600 + now.wMinute * 60 + now.wSecond;
    int diff = abs(taskTotal - nowTotal);
    diff = min(diff, 86400 - diff);
    return diff <= 3;
}

// 获取文件最后写入时间
static FILETIME GetFileLastWriteTime(const wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &info))
        return FILETIME{};
    return info.ftLastWriteTime;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
    Console console;
    console.attach();
    console.setLocale();

    EnsureSingleInstance();

    wstring iniPath = lpCmdLine;
    if (iniPath.empty())
        iniPath = GetCurrentProcessDir() + L"tasks.ini";

    if (GetFileAttributesW(iniPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        CreateSampleIni(iniPath);
        wcout << L"未找到配置文件，已创建示例文件：" << iniPath << endl;
        wcout << L"请编辑后重新运行程序。" << endl;
        Sleep(5000);
        return 0;
    }

    auto tasks = ReadTasks(iniPath);
    if (tasks.empty()) {
        wcout << L"未找到有效任务，请检查INI文件：" << iniPath << endl;
        Sleep(5000);
        return 1;
    }

    vector<char> executed(tasks.size(), 0);
    FILETIME lastWrite = GetFileLastWriteTime(iniPath);
    wcout << L"已加载 " << tasks.size() << L" 个任务，监视文件修改中..." << endl;

    while (true) {
        SYSTEMTIME st;
        GetLocalTime(&st);

        // 跨天重置执行标记
        if (st.wHour == 0 && st.wMinute == 0 && st.wSecond < 5)
            fill(executed.begin(), executed.end(), 0);

        // 检查 INI 文件是否被修改
        FILETIME curWrite = GetFileLastWriteTime(iniPath);
        if (CompareFileTime(&lastWrite, &curWrite) != 0) {
            lastWrite = curWrite;
            auto newTasks = ReadTasks(iniPath);
            if (!newTasks.empty()) {
                tasks = move(newTasks);
                executed.assign(tasks.size(), 0);
                wcout << L"重新加载配置，当前任务数：" << tasks.size() << endl;
            }
            else {
                wcout << L"配置文件无效，保留原有任务。" << endl;
            }
        }

        // 执行到时的任务
        for (size_t i = 0; i < tasks.size(); ++i) {
            if (!executed[i] && IsTimeToExecute(tasks[i], st)) {
                if (RunExternalProgram(ResolvePath(tasks[i].launch.program),
                    tasks[i].launch.runAsAdmin ? L"runas" : L"open",
                    tasks[i].launch.params,
                    tasks[i].launch.workDir,
                    tasks[i].launch.showWnd))
                    wcout << L"执行任务 [" << tasks[i].name << L"] 成功" << endl;
                else
                    wcout << L"执行任务 [" << tasks[i].name << L"] 失败" << endl;
                executed[i] = 1;
            }
        }

        Sleep(1000);
    }
    return 0;
}