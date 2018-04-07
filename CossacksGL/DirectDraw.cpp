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
#include "resource.h"
#include "math.h"
#include "Windowsx.h"
#include "DirectDraw.h"
#include "DirectDrawSurface.h"
#include "DirectDrawInterface.h"
#include "ShaderSource.h"
#include "FpsCounter.h"

#pragma region Not Implemented
ULONG DirectDraw::AddRef() { return 0; }
HRESULT DirectDraw::Compact() { return DD_OK; }
HRESULT DirectDraw::CreateClipper(DWORD, LPDIRECTDRAWCLIPPER *, IUnknown *) { return DD_OK; }
HRESULT DirectDraw::EnumSurfaces(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMSURFACESCALLBACK) { return DD_OK; }
HRESULT DirectDraw::FlipToGDISurface(void) { return DD_OK; }
HRESULT DirectDraw::GetCaps(LPDDCAPS, LPDDCAPS) { return DD_OK; }
HRESULT DirectDraw::GetDisplayMode(LPDDSURFACEDESC) { return DD_OK; }
HRESULT DirectDraw::GetFourCCCodes(LPDWORD, LPDWORD) { return DD_OK; }
HRESULT DirectDraw::GetGDISurface(LPDIRECTDRAWSURFACE *) { return DD_OK; }
HRESULT DirectDraw::GetMonitorFrequency(LPDWORD) { return DD_OK; }
HRESULT DirectDraw::GetScanLine(LPDWORD) { return DD_OK; }
HRESULT DirectDraw::GetVerticalBlankStatus(LPBOOL) { return DD_OK; }
HRESULT DirectDraw::Initialize(GUID *) { return DD_OK; }
HRESULT DirectDraw::RestoreDisplayMode() { return DD_OK; }
HRESULT DirectDraw::DuplicateSurface(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE *) { return DD_OK; }
HRESULT DirectDraw::WaitForVerticalBlank(DWORD, HANDLE) { return DD_OK; }
HRESULT DirectDraw::QueryInterface(REFIID riid, LPVOID* ppvObj) {
	//IID_IDirectDraw2
	*ppvObj = new DirectDrawInterface();
	return DD_OK;
}
#pragma endregion

#define RECOUNT 32 
#define WIN_STYLE WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE | WS_SIZEBOX
#define WHITE 0xFFFFFFFF;

DisplayMode resolutionsList[32];

WNDPROC OldWindowProc;
HHOOK OldMouseHook;

LRESULT CALLBACK MouseHook(INT nCode, WPARAM wParam, LPARAM lParam)
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

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SIZE:
	{
		DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
		if (ddraw && ddraw->dwMode)
		{
			ddraw->viewport.width = LOWORD(lParam);
			ddraw->viewport.height = HIWORD(lParam) - (ddraw->windowState != WinStateWindowed ? -1 : 0);
			ddraw->viewport.refresh = TRUE;
		}

		return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
	}

	case WM_DISPLAYCHANGE:
	{
		DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
		if (ddraw)
		{
			DEVMODE devMode = { NULL };
			devMode.dmSize = sizeof(DEVMODE);
			EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
			ddraw->frequency = devMode.dmDisplayFrequency;
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	case WM_SETFOCUS:
	case WM_KILLFOCUS:
	case WM_ACTIVATE:
	case WM_ACTIVATEAPP:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	case WM_NCACTIVATE:
	{
		DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
		if ((BOOL)wParam)
		{
			if (ddraw)
			{
				if (ddraw->windowState == WinStateWindowed)
				{
					if (!OldMouseHook)
						OldMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHook, hDllModule, NULL);
				}
				else
					ddraw->SetInternalMode();
			}
		}
		else
		{
			if (ddraw && ddraw->windowState != WinStateWindowed && !ddraw->isFinish)
			{
				ChangeDisplaySettings(NULL, NULL);
				ClipCursor(NULL);
				ShowWindow(hWnd, SW_MINIMIZE);
			}

			if (OldMouseHook)
			{
				UnhookWindowsHookEx(OldMouseHook);
				OldMouseHook = NULL;
			}
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		if (wParam == VK_F11 || wParam == VK_RETURN && (HIWORD(lParam) & KF_ALTDOWN))
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);

			if (ddraw && ddraw->dwMode)
			{
				if (ddraw->windowState == WinStateWindowed)
				{
					if (OldMouseHook)
					{
						UnhookWindowsHookEx(OldMouseHook);
						OldMouseHook = NULL;
					}

					GetWindowPlacement(hWnd, &ddraw->windowPlacement);

					ddraw->windowState = WinStateFullScreen;
					SetWindowLong(hWnd, GWL_STYLE, ddraw->fsStyle);
					ddraw->SetInternalMode();
				}
				else
				{
					if (GetForegroundWindow() == hWnd && !OldMouseHook)
						OldMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHook, hDllModule, NULL);

					ChangeDisplaySettings(NULL, NULL);
					ClipCursor(NULL);

					RECT* rect = &ddraw->windowPlacement.rcNormalPosition;

					WindowState oldState = ddraw->windowState;
					if (ddraw->windowState == WinStateNone)
					{
						ddraw->fsStyle = GetWindowLong(hWnd, GWL_STYLE);
						MONITORINFO mi = { sizeof(mi) };
						HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
						GetMonitorInfo(hMon, &mi);
						DWORD monWidth = mi.rcWork.right - mi.rcWork.left;
						DWORD monHeight = mi.rcWork.bottom - mi.rcWork.top;
						DWORD newWidth = 0.75 * (mi.rcWork.right - mi.rcWork.left);
						DWORD newHeght = 0.75 * (mi.rcWork.bottom - mi.rcWork.top);

						FLOAT k = (FLOAT)ddraw->dwMode->dwWidth / ddraw->dwMode->dwHeight;
						if (newWidth > newHeght * k)
							newWidth = newHeght * k;
						else
							newHeght = newWidth / k;

						rect->left = mi.rcWork.left + (monWidth - newWidth) / 2;
						rect->top = mi.rcWork.top + (monHeight - newHeght) / 2;
						rect->right = rect->left + newWidth;
						rect->bottom = rect->top + newHeght;
						AdjustWindowRect(rect, WIN_STYLE, FALSE);

						ddraw->windowPlacement.ptMinPosition.x = ddraw->windowPlacement.ptMinPosition.y = -1;
						ddraw->windowPlacement.ptMaxPosition.x = ddraw->windowPlacement.ptMaxPosition.y = -1;

						ddraw->windowPlacement.flags = NULL;
						ddraw->windowPlacement.showCmd = SW_SHOWNORMAL;
					}

					ddraw->windowState = WinStateWindowed;

					SetWindowLong(hWnd, GWL_STYLE, WIN_STYLE);
					SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_WINDOWEDGE);
					SetWindowPlacement(hWnd, &ddraw->windowPlacement);
				}

				ddraw->isStateChanged = TRUE;
			}

			break;
		}
		else if (wParam == 0x46)
		{
			isFpsEnabled = !isFpsEnabled;
			isFpsChanged = TRUE;

			break;
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
		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	default:
		return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
	}

	return NULL;
}

