#pragma once

#include "cResource.hpp"

class rEffectList : public cResource
{
public:
	using uint32_t = unsigned int;
	float m_base_fps; //0x0060
	char *m_param_buf; //0x0064
	uint32_t m_param_buff_size; //0x0068
	uint32_t m_list_num; //0x006C
	uint32_t n00002_d88; //0x0070
	uint32_t n00002_d89; //0x0074
	uint32_t m_resource_ptr; //0x0078
	uint32_t m_resource_info_num; //0x007C
}; //Size: 0x0080
static_assert(sizeof(rEffectList) == 0x0080);
