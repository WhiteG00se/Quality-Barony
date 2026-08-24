#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "runtime_layout.hpp"
#include "runtime_patch.hpp"
#include "friendly_fire.hpp"
#include "minimap_runtime.hpp"
#include "xp.hpp"
#include "xp_participation.hpp"

namespace
{
	namespace layout = quality::runtime::layout;

	constexpr std::size_t relayStride = 256;
	constexpr std::size_t trampolineStride = 64;

	struct Node
	{
		Node* next;
		Node* previous;
		void* list;
		void* element;
	};

	struct MsvcString
	{
		union
		{
			char inlineData[16];
			char* heapData;
		} storage {};
		std::uint64_t size = 0;
		std::uint64_t capacity = 15;
	};
	static_assert(sizeof(MsvcString) == 32);

	struct FollowerSnapshot
	{
		std::uint8_t* entity = nullptr;
		std::uint8_t* stats = nullptr;
		int owner = -1;
	};

	using Patch = quality::runtime::Patch;

	using AwardXpFn = void (*)(std::uint8_t*, std::uint8_t*, bool, bool);
	using SetHpFn = void (*)(std::uint8_t*, int);
	using EntityPairVoidBoolMethodFn = void (*)(std::uint8_t*, std::uint8_t*, bool);
	using UpdateEnemyBarFn = void (*)(std::uint8_t*, std::uint8_t*, const char*,
		int, int, bool, int);
	using EntityMethodFn = void* (*)(std::uint8_t*);
	using EntityBoolMethodFn = bool (*)(std::uint8_t*);
	using EntityPairBoolMethodFn = bool (*)(std::uint8_t*, std::uint8_t*);
	using EntityIntMethodFn = int (*)(std::uint8_t*);
	using UidToEntityFn = std::uint8_t* (*)(std::uint32_t);
	using StatGetAttributeFn = MsvcString* (*)(void*, MsvcString*, const MsvcString*);
	using MsvcStringDestroyFn = void (*)(MsvcString*);
	using SteamAchievementEntityFn = void (*)(std::uint8_t*, const char*);
	using CompendiumEventUpdateFn = void (*)(int, int, int, int, bool, int);

	std::uint8_t* base = nullptr;
	AwardXpFn originalAwardXp = nullptr;
	SetHpFn originalSetHp = nullptr;
	EntityPairVoidBoolMethodFn originalKilledByMonsterObituary = nullptr;
	EntityPairVoidBoolMethodFn originalUpdateEntityOnHit = nullptr;
	UpdateEnemyBarFn originalUpdateEnemyBar = nullptr;
	EntityMethodFn getStats = nullptr;
	EntityBoolMethodFn monsterIsTinkeringCreation = nullptr;
	EntityMethodFn monsterAllyGetPlayerLeader = nullptr;
	EntityPairBoolMethodFn originalCheckEnemy = nullptr;
	EntityPairBoolMethodFn originalCheckFriend = nullptr;
	EntityPairBoolMethodFn originalFriendlyFireProtection = nullptr;
	EntityIntMethodFn entityInspiration = nullptr;
	UidToEntityFn uidToEntity = nullptr;
	StatGetAttributeFn statGetAttribute = nullptr;
	MsvcStringDestroyFn msvcStringDestroy = nullptr;
	SteamAchievementEntityFn steamAchievementEntity = nullptr;
	CompendiumEventUpdateFn compendiumEventUpdate = nullptr;
	void* relayPage = nullptr;
	void* xpTrampoline = nullptr;
	void* friendlyFireTrampolines = nullptr;
	void* participationTrampolines = nullptr;

	bool xpContextActive = false;
	std::uint8_t* rootFollowerEntity = nullptr;
	int partySize = 1;
	std::array<quality::xp::OwnerBase, 4> ownerBases {};
	std::vector<FollowerSnapshot> followers;

	struct ParticipationRecord
	{
		std::uint8_t* entity = nullptr;
		std::uint8_t* stats = nullptr;
		std::uint32_t uid = 0;
		std::uint32_t pendingTick = 0;
		bool pendingDamage = false;
		quality::xp::participation::State state;
	};

	std::unordered_map<std::uint32_t, ParticipationRecord> participation;
	std::uint32_t lastParticipationSweep = 0;
	bool syntheticPartyAward = false;
	LONG initializationState = 0;

	bool serverAuthoritative();
	bool vanillaXpVictimEligible(std::uint8_t*, std::uint8_t*);
	ParticipationRecord* findParticipation(std::uint8_t*);
	void correlateDamage(std::uint8_t*, std::uint8_t*);
	void grantParticipationXp(std::uint8_t*);

	void debug(const char* message)
	{
		OutputDebugStringA("[QualityBarony] ");
		OutputDebugStringA(message);
		OutputDebugStringA("\n");
	}

	template <typename T>
	T& field(std::uint8_t* object, const std::size_t offset)
	{
		return *reinterpret_cast<T*>(object + offset);
	}

	template <typename T>
	const T& field(const std::uint8_t* object, const std::size_t offset)
	{
		return *reinterpret_cast<const T*>(object + offset);
	}

	std::int32_t& skill(std::uint8_t* entity, const std::size_t index)
	{
		return field<std::int32_t>(entity,
			layout::entitySkill + index * sizeof(std::int32_t));
	}

	std::uintptr_t behavior(const std::uint8_t* entity)
	{
		return field<std::uintptr_t>(entity, layout::entityBehavior);
	}

	bool friendlyFireEnabled()
	{
		return field<std::uint32_t>(base, layout::serverFlagsRva)
			& layout::serverFlagFriendlyFire;
	}

	quality::friendly_fire::ActorKind actorKind(std::uint8_t* entity)
	{
		using quality::friendly_fire::ActorKind;
		if ( !entity )
		{
			return ActorKind::Other;
		}
		const auto entityBehavior = behavior(entity);
		if ( entityBehavior == reinterpret_cast<std::uintptr_t>(
			base + layout::actPlayerRva) )
		{
			return ActorKind::Player;
		}
		if ( entityBehavior != reinterpret_cast<std::uintptr_t>(
			base + layout::actMonsterRva) )
		{
			return ActorKind::Other;
		}
		if ( !getStats(entity) )
		{
			return ActorKind::Other;
		}
		return monsterAllyGetPlayerLeader(entity)
			? ActorKind::PlayerAlly
			: ActorKind::IndependentMonster;
	}

