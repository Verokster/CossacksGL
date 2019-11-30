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

DWORD __declspec(naked) __fastcall ForwardCompare(DWORD count, DWORD slice, VOID* ptr1, VOID* ptr2)
{
	__asm {
		push ebp
		mov ebp, esp
		push esi
		push edi

		mov esi, ptr1
		mov edi, ptr2

		sal edx, 2
		add esi, edx
		add edi, edx

		repe cmpsd
		jz lbl_ret
		inc ecx

		lbl_ret:
		mov eax, ecx

		pop edi
		pop esi
		mov esp, ebp
		pop ebp

		retn 8
	}
}

DWORD __declspec(naked) __fastcall BackwardCompare(DWORD count, DWORD slice, VOID* ptr1, VOID* ptr2)
{
	__asm {
		push ebp
		mov ebp, esp
		push esi
		push edi

		mov esi, ptr1
		mov edi, ptr2

		sal edx, 2
		add esi, edx
		add edi, edx

		std
		repe cmpsd
		cld
		jz lbl_ret
		inc ecx

		lbl_ret:
		mov eax, ecx

		pop edi
		pop esi
		mov esp, ebp
		pop ebp

		retn 8
	}
}

BOOL __declspec(naked) __fastcall ForwardCompare(DWORD width, DWORD height, DWORD pitch, DWORD slice, VOID* ptr1, VOID* ptr2, POINT* p)
{
	__asm {
		push ebp
		mov ebp, esp
		sub esp, __LOCAL_SIZE
		push ebx
		push esi
		push edi

		mov height, edx
		mov eax, ecx

		mov ebx, pitch
		sub ebx, eax
		sal ebx, 2

		mov esi, ptr1
		mov edi, ptr2

		mov ecx, slice
		sal ecx, 2

		add esi, ecx
		add edi, ecx

		lbl_cycle:
			mov ecx, eax
			repe cmpsd
			jne lbl_break

			add esi, ebx
			add edi, ebx
		dec edx
		jnz lbl_cycle

		xor eax, eax
		jmp lbl_ret

		lbl_break:
		mov ebx, p
		
		inc ecx
		sub eax, ecx
		mov [ebx], eax

		mov eax, height
		sub eax, edx
		mov [ebx+4], eax

		xor eax, eax
		inc eax

		lbl_ret:
		pop edi
		pop esi
		pop ebx
		mov esp, ebp
		pop ebp

		retn 20
	}
}

BOOL __declspec(naked) __fastcall BackwardCompare(DWORD width, DWORD height, DWORD pitch, DWORD slice, VOID* ptr1, VOID* ptr2, POINT* p)
{
	__asm {
		push ebp
		mov ebp, esp
		push ebx
		push esi
		push edi

		mov eax, ecx

		mov ebx, pitch
		sub ebx, eax
		sal ebx, 2

		mov esi, ptr1
		mov edi, ptr2

		mov ecx, slice
		sal ecx, 2

		add esi, ecx
		add edi, ecx

		std

		lbl_cycle:
			mov ecx, eax
			repe cmpsd
			jnz lbl_break

			sub esi, ebx
			sub edi, ebx
		dec edx
		jnz lbl_cycle

		xor eax, eax
		jmp lbl_ret

		lbl_break:
		mov ebx, p
		
		mov [ebx], ecx

		dec edx
		mov [ebx+4], edx

		xor eax, eax
		inc eax

		lbl_ret:
		cld
		pop edi
		pop esi
		pop ebx
		mov esp, ebp
		pop ebp

		retn 20
	}
}

DWORD __declspec(naked) __fastcall ForwardCompare(DWORD width, DWORD height, DWORD pitch, DWORD slice, VOID* ptr1, VOID* ptr2)
{
	__asm {
		push ebp
		mov ebp, esp
		sub esp, __LOCAL_SIZE
		push ebx
		push esi
		push edi

		mov width, ecx
		mov eax, ecx

		mov ebx, pitch
		sub ebx, eax
		sal ebx, 2

		mov esi, ptr1
		mov edi, ptr2

		mov ecx, slice
		sal ecx, 2

		add esi, ecx
		add edi, ecx

		lbl_cycle:
			mov ecx, eax
			repe cmpsd
			jz lbl_inc
			
			sub eax, ecx
			dec eax
			jz lbl_ret

			sal ecx, 2
			add ebx, ecx
			add esi, ebx
			add edi, ebx
			add ebx, 4
			jmp lbl_cont

			lbl_inc:
			add esi, ebx
			add edi, ebx
			
			lbl_cont:
		dec edx
		jnz lbl_cycle

		lbl_ret:
		mov ecx, width
		sub ecx, eax
		mov eax, ecx

		pop edi
		pop esi
		pop ebx
		mov esp, ebp
		pop ebp

		retn 16
	}
}

