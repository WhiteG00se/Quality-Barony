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
#include "../quality/minimap_chests.hpp"
#include "../quality/friendly_fire.hpp"
#include "../quality/follower_roster.hpp"
#include "../quality/xp_participation.hpp"

namespace
{
	using quality::xp::Follower;
	using quality::xp::OwnerBase;

	void testReadmePercentages()
	{
		constexpr std::array<int, 4> player = { 100, 90, 80, 70 };
		constexpr std::array<int, 4> follower = { 130, 117, 104, 91 };
		for ( int partySize = 1; partySize <= 4; ++partySize )
		{
			const auto index = static_cast<std::size_t>(partySize - 1);
			assert(quality::xp::playerPercent(partySize) == player[index]);
			assert(quality::xp::followerPercent(partySize) == follower[index]);
		}
		assert(quality::xp::playerPercent(0) == 100);
		assert(quality::xp::playerPercent(5) == 70);
		assert(quality::xp::followerPercent(0) == 130);
		assert(quality::xp::followerPercent(5) == 91);
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

		assert(quality::xp::followerGain(11, 1) == 14);
		assert(quality::xp::followerGain(11, 2) == 12);
		assert(quality::xp::followerGain(11, 3) == 11);
		assert(quality::xp::followerGain(11, 4) == 10);
		assert(quality::xp::followerGain(1, 4) == 0);
		assert(quality::xp::scale(INT_MAX, 150) == INT_MAX);
	}

