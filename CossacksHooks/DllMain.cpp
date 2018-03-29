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
#include "windows.h"

VOID PatchHook(DWORD addr, VOID* hook)
{
	BYTE* jump;

	DWORD old_prot;
	VirtualProtect((VOID*)addr, 5, PAGE_EXECUTE_READWRITE, &old_prot);

	jump = (BYTE*)addr;
	*jump = 0xE9;
	++jump;
	*(DWORD*)jump = (DWORD)hook - (DWORD)addr - 5;

	VirtualProtect((VOID*)addr, 5, old_prot, &old_prot);
}

VOID PatchNop(DWORD addr, DWORD size)
{
	DWORD old_prot;
	VirtualProtect((VOID*)addr, size, PAGE_EXECUTE_READWRITE, &old_prot);

	memset((VOID*)addr, 0x90, size);

	VirtualProtect((VOID*)addr, size, old_prot, &old_prot);
}

VOID PatchBlock(DWORD addr, VOID* block, DWORD size)
{
	DWORD old_prot;
	VirtualProtect((VOID*)addr, size, PAGE_EXECUTE_READWRITE, &old_prot);

	switch (size)
	{
	case 4:
		*(DWORD*)addr = *(DWORD*)block;
		break;
	case 2:
		*(WORD*)addr = *(WORD*)block;
		break;
	case 1:
		*(BYTE*)addr = *(BYTE*)block;
		break;
	default:
		memcpy((VOID*)addr, block, size);
		break;
	}

	VirtualProtect((VOID*)addr, size, old_prot, &old_prot);
}

VOID ReadBlock(DWORD addr, VOID* block, DWORD size)
{
	DWORD old_prot;
	VirtualProtect((VOID*)addr, size, PAGE_READONLY, &old_prot);

	switch (size)
	{
	case 4:
		*(DWORD*)block = *(DWORD*)addr;
		break;
	case 2:
		*(WORD*)block = *(WORD*)addr;
		break;
	case 1:
		*(BYTE*)block = *(BYTE*)addr;
		break;
	default:
		memcpy(block, (VOID*)addr, size);
		break;
	}

	VirtualProtect((VOID*)addr, size, old_prot, &old_prot);
}

VOID PatchDWord(DWORD addr, DWORD value)
{
	PatchBlock(addr, &value, sizeof(value));
}

VOID PatchByte(DWORD addr, BYTE value)
{
	PatchBlock(addr, &value, sizeof(value));
}

DWORD ReadDWord(DWORD addr)
{
	DWORD value;
	ReadBlock(addr, &value, sizeof(value));
	return value;
}

// =============================================================================================================================
BOOL called;
DWORD sendCommand, getErrorString;
DWORD play, back;
const CHAR* KZ_EW = "Cossacks: European Wars";
const CHAR* KZ_AW = "Cossacks: The Art Of War";
const CHAR* KZ_BW = "Cossacks: Back To War";
const CHAR* AC = "American Conquest";

VOID InjectPatch()
{
	if (!called)
	{
		sendCommand = (DWORD)mciSendCommand;
		getErrorString = (DWORD)mciGetErrorString;

		if (ReadDWord(0x0044ECFB + 1) == WS_POPUP)
			Patch_EW_GSC();
		else if (ReadDWord(0x0044EDCA + 1) == WS_POPUP)
			Patch_EW_GOG();
		else if (ReadDWord(0x004CEA96 + 1) == WS_POPUP)
			Patch_AW_GSC();
		else if (ReadDWord(0x004D30F0 + 1) == WS_POPUP)
			Patch_BW_GSC();
		else if (ReadDWord(0x004D30E3 + 1) == WS_POPUP)
			Patch_BW_GOG();
		else if (ReadDWord(0x004A8B89 + 1) == WS_POPUP)
			Patch_AC_GSC();
		else if (ReadDWord(0x004A8B8E + 1) == WS_POPUP)
			Patch_AC_GOG();
		else if (ReadDWord(0x004AA3BC + 1) == WS_POPUP)
			Patch_FB_GSC();
		else if (ReadDWord(0x004AA35C + 1) == WS_POPUP)
			Patch_FB_GOG();
	}

	called = TRUE;
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		InjectPatch();
		break;
	}

	default:
		break;
	}

	return TRUE;
}