DWORD __declspec(naked) __fastcall BackwardCompare(DWORD width, DWORD height, DWORD pitch, DWORD slice, VOID* ptr1, VOID* ptr2)
{
	__asm {
		push ebp
		mov ebp, esp
		sub esp, __LOCAL_SIZE
		push ebx
		push esi
		push edi

		mov width, ecx
		mov eax, ecx

		mov ebx, pitch
		sub ebx, eax
		sal ebx, 2

		mov esi, ptr1
		mov edi, ptr2

		mov ecx, slice
		sal ecx, 2

		add esi, ecx
		add edi, ecx

		std

		lbl_cycle:
			mov ecx, eax
			repe cmpsd
			jz lbl_inc
			
			sub eax, ecx
			dec eax
			jz lbl_ret

			sal ecx, 2
			add ebx, ecx
			sub esi, ebx
			sub edi, ebx
			add ebx, 4
			jmp lbl_cont

			lbl_inc:
			sub esi, ebx
			sub edi, ebx
			
			lbl_cont:
		dec edx
		jnz lbl_cycle

		lbl_ret:
		mov ecx, width
		sub ecx, eax
		mov eax, ecx

		cld

		pop edi
		pop esi
		pop ebx
		mov esp, ebp
		pop ebp

		retn 16
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
		DWORD left, right;
		DWORD size = this->pitch * this->size.height;
		if ((left = ForwardCompare(size, 0, (DWORD*)this->primaryBuffer, (DWORD*)this->secondaryBuffer))
			&& (right = BackwardCompare(size, size - 1, (DWORD*)this->primaryBuffer, (DWORD*)this->secondaryBuffer)))
		{
#ifdef BLOCK_DEBUG
			if (!this->isTrue)
				GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->size.width << 2, this->size.height, GL_ALPHA, GL_UNSIGNED_BYTE, this->tempBuffer);
			else
				GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->size.width, this->size.height, GL_RGBA, GL_UNSIGNED_BYTE, this->tempBuffer);
#endif

			DWORD top = (size - left) / this->pitch;
			DWORD bottom = (right - 1) / this->pitch + 1;
			for (DWORD y = top; y < bottom; y += this->block.height)
			{
				RECT rc;
				rc.top = *(LONG*)&y;
				rc.bottom = rc.top + this->block.height;
				if (rc.bottom > *(LONG*)&bottom)
					rc.bottom = *(LONG*)&bottom;
				--rc.bottom;

				for (DWORD x = 0; x < this->size.width; x += this->block.width)
				{
					rc.left = *(LONG*)&x;
					rc.right = rc.left + this->block.width;
					if (rc.right > *(LONG*)&this->size.width)
						rc.right = *(LONG*)&this->size.width;
					--rc.right;

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
	Size size = { rect->right - rect->left + 1, rect->bottom - rect->top + 1 };
	if (ForwardCompare(size.width, size.height, this->pitch,
			rect->top * this->pitch + rect->left,
			this->primaryBuffer, this->secondaryBuffer, (POINT*)&rc.left)
		&& BackwardCompare(size.width, size.height, this->pitch,
			rect->bottom * this->pitch + rect->right,
			this->primaryBuffer, this->secondaryBuffer, (POINT*)&rc.right))
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

		LONG height = rc.bottom - rc.top;
		if (height > 1)
		{
			if (rc.left != rect->left)
				rc.left -= ForwardCompare(rc.left - rect->left, height, this->pitch,
					(rc.top + 1) * this->pitch + rect->left,
					this->primaryBuffer, this->secondaryBuffer);

			if (rc.right != rect->right)
				rc.right += BackwardCompare(rect->right - rc.right, height, this->pitch,
					(rc.bottom - 1) * this->pitch + rect->right,
					this->primaryBuffer, this->secondaryBuffer);
		}

		if (!this->isTrue)
			GLTexSubImage2D(GL_TEXTURE_2D, 0, rc.left << 2, rc.top, (rc.right - rc.left + 1) << 2, height + 1, GL_ALPHA, GL_UNSIGNED_BYTE, this->primaryBuffer + rc.top * this->pitch + rc.left);
		else
			GLTexSubImage2D(GL_TEXTURE_2D, 0, rc.left, rc.top, rc.right - rc.left + 1, height + 1, GL_RGBA, GL_UNSIGNED_BYTE, this->primaryBuffer + rc.top * this->pitch + rc.left);

		return TRUE;
	}

	return FALSE;
}