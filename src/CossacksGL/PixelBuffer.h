/*
	MIT License

	Copyright (c) 2020 Oleksiy Ryabchun

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

#include "Allocation.h"
#include "ExtraTypes.h"

#define BLOCK_SIZE 256

typedef DWORD(__fastcall* COMPARE)(DWORD, DWORD, DWORD*, DWORD*);
typedef BOOL(__fastcall* BLOCKCOMPARE)(LONG, LONG, DWORD, DWORD, DWORD*, DWORD*, POINT*);
typedef DWORD(__fastcall* SIDECOMPARE)(LONG, LONG, DWORD, DWORD, DWORD*, DWORD*);

class PixelBuffer : public Allocation {
private:
	Size size;
	Size block;
	DWORD store;
	DWORD pitch;
	DWORD length;
	BOOL isTrue;
	BOOL reset;

	DWORD* secondaryBuffer;

	COMPARE ForwardCompare;
	COMPARE BackwardCompare;
	BLOCKCOMPARE BlockForwardCompare;
	BLOCKCOMPARE BlockBackwardCompare;
	SIDECOMPARE SideForwardCompare;
	SIDECOMPARE SideBackwardCompare;

	BOOL UpdateBlock(RECT*);

public:
	DWORD* primaryBuffer;

	PixelBuffer(DWORD, DWORD, DWORD, BOOL, UpdateMode);
	~PixelBuffer();

	VOID Prepare(VOID*);
	BOOL Update();
};