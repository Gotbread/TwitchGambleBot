#pragma once

#include <Windows.h>
#include "ThreadCommunication.h"

class Event
{
public:
	enum WaitResult { WaitMessage, WaitEvent, WaitFail };

	Event();
	Event(const Event &other) = delete;
	Event(Event &&other) = default;
	~Event();
	Event &operator = (const Event &other) = delete;

	void SetEvent();
	void WaitForEvent();
	WaitResult WaitForEventOrMessage();
private:
	HANDLE hEvent;
};

template<class T>
struct WindowsMessageQueue
{
	enum Reason { ReasonNone, ReasonData, ReasonExit, ReasonMessage };

	WindowsMessageQueue() : should_exit(false)
	{
	}
	void Add(AsyncFunction<T> &&func)
	{
		std::lock_guard<std::mutex> guard(mut);
		function_queue.push(std::move(func));
		event.SetEvent();
	}
	void Exit()
	{
		std::lock_guard<std::mutex> guard(mut);

		should_exit = true;
		event.SetEvent();
	}
	Reason Wait(AsyncFunction<T> &func)
	{
		{
			std::lock_guard<std::mutex> guard(mut);
			if (should_exit)
				return ReasonExit;

			if (!function_queue.empty())
			{
				func.swap(function_queue.front());
				function_queue.pop();
				return ReasonData;
			}
		}
		auto res = event.WaitForEventOrMessage();
		{
			std::lock_guard<std::mutex> guard(mut);
			if (should_exit)
				return ReasonExit;

			if (res == Event::WaitMessage)
				return ReasonMessage;

			if (!function_queue.empty())
			{
				func.swap(function_queue.front());
				function_queue.pop();
				return ReasonData;
			}
		}
		return ReasonNone;
	}
	size_t Size()
	{
		std::unique_lock<std::mutex> lock(mut);

		return function_queue.size();
	}
	bool should_exit;
	std::mutex mut;
	Event event;
	std::queue<AsyncFunction<T>> function_queue;
};