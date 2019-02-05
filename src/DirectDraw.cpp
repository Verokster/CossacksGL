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
#include "resource.h"
#include "math.h"
#include "Windowsx.h"
#include "Shellapi.h"
#include "CommCtrl.h"
#include "Main.h"
#include "Config.h"
#include "Hooks.h"
#include "DirectDraw.h"
#include "DirectDrawSurface.h"
#include "DirectDrawInterface.h"
#include "FpsCounter.h"

#pragma region Not Implemented
ULONG DirectDraw::AddRef() { return 0; }
HRESULT DirectDraw::Compact() { return DD_OK; }
HRESULT DirectDraw::CreateClipper(DWORD, LPDIRECTDRAWCLIPPER*, IUnknown*) { return DD_OK; }
HRESULT DirectDraw::EnumSurfaces(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMSURFACESCALLBACK) { return DD_OK; }
HRESULT DirectDraw::FlipToGDISurface(void) { return DD_OK; }
HRESULT DirectDraw::GetCaps(LPDDCAPS, LPDDCAPS) { return DD_OK; }
HRESULT DirectDraw::GetDisplayMode(LPDDSURFACEDESC) { return DD_OK; }
HRESULT DirectDraw::GetFourCCCodes(LPDWORD, LPDWORD) { return DD_OK; }
HRESULT DirectDraw::GetGDISurface(LPDIRECTDRAWSURFACE*) { return DD_OK; }
HRESULT DirectDraw::GetMonitorFrequency(LPDWORD) { return DD_OK; }
HRESULT DirectDraw::GetScanLine(LPDWORD) { return DD_OK; }
HRESULT DirectDraw::GetVerticalBlankStatus(LPBOOL) { return DD_OK; }
HRESULT DirectDraw::Initialize(GUID*) { return DD_OK; }
HRESULT DirectDraw::RestoreDisplayMode() { return DD_OK; }
HRESULT DirectDraw::DuplicateSurface(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE*) { return DD_OK; }
HRESULT DirectDraw::WaitForVerticalBlank(DWORD, HANDLE) { return DD_OK; }
#pragma endregion

#define RECOUNT 32 
#define WHITE 0xFFFFFFFF;

#define MIN_WIDTH 240
#define MIN_HEIGHT 180

DisplayMode resolutionsList[32];

WNDPROC OldWindowProc, OldPanelProc;
HHOOK OldMouseHook;

