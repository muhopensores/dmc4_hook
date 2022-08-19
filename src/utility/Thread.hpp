#pragma once

#include <queue>
#include <atlbase.h>

using HANDLE = void *;

class AutoHandle : public CHandle
{
public:
	explicit AutoHandle (HANDLE h_handle) noexcept : CHandle (h_handle) { };
	~AutoHandle (void)           noexcept
	{
		if ((intptr_t)m_h < (intptr_t)nullptr)
			m_h =           nullptr;
	}
};

namespace utility {
	void resume_threads(std::queue<DWORD>& threads);
	std::queue<DWORD> suspend_all_other_threads(void);
}