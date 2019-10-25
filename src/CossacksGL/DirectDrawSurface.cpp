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
#include "GLib.h"
#include "DirectDrawSurface.h"
#include "DirectDraw.h"

DirectDrawSurface::DirectDrawSurface(IDrawUnknown** list, DirectDraw* lpDD)
	: IDrawSurface(list)
{
	this->ddraw = lpDD;
	this->attachedPalette = NULL;
}

DirectDrawSurface::~DirectDrawSurface()
{
	if (this->ddraw->attachedSurface == this)
	{
		this->ddraw->RenderStop();
		this->ddraw->attachedSurface = NULL;
	}

	if (this->attachedPalette)
		this->attachedPalette->Release();
}

ULONG DirectDrawSurface::Release()
{
	if (--this->refCount)
		return this->refCount;

	delete this;
	return 0;
}

HRESULT DirectDrawSurface::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
	*ppvObj = new IDrawUnknown(NULL);
	return DD_OK;
}

HRESULT DirectDrawSurface::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	MemoryZero(lpDDPixelFormat, sizeof(DDPIXELFORMAT));
	lpDDPixelFormat->dwSize = sizeof(DDPIXELFORMAT);

	lpDDPixelFormat->dwRGBBitCount = 32;
	lpDDPixelFormat->dwRBitMask = 0x000000FF;
	lpDDPixelFormat->dwGBitMask = 0x0000FF00;
	lpDDPixelFormat->dwBBitMask = 0x00FF0000;
	lpDDPixelFormat->dwRGBAlphaBitMask = 0xFF000000;

	return DD_OK;
}

HRESULT DirectDrawSurface::GetDC(HDC* hDc)
{
	*hDc = this->ddraw->hDc;
	return DD_OK;
}

HRESULT DirectDrawSurface::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	lpDDSurfaceDesc->dwWidth = this->ddraw->dwMode->width;
	lpDDSurfaceDesc->dwHeight = this->ddraw->dwMode->height;
	lpDDSurfaceDesc->lPitch = this->ddraw->pitch;
	lpDDSurfaceDesc->lpSurface = this->ddraw->indexBuffer;

	return DD_OK;
}

HRESULT DirectDrawSurface::SetPalette(IDrawPalette* lpDDPalette)
{
	DirectDrawPalette* old = this->attachedPalette;
	this->attachedPalette = (DirectDrawPalette*)lpDDPalette;

	if (this->attachedPalette)
	{
		if (old != this->attachedPalette)
		{
			if (old)
				old->Release();

			this->attachedPalette->AddRef();
		}
	}
	else if (old)
		old->Release();

	return DD_OK;
}