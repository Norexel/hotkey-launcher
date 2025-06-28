// Test.cpp : Определяет точку входа для приложения.
//

#include <objidl.h>
#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")
#include <windows.h>
#include <vector>
#include <string>
#include <map>
#include <shlobj.h>
#include <shellapi.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <strsafe.h>
#include <tlhelp32.h>

#include "MegaTest1.h"
#include "resourcee.h"
#include "Design.h"

#ifndef HKS_DEFAULT
#define HKS_DEFAULT 0
#endif

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

Gdiplus::GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;

HWND hStatusLabel;
HWND hAddProgramButton;
HWND hProgramList;
HWND hToggleMonitoringButton;

HWND hPathLabel;
HWND hEditPath;
HWND hBrowseButton;
HWND hCommandLabel;
HWND hEditCommand;
HWND hHotkeyLabel;
HWND hHotkeyControl;
HWND hButtonOK;
HWND hButtonBack;

HFONT hAppFont = NULL;
HFONT hBoldAppFont = NULL;

std::vector<ProgramInfo> g_programs;

std::map<WORD, HANDLE> g_runningProcesses;

bool g_monitoringActive = false;

HHOOK g_keyboardHook = NULL;

HWND g_hWndMain = NULL;

NOTIFYICONDATA nid = {};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR      lpCmdLine,
    _In_ int         nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_VCAASSISTANT, szWindowClass, MAX_LOADSTRING);

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return FALSE;
    }

    g_hWndMain = FindWindowW(szWindowClass, szTitle);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_VCAASSISTANT));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (msg.message == WM_KEY_PRESS_EVENT) {
            WORD hotkey = (WORD)msg.wParam;
            ToggleProgramByHotkey(g_hWndMain, hotkey);
        }
        else if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    StopKeyboardMonitoring();

    Gdiplus::GdiplusShutdown(gdiplusToken);

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VCAASSISTANT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_VCAASSISTANT);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, 650, 450, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_HOTKEY_CLASS;
    InitCommonControlsEx(&icex);

    hStatusLabel = CreateWindowW(L"STATIC", L"Статус: Мониторинг неактивен",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        50, 20, 400, 20, hWnd, (HMENU)IDC_STATUS_LABEL, hInst, NULL);

    hToggleMonitoringButton = CreateWindowW(L"BUTTON", L"Старт Мониторинг",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        150, 60, 200, 30, hWnd, (HMENU)IDC_TOGGLE_MONITORING_BUTTON, hInst, NULL);

    hAddProgramButton = CreateWindowW(L"BUTTON", L"Добавить программу",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        150, 100, 200, 30, hWnd, (HMENU)IDC_ADD_PROGRAM_BUTTON, hInst, NULL);

    hProgramList = CreateWindowW(WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        20, 140, 460, 220, hWnd, (HMENU)IDC_PROGRAM_LIST, hInst, NULL);

    LVCOLUMN lvc = { 0 };
    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.cx = 250;
    lvc.pszText = const_cast<LPWSTR>(L"Команда / Путь");
    ListView_InsertColumn(hProgramList, 0, &lvc);

    lvc.cx = 100;
    lvc.pszText = const_cast<LPWSTR>(L"Клавиша");
    ListView_InsertColumn(hProgramList, 1, &lvc);

    hPathLabel = CreateWindowW(L"STATIC", L"Путь к программе:",
        WS_CHILD,
        20, 70, 150, 20, hWnd, (HMENU)IDC_PATH_LABEL, hInst, NULL);
    hEditPath = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        20, 95, 380, 25, hWnd, (HMENU)IDC_EDIT_PATH, hInst, NULL);
    hBrowseButton = CreateWindowW(L"BUTTON", L"Обзор...",
        WS_CHILD | BS_PUSHBUTTON,
        410, 95, 70, 25, hWnd, (HMENU)IDC_BROWSE_BUTTON, hInst, NULL);
    hCommandLabel = CreateWindowW(L"STATIC", L"Название/Описание команды:",
        WS_CHILD,
        20, 130, 250, 20, hWnd, (HMENU)IDC_COMMAND_LABEL, hInst, NULL);
    hEditCommand = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        20, 155, 460, 25, hWnd, (HMENU)IDC_EDIT_COMMAND, hInst, NULL);

    hHotkeyLabel = CreateWindowW(L"STATIC", L"Горячая клавиша:",
        WS_CHILD,
        20, 190, 150, 20, hWnd, (HMENU)IDC_HOTKEY_LABEL, hInst, NULL);
    hHotkeyControl = CreateWindowW(HOTKEY_CLASS, L"",
        WS_CHILD | WS_BORDER | HKS_DEFAULT,
        20, 215, 150, 25, hWnd, (HMENU)IDC_HOTKEY_CONTROL, hInst, NULL);

    SendMessage(hHotkeyControl, HKM_SETRULES, HKCOMB_NONE, 0);

    hButtonOK = CreateWindowW(L"BUTTON", L"ОК",
        WS_CHILD | BS_PUSHBUTTON,
        150, 260, 80, 30, hWnd, (HMENU)IDC_OK_BUTTON, hInst, NULL);
    hButtonBack = CreateWindowW(L"BUTTON", L"Назад",
        WS_CHILD | BS_PUSHBUTTON,
        270, 260, 80, 30, hWnd, (HMENU)IDC_BACK_BUTTON, hInst, NULL);


    hAppFont = CreateAppFont(9, false);
    hBoldAppFont = CreateAppFont(9, true);

    SetControlFont(hStatusLabel, hAppFont);
    SetControlFont(hToggleMonitoringButton, hAppFont);
    SetControlFont(hAddProgramButton, hAppFont);
    SetControlFont(hProgramList, hAppFont);
    SetControlFont(hPathLabel, hAppFont);
    SetControlFont(hEditPath, hAppFont);
    SetControlFont(hBrowseButton, hAppFont);
    SetControlFont(hCommandLabel, hAppFont);
    SetControlFont(hEditCommand, hAppFont);
    SetControlFont(hHotkeyLabel, hAppFont);
    SetControlFont(hHotkeyControl, hAppFont);
    SetControlFont(hButtonOK, hAppFont);
    SetControlFont(hButtonBack, hAppFont);

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = IDI_VCAASSISTANT;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON_MESSAGE;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VCAASSISTANT));
    StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), szTitle);
    Shell_NotifyIcon(NIM_ADD, &nid);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;

}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDM_ABOUT), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDC_ADD_PROGRAM_BUTTON:
            ShowMainWindowControls(hWnd, false);
            ShowAddProgramScreen(hWnd, true);
            ClearAddProgramInputs(hWnd);
            break;
        case IDC_TOGGLE_MONITORING_BUTTON:
            if (g_monitoringActive) {
                StopKeyboardMonitoring();
                SetWindowText(hToggleMonitoringButton, L"Старт Мониторинг");
                SetWindowText(hStatusLabel, L"Статус: Мониторинг неактивен");
            }
            else {
                StartKeyboardMonitoring(hWnd);
                SetWindowText(hToggleMonitoringButton, L"Стоп Мониторинг");
                SetWindowText(hStatusLabel, L"Статус: Мониторинг активен. Ожидание клавиши...");
            }
            g_monitoringActive = !g_monitoringActive;
            break;
        case IDC_BROWSE_BUTTON:
        {
            OPENFILENAME ofn;
            WCHAR szFile[MAX_PATH] = { 0 };

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = ARRAYSIZE(szFile);
            ofn.lpstrFilter = L"Исполняемые файлы (*.exe)\0*.exe\0Все файлы (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

            if (GetOpenFileName(&ofn) == TRUE) {
                SetWindowText(hEditPath, szFile);
            }
        }
        break;
        case IDC_OK_BUTTON:
        {
            WCHAR path[MAX_PATH];
            WCHAR command[MAX_PATH];
            WORD hotkey = (WORD)SendMessage(hHotkeyControl, HKM_GETHOTKEY, 0, 0);

            GetWindowText(hEditPath, path, ARRAYSIZE(path));
            GetWindowText(hEditCommand, command, ARRAYSIZE(command));

            if (wcslen(path) > 0 && wcslen(command) > 0 && hotkey != 0) {
                bool hotkeyExists = false;
                for (const auto& p : g_programs) {
                    if (p.hotkey == hotkey) {
                        hotkeyExists = true;
                        break;
                    }
                }

                if (hotkeyExists) {
                    MessageBox(hWnd, L"Эта горячая клавиша уже используется для другой программы. Пожалуйста, выберите другую.", L"Ошибка", MB_OK | MB_ICONWARNING);
                }
                else {
                    g_programs.push_back({ path, command, hotkey });
                    LoadProgramsToList(hWnd);
                    ShowAddProgramScreen(hWnd, false);
                    ShowMainWindowControls(hWnd, true);
                }
            }
            else {
                MessageBox(hWnd, L"Пожалуйста, заполните все поля и выберите горячую клавишу.", L"Ошибка", MB_OK | MB_ICONWARNING);
            }
        }
        break;
        case IDC_BACK_BUTTON:
            ShowAddProgramScreen(hWnd, false);
            ShowMainWindowControls(hWnd, true);
            break;
        case IDM_TRAY_RESTORE:
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            break;
        case IDM_TRAY_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        DrawCustomBackground(hdc, hWnd);
        DrawWSAHeader(hdc, hWnd);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_CREATE:
        LoadProgramsToList(hWnd);
        break;
    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        break;
    case WM_TRAYICON_MESSAGE:
        switch (LOWORD(lParam)) {
        case WM_RBUTTONDOWN:
        {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            if (hMenu) {
                InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_TRAY_RESTORE, L"Показать");
                InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_STRING, IDM_TRAY_EXIT, L"Выход");
                SetForegroundWindow(hWnd);
                TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
                PostMessage(hWnd, WM_NULL, 0, 0);
                DestroyMenu(hMenu);
            }
        }
        break;
        case WM_LBUTTONDBLCLK:
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            break;
        }
        break;
    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


