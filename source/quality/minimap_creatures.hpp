#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace quality::minimap::creatures
{
	constexpr int maximumPlayers = 4;
	constexpr double angleEpsilon = 1.0e-9;

	constexpr bool acceptSightingSnapshot(const bool capability,
		const bool acceptedSender, const bool matchingFloor,
		const bool matchingGeneration, const std::uint32_t sequence,
		const std::uint32_t currentSequence)
	{
		return capability && acceptedSender && matchingFloor && matchingGeneration
			&& sequence != 0 && sequence >= currentSequence;
	}

	enum class Disposition : std::uint8_t
	{
		Excluded,
		Hostile,
		Neutral,
	};

	constexpr bool ordinarySightingVisible(const Disposition disposition,
		const bool invisible, const bool invisibleDither,
		const bool insideCone, const bool clearLineOfSight)
	{
		return disposition == Disposition::Hostile
			&& !(invisible && !invisibleDither)
			&& insideCone && clearLineOfSight;
	}

	constexpr bool inForwardHalfPlane(const double originX,
		const double originY, const double yaw, const double targetX,
		const double targetY)
	{
		const double dx = targetX - originX;
		const double dy = targetY - originY;
		return dx == 0.0 && dy == 0.0
			? true : dx * std::cos(yaw) + dy * std::sin(yaw) >= -angleEpsilon;
	}

	struct Blocker
	{
		double minimumX = 0.0;
		double minimumY = 0.0;
		double maximumX = 0.0;
		double maximumY = 0.0;
	};

	inline bool segmentIntersectsBeforeTarget(const double originX,
		const double originY, const double targetX, const double targetY,
		const Blocker& blocker)
	{
		const double dx = targetX - originX;
		const double dy = targetY - originY;
		double entry = 0.0;
		double exit = 1.0;
		auto clip = [&](const double start, const double delta,
			const double minimum, const double maximum)
		{
			if ( std::abs(delta) <= angleEpsilon )
			{
				return start >= minimum && start <= maximum;
			}
			double first = (minimum - start) / delta;
			double second = (maximum - start) / delta;
			if ( first > second ) { std::swap(first, second); }
			entry = std::max(entry, first);
			exit = std::min(exit, second);
			return entry <= exit;
		};
		if ( !clip(originX, dx, blocker.minimumX, blocker.maximumX)
			|| !clip(originY, dy, blocker.minimumY, blocker.maximumY) )
		{
			return false;
		}
		return exit > angleEpsilon && entry < 1.0 - angleEpsilon;
	}

	template <typename TileBlocked>
	bool clearTileLine(const double originX, const double originY,
		const double targetX, const double targetY, TileBlocked blocked)
	{
		int tileX = static_cast<int>(std::floor(originX));
		int tileY = static_cast<int>(std::floor(originY));
		const int targetTileX = static_cast<int>(std::floor(targetX));
		const int targetTileY = static_cast<int>(std::floor(targetY));
		if ( tileX == targetTileX && tileY == targetTileY ) { return true; }

		const double dx = targetX - originX;
		const double dy = targetY - originY;
		const int stepX = dx > 0.0 ? 1 : (dx < 0.0 ? -1 : 0);
		const int stepY = dy > 0.0 ? 1 : (dy < 0.0 ? -1 : 0);
		const double infinity = 1.0e30;
		const double deltaX = stepX == 0 ? infinity : std::abs(1.0 / dx);
		const double deltaY = stepY == 0 ? infinity : std::abs(1.0 / dy);
		double nextX = stepX > 0
			? (std::floor(originX) + 1.0 - originX) * deltaX
			: (stepX < 0 ? (originX - std::floor(originX)) * deltaX : infinity);
		double nextY = stepY > 0
			? (std::floor(originY) + 1.0 - originY) * deltaY
			: (stepY < 0 ? (originY - std::floor(originY)) * deltaY : infinity);

		while ( tileX != targetTileX || tileY != targetTileY )
		{
			if ( nextX < nextY )
			{
				tileX += stepX;
				nextX += deltaX;
			}
			else if ( nextY < nextX )
			{
				tileY += stepY;
				nextY += deltaY;
			}
			else
			{
				const int adjacentX = tileX + stepX;
				const int adjacentY = tileY + stepY;
				if ( (adjacentX != targetTileX || tileY != targetTileY)
					&& blocked(adjacentX, tileY) )
				{
					return false;
				}
				if ( (tileX != targetTileX || adjacentY != targetTileY)
					&& blocked(tileX, adjacentY) )
				{
					return false;
				}
				tileX += stepX;
				tileY += stepY;
				nextX += deltaX;
				nextY += deltaY;
			}
			if ( tileX == targetTileX && tileY == targetTileY ) { return true; }
			if ( blocked(tileX, tileY) ) { return false; }
		}
		return true;
	}

	class FinalRevealState
	{
	public:
		void reset()
		{
			activated_ = false;
			latched_ = false;
			notificationPending_ = false;
			triggerHostiles_ = 0;
			triggerNeutrals_ = 0;
		}

		void activate() { activated_ = true; }

		bool update(const int hostiles, const int neutrals)
		{
			if ( !activated_ || latched_ || hostiles > 2 ) { return false; }
			latched_ = true;
			notificationPending_ = true;
			triggerHostiles_ = std::max(0, hostiles);
			triggerNeutrals_ = std::max(0, neutrals);
			return true;
		}

		bool takeNotification(int& hostiles, int& neutrals)
		{
			if ( !notificationPending_ ) { return false; }
			notificationPending_ = false;
			hostiles = triggerHostiles_;
			neutrals = triggerNeutrals_;
			return true;
		}

		bool activated() const { return activated_; }
		bool latched() const { return latched_; }

	private:
		bool activated_ = false;
		bool latched_ = false;
		bool notificationPending_ = false;
		int triggerHostiles_ = 0;
		int triggerNeutrals_ = 0;
	};

	class SightingUnion
	{
	public:
		bool replace(const int player, const std::uint32_t sequence,
			const std::vector<std::uint32_t>& uids)
		{
			if ( player < 0 || player >= maximumPlayers
				|| sequence == 0 || sequence <= sequences_[player] )
			{
				return false;
			}
			sequences_[player] = sequence;
			byPlayer_[player].clear();
			for ( const auto uid : uids )
			{
				if ( uid ) { byPlayer_[player].insert(uid); }
			}
			return true;
		}

		void clearPlayer(const int player)
		{
			if ( player < 0 || player >= maximumPlayers ) { return; }
			byPlayer_[player].clear();
			sequences_[player] = 0;
		}

		void reset()
		{
			for ( auto& sightings : byPlayer_ ) { sightings.clear(); }
			sequences_.fill(0);
		}

		std::unordered_set<std::uint32_t> combined() const
		{
			std::unordered_set<std::uint32_t> result;
			for ( const auto& sightings : byPlayer_ )
			{
				result.insert(sightings.begin(), sightings.end());
			}
			return result;
		}

	private:
		std::array<std::unordered_set<std::uint32_t>, maximumPlayers> byPlayer_;
		std::array<std::uint32_t, maximumPlayers> sequences_ {};
	};
}
