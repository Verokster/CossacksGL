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
#include "mmsystem.h"
#include "Resource.h"
#include "Hooks.h"
#include "Main.h"
#include "Config.h"

MciVideo mciVideo;

namespace Hooks
{
	PIMAGE_DOS_HEADER headDOS;
	INT baseOffset;

	BOOL __fastcall PatchRedirect(DWORD addr, DWORD dest, DWORD size, BYTE instruction)
	{
		DWORD address = addr + baseOffset;

		DWORD old_prot;
		if (VirtualProtect((VOID*)address, size, PAGE_EXECUTE_READWRITE, &old_prot))
		{
			BYTE* jump = (BYTE*)address;
			*jump = instruction;
			++jump;
			*(DWORD*)jump = dest - address - size;

			VirtualProtect((VOID*)address, size, old_prot, &old_prot);

			return TRUE;
		}
		return FALSE;
	}

	BOOL __fastcall PatchJump(DWORD addr, DWORD dest)
	{
		INT relative = dest - addr - baseOffset - 2;
		if (relative >= -128 && relative <= 127)
			return PatchRedirect(addr, dest, 2, 0xEB);
		else
			return PatchRedirect(addr, dest, 5, 0xE9);
	}

	BOOL __fastcall PatchHook(DWORD addr, VOID* hook)
	{
		return PatchRedirect(addr, (DWORD)hook, 5, 0xE9);
	}

	BOOL __fastcall PatchCall(DWORD addr, VOID* hook)
	{
		return PatchRedirect(addr, (DWORD)hook, 5, 0xE8);
	}

	BOOL __fastcall PatchNop(DWORD addr, DWORD size)
	{
		DWORD address = addr + baseOffset;

		DWORD old_prot;
		if (VirtualProtect((VOID*)address, size, PAGE_EXECUTE_READWRITE, &old_prot))
		{
			MemorySet((VOID*)address, 0x90, size);
			VirtualProtect((VOID*)address, size, old_prot, &old_prot);

			return TRUE;
		}
		return FALSE;
	}

	BOOL __fastcall PatchBlock(DWORD addr, VOID* block, DWORD size)
	{
		DWORD address = addr + baseOffset;

		DWORD old_prot;
		if (VirtualProtect((VOID*)address, size, PAGE_EXECUTE_READWRITE, &old_prot))
		{
			switch (size)
			{
			case 4:
				*(DWORD*)address = *(DWORD*)block;
				break;
			case 2:
				*(WORD*)address = *(WORD*)block;
				break;
			case 1:
				*(BYTE*)address = *(BYTE*)block;
				break;
			default:
				MemoryCopy((VOID*)address, block, size);
				break;
			}

			VirtualProtect((VOID*)address, size, old_prot, &old_prot);

			return TRUE;
		}
		return FALSE;
	}

	BOOL __fastcall ReadBlock(DWORD addr, VOID* block, DWORD size)
	{
		DWORD address = addr + baseOffset;

		DWORD old_prot;
		if (VirtualProtect((VOID*)address, size, PAGE_READONLY, &old_prot))
		{
			switch (size)
			{
			case 4:
				*(DWORD*)block = *(DWORD*)address;
				break;
			case 2:
				*(WORD*)block = *(WORD*)address;
				break;
			case 1:
				*(BYTE*)block = *(BYTE*)address;
				break;
			default:
				MemoryCopy(block, (VOID*)address, size);
				break;
			}

			VirtualProtect((VOID*)address, size, old_prot, &old_prot);

			return TRUE;
		}
		return FALSE;
	}

	BOOL __fastcall PatchWord(DWORD addr, WORD value)
	{
		return PatchBlock(addr, &value, sizeof(value));
	}

	BOOL __fastcall PatchInt(DWORD addr, INT value)
	{
		return PatchBlock(addr, &value, sizeof(value));
	}

	BOOL __fastcall PatchDWord(DWORD addr, DWORD value)
	{
		return PatchBlock(addr, &value, sizeof(value));
	}

	BOOL __fastcall PatchByte(DWORD addr, BYTE value)
	{
		return PatchBlock(addr, &value, sizeof(value));
	}

	BOOL __fastcall ReadWord(DWORD addr, WORD* value)
	{
		return ReadBlock(addr, value, sizeof(*value));
	}

	BOOL __fastcall ReadDWord(DWORD addr, DWORD* value)
	{
		return ReadBlock(addr, value, sizeof(*value));
	}

