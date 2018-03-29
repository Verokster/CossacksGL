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

#include "Main.h"

VOID __declspec(naked) hook_0044AB40()
{
	_asm
	{
		MOV ECX, DWORD PTR DS : [0x0090AB55]
		TEST ECX, ECX
		JNZ lbl_back
		MOV ECX, DWORD PTR SS : [EBP + 0x14]
		PUSH ECX
		MOV EDX, DWORD PTR SS : [EBP + 0x10]
		PUSH EDX
		JMP play
		lbl_back : JMP back
	}
}

VOID Patch_EW_GSC()
{
	// Window text & icon
	PatchDWord(0x0044ECFB + 1, WS_POPUP | WS_SYSMENU);
	PatchDWord(0x0044ED00 + 1, (DWORD)KZ_EW);

	// Patch Clear Window
	PatchDWord(0x0044F018 + 2, 768);
	PatchNop(0x0044F01E, 0x0044F02A - 0x0044F01E);

	// Patch DirectDraw window size
	PatchDWord(0x0044FD17 + 1, 768);
	PatchDWord(0x0044FD1C + 1, 1024);

	// Patch cursor rectangle
	PatchDWord(0x0044CA2D + 3, 1024);
	PatchDWord(0x0044CA34 + 3, 768);

	PatchDWord(0x0044CA45 + 1, 738);
	PatchDWord(0x0044CA4A + 1, 1023);

	// Patch MCI video
	PatchByte(0x0044CC53 + 1, 96);
	PatchByte(0x0044CCC5 + 1, 96);

	PatchDWord(0x0054DC79 + 6, 1024);
	PatchDWord(0x0054DC88 + 1, 768);

	// Play Video
	PatchDWord(0x0054DBDC + 2, (DWORD)&sendCommand);
	PatchDWord(0x0054DC2F + 2, (DWORD)&sendCommand);
	PatchDWord(0x0054DCAD + 2, (DWORD)&sendCommand);
	PatchDWord(0x0054DCF3 + 2, (DWORD)&sendCommand);
	
	// Stop Video
	PatchDWord(0x0054DD9B + 2, (DWORD)&sendCommand);

	PatchDWord(0x0054DC04 + 2, (DWORD)&getErrorString);
	PatchDWord(0x0054DC57 + 2, (DWORD)&getErrorString);
	PatchDWord(0x0054DCD5 + 2, (DWORD)&getErrorString);
	PatchDWord(0x0054DD1B + 2, (DWORD)&getErrorString);

	// Prevent play audio while playing video
	play = 0x0044AB48;
	back = 0x0044AB50;
	PatchHook(0x0044AB40, hook_0044AB40);
}