	void testInspiration()
	{
		assert(quality::xp::withInspiration(16, 25) == 20);
		assert(quality::xp::withInspiration(15, 10) == 16);
		assert(quality::xp::withInspiration(13, 0) == 13);
		assert(quality::xp::withInspiration(
			quality::xp::followerGain(11, 4), 25) == 12);
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
		assert(awards[0].key == 101 && awards[0].baseGain == 11);
		assert(awards[1].key == 202 && awards[1].baseGain == 16);
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
		assert(awards[0].key == 101 && awards[0].gain == 91);
		assert(awards[1].key == 202 && awards[1].baseGain == 72
			&& awards[1].gain == 90);
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
		candidate.identified = false;
		assert(eligible(candidate, false));
		candidate.identified = true;
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

	void testRevealedGroundItemEligibility()
	{
		using namespace quality::minimap::reveal;
		static_assert(roughRockItemType == 125);
		static_assert(!eligibleRevealedGroundItem(roughRockItemType, true));
		static_assert(eligibleRevealedGroundItem(roughRockItemType, false));
		static_assert(eligibleRevealedGroundItem(roughRockItemType + 1, true));
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
			== "Exit dungeon floor\n0 hostiles / 0 neutrals alive");
		quality::minimap::formatExitTooltip(text.data(), text.size(),
			"Exit secret level", 1, 1);
		assert(std::string(text.data())
			== "Exit secret level\n1 hostiles / 1 neutrals alive");
		quality::minimap::formatExitTooltip(text.data(), text.size(),
			"Step through portal", 12, 3);
		assert(std::string(text.data())
			== "Step through portal\n12 hostiles / 3 neutrals alive");
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

	void testChestInteractionLifecycle()
	{
		using namespace quality::minimap::chests;
		static_assert(eligibleOrdinary(true, 0));
		static_assert(!eligibleOrdinary(true, 1));
		static_assert(!eligibleOrdinary(false, 0));
		State state;
		assert(state.observeAuthoritative({{10, 2, 3, 4, false}}).empty());
		assert(state.markers().empty());
		assert(state.observeAuthoritative({{10, 2, 3, 4, true}}).empty());
		assert(!state.interacted(10));
		assert(state.observeAuthoritative({{10, 2, 3, 3, true}}).size() == 1);
		assert(state.interacted(10));
		auto markers = state.markers();
		assert(markers.size() == 1 && markers[0].uid == 10);

		assert(state.observeAuthoritative({{10, 2, 3, 0, true}}).size() == 1);
		assert(state.markers().empty());
		assert(state.interacted(10));
		assert(state.observeAuthoritative({{10, 2, 3, 2, true}}).size() == 1);
		assert(state.markers().size() == 1);
		state.observeAuthoritative({});
		assert(state.markers().empty());
		assert(!state.interacted(10));
	}

	void testChestClosedChangesAndRemoteState()
	{
		using namespace quality::minimap::chests;
		State authoritative;
		authoritative.observeAuthoritative({{20, 4, 5, 3, false}});
		assert(authoritative.observeAuthoritative({{20, 4, 5, 2, false}}).empty());
		assert(!authoritative.interacted(20));
		authoritative.observeAuthoritative({{20, 4, 5, 2, true}});
		const auto updates = authoritative.observeAuthoritative({{20, 4, 5, 1, false}});
		assert(updates.size() == 1 && updates[0].interacted
			&& updates[0].nonempty);

		State remote;
		remote.apply(updates[0]);
		assert(remote.interacted(20));
		assert(remote.markers().size() == 1);
		remote.apply({20, 4, 5, true, false});
		assert(remote.markers().empty());
		assert(remote.updates().size() == 1);
		remote.observeLive({{20, 7, 8, 0, false}});
		const auto snapshot = remote.updates();
		assert(snapshot[0].x == 7 && snapshot[0].y == 8);
		remote.reset();
		assert(remote.updates().empty());
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

	void testFriendlyFirePolicy()
	{
		using quality::friendly_fire::ActorKind;
		using quality::friendly_fire::Decision;
		using quality::friendly_fire::friendshipDecision;
		using quality::friendly_fire::hostilityDecision;
		using quality::friendly_fire::impaired;
		using quality::friendly_fire::protectionDecision;

		constexpr auto independent = ActorKind::IndependentMonster;
		constexpr auto sober = impaired(false, false);
		constexpr auto confused = impaired(true, false);
		constexpr auto drunk = impaired(false, true);
		static_assert(!sober);
		static_assert(confused);
		static_assert(drunk);
		static_assert(hostilityDecision(false, independent, independent, false)
			== Decision::ForceFalse);
		static_assert(friendshipDecision(false, independent, independent, false)
			== Decision::ForceTrue);
		static_assert(protectionDecision(false, independent, independent, false)
			== Decision::ForceTrue);
		static_assert(hostilityDecision(true, independent, independent, false)
			== Decision::Preserve);
		static_assert(friendshipDecision(true, independent, independent, false)
			== Decision::Preserve);
		static_assert(protectionDecision(true, independent, independent, false)
			== Decision::Preserve);
		static_assert(hostilityDecision(false, independent, independent,
			confused)
			== Decision::Preserve);
		static_assert(friendshipDecision(false, independent, independent,
			confused)
			== Decision::Preserve);
		static_assert(protectionDecision(false, independent, independent, confused)
			== Decision::ForceFalse);
		static_assert(hostilityDecision(false, independent, independent,
			drunk)
			== Decision::Preserve);
		static_assert(friendshipDecision(false, independent, independent,
			drunk)
			== Decision::Preserve);
		static_assert(confused && hostilityDecision(false, independent,
			independent, sober) == Decision::ForceFalse);
		static_assert(confused && friendshipDecision(false, independent,
			independent, sober) == Decision::ForceTrue);
		static_assert(drunk && hostilityDecision(false, independent,
			independent, sober) == Decision::ForceFalse);
		static_assert(drunk && friendshipDecision(false, independent,
			independent, sober) == Decision::ForceTrue);
		static_assert(protectionDecision(false, independent, independent, drunk)
			== Decision::ForceFalse);
		static_assert(protectionDecision(false, ActorKind::Player,
			ActorKind::Player, confused) == Decision::ForceFalse);
		static_assert(protectionDecision(false, ActorKind::Player,
			ActorKind::PlayerAlly, drunk) == Decision::ForceFalse);
		static_assert(protectionDecision(false, ActorKind::PlayerAlly,
			ActorKind::Player, sober) == Decision::ForceTrue);
		static_assert(protectionDecision(false, ActorKind::PlayerAlly,
			ActorKind::PlayerAlly, sober) == Decision::ForceTrue);
		static_assert(protectionDecision(false, ActorKind::PlayerAlly,
			ActorKind::Player, confused) == Decision::ForceFalse);
		static_assert(protectionDecision(false, ActorKind::PlayerAlly,
			ActorKind::PlayerAlly, drunk) == Decision::ForceFalse);
		static_assert(protectionDecision(true, ActorKind::PlayerAlly,
			ActorKind::Player, sober) == Decision::Preserve);
		static_assert(protectionDecision(false, ActorKind::Player,
			ActorKind::PlayerAlly, sober) == Decision::Preserve);
		static_assert(protectionDecision(false, ActorKind::PlayerAlly,
			independent, drunk) == Decision::Preserve);
		static_assert(protectionDecision(false, independent,
			ActorKind::PlayerAlly, sober) == Decision::Preserve);
		static_assert(hostilityDecision(false, independent,
			ActorKind::Other, false) == Decision::Preserve);
	}

	void testDamageParticipationPolicy()
	{
		using quality::friendly_fire::ActorKind;
		using quality::xp::participation::State;
		using quality::xp::participation::actualHpLoss;
		using quality::xp::participation::qualifyingAttacker;
		using quality::xp::participation::qualifyingVictim;

		static_assert(actualHpLoss(10, 9));
		static_assert(!actualHpLoss(10, 10));
		static_assert(!actualHpLoss(10, 11));
		static_assert(qualifyingAttacker(ActorKind::Player, false));
		static_assert(qualifyingAttacker(ActorKind::PlayerAlly, false));
		static_assert(!qualifyingAttacker(ActorKind::IndependentMonster, false));
		static_assert(qualifyingAttacker(ActorKind::IndependentMonster, true));
		static_assert(qualifyingAttacker(ActorKind::Other, true));
		static_assert(qualifyingVictim(ActorKind::IndependentMonster, true));
		static_assert(!qualifyingVictim(ActorKind::Player, true));
		static_assert(!qualifyingVictim(ActorKind::PlayerAlly, true));
		static_assert(!qualifyingVictim(ActorKind::IndependentMonster, false));

		State playerDamage;
		playerDamage.markDamageParticipation(ActorKind::Player, false,
			ActorKind::IndependentMonster, true);
		assert(playerDamage.qualified());
		assert(playerDamage.claimPartyXp(true,
			ActorKind::IndependentMonster, true));
		assert(!playerDamage.claimPartyXp(true,
			ActorKind::IndependentMonster, true));

		State ordinaryMonster;
		ordinaryMonster.markDamageParticipation(ActorKind::IndependentMonster,
			false, ActorKind::IndependentMonster, true);
		assert(!ordinaryMonster.qualified());
		ordinaryMonster.markDamageParticipation(ActorKind::IndependentMonster,
			true, ActorKind::IndependentMonster, true);
		assert(ordinaryMonster.qualified());
		assert(ordinaryMonster.claimPartyXp(true,
			ActorKind::IndependentMonster, true));

		State healedAfterDamage;
		healedAfterDamage.markDamageParticipation(ActorKind::PlayerAlly, false,
			ActorKind::IndependentMonster, true);
		assert(!actualHpLoss(5, 20));
		assert(healedAfterDamage.qualified());
		assert(healedAfterDamage.claimPartyXp(true,
			ActorKind::IndependentMonster, true));

		State normalPartyKill;
		normalPartyKill.markDamageParticipation(ActorKind::Player, false,
			ActorKind::IndependentMonster, true);
		normalPartyKill.notePartyAward();
		assert(normalPartyKill.partyAwarded());
		assert(!normalPartyKill.claimPartyXp(true,
			ActorKind::IndependentMonster, true));

		State clientState;
		clientState.markDamageParticipation(ActorKind::Other, true,
			ActorKind::IndependentMonster, true);
		assert(!clientState.claimPartyXp(false,
			ActorKind::IndependentMonster, true));

		State excludedVictim;
		excludedVictim.markDamageParticipation(ActorKind::Player, false,
			ActorKind::PlayerAlly, true);
		assert(!excludedVictim.qualified());
		assert(!excludedVictim.claimPartyXp(true, ActorKind::PlayerAlly, true));
	}
}

int main()
{
	{
		using namespace quality::follower_roster;
		State roster;
		assert(roster.upsert({ 30, 2, 4, 10, 20, 9, 123, 0, "Bones" }));
		assert(roster.upsert({ 20, 1, 3, 8, 12, 1, 456, 0, "Alice" }));
		assert(roster.upsert({ 15, 1, 2, 7, 11, 2, 654, 1, "Second" }));
		assert(roster.upsert({ 10, 0, 2, 6, 10, 2, 789, 0, "Mine" }));
		assert(!roster.upsert({ 20, 1, 3, 8, 12, 1, 456, 0, "Alice" }));

		const auto remote = visibleRemoteEntries(roster.entries(), 0);
		assert(remote.size() == 3);
		assert(remote[0].uid == 20 && remote[1].uid == 15
			&& remote[2].uid == 30);
		assert(displayName("Thomas", remote[0].name, remote[0].owner)
			== "T's Alice");
		assert(displayName("", "Bones", 2) == "P3's Bones");
		assert(firstUtf8Character("\xC3\x89mile") == "\xC3\x89");

		assert(roster.upsert({ 20, 1, 4, 5, 12, 1, 456, 0, "Alice" }));
		assert(roster.entries().at(20).level == 4);
		assert(roster.entries().at(20).hp == 5);
		assert(roster.upsert({ 20, 1, 4, 0, 12, 1, 456, 0, "Alice" }));
		assert(roster.entries().find(20) == roster.entries().end());
		assert(!eligible({ 0, 1, 1, 1, 1, 1, 1, 0, "Invalid" }));
		roster.reset();
		assert(roster.entries().empty());
	}

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
	static_assert(quality::minimap::isDetectedUnit(0x06000001U));
	static_assert(!quality::minimap::isDetectedUnit(0x06000000U));
	static_assert(!quality::minimap::isDetectedUnit(0x05000001U));
	static_assert(quality::minimap::classifyEntity(0, false, 0, true, false, false)
		== quality::minimap::MarkerAppearance::Exit);
	static_assert(quality::minimap::classifyEntity(990, false, 0, false, false, false)
		== quality::minimap::MarkerAppearance::Boulder);
	static_assert(quality::minimap::classifyEntity(0, false, 0, false, true, false)
		== quality::minimap::MarkerAppearance::Workbench);
	static_assert(quality::minimap::classifyEntity(0, false, 0, false, false, true)
		== quality::minimap::MarkerAppearance::Cauldron);
	static_assert(quality::minimap::classifyEntity(0, false, 0x06000001U,
		false, false, false) == quality::minimap::MarkerAppearance::DetectedUnit);
	testReadmePercentages();
	testRoundingAndMinimum();
	testInspiration();
	testOwnerSpecificRouting();
	testEligibilityDeduplicationAndLastHit();
	testRevealEligibility();
	testGroundGoldEligibility();
	testRevealedGroundItemEligibility();
	testExitCreatureCountsAndText();
	testPersistentRevealLifecycle();
	testGroundGoldLiveLifecycle();
	testTooltipDebounce();
	testRevealSynchronizationGates();
	testPartyItemPickupDropIdentity();
	testPartyItemStartingDropQueuesAndReset();
	testPartyItemFingerprintAndRemoteDrop();
	testFailedPickupKeepsGreenIdentity();
	testChestInteractionLifecycle();
	testChestClosedChangesAndRemoteState();
	testPartyItemEligibilityAndAuthority();
	testFriendlyFirePolicy();
	testDamageParticipationPolicy();
	std::cout << "Quality runtime unit tests passed\n";
	return 0;
}
