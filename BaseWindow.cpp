#include "BaseWindow.h"

BaseWindow::BaseWindow(HINSTANCE hInstance) : hInstance(hInstance), hWnd(0)
{
}
BaseWindow::~BaseWindow()
{
	SetWindowLongPtr(hWnd, 0, 0);

	if (hWnd)
		DestroyWindow(hWnd);
}
HWND BaseWindow::GetHWND()
{
	return hWnd;
}
void BaseWindow::RegisterClass(HINSTANCE hInstance, const char *classname, HBRUSH background, WORD iconname)
{
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.cbWndExtra = sizeof(BaseWindow *);
	wc.hbrBackground = background;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hIcon = iconname ? LoadIcon(hInstance, MAKEINTRESOURCE(iconname)) : 0;
	wc.hIconSm = wc.hIcon;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = sWndProc;
	wc.lpszClassName = classname;

	::RegisterClassEx(&wc);
}
LRESULT BaseWindow::sWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_NCCREATE)
		SetWindowLongPtr(hWnd, 0, reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCT *>(lParam)->lpCreateParams));
	BaseWindow *window = reinterpret_cast<BaseWindow *>(GetWindowLongPtr(hWnd, 0));
	if (window)
		return window->WndProc(hWnd, Msg, wParam, lParam);

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}