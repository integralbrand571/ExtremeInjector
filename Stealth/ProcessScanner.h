#pragma once
#include <Windows.h>
#include <vector>
#include <string>

struct ProcessEntry {
    DWORD pid;
    std::wstring name;
    std::wstring arch;
};

std::vector<ProcessEntry> ScanProcesses();
