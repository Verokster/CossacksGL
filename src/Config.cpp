/*
	MIT License

	Copyright (c) 2019 Oleksiy Ryabchun

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
#include "Config.h"
#include "Resource.h"

ConfigItems config;

namespace Config
{
	VOID __fastcall Load(HMODULE hModule)
	{
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

		config.singleThread = TRUE;
		DWORD processMask, systemMask;
		if (GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask))
		{
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

			config.vSync = FALSE;
			Config::Set(CONFIG, "VSync", config.vSync);

			config.fpsCounter = FALSE;
			Config::Set(CONFIG, "FpsCounter", config.fpsCounter);

			config.filtering = TRUE;
			Config::Set(CONFIG, "Filtering", config.filtering);

			config.aspectRatio = TRUE;
			Config::Set(CONFIG, "AspectRatio", config.aspectRatio);

			config.mouseCapture = TRUE;
			Config::Set(CONFIG, "MouseCapture", config.mouseCapture);
		}
		else
		{
			config.windowedMode = (BOOL)Config::Get(CONFIG, "WindowedMode", TRUE);
			config.vSync = (BOOL)Config::Get(CONFIG, "VSync", FALSE);
			config.fpsCounter = (BOOL)Config::Get(CONFIG, "FpsCounter", FALSE);
			config.filtering = (BOOL)Config::Get(CONFIG, "Filtering", TRUE);
			config.aspectRatio = (BOOL)Config::Get(CONFIG, "AspectRatio", TRUE);
			config.mouseCapture = (BOOL)Config::Get(CONFIG, "MouseCapture", TRUE);
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