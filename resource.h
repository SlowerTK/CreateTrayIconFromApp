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
// 
constexpr auto appName = L"CTIFA";
//File strings
constexpr auto FILEPATHNAME = L"\\settings";
constexpr auto FOLDERNAME = L"\\CTIFA";
constexpr auto REG_PATH = L"Software\\CTIFA";
//Warning texts
constexpr auto WT_ADMIN = L"Для корректной работы программы рекомендуется запускать её с правами Администратора";
//Error texts
constexpr auto ET_HOTKEY = L"Не удалось зарегистрировать горячие клавиши";
constexpr auto ET_APPDATA = L"Не удалось определить путь к AppData";
constexpr auto ET_CREATEFOLDER = L"Не удалось создать папку";
constexpr auto ET_CREATEFILE = L"Не удалось создать файл";
constexpr auto ET_FILEOPEN = L"Не удалось открыть файл";
constexpr auto ET_FILESIZE = L"Не удалось определить размер файла";
constexpr auto ET_FILECONTENT = L"Не удалось считать файл";
constexpr auto ET_FILEWRITE = L"Не удалось добавить запись в файл";
constexpr auto ET_COMMANDLINE = L"Не удалось получить строку запуска";
//Some texts
constexpr auto NEED_ADMIN_RIGHTS = L"Перезапустить от имени администратора";
constexpr auto WND_TITLE = L"Для корректной работы нужны права администратора";
constexpr auto SZ_TIP = L"Скрытые окна";
constexpr auto WND_NAME_TEXT = L"Все приложения:";
constexpr auto WND_NAME_TEXT2 = L"Будут скрываться автоматически:";
constexpr auto ADDBUTTON_TEXT = L"▶";
constexpr auto REMOVEBUTTON_TEXT = L"◀";
constexpr auto RELOADBUTTON_TEXT = L"↻";
constexpr auto STARTUP_TEXT = L"Запускать вместе с windows?";
constexpr auto HINT_TEXT = L"❓";
constexpr auto CHECKBOX_TEXT = L"Скрывать восстановленные избранные?";
constexpr auto EDITBOX_TEXT = L"Через сколько скрывать восстановленное окно?(мс)";
constexpr auto HOTKEY_TEXT = L"Текущие сочетание клавиш для скрытия активного окна";
constexpr auto HOTKEYBUTTON_TEXT = L"Изменить";
constexpr auto BUTTON = L"BUTTON";
constexpr auto LISTBOX = L"LISTBOX";
constexpr auto STATIC = L"STATIC";
constexpr auto EDIT = L"EDIT";
constexpr auto DEBUG_STRING = L"-debug";
constexpr auto TB_SETTINGS_TEXT = L"Авто. скрытие";
constexpr auto TB_EXIT_TEXT = L"Выход";
constexpr auto TB_HOTKEY_TEXT = L"Ctrl + Alt + H";
constexpr auto QUERY = L"SELECT CommandLine FROM Win32_Process WHERE ProcessId = ";
constexpr auto KEY1 = L"isTimer";
constexpr auto KEY2 = L"TimerNumber";
constexpr auto HINT_MSG = L"Если пункт «Скрывать восстановленные избранные» выключен, то избранные окна будут скрываться только при создании. Если нужно восстановить только одно окно, чтоб оно не скрывалось, можно при выборе окна зажать клавишу `Shift`. \nИначе восстановленное окно будет скрыто через выбранное количество миллисекунд.";
constexpr auto PRESSKEYS = L"Нажмите клавиши";
//
constexpr auto SECOND = 5'000;
//
//defines

#define RECT2CORD(rect) rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top
#define SIZEOF(str) (sizeof(str) / sizeof(str[0]))
#define MBATTENTION(str) MessageBox(NULL, str, L"Внимание!", MB_ICONWARNING)
#define MBERROR(str) MessageBox(NULL, str, L"Ошибка!", MB_ICONERROR)

#define HK_CTIFA_ID 1
#define TIMER_ID 1
#define TB_SETTINGS 1
#define TB_EXIT 2
#define TB_RESTART 3
#define TRAY_ICON_MESSAGE (WM_USER + 1)
#define wndX 1200
#define wndY 375
#define SAVED_WINDOW 2
#define TIMED_WINDOW 3
#define ID_LIST_APPLICATIONS       11
#define ID_LIST_FAVORITES          12
#define ID_BUTTON_ADD              13
#define ID_BUTTON_REMOVE           14
#define ID_BUTTON_RELOAD           15
#define ID_BUTTON_AUTOSTART        16
#define ID_BUTTON_TIMEAUTOHIDE     17
#define ID_BUTTON_HINT			   18
#define ID_EDIT_FIELD		       19
#define ID_BUTTON_HK		       20
#define ID_OK_BUTTON			101
#define ID_RESET_BUTTON			102
#define ID_CANCEL_BUTTON		103

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
