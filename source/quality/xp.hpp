#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <unordered_set>
#include <vector>

namespace quality::xp
{
	constexpr int minimumPartySize = 1;
	constexpr int maximumPartySize = 4;

	constexpr int clampPartySize(const int partySize)
	{
		return partySize < minimumPartySize ? minimumPartySize
			: (partySize > maximumPartySize ? maximumPartySize : partySize);
	}

	constexpr int playerPercent(const int partySize)
	{
		return 100 - (clampPartySize(partySize) - 1) * 10;
	}

	constexpr int followerPermille(const int partySize)
	{
		constexpr std::array<int, maximumPartySize> percentages = {
			1250, 1125, 1000, 875
		};
		return percentages[static_cast<std::size_t>(
			clampPartySize(partySize) - minimumPartySize)];
	}

	constexpr double followerPercent(const int partySize)
	{
		return followerPermille(partySize) / 10.0;
	}

	constexpr int scale(const int value, const int percent)
	{
		const auto scaled = static_cast<std::int64_t>(value) * percent / 100;
		return scaled > std::numeric_limits<int>::max()
			? std::numeric_limits<int>::max()
			: (scaled < std::numeric_limits<int>::min()
				? std::numeric_limits<int>::min()
				: static_cast<int>(scaled));
	}

	constexpr int playerGain(const int unsharedXp, const int partySize)
	{
		if ( unsharedXp <= 0 || clampPartySize(partySize) == 1 )
		{
			return unsharedXp;
		}
		const int gain = scale(unsharedXp, playerPercent(partySize));
		return gain < 3 ? 3 : gain;
	}

	constexpr int followerGain(const int ownerUnsharedXp, const int partySize)
	{
		const auto scaled = static_cast<std::int64_t>(ownerUnsharedXp)
			* followerPermille(partySize) / 1000;
		return scaled > std::numeric_limits<int>::max()
			? std::numeric_limits<int>::max()
			: (scaled < std::numeric_limits<int>::min()
				? std::numeric_limits<int>::min()
				: static_cast<int>(scaled));
	}

	constexpr int withInspiration(const int xp, const int inspirationPercent)
	{
		return inspirationPercent == 0
			? xp
			: scale(xp, 100 + inspirationPercent);
	}

	struct OwnerBase
	{
		bool available = false;
		int xp = 0;
	};

	struct Follower
	{
		std::uintptr_t key = 0;
		int owner = -1;
		bool eligible = false;
		int inspirationPercent = 0;
	};

	struct FollowerAward
	{
		std::uintptr_t key = 0;
		int owner = -1;
		int baseGain = 0;
		int gain = 0;
	};

	inline std::vector<FollowerAward> routeFollowers(const int partySize,
		const std::array<OwnerBase, maximumPartySize>& ownerBases,
		const std::vector<Follower>& followers)
	{
		std::vector<FollowerAward> awards;
		std::unordered_set<std::uintptr_t> awarded;
		for ( const auto& follower : followers )
		{
			if ( !follower.eligible || follower.key == 0
				|| follower.owner < 0 || follower.owner >= maximumPartySize )
			{
				continue;
			}
			const auto& ownerBase = ownerBases[static_cast<std::size_t>(follower.owner)];
			if ( !ownerBase.available || !awarded.insert(follower.key).second )
			{
				continue;
			}
			const int baseGain = followerGain(ownerBase.xp, partySize);
			awards.push_back({ follower.key, follower.owner, baseGain,
				withInspiration(baseGain, follower.inspirationPercent) });
		}
		return awards;
	}
}
