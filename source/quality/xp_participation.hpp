#pragma once

#include "friendly_fire.hpp"

namespace quality::xp::participation
{
	using ActorKind = quality::friendly_fire::ActorKind;

	constexpr bool actualHpLoss(const int previousHp, const int currentHp)
	{
		return currentHp < previousHp;
	}

	constexpr bool qualifyingAttacker(const ActorKind attacker,
		const bool attackerImpaired)
	{
		return attacker == ActorKind::Player
			|| attacker == ActorKind::PlayerAlly
			|| attackerImpaired;
	}

	constexpr bool qualifyingVictim(const ActorKind victim,
		const bool vanillaXpEligible)
	{
		return vanillaXpEligible
			&& victim == ActorKind::IndependentMonster;
	}

	constexpr bool shouldGrantPartyXp(const bool authoritative,
		const bool qualified, const bool partyAlreadyAwarded,
		const ActorKind victim, const bool vanillaXpEligible)
	{
		return authoritative && qualified && !partyAlreadyAwarded
			&& qualifyingVictim(victim, vanillaXpEligible);
	}

	class State
	{
	public:
		constexpr void markDamageParticipation(const ActorKind attacker,
			const bool attackerImpaired, const ActorKind victim,
			const bool vanillaXpEligible)
		{
			if ( qualifyingVictim(victim, vanillaXpEligible)
				&& qualifyingAttacker(attacker, attackerImpaired) )
			{
				qualified_ = true;
			}
		}

		constexpr void notePartyAward()
		{
			partyAwarded_ = true;
		}

		constexpr bool claimPartyXp(const bool authoritative,
			const ActorKind victim, const bool vanillaXpEligible)
		{
			if ( completed_ )
			{
				return false;
			}
			completed_ = true;
			return shouldGrantPartyXp(authoritative, qualified_,
				partyAwarded_, victim, vanillaXpEligible);
		}

		constexpr bool qualified() const
		{
			return qualified_;
		}

		constexpr bool partyAwarded() const
		{
			return partyAwarded_;
		}

	private:
		bool qualified_ = false;
		bool partyAwarded_ = false;
		bool completed_ = false;
	};
}
