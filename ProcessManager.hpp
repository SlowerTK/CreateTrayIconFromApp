#pragma once
#include "types.hpp"
#include <Psapi.h>
#pragma comment(lib, "psapi.lib")

DWORD GetProcessId(HWND hwnd);
std::wstring GetAllowedProcessName(DWORD processID);