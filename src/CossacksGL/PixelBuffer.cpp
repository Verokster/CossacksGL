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

PixelBuffer::PixelBuffer(DWORD width, DWORD height, DWORD length)
{
	this->width = width;
	this->height = height;
	this->length = length;

	this->primaryBuffer = AlignedAlloc(this->length);
	this->secondaryBuffer = AlignedAlloc(this->length);

	this->reset = TRUE;
}

PixelBuffer::~PixelBuffer()
{
	AlignedFree(this->primaryBuffer);
	AlignedFree(this->secondaryBuffer);
}

VOID PixelBuffer::Prepare(VOID* data)
{
	MemoryCopy(this->primaryBuffer, data, this->length);
}

PixelBuffer8::PixelBuffer8(DWORD width, DWORD height)
	: PixelBuffer(width >> 2, height, width * height)
{
}

PixelBuffer32::PixelBuffer32(DWORD width, DWORD height)
	: PixelBuffer(width, height, (width * height) << 2)
{
}

BOOL PixelBuffer8::Update()
{
	BOOL res = FALSE;

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
			for (DWORD y = top; y < this->height; y += BLOCK_HEIGHT_8)
			{
				DWORD bottom = y + BLOCK_HEIGHT_8;
				if (bottom > this->height)
					bottom = this->height;

				for (DWORD x = 0; x < this->width; x += BLOCK_WIDTH_8)
				{
					DWORD right = x + BLOCK_WIDTH_8;
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

BOOL PixelBuffer32::Update()
{
	BOOL res = FALSE;

	if (this->reset)
	{
		this->reset = FALSE;
		GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->width, this->height, GL_RGBA, GL_UNSIGNED_BYTE, this->primaryBuffer);
		res = TRUE;
	}
	else
	{
		DWORD size = this->width * this->height;
		DWORD index = ForwardCompare((DWORD*)this->secondaryBuffer, (DWORD*)this->primaryBuffer, 0, size);
		if (index)
		{
			DWORD top = (size - index) / width;
			for (DWORD y = top; y < this->height; y += BLOCK_HEIGHT_32)
			{
				DWORD bottom = y + BLOCK_HEIGHT_32;
				if (bottom > this->height)
					bottom = this->height;

				for (DWORD x = 0; x < this->width; x += BLOCK_WIDTH_32)
				{
					DWORD right = x + BLOCK_WIDTH_32;
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

BOOL PixelBuffer8::UpdateBlock(RECT* rect)
{
	DWORD found = NULL;
	RECT rc;

	{
		DWORD* srcData = (DWORD*)this->secondaryBuffer + rect->top * this->width;
		DWORD* dstData = (DWORD*)this->primaryBuffer + rect->top * this->width;

		for (LONG y = rect->top; y < rect->bottom; ++y)
		{
			DWORD index = ForwardCompare(srcData, dstData, rect->left, rect->right - rect->left);
			if (index)
			{
				found = 1;
				rc.left = rc.right = rect->right - index;
			}

			if (found)
			{
				LONG start = rect->right - 1;
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
			DWORD* srcData = (DWORD*)this->secondaryBuffer + (rect->bottom - 1) * this->width;
			DWORD* dstData = (DWORD*)this->primaryBuffer + (rect->bottom - 1) * this->width;

			for (LONG y = rect->bottom - 1; y > rc.top; --y)
			{
				DWORD index = ForwardCompare(srcData, dstData, rect->left, rect->right - rect->left);
				if (index)
				{
					found |= 2;

					LONG x = rect->right - index;
					if (rc.left > x)
						rc.left = x;
					else if (rc.right < x)
						rc.right = x;
				}

				if (found & 2)
				{
					LONG start = rect->right - 1;
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
				rc.left -= ForwardCompare(srcData, dstData, rect->left, rc.left - rect->left);

				LONG start = rect->right - 1;
				rc.right += BackwardCompare(srcData, dstData, start, start - rc.right);

				srcData += this->width;
				dstData += this->width;
			}
		}

		GLTexSubImage2D(GL_TEXTURE_2D, 0, rc.left << 2, rc.top, (rc.right - rc.left + 1) << 2, rc.bottom - rc.top + 1, GL_ALPHA, GL_UNSIGNED_BYTE, (DWORD*)this->primaryBuffer + rc.top * this->width + rc.left);
	}

	return found;
}

BOOL PixelBuffer32::UpdateBlock(RECT* rect)
{
	DWORD found = NULL;
	RECT rc;

	{
		DWORD* srcData = (DWORD*)this->secondaryBuffer + rect->top * this->width;
		DWORD* dstData = (DWORD*)this->primaryBuffer + rect->top * this->width;

		for (LONG y = rect->top; y < rect->bottom; ++y)
		{
			DWORD index = ForwardCompare(srcData, dstData, rect->left, rect->right - rect->left);
			if (index)
			{
				found = 1;
				rc.left = rc.right = rect->right - index;
			}

			if (found)
			{
				LONG start = rect->right - 1;
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
			DWORD* srcData = (DWORD*)this->secondaryBuffer + (rect->bottom - 1) * this->width;
			DWORD* dstData = (DWORD*)this->primaryBuffer + (rect->bottom - 1) * this->width;

			for (LONG y = rect->bottom - 1; y > rc.top; --y)
			{
				DWORD index = ForwardCompare(srcData, dstData, rect->left, rect->right - rect->left);
				if (index)
				{
					found |= 2;

					LONG x = rect->right - index;
					if (rc.left > x)
						rc.left = x;
					else if (rc.right < x)
						rc.right = x;
				}

				if (found & 2)
				{
					LONG start = rect->right - 1;
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
				rc.left -= ForwardCompare(srcData, dstData, rect->left, rc.left - rect->left);

				LONG start = rect->right - 1;
				rc.right += BackwardCompare(srcData, dstData, start, start - rc.right);

				srcData += this->width;
				dstData += this->width;
			}
		}

		GLTexSubImage2D(GL_TEXTURE_2D, 0, rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top + 1, GL_RGBA, GL_UNSIGNED_BYTE, (DWORD*)this->primaryBuffer + rc.top * this->width + rc.left);
	}

	return found;
}