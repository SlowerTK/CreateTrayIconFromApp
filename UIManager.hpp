#pragma once
#include "Types.hpp"

void OpenSettings();
void UpdateTrayMenu(bool isDebug);
void UpdateApplicationsList();
void UpdateFavoriteList();
void DeleteList(HWND list);
int GetMaxTextWidth(HWND hwndListBox);
void AddTrayIcon(HWND hwnd);
void RemoveTrayIcon(HWND hwnd);
void CreateHKSettWnd();
std::wstring TruncateWithEllipsis(const std::wstring& text, size_t maxLen = 35);
void CloseApp();