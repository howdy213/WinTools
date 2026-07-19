#ifndef SERVICEUTILS_H
#define SERVICEUTILS_H

#include <windows.h>
#include <string>

void WINAPI ServiceMain(DWORD argc, LPWSTR* argv);
void WINAPI ServiceCtrlHandler(DWORD ctrlCode);
std::wstring GetServiceBinaryPath(const std::wstring& serviceName);
#endif