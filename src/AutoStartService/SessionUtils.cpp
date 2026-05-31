#include <WinUtils/WinPch.h>

#include "SessionUtils.h"
#include <windows.h>
#include <userenv.h>
#include <wtsapi32.h>
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "wtsapi32.lib")

bool GetActiveSessionId(DWORD& sessionId) {
    sessionId = 0;
    DWORD count = 0;
    PWTS_SESSION_INFO pInfo = nullptr;
    if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pInfo, &count))
        return false;
    UniqueWtsMemory mem(pInfo);
    for (DWORD i = 0; i < count; ++i) {
        if (pInfo[i].State == WTSActive) {
            sessionId = pInfo[i].SessionId;
            return true;
        }
    }
    return false;
}

bool GetUserTokenFromSessionId(DWORD sessionId, UniqueHandle& token) {
    HANDLE raw = nullptr;
    if (!WTSQueryUserToken(sessionId, &raw)) return false;
    token.reset(raw);
    return true;
}

DWORD StartProcessInUserSession(const std::wstring& appPath, const std::wstring& args,
    const std::wstring& workingDir, int showWnd) {
    // 参数校验
    if (appPath.empty()) {
        OutputDebugStringW(L"StartProcessInUserSession: appPath is empty\n");
        return 0;
    }

    // 获取活动会话 ID
    DWORD sessionId = 0;
    if (!GetActiveSessionId(sessionId)) {
        OutputDebugStringW(L"GetActiveSessionId failed\n");
        return 0;
    }

    // 获取该会话的用户令牌（已是主令牌）
    UniqueHandle userToken;
    if (!GetUserTokenFromSessionId(sessionId, userToken)) {
        OutputDebugStringW(L"GetUserTokenFromSessionId failed\n");
        return 0;
    }

    // 构建命令行
    std::wstring cmdLine;
    if (appPath.find(L' ') != std::wstring::npos)
        cmdLine = L"\"" + appPath + L"\"";
    else
        cmdLine = appPath;
    if (!args.empty())
        cmdLine += L" " + args;

    // 启动信息：指定交互式桌面，显示窗口风格
    STARTUPINFOW si = { sizeof(si) };
    si.lpDesktop = const_cast<LPWSTR>(L"winsta0\\default");
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = static_cast<WORD>(showWnd);

    // 确定工作目录
    std::wstring effectiveWorkDir = workingDir;
    if (effectiveWorkDir.empty()) {
        size_t pos = appPath.find_last_of(L"\\/");
        if (pos != std::wstring::npos)
            effectiveWorkDir = appPath.substr(0, pos + 1);
    }

    // 创建用户环境块
    LPVOID envBlock = nullptr;
    if (!CreateEnvironmentBlock(&envBlock, userToken.get(), FALSE)) {
        OutputDebugStringW(L"CreateEnvironmentBlock failed, using service environment\n");
        // envBlock 保持 nullptr，进程将继承服务环境
    }

    PROCESS_INFORMATION pi = {};
    DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS;
    if (envBlock) {
        dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
    }

    BOOL success = CreateProcessAsUserW(
        userToken.get(),
        nullptr,
        &cmdLine[0],
        nullptr, nullptr,
        FALSE,
        dwCreationFlags,
        envBlock,
        effectiveWorkDir.empty() ? nullptr : effectiveWorkDir.c_str(),
        &si,
        &pi
    );

    if (envBlock) {
        DestroyEnvironmentBlock(envBlock);
    }

    if (success) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        OutputDebugStringW((L"Process started successfully, PID=" + std::to_wstring(pi.dwProcessId) + L"\n").c_str());
        return pi.dwProcessId;
    }
    else {
        DWORD err = GetLastError();
        OutputDebugStringW((L"CreateProcessAsUser failed, error=" + std::to_wstring(err) + L"\n").c_str());
        return 0;
    }
}