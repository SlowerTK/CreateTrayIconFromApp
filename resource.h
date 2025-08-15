#define IDI_ICON1                       101
//
//
//
//System keys
constexpr auto SY_APP_NAME = L"CTIFA";
constexpr auto SY_FOLDER_NAME = L"\\CTIFA";
constexpr auto SY_SETTINGS_FILENAME = L"\\settings";
constexpr auto SY_REGISTRY_PATH = L"Software\\CTIFA";
constexpr auto SY_FILE_EXTENSION = L".log";
constexpr auto SY_DEBUG_ARG = L"-debug";
constexpr auto SY_REG_KEY_TIMER_FLAG = L"isTimer";
constexpr auto SY_REG_KEY_TIMER_VAL = L"TimerNumber";
constexpr auto SY_REG_KEY_MODKEYS = L"modKeys";
constexpr auto SY_REG_KEY_VKEYS = L"Keys";
constexpr auto SY_TASKBAR_MSG = L"TaskbarCreated";
constexpr auto SY_CLASS_TRAY = L"CTRIFATrayApp";
constexpr auto SY_CLASS_SETTINGS = L"CTIFA Settings";
constexpr auto SY_CLASS_TIMERSET = L"CTIFA Timer Settings";
constexpr auto SY_COMMAND_LINE = L"CommandLine";
//Warning texts
constexpr auto WT_REQUIRE_ADMIN = L"Для корректной работы программы рекомендуется запускать её с правами Администратора";
constexpr auto WT_ALREADY_RUNNING = L"Экземпляр программы уже запущен\tВыключение\n";
//Error texts
constexpr auto ET_HOTKEY_REGISTER = L"Не удалось зарегистрировать горячие клавиши";
constexpr auto ET_GET_APPDATA = L"Не удалось определить путь к AppData";
constexpr auto ET_CREATE_FOLDER = L"Не удалось создать папку";
constexpr auto ET_CREATE_FILE = L"Не удалось создать файл";
constexpr auto ET_OPEN_FILE = L"Не удалось открыть файл";
constexpr auto ET_FILE_SIZE = L"Не удалось определить размер файла";
constexpr auto ET_FILE_READ = L"Не удалось считать файл";
constexpr auto ET_FILE_WRITE = L"Не удалось добавить запись в файл";
constexpr auto ET_GET_CMDLINE = L"Не удалось получить строку запуска";

constexpr auto ET_COM_INIT = L"Ошибка инициализации COM";
constexpr auto ET_COM_SECURITY = L"Ошибка настройки безопасности COM";
constexpr auto ET_WMI_CREATE = L"Не удалось создать WMI-локатор";
constexpr auto ET_WMI_CONNECT = L"Не удалось подключиться к WMI";
constexpr auto ET_WMI_SECURITY = L"Не удалось установить параметры безопасности";
constexpr auto ET_WMI_QUERY = L"Не удалось выполнить запрос WMI";

constexpr auto ET_ICON_LOAD = L"Ошибка загрузки иконки";
constexpr auto ET_ICON_CREATE = L"Ошибка создания иконки: 0x%08X";
constexpr auto ET_ICON_DELETE = L"Ошибка удаления иконки: 0x%08X";

