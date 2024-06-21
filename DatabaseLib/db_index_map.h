#ifndef DB_INDEX_MAP_HEADER_FILE
#define DB_INDEX_MAP_HEADER_FILE
#pragma once

#include <map>
#include "time_conversions.h"
#include "db_concepts.h"

namespace mz {
	namespace db {


		template <mz::db::KeyType primary_key>
		class db_index_map
		{
		public:

			static constexpr bool monotone{ false };


			using key_type = primary_key;
			using value_type = int64_t;
			using keyval_ref = std::pair<key_type&, value_type&>;

			using indexer = std::map<key_type, value_type>;
			using iterator = indexer::iterator;
			using const_iterator = indexer::const_iterator;
			using insert_return_type = std::pair<iterator, bool>;

			db_index_map() noexcept = default;



			iterator lower_bound(key_type Key) noexcept { return Map.lower_bound(Key.lower()); }

			iterator upper_bound(key_type Key) noexcept { return Map.upper_bound(Key.next()); }

			iterator find(key_type Key) noexcept {
				auto it = lower_bound(Key);
				if (it != end() && it->first == Key) {
					return it;
				}
				else {
					return end();
				}
			}

			iterator select(key_type Key, value_type& Val)
			{
				auto it = find(Key);
				Val = it != end() ? it->second : -1;
				return it;
			}

			iterator select(keyval_ref KV)
			{
				auto it = find(KV.first);
				if (it != end()) {
					KV.first = it->first;
					KV.second = it->second;
				}
				else {
					KV.first.erase();
					KV.second = -1;
				}
				return it;
			}



			insert_return_type insert(key_type Key, value_type Val) noexcept
			{
				if (LastValue + 1 != Val)
				{
					return insert_return_type{ upper_bound(Key), false };
				}

				auto it = lower_bound(Key); // (Key <= *it )
				if (it != end() && it->first == Key)
				{
					return insert_return_type{ it, false };
				}
				else
				{
					it = Map.insert(it, { Key, Val });
					if (it->second == Val) {
						LastValue = Val;
						return insert_return_type{ it, true };
					}
					else {
						return insert_return_type{ it, false };
					}
				}
			}

			bool pop(iterator pos) noexcept
			{
				bool is_back = pos->second == LastValue;
				Map.erase(pos);
				if (is_back) {
					--LastValue;
					return true;
				}
				else {
					return false;
				}
			}

			iterator erase(iterator pos) noexcept
			{
				if (pos->second == LastValue)
				{
					--LastValue;
				}
				return Map.erase(pos);
			}




			indexer Map{};
			value_type LastValue{ -1 };

			constexpr void clear() noexcept { Map.clear(); LastValue = -1; }
			constexpr void reserve(size_t Count) noexcept { }
			constexpr iterator end() noexcept { return Map.end(); }
			constexpr iterator begin() noexcept { return Map.begin(); }

			constexpr const_iterator end() const noexcept { return Map.end(); }
			constexpr const_iterator begin() const noexcept { return Map.begin(); }
			constexpr size_t size() const noexcept { return Map.size(); }

			const_iterator find(key_type Key) const noexcept { return const_cast<db_index_map*>(this)->find(Key); }
			const_iterator lower_bound(key_type Key) const noexcept { return const_cast<db_index_map*>(this)->lower_bound(Key); }
			const_iterator upper_bound(key_type Key) const noexcept { return const_cast<db_index_map*>(this)->upper_bound(Key); }
			const_iterator select(key_type Key, value_type& Val) const noexcept { return const_cast<db_index_map*>(this)->select(Key, Val); }


		};

	}
};


#endif
