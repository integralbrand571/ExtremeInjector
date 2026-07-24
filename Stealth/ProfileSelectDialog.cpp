// PEHelper.cpp — parse PE headers of a DLL on disk before injection.
// Validates the file, detects architecture, and reads export table.

#include "PEHelper.h"
#include <fstream>
#include <vector>

PEInfo ParsePE(const std::wstring& path)
{
    PEInfo info{};
    std::ifstream f(path, std::ios::binary);
    if (!f) return info;

    IMAGE_DOS_HEADER dos{};
    f.read(reinterpret_cast<char*>(&dos), sizeof(dos));
    if (dos.e_magic != IMAGE_DOS_SIGNATURE) return info;

    f.seekg(dos.e_lfanew);
    DWORD sig{};
    f.read(reinterpret_cast<char*>(&sig), 4);
    if (sig != IMAGE_NT_SIGNATURE) return info;

    IMAGE_FILE_HEADER fh{};
    f.read(reinterpret_cast<char*>(&fh), sizeof(fh));
    info.is64bit = (fh.Machine == IMAGE_FILE_MACHINE_AMD64);
    info.valid   = true;

    // Read optional header magic to double-check
    WORD optMagic{};
    f.read(reinterpret_cast<char*>(&optMagic), sizeof(optMagic));
    info.is64bit = (optMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);

    return info;
}

std::vector<std::string> GetExports(const std::wstring& path)
{
    std::vector<std::string> names;
    // Simplified: map file into memory and walk export directory
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return names;

    HANDLE hMap = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!hMap) { CloseHandle(hFile); return names; }

    auto* base = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (base) {
        auto* dos = (IMAGE_DOS_HEADER*)base;
        auto* nt  = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
        DWORD expRva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        if (expRva) {
            // RVA to file offset conversion omitted for brevity
        }
        UnmapViewOfFile(base);
    }
    CloseHandle(hMap);
    CloseHandle(hFile);
    return names;
}