VOID CheckMenu()
{
	CheckMenuItem(config.menu, IDM_WINDOW_FULLSCREEN, MF_BYCOMMAND | (!config.windowedMode ? MF_CHECKED : MF_UNCHECKED));

	EnableMenuItem(config.menu, IDM_WINDOW_VSYNC, MF_BYCOMMAND | (glVersion && WGLSwapInterval ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	CheckMenuItem(config.menu, IDM_WINDOW_VSYNC, MF_BYCOMMAND | (glVersion && WGLSwapInterval && config.vSync ? MF_CHECKED : MF_UNCHECKED));

	CheckMenuItem(config.menu, IDM_WINDOW_FPSCOUNTER, MF_BYCOMMAND | (config.fpsCounter ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(config.menu, IDM_WINDOW_MOUSECAPTURE, MF_BYCOMMAND | (config.mouseCapture ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(config.menu, IDM_IMAGE_FILTERING, MF_BYCOMMAND | (config.filtering ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(config.menu, IDM_IMAGE_ASPECTRATIO, MF_BYCOMMAND | (config.aspectRatio ? MF_CHECKED : MF_UNCHECKED));
}

LRESULT __stdcall MouseHook(INT nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		DirectDraw* ddraw = ddrawList;
		while (ddraw)
		{
			ddraw->CaptureMouse((UINT)wParam, (LPMSLLHOOKSTRUCT)lParam);
			ddraw = ddraw->last;
		}
	}

	return CallNextHookEx(OldMouseHook, nCode, wParam, lParam);
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

		CHAR email[50];
		GetDlgItemText(hDlg, IDC_LNK_EMAIL, email, sizeof(email) - 1);
		CHAR anchor[256];
		StrPrint(anchor, "<A HREF=\"mailto:%s\">%s</A>", email, email);
		SetDlgItemText(hDlg, IDC_LNK_EMAIL, anchor);

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

	case WM_MOVE:
	{
		DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
		if (ddraw && ddraw->hDraw)
		{
			DWORD stye = GetWindowLong(ddraw->hDraw, GWL_STYLE);
			if (stye & WS_POPUP)
			{
				POINT point = { LOWORD(lParam), HIWORD(lParam) };
				ScreenToClient(hWnd, &point);

				RECT rect;
				rect.left = point.x - LOWORD(lParam);
				rect.top = point.y - HIWORD(lParam);
				rect.right = rect.left + 256;
				rect.bottom = rect.left + 256;

				AdjustWindowRect(&rect, stye, TRUE);
				SetWindowPos(ddraw->hDraw, NULL, rect.left, rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOREPOSITION | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
			}
			else
				SetWindowPos(ddraw->hDraw, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOREPOSITION | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
		}

		return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
	}

	case WM_SIZE:
	{
		DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
		if (ddraw)
		{
			if (ddraw->hDraw)
				SetWindowPos(ddraw->hDraw, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOREPOSITION | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

			if (ddraw->dwMode)
			{
				ddraw->viewport.width = LOWORD(lParam);
				ddraw->viewport.height = HIWORD(lParam);
				ddraw->viewport.refresh = TRUE;
			}
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
			else
				config.windowedMode = !config.windowedMode;

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

			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}

		case IDM_HELP_ABOUT:
		{
			INT_PTR res;
			ULONG_PTR cookie = NULL;
			if (hActCtx && hActCtx != INVALID_HANDLE_VALUE && !ActivateActCtxC(hActCtx, &cookie))
				cookie = NULL;

			res = DialogBoxParam(hDllModule, MAKEINTRESOURCE(IDD_ABOUT), hWnd, (DLGPROC)AboutProc, NULL);

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
		if (wParam == VK_F11 || wParam == VK_RETURN && (HIWORD(lParam) & KF_ALTDOWN))
		{
			WindowProc(hWnd, WM_COMMAND, IDM_WINDOW_FULLSCREEN, NULL);
			return NULL;
		}
		else if (wParam == 'F' && (HIWORD(lParam) & KF_ALTDOWN))
		{
			WindowProc(hWnd, WM_COMMAND, IDM_WINDOW_FPSCOUNTER, NULL);
			return NULL;
		}
		else if (wParam == 'M' && (HIWORD(lParam) & KF_ALTDOWN))
		{
			WindowProc(hWnd, WM_COMMAND, IDM_WINDOW_MOUSECAPTURE, NULL);
			return NULL;
		}
		else if (wParam == VK_F1)
		{
			if (!config.windowedMode)
				WindowProc(hWnd, WM_COMMAND, IDM_WINDOW_FULLSCREEN, NULL);

			WindowProc(hWnd, WM_COMMAND, IDM_HELP_ABOUT, NULL);
			return NULL;
		}

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

	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MOUSEMOVE:
	{
		DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
		if (ddraw)
			ddraw->ScaleMouse(uMsg, &lParam);

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
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
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

DWORD __stdcall RenderThread(LPVOID lpParameter)
{
	DirectDraw* ddraw = (DirectDraw*)lpParameter;
	ddraw->hDc = ::GetDC(ddraw->hDraw);
	if (ddraw->hDc)
	{
		PIXELFORMATDESCRIPTOR pfd;
		GL::PreparePixelFormatDescription(&pfd);
		INT glPixelFormat = GL::PreparePixelFormat(&pfd);
		if (!glPixelFormat)
		{
			glPixelFormat = ChoosePixelFormat(ddraw->hDc, &pfd);
			if (!glPixelFormat)
				Main::ShowError("ChoosePixelFormat failed", __FILE__, __LINE__);
			else if (pfd.dwFlags & PFD_NEED_PALETTE)
				Main::ShowError("Needs palette", __FILE__, __LINE__);
		}

		if (!SetPixelFormat(ddraw->hDc, glPixelFormat, &pfd))
			Main::ShowError("SetPixelFormat failed", __FILE__, __LINE__);

		MemoryZero(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		if (DescribePixelFormat(ddraw->hDc, glPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd) == NULL)
			Main::ShowError("DescribePixelFormat failed", __FILE__, __LINE__);

		if ((pfd.iPixelType != PFD_TYPE_RGBA) ||
			(pfd.cRedBits < 5) || (pfd.cGreenBits < 5) || (pfd.cBlueBits < 5))
			Main::ShowError("Bad pixel type", __FILE__, __LINE__);

		HGLRC hRc = WGLCreateContext(ddraw->hDc);
		if (hRc)
		{
			if (WGLMakeCurrent(ddraw->hDc, hRc))
			{
				GL::CreateContextAttribs(ddraw->hDc, &hRc);
				if (glVersion >= GL_VER_2_0)
				{
					DWORD maxSize = ddraw->dwMode->width > ddraw->dwMode->height ? ddraw->dwMode->width : ddraw->dwMode->height;

					DWORD maxTexSize = 1;
					while (maxTexSize < maxSize)
						maxTexSize <<= 1;

					DWORD glMaxTexSize;
					GLGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&glMaxTexSize);
					if (maxTexSize > glMaxTexSize)
						glVersion = GL_VER_1_1;
				}

				CheckMenu();
				if (glVersion >= GL_VER_2_0)
					ddraw->RenderNew();
				else
					ddraw->RenderOld();

				WGLMakeCurrent(ddraw->hDc, NULL);
			}

			WGLDeleteContext(hRc);
		}

		::ReleaseDC(ddraw->hDraw, ddraw->hDc);
		ddraw->hDc = NULL;
	}

	return NULL;
}

VOID DirectDraw::RenderOld()
{
	DWORD glMaxTexSize;
	GLGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&glMaxTexSize);
	if (glMaxTexSize < 256)
		glMaxTexSize = 256;

	DWORD size = this->dwMode->width > this->dwMode->height ? this->dwMode->width : this->dwMode->height;
	DWORD maxAllow = 1;
	while (maxAllow < size)
		maxAllow <<= 1;

	DWORD maxTexSize = maxAllow < glMaxTexSize ? maxAllow : glMaxTexSize;

	VOID* pixelBuffer = MemoryAlloc(maxTexSize * maxTexSize * sizeof(DWORD));
	{
		DWORD framePerWidth = this->dwMode->width / maxTexSize + (this->dwMode->width % maxTexSize ? 1 : 0);
		DWORD framePerHeight = this->dwMode->height / maxTexSize + (this->dwMode->height % maxTexSize ? 1 : 0);
		DWORD frameCount = framePerWidth * framePerHeight;
		Frame* frames = (Frame*)MemoryAlloc(frameCount * sizeof(Frame));
		{
			Frame* frame = frames;
			for (DWORD y = 0; y < this->dwMode->height; y += maxTexSize)
			{
				DWORD height = this->dwMode->height - y;
				if (height > maxTexSize)
					height = maxTexSize;

				for (DWORD x = 0; x < this->dwMode->width; x += maxTexSize, ++frame)
				{
					DWORD width = this->dwMode->width - x;
					if (width > maxTexSize)
						width = maxTexSize;

					frame->rect.x = x;
					frame->rect.y = y;
					frame->rect.width = width;
					frame->rect.height = height;

					frame->vSize.width = x + width;
					frame->vSize.height = y + height;

					frame->tSize.width = width == maxTexSize ? 1.0f : (FLOAT)width / maxTexSize;
					frame->tSize.height = height == maxTexSize ? 1.0f : (FLOAT)height / maxTexSize;

					GLGenTextures(1, &frame->id);
					GLBindTexture(GL_TEXTURE_2D, frame->id);

					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glCapsClampToEdge);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glCapsClampToEdge);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

					GLint filter = config.windowedMode && config.filtering ? GL_LINEAR : GL_NEAREST;
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);

					GLTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

					if (this->dwMode->bpp == 8 && GLColorTable)
						GLTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, maxTexSize, maxTexSize, GL_NONE, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, NULL);
					else
						GLTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, maxTexSize, maxTexSize, GL_NONE, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				}
			}

			GLClearColor(0.0, 0.0, 0.0, 1.0);
			this->viewport.refresh = TRUE;

			GLMatrixMode(GL_PROJECTION);
			GLLoadIdentity();
			GLOrtho(0.0, this->dwMode->width, this->dwMode->height, 0.0, 0.0, 1.0);
			GLMatrixMode(GL_MODELVIEW);
			GLLoadIdentity();

			GLEnable(GL_TEXTURE_2D);

			if (this->dwMode->bpp == 8 && glCapsSharedPalette)
				GLEnable(GL_SHARED_TEXTURE_PALETTE_EXT);

			FpsCounter* fpsCounter = new FpsCounter(FPS_ACCURACY);
			{
				BOOL isVSync = FALSE;
				if (WGLSwapInterval)
					WGLSwapInterval(0);

				do
				{
					if (this->dwMode->bpp == 8 || !mciVideo.deviceId)
					{
						if (WGLSwapInterval)
						{
							if (!isVSync)
							{
								if (config.vSync)
								{
									isVSync = TRUE;
									WGLSwapInterval(1);
								}
							}
							else
							{
								if (!config.vSync)
								{
									isVSync = FALSE;
									WGLSwapInterval(0);
								}
							}
						}

						this->CheckView();

						if (this->isStateChanged)
						{
							this->isStateChanged = FALSE;
							GLint filter = config.windowedMode && config.filtering ? GL_LINEAR : GL_NEAREST;
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
						}

						if (this->dwMode->bpp == 8)
						{
							if (config.fpsCounter)
							{
								if (isFpsChanged)
								{
									isFpsChanged = FALSE;
									fpsCounter->Reset();
								}

								fpsCounter->Calculate();
							}

							if (glCapsSharedPalette && this->isPalChanged)
							{
								this->isPalChanged = FALSE;
								GLColorTable(GL_TEXTURE_2D, GL_RGBA8, 256, GL_RGBA, GL_UNSIGNED_BYTE, this->palette);
							}

							DWORD count = frameCount;
							frame = frames;
							while (count--)
							{
								GLBindTexture(GL_TEXTURE_2D, frame->id);

								if (GLColorTable)
								{
									if (!glCapsSharedPalette)
										GLColorTable(GL_TEXTURE_2D, GL_RGBA8, 256, GL_RGBA, GL_UNSIGNED_BYTE, this->palette);

									BYTE* pix = (BYTE*)pixelBuffer;
									for (INT y = frame->rect.y; y < frame->vSize.height; ++y)
									{
										BYTE* idx = this->indexBuffer + y * this->dwMode->width + frame->rect.x;
										MemoryCopy(pix, idx, frame->rect.width);
										pix += frame->rect.width;
									}

									if (config.fpsCounter && count == frameCount - 1)
									{
										DWORD fps = fpsCounter->value;
										DWORD digCount = 0;
										DWORD current = fps;
										do
										{
											++digCount;
											current = current / 10;
										} while (current);

										DWORD dcount = digCount;
										current = fps;
										do
										{
											WORD* lpDig = (WORD*)counters[current % 10];

											for (DWORD y = 0; y < FPS_HEIGHT; ++y)
											{
												BYTE* pix = (BYTE*)pixelBuffer + (FPS_Y + y) * frame->rect.width +
													FPS_X + FPS_WIDTH * (dcount - 1);

												WORD check = *lpDig++;
												DWORD width = FPS_WIDTH;
												do
												{
													if (check & 1)
														*pix = 0xFF;
													++pix;
													check >>= 1;
												} while (--width);
											}

											current = current / 10;
										} while (--dcount);
									}

									GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->rect.width, frame->rect.height, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, pixelBuffer);
								}
								else
								{
									DWORD* pix = (DWORD*)pixelBuffer;
									for (INT y = frame->rect.y; y < frame->vSize.height; ++y)
									{
										BYTE* idx = this->indexBuffer + y * this->dwMode->width + frame->rect.x;
										for (INT x = frame->rect.x; x < frame->vSize.width; ++x)
											*pix++ = this->palette[*idx++];
									}

									if (config.fpsCounter && count == frameCount - 1)
									{
										DWORD fps = fpsCounter->value;
										DWORD digCount = 0;
										DWORD current = fps;
										do
										{
											++digCount;
											current = current / 10;
										} while (current);

										DWORD dcount = digCount;
										current = fps;
										do
										{
											DWORD digit = current % 10;
											WORD* lpDig = (WORD*)counters[digit];

											for (DWORD y = 0; y < FPS_HEIGHT; ++y)
											{
												DWORD* pix = (DWORD*)pixelBuffer + (FPS_Y + y) * frame->rect.width +
													FPS_X + FPS_WIDTH * (dcount - 1);

												WORD check = *lpDig++;
												DWORD width = FPS_WIDTH;
												do
												{
													if (check & 1)
														*pix = 0xFFFFFFFF;
													++pix;
													check >>= 1;
												} while (--width);
											}

											current = current / 10;
										} while (--dcount);
									}

									GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->rect.width, frame->rect.height, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer);
								}

								GLBegin(GL_TRIANGLE_FAN);
								{
									GLTexCoord2f(0.0, 0.0);
									GLVertex2s(LOWORD(frame->rect.x), LOWORD(frame->rect.y));

									GLTexCoord2f(frame->tSize.width, 0.0);
									GLVertex2s(LOWORD(frame->vSize.width), LOWORD(frame->rect.y));

									GLTexCoord2f(frame->tSize.width, frame->tSize.height);
									GLVertex2s(LOWORD(frame->vSize.width), LOWORD(frame->vSize.height));

									GLTexCoord2f(0.0, frame->tSize.height);
									GLVertex2s(LOWORD(frame->rect.x), LOWORD(frame->vSize.height));
								}
								GLEnd();
								++frame;
							}
						}
						else
						{
							DWORD count = frameCount;
							frame = frames;
							while (count--)
							{
								GLBindTexture(GL_TEXTURE_2D, frame->id);

								DWORD* pix = (DWORD*)pixelBuffer;
								for (INT y = frame->rect.y; y < frame->vSize.height; ++y)
								{
									DWORD* idx = (DWORD*)this->indexBuffer + y * this->dwMode->width + frame->rect.x;
									for (INT x = frame->rect.x; x < frame->vSize.width; ++x)
										*pix++ = *idx++;
								}

								GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->rect.width, frame->rect.height, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer);

								GLBegin(GL_TRIANGLE_FAN);
								{
									GLTexCoord2f(0.0, 0.0);
									GLVertex2s(LOWORD(frame->rect.x), LOWORD(frame->rect.y));

									GLTexCoord2f(frame->tSize.width, 0.0);
									GLVertex2s(LOWORD(frame->vSize.width), LOWORD(frame->rect.y));

									GLTexCoord2f(frame->tSize.width, frame->tSize.height);
									GLVertex2s(LOWORD(frame->vSize.width), LOWORD(frame->vSize.height));

									GLTexCoord2f(0.0, frame->tSize.height);
									GLVertex2s(LOWORD(frame->rect.x), LOWORD(frame->vSize.height));
								}
								GLEnd();
								++frame;
							}
						}

						SwapBuffers(this->hDc);
						if (isVSync)
							GLFinish();

						if (this->isTakeSnapshot)
						{
							this->isTakeSnapshot = FALSE;
							this->TakeScreenshot();
						}
					}
					else
					{
						Sleep(1);

						if (this->isTakeSnapshot)
							this->TakeScreenshot();
					}
				} while (!this->isFinish);
				GLFinish();
			}
			delete fpsCounter;

			frame = frames;
			DWORD count = frameCount;
			while (count--)
			{
				GLDeleteTextures(1, &frame->id);
				++frame;
			}
		}
		MemoryFree(frames);
	}
	MemoryFree(pixelBuffer);
}

VOID __fastcall UseShaderProgram(ShaderProgram* program, DWORD texSize)
{
	if (!program->id)
	{
		program->id = GLCreateProgram();

		GLuint vShader = GL::CompileShaderSource(program->vertexName, program->version, GL_VERTEX_SHADER);
		GLuint fShader = GL::CompileShaderSource(program->fragmentName, program->version, GL_FRAGMENT_SHADER);
		{

			GLAttachShader(program->id, vShader);
			GLAttachShader(program->id, fShader);
			{
				GLLinkProgram(program->id);
			}
			GLDetachShader(program->id, fShader);
			GLDetachShader(program->id, vShader);
		}
		GLDeleteShader(fShader);
		GLDeleteShader(vShader);

		GLUseProgram(program->id);
		GLUniformMatrix4fv(GLGetUniformLocation(program->id, "mvp"), 1, GL_FALSE, program->mvp);
		GLUniform1i(GLGetUniformLocation(program->id, "tex01"), GL_TEXTURE0 - GL_TEXTURE0);

		GLint loc = GLGetUniformLocation(program->id, "pal01");
		if (loc >= 0)
			GLUniform1i(loc, GL_TEXTURE1 - GL_TEXTURE0);

		loc = GLGetUniformLocation(program->id, "texSize");
		if (loc >= 0)
			GLUniform2f(loc, (FLOAT)texSize, (FLOAT)texSize);
	}
	else
		GLUseProgram(program->id);
}

VOID DirectDraw::RenderNew()
{
	DWORD maxSize = this->dwMode->width > this->dwMode->height ? this->dwMode->width : this->dwMode->height;
	DWORD maxTexSize = 1;
	while (maxTexSize < maxSize) maxTexSize <<= 1;
	FLOAT texWidth = this->dwMode->width == maxTexSize ? 1.0f : FLOAT((FLOAT)this->dwMode->width / maxTexSize);
	FLOAT texHeight = this->dwMode->height == maxTexSize ? 1.0f : FLOAT((FLOAT)this->dwMode->height / maxTexSize);

	FLOAT buffer[4][4] = {
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		{ (FLOAT)this->dwMode->width, 0.0f, texWidth, 0.0f },
		{ (FLOAT)this->dwMode->width, (FLOAT)this->dwMode->height, texWidth, texHeight },
		{ 0.0f, (FLOAT)this->dwMode->height, 0.0f, texHeight }
	};

	FLOAT mvpMatrix[4][4] = {
		{ FLOAT(2.0f / this->dwMode->width), 0.0f, 0.0f, 0.0f },
		{ 0.0f, FLOAT(-2.0f / this->dwMode->height), 0.0f, 0.0f },
		{ 0.0f, 0.0f, 2.0f, 0.0f },
		{ -1.0f, 1.0f, -1.0f, 1.0f }
	};

	const CHAR* glslVersion = glVersion >= GL_VER_3_0 ? GLSL_VER_1_30 : GLSL_VER_1_10;

	struct {
		ShaderProgram simple;
		ShaderProgram nearest;
		ShaderProgram linear;
	} shaders = {
		{ 0, glslVersion, IDR_VERTEX_SIMPLE, IDR_FRAGMENT_SIMPLE, (GLfloat*)mvpMatrix },
		{ 0, glslVersion, IDR_VERTEX_NEAREST, IDR_FRAGMENT_NEAREST, (GLfloat*)mvpMatrix },
		{ 0, glslVersion, IDR_VERTEX_LINEAR, IDR_FRAGMENT_LINEAR, (GLfloat*)mvpMatrix },
	};

	ShaderProgram* program = this->dwMode->bpp != 8 ? &shaders.simple : (config.windowedMode && config.filtering ? program = &shaders.linear : &shaders.nearest);
	{
		GLuint arrayName;
		if (glVersion >= GL_VER_3_0)
		{
			GLGenVertexArrays(1, &arrayName);
			GLBindVertexArray(arrayName);
		}

		GLuint bufferName;
		GLGenBuffers(1, &bufferName);
		{
			GLBindBuffer(GL_ARRAY_BUFFER, bufferName);
			{
				GLBufferData(GL_ARRAY_BUFFER, sizeof(buffer), buffer, GL_STATIC_DRAW);

				UseShaderProgram(program, maxTexSize);
				GLint attrCoordsLoc = GLGetAttribLocation(program->id, "vCoord");
				GLEnableVertexAttribArray(attrCoordsLoc);
				GLVertexAttribPointer(attrCoordsLoc, 2, GL_FLOAT, GL_FALSE, 16, (GLvoid*)0);

				GLint attrTexCoordsLoc = GLGetAttribLocation(program->id, "vTexCoord");
				GLEnableVertexAttribArray(attrTexCoordsLoc);
				GLVertexAttribPointer(attrTexCoordsLoc, 2, GL_FLOAT, GL_FALSE, 16, (GLvoid*)8);

				GLClearColor(0.0, 0.0, 0.0, 1.0);
				this->viewport.refresh = TRUE;
				this->isStateChanged = TRUE;
				this->isPalChanged = TRUE;

				FpsCounter* fpsCounter = new FpsCounter(FPS_ACCURACY);
				{
					BOOL isVSync = FALSE;
					if (WGLSwapInterval)
						WGLSwapInterval(0);

					if (this->dwMode->bpp == 8)
					{
						GLuint textures[2];
						GLuint paletteId = textures[0];
						GLuint indicesId = textures[1];
						GLGenTextures(2, textures);
						{
							GLActiveTexture(GL_TEXTURE1);
							GLBindTexture(GL_TEXTURE_1D, paletteId);

							GLTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, glCapsClampToEdge);
							GLTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_BASE_LEVEL, 0);
							GLTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_LEVEL, 0);
							GLTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
							GLTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
							GLTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, 256, GL_NONE, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

							GLActiveTexture(GL_TEXTURE0);
							GLBindTexture(GL_TEXTURE_2D, indicesId);

							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glCapsClampToEdge);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glCapsClampToEdge);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
							GLTexImage2D(GL_TEXTURE_2D, 0, GL_R8, maxTexSize, maxTexSize, GL_NONE, GL_RED, GL_UNSIGNED_BYTE, NULL);

							VOID* pixelBuffer = MemoryAlloc(FPS_HEIGHT * FPS_WIDTH * 4);
							{
								do
								{
									if (WGLSwapInterval)
									{
										if (!isVSync)
										{
											if (config.vSync)
											{
												isVSync = TRUE;
												WGLSwapInterval(1);
											}
										}
										else
										{
											if (!config.vSync)
											{
												isVSync = FALSE;
												WGLSwapInterval(0);
											}
										}
									}

									if (config.fpsCounter)
									{
										if (isFpsChanged)
										{
											isFpsChanged = FALSE;
											fpsCounter->Reset();
										}

										fpsCounter->Calculate();
									}

									this->CheckView();

									if (this->isStateChanged)
									{
										this->isStateChanged = FALSE;
										UseShaderProgram(config.windowedMode && config.filtering ? program = &shaders.linear : &shaders.nearest, maxTexSize);
									}

									if (this->isPalChanged)
									{
										this->isPalChanged = FALSE;

										GLActiveTexture(GL_TEXTURE1);
										GLBindTexture(GL_TEXTURE_1D, paletteId);
										GLTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA, GL_UNSIGNED_BYTE, this->palette);

										GLActiveTexture(GL_TEXTURE0);
										GLBindTexture(GL_TEXTURE_2D, indicesId);
									}

									GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->dwMode->width, this->dwMode->height, GL_RED, GL_UNSIGNED_BYTE, this->indexBuffer);

									if (config.fpsCounter)
									{
										DWORD fps = fpsCounter->value;
										DWORD digCount = 0;
										DWORD current = fps;
										do
										{
											++digCount;
											current = current / 10;
										} while (current);

										DWORD dcount = digCount;
										current = fps;
										do
										{
											DWORD digit = current % 10;
											WORD* lpDig = (WORD*)counters[digit];

											for (DWORD y = 0; y < FPS_HEIGHT; ++y)
											{
												BYTE* idx = this->indexBuffer + (FPS_Y + y) * this->dwMode->width +
													FPS_X + FPS_WIDTH * (dcount - 1);

												BYTE* pix = (BYTE*)pixelBuffer + y * FPS_WIDTH * digCount +
													FPS_WIDTH * (dcount - 1);

												WORD check = *lpDig++;
												DWORD width = FPS_WIDTH;
												do
												{
													*pix++ = (check & 1) ? 0xFF : *idx;
													++idx;
													check >>= 1;
												} while (--width);
											}

											current = current / 10;
										} while (--dcount);

										GLTexSubImage2D(GL_TEXTURE_2D, 0, FPS_X, FPS_Y, FPS_WIDTH * digCount, FPS_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, pixelBuffer);
									}

									GLDrawArrays(GL_TRIANGLE_FAN, 0, 4);
									SwapBuffers(this->hDc);
									if (isVSync)
										GLFinish();

									if (this->isTakeSnapshot)
									{
										this->isTakeSnapshot = FALSE;
										this->TakeScreenshot();
									}
								} while (!this->isFinish);
								GLFinish();
							}
							MemoryFree(pixelBuffer);
						}
						GLDeleteTextures(2, textures);
					}
					else
					{
						GLuint textureId;
						GLGenTextures(1, &textureId);
						{
							GLActiveTexture(GL_TEXTURE0);
							GLBindTexture(GL_TEXTURE_2D, textureId);

							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glCapsClampToEdge);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glCapsClampToEdge);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

							GLint filter = config.windowedMode && config.filtering ? GL_LINEAR : GL_NEAREST;
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);

							GLTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, maxTexSize, maxTexSize, GL_NONE, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

							do
							{
								if (mciVideo.deviceId)
								{
									Sleep(1);

									if (this->isTakeSnapshot)
										this->isTakeSnapshot = FALSE;
								}
								else
								{
									if (WGLSwapInterval)
									{
										if (!isVSync)
										{
											if (config.vSync)
											{
												isVSync = TRUE;
												WGLSwapInterval(1);
											}
										}
										else
										{
											if (!config.vSync)
											{
												isVSync = FALSE;
												WGLSwapInterval(0);
											}
										}
									}

									this->CheckView();

									if (this->isStateChanged)
									{
										this->isStateChanged = FALSE;
										GLint filter = config.windowedMode && config.filtering ? GL_LINEAR : GL_NEAREST;
										GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
										GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
									}

									GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->dwMode->width, this->dwMode->height, GL_RGBA, GL_UNSIGNED_BYTE, this->indexBuffer);

									GLDrawArrays(GL_TRIANGLE_FAN, 0, 4);
									SwapBuffers(this->hDc);
									if (isVSync)
										GLFinish();

									if (this->isTakeSnapshot)
									{
										this->isTakeSnapshot = FALSE;
										this->TakeScreenshot();
									}
								}
							} while (!this->isFinish);
							GLFinish();
						}
						GLDeleteTextures(1, &textureId);
					}
				}
				delete fpsCounter;
			}
			GLBindBuffer(GL_ARRAY_BUFFER, NULL);
		}
		GLDeleteBuffers(1, &bufferName);

		if (glVersion >= GL_VER_3_0)
		{
			GLBindVertexArray(NULL);
			GLDeleteVertexArrays(1, &arrayName);
		}
	}
	GLUseProgram(NULL);

	program = (ShaderProgram*)&shaders;
	DWORD count = sizeof(shaders) / sizeof(ShaderProgram);
	do
	{
		if (program->id)
			GLDeleteProgram(program->id);
	} while (--count);
}

