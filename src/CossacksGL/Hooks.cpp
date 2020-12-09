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
#include "mmsystem.h"
#include "winnt.h"
#include "Resource.h"
#include "Hooks.h"
#include "Main.h"
#include "Config.h"
#include "Window.h"
#include "Hooker.h"

MciVideo mciVideo;
MCIDEVICEID mciList[16];
DWORD mciIndex;
INT baseOffset;

namespace Hooks
{
	MCISENDCOMMANDA MciSendCommandOld;
	MCIGETERRORSTRINGA MciGetErrorStringOld;

	VOID __fastcall CalcVideoSize(LONG width, LONG height, RECT* rc)
	{
		rc->left = 0;
		rc->top = 0;
		rc->right = width;
		rc->bottom = height;

		if (config.aspectRatio)
		{
			FLOAT fx = (FLOAT)width / mciVideo.size.cx;
			FLOAT fy = (FLOAT)height / mciVideo.size.cy;
			if (fx != fy)
			{
				if (fx > fy)
				{
					FLOAT f = fy * mciVideo.size.cx;
					rc->left = (DWORD)MathRound(((FLOAT)width - f) / 2.0f);
					rc->right = (DWORD)MathRound(f);
				}
				else
				{
					FLOAT f = fx * mciVideo.size.cy;
					rc->top = (DWORD)MathRound(((FLOAT)height - f) / 2.0f);
					rc->bottom = (DWORD)MathRound(f);
				}
			}
		}
	}

	ATOM __stdcall RegisterClassHook(WNDCLASSA* lpWndClass)
	{
		lpWndClass->hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		lpWndClass->hIcon = config.icon;
		return RegisterClass(lpWndClass);
	}

	HWND __stdcall CreateWindowExHook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, INT X, INT Y, INT nWidth, INT nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
	{
		INT monWidth = GetSystemMetrics(SM_CXSCREEN);
		INT monHeight = GetSystemMetrics(SM_CYSCREEN);

		RECT rect;

		if (config.windowedMode)
		{
			dwStyle = WIN_STYLE;

			nWidth = (INT)MathRound(0.75f * monWidth);
			nHeight = (INT)MathRound(0.75f * monHeight);

			FLOAT k = 4.0f / 3.0f;

			INT check = (INT)MathRound((FLOAT)nHeight * k);
			if (nWidth > check)
				nWidth = check;
			else
				nHeight = (INT)MathRound((FLOAT)nWidth / k);

			X = (monWidth - nWidth) >> 1;
			Y = (monHeight - nHeight) >> 1;

			rect.left = X;
			rect.top = Y;
			rect.right = X + nWidth;
			rect.bottom = Y + nHeight;

			AdjustWindowRect(&rect, dwStyle, TRUE);
		}
		else
		{
			dwStyle = FS_STYLE | WS_CAPTION;

			rect.left = 0;
			rect.top = 0;
			rect.right = monWidth;
			rect.bottom = monHeight;
		}

		X = rect.left;
		Y = rect.top;
		nWidth = rect.right - rect.left;
		nHeight = rect.bottom - rect.top;

		HWND hWnd = CreateWindowEx(
			dwExStyle,
			lpClassName,
			lpWindowName,
			dwStyle,
			X, Y,
			nWidth, nHeight,
			hWndParent,
			hMenu,
			hInstance,
			lpParam);

		if (hWnd)
		{
			if (config.windowedMode)
				SetMenu(hWnd, config.menu);
			else
				SetWindowLong(hWnd, GWL_STYLE, FS_STYLE);

			Window::SetCaptureWindow(hWnd);
		}

		return hWnd;
	}

