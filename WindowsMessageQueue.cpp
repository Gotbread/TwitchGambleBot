#include "WindowsMessageQueue.h"

Event::Event()
{
	hEvent = CreateEvent(0, 0, 0, 0);
}
Event::~Event()
{
	CloseHandle(hEvent);
}
void Event::SetEvent()
{
	::SetEvent(hEvent);
}
void Event::WaitForEvent()
{
	WaitForSingleObject(hEvent, INFINITE);
}
Event::WaitResult Event::WaitForEventOrMessage()
{
	auto res = MsgWaitForMultipleObjects(1, &hEvent, FALSE, INFINITE, QS_ALLEVENTS);
	if (res == WAIT_OBJECT_0)
		return WaitEvent;
	if (res == WAIT_OBJECT_0 + 1)
		return WaitMessage;
	return WaitFail;
}