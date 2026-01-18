#include "StartupManager.hpp"
#include "AppConstants.hpp"
#include "Logger.hpp"

bool InitializeTaskService(CComPtr<ITaskService>& pService) {
    HRESULT hr = pService.CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER);
    if (FAILED(hr)) {
        wchar_t buf[64];
        swprintf_s(buf, ET_TASKSERVICE_CREATE, static_cast<UINT>(hr));
        LogAdd(buf);
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        wchar_t buf[64];
        swprintf_s(buf, ET_TASKSERVICE_CONNECT, static_cast<UINT>(hr));
        LogAdd(buf);
        return false;
    }
    return true;
}
bool GetRootFolder(CComPtr<ITaskService>& pService, CComPtr<ITaskFolder>& pRootFolder) {
    HRESULT hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) {
        wchar_t buf[64];
        swprintf_s(buf, ET_TASK_ROOT_ACCESS, static_cast<UINT>(hr));
        LogAdd(buf);
        return false;
    }
    return true;
}
bool IsTaskScheduled(const std::wstring& taskName) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        wchar_t buf[64];
        swprintf_s(buf, ET_COINIT_EX, static_cast<UINT>(hr));
        LogAdd(buf);
        return false;
    }

    CComPtr<ITaskService> pService;
    if (!InitializeTaskService(pService)) {
        LogAdd(ET_INITIALIZE_TASK);
        CoUninitialize();
        return false;
    }

    CComPtr<ITaskFolder> pRootFolder;
    if (!GetRootFolder(pService, pRootFolder)) {
        LogAdd(ET_GET_ROOT_FOLDER);
        CoUninitialize();
        return false;
    }

    if (!pRootFolder) {
        LogAdd(ET_ROOTFOLDER_NULL);
        CoUninitialize();
        return false;
    }

    CComPtr<IRegisteredTask> pTask = nullptr;
    hr = pRootFolder->GetTask(_bstr_t(taskName.c_str()), &pTask);

    bool result = SUCCEEDED(hr) && pTask != nullptr;
    pTask.Release();
    pRootFolder.Release();
    pService.Release();
    CoUninitialize();
    return result;
}
void DeleteScheduledTask(CComPtr<ITaskService>& pService, const std::wstring& taskName) {
    CComPtr<ITaskFolder> pRootFolder;
    if (!GetRootFolder(pService, pRootFolder)) {
        return;
    }
    HRESULT hr = pRootFolder->DeleteTask(_bstr_t(taskName.c_str()), 0);
    if (FAILED(hr)) {
        wchar_t buf[64];
        swprintf_s(buf, ET_TASK_DELETE, static_cast<UINT>(hr));
        LogAdd(buf);
    }
    else {
        LogAdd(IT_TASK_DELETED);
    }
}
void CreateScheduledTask(CComPtr<ITaskService>& pService, const std::wstring& taskName, const std::wstring& exePath) {
    CComPtr<ITaskFolder> pRootFolder;
    if (!GetRootFolder(pService, pRootFolder)) {
        return;
    }
    pRootFolder->DeleteTask(_bstr_t(taskName.c_str()), 0);

    CComPtr<ITaskDefinition> pTask;
    HRESULT hr = pService->NewTask(0, &pTask);
    if (FAILED(hr)) {
        wchar_t buf[64];
        swprintf_s(buf, ET_TASK_CREATE, static_cast<UINT>(hr));
        LogAdd(buf);
        return;
    }

    CComPtr<IRegistrationInfo> pRegInfo;
    pTask->get_RegistrationInfo(&pRegInfo);
    pRegInfo->put_Author(const_cast<BSTR>(L"Admin"));

    CComPtr<IPrincipal> pPrincipal;
    pTask->get_Principal(&pPrincipal);
    pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
    pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);

    CComPtr<ITriggerCollection> pTriggerCollection;
    pTask->get_Triggers(&pTriggerCollection);
    CComPtr<ITrigger> pTrigger;
    pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);

    CComPtr<IActionCollection> pActionCollection;
    pTask->get_Actions(&pActionCollection);
    CComPtr<IAction> pAction;
    pActionCollection->Create(TASK_ACTION_EXEC, &pAction);

    CComPtr<IExecAction> pExecAction;
    pAction->QueryInterface(IID_IExecAction, reinterpret_cast<void**>(&pExecAction));
    pExecAction->put_Path(_bstr_t(exePath.c_str()));

    CComPtr<ITaskSettings> pSettings;
    pTask->get_Settings(&pSettings);
    pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
    pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
    pSettings->put_AllowHardTerminate(VARIANT_FALSE);
    pSettings->put_ExecutionTimeLimit(_bstr_t(L"PT0S"));

    CComPtr<IRegisteredTask> pRegisteredTask;
    hr = pRootFolder->RegisterTaskDefinition(
        _bstr_t(taskName.c_str()), pTask, TASK_CREATE_OR_UPDATE,
        _variant_t(L""), _variant_t(L""), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(L""), &pRegisteredTask
    );

    if (FAILED(hr)) {
        wchar_t buf[64];
        swprintf_s(buf, ET_TASK_REGISTER, static_cast<UINT>(hr));
        LogAdd(buf);
    }
    else {
        LogAdd(IT_TASK_REGISTERED);
    }
}
void StartupChanging(bool isAdd) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        wchar_t buf[64];
        swprintf_s(buf, ET_COINIT_EX, static_cast<UINT>(hr));
        LogAdd(buf);
        return;
    }

    CComPtr<ITaskService> pService;
    if (!InitializeTaskService(pService)) {
        CoUninitialize();
        return;
    }

    wchar_t szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);
    std::wstring appPath = szPath;
    if (isAdd) {
        CreateScheduledTask(pService, SY_APP_NAME, appPath);
    }
    else {
        DeleteScheduledTask(pService, SY_APP_NAME);
    }

    pService.Release();
    CoUninitialize();
}