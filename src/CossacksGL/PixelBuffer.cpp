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

		MOV ESI, ptr1
		MOV EDI, ptr2

		MOV EAX, slice
		SAL EAX, 2

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

		MOV ESI, ptr1
		MOV EDI, ptr2

		MOV EAX, slice
		SAL EAX, 2

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

DWORD __forceinline ForwardCompare(DWORD* ptr1, DWORD* ptr2, DWORD slice, DWORD width, DWORD height, DWORD pitch, POINT* p)
{
	__asm {
		MOV EAX, width
		MOV EDX, height

		MOV EBX, pitch
		SUB EBX, EAX
		SAL EBX, 2

		MOV ESI, ptr1
		MOV EDI, ptr2

		MOV ECX, slice
		SAL ECX, 2

		ADD ESI, ECX
		ADD EDI, ECX

		lbl_cycle:
			MOV ECX, EAX
			REPE CMPSD
			JNE lbl_break

			ADD ESI, EBX
			ADD EDI, EBX
		DEC EDX
		JNZ lbl_cycle

		XOR EAX, EAX
		JMP lbl_ret

		lbl_break:
		MOV EBX, p
		
		INC ECX
		SUB EAX, ECX
		MOV [EBX], EAX

		MOV EAX, height
		SUB EAX, EDX
		MOV [EBX+4], EAX

		XOR EAX, EAX
		INC EAX

		lbl_ret:
	}
}

DWORD __forceinline BackwardCompare(DWORD* ptr1, DWORD* ptr2, DWORD slice, DWORD width, DWORD height, DWORD pitch, POINT* p)
{
	__asm {
		MOV EAX, width
		MOV EDX, height

		MOV EBX, pitch
		SUB EBX, EAX
		SAL EBX, 2

		MOV ESI, ptr1
		MOV EDI, ptr2

		MOV ECX, slice
		SAL ECX, 2

		ADD ESI, ECX
		ADD EDI, ECX

		STD

		lbl_cycle:
			MOV ECX, EAX
			REPE CMPSD
			JNZ lbl_break

			SUB ESI, EBX
			SUB EDI, EBX
		DEC EDX
		JNZ lbl_cycle

		XOR EAX, EAX
		JMP lbl_ret

		lbl_break:
		MOV EBX, p
		
		MOV [EBX], ECX

		DEC EDX
		MOV [EBX+4], EDX

		XOR EAX, EAX
		INC EAX

		lbl_ret:
		CLD
	}
}

DWORD __forceinline ForwardCompare(DWORD* ptr1, DWORD* ptr2, DWORD slice, DWORD width, DWORD height, DWORD pitch)
{
	__asm {
		MOV EAX, width
		MOV EDX, height

		MOV EBX, pitch
		SUB EBX, EAX
		SAL EBX, 2

		MOV ESI, ptr1
		MOV EDI, ptr2

		MOV ECX, slice
		SAL ECX, 2

		ADD ESI, ECX
		ADD EDI, ECX

		lbl_cycle:
			MOV ECX, EAX
			REPE CMPSD
			JZ lbl_inc
			
			SUB EAX, ECX
			DEC EAX
			JZ lbl_ret

			SAL ECX, 2
			ADD EBX, ECX
			ADD ESI, EBX
			ADD EDI, EBX
			ADD EBX, 4
			JMP lbl_cont

			lbl_inc:
			ADD ESI, EBX
			ADD EDI, EBX
			
			lbl_cont:
		DEC EDX
		JNZ lbl_cycle

		lbl_ret:
		MOV ECX, width
		SUB ECX, EAX
		MOV EAX, ECX
	}
}

DWORD __forceinline BackwardCompare(DWORD* ptr1, DWORD* ptr2, DWORD slice, DWORD width, DWORD height, DWORD pitch)
{
	__asm {
		MOV EAX, width
		MOV EDX, height

		MOV EBX, pitch
		SUB EBX, EAX
		SAL EBX, 2

		MOV ESI, ptr1
		MOV EDI, ptr2

		MOV ECX, slice
		SAL ECX, 2

		ADD ESI, ECX
		ADD EDI, ECX

		STD

		lbl_cycle:
			MOV ECX, EAX
			REPE CMPSD
			JZ lbl_inc
			
			SUB EAX, ECX
			DEC EAX
			JZ lbl_ret

			SAL ECX, 2
			ADD EBX, ECX
			SUB ESI, EBX
			SUB EDI, EBX
			ADD EBX, 4
			JMP lbl_cont

			lbl_inc:
			SUB ESI, EBX
			SUB EDI, EBX
			
			lbl_cont:
		DEC EDX
		JNZ lbl_cycle

		lbl_ret:
		MOV ECX, width
		SUB ECX, EAX
		MOV EAX, ECX

		CLD
	}
}

