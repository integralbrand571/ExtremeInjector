#include "ProcessScanner.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>

struct ProcessEntry {
    DWORD  pid;
    std::wstring name;
    std::wstring arch; // "x64" or "x86"
};

std::vector<ProcessEntry> ScanProcesses()
{
    std::vector<ProcessEntry> result;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return result;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(snap, &pe)) {
        do {
            HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
            if (hProc) {
                BOOL isWow64 = FALSE;
                IsWow64Process(hProc, &isWow64);
                result.push_back({ pe.th32ProcessID, pe.szExeFile, isWow64 ? L"x86" : L"x64" });
                CloseHandle(hProc);
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return result;
}
