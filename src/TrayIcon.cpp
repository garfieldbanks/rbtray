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

#include "TrayIcon.h"

#include "DebugPrint.h"

static volatile LONG id_;

TrayIcon::TrayIcon() { ZeroMemory(&nid_, sizeof(nid_)); }
TrayIcon::~TrayIcon() { destroy(); }

bool TrayIcon::create(HWND hwnd, UINT msg, HICON icon)
{
    if (nid_.uID) {
        DEBUG_PRINTF("attempt to re-create tray icon\n");
        return false;
    }

    LONG id = InterlockedIncrement(&id_);

    ZeroMemory(&nid_, sizeof(nid_));
    nid_.cbSize = NOTIFYICONDATA_V2_SIZE;
    nid_.hWnd = hwnd;
    nid_.uID = (UINT)id;
    nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid_.uCallbackMessage = msg;
    nid_.hIcon = icon;
    GetWindowText(hwnd, nid_.szTip, sizeof(nid_.szTip) / sizeof(nid_.szTip[0]));
    nid_.uVersion = NOTIFYICON_VERSION;

    if (!Shell_NotifyIcon(NIM_ADD, &nid_)) {
        DEBUG_PRINTF("Shell_NotifyIcon(NIM_ADD) failed\n");
        ZeroMemory(&nid_, sizeof(nid_));
        return false;
    }

    if (!Shell_NotifyIcon(NIM_SETVERSION, &nid_)) {
        DEBUG_PRINTF("Shell_NotifyIcon(NIM_SETVERSION) failed\n");
        destroy();
        return false;
    }

    return true;
}

void TrayIcon::destroy()
{
    if (nid_.uID) {
        if (!Shell_NotifyIcon(NIM_DELETE, &nid_)) {
            DEBUG_PRINTF("Shell_NotifyIcon(NIM_DELETE) failed\n");
        }

        ZeroMemory(&nid_, sizeof(nid_));
    }
}

void TrayIcon::refresh()
{
    // nid_.uFlags = NIF_TIP;
    // GetWindowText(nid_.hWnd, nid_.szTip, sizeof(nid_.szTip) / sizeof(nid_.szTip[0]));
    // Shell_NotifyIcon(NIM_MODIFY, &nid_);
}
