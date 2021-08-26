#pragma once

#include <queue>
#include <atlbase.h>

using HANDLE = void *;

class AutoHandle : public CHandle
{
public:
	AutoHandle (HANDLE hHandle) noexcept : CHandle (hHandle) { };
	~AutoHandle (void)           noexcept
	{
		if ((intptr_t)m_h < (intptr_t)nullptr)
			m_h =           nullptr;
	}
};

namespace utils {
	void ResumeThreads(std::queue<DWORD> threads);
	std::queue<DWORD> SuspendAllOtherThreads(void);
}