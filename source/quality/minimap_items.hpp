#pragma once

#include <cstdint>

namespace quality::minimap::items
{
	constexpr int roughRockItemType = 125;
	constexpr int artifactOrbBlue = 211;
	constexpr int artifactOrbRed = 212;
	constexpr int artifactOrbPurple = 213;
	constexpr int artifactOrbGreen = 214;

	enum class Appearance : std::uint8_t
	{
		Hidden,
		Green,
		ShinyYellow,
		Blue,
	};

	constexpr bool isOrb(const int itemType)
	{
		return itemType >= artifactOrbBlue && itemType <= artifactOrbGreen;
	}

	constexpr Appearance appearance(const int itemType, const bool identified,
		const bool lootBag)
	{
		if ( lootBag || itemType == roughRockItemType )
		{
			return Appearance::Hidden;
		}
		if ( isOrb(itemType) )
		{
			return Appearance::ShinyYellow;
		}
		return identified ? Appearance::Blue : Appearance::Green;
	}

	constexpr bool eligibleGroundItem(const bool itemBehavior,
		const bool contained, const bool lootBag)
	{
		return itemBehavior && !contained && !lootBag;
	}
}
