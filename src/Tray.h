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

#pragma once

#include <Windows.h>

void MinimizeWindowToTray(HWND hwnd, HWND messageWnd);
void RestoreWindowFromTray(HWND hwnd);
void CloseWindowFromTray(HWND hwnd);
void RefreshWindowInTray(HWND hwnd);
void AddAllWindowsToTray();
void RestoreAllWindowsFromTray();
HWND GetWindowFromID(UINT id);