constexpr auto ET_TASK_CREATE = L"Не удалось создать новую задачу: 0x%08X";
constexpr auto ET_TASK_DELETE = L"Не удалось удалить задачу: 0x%08X";
constexpr auto ET_TASK_REGISTER = L"Не удалось зарегистрировать задачу: 0x%08X";
constexpr auto ET_TASKSERVICE_CREATE = L"Не удалось создать экземпляр TaskService: 0x%08X";
constexpr auto ET_TASKSERVICE_CONNECT = L"Не удалось подключиться к TaskScheduler: 0x%08X";
constexpr auto ET_TASK_ROOT_ACCESS = L"Не удалось получить доступ к корневой папке: 0x%08X";
constexpr auto ET_COINIT_EX = L"Не удалось выполнить CoInitializeEx: 0x%08X";
constexpr auto ET_INITIALIZE_TASK = L"Не удалось выполнить InitializeTaskService";
constexpr auto ET_GET_ROOT_FOLDER = L"Не удалось выполнить GetRootFolder";
constexpr auto ET_ROOTFOLDER_NULL = L"pRootFolder == NULL";
//Information texts
constexpr auto IT_AUTORUN_ADDED = L"Добавлено в автозагрузку";
constexpr auto IT_AUTORUN_REMOVED = L"Удалено из автозагрузки";
constexpr auto IT_SETTINGS_UPDATED = L"Файл настроек переписан";
constexpr auto IT_RESTARTING = L"Перезапуск запущен";
constexpr auto IT_ICON_RECREATE = L"Пересоздаю иконку";
constexpr auto IT_ADMIN_MISSING = L"Без прав администратора";
constexpr auto IT_ADMIN_GRANTED = L"Права администратора получены";
constexpr auto IT_SHUTDOWN = L"Завершение работы\n";
constexpr auto IT_TASK_DELETED = L"Задача успешно удалена!";
constexpr auto IT_TASK_REGISTERED = L"Задача успешно зарегистрирована!";
constexpr auto IT_NO_KEY1 = L"Значение KEY1 отсутствует, используем стандартное: ";
constexpr auto IT_NO_KEY2 = L"Значение KEY2 отсутствует, используем стандартное: ";
constexpr auto IT_TIMER_VALUE_SET = L"Записано значение таймера: ";
constexpr auto IT_NEW_HOTKEY = L"Новое сочетание записано: ";
constexpr auto IT_ICON_CREATED = L"Иконка создана";
constexpr auto IT_ICON_DELETED = L"Иконка успешно удалена";
constexpr auto IT_TRAY_WAIT_RETRY = L"Первая попытка добавить иконку не удалась, жду TaskbarCreated...";
constexpr auto IT_OPEN_SETTINGS = L"Открыты настройки";
constexpr auto IT_FAVORITE_ADDED = L"В избранное добавлено ";
constexpr auto IT_HOTKEYMVALUEMISS = L"Значение HOTKEYM отсутствует, используем стандартное: ";
constexpr auto IT_HOTKEYVKVALUEMISS = L"Значение HOTKEYVK отсутствует, используем стандартное: ";
constexpr auto IT_TIMER = L"Таймер ";
constexpr auto IT_TIMER_ON = L"включён";
constexpr auto IT_TIMER_OFF = L"выключен";
//UI texts
constexpr auto TX_ADMIN_RESTART = L"Перезапустить от имени администратора";
constexpr auto TX_AUTORUN = L"Запускать вместе с windows";
constexpr auto TX_UI_TITLE = L"Для корректной работы нужны права администратора";
constexpr auto TX_WINDOWS_LIST = L"Все приложения:";
constexpr auto TX_WINDOWS_SELECTED = L"Будут скрываться автоматически:";
constexpr auto TX_TRAY_TOOLTIP = L"Скрытые окна";
constexpr auto TX_TRAY_BTN_SETTINGS = L"Настройки";
constexpr auto TX_TRAY_BTN_EXIT = L"Выход";
constexpr auto TX_TRAY_BTN_HOTKEY = L"Ctrl + Alt + H";
constexpr auto TX_BTN_ADD = L"▶";
constexpr auto TX_BTN_REMOVE = L"◀";
constexpr auto TX_BTN_RELOAD = L"↻";
constexpr auto TX_BTN_HOTKEY_SET = L"Изменить";
constexpr auto TX_BTN_SAVE = L"Сохранить";
constexpr auto TX_BTN_RESET = L"Сброс";
constexpr auto TX_BTN_CANCEL = L"Отмена";
constexpr auto TX_HOTKEY_TEXT = L"Текущие сочетание клавиш для скрытия активного окна";
constexpr auto TX_PRESS_HOTKEY = L"Введите новое сочетание клавиш";
constexpr auto TX_PRESS_KEYS = L"Нажмите клавиши";
constexpr auto TX_CHECKBOX_RESTORE = L"Скрывать восстановленные избранные";
constexpr auto TX_EDITBOX_DELAY = L"Через сколько скрывать восстановленное окно?(мс)";
constexpr auto TX_ICON_HINT = L"❓";
constexpr auto TX_UI_CONTROL_BUTTON = L"BUTTON";
constexpr auto TX_UI_CONTROL_LIST = L"LISTBOX";
constexpr auto TX_UI_CONTROL_STATIC = L"STATIC";
constexpr auto TX_UI_CONTROL_EDIT = L"EDIT";
constexpr auto IX_WINDOW_HINT = L"Если опция «Скрывать восстановленные избранные» отключена, то избранные окна будут автоматически скрываться только в момент их создания.\nЧтобы восстановить конкретное окно и предотвратить его повторное скрытие, удерживайте клавишу Shift при восстановлении этого окна.\nВо всех остальных случаях восстановленные окна будут автоматически скрыты спустя заданное количество миллисекунд.";
constexpr auto IX_ALLOWED_KEYS = L"Допустимы только сочетания клавиш, имеющие не менее одной модальной клавиши,\n такой как Ctrl, Alt, Shift или Win.";

