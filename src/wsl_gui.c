#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "wsl_core.h"

// DPI Support
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

typedef UINT (WINAPI *GETDPIFORWINDOWPROC)(HWND);

UINT GetWindowDPI(HWND hwnd) {
    HMODULE hUser32 = GetModuleHandle(L"user32.dll");
    if (hUser32) {
        GETDPIFORWINDOWPROC pGetDpiForWindow = (GETDPIFORWINDOWPROC)GetProcAddress(hUser32, "GetDpiForWindow");
        if (pGetDpiForWindow) return pGetDpiForWindow(hwnd);
    }
    HDC hdc = GetDC(hwnd);
    UINT dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwnd, hdc);
    return dpi;
}

int Scale(int value, UINT dpi) {
    return MulDiv(value, dpi, 96);
}

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")

#define ID_LISTVIEW 101
#define ID_BTN_REFRESH 102
#define ID_BTN_START 103
#define ID_BTN_STOP 104
#define ID_BTN_UNREGISTER 105
#define ID_BTN_INSTALL_LIST 106
#define ID_BTN_SHUTDOWN 107

#define ID_ONLINE_LISTVIEW 201
#define ID_ONLINE_BTN_INSTALL 202

Distro g_distros[64];
int g_distro_count = 0;
OnlineDistro g_online_distros[128];
int g_online_count = 0;

HWND g_hListView;
HWND g_hOnlineWindow = NULL;
HWND g_hOnlineListView;
HINSTANCE g_hInstance;
HFONT g_hFont;
HWND g_hBtns[6]; 

void RefreshList() {
    ListView_DeleteAllItems(g_hListView);
    g_distro_count = 0;
    wchar_t* output = get_wsl_output(L"wsl.exe --list --verbose");
    if (!output) return;
    wchar_t* next_line = NULL;
    wchar_t* line = wcstok(output, L"\r\n", &next_line);
    int line_num = 0;
    while (line != NULL) {
        line_num++;
        if (line_num > 1) {
            Distro d = {0};
            wchar_t* ptr = line;
            while (*ptr == L' ') ptr++;
            if (*ptr == L'*') { d.is_default = 1; ptr++; while (*ptr == L' ') ptr++; }
            if (swscanf(ptr, L"%ls %ls %d", d.name, d.state, &d.version) >= 3) {
                g_distros[g_distro_count++] = d;
                LVITEM lvItem = {0};
                lvItem.mask = LVIF_TEXT;
                lvItem.iItem = g_distro_count - 1;
                lvItem.pszText = d.is_default ? L"*" : L"";
                ListView_InsertItem(g_hListView, &lvItem);
                ListView_SetItemText(g_hListView, lvItem.iItem, 1, d.name);
                ListView_SetItemText(g_hListView, lvItem.iItem, 2, d.state);
                wchar_t verStr[10]; swprintf(verStr, 10, L"%d", d.version);
                ListView_SetItemText(g_hListView, lvItem.iItem, 3, verStr);
            }
        }
        line = wcstok(NULL, L"\r\n", &next_line);
    }
    free(output);
}

void RefreshOnlineList() {
    ListView_DeleteAllItems(g_hOnlineListView);
    g_online_count = get_online_distros(g_online_distros, 128);
    for (int i = 0; i < g_online_count; i++) {
        LVITEM lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = i;
        lvItem.pszText = g_online_distros[i].name;
        ListView_InsertItem(g_hOnlineListView, &lvItem);
        ListView_SetItemText(g_hOnlineListView, i, 1, g_online_distros[i].friendly_name);
    }
}

