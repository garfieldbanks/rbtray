# ****************************************************************************
#
# RBTray
# Copyright (C) 1998-2010  Nikolay Redko, J.D. Purcell
# Copyright (C) 2015 Benbuck Nason
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# ****************************************************************************

cmake_minimum_required(VERSION 3.17)

project(
    RBTray
    VERSION 4.11
)

add_library(
    RBHook
    SHARED
    src/RBHook.cpp
)

add_executable(
    RBTray
    WIN32
    src/cJSON.c
    src/cJSON.h
    src/DebugPrint.h
    src/RBTray.cpp
    src/RBTray.h
    src/RBTray.ico
    src/RBTray.rc
    src/resource.h
    src/Settings.cpp
    src/Settings.h
    src/Tray.cpp
    src/Tray.h
    src/TrayIcon.cpp
    src/TrayIcon.h
)

target_link_libraries(RBTray RBHook)

install(
    TARGETS RBTray
    RUNTIME DESTINATION . COMPONENT Runtime
)

install(
    TARGETS RBHook
    RUNTIME DESTINATION . COMPONENT Runtime
)

set(CPACK_GENERATOR "NSIS64")
set(CPACK_NSIS_INSTALLED_ICON_NAME "RBTray.exe")
set(CPACK_NSIS_HELP_LINK "https://github.com/benbuck/rbtray/")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "RBTray")
set(CPACK_PACKAGE_VENDOR "Benbuck Nason")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
include(CPack)

if(CMAKE_GENERATOR MATCHES "^Visual Studio.*")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT RBTray)
endif()