	bool entityImpaired(std::uint8_t* entity)
	{
		if ( !entity )
		{
			return false;
		}
		auto* stats = static_cast<std::uint8_t*>(getStats(entity));
		return stats && quality::friendly_fire::impaired(
			field<std::uint8_t>(stats,
				layout::statEffects + layout::effectConfused) != 0,
			field<std::uint8_t>(stats,
				layout::statEffects + layout::effectDrunk) != 0);
	}

	bool decisionValue(const quality::friendly_fire::Decision decision,
		const bool original)
	{
		switch ( decision )
		{
			case quality::friendly_fire::Decision::ForceFalse:
				return false;
			case quality::friendly_fire::Decision::ForceTrue:
				return true;
			case quality::friendly_fire::Decision::Preserve:
			default:
				return original;
		}
	}

	bool checkEnemyHook(std::uint8_t* entity, std::uint8_t* target)
	{
		if ( friendlyFireEnabled() )
		{
			return originalCheckEnemy(entity, target);
		}
		const auto decision = quality::friendly_fire::hostilityDecision(
			false, actorKind(entity), actorKind(target),
			entityImpaired(entity));
		if ( decision != quality::friendly_fire::Decision::Preserve )
		{
			return decisionValue(decision, false);
		}
		return originalCheckEnemy(entity, target);
	}

	bool checkFriendHook(std::uint8_t* entity, std::uint8_t* target)
	{
		if ( friendlyFireEnabled() )
		{
			return originalCheckFriend(entity, target);
		}
		const auto decision = quality::friendly_fire::friendshipDecision(
			false, actorKind(entity), actorKind(target),
			entityImpaired(entity));
		if ( decision != quality::friendly_fire::Decision::Preserve )
		{
			return decisionValue(decision, false);
		}
		return originalCheckFriend(entity, target);
	}

	bool friendlyFireProtectionHook(std::uint8_t* entity,
		std::uint8_t* target)
	{
		if ( friendlyFireEnabled() )
		{
			return originalFriendlyFireProtection(entity, target);
		}
		const auto decision = quality::friendly_fire::protectionDecision(
			false, actorKind(entity), actorKind(target),
			entityImpaired(entity));
		if ( decision != quality::friendly_fire::Decision::Preserve )
		{
			return decisionValue(decision, false);
		}
		return originalFriendlyFireProtection(entity, target);
	}

	bool attributeNotEmpty(std::uint8_t* stats, const char* name)
	{
		if ( !stats || !statGetAttribute || !msvcStringDestroy )
		{
			return false;
		}
		MsvcString key {};
		const std::size_t length = std::strlen(name);
		if ( length >= sizeof(key.storage.inlineData) )
		{
			return false;
		}
		std::memcpy(key.storage.inlineData, name, length);
		key.storage.inlineData[length] = '\0';
		key.size = length;

		MsvcString result {};
		statGetAttribute(stats, &result, &key);
		const bool found = result.size != 0;
		msvcStringDestroy(&result);
		return found;
	}

	bool followerEligible(std::uint8_t* entity, std::uint8_t* stats)
	{
		if ( !entity || !stats
			|| field<std::int32_t>(stats, layout::statHp) <= 0
			|| monsterIsTinkeringCreation(entity) )
		{
			return false;
		}
		const int type = field<std::int32_t>(stats, layout::statType);
		switch ( type )
		{
			case layout::revenantSkull:
			case layout::adorcisedWeapon:
			case layout::flameElemental:
			case layout::hologram:
			case layout::duckSmall:
				return false;
			case layout::skeleton:
				return !attributeNotEmpty(stats, "revenant_skeleton");
			case layout::mothSmall:
				return !attributeNotEmpty(stats, "fire_sprite");
			case layout::earthElemental:
				return !monsterAllyGetPlayerLeader(entity);
			default:
				return true;
		}
	}

	std::uint8_t* playerEntity(const int player)
	{
		if ( player < 0 || player >= 4 )
		{
			return nullptr;
		}
		auto** players = reinterpret_cast<std::uint8_t**>(base + layout::playersRva);
		return players[player]
			? field<std::uint8_t*>(players[player], layout::playerEntity)
			: nullptr;
	}

	int connectedPartySize()
	{
		const auto* disconnected = base + layout::clientDisconnectedRva;
		int connected = 0;
		for ( int player = 0; player < 4; ++player )
		{
			if ( !disconnected[player] )
			{
				++connected;
			}
		}
		return quality::xp::clampPartySize(connected);
	}

	void snapshotFollowers(std::uint8_t* rootRecipient)
	{
		followers.clear();
		rootFollowerEntity = nullptr;
		ownerBases = {};
		partySize = connectedPartySize();

		auto** allStats = reinterpret_cast<std::uint8_t**>(base + layout::statsRva);
		const auto* disconnected = base + layout::clientDisconnectedRva;
		std::unordered_set<std::uintptr_t> recorded;
		for ( int owner = 0; owner < 4; ++owner )
		{
			auto* ownerStats = allStats[owner];
			if ( disconnected[owner] || !ownerStats )
			{
				continue;
			}
			auto* node = field<Node*>(ownerStats, layout::statFollowers);
			while ( node )
			{
				const auto* uidPointer = static_cast<const std::uint32_t*>(node->element);
				auto* follower = uidPointer ? uidToEntity(*uidPointer) : nullptr;
				const auto key = reinterpret_cast<std::uintptr_t>(follower);
				if ( follower && recorded.insert(key).second )
				{
					auto* stats = static_cast<std::uint8_t*>(getStats(follower));
					if ( followerEligible(follower, stats) )
					{
						followers.push_back({ follower, stats, owner });
						if ( follower == rootRecipient )
						{
							rootFollowerEntity = follower;
						}
					}
				}
				node = node->next;
			}
		}
	}

