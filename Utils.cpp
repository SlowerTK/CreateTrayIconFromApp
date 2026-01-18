#include "Utils.hpp"
#include "AppConstants.hpp"
#include "Types.hpp"

LPCWSTR RegisterNewClass(LPCWSTR className, WNDPROC wndproc) {
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpszClassName = className;
    wc.hInstance = pv.hInstance;
    wc.lpfnWndProc = wndproc;
    wc.hbrBackground = nullptr;
    wc.hCursor = reinterpret_cast<HCURSOR>(LoadImageW(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    RegisterClassW(&wc);
    return wc.lpszClassName;
}
bool isRunAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, NULL, NULL, NULL, NULL, NULL, NULL, &adminGroup)) {
        if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
            isAdmin = FALSE;
        }
        FreeSid(adminGroup);
    }
    return isAdmin;
}
void DebugModCheck(wchar_t* lpCmdLine) {
    pv.isDebugMode = false;
    int argsCount;
    wchar_t** cmdArpvLine;
    if (cmdArpvLine = CommandLineToArgvW(lpCmdLine, &argsCount))
        for (int i = 0; i != argsCount; i++)
            if (!lstrcmpW(cmdArpvLine[i], SY_DEBUG_ARG)) {
                pv.isDebugMode = true;
                break;
            }
}
HBITMAP IconToBitmap(HICON hIcon) {
    const int width = 16;
    const int height = 16;
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hdcMem, hBitmap));
    HBRUSH hBrush = pv.isDark ? CreateSolidBrush(ID_CLR_TRAYGRAY) : CreateSolidBrush(ID_CLR_TRAYLIGHT);
    RECT rect = { 0, 0, width, height };
    FillRect(hdcMem, &rect, hBrush);
    DeleteObject(hBrush);
    DrawIconEx(hdcMem, 0, 0, hIcon, width, height, 0, 0, DI_NORMAL);
    SelectObject(hdcMem, hOldBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return hBitmap;
}
void RestartWithAdminRights() {
    wchar_t szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, MAX_PATH);
    SHELLEXECUTEINFO sei = { 0 };
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = NULL;
    sei.lpVerb = L"runas";
    sei.lpFile = szPath;
    sei.nShow = SW_SHOWNORMAL;
    ShellExecuteExW(&sei);
}