DWORD WINAPI RenderThread(LPVOID lpParameter)
{
	DirectDraw* ddraw = (DirectDraw*)lpParameter;
	ddraw->hDc = ::GetDC(ddraw->hWnd);
	{
		if (!ddraw->wasPixelSet)
		{
			ddraw->wasPixelSet = TRUE;
			PIXELFORMATDESCRIPTOR pfd =
			{
				sizeof(PIXELFORMATDESCRIPTOR), // Size Of This Pixel Format Descriptor
				1, // Version Number
				PFD_DRAW_TO_WINDOW // Format Must Support Window
				| PFD_SUPPORT_OPENGL // Format Must Support OpenGL
				| PFD_DOUBLEBUFFER, // Must Support Double Buffering
				PFD_TYPE_RGBA, // Request An RGBA Format
				32, // Select Our Color Depth
				0, 0, 0, 0, 0, 0, // Color Bits Ignored
				0, // No Alpha Buffer
				0, // Shift Bit Ignored
				0, // No Accumulation Buffer
				0, 0, 0, 0, // Accumulation Bits Ignored
				16, // Z-Buffer (Depth Buffer) 
				0, // Stencil Buffer
				0, // No Auxiliary Buffer
				PFD_MAIN_PLANE, // Main Drawing Layer
				0, // Reserved
				0, 0, 0 // Layer Masks Ignored
			};

			DWORD pfIndex;
			if (!GL::PreparePixelFormat(&pfd, &pfIndex, ddraw->hWnd))
			{
				pfIndex = ChoosePixelFormat(ddraw->hDc, &pfd);
				if (pfIndex == NULL)
					Main::ShowError("ChoosePixelFormat failed", __FILE__, __LINE__);
				else if (pfd.dwFlags & PFD_NEED_PALETTE)
					Main::ShowError("Needs palette", __FILE__, __LINE__);
			}

			if (!SetPixelFormat(ddraw->hDc, pfIndex, &pfd))
				Main::ShowError("SetPixelFormat failed", __FILE__, __LINE__);

			memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
			pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
			pfd.nVersion = 1;
			if (DescribePixelFormat(ddraw->hDc, pfIndex, sizeof(PIXELFORMATDESCRIPTOR), &pfd) == NULL)
				Main::ShowError("DescribePixelFormat failed", __FILE__, __LINE__);

			if ((pfd.iPixelType != PFD_TYPE_RGBA) ||
				(pfd.cRedBits < 5) || (pfd.cGreenBits < 5) || (pfd.cBlueBits < 5))
				Main::ShowError("Bad pixel type", __FILE__, __LINE__);
		}

		HGLRC hRc = WGLCreateContext(ddraw->hDc);
		{
			WGLMakeCurrent(ddraw->hDc, hRc);
			{
				GL::CreateContextAttribs(ddraw->hDc, &hRc);

				GLint glMaxTexSize;
				GLGetIntegerv(GL_MAX_TEXTURE_SIZE, &glMaxTexSize);

				if (glVersion >= GL_VER_3_0)
				{
					DWORD maxSize = ddraw->dwMode->dwWidth > ddraw->dwMode->dwHeight ? ddraw->dwMode->dwWidth : ddraw->dwMode->dwHeight;

					DWORD maxTexSize = 1;
					while (maxTexSize < maxSize)
						maxTexSize <<= 1;

					if (maxTexSize > glMaxTexSize)
						glVersion = GL_VER_1_1;
				}

				if (glVersion >= GL_VER_3_0)
					ddraw->RenderNew();
				else
					ddraw->RenderOld(glMaxTexSize);
			}
			WGLMakeCurrent(ddraw->hDc, NULL);
		}
		WGLDeleteContext(hRc);
	}
	::ReleaseDC(ddraw->hWnd, ddraw->hDc);
	ddraw->hDc = NULL;

	return NULL;
}

