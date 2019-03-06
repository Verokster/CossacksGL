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

#include "StdAfx.h"

HMODULE hDllModule;
HANDLE hActCtx;
DirectDraw* ddrawList;

MCISENDCOMMANDA MciSendCommand;
MCIGETERRORSTRINGA MciGetErrorString;

CREATEACTCTXA CreateActCtxC;
RELEASEACTCTX ReleaseActCtxC;
ACTIVATEACTCTX ActivateActCtxC;
DEACTIVATEACTCTX DeactivateActCtxC;

SETPROCESSDPIAWARENESS SetProcessDpiAwarenessC;

MALLOC MemoryAlloc;
FREE MemoryFree;
ALIGNED_MALLOC AlignedAlloc;
ALIGNED_FREE AlignedFree;
MEMSET MemorySet;
MEMCPY MemoryCopy;
CEIL MathCeil;
FLOOR MathFloor;
ROUND MathRound;
SPRINTF StrPrint;
STRCMP StrCompare;
STRICMP StrCompareInsensitive;
STRCPY StrCopy;
STRCAT StrCat;
STRSTR StrStr;
STRRCHR StrLastChar;
WCSTOMBS StrToAnsi;
FOPEN FileOpen;
FCLOSE FileClose;
EXIT Exit;

DWORD
	pDirectPlayCreate,
	pDirectPlayEnumerateA,
	pDirectPlayEnumerateW,
	pDirectPlayLobbyCreateA,
	pDirectPlayLobbyCreateW,
	pDllCanUnloadNow,
	pDllGetClassObject,
	pDllRegisterServer,
	pDirectPlayEnumerate,
	pDllUnregisterServer,
	pGdwDPlaySPRefCount;

VOID _declspec(naked) __stdcall exDirectPlayCreate() { _asm { JMP pDirectPlayCreate } }
VOID _declspec(naked) __stdcall exDirectPlayEnumerateA() { _asm { JMP pDirectPlayEnumerateA } }
VOID _declspec(naked) __stdcall exDirectPlayEnumerateW() { _asm { JMP pDirectPlayEnumerateW } }
VOID _declspec(naked) __stdcall exDirectPlayLobbyCreateA() { _asm { JMP pDirectPlayLobbyCreateA } }
VOID _declspec(naked) __stdcall exDirectPlayLobbyCreateW() { _asm { JMP pDirectPlayLobbyCreateW } }
VOID _declspec(naked) __stdcall exDllCanUnloadNow() { _asm { JMP pDllCanUnloadNow } }
VOID _declspec(naked) __stdcall exDllGetClassObject() { _asm { JMP pDllGetClassObject } }
VOID _declspec(naked) __stdcall exDllRegisterServer() { _asm { JMP pDllRegisterServer } }
VOID _declspec(naked) __stdcall exDirectPlayEnumerate() { _asm { JMP pDirectPlayEnumerate } }
VOID _declspec(naked) __stdcall exDllUnregisterServer() { _asm { JMP pDllUnregisterServer } }
VOID _declspec(naked) __stdcall exGdwDPlaySPRefCount() { _asm { JMP pGdwDPlaySPRefCount } }

double __cdecl round(double number)
{
	double floorVal = MathFloor(number);
	return floorVal + 0.5f > number ? floorVal : MathCeil(number);
}

