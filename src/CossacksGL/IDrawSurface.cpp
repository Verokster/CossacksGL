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
#include "IDrawSurface.h"

IDrawSurface::IDrawSurface(IDrawUnknown** list)
	: IDrawUnknown(list)
{
}

// Inherited via IDrawSurface
HRESULT __stdcall IDrawSurface::AddAttachedSurface(IDrawSurface*) { return DD_OK; }
HRESULT __stdcall IDrawSurface::AddOverlayDirtyRect(LPRECT) { return DD_OK; }
HRESULT __stdcall IDrawSurface::Blt(LPRECT, IDrawSurface*, LPRECT, DWORD, LPDDBLTFX) { return DD_OK; }
HRESULT __stdcall IDrawSurface::BltBatch(LPDDBLTBATCH, DWORD, DWORD) { return DD_OK; }
HRESULT __stdcall IDrawSurface::BltFast(DWORD, DWORD, IDrawSurface*, LPRECT, DWORD) { return DD_OK; }
HRESULT __stdcall IDrawSurface::DeleteAttachedSurface(DWORD, IDrawSurface*) { return DD_OK; }
HRESULT __stdcall IDrawSurface::EnumAttachedSurfaces(LPVOID, LPDDENUMSURFACESCALLBACK) { return DD_OK; }
HRESULT __stdcall IDrawSurface::EnumOverlayZOrders(DWORD, LPVOID, LPDDENUMSURFACESCALLBACK) { return DD_OK; }
HRESULT __stdcall IDrawSurface::Flip(IDrawSurface*, DWORD) { return DD_OK; }
HRESULT __stdcall IDrawSurface::GetAttachedSurface(LPDDSCAPS, IDrawSurface**) { return DD_OK; }
HRESULT __stdcall IDrawSurface::GetBltStatus(DWORD) { return DD_OK; }
HRESULT __stdcall IDrawSurface::GetCaps(LPDDSCAPS) { return DD_OK; }
HRESULT __stdcall IDrawSurface::GetClipper(LPDIRECTDRAWCLIPPER*) { return DD_OK; }
HRESULT __stdcall IDrawSurface::GetColorKey(DWORD, LPDDCOLORKEY) { return DD_OK; }
HRESULT __stdcall IDrawSurface::GetDC(HDC*) { return DD_OK; }
HRESULT __stdcall IDrawSurface::GetFlipStatus(DWORD) { return DD_OK; }
HRESULT __stdcall IDrawSurface::GetOverlayPosition(LPLONG, LPLONG) { return DD_OK; }
HRESULT __stdcall IDrawSurface::GetPalette(IDrawPalette**) { return DD_OK; }
HRESULT __stdcall IDrawSurface::GetPixelFormat(LPDDPIXELFORMAT) { return DD_OK; }
HRESULT __stdcall IDrawSurface::GetSurfaceDesc(LPDDSURFACEDESC) { return DD_OK; }
HRESULT __stdcall IDrawSurface::Initialize(IDraw*, LPDDSURFACEDESC) { return DD_OK; }
HRESULT __stdcall IDrawSurface::IsLost() { return DD_OK; }
HRESULT __stdcall IDrawSurface::Lock(LPRECT, LPDDSURFACEDESC, DWORD, HANDLE) { return DD_OK; }
HRESULT __stdcall IDrawSurface::ReleaseDC(HDC) { return DD_OK; }
HRESULT __stdcall IDrawSurface::Restore() { return DD_OK; }
HRESULT __stdcall IDrawSurface::SetClipper(LPDIRECTDRAWCLIPPER) { return DD_OK; }
HRESULT __stdcall IDrawSurface::SetColorKey(DWORD, LPDDCOLORKEY) { return DD_OK; }
HRESULT __stdcall IDrawSurface::SetOverlayPosition(LONG, LONG) { return DD_OK; }
HRESULT __stdcall IDrawSurface::SetPalette(IDrawPalette*) { return DD_OK; }
HRESULT __stdcall IDrawSurface::Unlock(LPRECT) { return DD_OK; }
HRESULT __stdcall IDrawSurface::UpdateOverlay(LPRECT, IDrawSurface*, LPRECT, DWORD, LPDDOVERLAYFX) { return DD_OK; }
HRESULT __stdcall IDrawSurface::UpdateOverlayDisplay(DWORD) { return DD_OK; }
HRESULT __stdcall IDrawSurface::UpdateOverlayZOrder(DWORD, IDrawSurface*) { return DD_OK; }