	void captureOwnerBase(std::uint8_t* recipient, const int unsharedGain)
	{
		if ( !xpContextActive || !recipient
			|| behavior(recipient) != reinterpret_cast<std::uintptr_t>(
				base + layout::actPlayerRva) )
		{
			return;
		}
		const int owner = skill(recipient, layout::skillPlayerIndex);
		if ( owner >= 0 && owner < 4 )
		{
			ownerBases[static_cast<std::size_t>(owner)] = { true, unsharedGain };
		}
	}

	void applyFollowerXp()
	{
		std::vector<quality::xp::Follower> routingInput;
		routingInput.reserve(followers.size());
		for ( const auto& follower : followers )
		{
			auto* currentStats = static_cast<std::uint8_t*>(getStats(follower.entity));
			const bool eligible = currentStats == follower.stats
				&& followerEligible(follower.entity, currentStats);
			routingInput.push_back({ reinterpret_cast<std::uintptr_t>(follower.entity),
				follower.owner, eligible,
				eligible ? entityInspiration(follower.entity) : 0 });
		}

		const auto awards = quality::xp::routeFollowers(partySize, ownerBases,
			routingInput);
		for ( const auto& award : awards )
		{
			const auto found = std::find_if(followers.begin(), followers.end(),
				[&](const FollowerSnapshot& follower) {
					return reinterpret_cast<std::uintptr_t>(follower.entity) == award.key;
				});
			if ( found == followers.end() )
			{
				continue;
			}
			auto* stats = static_cast<std::uint8_t*>(getStats(found->entity));
			if ( stats != found->stats || !followerEligible(found->entity, stats) )
			{
				continue;
			}
			auto& experience = field<std::int32_t>(stats, layout::statExperience);
			if ( award.gain > award.baseGain )
			{
				if ( experience + award.gain >= 100
					&& experience + award.baseGain < 100 )
				{
					if ( auto* owner = playerEntity(award.owner) )
					{
						steamAchievementEntity(owner, reinterpret_cast<const char*>(
							base + layout::byExampleAchievementRva));
					}
				}
				constexpr int inspirationXpEvent = 29;
				constexpr int crownItemType = 0x134;
				compendiumEventUpdate(award.owner, inspirationXpEvent,
					crownItemType, award.gain - award.baseGain, false, -1);
			}
			experience += award.gain;
		}
	}

	void grantParticipationXp(std::uint8_t* victim)
	{
		auto* record = findParticipation(victim);
		if ( !record )
		{
			return;
		}
		const auto uid = record->uid;
		const bool shouldGrant = record->state.claimPartyXp(
			serverAuthoritative(), actorKind(victim),
			vanillaXpVictimEligible(victim, record->stats));
		if ( !shouldGrant )
		{
			participation.erase(uid);
			return;
		}

		std::uint8_t* recipient = nullptr;
		const auto* disconnected = base + layout::clientDisconnectedRva;
		for ( int player = 0; player < 4; ++player )
		{
			if ( !disconnected[player] )
			{
				auto* candidate = playerEntity(player);
				if ( candidate && getStats(candidate) )
				{
					recipient = candidate;
					break;
				}
			}
		}
		if ( !recipient )
		{
			participation.erase(uid);
			return;
		}

		record->state.notePartyAward();
		syntheticPartyAward = true;
		xpContextActive = true;
		snapshotFollowers(recipient);
		originalAwardXp(recipient, victim, true, false);
		applyFollowerXp();
		followers.clear();
		rootFollowerEntity = nullptr;
		ownerBases = {};
		xpContextActive = false;
		syntheticPartyAward = false;
		participation.erase(uid);
	}

	void awardXpHook(std::uint8_t* recipient, std::uint8_t* source,
		const bool share, const bool root)
	{
		if ( !syntheticPartyAward )
		{
			correlateDamage(source, recipient);
			if ( root )
			{
				if ( auto* record = findParticipation(source) )
				{
					const auto recipientKind = actorKind(recipient);
					if ( recipientKind == quality::friendly_fire::ActorKind::Player
						|| recipientKind
							== quality::friendly_fire::ActorKind::PlayerAlly )
					{
						record->state.notePartyAward();
					}
				}
			}
		}
		const bool outerRoot = root && !xpContextActive;
		if ( outerRoot )
		{
			xpContextActive = true;
			snapshotFollowers(recipient);
		}

		originalAwardXp(recipient, source, share, root);

		if ( outerRoot )
		{
			applyFollowerXp();
			followers.clear();
			rootFollowerEntity = nullptr;
			ownerBases = {};
			xpContextActive = false;
			grantParticipationXp(source);
		}
	}

	std::vector<std::uint8_t> absoluteJump(const void* destination)
	{
		std::vector<std::uint8_t> bytes = { 0xFF, 0x25, 0, 0, 0, 0 };
		const auto address = reinterpret_cast<std::uintptr_t>(destination);
		const auto* raw = reinterpret_cast<const std::uint8_t*>(&address);
		bytes.insert(bytes.end(), raw, raw + sizeof(address));
		return bytes;
	}

	bool serverAuthoritative()
	{
		return field<std::int32_t>(base, layout::multiplayerRva)
			!= layout::multiplayerClient;
	}

	std::uint32_t entityUid(const std::uint8_t* entity)
	{
		return entity ? field<std::uint32_t>(entity, layout::entityUid) : 0;
	}

	bool validIdentity(const ParticipationRecord& record)
	{
		return record.uid != 0 && record.entity && record.stats
			&& uidToEntity(record.uid) == record.entity
			&& getStats(record.entity) == record.stats;
	}

	void sweepParticipation(const std::uint32_t tick)
	{
		if ( tick - lastParticipationSweep < 64 )
		{
			return;
		}
		lastParticipationSweep = tick;
		for ( auto current = participation.begin(); current != participation.end(); )
		{
			if ( !validIdentity(current->second) )
			{
				current = participation.erase(current);
			}
			else
			{
				++current;
			}
		}
	}

