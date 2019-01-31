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
#include "Resource.h"
#include "Hooks.h"
#include "Config.h"

HHOOK OldWindowKeyHook;
LRESULT __stdcall WindowKeyHook(INT nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) &&
		!config.windowedMode && !mciVideo.deviceId)
	{
		KBDLLHOOKSTRUCT* phs = (KBDLLHOOKSTRUCT*)lParam;
		if (phs->vkCode == VK_SNAPSHOT)
		{
			HWND hWnd = GetActiveWindow();

			DirectDraw* ddraw = ddrawList;
			if (ddraw && (ddraw->hWnd == hWnd || ddraw->hDraw == hWnd))
			{
				ddraw->isTakeSnapshot = TRUE;
				return TRUE;
			}
		}
	}

	return CallNextHookEx(OldWindowKeyHook, nCode, wParam, lParam);
}

BOOL __stdcall DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		hDllModule = hModule;

		LoadMsvCRT();
		LoadDPlayX();
		LoadWinMM();
		LoadKernel32();
		LoadDwmAPI();

		Config::Load(GetModuleHandle(NULL));
		Hooks::Load();

		OldWindowKeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, WindowKeyHook, NULL, 0);

		{
			WNDCLASS wc = {
				CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS,
				DefWindowProc,
				0, 0,
				hDllModule,
				NULL, NULL,
				(HBRUSH)GetStockObject(BLACK_BRUSH), NULL,
				WC_DRAW
			};

			RegisterClass(&wc);
		}

		if (CreateActCtxC)
		{
			CHAR path[MAX_PATH];
			GetModuleFileName(hModule, path, MAX_PATH - 1);

			ACTCTX actCtx;
			MemoryZero(&actCtx, sizeof(ACTCTX));
			actCtx.cbSize = sizeof(ACTCTX);
			actCtx.lpSource = path;
			actCtx.hModule = hModule;
			actCtx.lpResourceName = MAKEINTRESOURCE(IDM_MANIFEST);
			actCtx.dwFlags = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID;
			hActCtx = CreateActCtxC(&actCtx);
		}

		break;
	}

	case DLL_PROCESS_DETACH:
		if (OldMouseHook)
			UnhookWindowsHookEx(OldMouseHook);

		if (OldWindowKeyHook)
			UnhookWindowsHookEx(OldWindowKeyHook);

		ChangeDisplaySettings(NULL, NULL);
		GL::Free();

		if (hActCtx && hActCtx != INVALID_HANDLE_VALUE)
			ReleaseActCtxC(hActCtx);

		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}