#include "ProcessManager.hpp"
#include "Constants.hpp"

DWORD GetProcessId(HWND hwnd) {
    DWORD processID;
    GetWindowThreadProcessId(hwnd, &processID);
    return processID;
}
std::wstring GetAllowedProcessName(DWORD processID) {
    std::wstring processName = {};
    wchar_t processNam[MAX_PATH] = { 0 };
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
    if (hProcess) {
        HMODULE hMod;
        DWORD cbNeeded;
        if (K32EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
            K32GetModuleBaseNameW(hProcess, hMod, processNam, SIZEOF(processNam));
        processName = processNam;
        CloseHandle(hProcess);
    }
    for (const auto& t : exceptionProcessNames) {
        if (t == processName)
            return{};
    }
    return processName;
}