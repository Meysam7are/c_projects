#ifndef DB_CONCEPTS_HEADER_FILE
#define DB_CONCEPTS_HEADER_FILE
#pragma once

#include <concepts>
#include "time_conversions.h"

namespace mz {

	namespace db {

		using db_duration = std::chrono::nanoseconds;
		using db_time = mz::time::systime<db_duration>;

		template <typename T>
		concept TrivialType =
			std::is_trivially_copyable_v<T> &&
			std::is_nothrow_default_constructible_v<T>;


		template<TrivialType T>
		struct indexed_type
		{
			int64_t Index;
			T Entry;
		};


		template <typename K>
		concept KeyType =
			TrivialType<K> &&
			std::equality_comparable<K> &&
			requires (K k, K const ck)
		{

			{ k.erase() } -> std::same_as<void>;
			{ k.commit() } -> std::same_as<void>;
			{ k.reserve() } -> std::same_as<void>;

			{ ck.valid() } -> std::same_as<bool>;
			{ ck.erased() } -> std::same_as<bool>;
			{ ck.reserved() } -> std::same_as<bool>;

			{ ck.lower() } -> std::same_as<K>;
			{ ck.upper() } -> std::same_as<K>;
			{ ck.prev() } -> std::same_as<K>;
			{ ck.next() } -> std::same_as<K>;

			{ std::less<K>{}(K(), K()) } -> std::same_as<bool>;

			(ck < ck.lower()) == false;
			(ck.upper() < ck) == false;
			(ck.prev() < ck.lower()) == true;
			(ck.upper() < ck.next()) == true;
			(ck.lower() < ck.upper()) == true;
			(ck.lower() == ck.upper()) == true;
			(ck.lower() != ck.upper()) == true;
		};


		template <typename E>
		concept EntryType =
			TrivialType<E> &&
			KeyType<typename E::key_type> &&
			requires(E e, E const ce)
		{
			{ e.pk() } -> std::same_as<typename E::key_type&>;
			{ ce.pk() } -> std::same_as<typename E::key_type>;
			{ e.erase() } -> std::same_as<void>;
			{ ce.valid() } -> std::same_as<bool>;
			{ ce.erased() } -> std::same_as<bool>;
		};


	}
};


#endif