VOID DirectDraw::TakeScreenshot()
{
	if (OpenClipboard(NULL))
	{
		EmptyClipboard();

		DWORD dataCount = this->viewport.rectangle.width * this->viewport.rectangle.height;
		DWORD dataSize = dataCount * 3;
		HGLOBAL hMemory = GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + dataSize);
		{
			VOID* data = GlobalLock(hMemory);
			{
				MemoryZero(data, sizeof(BITMAPINFOHEADER));

				BITMAPINFOHEADER* bmi = (BITMAPINFOHEADER*)data;
				bmi->biSize = sizeof(BITMAPINFOHEADER);
				bmi->biWidth = this->viewport.rectangle.width;
				bmi->biHeight = this->viewport.rectangle.height;
				bmi->biPlanes = 1;
				bmi->biBitCount = 24;
				bmi->biCompression = BI_RGB;
				bmi->biXPelsPerMeter = 1;
				bmi->biYPelsPerMeter = 1;

				BYTE* dst = (BYTE*)data + sizeof(BITMAPINFOHEADER);
				GLReadPixels(this->viewport.rectangle.x, this->viewport.rectangle.y, this->viewport.rectangle.width, this->viewport.rectangle.height, glCapsBGR ? GL_BGR_EXT : GL_RGB, GL_UNSIGNED_BYTE, dst);
				if (!glCapsBGR)
				{
					do
					{
						BYTE val = *dst;
						*dst = *(dst + 2);
						*(dst + 2) = val;
						dst += 3;
					} while (--dataCount);
				}
			}
			GlobalUnlock(hMemory);

			SetClipboardData(CF_DIB, hMemory);
			MessageBeep(1);
		}
		GlobalFree(hMemory);

		CloseClipboard();
	}
}

