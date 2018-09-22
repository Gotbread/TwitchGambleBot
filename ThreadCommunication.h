#pragma once

#include <functional>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

#ifdef _WINDOWS
void SetThreadName(const char* threadName);
const char *CleanName(const char *name);
#endif

template<class T>
class Signal
{
public:
	Signal() : signaled(false)
	{
	}
	Signal(const Signal &other) = default;
	void Set(T t)
	{
		std::lock_guard<std::mutex> guard(mut);
		signaled = true;
		data = t;
		cond.notify_one();
	}
	T Wait()
	{
		std::unique_lock<std::mutex> lock(mut);
		cond.wait(lock, [&] {return signaled; });
		signaled = false;
		return data;
	}
private:
	std::condition_variable cond;
	std::mutex mut;
	bool signaled;
	T data;
};

class ThreadManager;
class ThreadInterface
{
public:
	ThreadInterface();
	virtual ~ThreadInterface();

	virtual void Run(Signal<bool> *init_signal, Signal<bool> *run_signal) = 0;
	virtual void ExitThread() = 0;
	virtual void WaitThread() = 0;
protected:
	friend class ThreadManager;
};

template<class T>
using AsyncFunction = std::function<void(T *)>;

template<class T>
struct MessageQueue
{
	enum Reason { ReasonNone, ReasonData, ReasonExit };

	MessageQueue() : wakeup_reason(ReasonNone), should_exit(false)
	{
	}
	void Add(AsyncFunction<T> &&func)
	{
		std::lock_guard<std::mutex> guard(mut);
		function_queue.push(std::move(func));

		wakeup_reason = ReasonData;
		cond.notify_one();
	}
	void Exit()
	{
		std::lock_guard<std::mutex> guard(mut);

		should_exit = true;
		wakeup_reason = ReasonExit;
		cond.notify_one();
	}
	Reason Wait(AsyncFunction<T> &func)
	{
		std::unique_lock<std::mutex> lock(mut);

		if (should_exit)
			return ReasonExit;

		if (!function_queue.empty())
		{
			func.swap(function_queue.front());
			function_queue.pop();
			return ReasonData;
		}

		// buddifix 03.09.2018
		do
		{
			cond.wait(lock);
		} while (wakeup_reason == ReasonNone);

		if (should_exit)
			return ReasonExit;

		if (!function_queue.empty())
		{
			func.swap(function_queue.front());
			function_queue.pop();
			return ReasonData;
		}

		return ReasonNone;
	}
	size_t Size()
	{
		std::unique_lock<std::mutex> lock(mut);

		return function_queue.size();
	}
	Reason wakeup_reason;
	bool should_exit;
	std::mutex mut;
	std::condition_variable cond;
	std::queue<AsyncFunction<T>> function_queue;
};

template<class T, class QueueType = MessageQueue<T>>
class BaseThread : public ThreadInterface
{
public:
	typedef QueueType Queue;

	template<typename F, typename ...Args>
	static void RPC(F f, Args&& ...args)
	{
		queue.Add(std::bind(f, std::placeholders::_1, std::forward<Args>(args)...));
	}
	static size_t QueueSize()
	{
		return queue.Size();
	}
	virtual bool OnInit()
	{
		return true;
	}
	virtual void OnStart()
	{
	}
	virtual void OnExit()
	{
	}
	virtual void OnOtherEvent(typename Queue::Reason)
	{
	}
	std::thread::id GetThreadID()
	{
		return thread.get_id();
	}
	static std::thread::id GetFirstThreadID()
	{
		return first_thread_id;
	}
private:
	void Run(Signal<bool> *init_signal, Signal<bool> *run_signal) override
	{
		thread = std::thread(&BaseThread<T, QueueType>::ThreadFunc, this, init_signal, run_signal);
	}
	void ExitThread() override
	{
		queue.Exit();
	}
	void WaitThread() override
	{
		thread.join();
	}
	void ThreadFunc(Signal<bool> *init_signal, Signal<bool> *run_signal)
	{
		{
#ifdef _WINDOWS
#ifdef _DEBUG
			SetThreadName(CleanName(typeid(T).name()));
#endif
#endif
			std::lock_guard<std::mutex> lock(thread_id_mutex);
			if (first_thread_id != std::thread::id())
				first_thread_id = GetThreadID();
		}

		init_signal->Set(OnInit());
		bool run = run_signal->Wait();
		init_signal->Set(true);
		if (!run)
			return;

		OnStart();

		for (;;)
		{
			AsyncFunction<T> func;
			auto reason = queue.Wait(func);
			if (reason == queue.ReasonData)
			{
				func(static_cast<T *>(this));
			}
			else if (reason == queue.ReasonExit)
			{
				break;
			}
			else
			{
				OnOtherEvent(reason);
			}
		}

		OnExit();
	}

	static Queue queue;
	static std::thread::id first_thread_id;
	static std::mutex thread_id_mutex;
	std::thread thread;
};

class ThreadManager
{
public:
	static bool Run();
	static void Exit();
	static void Wait();
private:
	friend class ThreadInterface;

	static void AddThread(ThreadInterface *thread);
	static void RemoveThread(ThreadInterface *thread);

	static std::vector<ThreadInterface *> threads;
};

template<class T, class QueueType>
QueueType BaseThread<T, QueueType>::queue;
template<class T, class QueueType>
std::thread::id BaseThread<T, QueueType>::first_thread_id;
template<class T, class QueueType>
std::mutex BaseThread<T, QueueType>::thread_id_mutex;
