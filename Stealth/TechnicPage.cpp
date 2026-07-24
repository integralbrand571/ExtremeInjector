// StealthInjector.cpp — thread-hijacking injection: suspends a thread in target,
// redirects it to shellcode that calls LoadLibraryW, then resumes.

#include "StealthInjector.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>

static DWORD GetFirstThreadId(DWORD pid)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    THREADENTRY32 te{ sizeof(te) };
    DWORD tid = 0;
    if (Thread32First(snap, &te)) {
        do {
            if (te.th32OwnerProcessID == pid) { tid = te.th32ThreadID; break; }
        } while (Thread32Next(snap, &te));
    }
    CloseHandle(snap);
    return tid;
}

bool StealthInject(DWORD pid, const std::wstring& dllPath)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) return false;

    SIZE_T pathBytes = (dllPath.size() + 1) * sizeof(wchar_t);
    LPVOID remotePath = VirtualAllocEx(hProcess, nullptr, pathBytes,
                                       MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remotePath) { CloseHandle(hProcess); return false; }
    WriteProcessMemory(hProcess, remotePath, dllPath.c_str(), pathBytes, nullptr);

    DWORD tid = GetFirstThreadId(pid);
    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
    if (!hThread) { VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE); CloseHandle(hProcess); return false; }

    SuspendThread(hThread);
    CONTEXT ctx{ CONTEXT_FULL };
    GetThreadContext(hThread, &ctx);

    // Redirect RIP to LoadLibraryW with our path arg in RCX
#ifdef _WIN64
    ctx.Rcx = (DWORD64)remotePath;
    ctx.Rip = (DWORD64)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
#else
    ctx.Eip = (DWORD)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
#endif
    SetThreadContext(hThread, &ctx);
    ResumeThread(hThread);

    WaitForSingleObject(hThread, 3000);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return true;
}
