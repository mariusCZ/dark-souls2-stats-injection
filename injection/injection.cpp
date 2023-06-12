#include <Windows.h>
#include <wchar.h>
#include <stdio.h>
#include <tlhelp32.h>
#include "injection.h"

int main()
{
    // Process variables
    DWORD PID;
    HANDLE hProcess;
    // Address for base of the module and for where the damage is stored
    DWORD BaseAddress;
    DWORD DmgAddress;

    // Proccess name
    wchar_t PrName[] = L"DarkSoulsII.exe";
    // Address of HP (technically I think this is the base player structure address)
    DWORD HpAddress = 0x01150414;
    // Offsets for HP
    DWORD HpOffsets[] = { 0x74, 0xB8, 0x08, 0x14, 0x04, 0x04, 0xFC };
    // Offsets for deaths
    DWORD DeathOffsets[] = { 0x74, 0xC, 0x08, 0x08, 0x10, 0x04, 0x1E0 };

    // Multi-level pointer variables for HP and death count
    DWORD finalHP = 0;
    DWORD finalDeath = 0;
    unsigned int resultHP = 0;
    unsigned int resultDMG = 0;
    unsigned int resultDeath = 0;
    unsigned char resAssert = 0;

    // Find PID of requested process
    if (!(PID = get_PID(PrName)))
    {
        printf("Process does not exist\n");
        system("pause");
        return 1;
    }
    printf("Process found!\n");
    printf("PID: %d\n\n", PID);

    // Open requested process and get its handle
    if (!(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID)))
    {
        printf("OpenProcess error\n");
        return 1;
    }

    printf("OpenProcess is ok\n");

    // Get the address for the module base
    if (!(BaseAddress = GetModuleBase(PrName, PID)))
    {
        printf("GetModuleBase error\n");
        return 1;
    }
    printf("GetModuleBase is ok\n");

    // Get the addresses of the multi-level pointer variables
    finalHP = CalcPtr_Ext(hProcess, BaseAddress+HpAddress, HpOffsets, 7);
    finalDeath = CalcPtr_Ext(hProcess, BaseAddress + HpAddress, DeathOffsets, 7);
    
    // check first whether the code is already injected
    ReadProcessMemory(hProcess, (LPCVOID)(BaseAddress + HookAddr), &resAssert, 1, 0);
    if(resAssert != 0x68)
        DmgAddress = InjectRoutine(hProcess, BaseAddress);
    while (1) {
        // Read memory where appropriate variables are stored
        ReadProcessMemory(hProcess, (LPCVOID)finalHP, &resultHP, sizeof(int), 0);
        ReadProcessMemory(hProcess, (LPCVOID)finalDeath, &resultDeath, sizeof(int), 0);
        ReadProcessMemory(hProcess, (LPCVOID)DmgAddress, &resultDMG, sizeof(int), 0);

        printf("Health: %d\n", resultHP);
        printf("Deaths: %d\n", resultDeath);
        printf("Damage: %d\n", resultDMG);
        Sleep(500);
    }

    return 0;
}

// Function to get the PID of requested proccess
DWORD get_PID(wchar_t PrName[])
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            //printf("%s\n", entry.szExeFile);
            if (wcscmp(entry.szExeFile, PrName) == 0)
            {
                return entry.th32ProcessID;
            }
        }
    }
    CloseHandle(snapshot);
    return NULL;
}

// Function to get the address of the base module.
DWORD GetModuleBase(wchar_t lpModuleName[], DWORD dwProcessId)
{
    MODULEENTRY32 lpModuleEntry = { 0 };
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);

    if (!hSnapShot)
        return NULL;
    lpModuleEntry.dwSize = sizeof(lpModuleEntry);
    BOOL bModule = Module32First(hSnapShot, &lpModuleEntry);
    while (bModule)
    {
        if (!wcscmp(lpModuleEntry.szModule, lpModuleName))
        {
            CloseHandle(hSnapShot);
            return (DWORD)lpModuleEntry.modBaseAddr;
        }
        bModule = Module32Next(hSnapShot, &lpModuleEntry);
    }
    CloseHandle(hSnapShot);
    return NULL;
}

// Routine to inject shellcode
DWORD InjectRoutine(HANDLE hProc, DWORD base)
{
    // This is shellcode that replaces a bit of code in base module for rerouting (pop and ret for absolute jump)
    char HookShellcode[8] = { 0 };
    // This is the code that will be executed after rerouting.
    char InjectShellcode[128] = { 0 };
    // Allocate memory for injected code
    DWORD pInjectedFunction = (DWORD)VirtualAllocEx(hProc, NULL, 128, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    // Allocate memory for a variable (damage)
    DWORD pVar = (DWORD)VirtualAllocEx(hProc, NULL, 4, MEM_COMMIT, PAGE_READWRITE);

    // This is code that will be injected, but this is not full code, since some code has to be generated depending on resulting addresses from allocation
    const unsigned char iCode[] = {0x89, 0x0B, 0x5B, 0x89, 0x46, 0x14, 0xF3, 0x0F, 0x11, 0x46, 0x2C};

    // This is a pop code
    memcpy(HookShellcode, "\x68", 1);
    // Copy address to where you want the jump to be. Memcpy used so as to make little endian
    memcpy(HookShellcode+1, &pInjectedFunction, sizeof(DWORD));
    // A ret and a two byte nop to keep the same size of the overwritten instructions
    memcpy(HookShellcode+5, "\xc3\x66\x90", 3);

    // This generates a mov with appropriate variable address and a pop and return back to base module.
    memcpy(InjectShellcode, "\x53\xbb", 2);
    memcpy(InjectShellcode+2, &pVar, sizeof(DWORD));
    memcpy(InjectShellcode+6, &iCode, sizeof(iCode));
    memcpy(InjectShellcode+6+sizeof(iCode), "\x68", 1);
    DWORD retAddr = base + 0x126390;
    memcpy(InjectShellcode+6+sizeof(iCode)+1, &retAddr, sizeof(DWORD));
    memcpy(InjectShellcode+6+sizeof(iCode)+5, "\xc3\x66\x90", 3);

    // Inject the code
    WriteProcessMemory(hProc, (LPVOID)(pInjectedFunction), InjectShellcode, sizeof(InjectShellcode), 0);
    WriteProcessMemory(hProc, (LPVOID)(base + HookAddr), HookShellcode, sizeof(HookShellcode), 0);
    // Return the address of damage variable.
    return pVar;
}

// Function that obtains the address of a multi-level pointer
DWORD CalcPtr_Ext(HANDLE hProc, DWORD base, DWORD offs[], int lvl)
{
    DWORD product = base;
    for (int i = 0; i < lvl; ++i)
    {
        ReadProcessMemory(hProc, (LPCVOID)product, &product, sizeof(DWORD), 0);
        product += offs[i];
    }
    return product;
}