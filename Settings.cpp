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

#include "Settings.h"

#include "DebugPrint.h"
#include "cJSON.h"

Settings::Settings() : shouldExit_(false), useHook_(true), autotray_(nullptr), autotraySize_(0) {}

Settings::~Settings()
{
    if (autotray_) {
        for (size_t i = 0; i < autotraySize_; ++i) {
            free(autotray_[i].className_);
        }

        free(autotray_);
    }
}

void Settings::parseCommandLine()
{
    int argc;
    LPWSTR * argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    for (int a = 0; a < argc; ++a) {
        if (!wcscmp(argv[a], L"--exit")) {
            shouldExit_ = true;
        }

        if (!wcscmp(argv[a], L"--no-hook")) {
            useHook_ = false;
        }
    }
}

void Settings::parseJson(const char * json)
{
    const cJSON * cjson = cJSON_Parse(json);
    DEBUG_PRINTF("Parsed settings JSON: %s\n", cJSON_Print(cjson));

    const cJSON * hook = cJSON_GetObjectItemCaseSensitive(cjson, "hook");
    if (hook) {
        if (!cJSON_IsBool(hook)) {
            DEBUG_PRINTF("bad type for '%s'\n", hook->string);
        } else {
            useHook_ = cJSON_IsTrue(hook) ? true : false;
        }
    }

    const cJSON * autotray = cJSON_GetObjectItemCaseSensitive(cjson, "autotray");
    if (autotray) {
        if (!cJSON_IsArray(autotray)) {
            DEBUG_PRINTF("bad type for '%s'\n", autotray->string);
        } else {
            int arrSize = cJSON_GetArraySize(autotray);
            for (int i = 0; i < arrSize; ++i) {
                const cJSON * item = cJSON_GetArrayItem(autotray, i);
                if (!cJSON_IsObject(item)) {
                    DEBUG_PRINTF("bad type for '%s'\n", item->string);
                } else {
                    cJSON * classname = cJSON_GetObjectItemCaseSensitive(item, "classname");
                    if (!classname) {
                        DEBUG_PRINTF("missing classname for '%s'\n", item->string);
                    } else {
                        const char * classnameStr = cJSON_GetStringValue(classname);
                        if (!classnameStr) {
                            DEBUG_PRINTF("bad type for '%s'\n", classname->string);
                        } else {
                            addAutotray(classnameStr);
                        }
                    }
                }
            }
        }
    }
}

void Settings::addAutotray(const char * className)
{
    size_t newAutotraySize = autotraySize_ + 1;
    autotray_ = (Autotray *)realloc(autotray_, sizeof(Autotray) * newAutotraySize);
    if (!autotray_) {
        DEBUG_PRINTF("could not allocate %zu window specs\n", newAutotraySize);
        autotray_ = nullptr;
        autotraySize_ = 0;
        return;
    }

    Autotray & autotray = autotray_[autotraySize_];
    autotraySize_ = newAutotraySize;

    size_t len = strlen(className) + 1;
    autotray.className_ = (WCHAR *)malloc(sizeof(WCHAR) * len);
    if (!autotray.className_) {
        DEBUG_PRINTF("could not allocate %zu wchars\n", len);
    } else {
        if (MultiByteToWideChar(CP_UTF8, 0, className, (int)len, autotray.className_, (int)len) <= 0) {
            DEBUG_PRINTF("could not convert class name to wide chars\n");
            free(autotray.className_);
            autotray.className_ = nullptr;
        }
    }
}