LRESULT CALLBACK OnlineWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static UINT dpi = 96;
    switch (uMsg) {
        case WM_CREATE: {
            dpi = GetWindowDPI(hwnd);
            g_hOnlineListView = CreateWindow(WC_LISTVIEW, NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_BORDER, 
                                           0, 0, 0, 0, hwnd, (HMENU)ID_ONLINE_LISTVIEW, NULL, NULL);
            SendMessage(g_hOnlineListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            SendMessage(g_hOnlineListView, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            LVCOLUMN lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.pszText = L"名称"; lvc.cx = Scale(150, dpi); ListView_InsertColumn(g_hOnlineListView, 0, &lvc);
            lvc.pszText = L"友好名称"; lvc.cx = Scale(280, dpi); ListView_InsertColumn(g_hOnlineListView, 1, &lvc);
            HWND hBtn = CreateWindow(L"BUTTON", L"安装所选", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hwnd, (HMENU)ID_ONLINE_BTN_INSTALL, NULL, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            RefreshOnlineList();
            break;
        }
        case WM_DPICHANGED: {
            dpi = LOWORD(wParam);
            RECT* const prcNewWindow = (RECT*)lParam;
            SetWindowPos(hwnd, NULL, prcNewWindow->left, prcNewWindow->top, prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top, SWP_NOZORDER | SWP_NOACTIVATE);
            ListView_SetColumnWidth(g_hOnlineListView, 0, Scale(150, dpi));
            ListView_SetColumnWidth(g_hOnlineListView, 1, Scale(280, dpi));
            break;
        }
        case WM_SIZE: {
            int width = LOWORD(lParam); int height = HIWORD(lParam);
            int margin = Scale(10, dpi);
            int btnHeight = Scale(30, dpi);
            MoveWindow(g_hOnlineListView, margin, margin, width - 2*margin, height - Scale(60, dpi), TRUE);
            HWND hBtn = GetDlgItem(hwnd, ID_ONLINE_BTN_INSTALL);
            MoveWindow(hBtn, margin, height - Scale(40, dpi), Scale(100, dpi), btnHeight, TRUE);
            break;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == ID_ONLINE_BTN_INSTALL) {
                int sel = ListView_GetNextItem(g_hOnlineListView, -1, LVNI_SELECTED);
                if (sel != -1) {
                    install_wsl_distro(g_online_distros[sel].name);
                    MessageBox(hwnd, L"安装已启动。", L"提示", MB_OK);
                }
            }
            break;
        }
        case WM_CLOSE: g_hOnlineWindow = NULL; DestroyWindow(hwnd); break;
        default: return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void ShowOnlineInstallWindow(HWND parent) {
    if (g_hOnlineWindow) { SetForegroundWindow(g_hOnlineWindow); return; }
    const wchar_t CLASS[] = L"WSLOnlineInstallGUI";
    WNDCLASS wc = {0}; wc.lpfnWndProc = OnlineWindowProc; wc.hInstance = g_hInstance; wc.lpszClassName = CLASS;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);
    HDC hdc = GetDC(NULL);
    UINT screenDpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);
    g_hOnlineWindow = CreateWindowEx(0, CLASS, L"可安装列表", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, Scale(500, screenDpi), Scale(450, screenDpi), parent, NULL, g_hInstance, NULL);
    if (g_hOnlineWindow) ShowWindow(g_hOnlineWindow, SW_SHOW);
}

void RunAction(const wchar_t* action, const wchar_t* name) {
    run_wsl_action(action, name);
    RefreshList();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static UINT dpi = 96;
    switch (uMsg) {
        case WM_CREATE: {
            dpi = GetWindowDPI(hwnd);
            INITCOMMONCONTROLSEX icex; icex.dwSize = sizeof(icex); icex.dwICC = ICC_LISTVIEW_CLASSES; InitCommonControlsEx(&icex);
            g_hFont = CreateFont(Scale(16, dpi), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            g_hListView = CreateWindow(WC_LISTVIEW, NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_BORDER, 0, 0, 0, 0, hwnd, (HMENU)ID_LISTVIEW, NULL, NULL);
            SendMessage(g_hListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            SendMessage(g_hListView, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            LVCOLUMN lvc = {0}; lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.pszText = L"默认"; lvc.cx = Scale(40, dpi); ListView_InsertColumn(g_hListView, 0, &lvc);
            lvc.pszText = L"名称"; lvc.cx = Scale(150, dpi); ListView_InsertColumn(g_hListView, 1, &lvc);
            lvc.pszText = L"状态"; lvc.cx = Scale(100, dpi); ListView_InsertColumn(g_hListView, 2, &lvc);
            lvc.pszText = L"版本"; lvc.cx = Scale(60, dpi); ListView_InsertColumn(g_hListView, 3, &lvc);
            
            const wchar_t* btnLabels[] = { L"刷新列表", L"启动", L"停止", L"卸载", L"在线安装", L"全部关闭" };
            int btnIds[] = { ID_BTN_REFRESH, ID_BTN_START, ID_BTN_STOP, ID_BTN_UNREGISTER, ID_BTN_INSTALL_LIST, ID_BTN_SHUTDOWN };
            for(int i=0; i<6; i++) {
                g_hBtns[i] = CreateWindow(L"BUTTON", btnLabels[i], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hwnd, (HMENU)btnIds[i], NULL, NULL);
                SendMessage(g_hBtns[i], WM_SETFONT, (WPARAM)g_hFont, TRUE);
            }
            RefreshList();
            break;
        }
        case WM_DPICHANGED: {
            dpi = LOWORD(wParam);
            RECT* const prcNewWindow = (RECT*)lParam;
            SetWindowPos(hwnd, NULL, prcNewWindow->left, prcNewWindow->top, prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top, SWP_NOZORDER | SWP_NOACTIVATE);
            
            if (g_hFont) DeleteObject(g_hFont);
            g_hFont = CreateFont(Scale(16, dpi), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            
            SendMessage(g_hListView, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            ListView_SetColumnWidth(g_hListView, 0, Scale(40, dpi));
            ListView_SetColumnWidth(g_hListView, 1, Scale(150, dpi));
            ListView_SetColumnWidth(g_hListView, 2, Scale(100, dpi));
            ListView_SetColumnWidth(g_hListView, 3, Scale(60, dpi));
            
            for(int i=0; i<6; i++) SendMessage(g_hBtns[i], WM_SETFONT, (WPARAM)g_hFont, TRUE);
            break;
        }
        case WM_SIZE: {
            int width = LOWORD(lParam); int height = HIWORD(lParam);
            int margin = Scale(10, dpi);
            int btnHeight = Scale(30, dpi);
            int bottomSpacing = Scale(90, dpi);
            
            MoveWindow(g_hListView, margin, margin, width - 2*margin, height - bottomSpacing, TRUE);
            
            int bw = (width - 4*margin) / 4; if (bw > Scale(100, dpi)) bw = Scale(100, dpi);
            for(int i=0; i<4; i++) MoveWindow(g_hBtns[i], margin + i*(bw+Scale(5, dpi)), height - Scale(75, dpi), bw, btnHeight, TRUE);
            
            MoveWindow(g_hBtns[4], margin, height - Scale(40, dpi), Scale(120, dpi), btnHeight, TRUE);
            MoveWindow(g_hBtns[5], width - margin - Scale(100, dpi), height - Scale(40, dpi), Scale(100, dpi), btnHeight, TRUE);
            break;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == ID_BTN_REFRESH) RefreshList();
            else if (id == ID_BTN_START || id == ID_BTN_STOP || id == ID_BTN_UNREGISTER) {
                int sel = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
                if (sel != -1) {
                    if (id == ID_BTN_START) RunAction(L"start", g_distros[sel].name);
                    else if (id == ID_BTN_STOP) RunAction(L"stop", g_distros[sel].name);
                    else {
                        if (MessageBox(hwnd, L"确定要注销(卸载)吗？此操作不可逆！", L"警告", MB_YESNO | MB_ICONWARNING) == IDYES)
                            RunAction(L"unregister", g_distros[sel].name);
                    }
                } else {
                    MessageBox(hwnd, L"请先选择一个分发版", L"提示", MB_OK | MB_ICONINFORMATION);
                }
            } else if (id == ID_BTN_INSTALL_LIST) ShowOnlineInstallWindow(hwnd);
            else if (id == ID_BTN_SHUTDOWN) RunAction(L"shutdown", NULL);
            break;
        }
        case WM_DESTROY: PostQuitMessage(0); break;
        default: return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR cmd, int show) {
    g_hInstance = hInst;
    const wchar_t NAME[] = L"WSLManagerGUI";
    WNDCLASS wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hInst; wc.lpszClassName = NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);
    // Get system DPI for initial window size
    HDC hdc = GetDC(NULL);
    UINT screenDpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);

    HWND hwnd = CreateWindowEx(0, NAME, L"WSL 管理工具 (GUI)", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, Scale(450, screenDpi), Scale(400, screenDpi), NULL, NULL, hInst, NULL);
    if (!hwnd) return 0;
    ShowWindow(hwnd, show);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}
