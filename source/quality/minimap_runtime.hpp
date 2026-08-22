#pragma once

#include <cstdint>
#include <vector>

#include "runtime_patch.hpp"

namespace quality::minimap_runtime
{
	bool prepare(std::uint8_t* moduleBase,
		std::vector<quality::runtime::Patch>& patches);
	void release();
}
