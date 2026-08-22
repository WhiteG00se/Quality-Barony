#include <array>
#include <cassert>
#include <climits>
#include <cstdint>
#include <iostream>
#include <vector>

#include "../quality/xp.hpp"
#include "../quality/minimap.hpp"
#include "../quality/minimap_items.hpp"
#include "../quality/minimap_reveal.hpp"

namespace
{
	using quality::xp::Follower;
	using quality::xp::OwnerBase;

	void testReadmePercentages()
	{
		constexpr std::array<int, 4> player = { 100, 90, 80, 70 };
		constexpr std::array<int, 4> follower = { 125, 100, 90, 80 };
		for ( int partySize = 1; partySize <= 4; ++partySize )
		{
			const auto index = static_cast<std::size_t>(partySize - 1);
			assert(quality::xp::playerPercent(partySize) == player[index]);
			assert(quality::xp::followerPercent(partySize) == follower[index]);
		}
		assert(quality::xp::playerPercent(0) == 100);
		assert(quality::xp::playerPercent(5) == 70);
		assert(quality::xp::followerPercent(0) == 125);
		assert(quality::xp::followerPercent(5) == 80);
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

		assert(quality::xp::followerGain(11, 1) == 13);
		assert(quality::xp::followerGain(11, 2) == 11);
		assert(quality::xp::followerGain(11, 3) == 9);
		assert(quality::xp::followerGain(11, 4) == 8);
		assert(quality::xp::followerGain(1, 4) == 0);
		assert(quality::xp::scale(INT_MAX, 150) == INT_MAX);
	}

	void testInspiration()
	{
		assert(quality::xp::withInspiration(16, 25) == 20);
		assert(quality::xp::withInspiration(15, 10) == 16);
		assert(quality::xp::withInspiration(13, 0) == 13);
		assert(quality::xp::withInspiration(
			quality::xp::followerGain(11, 4), 25) == 10);
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
		assert(awards[0].key == 101 && awards[0].baseGain == 10);
		assert(awards[1].key == 202 && awards[1].baseGain == 14);
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
		assert(awards[0].key == 101 && awards[0].gain == 80);
		assert(awards[1].key == 202 && awards[1].baseGain == 64
			&& awards[1].gain == 80);
	}

	void testRevealEligibility()
	{
		using namespace quality::minimap::reveal;
		Candidate candidate { 1, 2, 3, CandidateKind::Chest };
		assert(eligible(candidate, false));
		assert(!eligible(candidate, true));
		candidate.kind = CandidateKind::Grave;
		assert(eligible(candidate, false));
		candidate.kind = CandidateKind::Fountain;
		candidate.available = false;
		assert(!eligible(candidate, false));
		candidate.kind = CandidateKind::BreakableContainer;
		candidate.available = true;
		assert(!eligible(candidate, false));
		candidate.hasLoot = true;
		assert(eligible(candidate, false));
		candidate.kind = CandidateKind::Item;
		candidate.hasLoot = false;
		assert(eligible(candidate, false));
		candidate.playerOwned = true;
		assert(!eligible(candidate, false));
		candidate.playerOwned = false;
		candidate.lootBag = true;
		assert(!eligible(candidate, false));
		candidate.kind = CandidateKind::Gold;
		candidate.lootBag = false;
		candidate.playerOwned = true;
		assert(eligible(candidate, false));
		candidate.available = false;
		assert(!eligible(candidate, false));
		candidate.kind = CandidateKind::None;
		candidate.available = true;
		candidate.playerOwned = false;
		candidate.lootBag = false;
		assert(!eligible(candidate, false));
	}

	void testGroundGoldEligibility()
	{
		using quality::minimap::reveal::eligibleGroundGold;
		static_assert(eligibleGroundGold(1, 0));
		static_assert(eligibleGroundGold(4, 0));
		static_assert(eligibleGroundGold(5, 0));
		static_assert(eligibleGroundGold(1000, 0));
		static_assert(!eligibleGroundGold(0, 0));
		static_assert(!eligibleGroundGold(-1, 0));
		static_assert(!eligibleGroundGold(1, 42));
	}

