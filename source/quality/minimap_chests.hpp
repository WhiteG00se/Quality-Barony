#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace quality::minimap::chests
{
	constexpr bool eligibleOrdinary(const bool chest, const int voidState)
	{
		return chest && voidState == 0;
	}

	struct Observation
	{
		std::uint32_t uid = 0;
		int x = 0;
		int y = 0;
		int itemCount = 0;
		bool open = false;
	};

	struct Update
	{
		std::uint32_t uid = 0;
		int x = 0;
		int y = 0;
		bool interacted = false;
		bool nonempty = false;
	};

	using Marker = Update;

	class State
	{
	public:
		void reset()
		{
			records_.clear();
		}

		std::vector<Update> observeAuthoritative(
			const std::vector<Observation>& observations)
		{
			std::vector<Update> updates;
			std::unordered_set<std::uint32_t> live;
			for ( const auto& observation : observations )
			{
				if ( observation.uid == 0 )
				{
					continue;
				}
				live.insert(observation.uid);
				auto found = records_.find(observation.uid);
				if ( found == records_.end() )
				{
					records_[observation.uid] = {
						observation.x, observation.y, observation.itemCount,
						observation.open, false, false,
					};
					continue;
				}

				auto& record = found->second;
				const bool countChanged = observation.itemCount != record.itemCount;
				const bool transfer = countChanged && (observation.open || record.open);
				const bool wasInteracted = record.interacted;
				const bool wasNonempty = record.nonempty;
				if ( transfer )
				{
					record.interacted = true;
				}
				if ( record.interacted )
				{
					record.nonempty = observation.itemCount > 0;
				}
				record.x = observation.x;
				record.y = observation.y;
				record.itemCount = observation.itemCount;
				record.open = observation.open;
				if ( record.interacted
					&& (!wasInteracted || record.nonempty != wasNonempty) )
				{
					updates.push_back(makeUpdate(observation.uid, record));
				}
			}
			prune(live);
			return updates;
		}

		void observeLive(const std::vector<Observation>& observations)
		{
			std::unordered_set<std::uint32_t> live;
			for ( const auto& observation : observations )
			{
				if ( observation.uid == 0 )
				{
					continue;
				}
				live.insert(observation.uid);
				const auto found = records_.find(observation.uid);
				if ( found != records_.end() )
				{
					found->second.x = observation.x;
					found->second.y = observation.y;
				}
			}
			prune(live);
		}

		void apply(const Update& update)
		{
			if ( update.uid == 0 || !update.interacted )
			{
				return;
			}
			auto& record = records_[update.uid];
			record.x = update.x;
			record.y = update.y;
			record.interacted = true;
			record.nonempty = update.nonempty;
		}

		std::vector<Update> updates() const
		{
			std::vector<Update> result;
			for ( const auto& entry : records_ )
			{
				if ( entry.second.interacted )
				{
					result.push_back(makeUpdate(entry.first, entry.second));
				}
			}
			sort(result);
			return result;
		}

		std::vector<Marker> markers() const
		{
			auto result = updates();
			result.erase(std::remove_if(result.begin(), result.end(),
				[](const Marker& marker) { return !marker.nonempty; }), result.end());
			return result;
		}

		bool interacted(const std::uint32_t uid) const
		{
			const auto found = records_.find(uid);
			return found != records_.end() && found->second.interacted;
		}

	private:
		struct Record
		{
			int x = 0;
			int y = 0;
			int itemCount = 0;
			bool open = false;
			bool interacted = false;
			bool nonempty = false;
		};

		static Update makeUpdate(const std::uint32_t uid, const Record& record)
		{
			return {uid, record.x, record.y, record.interacted, record.nonempty};
		}

		static void sort(std::vector<Update>& updates)
		{
			std::sort(updates.begin(), updates.end(), [](const Update& left,
				const Update& right) { return left.uid < right.uid; });
		}

		void prune(const std::unordered_set<std::uint32_t>& live)
		{
			for ( auto record = records_.begin(); record != records_.end(); )
			{
				if ( !live.count(record->first) )
				{
					record = records_.erase(record);
				}
				else
				{
					++record;
				}
			}
		}

		std::unordered_map<std::uint32_t, Record> records_;
	};
}
