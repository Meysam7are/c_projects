#ifndef DB_TABLE_ID_HEADER_FILE
#define DB_TABLE_ID_HEADER_FILE
#pragma once


#include "time_conversions.h"

namespace mz {

	namespace db {

		using db_duration = std::chrono::nanoseconds;
		using db_time = mz::time::systime<db_duration>;
		/*
		struct id_flags
		{
			static constexpr int64_t NumFlags{ 4 };
			static constexpr int64_t RESERVED{ 0b0100 };
			static constexpr int64_t ERASED{ 0b1000 };
			static constexpr int64_t NOTFOUND{ 0b1100 };
		};
		*/



		struct row_id
		{
		protected:

			static constexpr int64_t NumFlags{ 4 };
			static constexpr int64_t FlagBits{ 15 };
			static constexpr int64_t Increment{ 1ll << NumFlags };

			static constexpr int64_t StateBits{ 12 };
			static constexpr int64_t ReserveBit{ 8 };

			constexpr void update_id(int64_t Id) noexcept { Key = (Key & FlagBits) | (Id << NumFlags); }
			constexpr void update_flags(int64_t Flags) noexcept { Key = (Key & ~FlagBits) | (Flags & FlagBits); }

		public:

			int64_t Key{ 0 };

			constexpr row_id() noexcept = default;
			explicit constexpr row_id(int64_t Key) noexcept : Key{ Key } {}
			explicit constexpr row_id(db_time Time) noexcept : Key{ Time.tsep | FlagBits } {}
			constexpr row_id& operator = (db_time Time) noexcept { update_id(Time.tsep >> FlagBits); return *this; }

			constexpr int64_t id() const noexcept { return (Key >> NumFlags); }
			constexpr db_time time() const noexcept { return db_time{ Key | FlagBits }; }
			constexpr int64_t flags() const noexcept { return Key & FlagBits; }
			constexpr std::string skey() const noexcept { return mz::encoder64::ToString(id()); }

			std::string string() const noexcept { return std::format("[{}:{}]", mz::encoder64::ToString(id()), mz::encoder64::ToString(flags())); }


			// KeyType requirements
			constexpr void clear() noexcept { Key = 0; }
			constexpr void erase() noexcept { Key &= ~StateBits; }
			constexpr void commit() noexcept { Key |= StateBits; }
			constexpr void reserve() noexcept { Key = (Key & ~StateBits) | ReserveBit; }

			constexpr bool empty() const noexcept { return Key == 0; }
			constexpr bool valid() const noexcept { return id() > 0; }
			constexpr bool erased() const noexcept { return !(Key & StateBits); }
			constexpr bool reserved() const noexcept { return (Key & StateBits) == ReserveBit; }
			constexpr bool committed() const noexcept { return valid() && (Key & StateBits) == StateBits; }


			constexpr row_id lower() const noexcept { return row_id{ Key & ~FlagBits }; }
			constexpr row_id upper() const noexcept { return row_id{ Key | FlagBits }; }
			constexpr row_id prev() const noexcept { return row_id{ lower().Key - 1 }; }
			constexpr row_id next() const noexcept { return row_id{ upper().Key + 1 }; }

			friend constexpr bool operator < (row_id L, row_id R) noexcept { return L.Key < R.Key; }
			friend constexpr bool operator != (row_id L, row_id R) noexcept { return L.Key != R.Key; }
			friend constexpr bool operator == (row_id L, row_id R) noexcept { return L.id() == R.id(); }

		};







		enum class bykey {
			id,
			name,
			index,
		};






	}
}
#endif