VOID DirectDraw::RenderOld(DWORD glMaxTexSize)
{
	if (glMaxTexSize < 256)
		glMaxTexSize = 256;

	DWORD minSize = this->dwMode->dwWidth < this->dwMode->dwHeight ? this->dwMode->dwWidth : this->dwMode->dwHeight;

	DWORD maxAllow = 1;
	while (maxAllow < minSize)
		maxAllow <<= 1;

	DWORD maxTexSize = maxAllow < glMaxTexSize ? maxAllow : glMaxTexSize;

	VOID* pixelBuffer = malloc(maxTexSize * maxTexSize * sizeof(DWORD));
	{
		DWORD framePerWidth = this->dwMode->dwWidth / maxTexSize + (this->dwMode->dwWidth % maxTexSize ? 1 : 0);
		DWORD framePerHeight = this->dwMode->dwHeight / maxTexSize + (this->dwMode->dwHeight % maxTexSize ? 1 : 0);
		DWORD frameCount = framePerWidth * framePerHeight;
		Frame* frames = (Frame*)malloc(frameCount * sizeof(Frame));
		{
			Frame* frame = frames;
			for (DWORD y = 0; y < this->dwMode->dwHeight; y += maxTexSize)
			{
				DWORD height = this->dwMode->dwHeight - y;
				if (height > maxTexSize)
					height = maxTexSize;

				for (DWORD x = 0; x < this->dwMode->dwWidth; x += maxTexSize, ++frame)
				{
					DWORD width = this->dwMode->dwWidth - x;
					if (width > maxTexSize)
						width = maxTexSize;

					frame->rect.x = x;
					frame->rect.y = y;
					frame->rect.width = width;
					frame->rect.height = height;

					frame->vSize.width = x + width;
					frame->vSize.height = y + height;

					frame->tSize.width = width == maxTexSize ? 1.0 : (FLOAT)width / maxTexSize;
					frame->tSize.height = height == maxTexSize ? 1.0 : (FLOAT)height / maxTexSize;


					GLGenTextures(1, &frame->id);
					GLBindTexture(GL_TEXTURE_2D, frame->id);

					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glCapsClampToEdge);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glCapsClampToEdge);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					if (this->windowState == WinStateWindowed)
					{
						GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					}
					else
					{
						GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
						GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					}

					GLTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

					if (GLColorTable)
						GLTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, maxTexSize, maxTexSize, GL_NONE, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, NULL);
					else
						GLTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, maxTexSize, maxTexSize, GL_NONE, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				}
			}

			GLClearColor(0.0, 0.0, 0.0, 1.0);
			this->viewport.refresh = TRUE;

			GLMatrixMode(GL_PROJECTION);
			GLLoadIdentity();
			GLOrtho(0.0, this->dwMode->dwWidth, this->dwMode->dwHeight, 0.0, 0.0, 1.0);
			GLMatrixMode(GL_MODELVIEW);
			GLLoadIdentity();

			GLEnable(GL_TEXTURE_2D);

			if (glCapsSharedPalette)
				GLEnable(GL_SHARED_TEXTURE_PALETTE_EXT);

			DWORD fpsQueue[FPS_COUNT];
			DWORD tickQueue[FPS_COUNT];

			DWORD fpsIdx = -1;
			DWORD fpsTotal = 0;
			DWORD fpsCount = 0;
			INT fpsSum = 0;
			memset(fpsQueue, 0, sizeof(fpsQueue));
			memset(tickQueue, 0, sizeof(tickQueue));

			do
			{
				if (isFpsEnabled)
				{
					DWORD tick = GetTickCount();

					if (isFpsChanged)
					{
						isFpsChanged = FALSE;
						fpsIdx = -1;
						fpsTotal = 0;
						fpsCount = 0;
						fpsSum = 0;
						memset(fpsQueue, 0, sizeof(fpsQueue));
						memset(tickQueue, 0, sizeof(tickQueue));
					}

					++fpsTotal;
					if (fpsCount < FPS_COUNT)
						++fpsCount;

					++fpsIdx;
					if (fpsIdx == FPS_COUNT)
						fpsIdx = 0;

					DWORD diff = tick - tickQueue[fpsTotal != fpsCount ? fpsIdx : 0];
					tickQueue[fpsIdx] = tick;

					DWORD fps = diff ? Main::Round(1000.0 / diff * fpsCount) : 9999;

					DWORD* queue = &fpsQueue[fpsIdx];
					fpsSum -= *queue - fps;
					*queue = fps;
				}

				if (!OldMouseHook)
					ClipCursor(NULL);

				this->CheckView();

				if (this->isStateChanged)
				{
					this->isStateChanged = FALSE;
					if (this->windowState == WinStateWindowed)
					{
						GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					}
					else
					{
						GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
						GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					}
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
						for (DWORD y = frame->rect.y; y < frame->vSize.height; ++y)
						{
							BYTE* idx = this->indexBuffer + y * this->dwMode->dwWidth + frame->rect.x;
							memcpy(pix, idx, frame->rect.width);
							pix += frame->rect.width;
						}

						if (isFpsEnabled && count == frameCount - 1)
						{
							DWORD fps = Main::Round((FLOAT)fpsSum / fpsCount);

							DWORD offset = FPS_X;

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
								bool* lpDig = (bool*)counters + FPS_WIDTH * FPS_HEIGHT * digit;

								for (DWORD y = 0; y < FPS_HEIGHT; ++y)
								{
									BYTE* pix = (BYTE*)pixelBuffer + (FPS_Y + y) * (maxTexSize)+
										FPS_X + (FPS_STEP + FPS_WIDTH) * (dcount - 1) + FPS_STEP;

									DWORD width = FPS_WIDTH;
									do
									{
										if (*lpDig++)
											*pix = 0xFF;
										++pix;
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
						for (DWORD y = frame->rect.y; y < frame->vSize.height; ++y)
						{
							BYTE* idx = this->indexBuffer + y * this->dwMode->dwWidth + frame->rect.x;
							for (DWORD x = frame->rect.x; x < frame->vSize.width; ++x)
								*pix++ = *(DWORD*)&this->palette[*idx++];
						}

						if (isFpsEnabled && count == frameCount - 1)
						{
							DWORD fps = Main::Round((FLOAT)fpsSum / fpsCount);

							DWORD offset = FPS_X;

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
								bool* lpDig = (bool*)counters + FPS_WIDTH * FPS_HEIGHT * digit;

								for (DWORD y = 0; y < FPS_HEIGHT; ++y)
								{
									DWORD* pix = (DWORD*)pixelBuffer + (FPS_Y + y) * maxTexSize +
										FPS_X + (FPS_STEP + FPS_WIDTH) * (dcount - 1) + FPS_STEP;

									DWORD width = FPS_WIDTH;
									do
									{
										if (*lpDig++)
											*pix = 0xFFFFFFFF;
										++pix;
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
						GLVertex2s(frame->rect.x, frame->rect.y);

						GLTexCoord2f(frame->tSize.width, 0.0);
						GLVertex2s(frame->vSize.width, frame->rect.y);

						GLTexCoord2f(frame->tSize.width, frame->tSize.height);
						GLVertex2s(frame->vSize.width, frame->vSize.height);

						GLTexCoord2f(0.0, frame->tSize.height);
						GLVertex2s(frame->rect.x, frame->vSize.height);
					}
					GLEnd();
					++frame;
				}

				GLFlush();
				SwapBuffers(this->hDc);
			} while (!this->isFinish);
			GLFinish();

			frame = frames;
			DWORD count = frameCount;
			while (count--)
			{
				GLDeleteTextures(1, &frame->id);
				++frame;
			}
		}
		free(frames);
	}
	free(pixelBuffer);
}

VOID DirectDraw::RenderNew()
{
	DWORD maxSize = this->dwMode->dwWidth > this->dwMode->dwHeight ? this->dwMode->dwWidth : this->dwMode->dwHeight;
	DWORD maxTexSize = 1;
	while (maxTexSize < maxSize) maxTexSize <<= 1;
	FLOAT texWidth = this->dwMode->dwWidth == maxTexSize ? 1.0 : (FLOAT)this->dwMode->dwWidth / maxTexSize;
	FLOAT texHeight = this->dwMode->dwHeight == maxTexSize ? 1.0 : (FLOAT)this->dwMode->dwHeight / maxTexSize;

	FLOAT buffer[4][4] = {
		{ 0.0, 0.0, 0.0, 0.0 },
		{ this->dwMode->dwWidth, 0.0, texWidth, 0.0 },
		{ this->dwMode->dwWidth, this->dwMode->dwHeight, texWidth, texHeight },
		{ 0.0, this->dwMode->dwHeight, 0.0, texHeight }
	};

	FLOAT mvpMatrix[4][4] = {
		{ 2.0f / this->dwMode->dwWidth, 0.0f, 0.0f, 0.0f },
		{ 0.0f, -2.0f / this->dwMode->dwHeight, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 2.0f, 0.0f },
		{ -1.0f, 1.0f, -1.0f, 1.0f }
	};

	GLuint vShader = GL::CompileShaderSource(shVertexSoure, GL_VERTEX_SHADER, this->hWnd);
	GLuint fShaderFull = GL::CompileShaderSource(shFragmentFullscreen, GL_FRAGMENT_SHADER, this->hWnd);
	GLuint fShaderWin = GL::CompileShaderSource(shFragmentWindowed, GL_FRAGMENT_SHADER, this->hWnd);

	GLuint shProgramWin = GLCreateProgram();
	{
		GLAttachShader(shProgramWin, vShader);
		GLAttachShader(shProgramWin, fShaderWin);

		GLLinkProgram(shProgramWin);
		GLUseProgram(shProgramWin);
		{
			GLUniformMatrix4fv(GLGetUniformLocation(shProgramWin, "mvp"), 1, GL_FALSE, (GLfloat*)mvpMatrix);
			GLUniform1i(GLGetUniformLocation(shProgramWin, "tex01"), 0);

			GLuint shProgramFull = GLCreateProgram();
			{
				GLAttachShader(shProgramFull, vShader);
				GLAttachShader(shProgramFull, fShaderFull);

				GLLinkProgram(shProgramFull);
				GLUseProgram(shProgramFull);
				{
					GLUniformMatrix4fv(GLGetUniformLocation(shProgramFull, "mvp"), 1, GL_FALSE, (GLfloat*)mvpMatrix);
					GLUniform1i(GLGetUniformLocation(shProgramFull, "tex01"), 0);
					GLUniform1i(GLGetUniformLocation(shProgramFull, "pal01"), 1);

					GLuint arrayName;
					GLGenVertexArrays(1, &arrayName);
					{
						GLBindVertexArray(arrayName);
						{
							GLuint bufferName;
							GLGenBuffers(1, &bufferName);
							{
								GLBindBuffer(GL_ARRAY_BUFFER, bufferName);
								{
									GLBufferData(GL_ARRAY_BUFFER, sizeof(buffer), buffer, GL_STREAM_DRAW);

									GLint attrCoordsLoc = GLGetAttribLocation(shProgramFull, "vCoord");
									GLEnableVertexAttribArray(attrCoordsLoc);
									GLVertexAttribPointer(attrCoordsLoc, 2, GL_FLOAT, GL_FALSE, 16, (GLvoid*)0);

									GLint attrTexCoordsLoc = GLGetAttribLocation(shProgramFull, "vTexCoord");
									GLEnableVertexAttribArray(attrTexCoordsLoc);
									GLVertexAttribPointer(attrTexCoordsLoc, 2, GL_FLOAT, GL_FALSE, 16, (GLvoid*)8);

									GLuint textures[3];
									GLGenTextures(sizeof(textures) / sizeof(GLuint), textures);
									{
										GLuint paletteId = textures[0];
										GLuint indicesId = textures[1];
										GLuint textureId = textures[2];

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

										GLBindTexture(GL_TEXTURE_2D, textureId);
										GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glCapsClampToEdge);
										GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glCapsClampToEdge);
										GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
										GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
										GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
										GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
										GLTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, maxTexSize, maxTexSize, GL_NONE, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

										GLClearColor(0.0, 0.0, 0.0, 1.0);
										this->viewport.refresh = TRUE;

										VOID* pixelBuffer = malloc(maxTexSize * maxTexSize * sizeof(DWORD));
										{
											this->isStateChanged = TRUE;

											DWORD fpsQueue[FPS_COUNT];
											DWORD tickQueue[FPS_COUNT];

											DWORD fpsIdx = -1;
											DWORD fpsTotal = 0;
											DWORD fpsCount = 0;
											INT fpsSum = 0;
											memset(fpsQueue, 0, sizeof(fpsQueue));
											memset(tickQueue, 0, sizeof(tickQueue));
											
											do
											{
												if (isFpsEnabled)
												{
													DWORD tick = GetTickCount();

													if (isFpsChanged)
													{
														isFpsChanged = FALSE;
														fpsIdx = -1;
														fpsTotal = 0;
														fpsCount = 0;
														fpsSum = 0;
														memset(fpsQueue, 0, sizeof(fpsQueue));
														memset(tickQueue, 0, sizeof(tickQueue));
													}

													++fpsTotal;
													if (fpsCount < FPS_COUNT)
														++fpsCount;

													++fpsIdx;
													if (fpsIdx == FPS_COUNT)
														fpsIdx = 0;

													DWORD diff = tick - tickQueue[fpsTotal != fpsCount ? fpsIdx : 0];
													tickQueue[fpsIdx] = tick;

													DWORD fps = diff ? Main::Round(1000.0 / diff * fpsCount) : 9999;

													DWORD* queue = &fpsQueue[fpsIdx];
													fpsSum -= *queue - fps;
													*queue = fps;
												}

												if (!OldMouseHook)
													ClipCursor(NULL);

												this->CheckView();

												if (this->windowState == WinStateWindowed)
												{
													if (this->isStateChanged)
													{
														this->isStateChanged = FALSE;
														GLUseProgram(shProgramWin);
														GLBindTexture(GL_TEXTURE_2D, textureId);
													}

													BYTE* idx = this->indexBuffer;
													DWORD* pix = (DWORD*)pixelBuffer;
													DWORD count = this->dwMode->dwWidth * this->dwMode->dwHeight;
													while (count--)
														*pix++ = *(DWORD*)&this->palette[*idx++];

													if (isFpsEnabled)
													{
														DWORD fps = Main::Round((FLOAT)fpsSum / fpsCount);

														DWORD offset = FPS_X;

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
															bool* lpDig = (bool*)counters + FPS_WIDTH * FPS_HEIGHT * digit;

															for (DWORD y = 0; y < FPS_HEIGHT; ++y)
															{
																DWORD* pix = (DWORD*)pixelBuffer + (FPS_Y + y) * this->dwMode->dwWidth +
																	FPS_X + (FPS_STEP + FPS_WIDTH) * (dcount - 1) + FPS_STEP;

																DWORD width = FPS_WIDTH;
																do
																{
																	if (*lpDig++)
																		*pix = 0xFFFFFFFF;
																	++pix;
																} while (--width);
															}

															current = current / 10;
														} while (--dcount);
													}

													GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->dwMode->dwWidth, this->dwMode->dwHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer);
												}
												else
												{
													if (this->isPalChanged)
													{
														this->isPalChanged = FALSE;

														if (this->isStateChanged)
														{
															this->isStateChanged = FALSE;
															GLUseProgram(shProgramFull);
														}

														GLActiveTexture(GL_TEXTURE1);
														GLBindTexture(GL_TEXTURE_1D, paletteId);
														GLTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA, GL_UNSIGNED_BYTE, this->palette);

														GLActiveTexture(GL_TEXTURE0);
														GLBindTexture(GL_TEXTURE_2D, indicesId);
													}
													else if (this->isStateChanged)
													{
														this->isStateChanged = FALSE;
														GLUseProgram(shProgramFull);
														GLBindTexture(GL_TEXTURE_2D, indicesId);
													}

													GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->dwMode->dwWidth, this->dwMode->dwHeight, GL_RED, GL_UNSIGNED_BYTE, this->indexBuffer);

													if (isFpsEnabled)
													{
														DWORD fps = Main::Round((FLOAT)fpsSum / fpsCount);

														DWORD offset = FPS_X;

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
															bool* lpDig = (bool*)counters + FPS_WIDTH * FPS_HEIGHT * digit;

															for (DWORD y = 0; y < FPS_HEIGHT; ++y)
															{
																BYTE* idx = this->indexBuffer + (FPS_Y + y) * this->dwMode->dwWidth +
																	FPS_X + (FPS_STEP + FPS_WIDTH) * (dcount - 1);

																BYTE* pix = (BYTE*)pixelBuffer + y * (FPS_STEP + FPS_WIDTH) * digCount +
																	(FPS_STEP + FPS_WIDTH) * (dcount - 1);

																*(DWORD*)pix = *(DWORD*)idx;

																pix += FPS_STEP;
																idx += FPS_STEP;

																DWORD width = FPS_WIDTH;
																do
																{
																	*pix++ = *lpDig++ ? 0xFF : *idx;
																	++idx;
																} while (--width);
															}

															current = current / 10;
														} while (--dcount);

														GLPixelStorei(GL_UNPACK_ALIGNMENT, 1);
														GLTexSubImage2D(GL_TEXTURE_2D, 0, FPS_X, FPS_Y, (FPS_WIDTH + FPS_STEP) * digCount, FPS_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, pixelBuffer);
														GLPixelStorei(GL_UNPACK_ALIGNMENT, 4);
													}
												}

												GLDrawArrays(GL_TRIANGLE_FAN, 0, 4);

												GLFlush();
												SwapBuffers(this->hDc);
											} while (!this->isFinish);
											GLFinish();
										}
										free(pixelBuffer);
									}
									GLDeleteTextures(sizeof(textures) / sizeof(GLuint), textures);
								}
								GLBindBuffer(GL_ARRAY_BUFFER, NULL);
							}
							GLDeleteBuffers(1, &bufferName);
						}
						GLBindVertexArray(NULL);
					}
					GLDeleteVertexArrays(1, &arrayName);
				}
				GLUseProgram(NULL);
			}
			GLDeleteProgram(shProgramFull);
		}
	}
	GLDeleteProgram(shProgramWin);
}

