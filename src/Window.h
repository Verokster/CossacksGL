#pragma once

namespace Window
{
	VOID __fastcall CheckMenu();
	VOID __fastcall SetCaptureMouse(BOOL state);
	VOID __fastcall SetCaptureKeys(BOOL state);
	VOID __fastcall SetCaptureWindow(HWND hWnd);
	VOID __fastcall SetCapturePanel(HWND hWnd);
}