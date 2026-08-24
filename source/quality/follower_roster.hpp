#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace quality::follower_roster
{
	constexpr int maximumPlayers = 4;

	struct Entry
	{
		std::uint32_t uid = 0;
		int owner = -1;
		int level = 0;
		int hp = 0;
		int maxHp = 0;
		int type = 0;
		int model = 0;
		int order = 0;
		std::string name;

		bool operator==(const Entry& other) const
		{
			return uid == other.uid && owner == other.owner
				&& level == other.level && hp == other.hp && maxHp == other.maxHp
				&& type == other.type && model == other.model
				&& order == other.order && name == other.name;
		}
	};

	constexpr bool eligible(const Entry& entry)
	{
		return entry.uid != 0 && entry.owner >= 0
			&& entry.owner < maximumPlayers && entry.maxHp > 0 && entry.hp > 0;
	}

	inline std::string firstUtf8Character(const std::string& text)
	{
		if ( text.empty() )
		{
			return {};
		}
		const auto first = static_cast<unsigned char>(text.front());
		std::size_t length = 1;
		if ( (first & 0xE0U) == 0xC0U ) { length = 2; }
		else if ( (first & 0xF0U) == 0xE0U ) { length = 3; }
		else if ( (first & 0xF8U) == 0xF0U ) { length = 4; }
		if ( length > text.size() )
		{
			length = 1;
		}
		return text.substr(0, length);
	}

	inline std::string displayName(const std::string& ownerName,
		const std::string& followerName, const int owner)
	{
		std::string initial = firstUtf8Character(ownerName);
		if ( initial.empty() )
		{
			initial = "P" + std::to_string(owner + 1);
		}
		return initial + "'s " + followerName;
	}

	inline std::vector<Entry> visibleRemoteEntries(
		const std::unordered_map<std::uint32_t, Entry>& entries,
		const int viewer)
	{
		std::vector<Entry> result;
		for ( const auto& pair : entries )
		{
			if ( pair.second.owner != viewer && eligible(pair.second) )
			{
				result.push_back(pair.second);
			}
		}
		std::sort(result.begin(), result.end(), [](const Entry& left,
			const Entry& right) {
			if ( left.owner != right.owner ) { return left.owner < right.owner; }
			if ( left.order != right.order ) { return left.order < right.order; }
			return left.uid < right.uid;
		});
		return result;
	}

	class State
	{
	public:
		bool upsert(const Entry& entry)
		{
			if ( !eligible(entry) )
			{
				return erase(entry.uid);
			}
			const auto found = entries_.find(entry.uid);
			if ( found != entries_.end() && found->second == entry )
			{
				return false;
			}
			entries_[entry.uid] = entry;
			return true;
		}

		bool erase(const std::uint32_t uid)
		{
			return entries_.erase(uid) != 0;
		}

		void reset() { entries_.clear(); }
		const auto& entries() const { return entries_; }

	private:
		std::unordered_map<std::uint32_t, Entry> entries_;
	};
}