DWORD __fastcall AddDisplayMode(DEVMODE* devMode)
{
	DisplayMode* resList = resolutionsList;

	for (DWORD i = 0; i < RECOUNT; ++i, ++resList)
	{
		if (!resList->dwWidth)
		{
			resList->dwWidth = devMode->dmPelsWidth;
			resList->dwHeight = devMode->dmPelsHeight;
			resList->dwBPP = devMode->dmBitsPerPel;
			resList->dwFrequency = devMode->dmDisplayFrequency > 60 ? devMode->dmDisplayFrequency : 60;
			return i + 1;
		}

		if (resList->dwWidth == devMode->dmPelsWidth)
		{
			if (resList->dwHeight == devMode->dmPelsHeight)
			{
				BOOL succ = FALSE;
				if (resList->dwBPP == 8 && resList->dwBPP == devMode->dmBitsPerPel)
					succ = TRUE;
				else if (resList->dwBPP != 8 && devMode->dmBitsPerPel != 8)
				{
					succ = TRUE;

					if (resList->dwBPP <= devMode->dmBitsPerPel)
						resList->dwBPP = devMode->dmBitsPerPel;
				}

				if (succ)
				{
					if (resList->dwFrequency <= devMode->dmDisplayFrequency)
						resList->dwFrequency = devMode->dmDisplayFrequency;

					return i + 1;
				}
			}
		}
	}

	return 0;
}