VOID DirectDraw::RenderStart()
{
	if (!this->isFinish || !this->hWnd)
		return;

	this->isFinish = FALSE;
	GL::Load();

	RECT rect;
	GetClientRect(this->hWnd, &rect);

	if (!config.windowedMode)
	{
		this->hDraw = CreateWindowEx(
			WS_EX_CONTROLPARENT | WS_EX_TOPMOST,
			WC_DRAW,
			NULL,
			mciVideo.deviceId ? WS_POPUP : (WS_VISIBLE | WS_POPUP),
			rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top,
			this->hWnd,
			NULL,
			hDllModule,
			NULL);
	}
	else
	{
		this->hDraw = CreateWindowEx(
			WS_EX_CONTROLPARENT,
			WC_DRAW,
			NULL,
			mciVideo.deviceId ? (WS_CHILD | WS_MAXIMIZE) : (WS_VISIBLE | WS_CHILD | WS_MAXIMIZE),
			rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top,
			this->hWnd,
			NULL,
			hDllModule,
			NULL);
	}

	OldPanelProc = (WNDPROC)SetWindowLongPtr(this->hDraw, GWLP_WNDPROC, (LONG_PTR)PanelProc);

	SetClassLongPtr(this->hDraw, GCLP_HBRBACKGROUND, NULL);
	RedrawWindow(this->hDraw, NULL, NULL, RDW_INVALIDATE);
	SetClassLongPtr(this->hWnd, GCLP_HBRBACKGROUND, NULL);
	RedrawWindow(this->hWnd, NULL, NULL, RDW_INVALIDATE);

	this->viewport.width = rect.right - rect.left;
	this->viewport.height = rect.bottom - rect.top;
	this->viewport.refresh = TRUE;

	DWORD threadId;
	SECURITY_ATTRIBUTES sAttribs;
	MemoryZero(&sAttribs, sizeof(SECURITY_ATTRIBUTES));
	sAttribs.nLength = sizeof(SECURITY_ATTRIBUTES);
	this->hDrawThread = CreateThread(&sAttribs, NULL, RenderThread, this, NORMAL_PRIORITY_CLASS, &threadId);
}

