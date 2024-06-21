#ifndef DB_INDEX_LIN_HEADER_FILE
#define DB_INDEX_LIN_HEADER_FILE
#pragma once

#include <algorithm>
#include "time_conversions.h"
#include "db_concepts.h"

namespace mz {
	namespace db {


		template <mz::db::KeyType primary_key>
		class db_index_lin
		{
		public:

			static constexpr bool monotone{ true };

			using key_type = primary_key;
			using value_type = int64_t;
			using keyval_ref = std::pair<key_type&, value_type&>;

			using indexer = std::vector<key_type>;
			using iterator = indexer::iterator;
			using const_iterator = indexer::const_iterator;
			using insert_return_type = std::pair<iterator, bool>;

			db_index_lin() noexcept = default;



			iterator lower_bound(key_type Key) noexcept
			{
				return std::lower_bound(
					Rows.begin(), Rows.end(), Key.lower(),
					[](key_type L, key_type R) constexpr noexcept -> bool { return L < R; });
			}

			iterator upper_bound(key_type Key) noexcept { return lower_bound(Key.next()); }

			iterator find(key_type Key) noexcept
			{
				auto it = lower_bound(Key);
				if (it != end() && Key == *it) {
					return it;
				}
				else {
					return end();
				}
			}


			// this function checks if KV.first exists in the map
			// if yes copies the flag updates the value and return it.
			iterator select(keyval_ref KV)
			{
				size_t Index = KV.second;
				if (size_t(Index) < Rows.size() && Rows[Index] == KV.first)
				{
					KV.first = Rows[Index];
					return Rows.begin() + Index;
				}
				else {
					auto it = find(KV.first);
					if (it != end()) {
						KV.first = *it;
						KV.second = static_cast<value_type>(it - begin());
					}
					else {
						KV.first.erase();
						KV.second = -1;
					}
					return it;
				}
			}


			insert_return_type insert(key_type Key, value_type Val) noexcept
			{
				if (Val == size() && (LastKey < Key))
				{
					Rows.push_back(Key);
					LastKey = Key.upper();
					return insert_return_type{ Rows.begin() + Val, true };
				}
				else {
					return insert_return_type{ upper_bound(Key), false };
				}
			}

			iterator erase(iterator pos) noexcept
			{
				if (pos != --end())
				{
					pos->erase();
					return ++pos;
				}
				Rows.pop_back();
				if (!Rows.empty()) {
					LastKey = Rows.back();
				}
				else {
					LastKey.clear();
				}
				return end();
			}

			bool pop(iterator pos) noexcept
			{
				if (pos != --end())
				{
					pos->erase();
					return false;
				}
				Rows.pop_back();
				if (!Rows.empty()) {
					LastKey = Rows.back();
				}
				else {
					LastKey.clear();
				}
				return true;
			}


			indexer Rows{};
			key_type LastKey{};

			constexpr void clear() noexcept { Rows.clear(); }
			constexpr iterator end() noexcept { return Rows.end(); }
			constexpr iterator begin() noexcept { return Rows.begin(); }
			constexpr void reserve(size_t Count) noexcept { Rows.reserve(Count); }

			constexpr bool empty() const noexcept { return Rows.empty(); }
			constexpr size_t size() const noexcept { return Rows.size(); }
			constexpr const_iterator end() const noexcept { return Rows.end(); }
			constexpr const_iterator begin() const noexcept { return Rows.begin(); }

			const_iterator find(key_type Key) const noexcept { return const_cast<db_index_lin*>(this)->find(Key); }
			const_iterator lower_bound(key_type Key) const noexcept { return const_cast<db_index_lin*>(this)->lower_bound(Key); }
			const_iterator upper_bound(key_type Key) const noexcept { return const_cast<db_index_lin*>(this)->upper_bound(Key); }
			const_iterator select(key_type Key, value_type& Val) const noexcept { return const_cast<db_index_lin*>(this)->select(Key, Val); }

		};









	}
};

#endif
