/*
	MIT License

	Copyright (c) 2020 Oleksiy Ryabchun

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#include "stdafx.h"
#include "intrin.h"
#include "Config.h"
#include "Resource.h"

ConfigItems config;

namespace Config
{
	VOID __fastcall Load()
	{
		HMODULE hModule = GetModuleHandle(NULL);

		GetModuleFileName(hModule, config.file, MAX_PATH - 1);
		CHAR* p = StrLastChar(config.file, '.');
		*p = NULL;
		StrCopy(p, ".ini");

		FILE* file = FileOpen(config.file, "rb");
		if (file)
		{
			config.isExist = TRUE;
			FileClose(file);
		}

		config.cursor = LoadCursor(NULL, IDC_ARROW);
		config.menu = LoadMenu(hDllModule, MAKEINTRESOURCE(IDR_MENU));
		config.icon = LoadIcon(hModule, MAKEINTRESOURCE(RESOURCE_ICON));
		config.font = (HFONT)CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
			OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, TEXT("MS Shell Dlg"));
		config.msgMenu = RegisterWindowMessage(WM_CHECK_MENU);

		config.singleThread = TRUE;
		DWORD processMask, systemMask;
		HANDLE process = GetCurrentProcess();
		if (GetProcessAffinityMask(process, &processMask, &systemMask))
		{
			if (processMask != systemMask && SetProcessAffinityMask(process, systemMask))
				GetProcessAffinityMask(process, &processMask, &systemMask);

			BOOL isSingle = FALSE;
			DWORD count = sizeof(DWORD) << 3;
			do
			{
				if (processMask & 1)
				{
					if (isSingle)
					{
						config.singleThread = FALSE;
						break;
					}
					else
						isSingle = TRUE;
				}

				processMask >>= 1;
			} while (--count);
		}

		if (!config.isExist)
		{
			config.windowedMode = TRUE;
			Config::Set(CONFIG, "WindowedMode", config.windowedMode);

			config.vSync = TRUE;
			Config::Set(CONFIG, "VSync", config.vSync);

			config.filtering = TRUE;
			Config::Set(CONFIG, "Filtering", config.filtering);

			config.aspectRatio = TRUE;
			Config::Set(CONFIG, "AspectRatio", config.aspectRatio);

			config.mouseCapture = TRUE;
			Config::Set(CONFIG, "MouseCapture", config.mouseCapture);

			config.updateMode = UpdateSSE;
			Config::Set(CONFIG, "UpdateMode", (INT)config.updateMode);
		}
		else
		{
			config.windowedMode = (BOOL)Config::Get(CONFIG, "WindowedMode", TRUE);
			config.vSync = (BOOL)Config::Get(CONFIG, "VSync", TRUE);
			config.filtering = (BOOL)Config::Get(CONFIG, "Filtering", TRUE);
			config.aspectRatio = (BOOL)Config::Get(CONFIG, "AspectRatio", TRUE);
			config.mouseCapture = (BOOL)Config::Get(CONFIG, "MouseCapture", TRUE);

			config.updateMode = (UpdateMode)Config::Get(CONFIG, "UpdateMode", UpdateSSE);
			if (config.updateMode < UpdateNone || config.updateMode > UpdateASM)
				config.updateMode = UpdateSSE;
		}

		if (config.updateMode == UpdateSSE)
		{
			INT cpuinfo[4];
			__cpuid(cpuinfo, 1);
			BOOL isSSE2 = cpuinfo[3] & (1 << 26) || FALSE;
			if (!isSSE2)
				config.updateMode = UpdateCPP;
		}

		HMODULE hLibrary = LoadLibrary("NTDLL.dll");
		if (hLibrary)
		{
			if (GetProcAddress(hLibrary, "wine_get_version"))
				config.singleWindow = TRUE;
			FreeLibrary(hLibrary);
		}
	}

	INT __fastcall Get(const CHAR* app, const CHAR* key, INT default)
	{
		return GetPrivateProfileInt(app, key, (INT)default, config.file);
	}

	DWORD __fastcall Get(const CHAR* app, const CHAR* key, CHAR* default, CHAR* returnString, DWORD nSize)
	{
		return GetPrivateProfileString(app, key, default, returnString, nSize, config.file);
	}

	BOOL __fastcall Set(const CHAR* app, const CHAR* key, INT value)
	{
		CHAR res[20];
		StrPrint(res, "%d", value);
		return WritePrivateProfileString(app, key, res, config.file);
	}

	BOOL __fastcall Set(const CHAR* app, const CHAR* key, CHAR* value)
	{
		return WritePrivateProfileString(app, key, value, config.file);
	}
}