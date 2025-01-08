//{{NO_DEPENDENCIES}}
// Включаемый файл, созданный в Microsoft Visual C++.
// Используется CreateTrayIconFromApp.rc
//
#define IDI_ICON1                       101

//
//
//
//
//
//File strings
constexpr auto FILEPATHNAME = L"\\settings";
constexpr auto FOLDERNAME = L"\\CTIFA";
//Warning texts
constexpr auto WT_ADMIN = L"Для корректной работы программы рекомендуется запускать её с правами Администратора";
//Error texts
constexpr auto ET_HOTKEY = L"Не удалось зарегистрировать горячие клавиши";
constexpr auto ET_APPDATA = L"Не удалось определить путь к AppData";
constexpr auto ET_CREATEFOLDER = L"Не удалось создать папку";
constexpr auto ET_CREATEFILE = L"Не удалось созадть файл";
constexpr auto ET_FILEOPEN = L"Не удалось открыть файл настроек.";
constexpr auto ET_FILESIZE = L"Не удалось определить размер файла.";
constexpr auto ET_FILECONTENT = L"Не удалось считать файл.";
constexpr auto ET_FILEWRITE = L"Не удалось добавить запись в файл.";
constexpr auto ET_COMMANDLINE = L"Не удалось получить строку запуска.";
constexpr auto ET_COMMANDLINE_PROC = L"Не удалось найти процесс.";
constexpr auto ET_ARGLOST = L"Аргументы отсутствуют.";
//Some texts
constexpr auto SZ_TIP = L"Скрытые окна";
constexpr auto WND_NAME_TEXT = L"Все приложения:";
constexpr auto WND_NAME_TEXT2 = L"Будут скрываться автоматически:";
constexpr auto ADDBUTTON_TEXT = L"▶";
constexpr auto REMOVEBUTTON_TEXT = L"◀";
constexpr auto RELOADBUTTON_TEXT = L"↻";
constexpr auto BUTTON = L"BUTTON";
constexpr auto LISTBOX = L"LISTBOX";
constexpr auto STATIC = L"STATIC";
constexpr auto DEBUG_STRING = L"-debug";
constexpr auto TB_SETTINGS_TEXT = L"Авто. скрытие";
constexpr auto TB_EXIT_TEXT = L"Выход";
constexpr auto TB_HOTKEY_TEXT = L"Ctrl + Alt + H";
constexpr auto QUERY = L"SELECT CommandLine FROM Win32_Process WHERE ProcessId = ";

//
//defines

#define POINT2CORD(point) point.left, point.top, point.right - point.left, point.bottom - point.top
#define SIZEOF(str) (sizeof(str) / sizeof(str[0]))
#define MBATTENTION(str) MessageBox(NULL, str, L"Внимание!", MB_ICONWARNING)
#define MBERROR(str) MessageBox(NULL, str, L"Ошибка!", MB_ICONERROR)

#define HK_CTIFA_ID 1
#define TIMER_ID 1
#define TB_SETTINGS 1
#define TB_EXIT 2
#define TRAY_ICON_MESSAGE (WM_USER + 1)
#define wndX 1000
#define wndY 375
#define SAVED_WINDOW 2
#define ID_LIST_APPLICATIONS 11
#define ID_LIST_FAVORITES    12
#define ID_BUTTON_ADD        13
#define ID_BUTTON_REMOVE     14
#define ID_BUTTON_RELOAD     15

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        102
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1001
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif
