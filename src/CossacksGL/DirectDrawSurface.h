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

#pragma once
#include "IDrawSurface.h"
#include "DirectDrawPalette.h"
#include "GLib.h"
#include "Allocation.h"

class DirectDraw;

class DirectDrawSurface : public IDrawSurface {
public:
	DirectDraw* ddraw;
	DirectDrawPalette* attachedPalette;

	DirectDrawSurface(IDrawUnknown**, DirectDraw*);
	~DirectDrawSurface();

	// Inherited via IDirectDrawSurface
	HRESULT __stdcall QueryInterface(REFIID, LPVOID*) override;
	ULONG __stdcall Release() override;
	HRESULT __stdcall GetDC(HDC*) override;
	HRESULT __stdcall GetPixelFormat(LPDDPIXELFORMAT) override;
	HRESULT __stdcall Lock(LPRECT, LPDDSURFACEDESC, DWORD, HANDLE) override;
	HRESULT __stdcall SetPalette(IDrawPalette*) override;
};