	bool vanillaXpVictimEligible(std::uint8_t* entity, std::uint8_t* stats)
	{
		if ( !entity || !stats
			|| behavior(entity) != reinterpret_cast<std::uintptr_t>(
				base + layout::actMonsterRva)
			|| skill(entity, layout::skillMonsterAllySummonRank) != 0
			|| monsterIsTinkeringCreation(entity) )
		{
			return false;
		}

		const int type = field<std::int32_t>(stats, layout::statType);
		switch ( type )
		{
			case layout::revenantSkull:
			case layout::adorcisedWeapon:
			case layout::flameElemental:
			case layout::hologram:
			case layout::duckSmall:
				return false;
			case layout::skeleton:
				return !attributeNotEmpty(stats, "revenant_skeleton");
			case layout::mothSmall:
				return !attributeNotEmpty(stats, "fire_sprite");
			case layout::earthElemental:
				return !monsterAllyGetPlayerLeader(entity);
			default:
				break;
		}

		constexpr int incubus = 24;
		return type != incubus || std::strncmp(reinterpret_cast<const char*>(
			stats + layout::statName), "inner demon", 11) != 0;
	}

	ParticipationRecord* findParticipation(std::uint8_t* entity)
	{
		const auto uid = entityUid(entity);
		const auto found = participation.find(uid);
		if ( uid == 0 || found == participation.end() )
		{
			return nullptr;
		}
		if ( !validIdentity(found->second) )
		{
			participation.erase(found);
			return nullptr;
		}
		return &found->second;
	}

	void observeHpLoss(std::uint8_t* victim, std::uint8_t* stats,
		const int previousHp, const int currentHp)
	{
		if ( !serverAuthoritative() || !victim || !stats )
		{
			return;
		}
		const auto uid = entityUid(victim);
		if ( uid == 0 || uidToEntity(uid) != victim )
		{
			return;
		}
		const auto tick = field<std::uint32_t>(base, layout::ticksRva);
		sweepParticipation(tick);
		auto& record = participation[uid];
		if ( record.entity != victim || record.stats != stats )
		{
			record = {};
			record.entity = victim;
			record.stats = stats;
			record.uid = uid;
		}
		record.pendingDamage = quality::xp::participation::actualHpLoss(
			previousHp, currentHp);
		record.pendingTick = tick;
	}

	void correlateDamage(std::uint8_t* victim, std::uint8_t* attacker)
	{
		if ( !serverAuthoritative() || !victim || !attacker )
		{
			return;
		}
		auto* record = findParticipation(victim);
		if ( !record || !record->pendingDamage
			|| record->pendingTick != field<std::uint32_t>(base, layout::ticksRva) )
		{
			return;
		}
		record->pendingDamage = false;
		record->state.markDamageParticipation(actorKind(attacker),
			entityImpaired(attacker), actorKind(victim),
			vanillaXpVictimEligible(victim, record->stats));
	}

	void setHpHook(std::uint8_t* entity, const int amount)
	{
		auto* previousStats = entity
			? static_cast<std::uint8_t*>(getStats(entity)) : nullptr;
		const int previousHp = previousStats
			? field<std::int32_t>(previousStats, layout::statHp) : 0;
		originalSetHp(entity, amount);
		auto* currentStats = entity
			? static_cast<std::uint8_t*>(getStats(entity)) : nullptr;
		if ( previousStats && currentStats == previousStats )
		{
			observeHpLoss(entity, currentStats, previousHp,
				field<std::int32_t>(currentStats, layout::statHp));
		}
	}

	void killedByMonsterObituaryHook(std::uint8_t* attacker,
		std::uint8_t* victim, const bool fromSpell)
	{
		correlateDamage(victim, attacker);
		originalKilledByMonsterObituary(attacker, victim, fromSpell);
	}

	void updateEntityOnHitHook(std::uint8_t* victim,
		std::uint8_t* attacker, const bool alertTarget)
	{
		correlateDamage(victim, attacker);
		originalUpdateEntityOnHit(victim, attacker, alertTarget);
	}

	void updateEnemyBarHook(std::uint8_t* attacker, std::uint8_t* victim,
		const char* name, const int hp, const int maxHp,
		const bool lowPriority, const int gibType)
	{
		correlateDamage(victim, attacker);
		originalUpdateEnemyBar(attacker, victim, name, hp, maxHp,
			lowPriority, gibType);
	}

	void monsterCommittedDeathHook(std::uint8_t* victim)
	{
		grantParticipationXp(victim);
	}

	void prepareOrdinaryTrampoline(std::uint8_t* trampoline,
		const std::uintptr_t rva, const std::size_t overwrittenBytes)
	{
		std::memcpy(trampoline, base + rva, overwrittenBytes);
		const auto jumpBack = absoluteJump(base + rva + overwrittenBytes);
		std::memcpy(trampoline + overwrittenBytes, jumpBack.data(),
			jumpBack.size());
	}

	void prepareFriendlyFireProtectionTrampoline(std::uint8_t* trampoline)
	{
		constexpr std::size_t instructionsBeforeBranch = 12;
		std::memcpy(trampoline, base + layout::friendlyFireProtectionRva,
			instructionsBeforeBranch);
		std::vector<std::uint8_t> code(trampoline,
			trampoline + instructionsBeforeBranch);
		code.insert(code.end(), { 0x75, 0x0E });
		const auto nullTarget = absoluteJump(
			base + layout::friendlyFireProtectionNullTargetRva);
		code.insert(code.end(), nullTarget.begin(), nullTarget.end());
		const auto continuation = absoluteJump(
			base + layout::friendlyFireProtectionContinueRva);
		code.insert(code.end(), continuation.begin(), continuation.end());
		std::memcpy(trampoline, code.data(), code.size());
	}

	std::vector<std::uint8_t> relativeBranch(const std::uintptr_t fromRva,
		const void* destination, const std::uint8_t opcode)
	{
		std::vector<std::uint8_t> bytes(5);
		bytes[0] = opcode;
		const auto from = reinterpret_cast<std::uintptr_t>(base + fromRva + 5);
		const auto to = reinterpret_cast<std::uintptr_t>(destination);
		const auto difference = static_cast<std::intptr_t>(to - from);
		const auto displacement = static_cast<std::int32_t>(difference);
		if ( static_cast<std::intptr_t>(displacement) != difference )
		{
			return {};
		}
		std::memcpy(bytes.data() + 1, &displacement, sizeof(displacement));
		return bytes;
	}

