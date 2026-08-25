#pragma once

#include <array>
#include <cstddef>
#include <cstdio>
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
	constexpr std::uint32_t detectedPurple = color(191, 127, 191);
	constexpr std::uint32_t boulderCyan = color(0, 255, 255);
	constexpr std::uint32_t stationBlue = color(140, 220, 255);
	constexpr std::uint32_t uninteractedGreen = color(0x55, 0x6B, 0x2F);
	constexpr std::uint32_t interactedBlue = color(0x41, 0x69, 0xE1);
	constexpr std::uint32_t shinyYellow = color(0xFF, 0xD7, 0x00);
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

	enum class ExitCreatureDisposition : std::uint8_t
	{
		Excluded,
		Hostile,
		Neutral,
	};

	constexpr ExitCreatureDisposition classifyExitCreature(
		const bool monster, const int allyIndex, const bool hasStats,
		const int hp, const bool friendly)
	{
		if ( !monster || allyIndex >= 0 || !hasStats || hp <= 0 )
		{
			return ExitCreatureDisposition::Excluded;
		}
		return friendly ? ExitCreatureDisposition::Neutral
			: ExitCreatureDisposition::Hostile;
	}

	inline int formatCreatureCounts(char* output, const std::size_t outputSize,
		const int hostiles, const int neutrals)
	{
		if ( !output || outputSize == 0 )
		{
			return -1;
		}
		return std::snprintf(output, outputSize,
			"%d hostiles / %d neutrals alive", hostiles, neutrals);
	}

	inline int formatExitTooltip(char* output, const std::size_t outputSize,
		const char* exitText, const int hostiles, const int neutrals)
	{
		if ( !output || outputSize == 0 )
		{
			return -1;
		}
		std::array<char, 64> counts {};
		formatCreatureCounts(counts.data(), counts.size(), hostiles, neutrals);
		return std::snprintf(output, outputSize, "%s\n%s",
			exitText ? exitText : "", counts.data());
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

	constexpr bool isDetectedUnit(const std::uint32_t showOnMap)
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
		DetectedUnit,
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
		if ( isDetectedUnit(showOnMap) )
		{
			return MarkerAppearance::DetectedUnit;
		}
		return MarkerAppearance::None;
	}
}
