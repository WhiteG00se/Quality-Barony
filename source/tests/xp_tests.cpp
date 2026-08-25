#include <array>
#include <cassert>
#include <climits>
#include <cstdint>
#include <iostream>
#include <vector>

#include "../quality/xp.hpp"
#include "../quality/minimap.hpp"
#include "../quality/minimap_items.hpp"
#include "../quality/minimap_network.hpp"
#include "../quality/minimap_reveal.hpp"
#include "../quality/minimap_chests.hpp"
#include "../quality/minimap_creatures.hpp"
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
		constexpr std::array<int, 4> follower = { 120, 108, 96, 84 };
		for ( int partySize = 1; partySize <= 4; ++partySize )
		{
			const auto index = static_cast<std::size_t>(partySize - 1);
			assert(quality::xp::playerPercent(partySize) == player[index]);
			assert(quality::xp::followerPercent(partySize) == follower[index]);
		}
		assert(quality::xp::playerPercent(0) == 100);
		assert(quality::xp::playerPercent(5) == 70);
		assert(quality::xp::followerPercent(0) == 120);
		assert(quality::xp::followerPercent(5) == 84);
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
		assert(quality::xp::followerGain(11, 3) == 10);
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
		assert(awards[0].key == 101 && awards[0].baseGain == 10);
		assert(awards[1].key == 202 && awards[1].baseGain == 15);
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
		assert(awards[0].key == 101 && awards[0].gain == 84);
		assert(awards[1].key == 202 && awards[1].baseGain == 67
			&& awards[1].gain == 83);
	}

	void testRevealEligibility()
	{
		using namespace quality::minimap::reveal;
		Candidate candidate {1, 2, 3, CandidateKind::Grave};
		assert(eligible(candidate, false));
		assert(!eligible(candidate, true));
		assert(markerKind(candidate) == Kind::ShinyYellow);
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
		candidate.itemType = 126;
		candidate.identified = false;
		assert(eligible(candidate, false));
		candidate.identified = true;
		assert(!eligible(candidate, false));
		assert(immediateBlue(candidate));
		candidate.lootBag = true;
		assert(!eligible(candidate, false));
		assert(!immediateBlue(candidate));
		candidate.kind = CandidateKind::Gold;
		candidate.lootBag = false;
		assert(eligible(candidate, false));
		candidate.available = false;
		assert(!eligible(candidate, false));
		candidate.kind = CandidateKind::None;
		candidate.available = true;
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

	void testGroundItemClassification()
	{
		using namespace quality::minimap::items;
		static_assert(appearance(126, false, false) == Appearance::Green);
		static_assert(appearance(126, true, false) == Appearance::Blue);
		static_assert(appearance(artifactOrbBlue, false, false)
			== Appearance::ShinyYellow);
		static_assert(appearance(artifactOrbGreen, true, false)
			== Appearance::ShinyYellow);
		static_assert(appearance(roughRockItemType, false, false)
			== Appearance::Hidden);
		static_assert(appearance(roughRockItemType, true, false)
			== Appearance::Hidden);
		static_assert(appearance(126, false, true) == Appearance::Hidden);
		static_assert(eligibleGroundItem(true, false, false));
		static_assert(!eligibleGroundItem(false, false, false));
		static_assert(!eligibleGroundItem(true, true, false));
		static_assert(!eligibleGroundItem(true, false, true));
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
		quality::minimap::formatCreatureCounts(text.data(), text.size(), 2, 5);
		assert(std::string(text.data()) == "2 hostiles / 5 neutrals alive");
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
		auto unidentifiedItem = [](const std::uint32_t uid, const int x,
			const int y)
		{
			Candidate candidate {uid, x, y, CandidateKind::Item};
			candidate.itemType = 126;
			candidate.identified = false;
			return candidate;
		};
		State state;
		state.reset();
		state.observeLive({unidentifiedItem(5, 1, 1)});
		assert(!state.contains(5));
		state.rememberInitialUses(10, 2);
		state.refresh({
			{10, 1, 1, CandidateKind::Fountain},
			unidentifiedItem(20, 2, 2),
			{30, 3, 3, CandidateKind::Exit},
		});
		assert(state.active());
		assert(state.contains(10));
		assert(state.contains(20));
		assert(state.contains(30));

		state.observeLive({
			{10, 4, 5, CandidateKind::Fountain},
			{30, 3, 3, CandidateKind::Exit},
			unidentifiedItem(40, 4, 4),
		});
		assert(state.contains(10));
		assert(!state.contains(20));
		assert(state.contains(40));
		const auto markers = state.markers();
		assert(markers.size() == 3);
		assert(markers[0].uid == 10 && markers[0].x == 4 && markers[0].y == 5);
		assert(markers[2].uid == 40
			&& markers[2].kind == Kind::Green);

		Candidate ineligible = unidentifiedItem(50, 5, 5);
		ineligible.itemType = quality::minimap::items::roughRockItemType;
		Candidate lootBag {51, 5, 6, CandidateKind::Item};
		lootBag.itemType = 126;
		lootBag.identified = false;
		lootBag.lootBag = true;
		state.observeLive({
			{10, 4, 5, CandidateKind::Fountain},
			{30, 3, 3, CandidateKind::Exit},
			unidentifiedItem(40, 4, 4),
			ineligible,
			lootBag,
		});
		assert(!state.contains(50));
		assert(!state.contains(51));
		ineligible.itemType = 126;
		state.observeLive({
			{10, 4, 5, CandidateKind::Fountain},
			{30, 3, 3, CandidateKind::Exit},
			unidentifiedItem(40, 4, 4),
			ineligible,
		});
		assert(state.contains(50));

		state.observeUses(10, 1);
		assert(!state.contains(10));
		state.observeLive({
			{10, 1, 1, CandidateKind::Fountain},
			unidentifiedItem(40, 4, 4),
		});
		assert(!state.contains(10));
		assert(state.contains(40));
		state.reset();
		assert(!state.active());
		assert(state.markers().empty());
		state.observeLive({unidentifiedItem(60, 6, 6)});
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

	void testRevealGatingIsPerPlayer()
	{
		using namespace quality::minimap::reveal;
		Candidate green {10, 2, 3, CandidateKind::Item};
		green.itemType = 126;
		green.identified = false;
		Candidate yellow {11, 4, 5, CandidateKind::Item};
		yellow.itemType = quality::minimap::items::artifactOrbBlue;
		State first;
		State second;
		first.refresh({green, yellow});
		second.observeLive({green, yellow});
		assert(first.contains(10) && first.contains(11));
		assert(!second.contains(10) && !second.contains(11));
		assert(!immediateBlue(green) && !immediateBlue(yellow));
		green.identified = true;
		assert(immediateBlue(green));
	}

	void testChestContentLifecycle()
	{
		using namespace quality::minimap::chests;
		static_assert(eligibleOrdinary(true, 0));
		static_assert(!eligibleOrdinary(true, 1));
		static_assert(!eligibleOrdinary(false, 0));
		static_assert(classify(0, 0) == Contents::Empty);
		static_assert(classify(2, 0) == Contents::HasUnidentified);
		static_assert(classify(0, 2) == Contents::IdentifiedOnly);
		static_assert(classify(1, 3) == Contents::HasUnidentified);
		static_assert(validContentsValue(0));
		static_assert(validContentsValue(2));
		static_assert(!validContentsValue(3));

		State authoritative;
		auto updates = authoritative.observeAuthoritative({{20, 4, 5, 0, 0}});
		assert(updates.size() == 1 && updates[0].contents == Contents::Empty);
		assert(authoritative.snapshots().empty());
		updates = authoritative.observeAuthoritative({{20, 4, 5, 2, 0}});
		assert(updates.size() == 1
			&& updates[0].contents == Contents::HasUnidentified);
		assert(authoritative.markers(Contents::HasUnidentified).size() == 1);
		updates = authoritative.observeAuthoritative({{20, 4, 5, 1, 3}});
		assert(updates.empty());
		updates = authoritative.observeAuthoritative({{20, 4, 5, 0, 3}});
		assert(updates.size() == 1
			&& updates[0].contents == Contents::IdentifiedOnly);
		updates = authoritative.observeAuthoritative({{20, 4, 5, 0, 0}});
		assert(updates.size() == 1 && updates[0].contents == Contents::Empty);
		updates = authoritative.observeAuthoritative({{20, 4, 5, 4, 0}});
		assert(updates.size() == 1
			&& updates[0].contents == Contents::HasUnidentified);
		updates = authoritative.observeAuthoritative({});
		assert(updates.size() == 1 && updates[0].contents == Contents::Empty);

		State remote;
		remote.apply({20, 4, 5, Contents::IdentifiedOnly});
		assert(remote.markers(Contents::IdentifiedOnly).size() == 1);
		remote.observeLive({{20, 7, 8, 0, 0}});
		const auto snapshot = remote.snapshots();
		assert(snapshot[0].x == 7 && snapshot[0].y == 8);
		remote.apply({20, 7, 8, Contents::Empty});
		assert(remote.snapshots().empty());
		remote.reset();
		assert(remote.snapshots().empty());
	}

	void testMinimapNetworkProtocolAndRouting()
	{
		using namespace quality::minimap::network;
		static_assert(compatible(Stream::State, protocolVersion));
		static_assert(compatible(Stream::Roster, rosterProtocolVersion));
		static_assert(compatible(Stream::Sightings, sightingProtocolVersion));
		static_assert(!compatible(Stream::State, protocolVersion - 1));
		static_assert(!compatible(Stream::Roster, rosterProtocolVersion - 1));
		static_assert(validSender(1, 1));
		static_assert(validSender(1, 3));
		static_assert(!validSender(1, 0));
		static_assert(validSender(2, 0));
		static_assert(!validSender(2, 1));
		static_assert(hostNumberForTarget(1, 1) == 0);
		static_assert(hostNumberForTarget(1, 3) == 2);
		static_assert(hostNumberForTarget(2, 0) == 0);
		static_assert(hostNumberForTarget(2, 1) == -1);
		static_assert(deliveryKey(1, 7) != deliveryKey(2, 7));
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

	void testCreatureVisionAndFinalReveal()
	{
		using namespace quality::minimap::creatures;
		constexpr double pi = 3.14159265358979323846;
		assert(inForwardHalfPlane(0, 0, 0, 10, 0));
		assert(inForwardHalfPlane(0, 0, 0, 0, 10));
		assert(inForwardHalfPlane(0, 0, 0, 0, -10));
		assert(!inForwardHalfPlane(0, 0, 0, -0.001, 10));
		assert(!inForwardHalfPlane(0, 0, pi, 10, 0));
		assert(acceptSightingSnapshot(true, true, true, true, 3, 2));
		assert(acceptSightingSnapshot(true, true, true, true, 2, 2));
		assert(!acceptSightingSnapshot(true, true, true, true, 1, 2));
		assert(!acceptSightingSnapshot(true, true, false, true, 3, 2));
		assert(!acceptSightingSnapshot(false, true, true, true, 3, 2));
		assert(!acceptSightingSnapshot(true, false, true, true, 3, 2));
		assert(!acceptSightingSnapshot(true, true, true, false, 3, 2));
		assert(ordinarySightingVisible(Disposition::Hostile,
			false, false, true, true));
		assert(!ordinarySightingVisible(Disposition::Hostile,
			true, false, true, true));
		assert(ordinarySightingVisible(Disposition::Hostile,
			true, true, true, true));
		assert(!ordinarySightingVisible(Disposition::Neutral,
			false, false, true, true));
		assert(!ordinarySightingVisible(Disposition::Hostile,
			false, false, false, true));
		assert(!ordinarySightingVisible(Disposition::Hostile,
			false, false, true, false));

		const std::array<std::array<bool, 5>, 5> walls {{
			{{false, false, false, false, false}},
			{{false, false, false, false, false}},
			{{false, false, true,  false, false}},
			{{false, false, false, false, false}},
			{{false, false, false, false, false}},
		}};
		auto blocked = [&](const int x, const int y)
		{
			return x < 0 || y < 0 || x >= 5 || y >= 5
				|| walls[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
		};
		assert(clearTileLine(0.5, 0.5, 4.5, 0.5, blocked));
		assert(!clearTileLine(0.5, 2.5, 4.5, 2.5, blocked));
		assert(!clearTileLine(0.5, 0.5, 4.5, 4.5, blocked));
		assert(segmentIntersectsBeforeTarget(0, 0, 10, 0,
			{4, -1, 6, 1}));
		assert(!segmentIntersectsBeforeTarget(0, 0, 10, 0,
			{11, -1, 12, 1}));
		const std::array<Blocker, 2> blockers {{
			{11, -1, 12, 1}, {7, -1, 8, 1},
		}};
		assert(std::any_of(blockers.begin(), blockers.end(),
			[](const Blocker& blocker)
			{
				return segmentIntersectsBeforeTarget(0, 0, 10, 0, blocker);
			}));

		FinalRevealState reveal;
		assert(!reveal.update(2, 3));
		reveal.activate();
		assert(!reveal.update(3, 4));
		assert(reveal.update(2, 4));
		assert(reveal.latched());
		assert(!reveal.update(5, 9));
		int hostiles = 0;
		int neutrals = 0;
		assert(reveal.takeNotification(hostiles, neutrals));
		assert(hostiles == 2 && neutrals == 4);
		assert(!reveal.takeNotification(hostiles, neutrals));
		FinalRevealState oneHostile;
		oneHostile.activate();
		assert(oneHostile.update(1, 0));
		FinalRevealState zeroHostiles;
		zeroHostiles.activate();
		assert(zeroHostiles.update(0, 2));
		reveal.reset();
		assert(!reveal.activated() && !reveal.latched());

		SightingUnion sightings;
		assert(sightings.replace(0, 1, {10, 20}));
		assert(sightings.replace(1, 1, {20, 30}));
		assert(!sightings.replace(1, 1, {40}));
		auto combined = sightings.combined();
		assert(combined.size() == 3 && combined.count(10)
			&& combined.count(20) && combined.count(30));
		assert(sightings.replace(0, 2, {}));
		combined = sightings.combined();
		assert(combined.size() == 2 && !combined.count(10));
		sightings.clearPlayer(1);
		assert(sightings.combined().empty());
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
	static_assert(quality::minimap::detectedPurple == 0xFFBF7FBFU);
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
	static_assert(quality::minimap::shinyYellow
		== quality::minimap::color(0xFF, 0xD7, 0x00));
	testReadmePercentages();
	testRoundingAndMinimum();
	testInspiration();
	testOwnerSpecificRouting();
	testEligibilityDeduplicationAndLastHit();
	testRevealEligibility();
	testGroundGoldEligibility();
	testGroundItemClassification();
	testExitCreatureCountsAndText();
	testPersistentRevealLifecycle();
	testGroundGoldLiveLifecycle();
	testTooltipDebounce();
	testRevealSynchronizationGates();
	testRevealGatingIsPerPlayer();
	testChestContentLifecycle();
	testMinimapNetworkProtocolAndRouting();
	testFriendlyFirePolicy();
	testDamageParticipationPolicy();
	testCreatureVisionAndFinalReveal();
	std::cout << "Quality runtime unit tests passed\n";
	return 0;
}
