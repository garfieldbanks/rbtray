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

#include "Tray.h"

#include "RBTray.h"
#include "TrayIcon.h"

#include <Windows.h>
#include <map>
#include <memory>

typedef std::map<HWND, std::unique_ptr<TrayIcon>> TrayIconMap;
static TrayIconMap trayIconMap_;

static HICON GetWindowIcon(HWND hwnd)
{
    HICON icon;
    if (icon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_SMALL, 0)) {
        return icon;
    }
    if (icon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_BIG, 0)) {
        return icon;
    }
    if (icon = (HICON)GetClassLongPtr(hwnd, GCLP_HICONSM)) {
        return icon;
    }
    if (icon = (HICON)GetClassLongPtr(hwnd, GCLP_HICON)) {
        return icon;
    }
    return LoadIcon(NULL, IDI_WINLOGO);
}

static bool AddWindowToTray(HWND hwnd, HWND messageWnd)
{
    TrayIconMap::iterator it = trayIconMap_.find(hwnd);
    if (it != trayIconMap_.end()) {
        return false;
    }
    trayIconMap_[hwnd].reset(new TrayIcon());
    return trayIconMap_[hwnd]->create(messageWnd, WM_TRAYCMD, GetWindowIcon(hwnd));
}

static bool RemoveWindowFromTray(HWND hwnd)
{
    TrayIconMap::iterator it = trayIconMap_.find(hwnd);
    if (it == trayIconMap_.end()) {
        return false;
    }
    trayIconMap_.erase(it);
    return true;
}

void MinimizeWindowToTray(HWND hwnd, HWND messageWnd)
{
    // Don't minimize MDI child windows
    if ((UINT)GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_MDICHILD) {
        return;
    }

    // If hwnd is a child window, find parent window (e.g. minimize button in
    // Office 2007 (ribbon interface) is in a child window)
    if ((UINT)GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CHILD) {
        hwnd = GetAncestor(hwnd, GA_ROOT);
    }

    // Hide window before AddWindowToTray call because sometimes RefreshWindowInTray
    // can be called from inside ShowWindow before program window is actually hidden
    // and as a result RemoveWindowFromTray is called which immediately removes just
    // added tray icon.
    ShowWindow(hwnd, SW_HIDE);

    // Add icon to tray if it's not already there
    if (trayIconMap_.find(hwnd) == trayIconMap_.end()) {
        if (!AddWindowToTray(hwnd, messageWnd)) {
            // If there is something wrong with tray icon restore program window.
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
            return;
        }
    }
}

void RestoreWindowFromTray(HWND hwnd)
{
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    RemoveWindowFromTray(hwnd);
}

void CloseWindowFromTray(HWND hwnd)
{
    // Use PostMessage to avoid blocking if the program brings up a dialog on exit.
    // Also, Explorer windows ignore WM_CLOSE messages from SendMessage.
    PostMessage(hwnd, WM_CLOSE, 0, 0);

    Sleep(50);
    if (IsWindow(hwnd)) {
        Sleep(50);
    }

    if (!IsWindow(hwnd)) {
        // Closed successfully
        RemoveWindowFromTray(hwnd);
    }
}

void RefreshWindowInTray(HWND hwnd)
{
    TrayIconMap::iterator it = trayIconMap_.find(hwnd);
    if (it == trayIconMap_.end()) {
        return;
    }

    if (!IsWindow(hwnd) || IsWindowVisible(hwnd)) {
        RemoveWindowFromTray(hwnd);
    } else {
        it->second->refresh();
    }
}

void AddAllWindowsToTray()
{
    //for (int i = 0; i < MAXTRAYITEMS; i++) {
    //    if (hwndItems_[i]) {
    //        AddToTray(i);
    //    }
    //}
}

void RestoreAllWindowsFromTray()
{
    for (TrayIconMap::const_iterator cit = trayIconMap_.cbegin(); cit != trayIconMap_.cend(); ++cit) {
        HWND hwnd = cit->first;
        ShowWindow(hwnd, SW_SHOW);
        SetForegroundWindow(hwnd);
    }

    trayIconMap_.clear();
}

HWND GetWindowFromID(UINT id)
{
    for (TrayIconMap::const_iterator cit = trayIconMap_.cbegin(); cit != trayIconMap_.cend(); ++cit) {
        if (cit->second->id() == id) {
            return cit->first;
        }
    }

    return NULL;
}
