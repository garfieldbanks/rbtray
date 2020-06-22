// ****************************************************************************
//
// RBTray
// Copyright (C) 1998-2010  Nikolay Redko, J.D. Purcell
// Copyright (C) 2015 Benbuck Nason
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// ****************************************************************************

#include "RBTray.h"

#include "DebugPrint.h"
#include "Settings.h"
#include "Tray.h"
#include "TrayIcon.h"
#include "resource.h"

#include <Windows.h>

static UINT WM_TASKBAR_CREATED;

static HINSTANCE hInstance_;
static HWND hwnd_ = NULL;
static HMODULE hookDll_;
static Settings settings_;
static HWND hwndForMenu_;

static WCHAR * getExecutablePath();
static const char * readFile(const WCHAR * fileName);
static void showContextMenu(HWND hwnd);
static BOOL CALLBACK aboutDialogProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK mainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR szCmdLine, _In_ int iCmdShow);

WCHAR * getExecutablePath()
{
    WCHAR path[MAX_PATH];
    if (GetModuleFileNameW(NULL, path, MAX_PATH) <= 0) {
        DEBUG_PRINTF("GetModuleFileName() failed\n");
        return nullptr;
    }

    WCHAR * sep = wcsrchr(path, L'\\');
    if (!sep) {
        DEBUG_PRINTF("path has no separator\n");
        return nullptr;
    }

    *sep = L'\0';

    size_t pathChars = sep - path + 1;
    WCHAR * exePath = new WCHAR[pathChars];
    if (!exePath) {
        DEBUG_PRINTF("could not allocate %zu chars\n", pathChars);
        return nullptr;
    }

    wcsncpy_s(exePath, pathChars, path, pathChars - 1);
    return exePath;
}

const char * readFile(const WCHAR * fileName)
{
    char * contents = nullptr;

    HANDLE file = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        DEBUG_PRINTF("Could not open '%ls' for reading\n", fileName);
    } else {
        DWORD fileSize = GetFileSize(file, NULL);
        DWORD bufferSize = fileSize + 1;
        char * buffer = new char[bufferSize];
        buffer[bufferSize - 1] = '\0';
        if (!buffer) {
            DEBUG_PRINTF("Could not allocate buffer size %d for reading\n", bufferSize);
        } else {
            DWORD bytesRead = 0;
            if (!ReadFile(file, buffer, fileSize, &bytesRead, NULL)) {
                DEBUG_PRINTF("Could not read %d bytes from '%ls'\n", fileSize, fileName);
                delete[] buffer;
                buffer = nullptr;
            } else {
                if (bytesRead < fileSize) {
                    DEBUG_PRINTF("Only read %d bytes from '%ls', expected %d\n", bytesRead, fileName, fileSize);
                    delete[] buffer;
                    buffer = nullptr;
                } else {
                    contents = buffer;
                }
            }
        }

        CloseHandle(file);
    }

    return contents;
}

void showContextMenu(HWND hwnd)
{
    HMENU hMenu;
    POINT point;

    hMenu = CreatePopupMenu();
    if (!hMenu) {
        MessageBoxW(NULL, L"Error creating menu.", L"RBTray", MB_OK | MB_ICONERROR);
        return;
    }

    AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, L"About RBTray");
    AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"Exit RBTray");
    if (hwnd) {
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, IDM_CLOSE, L"Close Window");
        AppendMenuW(hMenu, MF_STRING, IDM_RESTORE, L"Restore Window");
    }

    GetCursorPos(&point);
    SetForegroundWindow(hwnd_);

    TrackPopupMenu(hMenu, TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RIGHTALIGN | TPM_BOTTOMALIGN, point.x, point.y, 0, hwnd_, NULL);

    PostMessage(hwnd_, WM_USER, 0, 0);
    DestroyMenu(hMenu);
}

BOOL CALLBACK aboutDialogProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg) {
        case WM_CLOSE: {
            PostMessage(hWnd, WM_COMMAND, IDCANCEL, 0);
            break;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDOK: {
                    EndDialog(hWnd, TRUE);
                    break;
                }

                case IDCANCEL: {
                    EndDialog(hWnd, FALSE);
                    break;
                }
            }
            break;
        }

        default: {
            return FALSE;
        }
    }
    return TRUE;
}