PixelBuffer::PixelBuffer(DWORD width, DWORD height, DWORD pitch, BOOL isTrue)
{
	this->isTrue = isTrue;
	if (!isTrue)
	{
		this->size = { (width + 3) >> 2, height };
		this->block = { BLOCK_SIZE >> 2, BLOCK_SIZE };
		this->pitch = pitch >> 2;
		this->length = pitch * height;
	}
	else
	{
		this->size = { width, height };
		this->block = { BLOCK_SIZE, BLOCK_SIZE };
		this->pitch = pitch;
		this->length = (pitch * height) << 2;
	}

	this->primaryBuffer = (DWORD*)AlignedAlloc(this->length);
	this->secondaryBuffer = (DWORD*)AlignedAlloc(this->length);

#ifdef BLOCK_DEBUG
	this->tempBuffer = AlignedAlloc(this->length);
	MemoryZero(this->tempBuffer, this->length);
#endif

	this->reset = TRUE;
}

PixelBuffer::~PixelBuffer()
{
	AlignedFree(this->primaryBuffer);
	AlignedFree(this->secondaryBuffer);

#ifdef BLOCK_DEBUG
	AlignedFree(this->tempBuffer);
#endif
}

VOID PixelBuffer::Prepare(VOID* data)
{
	MemoryCopy(this->primaryBuffer, data, this->length);
}

BOOL PixelBuffer::Update()
{
	BOOL res = FALSE;

	if (this->reset)
	{
		this->reset = FALSE;

		if (!this->isTrue)
			GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->size.width << 2, this->size.height, GL_ALPHA, GL_UNSIGNED_BYTE, this->primaryBuffer);
		else
			GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->size.width, this->size.height, GL_RGBA, GL_UNSIGNED_BYTE, this->primaryBuffer);

		res = TRUE;
	}
	else
	{
		DWORD size = this->pitch * this->size.height;
		DWORD index = ForwardCompare((DWORD*)this->secondaryBuffer, (DWORD*)this->primaryBuffer, 0, size);
		if (index)
		{
#ifdef BLOCK_DEBUG
			if (!this->isTrue)
				GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->size.width << 2, this->size.height, GL_ALPHA, GL_UNSIGNED_BYTE, this->tempBuffer);
			else
				GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->size.width, this->size.height, GL_RGBA, GL_UNSIGNED_BYTE, this->tempBuffer);
#endif

			DWORD top = (size - index) / this->pitch;
			for (DWORD y = top; y < this->size.height; y += this->block.height)
			{
				RECT rc;
				rc.top = *(LONG*)&y;
				rc.bottom = rc.top + this->block.height;
				if (rc.bottom > *(LONG*)&this->size.height)
					rc.bottom = *(LONG*)&this->size.height;

				for (DWORD x = 0; x < this->size.width; x += this->block.width)
				{
					rc.left = *(LONG*)&x;
					rc.right = rc.left + this->block.width;
					if (rc.right > *(LONG*)&this->size.width)
						rc.right = *(LONG*)&this->size.width;

					this->UpdateBlock(&rc);
				}
			}

			res = TRUE;
		}
	}

	if (res)
	{
		DWORD* buffer = this->secondaryBuffer;
		this->secondaryBuffer = this->primaryBuffer;
		this->primaryBuffer = buffer;
	}

	return res;
}

BOOL PixelBuffer::UpdateBlock(RECT* rect)
{
	RECT rc;
	Size size = { rect->right - rect->left, rect->bottom - rect->top };
	if (ForwardCompare(this->primaryBuffer, this->secondaryBuffer,
			rect->top * this->pitch + rect->left,
			size.width, size.height, this->pitch, (POINT*)&rc.left)
		&& BackwardCompare(this->primaryBuffer, this->secondaryBuffer,
			(rect->bottom - 1) * this->pitch + (rect->right - 1),
			size.width, size.height, this->pitch, (POINT*)&rc.right))
	{
		if (rc.left > rc.right)
		{
			LONG p = rc.left;
			rc.left = rc.right;
			rc.right = p;
		}

		rc.left += rect->left;
		rc.right += rect->left;
		rc.top += rect->top;
		rc.bottom += rect->top;

		if (rc.bottom != rc.top)
		{
			if (rc.left != rect->left)
				rc.left -= ForwardCompare(this->primaryBuffer, this->secondaryBuffer,
					(rc.top + 1) * this->pitch + rect->left,
					rc.left - rect->left, rc.bottom - rc.top, this->pitch);

			if (rc.right != (rect->right - 1))
				rc.right += BackwardCompare(this->primaryBuffer, this->secondaryBuffer,
								(rc.bottom - 1) * this->pitch + (rect->right - 1),
								rect->right - rc.right, rc.bottom - rc.top, this->pitch)
					- 1;
		}

		if (!this->isTrue)
			GLTexSubImage2D(GL_TEXTURE_2D, 0, rc.left << 2, rc.top, (rc.right - rc.left + 1) << 2, rc.bottom - rc.top + 1, GL_ALPHA, GL_UNSIGNED_BYTE, this->primaryBuffer + rc.top * this->pitch + rc.left);
		else
			GLTexSubImage2D(GL_TEXTURE_2D, 0, rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top + 1, GL_RGBA, GL_UNSIGNED_BYTE, this->primaryBuffer + rc.top * this->pitch + rc.left);

		return TRUE;
	}

	return FALSE;
}