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

VOID __declspec(naked) hook_004CE179()
{
	_asm
	{
		MOV ECX, DWORD PTR DS : [0x0083AC65]
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

VOID Patch_BW_GOG()
{
	// Window text & icon
	PatchDWord(0x004D30E3 + 1, WS_POPUP | WS_SYSMENU);
	PatchDWord(0x004D30E8 + 1, (DWORD)KZ_BW);
	
	// Patch DirectDraw window size
	PatchDWord(0x004606BE + 1, 768);
	PatchDWord(0x004606C3 + 1, 1024);

	// Patch cursor rectangle
	PatchDWord(0x004D06E2 + 3, 1024);
	PatchDWord(0x004D06E9 + 3, 768);

	PatchDWord(0x004D06FA + 1, 738);
	PatchDWord(0x004D06FF + 1, 1023);

	// Patch MCI video
	PatchByte(0x004D0944 + 1, 96);
	PatchByte(0x004D09CD + 1, 96);

	PatchDWord(0x004C6999 + 6, 1024);
	PatchDWord(0x004C69A8 + 1, 768);

	// Play Video
	PatchDWord(0x004C68FC + 2, (DWORD)&sendCommand);
	PatchDWord(0x004C694F + 2, (DWORD)&sendCommand);
	PatchDWord(0x004C69CD + 2, (DWORD)&sendCommand);
	PatchDWord(0x004C6A13 + 2, (DWORD)&sendCommand);

	// Stop Video
	PatchDWord(0x004C6ABB + 2, (DWORD)&sendCommand);

	PatchDWord(0x004C6924 + 2, (DWORD)&getErrorString);
	PatchDWord(0x004C6977 + 2, (DWORD)&getErrorString);
	PatchDWord(0x004C69F5 + 2, (DWORD)&getErrorString);
	PatchDWord(0x004C6A3B + 2, (DWORD)&getErrorString);

	// Prevent play audio while playing video
	play = 0x004CE181;
	back = 0x004CE189;
	PatchHook(0x004CE179, hook_004CE179);
}