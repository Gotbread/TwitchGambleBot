#include "GDIHelper.h"

HDCWrapper HDCWrapper::FromHDC(HDC hdc)
{
	HDCWrapper wrapper(HDCSourceExternal);
	wrapper.hdc = hdc;
	return wrapper;
}
HDCWrapper HDCWrapper::BeginPaint(HWND hWnd)
{
	HDCWrapper wrapper(HDCSourceBeginPaint);
	wrapper.hWnd = hWnd;
	wrapper.hdc = ::BeginPaint(hWnd, &wrapper.ps);
	return wrapper;
}
HDCWrapper HDCWrapper::GetDC(HWND hWnd)
{
	HDCWrapper wrapper(HDCSourceGetDC);
	wrapper.hWnd = hWnd;
	wrapper.hdc = ::GetDC(hWnd);
	return wrapper;
}
HDCWrapper HDCWrapper::CreateCompatibleDC(HDC hdc)
{
	HDCWrapper wrapper(HDCSourceCreateCompatibleDC);
	wrapper.hdc = ::CreateCompatibleDC(hdc);
	return wrapper;
}
HDCWrapper::HDCWrapper(HDCWrapper &&other) : hdc(other.hdc), source(other.source), objects(std::move(other.objects)), hWnd(other.hWnd), ps(other.ps)
{
	other.hdc = 0;
}
HDCWrapper::HDCWrapper(HDCSource source) : hdc(0), source(source)
{
}
HDCWrapper::~HDCWrapper()
{
	for (auto iter : objects)
	{
		if (iter.second.second)
			iter.second.second->UnregisterFromParent();
	}
	if (hdc)
	{
		switch (source)
		{
		case HDCSourceBeginPaint:
			EndPaint(hWnd, &ps);
			break;
		case HDCSourceGetDC:
			ReleaseDC(hWnd, hdc);
			break;
		case HDCSourceCreateCompatibleDC:
			DeleteDC(hdc);
			break;
		}
	}
}
HDCWrapper::operator HDC() const
{
	return hdc;
}
void HDCWrapper::Select(GDIObject &obj)
{
	obj.UnregisterFromParent();

	DWORD type = GetObjectType(obj.Get());

	auto &res = objects[type];
	if (!res.first)
	{
		res.first = SelectObject(hdc, obj.Get());
	}
	else
	{
		SelectObject(hdc, obj.Get());
	}
	res.second = &obj;
	obj.SetParent(this);
}
void HDCWrapper::Unselect(GDIObject &obj)
{
	DWORD type = GetObjectType(obj.Get());

	auto &res = objects[type];
	if (res.second == &obj)
	{
		if (res.first)
		{
			SelectObject(hdc, res.first);
		}
		res.second = 0;
	}
}
void HDCWrapper::UnselectType(GDIObject &obj)
{
	DWORD type = GetObjectType(obj.Get());
	if (!type)
		return;

	auto &res = objects[type];
	if (res.first)
	{
		SelectObject(hdc, res.first);
	}
	res.second = 0;
}

GDIObject::GDIObject(HGDIOBJ obj) : obj(obj), parent(0)
{
}
HGDIOBJ GDIObject::operator = (HGDIOBJ other)
{
	UnregisterFromParent();
	if (obj)
		DeleteObject(obj);
	obj = other;
	return obj;
}
GDIObject::operator bool() const
{
	return obj != 0;
}
GDIObject::~GDIObject()
{
	UnregisterFromParent();
	if (obj)
		DeleteObject(obj);
}
void GDIObject::UnregisterFromParent()
{
	if (parent)
	{
		parent->Unselect(*this);
		parent = 0;
	}
}
void GDIObject::SetParent(HDCWrapper *parent)
{
	this->parent = parent;
}
HGDIOBJ GDIObject::Get() const
{
	return obj;
}
template<class T>
T GetGDIObjectAsType(HGDIOBJ obj, DWORD target_type)
{
	if (obj && GetObjectType(obj) == target_type)
		return static_cast<T>(obj);
	return 0;
}
HCURSOR GDIObject::GetCursor() const
{
	return GetGDIObjectAsType<HCURSOR>(obj, 0);
}
HBITMAP GDIObject::GetBitmap() const
{
	return GetGDIObjectAsType<HBITMAP>(obj, OBJ_BITMAP);
}
HPEN GDIObject::GetPen() const
{
	return GetGDIObjectAsType<HPEN>(obj, OBJ_PEN);
}
HBRUSH GDIObject::GetBrush() const
{
	return GetGDIObjectAsType<HBRUSH>(obj, OBJ_BRUSH);
}
HFONT GDIObject::GetFont() const
{
	return GetGDIObjectAsType<HFONT>(obj, OBJ_FONT);
}