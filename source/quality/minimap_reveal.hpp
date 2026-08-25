#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "minimap_items.hpp"

namespace quality::minimap::reveal
{
	constexpr bool acceptHostRequest(const bool connectedClient,
		const bool sameFloor, const std::uint32_t requestGeneration,
		const std::uint32_t readyGeneration)
	{
		return connectedClient && sameFloor && requestGeneration != 0
			&& requestGeneration == readyGeneration;
	}

	constexpr bool acceptClientRefresh(const bool fromHost,
		const bool sameFloor, const std::uint32_t targetGeneration,
		const std::uint32_t localFloorGeneration)
	{
		return fromHost && sameFloor && targetGeneration != 0
			&& targetGeneration == localFloorGeneration;
	}

	enum class Kind : std::uint8_t
	{
		Green,
		ShinyYellow,
		Exit,
		Workbench,
		Cauldron,
	};

	enum class CandidateKind : std::uint8_t
	{
		None,
		Grave,
		Fountain,
		Sink,
		BreakableContainer,
		Item,
		Gold,
		Exit,
		Workbench,
		Cauldron,
	};

	struct Candidate
	{
		std::uint32_t uid = 0;
		int x = 0;
		int y = 0;
		CandidateKind kind = CandidateKind::None;
		bool available = true;
		int itemType = -1;
		bool identified = true;
		bool hasLoot = false;
		bool lootBag = false;
	};

	struct Marker
	{
		std::uint32_t uid = 0;
		int x = 0;
		int y = 0;
		Kind kind = Kind::Green;
	};

	constexpr bool isInteractable(const CandidateKind kind)
	{
		return kind == CandidateKind::Grave || kind == CandidateKind::Fountain
			|| kind == CandidateKind::Sink;
	}

	constexpr bool eligibleGroundGold(const int amount,
		const std::uint32_t containerUid)
	{
		return amount > 0 && containerUid == 0;
	}

	constexpr bool eligible(const Candidate& candidate, const bool used)
	{
		if ( candidate.uid == 0 || !candidate.available )
		{
			return false;
		}
		switch ( candidate.kind )
		{
			case CandidateKind::Grave:
			case CandidateKind::Fountain:
			case CandidateKind::Sink:
				return !used;
			case CandidateKind::BreakableContainer:
				return candidate.hasLoot;
			case CandidateKind::Item:
			{
				const auto appearance = quality::minimap::items::appearance(
					candidate.itemType, candidate.identified, candidate.lootBag);
				return appearance == quality::minimap::items::Appearance::Green
					|| appearance
						== quality::minimap::items::Appearance::ShinyYellow;
			}
			case CandidateKind::Gold:
				return true;
			case CandidateKind::Exit:
			case CandidateKind::Workbench:
			case CandidateKind::Cauldron:
				return true;
			default:
				return false;
		}
	}

	constexpr bool immediateBlue(const Candidate& candidate)
	{
		return candidate.kind == CandidateKind::Item
			&& quality::minimap::items::appearance(candidate.itemType,
				candidate.identified, candidate.lootBag)
				== quality::minimap::items::Appearance::Blue;
	}

	constexpr Kind markerKind(const Candidate& candidate)
	{
		switch ( candidate.kind )
		{
			case CandidateKind::Grave:
			case CandidateKind::Fountain:
			case CandidateKind::Sink:
				return Kind::ShinyYellow;
			case CandidateKind::Item:
				return quality::minimap::items::isOrb(candidate.itemType)
					? Kind::ShinyYellow : Kind::Green;
			case CandidateKind::Exit: return Kind::Exit;
			case CandidateKind::Workbench: return Kind::Workbench;
			case CandidateKind::Cauldron: return Kind::Cauldron;
			default: return Kind::Green;
		}
	}

	class State
	{
	public:
		void reset()
		{
			markers_.clear();
			used_.clear();
			initialUses_.clear();
			active_ = false;
			lastTooltipTick_ = 0;
			tooltipSeen_ = false;
		}

		void rememberInitialUses(const std::uint32_t uid, const int uses)
		{
			initialUses_.emplace(uid, uses);
		}

		void observeUses(const std::uint32_t uid, const int uses)
		{
			const auto initial = initialUses_.find(uid);
			if ( initial == initialUses_.end() )
			{
				initialUses_.emplace(uid, uses);
			}
			else if ( uses < initial->second )
			{
				markUsed(uid);
			}
		}

		void markUsed(const std::uint32_t uid)
		{
			used_.insert(uid);
			markers_.erase(uid);
		}

		bool used(const std::uint32_t uid) const
		{
			return used_.count(uid) != 0;
		}

		bool tooltipEdge(const std::uint32_t tick)
		{
			constexpr std::uint32_t viewingGapTicks = 10;
			const bool edge = !tooltipSeen_
				|| tick > lastTooltipTick_ + viewingGapTicks;
			lastTooltipTick_ = tick;
			tooltipSeen_ = true;
			return edge;
		}

		void refresh(const std::vector<Candidate>& candidates)
		{
			std::unordered_map<std::uint32_t, Marker> refreshed;
			for ( const auto& candidate : candidates )
			{
				if ( !eligible(candidate, used(candidate.uid)) )
				{
					continue;
				}
				refreshed[candidate.uid] = {
					candidate.uid, candidate.x, candidate.y,
					markerKind(candidate),
				};
			}
			markers_ = std::move(refreshed);
			active_ = true;
		}

		void observeLive(const std::vector<Candidate>& candidates)
		{
			std::unordered_set<std::uint32_t> live;
			for ( const auto& candidate : candidates )
			{
				live.insert(candidate.uid);
				auto marker = markers_.find(candidate.uid);
				if ( !eligible(candidate, used(candidate.uid)) )
				{
					if ( marker != markers_.end() )
					{
						markers_.erase(marker);
					}
					continue;
				}
				if ( marker == markers_.end() )
				{
					if ( active_ )
					{
						markers_[candidate.uid] = {
							candidate.uid, candidate.x, candidate.y,
							markerKind(candidate),
						};
					}
					continue;
				}
				marker->second.x = candidate.x;
				marker->second.y = candidate.y;
				marker->second.kind = markerKind(candidate);
			}
			for ( auto marker = markers_.begin(); marker != markers_.end(); )
			{
				if ( !live.count(marker->first) )
				{
					marker = markers_.erase(marker);
				}
				else
				{
					++marker;
				}
			}
		}

		bool contains(const std::uint32_t uid) const
		{
			return markers_.count(uid) != 0;
		}

		bool rebind(const std::uint32_t oldUid, const std::uint32_t newUid,
			const int x, const int y)
		{
			const auto found = markers_.find(oldUid);
			if ( found == markers_.end() || newUid == 0 )
			{
				return false;
			}
			Marker replacement = found->second;
			replacement.uid = newUid;
			replacement.x = x;
			replacement.y = y;
			markers_.erase(found);
			markers_[newUid] = replacement;
			return true;
		}

		bool active() const { return active_; }

		std::vector<Marker> markers() const
		{
			std::vector<Marker> result;
			result.reserve(markers_.size());
			for ( const auto& entry : markers_ )
			{
				result.push_back(entry.second);
			}
			std::sort(result.begin(), result.end(), [](const Marker& left,
				const Marker& right) { return left.uid < right.uid; });
			return result;
		}

	private:
		std::unordered_map<std::uint32_t, Marker> markers_;
		std::unordered_set<std::uint32_t> used_;
		std::unordered_map<std::uint32_t, int> initialUses_;
		bool active_ = false;
		std::uint32_t lastTooltipTick_ = 0;
		bool tooltipSeen_ = false;
	};
}
