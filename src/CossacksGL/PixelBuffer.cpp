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
#include "PixelBuffer.h"

PixelBuffer::PixelBuffer(DWORD width, DWORD height)
{
	this->width = width >> 2;
	this->height = height;
	this->length = width * height;

	this->primaryBuffer = AlignedAlloc(this->length);
	this->secondaryBuffer = AlignedAlloc(this->length);

	this->reset = TRUE;
}

PixelBuffer::~PixelBuffer()
{
	AlignedFree(this->primaryBuffer);
	AlignedFree(this->secondaryBuffer);
}

VOID PixelBuffer::Reset()
{
	this->reset = TRUE;
}

DWORD __forceinline ForwardCompare(DWORD* ptr1, DWORD* ptr2, DWORD slice, DWORD count)
{
	__asm {
		MOV ECX, count
		TEST ECX, ECX
		JZ lbl_ret

		MOV ESI, ptr1
		MOV EDI, ptr2

		MOV EAX, slice
		LEA EAX, [EAX * 0x4]

		ADD ESI, EAX
		ADD EDI, EAX

		REPE CMPSD
		JZ lbl_ret
		INC ECX

		lbl_ret:
		MOV EAX, ECX
	}
}

DWORD __forceinline BackwardCompare(DWORD* ptr1, DWORD* ptr2, DWORD slice, DWORD count)
{
	__asm {
		MOV ECX, count
		TEST ECX, ECX
		JZ lbl_ret

		MOV ESI, ptr1
		MOV EDI, ptr2

		MOV EAX, slice
		LEA EAX, [EAX * 0x4]

		ADD ESI, EAX
		ADD EDI, EAX

		STD
		REPE CMPSD
		CLD
		JZ lbl_ret
		INC ECX

		lbl_ret:
		MOV EAX, ECX
	}
}

BOOL PixelBuffer::Update(VOID* data)
{
	BOOL res = FALSE;

	MemoryCopy(this->primaryBuffer, data, this->length);

	if (this->reset)
	{
		this->reset = FALSE;

		GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->width << 2, this->height, GL_ALPHA, GL_UNSIGNED_BYTE, this->primaryBuffer);

		res = TRUE;
	}
	else
	{
		DWORD size = this->width * this->height;
		DWORD index = ForwardCompare((DWORD*)this->secondaryBuffer, (DWORD*)this->primaryBuffer, 0, size);
		if (index)
		{
			DWORD top = (size - index) / width;
			for (DWORD y = top; y < this->height; y += BLOCK_HEIGHT)
			{
				DWORD bottom = y + BLOCK_HEIGHT;
				if (bottom > this->height)
					bottom = this->height;

				for (DWORD x = 0; x < this->width; x += BLOCK_WIDTH)
				{
					DWORD right = x + BLOCK_WIDTH;
					if (right > this->width)
						right = this->width;

					RECT rc = { *(LONG*)&x, *(LONG*)&y, *(LONG*)&right, *(LONG*)&bottom };
					this->UpdateBlock(&rc);
				}
			}

			res = TRUE;
		}
	}

	if (res)
	{
		VOID* buffer = this->secondaryBuffer;
		this->secondaryBuffer = this->primaryBuffer;
		this->primaryBuffer = buffer;
	}

	return res;
}

BOOL PixelBuffer::UpdateBlock(RECT* newRect)
{
	DWORD found = NULL;
	RECT rc;

	{
		DWORD* srcData = (DWORD*)this->secondaryBuffer + newRect->top * this->width;
		DWORD* dstData = (DWORD*)this->primaryBuffer + newRect->top * this->width;

		for (LONG y = newRect->top; y < newRect->bottom; ++y)
		{
			DWORD index = ForwardCompare(srcData, dstData, newRect->left, newRect->right - newRect->left);
			if (index)
			{
				found = 1;
				rc.left = rc.right = newRect->right - index;
			}

			if (found)
			{
				LONG start = newRect->right - 1;
				rc.right += BackwardCompare(srcData, dstData, start, start - rc.right);
				rc.top = rc.bottom = y;
				break;
			}

			srcData += this->width;
			dstData += this->width;
		}
	}

	if (found)
	{
		{
			{
				DWORD* srcData = (DWORD*)this->secondaryBuffer + (newRect->bottom - 1) * this->width;
				DWORD* dstData = (DWORD*)this->primaryBuffer + (newRect->bottom - 1) * this->width;

				for (LONG y = newRect->bottom - 1; y > rc.top; --y)
				{
					DWORD index = ForwardCompare(srcData, dstData, newRect->left, newRect->right - newRect->left);
					if (index)
					{
						found |= 2;

						LONG x = newRect->right - index;
						if (rc.left > x)
							rc.left = x;
						else if (rc.right < x)
							rc.right = x;
					}

					if (found & 2)
					{
						LONG start = newRect->right - 1;
						rc.right += BackwardCompare(srcData, dstData, start, start - rc.right);
						rc.bottom = y;
						break;
					}

					srcData -= this->width;
					dstData -= this->width;
				}
			}

			{
				DWORD* srcData = (DWORD*)this->secondaryBuffer + (rc.top + 1) * this->width;
				DWORD* dstData = (DWORD*)this->primaryBuffer + (rc.top + 1) * this->width;

				for (LONG y = rc.top + 1; y < rc.bottom; ++y)
				{
					rc.left -= ForwardCompare(srcData, dstData, newRect->left, rc.left - newRect->left);

					LONG start = newRect->right - 1;
					rc.right += BackwardCompare(srcData, dstData, start, start - rc.right);

					srcData += this->width;
					dstData += this->width;
				}
			}
		}

		++rc.right;
		++rc.bottom;

		Rect rect = { rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top };

		DWORD* ptr = (DWORD*)this->primaryBuffer + rect.y * this->width + rect.x;
		GLTexSubImage2D(GL_TEXTURE_2D, 0, rect.x << 2, rect.y, rect.width << 2, rect.height, GL_ALPHA, GL_UNSIGNED_BYTE, ptr);
	}

	return found;
}