DirectDraw::DirectDraw(DirectDraw* lastObj)
{
	GL::Load();

	this->last = lastObj;

	this->dwMode = NULL;
	this->indexBuffer = NULL;
	this->palette = NULL;
}

DirectDraw::~DirectDraw() { }

VOID DirectDraw::CheckView()
{
	if (this->viewport.refresh)
	{
		this->viewport.refresh = FALSE;

		this->viewport.rectangle.x = this->viewport.rectangle.y = 0;
		this->viewport.point.x = this->viewport.point.y = 0.0f;

		this->viewport.rectangle.width = this->viewport.width;
		this->viewport.rectangle.height = this->viewport.height;

		this->viewport.clipFactor.x = this->viewport.viewFactor.x = (FLOAT)this->viewport.width / this->dwMode->dwWidth;
		this->viewport.clipFactor.y = this->viewport.viewFactor.y = (FLOAT)this->viewport.height / this->dwMode->dwHeight;

		if (this->viewport.viewFactor.x != this->viewport.viewFactor.y)
		{
			if (this->viewport.viewFactor.x > this->viewport.viewFactor.y)
			{
				FLOAT fw = this->viewport.viewFactor.y * this->dwMode->dwWidth;
				this->viewport.rectangle.width = Main::Round(fw);

				this->viewport.point.x = ((FLOAT)this->viewport.width - fw) / 2.0f;
				this->viewport.rectangle.x = Main::Round(this->viewport.point.x);

				this->viewport.clipFactor.x = this->viewport.viewFactor.y;
			}
			else
			{
				FLOAT fh = this->viewport.viewFactor.x * this->dwMode->dwHeight;
				this->viewport.rectangle.height = Main::Round(fh);

				this->viewport.point.y = ((FLOAT)this->viewport.height - fh) / 2.0f;
				this->viewport.rectangle.y = Main::Round(this->viewport.point.y);

				this->viewport.clipFactor.y = this->viewport.viewFactor.x;
			}
		}


		GLViewport(this->viewport.rectangle.x, this->viewport.rectangle.y, this->viewport.rectangle.width, this->viewport.rectangle.height);

		this->clearStage = 0;
	}

	if (++this->clearStage <= 2)
		GLClear(GL_COLOR_BUFFER_BIT);
}

