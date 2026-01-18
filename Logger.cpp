#include "Logger.hpp"
#include <sstream>
#include <ctime>
#include <iomanip>
#include <ShlObj.h>
#include "AppConstants.hpp"
#include "FileManager.hpp"

std::wstring GetCurrentDate() {
    std::wstringstream wss;
    std::time_t t = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &t);
    wss << (tm.tm_year + 1900) << L'-'
        << (tm.tm_mon + 1) << L'-'
        << tm.tm_mday;
    return wss.str();
}
#ifdef DEBUG
void LogAdd(std::wstring&& content) {
    OutputDebugStringW((content + L"\n").c_str());
}
#else
static std::wstring GetTime() {
    std::wstringstream wss;
    std::time_t t = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &t);
    wss << std::setw(2) << std::setfill(L'0') << tm.tm_hour << L':'
        << std::setw(2) << std::setfill(L'0') << tm.tm_min << L':'
        << std::setw(2) << std::setfill(L'0') << tm.tm_sec;
    return wss.str();
}
void LogAdd(std::wstring&& content) {
    std::wstring currentLogFile = L"\\" + GetCurrentDate() + SY_FILE_EXTENSION;
    CheckFolderAndFile(currentLogFile);
    WriteMyFile(currentLogFile, L"[" + GetTime() + L"]\t" + content + L"\n", true);
}
#endif // DEBUG
void CheckAndDeleteOldLogs(const std::wstring& currentLogFile) {
    PWSTR appDataPath;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath))) {
        MBATTENTION(ET_GET_APPDATA);
        return;
    }
    std::wstring folderPath = std::wstring(appDataPath) + SY_FOLDER_NAME;
    std::vector<std::pair<std::wstring, FILETIME>> logFiles;
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFileW((folderPath + L"\\*" + SY_FILE_EXTENSION).c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && findFileData.cFileName != currentLogFile)
            logFiles.push_back({ findFileData.cFileName, findFileData.ftLastWriteTime });
    } while (FindNextFileW(hFind, &findFileData) != 0);
    FindClose(hFind);
    if (logFiles.size() > 7) {
        auto oldestFile = std::min_element(logFiles.begin(), logFiles.end(), [](const auto& a, const auto& b) {return CompareFileTime(&a.second, &b.second) < 0; });
        if (oldestFile != logFiles.end())
            DeleteFileW((folderPath + L"\\" + oldestFile->first).c_str());
    }
}