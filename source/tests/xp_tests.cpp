#include <array>
#include <cassert>
#include <climits>
#include <cstdint>
#include <iostream>
#include <vector>

#include "../quality/xp.hpp"
#include "../quality/minimap.hpp"

namespace
{
	using quality::xp::Follower;
	using quality::xp::OwnerBase;

	void testReadmePercentages()
	{
		constexpr std::array<int, 4> player = { 100, 90, 80, 70 };
		constexpr std::array<int, 4> follower = { 150, 130, 110, 90 };
		for ( int partySize = 1; partySize <= 4; ++partySize )
		{
			const auto index = static_cast<std::size_t>(partySize - 1);
			assert(quality::xp::playerPercent(partySize) == player[index]);
			assert(quality::xp::followerPercent(partySize) == follower[index]);
		}
		assert(quality::xp::playerPercent(0) == 100);
		assert(quality::xp::playerPercent(5) == 70);
		assert(quality::xp::followerPercent(0) == 150);
		assert(quality::xp::followerPercent(5) == 90);
	}

	void testRoundingAndMinimum()
	{
		assert(quality::xp::playerGain(11, 1) == 11);
		assert(quality::xp::playerGain(11, 2) == 9);
		assert(quality::xp::playerGain(11, 3) == 8);
		assert(quality::xp::playerGain(11, 4) == 7);
		assert(quality::xp::playerGain(1, 2) == 3);
		assert(quality::xp::playerGain(2, 4) == 3);
		assert(quality::xp::playerGain(0, 4) == 0);

		assert(quality::xp::followerGain(11, 1) == 16);
		assert(quality::xp::followerGain(11, 2) == 14);
		assert(quality::xp::followerGain(11, 3) == 12);
		assert(quality::xp::followerGain(11, 4) == 9);
		assert(quality::xp::followerGain(1, 4) == 0);
		assert(quality::xp::scale(INT_MAX, 150) == INT_MAX);
	}

	void testInspiration()
	{
		assert(quality::xp::withInspiration(16, 25) == 20);
		assert(quality::xp::withInspiration(15, 10) == 16);
		assert(quality::xp::withInspiration(13, 0) == 13);
		assert(quality::xp::withInspiration(
			quality::xp::followerGain(11, 4), 25) == 11);
	}

	void testOwnerSpecificRouting()
	{
		std::array<OwnerBase, 4> bases {};
		bases[0] = { true, 10 };
		bases[1] = { true, 14 };
		const std::vector<Follower> followers = {
			{ 101, 0, true, 0 },
			{ 202, 1, true, 0 },
		};
		const auto awards = quality::xp::routeFollowers(2, bases, followers);
		assert(awards.size() == 2);
		assert(awards[0].key == 101 && awards[0].baseGain == 13);
		assert(awards[1].key == 202 && awards[1].baseGain == 18);
	}

	void testEligibilityDeduplicationAndLastHit()
	{
		std::array<OwnerBase, 4> bases {};
		bases[0] = { true, 100 };
		bases[1] = { true, 80 };
		const std::vector<Follower> followers = {
			{ 101, 0, true, 0 },       // last-hitting follower
			{ 202, 1, true, 25 },
			{ 101, 1, true, 0 },       // duplicate party-list entry
			{ 303, 0, false, 0 },      // dead or temporary follower
			{ 404, 2, true, 0 },       // owner produced no base award
			{ 0, 0, true, 0 },
		};
		const auto awards = quality::xp::routeFollowers(4, bases, followers);
		assert(awards.size() == 2);
		assert(awards[0].key == 101 && awards[0].gain == 90);
		assert(awards[1].key == 202 && awards[1].baseGain == 72
			&& awards[1].gain == 90);
	}
}

int main()
{
	static_assert(quality::minimap::dimension == 512);
	static_assert(quality::minimap::tileIndex(17, 9) == 4625);
	static_assert(quality::minimap::white == 0xFFFFFFFFU);
	static_assert(quality::minimap::stationBlue == 0xFFFFDC8CU);
	static_assert(quality::minimap::minotaurRed == 0xFF0000FFU);
	static_assert(quality::minimap::shadowGray == 0xFFBFBFBFU);
	static_assert(quality::minimap::followerGhostScale == 0.7);
	static_assert(quality::minimap::ownerColor(2, 2)
		== quality::minimap::white);
	static_assert(quality::minimap::ownerColor(0, 2)
		== quality::minimap::playerGreen);
	static_assert(quality::minimap::ownerColor(1, 2)
		== quality::minimap::playerRed);
	static_assert(quality::minimap::ownerColor(3, 2)
		== quality::minimap::playerPink);
	static_assert(quality::minimap::ordinarilyExplored(1));
	static_assert(quality::minimap::ordinarilyExplored(2));
	static_assert(!quality::minimap::ordinarilyExplored(3));
	static_assert(quality::minimap::exitVisible(1, 0, false));
	static_assert(quality::minimap::exitVisible(3, 0, false));
	static_assert(quality::minimap::exitVisible(4, 0, true));
	static_assert(quality::minimap::exitVisible(0, 1, false));
	static_assert(!quality::minimap::exitVisible(0, 1, true));
	static_assert(!quality::minimap::exitVisible(0, 0, false));
	static_assert(quality::minimap::isBoulder(245, false));
	static_assert(quality::minimap::isBoulder(989, false));
	static_assert(!quality::minimap::isBoulder(989, true));
	static_assert(quality::minimap::isExitSprite(161));
	static_assert(quality::minimap::isExitSprite(256));
	static_assert(!quality::minimap::isExitSprite(258));
	static_assert(quality::minimap::isDetectedHostile(0x06000001U));
	static_assert(!quality::minimap::isDetectedHostile(0x06000000U));
	static_assert(!quality::minimap::isDetectedHostile(0x05000001U));
	static_assert(quality::minimap::classifyEntity(0, false, 0, true, false, false)
		== quality::minimap::MarkerAppearance::Exit);
	static_assert(quality::minimap::classifyEntity(990, false, 0, false, false, false)
		== quality::minimap::MarkerAppearance::Boulder);
	static_assert(quality::minimap::classifyEntity(0, false, 0, false, true, false)
		== quality::minimap::MarkerAppearance::Workbench);
	static_assert(quality::minimap::classifyEntity(0, false, 0, false, false, true)
		== quality::minimap::MarkerAppearance::Cauldron);
	static_assert(quality::minimap::classifyEntity(0, false, 0x06000001U,
		false, false, false) == quality::minimap::MarkerAppearance::DetectedHostile);
	testReadmePercentages();
	testRoundingAndMinimum();
	testInspiration();
	testOwnerSpecificRouting();
	testEligibilityDeduplicationAndLastHit();
	std::cout << "Quality runtime unit tests passed\n";
	return 0;
}
