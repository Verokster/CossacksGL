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

VOID __declspec(naked) hook_004CA41C()
{
	_asm
	{
		MOV ECX, DWORD PTR DS : [0x0082B705]
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

VOID Patch_AW_GSC()
{
	// Window text & icon
	PatchDWord(0x004CEA96 + 1, WS_POPUP | WS_SYSMENU);
	PatchDWord(0x004CEA9B + 1, (DWORD)KZ_AW);
	
	// Patch DirectDraw window size
	PatchDWord(0x0045F735 + 1, 768);
	PatchDWord(0x0045F73A + 1, 1024);

	// Patch cursor rectangle
	PatchDWord(0x004CC404 + 3, 1024);
	PatchDWord(0x004CC40B + 3, 768);

	PatchDWord(0x004CC41C + 1, 738);
	PatchDWord(0x004CC421 + 1, 1023);

	// Patch MCI video
	PatchByte(0x004CC694 + 1, 96);
	PatchByte(0x004CC724 + 1, 96);

	PatchDWord(0x004C2839 + 6, 1024);
	PatchDWord(0x004C2848 + 1, 768);

	// Check video path
	PatchNop(0x004CC4EB, 0x004CC503 - 0x004CC4EB);
	PatchDWord(0x004CC50A + 1, 0x005D9B53); // kz.avi
	PatchDWord(0x004CC64F + 1, 0x005D9B73); // logo.avi
	PatchDWord(0x004CC710 + 1, 0x005D9B87); // kz.avi

	// Play Video
	PatchDWord(0x004C279C + 2, (DWORD)&sendCommand);
	PatchDWord(0x004C27EF + 2, (DWORD)&sendCommand);
	PatchDWord(0x004C286D + 2, (DWORD)&sendCommand);
	PatchDWord(0x004C28B3 + 2, (DWORD)&sendCommand);

	// Stop Video
	PatchDWord(0x004C295B + 2, (DWORD)&sendCommand);

	PatchDWord(0x004C27C4 + 2, (DWORD)&getErrorString);
	PatchDWord(0x004C2817 + 2, (DWORD)&getErrorString);
	PatchDWord(0x004C2895 + 2, (DWORD)&getErrorString);
	PatchDWord(0x004C28DB + 2, (DWORD)&getErrorString);

	// Prevent play audio while playing video
	play = 0x004CA424;
	back = 0x004CA42C;
	PatchHook(0x004CA41C, hook_004CA41C);
}