HRESULT DirectDraw::SetInternalMode()
{
	DEVMODE devMode = { NULL };
	devMode.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devMode);

	devMode.dmPelsWidth = this->dwMode->dwWidth;
	devMode.dmPelsHeight = this->dwMode->dwHeight;
	devMode.dmBitsPerPel = 32;
	devMode.dmDisplayFrequency = this->dwMode->dwFrequency;
	devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;

	DWORD res = ChangeDisplaySettingsEx(NULL, &devMode, NULL, CDS_FULLSCREEN | CDS_TEST | CDS_RESET, NULL);
	if (res != DISP_CHANGE_SUCCESSFUL)
		return DDERR_INVALIDMODE;

	ChangeDisplaySettingsEx(NULL, &devMode, NULL, CDS_FULLSCREEN | CDS_RESET, NULL);
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);

	DWORD dwStyle = GetWindowLong(this->hWnd, GWL_STYLE);

	RECT rect = { 0, -1, this->dwMode->dwWidth, this->dwMode->dwHeight };
	AdjustWindowRect(&rect, dwStyle, FALSE);

	rect.left += devMode.dmPosition.x;
	rect.right += devMode.dmPosition.x;
	rect.top += devMode.dmPosition.y;
	rect.bottom += devMode.dmPosition.y;
	SetWindowPos(this->hWnd, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, NULL);
	SetForegroundWindow(this->hWnd);

	ClipCursor(&rect);
	SetCursor(NULL);
	while (ShowCursor(FALSE) >= -1);
}