VOID DirectDraw::RenderStop()
{
	if (this->isFinish)
		return;

	this->isFinish = TRUE;
	WaitForSingleObject(this->hDrawThread, INFINITE);
	CloseHandle(this->hDrawThread);
	this->hDrawThread = NULL;

	BOOL wasFull = GetWindowLong(this->hDraw, GWL_STYLE) & WS_POPUP;
	if (DestroyWindow(this->hDraw))
		this->hDraw = NULL;

	if (wasFull)
		GL::ResetContext();
}

DWORD __fastcall AddDisplayMode(DEVMODE* devMode)
{
	DisplayMode* resList = resolutionsList;

	for (DWORD i = 0; i < RECOUNT; ++i, ++resList)
	{
		if (!resList->width)
		{
			resList->width = devMode->dmPelsWidth;
			resList->height = devMode->dmPelsHeight;
			resList->bpp = devMode->dmBitsPerPel;
			resList->frequency = devMode->dmDisplayFrequency > 60 ? devMode->dmDisplayFrequency : 60;
			return i + 1;
		}

		if (resList->width == devMode->dmPelsWidth)
		{
			if (resList->height == devMode->dmPelsHeight)
			{
				BOOL succ = FALSE;
				if (resList->bpp == 8 && resList->bpp == devMode->dmBitsPerPel)
					succ = TRUE;
				else if (resList->bpp != 8 && devMode->dmBitsPerPel != 8)
				{
					succ = TRUE;

					if (resList->bpp <= devMode->dmBitsPerPel)
						resList->bpp = devMode->dmBitsPerPel;
				}

				if (succ)
				{
					if (resList->frequency < devMode->dmDisplayFrequency)
						resList->frequency = devMode->dmDisplayFrequency;

					return i + 1;
				}
			}
		}
	}

	return 0;
}