	void appendImmediate(std::vector<std::uint8_t>& code, const void* address)
	{
		const auto value = reinterpret_cast<std::uintptr_t>(address);
		const auto* raw = reinterpret_cast<const std::uint8_t*>(&value);
		code.insert(code.end(), raw, raw + sizeof(value));
	}

	bool appendRelativeJump(std::vector<std::uint8_t>& code,
		const std::uint8_t* stub, const void* destination)
	{
		const auto from = reinterpret_cast<std::uintptr_t>(stub + code.size() + 5);
		const auto to = reinterpret_cast<std::uintptr_t>(destination);
		const auto difference = static_cast<std::intptr_t>(to - from);
		const auto displacement = static_cast<std::int32_t>(difference);
		if ( static_cast<std::intptr_t>(displacement) != difference )
		{
			return false;
		}
		code.push_back(0xE9);
		const auto* raw = reinterpret_cast<const std::uint8_t*>(&displacement);
		code.insert(code.end(), raw, raw + sizeof(displacement));
		return true;
	}

	std::vector<std::uint8_t> makeCaptureStub()
	{
		std::vector<std::uint8_t> code = {
			0x9C,
			0x50, 0x51, 0x52,
			0x41, 0x50, 0x41, 0x51,
			0x41, 0x52, 0x41, 0x53,
			0x48, 0x81, 0xEC, 0x88, 0x00, 0x00, 0x00,
			0xF3, 0x0F, 0x7F, 0x44, 0x24, 0x20,
			0xF3, 0x0F, 0x7F, 0x4C, 0x24, 0x30,
			0xF3, 0x0F, 0x7F, 0x54, 0x24, 0x40,
			0xF3, 0x0F, 0x7F, 0x5C, 0x24, 0x50,
			0xF3, 0x0F, 0x7F, 0x64, 0x24, 0x60,
			0xF3, 0x0F, 0x7F, 0x6C, 0x24, 0x70,
			0x49, 0x8B, 0xCE,
			0x41, 0x8B, 0xD4,
			0x48, 0xB8,
		};
		appendImmediate(code, reinterpret_cast<const void*>(&captureOwnerBase));
		const std::initializer_list<std::uint8_t> tail = {
			0xFF, 0xD0,
			0xF3, 0x0F, 0x6F, 0x44, 0x24, 0x20,
			0xF3, 0x0F, 0x6F, 0x4C, 0x24, 0x30,
			0xF3, 0x0F, 0x6F, 0x54, 0x24, 0x40,
			0xF3, 0x0F, 0x6F, 0x5C, 0x24, 0x50,
			0xF3, 0x0F, 0x6F, 0x64, 0x24, 0x60,
			0xF3, 0x0F, 0x6F, 0x6C, 0x24, 0x70,
			0x48, 0x81, 0xC4, 0x88, 0x00, 0x00, 0x00,
			0x41, 0x5B, 0x41, 0x5A,
			0x41, 0x59, 0x41, 0x58,
			0x5A, 0x59, 0x58, 0x9D,
			0xBB, 0x9F, 0x86, 0x01, 0x00,
			0xC3,
		};
		code.insert(code.end(), tail.begin(), tail.end());
		return code;
	}

	std::vector<std::uint8_t> makeFollowerBlockStub(std::uint8_t* stub)
	{
		std::vector<std::uint8_t> code = { 0x48, 0xB9 };
		appendImmediate(code, base + layout::actPlayerRva);
		code.push_back(0x50);
		code.insert(code.end(), { 0x48, 0xB8 });
		appendImmediate(code, &xpContextActive);
		code.insert(code.end(), { 0x80, 0x38, 0x00 });
		const std::size_t activeJump = code.size();
		code.insert(code.end(), { 0x75, 0x00 });
		code.push_back(0x58);
		code.insert(code.end(), { 0x49, 0x39, 0x8E, 0x50, 0x13, 0x00, 0x00 });
		const std::size_t behaviorJump = code.size();
		code.insert(code.end(), { 0x75, 0x00 });
		if ( !appendRelativeJump(code, stub,
			base + layout::xpFollowerBlockContinueRva) )
		{
			return {};
		}
		const std::size_t active = code.size();
		code.push_back(0x58);
		const std::size_t skip = code.size();
		if ( !appendRelativeJump(code, stub,
			base + layout::xpFollowerBlockSkipRva) )
		{
			return {};
		}
		code[activeJump + 1] = static_cast<std::uint8_t>(active - (activeJump + 2));
		code[behaviorJump + 1] = static_cast<std::uint8_t>(skip - (behaviorJump + 2));
		return code;
	}

	std::vector<std::uint8_t> makeMainBlockStub(std::uint8_t* stub)
	{
		std::vector<std::uint8_t> code = { 0x84, 0xDB };
		const std::size_t excludedJump = code.size();
		code.insert(code.end(), { 0x74, 0x00 });
		code.push_back(0x50);
		code.insert(code.end(), { 0x48, 0xB8 });
		appendImmediate(code, &rootFollowerEntity);
		code.insert(code.end(), { 0x4C, 0x3B, 0x30 });
		code.push_back(0x58);
		const std::size_t followerJump = code.size();
		code.insert(code.end(), { 0x74, 0x00 });
		if ( !appendRelativeJump(code, stub, base + layout::xpMainBlockContinueRva) )
		{
			return {};
		}
		const std::size_t skip = code.size();
		if ( !appendRelativeJump(code, stub, base + layout::xpMainBlockSkipRva) )
		{
			return {};
		}
		code[excludedJump + 1] = static_cast<std::uint8_t>(skip - (excludedJump + 2));
		code[followerJump + 1] = static_cast<std::uint8_t>(skip - (followerJump + 2));
		return code;
	}