void ShowMainWindowControls(HWND hWnd, bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    ShowWindow(hStatusLabel, cmd);
    ShowWindow(hAddProgramButton, cmd);
    ShowWindow(hToggleMonitoringButton, cmd);
    ShowWindow(hProgramList, cmd);
}

void ShowAddProgramScreen(HWND hWnd, bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    ShowWindow(hPathLabel, cmd);
    ShowWindow(hEditPath, cmd);
    ShowWindow(hBrowseButton, cmd);
    ShowWindow(hCommandLabel, cmd);
    ShowWindow(hEditCommand, cmd);
    ShowWindow(hHotkeyLabel, cmd);
    ShowWindow(hHotkeyControl, cmd);
    ShowWindow(hButtonOK, cmd);
    ShowWindow(hButtonBack, cmd);

    if (show) {
        SetFocus(hEditPath);
        SendMessage(hHotkeyControl, HKM_SETHOTKEY, 0, 0);
    }
}

void ClearAddProgramInputs(HWND hWnd) {
    SetWindowText(hEditPath, L"");
    SetWindowText(hEditCommand, L"");
    SendMessage(hHotkeyControl, HKM_SETHOTKEY, 0, 0);
}

void LoadProgramsToList(HWND hWnd) {
    ListView_DeleteAllItems(hProgramList);

    RECT lvRect;
    GetWindowRect(hProgramList, &lvRect);
    POINT ptListView = { lvRect.left, lvRect.top };
    ScreenToClient(hWnd, &ptListView);

    int itemHeight = 22;

    for (size_t i = 0; i < g_programs.size(); ++i) {
        LVITEM lvI = { 0 };
        lvI.mask = LVIF_TEXT;
        lvI.iItem = (int)i;
        lvI.iSubItem = 0;
        std::wstring itemText = L"\"" + g_programs[i].command + L"\" -> " + g_programs[i].path;
        lvI.pszText = const_cast<LPWSTR>(itemText.c_str());
        ListView_InsertItem(hProgramList, &lvI);

        lvI.iSubItem = 1;
        WCHAR szHotkey[256];
        WORD hotkey = g_programs[i].hotkey;
        BYTE vk = LOBYTE(hotkey);
        BYTE hotkeyFlags = HIBYTE(hotkey);

        std::wstring hotkeyStr;
        if (hotkeyFlags & HOTKEYF_CONTROL) hotkeyStr += L"Ctrl+";
        if (hotkeyFlags & HOTKEYF_ALT) hotkeyStr += L"Alt+";
        if (hotkeyFlags & HOTKEYF_SHIFT) hotkeyStr += L"Shift+";

        UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
        if (scanCode != 0) {
            if (GetKeyNameTextW(scanCode << 16, szHotkey, ARRAYSIZE(szHotkey))) {
                hotkeyStr += szHotkey;
            }
            else {
                hotkeyStr += L"VK_" + std::to_wstring(vk);
            }
        }
        else {
            hotkeyStr += L"VK_" + std::to_wstring(vk);
        }

        lvI.pszText = const_cast<LPWSTR>(hotkeyStr.c_str());
        ListView_SetItem(hProgramList, &lvI);
    }
}

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_monitoringActive) {
        KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            WORD hotkey = 0;
            BYTE hotkeyFlags = 0;
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000) hotkeyFlags |= HOTKEYF_CONTROL;
            if (GetAsyncKeyState(VK_MENU) & 0x8000) hotkeyFlags |= HOTKEYF_ALT;
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) hotkeyFlags |= HOTKEYF_SHIFT;

            hotkey = MAKEWORD(pKeyBoard->vkCode, hotkeyFlags);

            PostMessage(g_hWndMain, WM_KEY_PRESS_EVENT, (WPARAM)hotkey, 0);

            for (const auto& program : g_programs) {
                if (program.hotkey == hotkey) {
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

void StartKeyboardMonitoring(HWND hWnd) {
    if (g_keyboardHook == NULL) {
        g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, hInst, 0);
        if (g_keyboardHook == NULL) {
            MessageBox(hWnd, L"Не удалось установить хук клавиатуры. Проверьте права доступа.", L"Ошибка", MB_OK | MB_ICONERROR);
        }
    }
}

void StopKeyboardMonitoring() {
    if (g_keyboardHook != NULL) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = NULL;
    }
}