	MCIERROR __stdcall mciSendCommandHook(MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
	{
		MCIERROR mcierr;

		switch (uMsg)
		{
		case MCI_OPEN: {
			LPMCI_OPEN_PARMS params = (LPMCI_OPEN_PARMS)dwParam2;
			if (!StrCompare(params->lpstrDeviceType, "AVIVideo"))
			{
				HWND hWnd = (HWND)params->dwCallback;
				DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
				if (ddraw)
					ddraw->RenderStop();

				mciVideo.error = mcierr = MciSendCommand(mciId, uMsg, dwParam1, dwParam2);
				if (mcierr)
					return mcierr;

				mciList[mciIndex++] = params->wDeviceID;
				mciVideo.deviceId = mciIndex;

				SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG)GetStockObject(BLACK_BRUSH));
				if (!config.windowedMode)
					SetWindowPos(hWnd, NULL, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) + 1, SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOREPOSITION | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
			}
			else
			{
				mcierr = MciSendCommandOld(mciId, uMsg, dwParam1, dwParam2);
				if (mcierr)
					return mcierr;

				mciList[mciIndex++] = params->wDeviceID;
			}

			params->wDeviceID = mciIndex;
			mciIndex &= 0xFF;

			break;
		}

		case MCI_CLOSE: {
			if (mciVideo.deviceId == mciId)
			{
				mciVideo.error = mcierr = MciSendCommand(mciList[mciId - 1], uMsg, dwParam1, dwParam2);
				if (mcierr)
					return mcierr;

				mciVideo.deviceId = NULL;
			}
			else
				mcierr = MciSendCommandOld(mciList[mciId - 1], uMsg, dwParam1, dwParam2);

			mciList[mciId - 1] = NULL;

			break;
		}

		case MCI_PUT: {
			if (mciVideo.deviceId == mciId)
			{
				if (dwParam1 & MCI_OVLY_PUT_DESTINATION)
				{
					LPMCI_OVLY_RECT_PARMS params = (LPMCI_OVLY_RECT_PARMS)dwParam2;

					mciVideo.size.cx = params->rc.right;
					mciVideo.size.cy = params->rc.bottom;

					RECT rc;
					if (GetClientRect((HWND)params->dwCallback, &rc))
						CalcVideoSize(rc.right, rc.bottom, &params->rc);
				}

				mciVideo.error = mcierr = MciSendCommand(mciList[mciId - 1], uMsg, dwParam1, dwParam2);
			}
			else
				mcierr = MciSendCommandOld(mciList[mciId - 1], uMsg, dwParam1, dwParam2);

			break;
		}

		default: {
			if (mciVideo.deviceId == mciId)
				mciVideo.error = mcierr = MciSendCommand(mciList[mciId - 1], uMsg, dwParam1, dwParam2);
			else
				mcierr = MciSendCommandOld(mciList[mciId - 1], uMsg, dwParam1, dwParam2);

			break;
		}
		}

