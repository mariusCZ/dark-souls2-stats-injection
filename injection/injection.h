#pragma once

#include <Windows.h>
#include <wchar.h>

DWORD get_PID(wchar_t PrName[]);
DWORD GetModuleBase(wchar_t lpModuleName[], DWORD dwProcessId);
DWORD InjectRoutine(HANDLE hProc, DWORD base);
DWORD CalcPtr_Ext(HANDLE hProc, DWORD base, DWORD offs[], int lvl);

// Address for where the code will be replaced in the base module
DWORD HookAddr = 0x126388;