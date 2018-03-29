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

#pragma once

#include "windows.h"
#include "stdio.h"

extern VOID PatchHook(DWORD addr, VOID* hook);
extern VOID PatchNop(DWORD addr, DWORD size);
extern VOID PatchDWord(DWORD addr, DWORD value);
extern VOID PatchByte(DWORD addr, BYTE value);
extern DWORD ReadDWord(DWORD addr);

extern DWORD sendCommand, getErrorString;
extern DWORD play, back;
extern const CHAR* KZ_EW;
extern const CHAR* KZ_AW;
extern const CHAR* KZ_BW;
extern const CHAR* AC;
extern const CHAR* AC_FB;

extern VOID Patch_EW_GSC();
extern VOID Patch_EW_GOG();

extern VOID Patch_AW_GSC();

extern VOID Patch_BW_GSC();
extern VOID Patch_BW_GOG();

extern VOID Patch_AC_GSC();
extern VOID Patch_AC_GOG();

extern VOID Patch_FB_GSC();
extern VOID Patch_FB_GOG();