#pragma once

#include <cstddef>
#include <cstdint>

namespace quality::minimap
{
	constexpr int dimension = 512;

	constexpr std::size_t tileIndex(const int x, const int y)
	{
		return static_cast<std::size_t>(y) * dimension
			+ static_cast<std::size_t>(x);
	}

	constexpr std::uint32_t color(const std::uint8_t red,
		const std::uint8_t green, const std::uint8_t blue,
		const std::uint8_t alpha = 255)
	{
		return static_cast<std::uint32_t>(red)
			| (static_cast<std::uint32_t>(green) << 8)
			| (static_cast<std::uint32_t>(blue) << 16)
			| (static_cast<std::uint32_t>(alpha) << 24);
	}

	constexpr std::uint32_t white = color(255, 255, 255);
	constexpr std::uint32_t playerGreen = color(64, 255, 64);
	constexpr std::uint32_t playerRed = color(255, 64, 64);
	constexpr std::uint32_t playerPink = color(255, 160, 255);
	constexpr std::uint32_t minotaurRed = color(255, 0, 0);
	constexpr std::uint32_t shadowGray = color(191, 191, 191);
	constexpr std::uint32_t boulderCyan = color(0, 255, 255);
	constexpr std::uint32_t stationBlue = color(140, 220, 255);
	constexpr double followerGhostScale = 0.7;

	constexpr std::uint32_t ownerColor(const int owner, const int viewer,
		const std::uint32_t fallback = white)
	{
		if ( owner < 0 || owner >= 4 || viewer < 0 || viewer >= 4 )
		{
			return fallback;
		}
		if ( owner == viewer )
		{
			return white;
		}
		int remoteRank = 0;
		for ( int player = 0; player < owner; ++player )
		{
			if ( player != viewer )
			{
				++remoteRank;
			}
		}
		switch ( remoteRank )
		{
			case 0: return playerGreen;
			case 1: return playerRed;
			case 2: return playerPink;
			default: return fallback;
		}
	}

	constexpr bool ordinarilyExplored(const std::uint8_t visibility)
	{
		return visibility == 1 || visibility == 2;
	}

	constexpr bool exitVisible(const std::uint8_t visibility,
		const std::uint32_t showOnMap, const bool customPortal)
	{
		return visibility != 0
			|| (!customPortal && (showOnMap & 0xFFFFFFU) != 0);
	}

	constexpr bool isBoulder(const int sprite, const bool invisible)
	{
		return !invisible && (sprite == 245 || sprite == 989 || sprite == 990);
	}

	constexpr bool isExitSprite(const int sprite)
	{
		return sprite == 161 || sprite == 278 || sprite == 614
			|| (sprite >= 254 && sprite < 258);
	}

	constexpr bool isDetectedHostile(const std::uint32_t showOnMap)
	{
		return (showOnMap >> 24) == 6 && (showOnMap & 0xFFFFFFU) != 0;
	}

	enum class MarkerAppearance : std::uint8_t
	{
		None,
		Exit,
		Boulder,
		Workbench,
		Cauldron,
		DetectedHostile,
	};

	constexpr MarkerAppearance classifyEntity(const int sprite,
		const bool invisible, const std::uint32_t showOnMap,
		const bool customPortal, const bool workbench, const bool cauldron)
	{
		if ( isExitSprite(sprite) || customPortal )
		{
			return MarkerAppearance::Exit;
		}
		if ( isBoulder(sprite, invisible) )
		{
			return MarkerAppearance::Boulder;
		}
		if ( workbench )
		{
			return MarkerAppearance::Workbench;
		}
		if ( cauldron )
		{
			return MarkerAppearance::Cauldron;
		}
		if ( isDetectedHostile(showOnMap) )
		{
			return MarkerAppearance::DetectedHostile;
		}
		return MarkerAppearance::None;
	}
}
