#include "ManualMap.h"
#include <Windows.h>
#include <fstream>
#include <vector>

// Manual map: load DLL into target process without calling LoadLibrary.
// Copies PE sections manually and relocates the image.

bool ManualMapInject(HANDLE hProcess, const std::wstring& dllPath)
{
    std::ifstream file(dllPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    auto size = file.tellg();
    file.seekg(0);
    std::vector<BYTE> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    file.close();

    auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(buffer.data());
    auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(buffer.data() + dosHeader->e_lfanew);

    LPVOID target = VirtualAllocEx(hProcess, nullptr,
        ntHeaders->OptionalHeader.SizeOfImage,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!target) return false;

    // Write headers
    WriteProcessMemory(hProcess, target, buffer.data(),
        ntHeaders->OptionalHeader.SizeOfHeaders, nullptr);

    // Write sections
    auto* section = IMAGE_FIRST_SECTION(ntHeaders);
    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i, ++section) {
        if (!section->SizeOfRawData) continue;
        WriteProcessMemory(hProcess,
            (LPBYTE)target + section->VirtualAddress,
            buffer.data() + section->PointerToRawData,
            section->SizeOfRawData, nullptr);
    }

    return true; // caller must fix relocations & imports, then create remote thread
}