VOID LoadMsvCRT()
{
	HMODULE hLib = LoadLibrary("MSVCRT.dll");
	if (hLib)
	{
		StrPrint = (SPRINTF)GetProcAddress(hLib, "sprintf");

		CHAR libName[MAX_PATH];
		for (DWORD i = 12; i >= 7; --i)
		{
			StrPrint(libName, "MSVCR%d0.dll", i);
			HMODULE hCrtLib = LoadLibrary(libName);
			if (hCrtLib)
			{
				FreeLibrary(hLib);
				hLib = hCrtLib;
				StrPrint = (SPRINTF)GetProcAddress(hLib, "sprintf");
				break;
			}
		}

		MemoryAlloc = (MALLOC)GetProcAddress(hLib, "malloc");
		MemoryFree = (FREE)GetProcAddress(hLib, "free");

		AlignedAlloc = (ALIGNED_MALLOC)GetProcAddress(hLib, "_aligned_malloc");
		AlignedFree = (ALIGNED_FREE)GetProcAddress(hLib, "_aligned_free");

		MemorySet = (MEMSET)GetProcAddress(hLib, "memset");
		MemoryCopy = (MEMCPY)GetProcAddress(hLib, "memcpy");

		MathCeil = (CEIL)GetProcAddress(hLib, "ceil");
		MathFloor = (FLOOR)GetProcAddress(hLib, "floor");
		MathRound = (ROUND)GetProcAddress(hLib, "round");
		if (!MathRound)
			MathRound = round;

		StrCompare = (STRCMP)GetProcAddress(hLib, "strcmp");
		StrCompareInsensitive = (STRICMP)GetProcAddress(hLib, "_stricmp");
		StrCopy = (STRCPY)GetProcAddress(hLib, "strcpy");
		StrCat = (STRCAT)GetProcAddress(hLib, "strcat");
		StrStr = (STRSTR)GetProcAddress(hLib, "strstr");
		StrLastChar = (STRRCHR)GetProcAddress(hLib, "strrchr");
		StrToAnsi = (WCSTOMBS)GetProcAddress(hLib, "wcstombs");

		FileOpen = (FOPEN)GetProcAddress(hLib, "fopen");
		FileClose = (FCLOSE)GetProcAddress(hLib, "fclose");

		Exit = (EXIT)GetProcAddress(hLib, "exit");
	}
}

VOID LoadDPlayX()
{
	CHAR dir[MAX_PATH];
	if (GetSystemDirectory(dir, MAX_PATH))
	{
		StrCat(dir, "\\DPLAYX.dll");
		HMODULE hLib = LoadLibrary(dir);
		if (hLib)
		{
			pDirectPlayCreate = (DWORD)GetProcAddress(hLib, "DirectPlayCreate");
			pDirectPlayEnumerateA = (DWORD)GetProcAddress(hLib, "DirectPlayEnumerateA");
			pDirectPlayEnumerateW = (DWORD)GetProcAddress(hLib, "DirectPlayEnumerateW");
			pDirectPlayLobbyCreateA = (DWORD)GetProcAddress(hLib, "DirectPlayLobbyCreateA");
			pDirectPlayLobbyCreateW = (DWORD)GetProcAddress(hLib, "DirectPlayLobbyCreateW");
			pDllCanUnloadNow = (DWORD)GetProcAddress(hLib, "DllCanUnloadNow");
			pDllGetClassObject = (DWORD)GetProcAddress(hLib, "DllGetClassObject");
			pDllRegisterServer = (DWORD)GetProcAddress(hLib, "DllRegisterServer");
			pDirectPlayEnumerate = (DWORD)GetProcAddress(hLib, "DirectPlayEnumerate");
			pDllUnregisterServer = (DWORD)GetProcAddress(hLib, "DllUnregisterServer");
			pGdwDPlaySPRefCount = (DWORD)GetProcAddress(hLib, "gdwDPlaySPRefCount");
		}
	}
}

VOID LoadKernel32()
{
	HMODULE hLib = GetModuleHandle("KERNEL32.dll");
	if (hLib)
	{
		CreateActCtxC = (CREATEACTCTXA)GetProcAddress(hLib, "CreateActCtxA");
		ReleaseActCtxC = (RELEASEACTCTX)GetProcAddress(hLib, "ReleaseActCtx");
		ActivateActCtxC = (ACTIVATEACTCTX)GetProcAddress(hLib, "ActivateActCtx");
		DeactivateActCtxC = (DEACTIVATEACTCTX)GetProcAddress(hLib, "DeactivateActCtx");
	}
}

VOID LoadWinMM()
{
	CHAR dir[MAX_PATH];
	if (GetSystemDirectory(dir, MAX_PATH))
	{
		StrCat(dir, "\\WINMM.dll");
		HMODULE hLib = LoadLibrary(dir);
		if (hLib)
		{
			MciSendCommand = (MCISENDCOMMANDA)GetProcAddress(hLib, "mciSendCommandA");
			MciGetErrorString = (MCIGETERRORSTRINGA)GetProcAddress(hLib, "mciGetErrorStringA");
		}
	}
}

VOID LoadShcore()
{
	HMODULE hLib = LoadLibrary("SHCORE.dll");
	if (hLib)
	{
		SetProcessDpiAwarenessC = (SETPROCESSDPIAWARENESS)GetProcAddress(hLib, "SetProcessDpiAwareness");
	}
}