		return mcierr;
	}

	BOOL __stdcall mciGetErrorStringHook(MCIERROR mcierr, LPSTR pszText, UINT cchText)
	{
		if (mciVideo.deviceId && mciVideo.error == mcierr)
			return MciGetErrorString(mcierr, pszText, cchText);
		else
			return MciGetErrorStringOld(mcierr, pszText, cchText);
	}

	BOOL __stdcall ClipCursorHook(const RECT* lpRect) { return TRUE; }

	INT __stdcall ShowCursorHook(BOOL bShow) { return bShow ? 1 : -1; }

	HCURSOR __stdcall SetCursorHook(HCURSOR hCursor) { return NULL; }

	BOOL __stdcall SetCursorPosHook(INT X, INT Y) { return TRUE; }

	INT __stdcall MessageBoxHook(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
	{
		INT res;
		ULONG_PTR cookie = NULL;
		if (hActCtx && hActCtx != INVALID_HANDLE_VALUE && !ActivateActCtxC(hActCtx, &cookie))
			cookie = NULL;

		res = MessageBox(hWnd, lpText, lpCaption, uType);

		if (cookie)
			DeactivateActCtxC(0, cookie);

		return res;
	}

	BOOL __stdcall ClientToScreenHook(HWND hWnd, LPPOINT lpPoint) { return TRUE; }

	BOOL __stdcall PeekMessageHook(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
	{
		BOOL res = PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
		if (!res)
			Sleep(0);
		return res;
	}

	HMODULE __stdcall LoadLibraryHook(LPCSTR lpLibFileName)
	{
		if (!StrCompareInsensitive(lpLibFileName, "DDRAW.dll") || !StrCompareInsensitive(lpLibFileName, "MDRAW.dll"))
			return hDllModule;
		return LoadLibrary(lpLibFileName);
	}

	BOOL __stdcall FreeLibraryHook(HMODULE hLibModule)
	{
		if (hLibModule == hDllModule)
			return TRUE;
		return FreeLibrary(hLibModule);
	}

	FARPROC __stdcall GetProcAddressHook(HMODULE hModule, LPCSTR lpProcName)
	{
		if (!StrCompare(lpProcName, "DirectDrawCreate"))
			return (FARPROC)Main::DirectDrawCreate;
		else if (!StrCompare(lpProcName, "DirectDrawCreateEx"))
			return (FARPROC)Main::DirectDrawCreateEx;
		else
			return GetProcAddress(hModule, lpProcName);
	}

	BOOL __stdcall FakeEntryPoint(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) { return TRUE; }

	VOID __fastcall RedirectLibrary(const CHAR* name)
	{
		HMODULE hLib = GetModuleHandle(name);
		if (hLib)
		{
			HOOKER hooker = CreateHooker(hLib);
			{
				PatchEntry(hooker, FakeEntryPoint);
			}
			ReleaseHooker(hooker);
		}
	}

	DOUBLE flush;
#define SYNC_STEP (1000.0 / 85.0)
	VOID __fastcall FlipPage()
	{
		LONGLONG qp;
		QueryPerformanceFrequency((LARGE_INTEGER*)&qp);
		DOUBLE time = (DOUBLE)qp * 0.001;
		QueryPerformanceCounter((LARGE_INTEGER*)&qp);
		time = (DOUBLE)qp / time;

		INT sleep = INT(flush - time);
		Sleep(sleep > 0 ? *(DWORD*)&sleep : 0);

		QueryPerformanceFrequency((LARGE_INTEGER*)&qp);
		time = (DOUBLE)qp * 0.001;
		QueryPerformanceCounter((LARGE_INTEGER*)&qp);
		time = (DOUBLE)qp / time;

		if (flush > time)
			flush += SYNC_STEP;
		else
			flush = SYNC_STEP * (DWORD(time / SYNC_STEP) + 1);
	}

	VOID __declspec(naked) FlipPageHook()
	{
		__asm {
			lbl_start:
				MOV ECX, [EBP-0x34]
				REP MOVSD
				ADD ESI, [EBP-0x30]
				ADD EDI, [EBP-0x2C]
				DEC EAX

			JNZ lbl_start
			JMP FlipPage
		}
	}

	VOID Load()
	{
		RedirectLibrary("DDRAW.dll");
		RedirectLibrary("MDRAW.dll");

		HMODULE hLib = LoadLibrary("DPWSOCKX.dll");
		if (hLib)
		{
			HOOKER hooker = CreateHooker(hLib);
			{
				PatchImportByName(hooker, "gdwDPlaySPRefCount", (VOID*)pGdwDPlaySPRefCount);
			}
			ReleaseHooker(hooker);
		}

		HOOKER hooker = CreateHooker(GetModuleHandle(NULL));
		{
			PatchImportByName(hooker, "DirectDrawCreate", Main::DirectDrawCreate);

			PatchImportByName(hooker, "LoadLibraryA", LoadLibraryHook);
			PatchImportByName(hooker, "FreeLibrary", FreeLibraryHook);
			PatchImportByName(hooker, "GetProcAddress", GetProcAddressHook);

			PatchImportByName(hooker, "RegisterClassA", RegisterClassHook);
			PatchImportByName(hooker, "CreateWindowExA", CreateWindowExHook);

			PatchImportByName(hooker, "ClipCursor", ClipCursorHook);
			PatchImportByName(hooker, "ShowCursor", ShowCursorHook);
			PatchImportByName(hooker, "SetCursor", SetCursorHook);
			PatchImportByName(hooker, "SetCursorPos", SetCursorPosHook);

			PatchImportByName(hooker, "MessageBoxA", MessageBoxHook);
			PatchImportByName(hooker, "ClientToScreen", ClientToScreenHook);

			PatchImportByName(hooker, "PeekMessageA", PeekMessageHook);
			PatchImportByName(hooker, "GetTickCount", timeGetTime);

			PatchImportByName(hooker, "mciSendCommandA", mciSendCommandHook, (DWORD*)&MciSendCommandOld);
			PatchImportByName(hooker, "mciGetErrorStringA", mciGetErrorStringHook, (DWORD*)&MciGetErrorStringOld);

			// Flip Page
			const BYTE flipBlock[] = { 0x8B, 0x4D, 0xCC, 0xF3, 0xA5, 0x03, 0x75, 0xD0, 0x03, 0x7D, 0xD4, 0x48 };
			DWORD address = FindBlock(hooker, (VOID*)flipBlock, sizeof(flipBlock));
			if (address)
				PatchCall(hooker, address - baseOffset, FlipPageHook, sizeof(flipBlock) + 2 - 5);

			// ============================

			DWORD val;
			if (ReadDWord(hooker, 0x02D8D02E, &val) && val == 0x006355B8) // HEW mod SKIDROW
				PatchHook(hooker, 0x02D8D025, (VOID*)val);
		}
		ReleaseHooker(hooker);
	}
}