VOID DirectDraw::CaptureMouse(UINT uMsg, LPMSLLHOOKSTRUCT mInfo)
{
	if (this->windowState == WinStateWindowed)
	{
		ClipCursor(NULL);

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
		SetCursor(hCursor);
		while (ShowCursor(TRUE) <= 0);

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
	else
	{
		SetCursor(NULL);
		while (ShowCursor(FALSE) >= -1);
	}
}

VOID DirectDraw::ScaleMouse(UINT uMsg, LPARAM* lParam)
{
	if (this->windowState == WinStateWindowed)
	{
		INT xPos = GET_X_LPARAM(*lParam);
		INT yPos = GET_Y_LPARAM(*lParam);

		if (xPos < this->viewport.rectangle.x)
			xPos = 0;
		else if (xPos >= this->viewport.rectangle.x + this->viewport.rectangle.width)
			xPos = this->dwMode->dwWidth - 1;
		else
		{
			FLOAT number = (FLOAT)(xPos - this->viewport.rectangle.x) / this->viewport.clipFactor.x;
			FLOAT floorVal = floor(number);
			xPos = INT(floorVal + 0.5f > number ? floorVal : ceil(number));
		}

		if (yPos < this->viewport.rectangle.y)
			yPos = 0;
		else if (yPos >= this->viewport.rectangle.y + this->viewport.rectangle.height)
			yPos = this->dwMode->dwHeight - 1;
		else
		{
			FLOAT number = (FLOAT)(yPos - this->viewport.rectangle.y) / this->viewport.clipFactor.y;
			FLOAT floorVal = floor(number);
			yPos = INT(floorVal + 0.5f > number ? floorVal : ceil(number));
		}

		*lParam = MAKELONG(xPos, yPos);
	}
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

HRESULT DirectDraw::SetCooperativeLevel(HWND hWnd, DWORD dwFlags)
{
	if (hWnd && hWnd != this->hWnd)
	{
		this->hWnd = hWnd;
		this->windowState = WinStateNone;
		this->windowPlacement.length = sizeof(WINDOWPLACEMENT);
		this->mbPressed = NULL;
		this->wasPixelSet = FALSE;

		if (hIcon)
		{
			SetClassLong(hWnd, GCL_HICON, (LONG)hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		}

		if (!OldWindowProc)
			OldWindowProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);

		this->hDc = NULL;
	}

	return DD_OK;
}

HRESULT DirectDraw::CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE* lplpDDPalette, IUnknown* pUnkOuter)
{
	this->ddPallete = new DirectDrawPalette(this, this->ddPallete);
	*(DirectDrawPalette**)lplpDDPalette = this->ddPallete;

	memcpy(this->palette, lpDDColorArray, 255 * sizeof(PALETTEENTRY));
	this->isPalChanged = TRUE;

	return DD_OK;
}

HRESULT DirectDraw::CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter)
{
	if (this->dwMode)
	{
		lpDDSurfaceDesc->dwWidth = this->dwMode->dwWidth;
		lpDDSurfaceDesc->dwHeight = this->dwMode->dwHeight;
		lpDDSurfaceDesc->lPitch = this->dwMode->dwWidth * (this->dwMode->dwBPP >> 3);
	}

	*(DirectDrawSurface**)lplpDDSurface = new DirectDrawSurface(this);
	this->surface = *lplpDDSurface;

	return DD_OK;
}

