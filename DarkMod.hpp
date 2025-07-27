#pragma once
#include <Windows.h>

extern int(__cdecl* AllowDarkModeForWindow)(HWND, int);
extern int(__cdecl* SetPreferredAppMode)(int);
extern int(__cdecl* FlushMenuThemes)(void);
extern int(__cdecl* RefreshImmersiveColorPolicyState)(void);

void InitDarkMode();
void EnableDarkForWindow(HWND hwnd, bool enable);
void ApplyThemeToControls(HWND hwnd, bool dark);
bool IsSystemInDarkMode();
void RefreshDarkMenuTheme();