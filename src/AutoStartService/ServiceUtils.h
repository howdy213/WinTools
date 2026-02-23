#ifndef SERVICEUTILS_H
#define SERVICEUTILS_H

#include <windows.h>

void WINAPI ServiceMain(DWORD argc, LPWSTR* argv);
void WINAPI ServiceCtrlHandler(DWORD ctrlCode);

#endif