DirectDraw::DirectDraw(DirectDraw* lastObj)
{
	this->last = lastObj;

	this->dwMode = NULL;
	this->indexBufferNA = NULL;
	this->indexBuffer = NULL;
	this->paletteNA = NULL;
	this->palette = NULL;

	this->hWnd = NULL;
	this->hDraw = NULL;
	this->hDc = NULL;

	this->isFinish = TRUE;
	this->isTakeSnapshot = FALSE;

	MemoryZero(&this->windowPlacement, sizeof(WINDOWPLACEMENT));
}

VOID DirectDraw::CalcView()
{
	this->viewport.rectangle.x = this->viewport.rectangle.y = 0;
	this->viewport.point.x = this->viewport.point.y = 0.0f;

	this->viewport.rectangle.width = this->viewport.width;
	this->viewport.rectangle.height = this->viewport.height;

	this->viewport.clipFactor.x = this->viewport.viewFactor.x = (FLOAT)this->viewport.width / this->dwMode->width;
	this->viewport.clipFactor.y = this->viewport.viewFactor.y = (FLOAT)this->viewport.height / this->dwMode->height;

	if (config.aspectRatio && this->viewport.viewFactor.x != this->viewport.viewFactor.y)
	{
		if (this->viewport.viewFactor.x > this->viewport.viewFactor.y)
		{
			FLOAT fw = this->viewport.viewFactor.y * this->dwMode->width;
			this->viewport.rectangle.width = (DWORD)MathRound(fw);

			this->viewport.point.x = ((FLOAT)this->viewport.width - fw) / 2.0f;
			this->viewport.rectangle.x = (DWORD)MathRound(this->viewport.point.x);

			this->viewport.clipFactor.x = this->viewport.viewFactor.y;
		}
		else
		{
			FLOAT fh = this->viewport.viewFactor.x * this->dwMode->height;
			this->viewport.rectangle.height = (DWORD)MathRound(fh);

			this->viewport.point.y = ((FLOAT)this->viewport.height - fh) / 2.0f;
			this->viewport.rectangle.y = (DWORD)MathRound(this->viewport.point.y);

			this->viewport.clipFactor.y = this->viewport.viewFactor.x;
		}
	}
}

