#pragma once
#include "Types.hpp"

void CheckFolderAndFile(const std::wstring& fileName);
std::wstring ReadSettingsFile();
std::wstring serializeToWstring(const HiddenWindow& hw);
void deserializeFromWstring(const std::wstring& str);
void WriteMyFile(std::wstring fileName, const std::wstring& content, bool isAdd = false);