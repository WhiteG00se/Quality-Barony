#pragma once

#include <cstdint>
#include <vector>

namespace quality::runtime
{
	struct Patch
	{
		std::uintptr_t rva = 0;
		std::vector<std::uint8_t> expected;
		std::vector<std::uint8_t> replacement;
		std::vector<std::uint8_t> original;
		bool applied = false;
	};
}
