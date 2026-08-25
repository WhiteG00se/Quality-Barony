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

	enum class Contents : std::uint8_t
	{
		Empty,
		HasUnidentified,
		IdentifiedOnly,
	};

	constexpr Contents classify(const int unidentifiedCount,
		const int identifiedCount)
	{
		return unidentifiedCount > 0 ? Contents::HasUnidentified
			: identifiedCount > 0 ? Contents::IdentifiedOnly : Contents::Empty;
	}

	constexpr bool validContentsValue(const std::uint32_t value)
	{
		return value <= static_cast<std::uint32_t>(Contents::IdentifiedOnly);
	}

	struct Observation
	{
		std::uint32_t uid = 0;
		int x = 0;
		int y = 0;
		int unidentifiedCount = 0;
		int identifiedCount = 0;
	};

	struct Update
	{
		std::uint32_t uid = 0;
		int x = 0;
		int y = 0;
		Contents contents = Contents::Empty;
	};

	using Marker = Update;

	class State
	{
	public:
		void reset() { records_.clear(); }

		std::vector<Update> observeAuthoritative(
			const std::vector<Observation>& observations)
		{
			std::vector<Update> updates;
			std::unordered_set<std::uint32_t> live;
			for ( const auto& observation : observations )
			{
				if ( observation.uid == 0 ) { continue; }
				live.insert(observation.uid);
				const Update update {observation.uid, observation.x, observation.y,
					classify(observation.unidentifiedCount,
						observation.identifiedCount)};
				auto found = records_.find(observation.uid);
				if ( found == records_.end() || found->second.contents != update.contents )
				{
					updates.push_back(update);
				}
				records_[observation.uid] = update;
			}
			for ( auto record = records_.begin(); record != records_.end(); )
			{
				if ( live.count(record->first) ) { ++record; continue; }
				if ( record->second.contents != Contents::Empty )
				{
					updates.push_back({record->first, record->second.x,
						record->second.y, Contents::Empty});
				}
				record = records_.erase(record);
			}
			sort(updates);
			return updates;
		}

		void observeLive(const std::vector<Observation>& observations)
		{
			std::unordered_set<std::uint32_t> live;
			for ( const auto& observation : observations )
			{
				if ( observation.uid == 0 ) { continue; }
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
			if ( update.uid != 0 ) { records_[update.uid] = update; }
		}

		std::vector<Update> snapshots() const
		{
			std::vector<Update> result;
			for ( const auto& entry : records_ )
			{
				if ( entry.second.contents != Contents::Empty )
				{
					result.push_back(entry.second);
				}
			}
			sort(result);
			return result;
		}

		std::vector<Marker> markers(const Contents contents) const
		{
			auto result = snapshots();
			result.erase(std::remove_if(result.begin(), result.end(),
				[contents](const Marker& marker)
				{
					return marker.contents != contents;
				}), result.end());
			return result;
		}

		Contents contents(const std::uint32_t uid) const
		{
			const auto found = records_.find(uid);
			return found == records_.end() ? Contents::Empty
				: found->second.contents;
		}

	private:
		static void sort(std::vector<Update>& updates)
		{
			std::sort(updates.begin(), updates.end(), [](const Update& left,
				const Update& right) { return left.uid < right.uid; });
		}

		void prune(const std::unordered_set<std::uint32_t>& live)
		{
			for ( auto record = records_.begin(); record != records_.end(); )
			{
				if ( !live.count(record->first) ) { record = records_.erase(record); }
				else { ++record; }
			}
		}

		std::unordered_map<std::uint32_t, Update> records_;
	};
}