HWND GetWindowHandleFromProcessId(DWORD processId) {
    HWND hWnd = GetTopWindow(NULL);
    while (hWnd) {
        DWORD currentProcessId;
        GetWindowThreadProcessId(hWnd, &currentProcessId);
        if (currentProcessId == processId) {
            if (IsWindowVisible(hWnd) && GetWindowTextLengthW(hWnd) > 0 && GetParent(hWnd) == NULL) {
                return hWnd;
            }
        }
        hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
    }
    return NULL;
}

void ToggleProgramByHotkey(HWND hWnd, WORD hotkey) {
    std::wstring programPath;
    std::wstring programCommandName;
    for (const auto& program : g_programs) {
        if (program.hotkey == hotkey) {
            programPath = program.path;
            programCommandName = program.command;
            break;
        }
    }

    if (programPath.empty()) {
        SetWindowText(hStatusLabel, L"Неизвестная горячая клавиша.");
        return;
    }

    if (g_runningProcesses.count(hotkey)) {
        HANDLE hProcess = g_runningProcesses[hotkey];
        DWORD exitCode;
        if (GetExitCodeProcess(hProcess, &exitCode) && exitCode == STILL_ACTIVE) {
            SetWindowText(hStatusLabel, (L"Попытка закрыть: " + programCommandName).c_str());

            HWND hProgramWindow = GetWindowHandleFromProcessId(GetProcessId(hProcess));
            if (hProgramWindow) {
                PostMessage(hProgramWindow, WM_CLOSE, 0, 0);
            }

            if (WaitForSingleObject(hProcess, 5000) == WAIT_TIMEOUT) {
                if (MessageBox(hWnd, (L"Приложение \"" + programCommandName + L"\" не отвечает. Принудительно завершить?").c_str(), L"Завершение программы", MB_YESNO | MB_ICONWARNING) == IDYES) {
                    TerminateProcess(hProcess, 0);
                    SetWindowText(hStatusLabel, (L"Принудительно закрыто: " + programCommandName).c_str());
                }
                else {
                    SetWindowText(hStatusLabel, (L"Отмена закрытия: " + programCommandName).c_str());
                    return;
                }
            }
            CloseHandle(hProcess);
            g_runningProcesses.erase(hotkey);
            SetWindowText(hStatusLabel, (L"Закрыто: " + programCommandName).c_str());
        }
        else {
            CloseHandle(hProcess);
            g_runningProcesses.erase(hotkey);
            SetWindowText(hStatusLabel, (L"Процесс завершен: " + programCommandName).c_str());
        }
    }
    else {
        SetWindowText(hStatusLabel, (L"Запуск: " + programCommandName).c_str());

        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = L"open";
        sei.lpFile = programPath.c_str();
        sei.nShow = SW_SHOWNORMAL;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;

        if (ShellExecuteEx(&sei)) {
            if (sei.hProcess != NULL) {
                g_runningProcesses[hotkey] = sei.hProcess;
                SetWindowText(hStatusLabel, (L"Запущено: " + programCommandName).c_str());
            }
            else {
                SetWindowText(hStatusLabel, (L"Ошибка запуска (не удалось получить handle): " + programCommandName).c_str());
            }
        }
        else {
            DWORD error = GetLastError();
            LPWSTR lpMsgBuf = nullptr;
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                error,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPWSTR)&lpMsgBuf,
                0, NULL);
            std::wstring errorMessage = (lpMsgBuf) ? std::wstring(lpMsgBuf) : L"Неизвестная ошибка.";
            MessageBox(hWnd, (L"Не удалось запустить программу: " + errorMessage).c_str(), L"Ошибка запуска", MB_OK | MB_ICONERROR);
            if (lpMsgBuf) LocalFree(lpMsgBuf);
            SetWindowText(hStatusLabel, (L"Ошибка запуска: " + programCommandName).c_str());
        }
    }
}