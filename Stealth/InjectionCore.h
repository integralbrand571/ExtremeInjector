#pragma once
#include <Windows.h>
#include <string>

bool InjectDLL(DWORD pid, const std::wstring& dllPath);
