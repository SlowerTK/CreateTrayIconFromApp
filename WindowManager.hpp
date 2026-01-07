#pragma once
#include "types.hpp"

void CollapseToTray(HWND hwnd, HiddenWindow* HW = nullptr);
void CollapseToTrayFromFavorite();
std::wstring GetWstringClassName(HWND hwnd);
std::wstring GetWindowTitle(HWND hwnd);
void SerchWindow(HiddenWindow& window);
void FindWindowFromFile(HiddenWindow& windowToFind, bool isFile);