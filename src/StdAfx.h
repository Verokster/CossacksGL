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

#pragma once
#define WIN32_LEAN_AND_MEAN

#include "windows.h"
#include "Mmsystem.h"
#include "stdlib.h"
#include "stdio.h"
#include "DirectDraw.h"

#define WC_DRAW "ca6eb59a-0eb1-4349-b913-39b03a7008ab"
#define WIN_STYLE (WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS)
#define FS_STYLE (WS_POPUP | WS_SYSMENU | WS_VISIBLE | WS_CLIPSIBLINGS | WS_MAXIMIZE)

typedef MCIERROR(__stdcall *MCISENDCOMMANDA)(MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
typedef BOOL(__stdcall *MCIGETERRORSTRINGA)(MCIERROR mcierr, LPSTR pszText, UINT cchText);

extern MCISENDCOMMANDA MciSendCommand;
extern MCIGETERRORSTRINGA MciGetErrorString;

typedef HRESULT(__stdcall *DWMISCOMPOSITIONENABLED)(BOOL* pfEnabled);
extern DWMISCOMPOSITIONENABLED IsDwmCompositionEnabled;

typedef HANDLE(__stdcall *CREATEACTCTXA)(ACTCTX* pActCtx);
typedef VOID(__stdcall *RELEASEACTCTX)(HANDLE hActCtx);
typedef BOOL(__stdcall *ACTIVATEACTCTX)(HANDLE hActCtx, ULONG_PTR* lpCookie);
typedef BOOL(__stdcall *DEACTIVATEACTCTX)(DWORD dwFlags, ULONG_PTR ulCookie);

extern CREATEACTCTXA CreateActCtxC;
extern RELEASEACTCTX ReleaseActCtxC;
extern ACTIVATEACTCTX ActivateActCtxC;
extern DEACTIVATEACTCTX DeactivateActCtxC;

typedef VOID*(__cdecl *MALLOC)(size_t);
typedef VOID(__cdecl *FREE)(VOID*);
typedef VOID*(__cdecl *MEMSET)(VOID*, INT, size_t);
typedef VOID*(__cdecl *MEMCPY)(VOID*, const VOID*, size_t);
typedef double(__cdecl *CEIL)(double);
typedef double(__cdecl *FLOOR)(double);
typedef double(__cdecl *ROUND)(double);
typedef INT(__cdecl *SPRINTF)(CHAR*, const CHAR*, ...);
typedef INT(__cdecl *STRCMP)(const CHAR*, const CHAR*);
typedef CHAR*(__cdecl *STRCPY)(CHAR*, const CHAR*);
typedef CHAR*(__cdecl *STRCAT)(CHAR*, const CHAR*);
typedef CHAR*(__cdecl *STRSTR)(const CHAR*, const CHAR*);
typedef CHAR*(__cdecl *STRRCHR)(const CHAR*, INT);
typedef size_t(__cdecl *WCSTOMBS)(CHAR*, const WCHAR*, size_t);
typedef FILE*(__cdecl *FOPEN)(const CHAR*, const CHAR*);
typedef INT(__cdecl *FCLOSE)(FILE*);
typedef VOID(__cdecl *EXIT)(INT);

extern MALLOC MemoryAlloc;
extern FREE MemoryFree;
extern MEMSET MemorySet;
extern MEMCPY MemoryCopy;
extern CEIL MathCeil;
extern FLOOR MathFloor;
extern ROUND MathRound;
extern SPRINTF StrPrint;
extern STRCMP StrCompare;
extern STRCPY StrCopy;
extern STRCAT StrCat;
extern STRSTR StrStr;
extern STRRCHR StrLastChar;
extern WCSTOMBS StrToAnsi;
extern FOPEN FileOpen;
extern FCLOSE FileClose;
extern EXIT Exit;

#define MemoryZero(Destination,Length) MemorySet((Destination),0,(Length))

extern HMODULE hDllModule;
extern HANDLE hActCtx;

extern DirectDraw* ddrawList;
extern HHOOK OldMouseHook;

VOID LoadMsvCRT();
VOID LoadDPlayX();
VOID LoadKernel32();
VOID LoadWinMM();
VOID LoadDwmAPI();