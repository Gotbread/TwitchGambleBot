#include "ThreadCommunication.h"

ThreadInterface::ThreadInterface()
{
	ThreadManager::AddThread(this);
}
ThreadInterface::~ThreadInterface()
{
	ThreadManager::RemoveThread(this);
}
bool ThreadManager::Run()
{
	size_t size = threads.size();
	std::vector<Signal<bool>> init_signals(size), run_signals(size);

	unsigned i = 0;
	for (auto thread : threads)
	{
		thread->Run(&init_signals[i], &run_signals[i]);
		++i;
	}
	bool success = true;
	for (auto &signal : init_signals)
	{
		if (!signal.Wait())
		{
			success = false;
			break;
		}
	}
	for (auto &signal : run_signals)
		signal.Set(success);
	for (auto &signal : init_signals) // to ensure no thread accesses already destroyed signals
		signal.Wait();
	return success;
}
void ThreadManager::Exit()
{
	for (auto thread : threads)
		thread->ExitThread();
}
void ThreadManager::Wait()
{
	for (auto thread : threads)
		thread->WaitThread();
}
void ThreadManager::AddThread(ThreadInterface *thread)
{
	threads.push_back(thread);
}
void ThreadManager::RemoveThread(ThreadInterface *thread)
{
	threads.erase(std::remove(threads.begin(), threads.end(), thread));
}

std::vector<ThreadInterface *> ThreadManager::threads;

#ifdef _WINDOWS

#include <Windows.h>
#include <cstring>

const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)  
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.  
	LPCSTR szName; // Pointer to name (in user addr space).  
	DWORD dwThreadID; // Thread ID (-1=caller thread).  
	DWORD dwFlags; // Reserved for future use, must be zero.  
} THREADNAME_INFO;
#pragma pack(pop)  
void SetThreadName(const char* threadName)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = -1;
	info.dwFlags = 0;
#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
#pragma warning(pop)  
}
// removes "class" from the front
const char *CleanName(const char *name)
{
	const char *to_compare = "class ";
	size_t len = strlen(to_compare);
	if (!strncmp(name, to_compare, len))
		name += len;
	return name;
}
#endif