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

#pragma once

extern MciVideo mciVideo;

namespace Hooks
{
	extern INT baseOffset;

	BOOL __fastcall PatchJump(DWORD addr, DWORD dest);
	BOOL __fastcall PatchHook(DWORD addr, VOID* hook);
	BOOL __fastcall PatchCall(DWORD addr, VOID* hook);
	BOOL __fastcall PatchNop(DWORD addr, DWORD size);
	BOOL __fastcall PatchBlock(DWORD addr, VOID* block, DWORD size);
	BOOL __fastcall PatchWord(DWORD addr, WORD value);
	BOOL __fastcall PatchInt(DWORD addr, INT value);
	BOOL __fastcall PatchDWord(DWORD addr, DWORD value);
	BOOL __fastcall PatchByte(DWORD addr, BYTE value);
	BOOL __fastcall ReadWord(DWORD addr, WORD* value);
	BOOL __fastcall ReadDWord(DWORD addr, DWORD* value);
	DWORD __fastcall PatchFunction(MappedFile* file, const CHAR* function, VOID* addr);

	VOID Load();
}