//
constexpr unsigned int CONST_AUTOHIDE_DELAY_MS = 5'000;
constexpr auto CONST_WND_WIDTH = 1200;
constexpr auto CONST_WND_HEIGHT = 375;

constexpr auto ID_HOTKEY_HIDE_ACTIVE = 1;
constexpr auto ID_TIMER_AUTOHIDE = 1;

constexpr auto ID_WND_SAVED_FAVORITES = 2;
constexpr auto ID_WND_TIMED_HIDE = 3;

constexpr auto ID_LIST_APPS = 11;
constexpr auto ID_LIST_FAVORITES = 12;

constexpr auto ID_BTN_ADD = 13;
constexpr auto ID_BTN_REMOVE = 14;
constexpr auto ID_BTN_RELOAD = 15;
constexpr auto ID_BTN_AUTOSTART = 16;
constexpr auto ID_BTN_TIME_AUTOHIDE = 17;
constexpr auto ID_BTN_HINT = 18;
constexpr auto ID_EDIT_DELAY_FIELD = 19;
constexpr auto ID_BTN_HOTKEY_CHANGE = 20;

constexpr auto ID_BTN_OK = 101;
constexpr auto ID_BTN_RESET = 102;
constexpr auto ID_BTN_CANCEL = 103;

constexpr auto ID_TRAY_SETTINGS = 1;
constexpr auto ID_TRAY_EXIT = 2;
constexpr auto ID_TRAY_RESTART = 3;

constexpr auto ID_CLR_BLACK = RGB(0, 0, 0);
constexpr auto ID_CLR_LIGHT = RGB(255, 255, 255);
constexpr auto ID_CLR_LIGHTGRAY = RGB(243, 243, 243);
constexpr auto ID_CLR_DARK = RGB(30, 30, 30);
constexpr auto ID_CLR_DARKGRAY = RGB(59, 59, 59);
constexpr auto ID_CLR_GRAY = RGB(109, 109, 109);
constexpr auto ID_CLR_TRAYLIGHT = RGB(249, 249, 249);
constexpr auto ID_CLR_TRAYGRAY = RGB(44, 44, 44);
constexpr auto ID_CLR_DISABLED_D = RGB(100, 100, 100);
constexpr auto ID_CLR_DISABLED_L = RGB(200, 200, 200);
//
//defines

#define RECT2CORD(rect) rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top
#define SIZEOF(str) (sizeof(str) / sizeof(str[0]))
#define MBATTENTION(str) MessageBox(NULL, str, L"Внимание!", MB_ICONWARNING | MB_SETFOREGROUND)
#define MBERROR(str) MessageBox(NULL, str, L"Ошибка!", MB_ICONERROR | MB_SETFOREGROUND)
#define MBINFO(str) MessageBox(hwnd, str, L"Инфо", MB_OK | MB_SETFOREGROUND);
#define TRAY_ICON_MESSAGE	(WM_USER + 1)


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