	void testExitCreatureCountsAndText()
	{
		using quality::minimap::ExitCreatureDisposition;
		using quality::minimap::classifyExitCreature;
		assert(classifyExitCreature(true, -1, true, 10, false)
			== ExitCreatureDisposition::Hostile);
		assert(classifyExitCreature(true, -1, true, 10, true)
			== ExitCreatureDisposition::Neutral);
		assert(classifyExitCreature(true, 0, true, 10, false)
			== ExitCreatureDisposition::Excluded);
		assert(classifyExitCreature(true, -1, true, 0, false)
			== ExitCreatureDisposition::Excluded);
		assert(classifyExitCreature(true, -1, true, -1, false)
			== ExitCreatureDisposition::Excluded);
		assert(classifyExitCreature(true, -1, false, 10, false)
			== ExitCreatureDisposition::Excluded);
		assert(classifyExitCreature(false, -1, true, 10, false)
			== ExitCreatureDisposition::Excluded);

		std::array<char, 128> text {};
		quality::minimap::formatExitTooltip(text.data(), text.size(),
			"Exit dungeon floor", 0, 0);
		assert(std::string(text.data())
			== "Exit dungeon floor\n[0 hostiles / 0 neutrals alive]");
		quality::minimap::formatExitTooltip(text.data(), text.size(),
			"Exit secret level", 1, 1);
		assert(std::string(text.data())
			== "Exit secret level\n[1 hostiles / 1 neutrals alive]");
		quality::minimap::formatExitTooltip(text.data(), text.size(),
			"Step through portal", 12, 3);
		assert(std::string(text.data())
			== "Step through portal\n[12 hostiles / 3 neutrals alive]");
	}

	void testPersistentRevealLifecycle()
	{
		using namespace quality::minimap::reveal;
		State state;
		state.reset();
		state.observeLive({{5, 1, 1, CandidateKind::Item}});
		assert(!state.contains(5));
		state.rememberInitialUses(10, 2);
		state.refresh({
			{10, 1, 1, CandidateKind::Fountain},
			{20, 2, 2, CandidateKind::Item},
			{30, 3, 3, CandidateKind::Exit},
		});
		assert(state.active());
		assert(state.contains(10));
		assert(state.contains(20));
		assert(state.contains(30));

		state.observeLive({
			{10, 4, 5, CandidateKind::Fountain},
			{30, 3, 3, CandidateKind::Exit},
			{40, 4, 4, CandidateKind::Item},
		});
		assert(state.contains(10));
		assert(!state.contains(20));
		assert(state.contains(40));
		const auto markers = state.markers();
		assert(markers.size() == 3);
		assert(markers[0].uid == 10 && markers[0].x == 4 && markers[0].y == 5);
		assert(markers[2].uid == 40
			&& markers[2].kind == Kind::Unused);

		Candidate ineligible {50, 5, 5, CandidateKind::Item};
		ineligible.playerOwned = true;
		Candidate lootBag {51, 5, 6, CandidateKind::Item};
		lootBag.lootBag = true;
		state.observeLive({
			{10, 4, 5, CandidateKind::Fountain},
			{30, 3, 3, CandidateKind::Exit},
			{40, 4, 4, CandidateKind::Item},
			ineligible,
			lootBag,
		});
		assert(!state.contains(50));
		assert(!state.contains(51));
		ineligible.playerOwned = false;
		state.observeLive({
			{10, 4, 5, CandidateKind::Fountain},
			{30, 3, 3, CandidateKind::Exit},
			{40, 4, 4, CandidateKind::Item},
			ineligible,
		});
		assert(state.contains(50));

		state.observeUses(10, 1);
		assert(!state.contains(10));
		state.observeLive({
			{10, 1, 1, CandidateKind::Fountain},
			{40, 4, 4, CandidateKind::Item},
		});
		assert(!state.contains(10));
		assert(state.contains(40));
		state.reset();
		assert(!state.active());
		assert(state.markers().empty());
		state.observeLive({{60, 6, 6, CandidateKind::Item}});
		assert(!state.contains(60));
	}

	void testGroundGoldLiveLifecycle()
	{
		using namespace quality::minimap::reveal;
		State state;
		state.refresh({{50, 8, 9, CandidateKind::Gold}});
		assert(state.contains(50));
		state.observeLive({
			{50, 10, 11, CandidateKind::Gold},
			{60, 12, 13, CandidateKind::Gold},
		});
		assert(state.contains(50));
		assert(state.contains(60));
		const auto markers = state.markers();
		assert(markers.size() == 2);
		assert(markers[0].uid == 50 && markers[0].x == 10
			&& markers[0].y == 11);
		state.observeLive({});
		assert(!state.contains(50));
		assert(!state.contains(60));
	}

	void testTooltipDebounce()
	{
		quality::minimap::reveal::State state;
		state.reset();
		assert(state.tooltipEdge(100));
		assert(!state.tooltipEdge(100));
		assert(!state.tooltipEdge(101));
		assert(!state.tooltipEdge(110));
		assert(state.tooltipEdge(121));
	}

	void testRevealSynchronizationGates()
	{
		using quality::minimap::reveal::acceptClientRefresh;
		using quality::minimap::reveal::acceptHostRequest;
		assert(acceptHostRequest(true, true, 4, 4));
		assert(!acceptHostRequest(false, true, 4, 4));
		assert(!acceptHostRequest(true, false, 4, 4));
		assert(!acceptHostRequest(true, true, 3, 4));
		assert(acceptClientRefresh(true, true, 8, 8));
		assert(!acceptClientRefresh(false, true, 8, 8));
		assert(!acceptClientRefresh(true, false, 8, 8));
		assert(!acceptClientRefresh(true, true, 7, 8));
	}