	DWORD __fastcall PatchFunction(MappedFile* file, const CHAR* function, VOID* addr)
	{
		DWORD res = NULL;

		DWORD base = (DWORD)headDOS;
		PIMAGE_NT_HEADERS headNT = (PIMAGE_NT_HEADERS)((BYTE*)base + headDOS->e_lfanew);

		PIMAGE_DATA_DIRECTORY dataDir = &headNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		if (dataDir->Size)
		{
			PIMAGE_IMPORT_DESCRIPTOR imports = (PIMAGE_IMPORT_DESCRIPTOR)(base + dataDir->VirtualAddress);
			for (DWORD idx = 0; imports->Name; ++idx, ++imports)
			{
				CHAR* libraryName = (CHAR*)(base + imports->Name);

				PIMAGE_THUNK_DATA addressThunk = (PIMAGE_THUNK_DATA)(base + imports->FirstThunk);
				PIMAGE_THUNK_DATA nameThunk;
				if (imports->OriginalFirstThunk)
					nameThunk = (PIMAGE_THUNK_DATA)(base + imports->OriginalFirstThunk);
				else
				{
					if (!file->hFile)
					{
						CHAR filePath[MAX_PATH];
						GetModuleFileName((HMODULE)headDOS, filePath, MAX_PATH);
						file->hFile = CreateFile(filePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
						if (!file->hFile)
							return res;
					}

					if (!file->hMap)
					{
						file->hMap = CreateFileMapping(file->hFile, NULL, PAGE_READONLY, 0, 0, NULL);
						if (!file->hMap)
							return res;
					}

					if (!file->address)
					{
						file->address = MapViewOfFile(file->hMap, FILE_MAP_READ, 0, 0, 0);;
						if (!file->address)
							return res;
					}

					headNT = (PIMAGE_NT_HEADERS)((BYTE*)file->address + ((PIMAGE_DOS_HEADER)file->address)->e_lfanew);
					PIMAGE_SECTION_HEADER sh = (PIMAGE_SECTION_HEADER)((DWORD)&headNT->OptionalHeader + headNT->FileHeader.SizeOfOptionalHeader);

					nameThunk = NULL;
					DWORD sCount = headNT->FileHeader.NumberOfSections;
					while (sCount--)
					{
						if (imports->FirstThunk >= sh->VirtualAddress && imports->FirstThunk < sh->VirtualAddress + sh->Misc.VirtualSize)
						{
							nameThunk = PIMAGE_THUNK_DATA((DWORD)file->address + sh->PointerToRawData + imports->FirstThunk - sh->VirtualAddress);
							break;
						}

						++sh;
					}

					if (!nameThunk)
						return res;
				}

				for (; nameThunk->u1.AddressOfData; ++nameThunk, ++addressThunk)
				{
					PIMAGE_IMPORT_BY_NAME name = PIMAGE_IMPORT_BY_NAME(base + nameThunk->u1.AddressOfData);

					WORD hint;
					if (ReadWord((INT)name - baseOffset, &hint) && !StrCompare((CHAR*)name->Name, function))
					{
						if (ReadDWord((INT)&addressThunk->u1.AddressOfData - baseOffset, &res))
							PatchDWord((INT)&addressThunk->u1.AddressOfData - baseOffset, (DWORD)addr);

						return res;
					}
				}
			}
		}

		return res;
	}

	// =====================================================================================================

	MCISENDCOMMANDA MciSendCommandOld;
	MCIGETERRORSTRINGA MciGetErrorStringOld;
	WNDPROC OldWindowProc;
	MCIDEVICEID mciList[16];
	DWORD mciIndex;

	VOID __fastcall CalcVideoSize(DWORD width, DWORD height, RECT* rc)
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

	LRESULT __stdcall WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_SIZE:
		{
			if (mciVideo.deviceId)
			{
				MCI_OVLY_RECT_PARMS params;
				params.dwCallback = (DWORD_PTR)hWnd;
				CalcVideoSize(LOWORD(lParam), HIWORD(lParam), &params.rc);
				MciSendCommand(mciList[mciVideo.deviceId - 1], MCI_PUT, MCI_OVLY_RECT | MCI_OVLY_PUT_DESTINATION, (DWORD_PTR)&params);
			}

			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}

		case WM_COMMAND:
		{
			if (wParam == IDM_IMAGE_ASPECTRATIO)
			{
				if (mciVideo.deviceId)
				{
					RECT rc;
					GetClientRect(hWnd, &rc);

					MCI_OVLY_RECT_PARMS params;
					params.dwCallback = (DWORD_PTR)hWnd;
					CalcVideoSize(rc.right - rc.left, rc.bottom - rc.top, &params.rc);
					MciSendCommand(mciList[mciVideo.deviceId - 1], MCI_PUT, MCI_OVLY_RECT | MCI_OVLY_PUT_DESTINATION, (DWORD_PTR)&params);
				}
				return NULL;
			}
			else
				return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}

		case MM_MCINOTIFY:
		{
			MCIDEVICEID mciId = (MCIDEVICEID)lParam;

			DWORD check = 16;
			DWORD idx = mciIndex;
			do
			{
				if (!idx)
					idx = 15;
				else
					--idx;

				if (mciList[idx] == mciId)
				{
					mciId = idx + 1;
					break;
				}
			} while (--check);

			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, (LPARAM)mciId);
		}

