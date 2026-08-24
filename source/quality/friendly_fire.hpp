#pragma once

namespace quality::friendly_fire
{
	enum class ActorKind
	{
		Other,
		Player,
		PlayerAlly,
		IndependentMonster,
	};

	enum class Decision
	{
		Preserve,
		ForceFalse,
		ForceTrue,
	};

	constexpr bool impaired(const bool confused, const bool drunk)
	{
		return confused || drunk;
	}

	constexpr bool independentPair(const ActorKind attacker,
		const ActorKind target)
	{
		return attacker == ActorKind::IndependentMonster
			&& target == ActorKind::IndependentMonster;
	}

	constexpr bool playerPartyMember(const ActorKind actor)
	{
		return actor == ActorKind::Player || actor == ActorKind::PlayerAlly;
	}

	constexpr bool playerPartyPair(const ActorKind attacker,
		const ActorKind target)
	{
		return playerPartyMember(attacker) && playerPartyMember(target);
	}

	constexpr Decision hostilityDecision(const bool friendlyFireEnabled,
		const ActorKind attacker, const ActorKind target,
		const bool attackerImpaired)
	{
		return !friendlyFireEnabled && !attackerImpaired
			&& independentPair(attacker, target)
			? Decision::ForceFalse
			: Decision::Preserve;
	}

	constexpr Decision friendshipDecision(const bool friendlyFireEnabled,
		const ActorKind attacker, const ActorKind target,
		const bool attackerImpaired)
	{
		return !friendlyFireEnabled && !attackerImpaired
			&& independentPair(attacker, target)
			? Decision::ForceTrue
			: Decision::Preserve;
	}

	constexpr Decision protectionDecision(const bool friendlyFireEnabled,
		const ActorKind attacker, const ActorKind target,
		const bool attackerImpaired)
	{
		if ( friendlyFireEnabled )
		{
			return Decision::Preserve;
		}
		if ( playerPartyPair(attacker, target) && attackerImpaired )
		{
			return Decision::ForceFalse;
		}
		if ( attacker == ActorKind::PlayerAlly && playerPartyMember(target) )
		{
			return Decision::ForceTrue;
		}
		if ( independentPair(attacker, target) )
		{
			return attackerImpaired
				? Decision::ForceFalse
				: Decision::ForceTrue;
		}
		return Decision::Preserve;
	}
}
