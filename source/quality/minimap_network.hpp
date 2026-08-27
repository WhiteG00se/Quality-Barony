#pragma once

#include <cstdint>

namespace quality::minimap::network
{
	constexpr std::uint8_t protocolVersion = 6;
	constexpr std::uint8_t rosterProtocolVersion = 2;

	enum class Stream : std::uint8_t { State, Roster };

	constexpr bool compatible(const Stream stream, const std::uint8_t version)
	{
		return stream == Stream::State ? version == protocolVersion
			: version == rosterProtocolVersion;
	}

	constexpr bool validPlayerSlot(const int player)
	{
		return player >= 0 && player < 4;
	}

	constexpr bool validSender(const int multiplayerMode, const int sender)
	{
		return multiplayerMode == 1 ? sender > 0 && sender < 4
			: multiplayerMode == 2 ? sender == 0 : false;
	}

	constexpr int hostNumberForTarget(const int multiplayerMode,
		const int targetPlayer)
	{
		return multiplayerMode == 1 && targetPlayer > 0 && targetPlayer < 4
			? targetPlayer - 1
			: multiplayerMode == 2 && targetPlayer == 0 ? 0 : -1;
	}

	constexpr std::uint64_t deliveryKey(const int sender,
		const std::uint32_t sequence)
	{
		return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(sender))
			<< 32) | sequence;
	}
}