	std::vector<std::uint8_t> makeCommittedDeathStub()
	{
		std::vector<std::uint8_t> code = {
			0x9C,
			0x50, 0x51, 0x52,
			0x41, 0x50, 0x41, 0x51,
			0x41, 0x52, 0x41, 0x53,
			0x48, 0x81, 0xEC, 0x80, 0x00, 0x00, 0x00,
			0xF3, 0x0F, 0x7F, 0x44, 0x24, 0x20,
			0xF3, 0x0F, 0x7F, 0x4C, 0x24, 0x30,
			0xF3, 0x0F, 0x7F, 0x54, 0x24, 0x40,
			0xF3, 0x0F, 0x7F, 0x5C, 0x24, 0x50,
			0xF3, 0x0F, 0x7F, 0x64, 0x24, 0x60,
			0xF3, 0x0F, 0x7F, 0x6C, 0x24, 0x70,
			0x49, 0x8B, 0xCF,
			0x48, 0xB8,
		};
		appendImmediate(code,
			reinterpret_cast<const void*>(&monsterCommittedDeathHook));
		const std::initializer_list<std::uint8_t> restore = {
			0xFF, 0xD0,
			0xF3, 0x0F, 0x6F, 0x44, 0x24, 0x20,
			0xF3, 0x0F, 0x6F, 0x4C, 0x24, 0x30,
			0xF3, 0x0F, 0x6F, 0x54, 0x24, 0x40,
			0xF3, 0x0F, 0x6F, 0x5C, 0x24, 0x50,
			0xF3, 0x0F, 0x6F, 0x64, 0x24, 0x60,
			0xF3, 0x0F, 0x6F, 0x6C, 0x24, 0x70,
			0x48, 0x81, 0xC4, 0x80, 0x00, 0x00, 0x00,
			0x41, 0x5B, 0x41, 0x5A,
			0x41, 0x59, 0x41, 0x58,
			0x5A, 0x59, 0x58, 0x9D,
		};
		code.insert(code.end(), restore.begin(), restore.end());
		code.insert(code.end(), layout::monsterCommittedDeathSignature.begin(),
			layout::monsterCommittedDeathSignature.end());
		const auto continuation = absoluteJump(base
			+ layout::monsterCommittedDeathRva
			+ layout::monsterCommittedDeathSignature.size());
		code.insert(code.end(), continuation.begin(), continuation.end());
		return code;
	}

	void* allocateNearModule(const std::size_t bytes)
	{
		SYSTEM_INFO info {};
		GetSystemInfo(&info);
		const auto granularity = static_cast<std::uintptr_t>(info.dwAllocationGranularity);
		const auto module = reinterpret_cast<std::uintptr_t>(base);
		for ( std::uintptr_t distance = 0x02000000;
			distance < 0x70000000; distance += granularity )
		{
			const auto candidate = reinterpret_cast<void*>(
				(module + distance) & ~(granularity - 1));
			if ( void* memory = VirtualAlloc(candidate, bytes,
				MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE) )
			{
				return memory;
			}
		}
		return nullptr;
	}

	void releaseAllocations()
	{
		participation.clear();
		if ( participationTrampolines )
		{
			VirtualFree(participationTrampolines, 0, MEM_RELEASE);
			participationTrampolines = nullptr;
			originalSetHp = nullptr;
			originalKilledByMonsterObituary = nullptr;
			originalUpdateEntityOnHit = nullptr;
			originalUpdateEnemyBar = nullptr;
		}
		if ( friendlyFireTrampolines )
		{
			VirtualFree(friendlyFireTrampolines, 0, MEM_RELEASE);
			friendlyFireTrampolines = nullptr;
			originalCheckEnemy = nullptr;
			originalCheckFriend = nullptr;
			originalFriendlyFireProtection = nullptr;
		}
		if ( xpTrampoline )
		{
			VirtualFree(xpTrampoline, 0, MEM_RELEASE);
			xpTrampoline = nullptr;
			originalAwardXp = nullptr;
		}
		if ( relayPage )
		{
			VirtualFree(relayPage, 0, MEM_RELEASE);
			relayPage = nullptr;
		}
		quality::minimap_runtime::release();
	}

	template <std::size_t Size>
	bool matches(const std::uintptr_t rva,
		const std::array<std::uint8_t, Size>& signature)
	{
		return std::memcmp(base + rva, signature.data(), signature.size()) == 0;
	}

	bool validateLayout()
	{
		return matches(layout::awardXpRva, layout::awardXpEntrySignature)
			&& matches(layout::awardXpHookRva, layout::awardXpHookSignature)
			&& matches(layout::getStatsRva, layout::getStatsSignature)
			&& matches(layout::uidToEntityRva, layout::uidToEntitySignature)
			&& matches(layout::entityInspirationRva, layout::entityInspirationSignature)
			&& matches(layout::monsterIsTinkeringCreationRva,
				layout::monsterIsTinkeringCreationSignature)
			&& matches(layout::monsterAllyGetPlayerLeaderRva,
				layout::monsterAllyGetPlayerLeaderSignature)
			&& matches(layout::checkEnemyRva, layout::checkEnemySignature)
			&& matches(layout::checkFriendRva, layout::checkFriendSignature)
			&& matches(layout::friendlyFireProtectionRva,
				layout::friendlyFireProtectionSignature)
			&& matches(layout::statGetAttributeRva, layout::statGetAttributeSignature)
			&& matches(layout::msvcStringDestroyRva, layout::msvcStringDestroySignature)
			&& matches(layout::steamAchievementEntityRva,
				layout::steamAchievementEntitySignature)
			&& matches(layout::compendiumEventUpdateRva,
				layout::compendiumEventUpdateSignature)
			&& matches(layout::setHpRva, layout::setHpSignature)
			&& matches(layout::killedByMonsterObituaryHookRva,
				layout::killedByMonsterObituaryHookSignature)
			&& matches(layout::updateEntityOnHitHookRva,
				layout::updateEntityOnHitHookSignature)
			&& matches(layout::updateEnemyBarRva,
				layout::updateEnemyBarSignature)
			&& matches(layout::monsterCommittedDeathRva,
				layout::monsterCommittedDeathSignature)
			&& matches(layout::xpCaptureRva, layout::xpCaptureSignature)
			&& matches(layout::xpFollowerBlockRva, layout::xpFollowerBlockSignature)
			&& matches(layout::xpMainBlockRva, layout::xpMainBlockSignature);
	}

