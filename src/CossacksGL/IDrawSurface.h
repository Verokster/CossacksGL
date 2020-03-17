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

#include "IDrawUnknown.h"
#include "IDrawPalette.h"

class IDraw;

class IDrawSurface : public IDrawUnknown {
public:
	IDrawSurface(IDrawUnknown** list);

	// Inherited via IDirectDrawSurface
	virtual HRESULT __stdcall AddAttachedSurface(IDrawSurface*);
	virtual HRESULT __stdcall AddOverlayDirtyRect(LPRECT);
	virtual HRESULT __stdcall Blt(LPRECT, IDrawSurface*, LPRECT, DWORD, LPDDBLTFX);
	virtual HRESULT __stdcall BltBatch(LPDDBLTBATCH, DWORD, DWORD);
	virtual HRESULT __stdcall BltFast(DWORD, DWORD, IDrawSurface*, LPRECT, DWORD);
	virtual HRESULT __stdcall DeleteAttachedSurface(DWORD, IDrawSurface*);
	virtual HRESULT __stdcall EnumAttachedSurfaces(LPVOID, LPDDENUMSURFACESCALLBACK);
	virtual HRESULT __stdcall EnumOverlayZOrders(DWORD, LPVOID, LPDDENUMSURFACESCALLBACK);
	virtual HRESULT __stdcall Flip(IDrawSurface*, DWORD);
	virtual HRESULT __stdcall GetAttachedSurface(LPDDSCAPS, IDrawSurface**);
	virtual HRESULT __stdcall GetBltStatus(DWORD);
	virtual HRESULT __stdcall GetCaps(LPDDSCAPS);
	virtual HRESULT __stdcall GetClipper(LPDIRECTDRAWCLIPPER*);
	virtual HRESULT __stdcall GetColorKey(DWORD, LPDDCOLORKEY);
	virtual HRESULT __stdcall GetDC(HDC*);
	virtual HRESULT __stdcall GetFlipStatus(DWORD);
	virtual HRESULT __stdcall GetOverlayPosition(LPLONG, LPLONG);
	virtual HRESULT __stdcall GetPalette(IDrawPalette**);
	virtual HRESULT __stdcall GetPixelFormat(LPDDPIXELFORMAT);
	virtual HRESULT __stdcall GetSurfaceDesc(LPDDSURFACEDESC);
	virtual HRESULT __stdcall Initialize(IDraw*, LPDDSURFACEDESC);
	virtual HRESULT __stdcall IsLost();
	virtual HRESULT __stdcall Lock(LPRECT, LPDDSURFACEDESC, DWORD, HANDLE);
	virtual HRESULT __stdcall ReleaseDC(HDC);
	virtual HRESULT __stdcall Restore();
	virtual HRESULT __stdcall SetClipper(LPDIRECTDRAWCLIPPER);
	virtual HRESULT __stdcall SetColorKey(DWORD, LPDDCOLORKEY);
	virtual HRESULT __stdcall SetOverlayPosition(LONG, LONG);
	virtual HRESULT __stdcall SetPalette(IDrawPalette*);
	virtual HRESULT __stdcall Unlock(LPRECT);
	virtual HRESULT __stdcall UpdateOverlay(LPRECT, IDrawSurface*, LPRECT, DWORD, LPDDOVERLAYFX);
	virtual HRESULT __stdcall UpdateOverlayDisplay(DWORD);
	virtual HRESULT __stdcall UpdateOverlayZOrder(DWORD, IDrawSurface*);
};