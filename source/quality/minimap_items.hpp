#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace quality::minimap::items
{
	constexpr bool eligibleGroundItem(const bool itemBehavior,
		const bool contained, const bool lootBag)
	{
		return itemBehavior && !contained && !lootBag;
	}

	constexpr bool locallyAuthoritative(const int multiplayerMode)
	{
		return multiplayerMode != 2;
	}

	struct Marker
	{
		std::uint32_t markerId = 0;
		std::uint32_t entityUid = 0;
		std::uint32_t inventoryKey = 0;
		int x = 0;
		int y = 0;
	};

	struct Record
	{
		Marker marker {};
		bool partyDropped = false;
	};

	constexpr std::uint32_t stackFingerprint(
		const std::array<std::uint32_t, 5>& fields)
	{
		std::uint32_t hash = 2166136261U;
		for ( const auto value : fields )
		{
			for ( int byte = 0; byte < 4; ++byte )
			{
				hash ^= static_cast<std::uint8_t>(value >> (byte * 8));
				hash *= 16777619U;
			}
		}
		return hash != 0 ? hash : 1;
	}

	class State
	{
	public:
		void reset()
		{
			records_.clear();
			inventoryMarkers_.clear();
			nextGeneratedMarkerId_ = 0x80000000U;
		}

		std::uint32_t observe(const std::uint32_t entityUid,
			const std::uint32_t inventoryKey, const int x, const int y,
			const bool partyDropped)
		{
			auto found = records_.find(entityUid);
			if ( found == records_.end() )
			{
				std::uint32_t markerId = entityUid;
				if ( partyDropped )
				{
					markerId = takeInventoryMarker(inventoryKey);
					if ( markerId == 0 )
					{
						markerId = entityUid != 0 ? entityUid : allocateMarkerId();
					}
				}
				Record record;
				record.marker = {markerId, entityUid, inventoryKey, x, y};
				record.partyDropped = partyDropped;
				records_[entityUid] = record;
				return markerId;
			}
			found->second.marker.inventoryKey = inventoryKey;
			found->second.marker.x = x;
			found->second.marker.y = y;
			found->second.partyDropped = found->second.partyDropped || partyDropped;
			return found->second.marker.markerId;
		}

		void applyDrop(const std::uint32_t markerId,
			const std::uint32_t entityUid, const std::uint32_t inventoryKey,
			const int x, const int y)
		{
			if ( markerId == 0 || entityUid == 0 )
			{
				return;
			}
			for ( auto record = records_.begin(); record != records_.end(); )
			{
				if ( record->second.marker.markerId == markerId
					&& record->first != entityUid )
				{
					record = records_.erase(record);
				}
				else
				{
					++record;
				}
			}
			records_[entityUid] = {
				{markerId, entityUid, inventoryKey, x, y}, true,
			};
		}

		std::uint32_t pickUp(const std::uint32_t entityUid,
			const std::uint32_t inventoryKey)
		{
			const auto found = records_.find(entityUid);
			if ( found == records_.end() )
			{
				return 0;
			}
			const auto markerId = found->second.marker.markerId;
			inventoryMarkers_[inventoryKey].push_back(markerId);
			records_.erase(found);
			return markerId;
		}

		void remove(const std::uint32_t entityUid)
		{
			records_.erase(entityUid);
		}

		bool isPartyDropped(const std::uint32_t entityUid) const
		{
			const auto found = records_.find(entityUid);
			return found != records_.end() && found->second.partyDropped;
		}

		const Record* find(const std::uint32_t entityUid) const
		{
			const auto found = records_.find(entityUid);
			return found == records_.end() ? nullptr : &found->second;
		}

		std::vector<Marker> markers() const
		{
			std::vector<Marker> result;
			for ( const auto& entry : records_ )
			{
				if ( entry.second.partyDropped )
				{
					result.push_back(entry.second.marker);
				}
			}
			std::sort(result.begin(), result.end(), [](const Marker& left,
				const Marker& right) { return left.markerId < right.markerId; });
			return result;
		}

	private:
		std::uint32_t takeInventoryMarker(const std::uint32_t inventoryKey)
		{
			auto found = inventoryMarkers_.find(inventoryKey);
			if ( found == inventoryMarkers_.end() || found->second.empty() )
			{
				return 0;
			}
			const auto markerId = found->second.front();
			found->second.erase(found->second.begin());
			if ( found->second.empty() )
			{
				inventoryMarkers_.erase(found);
			}
			return markerId;
		}

		std::uint32_t allocateMarkerId()
		{
			if ( nextGeneratedMarkerId_ == 0 )
			{
				nextGeneratedMarkerId_ = 0x80000000U;
			}
			return nextGeneratedMarkerId_++;
		}

		std::unordered_map<std::uint32_t, Record> records_;
		std::unordered_map<std::uint32_t, std::vector<std::uint32_t>>
			inventoryMarkers_;
		std::uint32_t nextGeneratedMarkerId_ = 0x80000000U;
	};
}