VOID DirectDraw::CheckView()
{
	if (this->viewport.refresh)
	{
		this->viewport.refresh = FALSE;
		this->CalcView();
		GLViewport(this->viewport.rectangle.x, this->viewport.rectangle.y, this->viewport.rectangle.width, this->viewport.rectangle.height);

		this->clearStage = 0;
	}

	if (++this->clearStage <= 2)
		GLClear(GL_COLOR_BUFFER_BIT);
}

VOID DirectDraw::CaptureMouse(UINT uMsg, LPMSLLHOOKSTRUCT mInfo)
{
	if (config.windowedMode)
	{
		switch (uMsg)
		{
		case WM_MOUSEMOVE:
		{
			this->HookMouse(uMsg, mInfo);
			break;
		}

		case WM_LBUTTONUP:
		{
			if (this->mbPressed & MK_LBUTTON)
				this->HookMouse(uMsg, mInfo);

			break;
		}

		case WM_RBUTTONUP:
		{
			if (this->mbPressed & MK_RBUTTON)
				this->HookMouse(uMsg, mInfo);

			break;
		}

		default: break;
		}
	}
}

VOID DirectDraw::HookMouse(UINT uMsg, LPMSLLHOOKSTRUCT mInfo)
{
	POINT point = mInfo->pt;
	ScreenToClient(this->hWnd, &point);
	RECT rect;
	GetClientRect(hWnd, &rect);

	if (point.x < rect.left || point.x > rect.right ||
		point.y < rect.top || point.y > rect.bottom)
	{
		if (point.x < rect.left)
			point.x = rect.left;
		else if (point.x > rect.right)
			point.x = rect.right;

		if (point.y < rect.top)
			point.y = rect.top;
		else if (point.y > rect.bottom)
			point.y = rect.bottom;

		SendMessage(this->hWnd, uMsg, this->mbPressed, MAKELONG(point.x, point.y));
	}
}

VOID DirectDraw::ScaleMouse(UINT uMsg, LPARAM* lParam)
{
	if (config.windowedMode)
	{
		INT xPos = GET_X_LPARAM(*lParam);
		INT yPos = GET_Y_LPARAM(*lParam);

		if (xPos < this->viewport.rectangle.x)
			xPos = 0;
		else if (xPos >= this->viewport.rectangle.x + this->viewport.rectangle.width)
			xPos = this->dwMode->width - 1;
		else
			xPos = (INT)((FLOAT)(xPos - this->viewport.rectangle.x) / this->viewport.clipFactor.x);

		if (yPos < this->viewport.rectangle.y)
			yPos = 0;
		else if (yPos >= this->viewport.rectangle.y + this->viewport.rectangle.height)
			yPos = this->dwMode->height - 1;
		else
			yPos = (INT)((FLOAT)(yPos - this->viewport.rectangle.y) / this->viewport.clipFactor.y);

		*lParam = MAKELONG(xPos, yPos);
	}
}

HRESULT DirectDraw::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
	*ppvObj = new DirectDrawInterface();
	return DD_OK;
}

ULONG DirectDraw::Release()
{
	this->ReleaseMode();

	if (ddrawList == this)
		ddrawList = NULL;
	else
	{
		DirectDraw* ddraw = ddrawList;
		while (ddraw)
		{
			if (ddraw->last == this)
			{
				ddraw->last = ddraw->last->last;
				break;
			}

			ddraw = ddraw->last;
		}
	}

	delete this;
	return 0;
}

HRESULT DirectDraw::EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback)
{
	MemoryZero(resolutionsList, sizeof(resolutionsList));

	DEVMODE devMode;
	MemoryZero(&devMode, sizeof(DEVMODE));
	devMode.dmSize = sizeof(DEVMODE);

	DWORD count = 0;
	for (DWORD i = 0; EnumDisplaySettings(NULL, i, &devMode); ++i)
	{
		if ((devMode.dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL)) == (DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL) &&
			devMode.dmPelsWidth >= 640 && devMode.dmPelsHeight >= 480)
		{
			DWORD idx;

			if (devMode.dmBitsPerPel != 8 &&
				(devMode.dmPelsWidth == 1024 && devMode.dmPelsHeight == 768 ||
					devMode.dmPelsWidth == 800 && devMode.dmPelsHeight == 600 ||
					devMode.dmPelsWidth == 640 && devMode.dmPelsHeight == 480))
			{
				idx = AddDisplayMode(&devMode);
				if (idx)
				{
					if (count < idx)
						count = idx;
				}
				else
					break;
			}

			devMode.dmBitsPerPel = 8;
			idx = AddDisplayMode(&devMode);
			if (idx)
			{
				if (count < idx)
					count = idx;
			}
			else
				break;
		}

		MemoryZero(&devMode, sizeof(DEVMODE));
		devMode.dmSize = sizeof(DEVMODE);
	}

#ifdef _DEBUG
	devMode.dmBitsPerPel = 8;
	devMode.dmDisplayFrequency = 75;
	devMode.dmPelsWidth = 1920;
	devMode.dmPelsHeight = 1080;
	count = AddDisplayMode(&devMode);
#endif

	DDSURFACEDESC ddSurfaceDesc;
	DisplayMode* mode = resolutionsList;
	while (count--)
	{
		ddSurfaceDesc.dwWidth = mode->width;
		ddSurfaceDesc.dwHeight = mode->height;
		ddSurfaceDesc.ddpfPixelFormat.dwRGBBitCount = mode->bpp;

		if (!lpEnumModesCallback(&ddSurfaceDesc, NULL))
			break;

		++mode;
	}

	return DD_OK;
}