	bool writePatch(Patch& patch)
	{
		auto* address = base + patch.rva;
		DWORD oldProtection = 0;
		if ( !VirtualProtect(address, patch.replacement.size(),
			PAGE_EXECUTE_READWRITE, &oldProtection) )
		{
			return false;
		}
		std::memcpy(address, patch.replacement.data(), patch.replacement.size());
		FlushInstructionCache(GetCurrentProcess(), address, patch.replacement.size());
		DWORD ignored = 0;
		VirtualProtect(address, patch.replacement.size(), oldProtection, &ignored);
		patch.applied = true;
		return std::memcmp(address, patch.replacement.data(),
			patch.replacement.size()) == 0;
	}

	void rollback(std::vector<Patch>& patches)
	{
		for ( auto patch = patches.rbegin(); patch != patches.rend(); ++patch )
		{
			if ( !patch->applied )
			{
				continue;
			}
			auto* address = base + patch->rva;
			DWORD oldProtection = 0;
			if ( VirtualProtect(address, patch->original.size(),
				PAGE_EXECUTE_READWRITE, &oldProtection) )
			{
				std::memcpy(address, patch->original.data(), patch->original.size());
				FlushInstructionCache(GetCurrentProcess(), address, patch->original.size());
				DWORD ignored = 0;
				VirtualProtect(address, patch->original.size(), oldProtection, &ignored);
			}
		}
	}

