#include "InjectionCore.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <string>

bool InjectDLL(DWORD pid, const std::wstring& dllPath)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) return false;

    SIZE_T pathSize = (dllPath.size() + 1) * sizeof(wchar_t);
    LPVOID remoteMem = VirtualAllocEx(hProcess, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) { CloseHandle(hProcess); return false; }

    WriteProcessMemory(hProcess, remoteMem, dllPath.c_str(), pathSize, nullptr);

    HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
    FARPROC pLoadLibrary = GetProcAddress(hKernel, "LoadLibraryW");

    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        (LPTHREAD_START_ROUTINE)pLoadLibrary, remoteMem, 0, nullptr);

    if (hThread) {
        WaitForSingleObject(hThread, 5000);
        CloseHandle(hThread);
    }

    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return hThread != nullptr;
}