	void testPartyItemPickupDropIdentity()
	{
		using quality::minimap::items::State;
		State state;
		state.reset();
		constexpr std::uint32_t key = 0x12345678U;
		assert(state.observe(200, key, 8, 9, false) == 200);
		assert(state.markers().empty());
		assert(state.pickUp(200, key) == 200);
		assert(state.observe(201, key, 12, 13, true) == 200);
		auto markers = state.markers();
		assert(markers.size() == 1);
		assert(markers[0].markerId == 200 && markers[0].entityUid == 201);
		assert(markers[0].x == 12 && markers[0].y == 13);
		state.observe(201, key, 14, 15, false);
		markers = state.markers();
		assert(markers[0].x == 14 && markers[0].y == 15);
		state.remove(201);
		assert(state.markers().empty());
	}

	void testPartyItemStartingDropQueuesAndReset()
	{
		using quality::minimap::items::State;
		State state;
		constexpr std::uint32_t key = 99;
		assert(state.observe(500, key, 1, 1, true) == 500);
		assert(state.isPartyDropped(500));
		state.reset();
		assert(state.markers().empty());

		state.observe(10, key, 1, 1, false);
		state.observe(11, key, 2, 2, false);
		assert(state.pickUp(10, key) == 10);
		assert(state.pickUp(11, key) == 11);
		assert(state.observe(20, key, 3, 3, true) == 10);
		assert(state.observe(21, key, 4, 4, true) == 11);
	}

	void testPartyItemFingerprintAndRemoteDrop()
	{
		using quality::minimap::items::stackFingerprint;
		constexpr std::array<std::uint32_t, 5> fields = {1, 2, 3, 4, 5};
		static_assert(stackFingerprint(fields) != 0);
		static_assert(stackFingerprint(fields)
			== stackFingerprint({1, 2, 3, 4, 5}));
		quality::minimap::items::State state;
		state.applyDrop(0x80000001U, 700, 77, 20, 21);
		const auto markers = state.markers();
		assert(markers.size() == 1 && markers[0].markerId == 0x80000001U);
		assert(state.pickUp(700, 77) == 0x80000001U);
		assert(state.markers().empty());
	}

	void testFailedPickupKeepsGreenIdentity()
	{
		quality::minimap::items::State items;
		quality::minimap::reveal::State reveal;
		constexpr std::uint32_t key = 77;
		items.observe(100, key, 4, 5, false);
		reveal.refresh({{100, 4, 5,
			quality::minimap::reveal::CandidateKind::Item}});
		assert(items.isTrackedOrdinary(100));
		assert(items.rebindOrdinary(100, 101, key, 5, 5));
		assert(reveal.rebind(100, 101, 5, 5));
		assert(!items.isPartyDropped(101));
		assert(items.isTrackedOrdinary(101));
		assert(items.markers().empty());
		assert(!reveal.contains(100));
		assert(reveal.contains(101));
		const auto markers = reveal.markers();
		assert(markers.size() == 1 && markers[0].uid == 101);
		assert(markers[0].x == 5 && markers[0].y == 5);

		items.observe(200, key, 1, 1, true);
		assert(!items.rebindOrdinary(200, 201, key, 1, 1));
		assert(items.isPartyDropped(200));
	}

	void testPartyItemEligibilityAndAuthority()
	{
		using quality::minimap::items::eligibleGroundItem;
		using quality::minimap::items::locallyAuthoritative;
		static_assert(eligibleGroundItem(true, false, false));
		static_assert(!eligibleGroundItem(false, false, false));
		static_assert(!eligibleGroundItem(true, true, false));
		static_assert(!eligibleGroundItem(true, false, true));
		static_assert(locallyAuthoritative(0));
		static_assert(locallyAuthoritative(1));
		static_assert(!locallyAuthoritative(2));
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
	static_assert(quality::minimap::uninteractedGreen == 0xFF2F6B55U);
	static_assert(quality::minimap::interactedBlue == 0xFFE16941U);
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
	testRevealEligibility();
	testGroundGoldEligibility();
	testExitCreatureCountsAndText();
	testPersistentRevealLifecycle();
	testGroundGoldLiveLifecycle();
	testTooltipDebounce();
	testRevealSynchronizationGates();
	testPartyItemPickupDropIdentity();
	testPartyItemStartingDropQueuesAndReset();
	testPartyItemFingerprintAndRemoteDrop();
	testFailedPickupKeepsGreenIdentity();
	testPartyItemEligibilityAndAuthority();
	std::cout << "Quality runtime unit tests passed\n";
	return 0;
}
