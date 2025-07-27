#pragma once
#include "DarkMod.hpp"
#include <uxtheme.h>
#include <dwmapi.h>
#include <shlwapi.h>

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shlwapi.lib")

int(__cdecl* AllowDarkModeForWindow)(HWND, int) = nullptr;
int(__cdecl* SetPreferredAppMode)(int) = nullptr;
int(__cdecl* FlushMenuThemes)(void) = nullptr;
int(__cdecl* RefreshImmersiveColorPolicyState)(void) = nullptr;

void InitDarkMode() {
    HMODULE hUx = LoadLibraryW(L"uxtheme.dll");
    if (hUx) {
        SetPreferredAppMode = (int(__cdecl*)(int))GetProcAddress(hUx, MAKEINTRESOURCEA(135));
        AllowDarkModeForWindow = (int(__cdecl*)(HWND, int))GetProcAddress(hUx, MAKEINTRESOURCEA(133));
        FlushMenuThemes = (int(__cdecl*)(void))GetProcAddress(hUx, MAKEINTRESOURCEA(136));
        RefreshImmersiveColorPolicyState = (int(__cdecl*)(void))GetProcAddress(hUx, MAKEINTRESOURCEA(104));

        if (SetPreferredAppMode)
            SetPreferredAppMode(0); // 0: Default, will follow system
        if (RefreshImmersiveColorPolicyState)
            RefreshImmersiveColorPolicyState();
    }
}

bool IsSystemInDarkMode() {
    DWORD value = 0;
    DWORD size = sizeof(value);
    if (SHGetValueW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", NULL, &value, &size) == ERROR_SUCCESS)
    {
        return value == 0;
    }
    return false; // fallback to light
}

void EnableDarkForWindow(HWND hwnd, bool enable) {
    if (AllowDarkModeForWindow)
        AllowDarkModeForWindow(hwnd, enable ? TRUE : FALSE);

    BOOL dark = enable ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));
}

void ApplyThemeToControls(HWND hwnd, bool dark) {
    HWND hChild = GetWindow(hwnd, GW_CHILD);
    while (hChild) {
        SetWindowTheme(hChild, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }
}
void RefreshDarkMenuTheme() {
    if (SetPreferredAppMode)
        SetPreferredAppMode(1); // AllowDark
    if (RefreshImmersiveColorPolicyState)
        RefreshImmersiveColorPolicyState();
    if (FlushMenuThemes)
        FlushMenuThemes();
}
