#pragma once

#include <string>
#include <concepts>

/*
*	A simple 64 character encoder implementation
* 
*	Author: Meysam Zare
*	Last Modified: 6/20/24
*/


namespace mz {

	class encoder64 {

		static constexpr char Alphabet[64]{
		  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		  'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
		};

		static constexpr char C(auto x) noexcept { return Alphabet[x & 63]; }


	public:

		template <std::integral T>
		constexpr static std::string ToString(T x) noexcept 
		{
			if constexpr (sizeof(T) == 1)
			{
				return std::string{ C(x >> 6), C(x) };
			}
			else if constexpr (sizeof(T) == 2)
			{
				return std::string{ C(x >> 12), C(x >> 6), C(x) };
			}
			else if constexpr (sizeof(T) == 4)
			{
				return std::string{ C(x >> 30), C(x >> 24), C(x >> 18), C(x >> 12), C(x >> 6), C(x) };
			}
			else
			{
				return std::string{ C(x >> 60), C(x >> 54), C(x >> 48), C(x >> 42), C(x >> 36), C(x >> 30), C(x >> 24), C(x >> 18), C(x >> 12), C(x >> 6), C(x) };
			}
		}

		template <std::integral T>
		constexpr std::string operator()(T x) const noexcept { return ToString(x); }


	};




}