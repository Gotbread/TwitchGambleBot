#pragma once

#include <Windows.h>
#include <map>
#include <functional>

class GDIObject;
class HDCWrapper
{
public:
	static HDCWrapper FromHDC(HDC hdc);
	static HDCWrapper BeginPaint(HWND hWnd);
	static HDCWrapper GetDC(HWND hWnd);
	static HDCWrapper CreateCompatibleDC(HDC hdc);

	HDCWrapper(HDCWrapper &&other);
	HDCWrapper(const HDCWrapper &other) = delete;
	HDCWrapper &operator = (const HDCWrapper &other) = delete;
	~HDCWrapper();

	void Select(GDIObject &);
	void Unselect(GDIObject &);
	void UnselectType(GDIObject &);

	operator HDC() const;
private:
	enum HDCSource { HDCSourceExternal, HDCSourceBeginPaint, HDCSourceGetDC, HDCSourceCreateCompatibleDC } source;
	HDCWrapper(HDCSource source);

	HWND hWnd;
	PAINTSTRUCT ps;

	HDC hdc;

	std::map<DWORD, std::pair<HGDIOBJ, GDIObject *>> objects;
};
class GDIObject
{
public:
	GDIObject(HGDIOBJ obj = 0);
	HGDIOBJ operator = (HGDIOBJ other);

	GDIObject(const GDIObject &other) = delete;
	GDIObject &operator = (const GDIObject &other) = delete;

	explicit operator bool() const;

	~GDIObject();

	HGDIOBJ Get() const;
	HCURSOR GetCursor() const;
	HBITMAP GetBitmap() const;
	HPEN GetPen() const;
	HBRUSH GetBrush() const;
	HFONT GetFont() const;
private:
	friend class HDCWrapper;

	void UnregisterFromParent();
	void SetParent(HDCWrapper *parent);

	HDCWrapper *parent;
	HGDIOBJ obj;
};