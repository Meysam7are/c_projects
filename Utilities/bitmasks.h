#ifndef MZ_UTILITIES_BITMASK_HEADER_FILE
#define MZ_UTILITIES_BITMASK_HEADER_FILE
#pragma once

#include <cstdint>
#include <type_traits>
#include <immintrin.h>

/*
*	Finding smallest mask for numerical literals
* 
*	Author: Meysam Zare
*	Last Modified: 6/20/24
*/


namespace mz {

	inline uint32_t bit_mask32_safe(uint32_t UpperBound) noexcept {
		uint32_t cnt{ 0 };
		while (UpperBound) { ++cnt; UpperBound >>= 1; }
		return (1ull << cnt) - 1ul;
	}

	inline uint64_t bit_mask64_safe(uint64_t UpperBound) noexcept {
		uint64_t cnt{ 64 };
		while (UpperBound) { --cnt; UpperBound >>= 1; }
		return cnt ^ 64 ? 0xffffffffffffffffull >> cnt : 0;
	}


	inline uint32_t bit_mask32(uint32_t UpperBound) noexcept {
#if defined(_MSC_VER)
		return 0xffffffffull >> __lzcnt(UpperBound);
#elif (__GNUC__) || defined(__GNUG__)
		return 0xffffffffull >> __builtin_clz(UpperBound);
#else
		return bit_mask32_safe(UpperBound);
#endif
	}

	inline uint64_t bit_mask64(uint64_t UpperBound) noexcept {
#if defined(_MSC_VER)
		return UpperBound ? 0xffffffffffffffff >> __lzcnt64(UpperBound) : 0ull;
#elif (__GNUC__) || defined(__GNUG__)
		return UpperBound ? 0xffffffffffffffff >> __builtin_clzll(UpperBound) : 0ull;
#else
		return bit_mask64_safe(UpperBound);
#endif

	}


}






#endif