		default:
			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}

		return NULL;
	}

	ATOM __stdcall RegisterClassHook(WNDCLASSA *lpWndClass)
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

			OldWindowProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);
		}

		return hWnd;
	}

	MCIERROR __stdcall mciSendCommandHook(MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
	{
		MCIERROR mcierr;

		switch (uMsg)
		{
		case MCI_OPEN:
		{
			LPMCI_OPEN_PARMS params = (LPMCI_OPEN_PARMS)dwParam2;
			if (!StrCompare(params->lpstrDeviceType, "AVIVideo"))
			{
				HWND hWnd = (HWND)params->dwCallback;

				if (!config.windowedMode)
					SetWindowPos(hWnd, NULL, 0, -1, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) + 1, SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOREPOSITION | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

				mciVideo.error = mcierr = MciSendCommand(mciId, uMsg, dwParam1, dwParam2);
				if (mcierr)
					return mcierr;

				mciList[mciIndex++] = params->wDeviceID;
				mciVideo.deviceId = mciIndex;

				SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG)GetStockObject(BLACK_BRUSH));

				DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
				if (ddraw && ddraw->hDraw)
					SetWindowLong(ddraw->hDraw, GWL_STYLE, WS_POPUP);
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

		case MCI_CLOSE:
		{
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

		case MCI_PUT:
		{
			if (mciVideo.deviceId == mciId)
			{
				if (dwParam1 & MCI_OVLY_PUT_DESTINATION)
				{
					LPMCI_OVLY_RECT_PARMS params = (LPMCI_OVLY_RECT_PARMS)dwParam2;

					mciVideo.size.cx = params->rc.right;
					mciVideo.size.cy = params->rc.bottom;

					RECT rc;
					if (GetClientRect((HWND)params->dwCallback, &rc))
						CalcVideoSize(DWORD(rc.right - rc.left), DWORD(rc.bottom - rc.top), &params->rc);
				}

				mciVideo.error = mcierr = MciSendCommand(mciList[mciId - 1], uMsg, dwParam1, dwParam2);
			}
			else
				mcierr = MciSendCommandOld(mciList[mciId - 1], uMsg, dwParam1, dwParam2);

			break;
		}

		default:
		{
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

	BOOL __stdcall ClipCursorHook(const RECT *lpRect) { return TRUE; }

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

	FARPROC __stdcall GetProcAddressHook(HMODULE hModule, LPCSTR lpProcName)
	{
		if (!StrCompare(lpProcName, "DirectDrawCreate"))
			return (FARPROC)Main::DirectDrawCreate;
		else
			return GetProcAddress(hModule, lpProcName);
	}

	VOID Load()
	{
		headDOS = (PIMAGE_DOS_HEADER)GetModuleHandle(NULL);
		PIMAGE_NT_HEADERS headNT = (PIMAGE_NT_HEADERS)((BYTE*)headDOS + headDOS->e_lfanew);
		baseOffset = (INT)headDOS - (INT)headNT->OptionalHeader.ImageBase;

		MappedFile file;
		MemoryZero(&file, sizeof(MappedFile));
		{
			DWORD all = PatchFunction(&file, "DirectDrawCreate", Main::DirectDrawCreate);

			PatchFunction(&file, "RegisterClassA", RegisterClassHook);
			PatchFunction(&file, "CreateWindowExA", CreateWindowExHook);
			MciSendCommandOld = (MCISENDCOMMANDA)PatchFunction(&file, "mciSendCommandA", mciSendCommandHook);
			MciGetErrorStringOld = (MCIGETERRORSTRINGA)PatchFunction(&file, "mciGetErrorStringA", mciGetErrorStringHook);
			PatchFunction(&file, "ClipCursor", ClipCursorHook);
			PatchFunction(&file, "ShowCursor", ShowCursorHook);
			PatchFunction(&file, "SetCursor", SetCursorHook);
			PatchFunction(&file, "SetCursorPos", SetCursorPosHook);
			PatchFunction(&file, "MessageBoxA", MessageBoxHook);
			PatchFunction(&file, "ClientToScreen", ClientToScreenHook);
			PatchFunction(&file, "GetProcAddress", GetProcAddressHook);
		}

		if (file.address)
			UnmapViewOfFile(file.address);

		if (file.hMap)
			CloseHandle(file.hMap);

		if (file.hFile)
			CloseHandle(file.hFile);
	}
}