/*
	MIT License

	Copyright (c) 2018 Oleksiy Ryabchun

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
#include "Hooks.h"

DWORD libCount;
HMODULE* modules;

namespace Hooks
{
	VOID __fastcall Load(HMODULE hModule)
	{
		TCHAR* tPath;
		WIN32_FIND_DATA search_data;
		HANDLE handle;

		TCHAR searchPath[MAX_PATH];

		TCHAR path[MAX_PATH];
		GetModuleFileName(hModule, path, MAX_PATH);

		tPath = strrchr(path, '\\');
		++tPath;
		*tPath = NULL;

		strcpy(searchPath, path);
		strcat(searchPath, "*.gsp");

		memset(&search_data, 0, sizeof(WIN32_FIND_DATA));
		handle = FindFirstFile(searchPath, &search_data);
		{
			while (handle != INVALID_HANDLE_VALUE)
			{
				++libCount;

				if (!FindNextFile(handle, &search_data))
					break;
			}
		}
		FindClose(handle);

		modules = (HMODULE*)malloc(sizeof(HMODULE) * libCount);
		memset(modules, 0, sizeof(HMODULE) * libCount);

		memset(&search_data, 0, sizeof(WIN32_FIND_DATA));
		handle = FindFirstFile(searchPath, &search_data);
		{
			HMODULE* currModule = modules;
			while (handle != INVALID_HANDLE_VALUE)
			{
				TCHAR hookPath[MAX_PATH];
				strcpy(hookPath, path);
				strcat(hookPath, search_data.cFileName);

				*currModule = LoadLibrary(hookPath);
				++currModule;
				if (!FindNextFile(handle, &search_data))
					break;
			}
		}
		FindClose(handle);
	}

	VOID __fastcall Free()
	{
		HMODULE* currModule = modules;
		while (libCount)
		{
			if (currModule)
				FreeLibrary(*currModule);
			--libCount;
		}
	}
}