#pragma once
#include <string>

std::wstring GetCurrentDate();
void LogAdd(std::wstring&& content);
void CheckAndDeleteOldLogs(const std::wstring& currentLogFile);