	bool installRuntime()
	{
		base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
		if ( !base || !validateLayout() )
		{
			debug("Barony v5.0.2 runtime layout mismatch");
			return false;
		}

		getStats = reinterpret_cast<EntityMethodFn>(base + layout::getStatsRva);
		uidToEntity = reinterpret_cast<UidToEntityFn>(base + layout::uidToEntityRva);
		entityInspiration = reinterpret_cast<EntityIntMethodFn>(
			base + layout::entityInspirationRva);
		monsterIsTinkeringCreation = reinterpret_cast<EntityBoolMethodFn>(
			base + layout::monsterIsTinkeringCreationRva);
		monsterAllyGetPlayerLeader = reinterpret_cast<EntityMethodFn>(
			base + layout::monsterAllyGetPlayerLeaderRva);
		statGetAttribute = reinterpret_cast<StatGetAttributeFn>(
			base + layout::statGetAttributeRva);
		msvcStringDestroy = reinterpret_cast<MsvcStringDestroyFn>(
			base + layout::msvcStringDestroyRva);
		steamAchievementEntity = reinterpret_cast<SteamAchievementEntityFn>(
			base + layout::steamAchievementEntityRva);
		compendiumEventUpdate = reinterpret_cast<CompendiumEventUpdateFn>(
			base + layout::compendiumEventUpdateRva);

		relayPage = allocateNearModule(4 * relayStride);
		xpTrampoline = VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE,
			PAGE_EXECUTE_READWRITE);
		friendlyFireTrampolines = VirtualAlloc(nullptr, 3 * trampolineStride,
			MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		participationTrampolines = VirtualAlloc(nullptr, 4 * trampolineStride,
			MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if ( !relayPage || !xpTrampoline || !friendlyFireTrampolines
			|| !participationTrampolines )
		{
			debug("Runtime hook allocation failed");
			releaseAllocations();
			return false;
		}

		auto* relay = static_cast<std::uint8_t*>(relayPage);
		auto* captureRelay = relay;
		auto* followerRelay = relay + relayStride;
		auto* mainRelay = relay + 2 * relayStride;
		auto* committedDeathRelay = relay + 3 * relayStride;
		const auto captureStub = makeCaptureStub();
		const auto followerStub = makeFollowerBlockStub(followerRelay);
		const auto mainStub = makeMainBlockStub(mainRelay);
		const auto committedDeathStub = makeCommittedDeathStub();
		if ( captureStub.empty() || followerStub.empty() || mainStub.empty()
			|| committedDeathStub.empty()
			|| captureStub.size() > relayStride
			|| followerStub.size() > relayStride
			|| mainStub.size() > relayStride
			|| committedDeathStub.size() > relayStride )
		{
			debug("EXP relay construction failed");
			releaseAllocations();
			return false;
		}
		std::memcpy(captureRelay, captureStub.data(), captureStub.size());
		std::memcpy(followerRelay, followerStub.data(), followerStub.size());
		std::memcpy(mainRelay, mainStub.data(), mainStub.size());
		std::memcpy(committedDeathRelay, committedDeathStub.data(),
			committedDeathStub.size());

		auto* trampoline = static_cast<std::uint8_t*>(xpTrampoline);
		std::memcpy(trampoline, base + layout::awardXpHookRva,
			layout::awardXpHookSignature.size());
		const auto jumpBack = absoluteJump(base + layout::awardXpHookRva
			+ layout::awardXpHookSignature.size());
		std::memcpy(trampoline + layout::awardXpHookSignature.size(),
			jumpBack.data(), jumpBack.size());
		originalAwardXp = reinterpret_cast<AwardXpFn>(trampoline);

		auto* friendlyTrampolines = static_cast<std::uint8_t*>(
			friendlyFireTrampolines);
		auto* enemyTrampoline = friendlyTrampolines;
		auto* friendTrampoline = friendlyTrampolines + trampolineStride;
		auto* protectionTrampoline = friendlyTrampolines + 2 * trampolineStride;
		prepareOrdinaryTrampoline(enemyTrampoline, layout::checkEnemyRva,
			layout::checkEnemySignature.size());
		prepareOrdinaryTrampoline(friendTrampoline, layout::checkFriendRva,
			layout::checkFriendSignature.size());
		prepareFriendlyFireProtectionTrampoline(protectionTrampoline);
		originalCheckEnemy = reinterpret_cast<EntityPairBoolMethodFn>(
			enemyTrampoline);
		originalCheckFriend = reinterpret_cast<EntityPairBoolMethodFn>(
			friendTrampoline);
		originalFriendlyFireProtection = reinterpret_cast<EntityPairBoolMethodFn>(
			protectionTrampoline);

		auto* participationHooks = static_cast<std::uint8_t*>(
			participationTrampolines);
		auto* setHpTrampoline = participationHooks;
		auto* obituaryTrampoline = participationHooks + trampolineStride;
		auto* entityHitTrampoline = participationHooks + 2 * trampolineStride;
		auto* enemyBarTrampoline = participationHooks + 3 * trampolineStride;
		prepareOrdinaryTrampoline(setHpTrampoline, layout::setHpRva,
			layout::setHpSignature.size());
		prepareOrdinaryTrampoline(obituaryTrampoline,
			layout::killedByMonsterObituaryHookRva,
			layout::killedByMonsterObituaryHookSignature.size());
		prepareOrdinaryTrampoline(entityHitTrampoline,
			layout::updateEntityOnHitHookRva,
			layout::updateEntityOnHitHookSignature.size());
		prepareOrdinaryTrampoline(enemyBarTrampoline,
			layout::updateEnemyBarRva, layout::updateEnemyBarSignature.size());
		originalSetHp = reinterpret_cast<SetHpFn>(setHpTrampoline);
		originalKilledByMonsterObituary =
			reinterpret_cast<EntityPairVoidBoolMethodFn>(obituaryTrampoline);
		originalUpdateEntityOnHit =
			reinterpret_cast<EntityPairVoidBoolMethodFn>(entityHitTrampoline);
		originalUpdateEnemyBar = reinterpret_cast<UpdateEnemyBarFn>(
			enemyBarTrampoline);

		std::vector<Patch> patches;
		const auto addPatch = [&](const std::uintptr_t rva, const auto& expected,
			std::vector<std::uint8_t> replacement) {
			Patch patch;
			patch.rva = rva;
			patch.expected.assign(expected.begin(), expected.end());
			patch.replacement = std::move(replacement);
			patch.original.assign(base + rva,
				base + rva + patch.replacement.size());
			patches.push_back(std::move(patch));
		};

		addPatch(layout::awardXpHookRva, layout::awardXpHookSignature,
			absoluteJump(reinterpret_cast<void*>(&awardXpHook)));
		addPatch(layout::checkEnemyRva, layout::checkEnemySignature,
			absoluteJump(reinterpret_cast<void*>(&checkEnemyHook)));
		addPatch(layout::checkFriendRva, layout::checkFriendSignature,
			absoluteJump(reinterpret_cast<void*>(&checkFriendHook)));
		addPatch(layout::friendlyFireProtectionRva,
			layout::friendlyFireProtectionSignature,
			absoluteJump(reinterpret_cast<void*>(&friendlyFireProtectionHook)));
		addPatch(layout::setHpRva, layout::setHpSignature,
			absoluteJump(reinterpret_cast<void*>(&setHpHook)));
		addPatch(layout::killedByMonsterObituaryHookRva,
			layout::killedByMonsterObituaryHookSignature,
			absoluteJump(reinterpret_cast<void*>(&killedByMonsterObituaryHook)));
		addPatch(layout::updateEntityOnHitHookRva,
			layout::updateEntityOnHitHookSignature,
			absoluteJump(reinterpret_cast<void*>(&updateEntityOnHitHook)));
		addPatch(layout::updateEnemyBarRva, layout::updateEnemyBarSignature,
			absoluteJump(reinterpret_cast<void*>(&updateEnemyBarHook)));
		auto committedDeathJump = relativeBranch(
			layout::monsterCommittedDeathRva, committedDeathRelay, 0xE9);
		if ( committedDeathJump.empty() )
		{
			debug("Committed-death relay is outside relative-branch range");
			releaseAllocations();
			return false;
		}
		committedDeathJump.resize(layout::monsterCommittedDeathSignature.size(),
			0x90);
		addPatch(layout::monsterCommittedDeathRva,
			layout::monsterCommittedDeathSignature,
			std::move(committedDeathJump));
		addPatch(layout::xpCaptureRva, layout::xpCaptureSignature,
			relativeBranch(layout::xpCaptureRva, captureRelay, 0xE8));
		auto followerJump = relativeBranch(layout::xpFollowerBlockRva,
			followerRelay, 0xE9);
		if ( followerJump.empty() )
		{
			debug("Follower EXP relay is outside relative-branch range");
			releaseAllocations();
			return false;
		}
		followerJump.resize(layout::xpFollowerBlockSignature.size(), 0x90);
		addPatch(layout::xpFollowerBlockRva,
			layout::xpFollowerBlockSignature, std::move(followerJump));
		auto mainJump = relativeBranch(layout::xpMainBlockRva, mainRelay, 0xE9);
		if ( mainJump.empty() )
		{
			debug("Main-recipient EXP relay is outside relative-branch range");
			releaseAllocations();
			return false;
		}
		mainJump.resize(layout::xpMainBlockSignature.size(), 0x90);
		addPatch(layout::xpMainBlockRva,
			layout::xpMainBlockSignature, std::move(mainJump));
		if ( !quality::minimap_runtime::prepare(base, patches) )
		{
			debug("Minimap appearance hook preparation failed");
			releaseAllocations();
			return false;
		}

		for ( const auto& patch : patches )
		{
			if ( patch.replacement.empty()
				|| patch.replacement.size() > patch.expected.size()
				|| std::memcmp(base + patch.rva, patch.expected.data(),
					patch.expected.size()) != 0 )
			{
				debug("Runtime patch transaction preflight failed");
				releaseAllocations();
				return false;
			}
		}
		for ( auto& patch : patches )
		{
			if ( !writePatch(patch) )
			{
				rollback(patches);
				releaseAllocations();
				debug("Runtime patch transaction rolled back");
				return false;
			}
		}

		debug("Quality EXP, friendly fire, and minimap runtime installed for Barony v5.0.2");
		return true;
	}
}

extern "C" __declspec(dllexport) DWORD WINAPI QualityBaronyInitialize(void*)
{
	const LONG previous = InterlockedCompareExchange(&initializationState, 1, 0);
	if ( previous == 2 )
	{
		return 1;
	}
	if ( previous != 0 )
	{
		return 0;
	}
	const bool installed = installRuntime();
	InterlockedExchange(&initializationState, installed ? 2 : 0);
	return installed ? 1 : 0;
}