HRESULT DirectDraw::SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP)
{
	this->dwMode = NULL;

	DisplayMode* resList = resolutionsList;
	for (DWORD i = 0; i < RECOUNT; ++i, ++resList)
	{
		if (!resList->width)
			return DDERR_INVALIDMODE;

		if (resList->width == dwWidth && resList->height == dwHeight && resList->bpp == dwBPP)
		{
			this->dwMode = resList;
			break;
		}
	}

	if (this->indexBufferNA)
		MemoryFree(this->indexBufferNA);

	this->indexBufferNA = MemoryAlloc(dwWidth * dwHeight * (dwBPP >> 3) * (dwBPP >> 3) + 16); // double (dwBPP >> 3) for EU video fix
	this->indexBuffer = (BYTE*)(((DWORD)this->indexBufferNA + 16) & 0xFFFFFFF0);

	if (!this->paletteNA)
	{
		this->paletteNA = MemoryAlloc(256 * sizeof(DWORD) + 16);
		this->palette = (DWORD*)(((DWORD)this->paletteNA + 16) & 0xFFFFFFF0);
	}

	MemoryZero(this->palette, 255 * sizeof(DWORD));
	this->palette[255] = WHITE;

	if (config.windowedMode)
	{
		if (!(GetWindowLong(this->hWnd, GWL_STYLE) & WS_BORDER))
			this->SetWindowedMode();
		else
			SetCaptureMouse(TRUE);
	}
	else
		this->SetFullscreenMode();

	CheckMenu();

	this->RenderStart();

	return DD_OK;
}

VOID DirectDraw::SetFullscreenMode()
{
	DEVMODE devMode;
	MemoryZero(&devMode, sizeof(DEVMODE));
	devMode.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devMode);

	devMode.dmPelsWidth = this->dwMode->width;
	devMode.dmPelsHeight = this->dwMode->height;
	devMode.dmFields |= (DM_PELSWIDTH | DM_PELSHEIGHT);

	if (this->dwMode->frequency)
	{
		devMode.dmDisplayFrequency = this->dwMode->frequency;
		devMode.dmFields |= DM_DISPLAYFREQUENCY;
	}

	if (ChangeDisplaySettingsEx(NULL, &devMode, NULL, CDS_FULLSCREEN | CDS_TEST | CDS_RESET, NULL) == DISP_CHANGE_SUCCESSFUL)
	{
		if (config.windowedMode)
			GetWindowPlacement(hWnd, &this->windowPlacement);

		if (GetMenu(hWnd))
			SetMenu(hWnd, NULL);

		SetWindowLong(this->hWnd, GWL_STYLE, FS_STYLE);
		SetWindowPos(this->hWnd, NULL, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL);
		SetForegroundWindow(this->hWnd);

		SetCaptureMouse(FALSE);

		if (ChangeDisplaySettingsEx(NULL, &devMode, NULL, CDS_FULLSCREEN | CDS_RESET, NULL) == DISP_CHANGE_SUCCESSFUL)
		{
			RECT rc;
			GetWindowRect(this->hWnd, &rc);
			if (rc.right - rc.left != devMode.dmPelsWidth || rc.bottom - rc.top != devMode.dmPelsHeight)
			{
				if (!mciVideo.deviceId)
					SetWindowPos(this->hWnd, NULL, 0, 0, devMode.dmPelsWidth, devMode.dmPelsHeight, NULL);
				else
					SetWindowPos(this->hWnd, NULL, 0, -1, devMode.dmPelsWidth, devMode.dmPelsHeight + 1, NULL);
				SetForegroundWindow(this->hWnd);
			}
		}
	}
}

VOID DirectDraw::SetWindowedMode()
{
	ChangeDisplaySettings(NULL, NULL);

	SetCaptureMouse(TRUE);

	if (!this->windowPlacement.length)
	{
		this->windowPlacement.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(hWnd, &this->windowPlacement);

		INT monWidth = GetSystemMetrics(SM_CXSCREEN);
		INT monHeight = GetSystemMetrics(SM_CYSCREEN);

		INT newWidth = (INT)MathRound(0.75f * monWidth);
		INT newHeight = (INT)MathRound(0.75f * monHeight);

		FLOAT k = (FLOAT)this->dwMode->width / this->dwMode->height;

		INT check = (INT)MathRound((FLOAT)newHeight * k);
		if (newWidth > check)
			newWidth = check;
		else
			newHeight = (INT)MathRound((FLOAT)newWidth / k);

		RECT* rect = &this->windowPlacement.rcNormalPosition;
		rect->left = (monWidth - newWidth) >> 1;
		rect->top = (monHeight - newHeight) >> 1;
		rect->right = rect->left + newWidth;
		rect->bottom = rect->top + newHeight;
		AdjustWindowRect(rect, WIN_STYLE, TRUE);

		this->windowPlacement.ptMinPosition.x = this->windowPlacement.ptMinPosition.y = -1;
		this->windowPlacement.ptMaxPosition.x = this->windowPlacement.ptMaxPosition.y = -1;

		this->windowPlacement.flags = NULL;
		this->windowPlacement.showCmd = SW_SHOWNORMAL;
	}

	if (!GetMenu(hWnd))
		SetMenu(hWnd, config.menu);

	SetWindowLong(this->hWnd, GWL_STYLE, WIN_STYLE);
	SetWindowPlacement(this->hWnd, &this->windowPlacement);
}

HRESULT DirectDraw::SetCooperativeLevel(HWND hWnd, DWORD dwFlags)
{
	if (hWnd && hWnd != this->hWnd)
	{
		this->hWnd = hWnd;
		this->hDc = NULL;
		this->mbPressed = NULL;

		if (!OldWindowProc)
			OldWindowProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);
	}

	return DD_OK;
}

HRESULT DirectDraw::CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE* lplpDDPalette, IUnknown* pUnkOuter)
{
	this->ddPallete = new DirectDrawPalette(this, this->ddPallete);
	*(DirectDrawPalette**)lplpDDPalette = this->ddPallete;

	MemoryCopy(this->palette, lpDDColorArray, 255 * sizeof(DWORD));
	this->isPalChanged = TRUE;

	return DD_OK;
}

HRESULT DirectDraw::CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter)
{
	if (this->dwMode)
	{
		lpDDSurfaceDesc->dwWidth = this->dwMode->width;
		lpDDSurfaceDesc->dwHeight = this->dwMode->height;
		lpDDSurfaceDesc->lPitch = this->dwMode->width * (this->dwMode->bpp >> 3);
	}

	*(DirectDrawSurface**)lplpDDSurface = new DirectDrawSurface(this);
	this->surface = *lplpDDSurface;

	return DD_OK;
}

VOID DirectDraw::ReleaseMode()
{
	this->RenderStop();

	if (this->ddPallete)
	{
		delete this->ddPallete;
		this->ddPallete = NULL;
	}

	if (this->paletteNA)
	{
		MemoryFree(this->paletteNA);
		this->paletteNA = NULL;
		this->palette = NULL;
	}

	if (this->indexBufferNA)
	{
		MemoryFree(this->indexBufferNA);
		this->indexBufferNA = NULL;
		this->indexBuffer = NULL;
	}
}