HRESULT DirectDraw::EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback)
{
	memset(resolutionsList, 0, sizeof(resolutionsList));

	DEVMODE devMode = { NULL };
	devMode.dmSize = sizeof(DEVMODE);

	DWORD count = 0;
	for (DWORD i = 0; EnumDisplaySettings(NULL, i, &devMode); ++i)
	{
		if (devMode.dmPelsWidth >= 640 && devMode.dmPelsHeight >= 480)
		{
			DWORD idx;

			if (devMode.dmBitsPerPel != 8 &&
				(devMode.dmPelsWidth == 1024 && devMode.dmPelsHeight == 768 ||
					devMode.dmPelsWidth == 800 && devMode.dmPelsHeight == 600 ||
					devMode.dmPelsWidth == 640 && devMode.dmPelsHeight == 480))
			{
				idx = AddDisplayMode(&devMode);
				if (idx)
					count = idx;
				else
					break;
			}

			devMode.dmBitsPerPel = 8;
			idx = AddDisplayMode(&devMode);
			if (idx)
				count = idx;
			else
				break;
		}

		memset(&devMode, NULL, sizeof(DEVMODE));
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
		ddSurfaceDesc.dwWidth = mode->dwWidth;
		ddSurfaceDesc.dwHeight = mode->dwHeight;
		ddSurfaceDesc.ddpfPixelFormat.dwRGBBitCount = mode->dwBPP;

		if (!lpEnumModesCallback(&ddSurfaceDesc, NULL))
			break;

		++mode;
	}

	return DD_OK;
}

HRESULT DirectDraw::SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP)
{
	DisplayMode* resList = resolutionsList;
	for (DWORD i = 0; i < RECOUNT; ++i, ++resList)
	{
		if (!resList->dwWidth)
			return DDERR_INVALIDMODE;

		if (resList->dwWidth == dwWidth &&
			resList->dwHeight == dwHeight &&
			resList->dwBPP == dwBPP)
		{
			this->dwMode = resList;
			break;
		}
	}

	this->indexBuffer = (BYTE*)malloc(dwWidth * dwHeight * (dwBPP >> 3));
	this->palette = (PALETTEENTRY*)malloc(256 * sizeof(PALETTEENTRY));
	*(DWORD*)&this->palette[255] = WHITE;

	if (windowState != WinStateWindowed)
		this->SetInternalMode();
	else
		ClipCursor(NULL);

	if (dwBPP == 8)
	{
		SetClassLongPtr(this->hWnd, GCLP_HBRBACKGROUND, NULL);
		RedrawWindow(this->hWnd, NULL, NULL, RDW_INVALIDATE);

		this->isFinish = FALSE;
		this->viewport.refresh = TRUE;
		DWORD threadId;
		SECURITY_ATTRIBUTES sAttribs = { NULL };
		this->hDrawThread = CreateThread(&sAttribs, NULL, RenderThread, this, BELOW_NORMAL_PRIORITY_CLASS, &threadId);
	}
	else
	{
		HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
		SetClassLongPtr(this->hWnd, GCLP_HBRBACKGROUND, (LONG)brush);
		RedrawWindow(this->hWnd, NULL, NULL, RDW_INVALIDATE);
	}

	return DD_OK;
}

VOID DirectDraw::ReleaseMode()
{
	this->isFinish = TRUE;

	if (this->dwMode && this->dwMode->dwBPP == 8)
	{
		WaitForSingleObject(this->hDrawThread, INFINITE);
		this->dwMode = NULL;
	}
	else
	{
		HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
		SetClassLongPtr(this->hWnd, GCLP_HBRBACKGROUND, (LONG)brush);
		RedrawWindow(this->hWnd, NULL, NULL, RDW_INVALIDATE);
	}

	if (this->ddPallete)
	{
		delete this->ddPallete;
		this->ddPallete = NULL;
	}

	if (this->palette)
	{
		free(this->palette);
		this->palette = NULL;
	}

	if (this->indexBuffer)
	{
		free(this->indexBuffer);
		this->indexBuffer = NULL;
	}
}