LRESULT CALLBACK mainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDM_RESTORE: {
                    RestoreWindowFromTray(hwndForMenu_);
                    break;
                }

                case IDM_CLOSE: {
                    CloseWindowFromTray(hwndForMenu_);
                    break;
                }

                case IDM_ABOUT: {
                    DialogBox(hInstance_, MAKEINTRESOURCE(IDD_ABOUT), hwnd_, (DLGPROC)aboutDialogProc);
                    break;
                }

                case IDM_EXIT: {
                    SendMessage(hwnd_, WM_DESTROY, 0, 0);
                    break;
                }
            }
            break;
        }

        case WM_ADDTRAY: {
            HWND hwnd = (HWND)lParam;

#if !defined(NDEBUG)
            WCHAR text[256];
            GetWindowTextW(hwnd, text, sizeof(text) / sizeof(text[0]));
            DEBUG_PRINTF("window text '%ls'\n", text);

            WCHAR className[256];
            GetClassNameW(hwnd, className, sizeof(className) / sizeof(className[0]));
            DEBUG_PRINTF("window class name '%ls'\n", className);
#endif

            MinimizeWindowToTray(hwnd, hwnd_);
            break;
        }

        case WM_REMTRAY: {
            HWND hwnd = (HWND)lParam;
            RestoreWindowFromTray(hwnd);
            break;
        }

        case WM_REFRTRAY: {
            HWND hwnd = (HWND)lParam;
            RefreshWindowInTray(hwnd);
            break;
        }

        case WM_TRAYCMD: {
            switch ((UINT)lParam) {
                case NIN_SELECT: {
                    HWND hwnd = GetWindowFromID(wParam);
                    RestoreWindowFromTray(hwnd);
                    break;
                }

                case WM_CONTEXTMENU: {
                    hwndForMenu_ = GetWindowFromID(wParam);
                    showContextMenu(hwndForMenu_);
                    break;
                }

                case WM_MOUSEMOVE: {
                    HWND hwnd = GetWindowFromID(wParam);
                    RefreshWindowInTray(hwnd);
                    break;
                }
            }
            break;
        }

        case WM_WNDCREATED: {
            DEBUG_PRINTF("window created\n");
            if (settings_.autotray_) {
                HWND hwnd = (HWND)lParam;
                if (hwnd != hwnd_) {
#if !defined(NDEBUG)
                    WCHAR text[256];
                    GetWindowTextW(hwnd, text, sizeof(text) / sizeof(text[0]));
                    DEBUG_PRINTF("window text '%ls'\n", text);
#endif

                    WCHAR className[256];
                    GetClassNameW(hwnd, className, sizeof(className) / sizeof(className[0]));
                    DEBUG_PRINTF("window class name '%ls'\n", className);

                    for (size_t i = 0; i < settings_.autotraySize_; ++i) {
                        const Settings::Autotray & autotray = settings_.autotray_[i];
                        if (autotray.className_) {
                            DEBUG_PRINTF("comparing '%ls' to '%ls'\n", className, autotray.className_);
                            if (wcsstr(className, autotray.className_)) {
                                DEBUG_PRINTF("auto-traying window '%ls'\n", className);
                                Sleep(1000); // FIX - terrible hack
                                MinimizeWindowToTray(hwnd, hwnd_);
                                break;
                            }
                        }
                    }
                }
            }
            break;
        }

        case WM_HOTKEY: {
            HWND hwnd = GetForegroundWindow();
            if (!hwnd) {
                break;
            }

            LONG style = GetWindowLong(hwnd, GWL_STYLE);
            if (!(style & WS_MINIMIZEBOX)) {
                // skip, no minimize box
                break;
            }

#if !defined(NDEBUG)
            WCHAR text[256];
            GetWindowTextW(hwnd, text, sizeof(text) / sizeof(text[0]));
            DEBUG_PRINTF("window text '%ls'\n", text);

            WCHAR className[256];
            GetClassNameW(hwnd, className, sizeof(className) / sizeof(className[0]));
            DEBUG_PRINTF("window class name '%ls'\n", className);
#endif

            MinimizeWindowToTray(hwnd, hwnd_);

            break;
        }

        case WM_DESTROY: {
            RestoreAllWindowsFromTray();

            if (hookDll_) {
                UnRegisterHook();
                FreeLibrary(hookDll_);
            }

            PostQuitMessage(0);
            break;
        }

        default: {
            if (msg == WM_TASKBAR_CREATED) {
                AddAllWindowsToTray();
            }
            break;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*szCmdLine*/, _In_ int /*iCmdShow*/)
{
    hInstance_ = hInstance;

    WCHAR * exePath = getExecutablePath();

    // get settings from json file
    const WCHAR settingsFileName[] = L"RBTray.json";
    const char * json = readFile(settingsFileName);
    if (!json) {
        // no settings file in current directory, try in executable dir
        WCHAR fileName[MAX_PATH];
        _snwprintf_s(fileName, MAX_PATH, L"%s\\%s", exePath, settingsFileName);
        json = readFile(fileName);
    }
    if (json) {
        settings_.parseJson(json);
        delete[] json;
    }

    // get settings from command line (can override json file)
    settings_.parseCommandLine();

    // check for existing RBTray window
    hwnd_ = FindWindowW(NAMEW, NAMEW);
    if (hwnd_) {
        if (settings_.shouldExit_) {
            SendMessage(hwnd_, WM_CLOSE, 0, 0);
        } else {
            MessageBoxW(NULL, L"RBTray is already running.", L"RBTray", MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }

    // load hook dll if enabled
    if (settings_.useHook_) {
        WCHAR hookDllPath[MAX_PATH];
        _snwprintf_s(hookDllPath, MAX_PATH, L"%s\\%s", exePath, L"RBHook.dll");
        if (!(hookDll_ = LoadLibraryW(hookDllPath))) {
            DEBUG_PRINTF("error loading RBHook.dll\n");
        } else {
            if (!RegisterHook(hookDll_)) {
                DEBUG_PRINTF("error setting hook procedure\n");
            }
        }
    }

    delete[] exePath;
    exePath = nullptr;

    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = mainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance_, MAKEINTRESOURCE(IDI_RBTRAY));
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = NAME;
    wc.hIconSm = LoadIcon(hInstance_, MAKEINTRESOURCE(IDI_RBTRAY));
    if (!RegisterClassEx(&wc)) {
        MessageBoxW(NULL, L"Error creating window class", L"RBTray", MB_OK | MB_ICONERROR);
        return 0;
    }

    if (!(hwnd_ = CreateWindowW(NAMEW, NAMEW, WS_OVERLAPPED, 0, 0, 0, 0, (HWND)NULL, (HMENU)NULL, (HINSTANCE)hInstance, (LPVOID)NULL))) {
        MessageBoxW(NULL, L"Error creating window", L"RBTray", MB_OK | MB_ICONERROR);
        return 0;
    }

    WM_TASKBAR_CREATED = RegisterWindowMessageW(L"TaskbarCreated");

    BOOL registeredHotKey = RegisterHotKey(hwnd_, 0, MOD_WIN | MOD_ALT, VK_DOWN);
    if (!registeredHotKey) {
        MessageBoxW(NULL, L"Couldn't register hotkey", L"RBTray", MB_OK | MB_ICONERROR);
    }

    TrayIcon trayIcon;
    if (settings_.trayIcon_) {
        if (!trayIcon.create(hwnd_, WM_TRAYCMD, LoadIcon(hInstance_, MAKEINTRESOURCE(IDI_RBTRAY)))) {
            MessageBoxW(NULL, L"Couldn't create tray icon", L"RBTray", MB_OK | MB_ICONERROR);
        }
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (registeredHotKey) {
        UnregisterHotKey(hwnd_, 0);
    }

    if (settings_.trayIcon_) {
        trayIcon.destroy();
    }

    return (int)msg.wParam;
}
