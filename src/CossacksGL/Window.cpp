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
#include "Shellapi.h"
#include "CommCtrl.h"
#include "Window.h"
#include "Resource.h"
#include "Config.h"
#include "Main.h"
#include "Hooks.h"
#include "FpsCounter.h"

#define MIN_WIDTH 240
#define MIN_HEIGHT 180

namespace Window
{
	HHOOK OldMouseHook, OldKeysHook;
	WNDPROC OldWindowProc, OldPanelProc;

#pragma optimize("t", on)
	LRESULT __stdcall KeysHook(INT nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) &&
			!config.windowedMode && !mciVideo.deviceId)
		{
			KBDLLHOOKSTRUCT* phs = (KBDLLHOOKSTRUCT*)lParam;
			if (phs->vkCode == VK_SNAPSHOT)
			{
				HWND hWnd = GetForegroundWindow();

				DirectDraw* ddraw = ddrawList;
				if (ddraw && (ddraw->hWnd == hWnd || ddraw->hDraw == hWnd))
				{
					ddraw->isTakeSnapshot = TRUE;
					return TRUE;
				}
			}
		}

		return CallNextHookEx(OldKeysHook, nCode, wParam, lParam);
	}

	LRESULT __stdcall MouseHook(INT nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode >= 0)
		{
			DirectDraw* ddraw = ddrawList;
			while (ddraw)
			{
				ddraw->CaptureMouse((UINT)wParam, (LPMSLLHOOKSTRUCT)lParam);
				ddraw = (DirectDraw*)ddraw->last;
			}
		}

		return CallNextHookEx(OldMouseHook, nCode, wParam, lParam);
	}

	BOOL __stdcall EnumChildProc(HWND hDlg, LPARAM lParam)
	{
		if ((GetWindowLong(hDlg, GWL_STYLE) & SS_ICON) == SS_ICON)
			SendMessage(hDlg, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)config.icon);
		else
			SendMessage(hDlg, WM_SETFONT, (WPARAM)config.font, TRUE);

		return TRUE;
	}

	LRESULT __stdcall AboutProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
		{
			SetWindowLong(hDlg, GWL_EXSTYLE, NULL);
			EnumChildWindows(hDlg, EnumChildProc, NULL);

			CHAR path[MAX_PATH];
			CHAR temp[100];

			GetModuleFileName(hDllModule, path, sizeof(path));

			DWORD hSize;
			DWORD verSize = GetFileVersionInfoSize(path, &hSize);

			if (verSize)
			{
				CHAR* verData = (CHAR*)MemoryAlloc(verSize);
				{
					if (GetFileVersionInfo(path, hSize, verSize, verData))
					{
						VOID* buffer;
						UINT size;
						if (VerQueryValue(verData, "\\", &buffer, &size) && size)
						{
							VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)buffer;

							GetDlgItemText(hDlg, IDC_VERSION, temp, sizeof(temp));
							StrPrint(path, temp, HIWORD(verInfo->dwProductVersionMS), LOWORD(verInfo->dwProductVersionMS), HIWORD(verInfo->dwProductVersionLS), LOWORD(verInfo->dwFileVersionLS));
							SetDlgItemText(hDlg, IDC_VERSION, path);
						}
					}
				}
				MemoryFree(verData);
			}

			if (GetDlgItemText(hDlg, IDC_LNK_EMAIL, temp, sizeof(temp)))
			{
				StrPrint(path, "<A HREF=\"mailto:%s\">%s</A>", temp, temp);
				SetDlgItemText(hDlg, IDC_LNK_EMAIL, path);
			}

			break;
		}

		case WM_NOTIFY:
		{
			if (((NMHDR*)lParam)->code == NM_CLICK && wParam == IDC_LNK_EMAIL)
			{
				NMLINK* pNMLink = (NMLINK*)lParam;
				LITEM iItem = pNMLink->item;

				CHAR url[MAX_PATH];
				StrToAnsi(url, pNMLink->item.szUrl, sizeof(url) - 1);

				SHELLEXECUTEINFO shExecInfo;
				MemoryZero(&shExecInfo, sizeof(SHELLEXECUTEINFO));
				shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
				shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
				shExecInfo.lpFile = url;
				shExecInfo.nShow = SW_SHOW;

				ShellExecuteEx(&shExecInfo);
			}

			break;
		}

		case WM_COMMAND:
		{
			if (wParam == IDOK)
				EndDialog(hDlg, TRUE);
			break;
		}

		default:
			break;
		}

		return DefWindowProc(hDlg, uMsg, wParam, lParam);
	}

	LRESULT __stdcall WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_GETMINMAXINFO:
		{
			if (config.windowedMode)
			{
				RECT rect = { 0, 0, MIN_WIDTH, MIN_HEIGHT };
				AdjustWindowRect(&rect, GetWindowLong(hWnd, GWL_STYLE), TRUE);

				MINMAXINFO* mmi = (MINMAXINFO*)lParam;
				mmi->ptMinTrackSize.x = rect.right - rect.left;
				mmi->ptMinTrackSize.y = rect.bottom - rect.top;
				mmi->ptMaxTrackSize.x = LONG_MAX >> 16;
				mmi->ptMaxTrackSize.y = LONG_MAX >> 16;
				mmi->ptMaxSize.x = LONG_MAX >> 16;
				mmi->ptMaxSize.y = LONG_MAX >> 16;

				return NULL;
			}

			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}

		case WM_SIZE:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
			{
				if (ddraw->hDraw && !config.singleWindow)
					SetWindowPos(ddraw->hDraw, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOREPOSITION | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

				if (ddraw->dwMode)
				{
					ddraw->viewport.width = LOWORD(lParam);
					ddraw->viewport.height = HIWORD(lParam);
					ddraw->viewport.refresh = TRUE;
				}
			}

			if (mciVideo.deviceId)
			{
				MCI_OVLY_RECT_PARMS params;
				params.dwCallback = (DWORD_PTR)hWnd;
				Hooks::CalcVideoSize(LOWORD(lParam), HIWORD(lParam), &params.rc);
				MciSendCommand(mciList[mciVideo.deviceId - 1], MCI_PUT, MCI_OVLY_RECT | MCI_OVLY_PUT_DESTINATION, (DWORD_PTR)&params);
			}

			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}

		case WM_DISPLAYCHANGE:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
			{
				DEVMODE devMode;
				MemoryZero(&devMode, sizeof(DEVMODE));
				devMode.dmSize = sizeof(DEVMODE);
				ddraw->frequency = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode) && (devMode.dmFields & DM_DISPLAYFREQUENCY) ? devMode.dmDisplayFrequency : 0;
			}

			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		case WM_ACTIVATE:
		case WM_NCACTIVATE:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);

		case WM_ACTIVATEAPP:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw && ddraw->dwMode)
			{
				if (!config.windowedMode)
				{
					if ((BOOL)wParam)
					{
						ddraw->SetFullscreenMode();
						ddraw->RenderStart();
					}
					else
					{
						ddraw->RenderStop();
						ChangeDisplaySettings(NULL, NULL);
					}

				}
				else
					SetCaptureMouse((BOOL)wParam);
			}

			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		case WM_COMMAND:
		{
			switch (wParam)
			{
			case IDM_WINDOW_FULLSCREEN:
			{
				DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
				if (ddraw && ddraw->dwMode)
				{
					ddraw->RenderStop();
					{
						if (!config.windowedMode)
							ddraw->SetWindowedMode();
						else
							ddraw->SetFullscreenMode();

						config.windowedMode = !config.windowedMode;
						Config::Set(CONFIG, "WindowedMode", config.windowedMode);
						CheckMenu();
					}
					ddraw->RenderStart();
				}

				return NULL;
			}

			case IDM_WINDOW_VSYNC:
			{
				config.vSync = !config.vSync;
				Config::Set(CONFIG, "VSync", config.vSync);
				CheckMenu();

				return NULL;
			}

			case IDM_WINDOW_FPSCOUNTER:
			{
				config.fpsCounter = !config.fpsCounter;
				Config::Set(CONFIG, "FpsCounter", config.fpsCounter);
				CheckMenu();
				isFpsChanged = TRUE;

				return NULL;
			}

			case IDM_WINDOW_MOUSECAPTURE:
			{
				config.mouseCapture = !config.mouseCapture;
				Config::Set(CONFIG, "MouseCapture", config.mouseCapture);
				CheckMenu();
				SetCaptureMouse(config.mouseCapture);

				return NULL;
			}

			case IDM_WINDOW_EXIT:
			{
				SendMessage(hWnd, WM_CLOSE, NULL, NULL);
				return NULL;
			}

			case IDM_IMAGE_FILTERING:
			{
				config.filtering = !config.filtering;
				Config::Set(CONFIG, "Filtering", config.filtering);
				CheckMenu();

				DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
				if (ddraw && ddraw->dwMode)
					ddraw->isStateChanged = TRUE;

				return NULL;
			}

			case IDM_IMAGE_ASPECTRATIO:
			{
				config.aspectRatio = !config.aspectRatio;
				Config::Set(CONFIG, "AspectRatio", config.aspectRatio);
				CheckMenu();

				DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
				if (ddraw && ddraw->dwMode)
					ddraw->viewport.refresh = TRUE;

				if (mciVideo.deviceId)
				{
					RECT rc;
					GetClientRect(hWnd, &rc);

					MCI_OVLY_RECT_PARMS params;
					params.dwCallback = (DWORD_PTR)hWnd;
					Hooks::CalcVideoSize(rc.right, rc.bottom, &params.rc);
					MciSendCommand(mciList[mciVideo.deviceId - 1], MCI_PUT, MCI_OVLY_RECT | MCI_OVLY_PUT_DESTINATION, (DWORD_PTR)&params);
				}
				return NULL;
			}

			case IDM_HELP_ABOUT:
			{
				INT_PTR res;
				ULONG_PTR cookie = NULL;
				if (hActCtx && hActCtx != INVALID_HANDLE_VALUE && !ActivateActCtxC(hActCtx, &cookie))
					cookie = NULL;

				res = DialogBoxParam(hDllModule, cookie ? MAKEINTRESOURCE(IDD_ABOUT) : MAKEINTRESOURCE(IDD_ABOUT_OLD), hWnd, (DLGPROC)AboutProc, NULL);

				if (cookie)
					DeactivateActCtxC(0, cookie);

				SetForegroundWindow(hWnd);
				return NULL;
			}

			default:
				return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
			}
		}

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			if (HIWORD(lParam) & KF_ALTDOWN)
			{
				if (wParam == 'W')
					WindowProc(hWnd, WM_COMMAND, IDM_WINDOW_FULLSCREEN, NULL);
				else if (wParam == 'F')
					WindowProc(hWnd, WM_COMMAND, IDM_WINDOW_FPSCOUNTER, NULL);
				else if (wParam == 'M')
					WindowProc(hWnd, WM_COMMAND, IDM_WINDOW_MOUSECAPTURE, NULL);
				else if (wParam == VK_F4)
					WindowProc(hWnd, WM_COMMAND, IDM_WINDOW_EXIT, NULL);

				return NULL;
			}

			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}

		case WM_MOUSEMOVE:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
				ddraw->ScaleMouse(uMsg, &lParam);

			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}

		case WM_LBUTTONDOWN:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
			{
				ddraw->mbPressed |= MK_LBUTTON;
				ddraw->ScaleMouse(uMsg, &lParam);
			}

			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}

		case WM_RBUTTONDOWN:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
			{
				ddraw->mbPressed |= MK_RBUTTON;
				ddraw->ScaleMouse(uMsg, &lParam);
			}

			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}

		case WM_MBUTTONDOWN:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
			{
				ddraw->mbPressed |= MK_MBUTTON;
				ddraw->ScaleMouse(uMsg, &lParam);
			}

			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}

		case WM_LBUTTONUP:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
			{
				ddraw->mbPressed ^= MK_LBUTTON;
				ddraw->ScaleMouse(uMsg, &lParam);
			}

			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}

		case WM_RBUTTONUP:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
			{
				ddraw->mbPressed ^= MK_RBUTTON;
				ddraw->ScaleMouse(uMsg, &lParam);
			}

			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}

		case WM_MBUTTONUP:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
			{
				ddraw->mbPressed ^= MK_MBUTTON;
				ddraw->ScaleMouse(uMsg, &lParam);
			}

			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}

		case WM_SETCURSOR:
		{
			if (LOWORD(lParam) == HTCLIENT)
			{
				if (!config.windowedMode || !config.aspectRatio)
				{
					SetCursor(NULL);
					return TRUE;
				}
				else
				{
					DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
					if (ddraw)
					{
						POINT p;
						GetCursorPos(&p);
						ScreenToClient(hWnd, &p);

						if (p.x >= ddraw->viewport.rectangle.x && p.x < ddraw->viewport.rectangle.x + ddraw->viewport.rectangle.width &&
							p.y >= ddraw->viewport.rectangle.y && p.y < ddraw->viewport.rectangle.y + ddraw->viewport.rectangle.height)
							SetCursor(NULL);
						else
							SetCursor(config.cursor);

						return TRUE;
					}
				}
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
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

	LRESULT __stdcall PanelProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_SYSCOMMAND:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_CHAR:
		case WM_SETCURSOR:
			return WindowProc(GetParent(hWnd), uMsg, wParam, lParam);

		default:
			return CallWindowProc(OldPanelProc, hWnd, uMsg, wParam, lParam);
		}
	}

	VOID __fastcall CheckMenu()
	{
		CheckMenuItem(config.menu, IDM_WINDOW_FULLSCREEN, MF_BYCOMMAND | (!config.windowedMode ? MF_CHECKED : MF_UNCHECKED));

		EnableMenuItem(config.menu, IDM_WINDOW_VSYNC, MF_BYCOMMAND | (glVersion && !config.singleThread && WGLSwapInterval ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
		CheckMenuItem(config.menu, IDM_WINDOW_VSYNC, MF_BYCOMMAND | (glVersion && !config.singleThread && WGLSwapInterval && config.vSync ? MF_CHECKED : MF_UNCHECKED));

		CheckMenuItem(config.menu, IDM_WINDOW_FPSCOUNTER, MF_BYCOMMAND | (config.fpsCounter ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(config.menu, IDM_WINDOW_MOUSECAPTURE, MF_BYCOMMAND | (config.mouseCapture ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(config.menu, IDM_IMAGE_FILTERING, MF_BYCOMMAND | (config.filtering ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(config.menu, IDM_IMAGE_ASPECTRATIO, MF_BYCOMMAND | (config.aspectRatio ? MF_CHECKED : MF_UNCHECKED));
	}
#pragma optimize("", on)

	VOID __fastcall SetCaptureKeys(BOOL state)
	{
		if (state)
		{
			if (!OldKeysHook)
				OldKeysHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeysHook, hDllModule, NULL);
		}
		else
		{
			if (OldKeysHook && UnhookWindowsHookEx(OldKeysHook))
				OldKeysHook = NULL;
		}
	}

	VOID __fastcall SetCaptureMouse(BOOL state)
	{
		if (state)
		{
			if (config.mouseCapture && !OldMouseHook)
				OldMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHook, hDllModule, NULL);
		}
		else
		{
			if (OldMouseHook && UnhookWindowsHookEx(OldMouseHook))
				OldMouseHook = NULL;
		}
	}

	VOID __fastcall SetCaptureWindow(HWND hWnd)
	{
		OldWindowProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);
	}

	VOID __fastcall SetCapturePanel(HWND hWnd)
	{
		OldPanelProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)PanelProc);
	}
}