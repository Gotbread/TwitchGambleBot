#pragma once

#include <Windows.h>

class BaseWindow
{
public:
	BaseWindow(HINSTANCE hInstance = 0);
	virtual ~BaseWindow();

	HWND GetHWND();
protected:
	static void RegisterClass(HINSTANCE hInstance, const char *classname, HBRUSH background, WORD iconname);

	static LRESULT CALLBACK sWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	virtual LRESULT WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) = 0;

	HINSTANCE hInstance;
	HWND hWnd;
};
