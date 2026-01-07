#pragma once
#include <string>
#include <taskschd.h>
#include <comdef.h>
#include <atlbase.h>
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

bool InitializeTaskService(CComPtr<ITaskService>& pService);
bool GetRootFolder(CComPtr<ITaskService>& pService, CComPtr<ITaskFolder>& pRootFolder);
bool IsTaskScheduled(const std::wstring& taskName);
void DeleteScheduledTask(CComPtr<ITaskService>& pService, const std::wstring& taskName);
void CreateScheduledTask(CComPtr<ITaskService>& pService, const std::wstring& taskName, const std::wstring& exePath);
void StartupChanging(bool isAdd);