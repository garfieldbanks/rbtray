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

#include "Tray.h"
#include "resource.h"

#include <windows.h>

static UINT WM_TASKBAR_CREATED;

HWND _hwnd = NULL;

static HINSTANCE _hInstance;
static HMODULE _hLib;
static HWND _hwndForMenu;

void ExecuteMenu()
{
    HMENU hMenu;
    POINT point;

    hMenu = CreatePopupMenu();
    if (!hMenu) {
        MessageBox(NULL, L"Error creating menu.", L"RBTray", MB_OK | MB_ICONERROR);
        return;
    }
    AppendMenu(hMenu, MF_STRING, IDM_ABOUT, L"About RBTray");
    AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"Exit RBTray");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL); //--------------
    AppendMenu(hMenu, MF_STRING, IDM_CLOSE, L"Close Window");
    AppendMenu(hMenu, MF_STRING, IDM_RESTORE, L"Restore Window");

    GetCursorPos(&point);
    SetForegroundWindow(_hwnd);

    TrackPopupMenu(hMenu, TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RIGHTALIGN | TPM_BOTTOMALIGN, point.x, point.y, 0, _hwnd, NULL);

    PostMessage(_hwnd, WM_USER, 0, 0);
    DestroyMenu(hMenu);
}

BOOL CALLBACK AboutDlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
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

LRESULT CALLBACK HookWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDM_RESTORE: {
                    RestoreWindowFromTray(_hwndForMenu);
                    break;
                }
                case IDM_CLOSE: {
                    CloseWindowFromTray(_hwndForMenu);
                    break;
                }
                case IDM_ABOUT: {
                    DialogBox(_hInstance, MAKEINTRESOURCE(IDD_ABOUT), _hwnd, (DLGPROC)AboutDlgProc);
                    break;
                }
                case IDM_EXIT: {
                    SendMessage(_hwnd, WM_DESTROY, 0, 0);
                    break;
                }
            }
            break;
        }
        case WM_ADDTRAY: {
            MinimizeWindowToTray((HWND)lParam);
            break;
        }
        case WM_REMTRAY: {
            RestoreWindowFromTray((HWND)lParam);
            break;
        }
        case WM_REFRTRAY: {
            RefreshWindowInTray((HWND)lParam);
            break;
        }
        case WM_TRAYCMD: {
            switch ((UINT)lParam) {
                case NIN_SELECT: {
                    RestoreWindowFromTray(_hwndItems[wParam]);
                    break;
                }
                case WM_CONTEXTMENU: {
                    _hwndForMenu = _hwndItems[wParam];
                    ExecuteMenu();
                    break;
                }
                case WM_MOUSEMOVE: {
                    RefreshWindowInTray(_hwndItems[wParam]);
                    break;
                }
            }
            break;
        }
        case WM_HOTKEY: {
            HWND fgWnd = GetForegroundWindow();
            if (!fgWnd) {
                break;
            }

            LONG style = GetWindowLong(fgWnd, GWL_STYLE);
            if (!(style & WS_MINIMIZEBOX)) {
                // skip, no minimize box
                break;
            }

            MinimizeWindowToTray(fgWnd);

            break;
        }
        case WM_DESTROY: {
            for (int i = 0; i < MAXTRAYITEMS; i++) {
                if (_hwndItems[i]) {
                    RestoreWindowFromTray(_hwndItems[i]);
                }
            }
            if (_hLib) {
                UnRegisterHook();
                FreeLibrary(_hLib);
            }
            PostQuitMessage(0);
            break;
        }
        default: {
            if (msg == WM_TASKBAR_CREATED) {
                for (int i = 0; i < MAXTRAYITEMS; i++) {
                    if (_hwndItems[i]) {
                        AddToTray(i);
                    }
                }
            }
            break;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*szCmdLine*/, _In_ int /*iCmdShow*/)
{
    _hInstance = hInstance;

    int argc;
    LPWSTR * argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool shouldExit = false;
    bool useHook = true;
    for (int a = 0; a < argc; ++a) {
        if (!wcscmp(argv[a], L"--exit")) {
            shouldExit = true;
        }
        if (!wcscmp(argv[a], L"--no-hook")) {
            useHook = false;
        }
    }

    _hwnd = FindWindow(NAME, NAME);
    if (_hwnd) {
        if (shouldExit) {
            SendMessage(_hwnd, WM_CLOSE, 0, 0);
        } else {
            MessageBox(NULL, L"RBTray is already running.", L"RBTray", MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }

    if (useHook) {
        if (!(_hLib = LoadLibrary(L"RBHook.dll"))) {
            // MessageBox(NULL, L"Error loading RBHook.dll.", L"RBTray", MB_OK | MB_ICONERROR);
            // return 0;
        } else {
            if (!RegisterHook(_hLib)) {
                MessageBox(NULL, L"Error setting hook procedure.", L"RBTray", MB_OK | MB_ICONERROR);
                return 0;
            }
        }
    }

    WNDCLASS wc;
    wc.style = 0;
    wc.lpfnWndProc = HookWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = NAME;
    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Error creating window class", L"RBTray", MB_OK | MB_ICONERROR);
        return 0;
    }

    if (!(_hwnd = CreateWindow(NAME, NAME, WS_OVERLAPPED, 0, 0, 0, 0, (HWND)NULL, (HMENU)NULL, (HINSTANCE)hInstance, (LPVOID)NULL))) {
        MessageBox(NULL, L"Error creating window", L"RBTray", MB_OK | MB_ICONERROR);
        return 0;
    }

    for (int i = 0; i < MAXTRAYITEMS; i++) {
        _hwndItems[i] = NULL;
    }

    WM_TASKBAR_CREATED = RegisterWindowMessage(L"TaskbarCreated");

    BOOL registeredHotKey = RegisterHotKey(_hwnd, 0, MOD_WIN | MOD_ALT, VK_DOWN);
    if (!registeredHotKey) {
        MessageBox(NULL, L"Couldn't register hotkey", L"RBTray", MB_OK | MB_ICONERROR);
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (registeredHotKey) {
        UnregisterHotKey(_hwnd, 0);
    }

    return (int)msg.wParam;
}
