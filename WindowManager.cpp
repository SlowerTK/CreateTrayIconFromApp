#include "WindowManager.hpp"
#include "Constants.hpp"
#include "ProcessManager.hpp"
#include "UIManager.hpp"
#include "Logger.hpp"
#include <Wbemidl.h>
#include <comdef.h>
#include <algorithm>
#pragma comment(lib, "wbemuuid.lib")

static HICON GetIcon(HWND hwnd) {
    HICON hIcon = reinterpret_cast<HICON>(SendMessageW(hwnd, WM_GETICON, ICON_SMALL, 0));
    if (!hIcon) {
        hIcon = reinterpret_cast<HICON>(GetClassLongPtrW(hwnd, GCLP_HICONSM));
        if (!hIcon) {
            hIcon = LoadIconW(NULL, IDI_APPLICATION);
        }
    }
    return hIcon;
}
static std::wstring GetProcessCommandLine(DWORD processID) {
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        LogAdd(ET_COM_INIT);
        return{};
    }
    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                                RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres)) {
        CoUninitialize();
        LogAdd(ET_COM_SECURITY);
        return{};
    }

    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<LPVOID*>(&pLoc));
    if (FAILED(hres)) {
        CoUninitialize();
        LogAdd(ET_WMI_CREATE);
        return{};
    }

    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        LogAdd(ET_WMI_CONNECT);
        return{};
    }

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, 0, 0, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_NONE);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        LogAdd(ET_WMI_SECURITY);
        return{};
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    std::wstring query = L"SELECT CommandLine FROM Win32_Process WHERE ProcessId = " + std::to_wstring(processID);

    hres = pSvc->ExecQuery(_bstr_t("WQL"), _bstr_t(query.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        LogAdd(ET_WMI_QUERY);
        return{};
    }

    IWbemClassObject* pClsObj = NULL;
    ULONG uReturn = 0;
    VARIANT vtProp{};

    std::wstring commandLine = {};
    if (pEnumerator->Next(WBEM_INFINITE, 1, &pClsObj, &uReturn) == S_OK) {
        hres = pClsObj->Get(SY_COMMAND_LINE, 0, &vtProp, 0, 0);
        if (SUCCEEDED(hres) && vtProp.vt == VT_BSTR) {
            commandLine = vtProp.bstrVal;
        }
        else {
            LogAdd(ET_GET_CMDLINE);
            commandLine = {};
        }
        VariantClear(&vtProp);
        pClsObj->Release();
    }
    else {
        commandLine = {};
    }

    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    return commandLine;
}
void CollapseToTray(HWND hwnd, HiddenWindow* HW) {
    if (HW == nullptr) {
        DWORD processID = GetProcessId(hwnd);
        std::wstring processName = GetAllowedProcessName(processID);
        if (processName.empty())
            return;
        std::wstring className = GetWstringClassName(hwnd);
        if (className.empty())
            return;
        hiddenWindows.push_back({ hwnd, GetIcon(hwnd), processID, FavoriteState::None, GetWindowTitle(hwnd), className, processName, GetProcessCommandLine(processID) });
    }
    else {
        HW->hIcon = GetIcon(hwnd);
        hiddenWindows.push_back(*HW);
    }
    ShowWindow(hwnd, SW_HIDE);
    UpdateTrayMenu(false);
}
void CollapseToTrayFromFavorite() {
    int count = static_cast<int>(SendMessageW(pv.hFavoritesList, LB_GETCOUNT, 0, 0));
    if (count == LB_ERR) return;

    for (int i = 0; i < count; ++i) {
        HiddenWindow* HW = reinterpret_cast<HiddenWindow*>(SendMessageW(pv.hFavoritesList, LB_GETITEMDATA, i, 0));

        if (HW->isFavorite == FavoriteState::Active && std::none_of(hiddenWindows.begin(), hiddenWindows.end(), [HW](const HiddenWindow& hw) { return hw.hwnd == HW->hwnd; })) {
            std::wstring commandLine = GetProcessCommandLine(HW->processID);
            if (!commandLine.empty()) {
                HW->commandLine = std::move(commandLine);
                if (std::none_of(favoriteWindows.begin(), favoriteWindows.end(), [HW](const HiddenWindow& hw) { return hw.commandLine == HW->commandLine && hw.hwnd == HW->hwnd; })) {
                    favoriteWindows.push_back(*HW);
                    std::wstring appTittle = HW->windowTitle.c_str();
                    LogAdd(IT_FAVORITE_ADDED + appTittle);
                }
                CollapseToTray(HW->hwnd, HW);
            }
        }
    }
}
std::wstring GetWstringClassName(HWND hwnd) {
    wchar_t classNam[256];
    ZeroMemory(&classNam, sizeof(classNam));
    GetClassNameW(hwnd, classNam, SIZEOF(classNam));
    for (const auto& t : exceptionClassNames) {
        if (lstrcmpW(classNam, t.c_str()) == 0)
            return{};
    }
    return classNam;
}
std::wstring GetWindowTitle(HWND hwnd) {
    wchar_t windowTitle[MAX_PATH];
    ZeroMemory(&windowTitle, sizeof(windowTitle));
    GetWindowTextW(hwnd, windowTitle, SIZEOF(windowTitle));
    return windowTitle;
}
void SerchWindow(HiddenWindow& window) {
    HWND hwnd = FindWindow(window.className.c_str(), window.windowTitle.c_str());
    DWORD processId = GetProcessId(hwnd);
    std::wstring processName = GetAllowedProcessName(processId);
    if (processName == window.processName) {
        std::wstring commandLine = GetProcessCommandLine(processId);
        if (commandLine == window.commandLine) {
            window.hwnd = hwnd;
            window.processID = processId;
        }
    }
}
void FindWindowFromFile(HiddenWindow& windowToFind) {
    if (windowToFind.isFavorite == FavoriteState::SavedFavorite) {
        SerchWindow(windowToFind);
        if (windowToFind.hwnd) {
            windowToFind.isFavorite = FavoriteState::Active;
            CollapseToTray(windowToFind.hwnd, &windowToFind);
        }
    }
}