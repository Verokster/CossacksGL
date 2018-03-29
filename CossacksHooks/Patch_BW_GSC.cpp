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

VOID __declspec(naked) hook_004CE8C6()
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

VOID Patch_BW_GSC()
{
	// Window text & icon
	PatchDWord(0x004D30F0 + 1, WS_POPUP | WS_SYSMENU);
	PatchDWord(0x004D30F5 + 1, (DWORD)KZ_BW);
	
	// Patch DirectDraw window size
	PatchDWord(0x004606BE + 1, 768);
	PatchDWord(0x004606C3 + 1, 1024);

	// Patch cursor rectangle
	PatchDWord(0x004D0712 + 3, 1024);
	PatchDWord(0x004D0719 + 3, 768);

	PatchDWord(0x004D072A + 1, 738);
	PatchDWord(0x004D072F + 1, 1023);

	// Patch MCI video
	PatchByte(0x004D0974 + 1, 96);
	PatchByte(0x004D09FD + 1, 96);

	PatchDWord(0x004C69C9 + 6, 1024);
	PatchDWord(0x004C69D8 + 1, 768);

	// Play Video
	PatchDWord(0x004C692C + 2, (DWORD)&sendCommand);
	PatchDWord(0x004C697F + 2, (DWORD)&sendCommand);
	PatchDWord(0x004C69FD + 2, (DWORD)&sendCommand);
	PatchDWord(0x004C6A43 + 2, (DWORD)&sendCommand);

	// Stop Video
	PatchDWord(0x004C6AEB + 2, (DWORD)&sendCommand);

	PatchDWord(0x004C6954 + 2, (DWORD)&getErrorString);
	PatchDWord(0x004C69A7 + 2, (DWORD)&getErrorString);
	PatchDWord(0x004C6A25 + 2, (DWORD)&getErrorString);
	PatchDWord(0x004C6A6B + 2, (DWORD)&getErrorString);

	// Prevent play audio while playing video
	play = 0x004CE8CE;
	back = 0x004CE8D6;
	PatchHook(0x004CE8C6, hook_004CE8C6);
}