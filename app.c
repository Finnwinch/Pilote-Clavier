#include <windows.h>
#include <stdio.h>
#include <wchar.h>

#define IDI_ICON1 101
#define ID_BUTTON_START  1
#define ID_BUTTON_STOP   2
#define ID_EDIT_KEY      3
#define ID_EDIT_RATE     4
#define ID_EDIT_WINDOW   5
#define ID_EDIT_HOLD     6

HWND hwndTarget = NULL;
HANDLE hThread = NULL;
BOOL running = FALSE;
WCHAR keyChar = L' ';
int delayMs = 2000;
int holdMs = 100;
WCHAR targetWindowTitle[256] = L"";

DWORD WINAPI SendKeyThread(LPVOID lpParam) {
    while (running) {
        if (hwndTarget) {
            SendMessage(hwndTarget, WM_KEYDOWN, (WPARAM)keyChar, 0);
            Sleep(holdMs);
            SendMessage(hwndTarget, WM_KEYUP, (WPARAM)keyChar, 0);
        }
        Sleep(delayMs);
    }
    return 0;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    WCHAR title[256];
    GetWindowTextW(hwnd, title, sizeof(title) / sizeof(WCHAR));
    if (wcsstr(title, targetWindowTitle)) {
        HWND* target = (HWND*)lParam;
        *target = hwnd;
        return FALSE;
    }
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hStart, hStop, hEditKey, hEditRate, hEditWindow, hEditHold, hStatus;

    switch (msg) {
        case WM_CREATE: {
            HFONT hTitleFont = CreateFontW(
                24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");

            HWND hTitle = CreateWindowW(L"static", L"Pilote Clavier",
                WS_VISIBLE | WS_CHILD | SS_CENTER,
                10, 10, 260, 40, hwnd, NULL, NULL, NULL);
            SendMessageW(hTitle, WM_SETFONT, (WPARAM)hTitleFont, TRUE);

            CreateWindowW(L"static", L"Fenêtre cible :", WS_VISIBLE | WS_CHILD,
                          10, 60, 90, 20, hwnd, NULL, NULL, NULL);
            hEditWindow = CreateWindowW(L"edit", L"",
                          WS_VISIBLE | WS_CHILD | WS_BORDER,
                          110, 60, 160, 20, hwnd, (HMENU)ID_EDIT_WINDOW, NULL, NULL);

            CreateWindowW(L"static", L"Touche :", WS_VISIBLE | WS_CHILD,
                          10, 90, 90, 20, hwnd, NULL, NULL, NULL);
            hEditKey = CreateWindowW(L"edit", L" ",
                          WS_VISIBLE | WS_CHILD | WS_BORDER,
                          110, 90, 30, 20, hwnd, (HMENU)ID_EDIT_KEY, NULL, NULL);

            CreateWindowW(L"static", L"Délai (ms) :", WS_VISIBLE | WS_CHILD,
                          150, 90, 70, 20, hwnd, NULL, NULL, NULL);
            hEditRate = CreateWindowW(L"edit", L"2000",
                          WS_VISIBLE | WS_CHILD | WS_BORDER,
                          220, 90, 50, 20, hwnd, (HMENU)ID_EDIT_RATE, NULL, NULL);

            CreateWindowW(L"static", L"Hold (ms) :", WS_VISIBLE | WS_CHILD,
                          10, 120, 90, 20, hwnd, NULL, NULL, NULL);
            hEditHold = CreateWindowW(L"edit", L"100",
                          WS_VISIBLE | WS_CHILD | WS_BORDER,
                          110, 120, 50, 20, hwnd, (HMENU)ID_EDIT_HOLD, NULL, NULL);

            hStart = CreateWindowW(L"button", L"Start",
                          WS_VISIBLE | WS_CHILD,
                          180, 120, 45, 25, hwnd, (HMENU)ID_BUTTON_START, NULL, NULL);
            hStop = CreateWindowW(L"button", L"Stop",
                          WS_VISIBLE | WS_CHILD,
                          230, 120, 45, 25, hwnd, (HMENU)ID_BUTTON_STOP, NULL, NULL);

            hStatus = CreateWindowW(L"static", L"Arrêté",
                          WS_VISIBLE | WS_CHILD | SS_CENTER,
                          10, 150, 260, 20, hwnd, NULL, NULL, NULL);

            EnableWindow(hStop, FALSE);
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rect;
            GetClientRect(hwnd, &rect);
            HBRUSH brush = CreateSolidBrush(RGB(240, 248, 255));
            FillRect(hdc, &rect, brush);
            DeleteObject(brush);

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BUTTON_START) {
                WCHAR buf[8];
                GetWindowTextW(hEditKey, buf, sizeof(buf) / sizeof(WCHAR));
                keyChar = buf[0];

                WCHAR rateBuf[16];
                GetWindowTextW(hEditRate, rateBuf, sizeof(rateBuf) / sizeof(WCHAR));
                delayMs = _wtoi(rateBuf);
                if (delayMs <= 0) delayMs = 2000;

                WCHAR holdBuf[16];
                GetWindowTextW(hEditHold, holdBuf, sizeof(holdBuf) / sizeof(WCHAR));
                holdMs = _wtoi(holdBuf);
                if (holdMs < 0) holdMs = 100;

                GetWindowTextW(hEditWindow, targetWindowTitle, sizeof(targetWindowTitle) / sizeof(WCHAR));

                hwndTarget = NULL;
                EnumWindows(EnumWindowsProc, (LPARAM)&hwndTarget);
                if (!hwndTarget) {
                    MessageBoxW(hwnd, L"Fenêtre cible non trouvée", L"Erreur", MB_OK | MB_ICONERROR);
                    break;
                }

                if (!running) {
                    running = TRUE;
                    hThread = CreateThread(NULL, 0, SendKeyThread, NULL, 0, NULL);
                    EnableWindow(hStart, FALSE);
                    EnableWindow(hStop, TRUE);
                    SetWindowTextW(hStatus, L"En cours...");
                }
            }
            if (LOWORD(wParam) == ID_BUTTON_STOP) {
                running = FALSE;
                if (hThread) {
                    WaitForSingleObject(hThread, INFINITE);
                    CloseHandle(hThread);
                    hThread = NULL;
                }
                EnableWindow(hStart, TRUE);
                EnableWindow(hStop, FALSE);
                SetWindowTextW(hStatus, L"Arrêté");
            }
            break;
        case WM_DESTROY:
            running = FALSE;
            if (hThread) {
                WaitForSingleObject(hThread, INFINITE);
                CloseHandle(hThread);
            }
            PostQuitMessage(0);
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"AutoKeySender";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = hIcon;

    RegisterClassW(&wc);

    int width = 300;
    int height = 200;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;

    HWND hwnd = CreateWindowW(L"AutoKeySender", L"Pilote Clavier",
                             WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX,
                             x, y, width, height,
                             NULL, NULL, hInstance, NULL);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}

