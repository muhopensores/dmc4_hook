#pragma once

#include <string_view>
#include "d3d9.h"
#include "glm/vec2.hpp"
#include <vector>
#include "imgui.h"

namespace utility {
	struct Texture {
		constexpr explicit Texture(const glm::vec2 offset, const glm::vec2 size, const glm::vec2 atlas_sz) : offset_(std::move(offset)), size_(std::move(size)) {
			uv0 = glm::vec2(offset.x / atlas_sz.x, offset.y / atlas_sz.y);
			uv1 = glm::vec2((offset.x + size.x) / atlas_sz.x, (offset.y + size.y) / atlas_sz.y);
		};
		
		glm::vec2 uv0, uv1;
		glm::vec2 offset_, size_;
	};

#if 0
	struct TextureAtlas {
		//TODO(): constexpr map like thing with std::array??
		int m_width, m_height;
		PDIRECT3DTEXTURE9 m_texture_handle{ nullptr };
	};
#endif
}