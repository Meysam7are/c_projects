#ifndef MZ_CRYPT_BLOW_KEY_HEADER_FILE
#define MZ_CRYPT_BLOW_KEY_HEADER_FILE
#pragma once

/*
*   Structure implementing the Key operations of the Blow Fish algorithm
* 
*   Author: Meysam Zare
*   Last Modified: 6/20/24
*/



#include <cstdint>
#include "endian_read_buffer.h"
#include "endian_byte_array.h"
#include "blow_buffers.h"


namespace mz {
    namespace crypt {


        struct blow_key {

            inline static constexpr size_t NumKeys{ 20 };

            uint32_t Id{ 0 };
            uint32_t ItrIndex{ 0 };
            uint32_t Keys[NumKeys]{
                0x3A39CE37, 0xD3FAF5CF, 0xABC27737, 0x5AC52D1B, 0x5CB0679E, 0x4FA33742, 0xD3822740, 0x99BC9BBE,
                0xD5118E9D, 0xBF0F7315, 0xD62D1C7E, 0xC700C47B, 0xB78C1B6B, 0x21A19045, 0xB26EB1BE, 0x6A366EB4,
                0x5748AB2F, 0xBC946E79, 0xC6A376D2, 0x6549C2C8 };// , 0x530FF8EE, 0x468DDE7D, 0xD5730A1D, 0x4CD04DC6,
//                0x2939BBDB, 0xA9BA4650, 0xAC9526E8, 0xBE5EE304, 0xA1FAD5F0, 0x6A2D519A, 0x63EF8CE2, 0x9A86EE22 };


            constexpr size_t size() const noexcept { return NumKeys; }
            constexpr uint32_t operator[](size_t index) const noexcept { return Keys[index]; }
            constexpr uint32_t& operator[](size_t index) noexcept { return Keys[index]; }


            template <size_t N> requires (N > 0)
            void xor_with(mz::endian::byte_array<N> const& Salt) noexcept {
                const_pair_stream Stream{ Salt };
                for (uint32_t i = 0; i < size(); i += 2) {
                    pair32 NativePair = Stream.next_native_pair(ItrIndex);
                    Keys[i] ^= NativePair[0];
                    Keys[i + 1] ^= NativePair[1];
                }
            }


            bool pop_back(mz::endian::read_buffer PS) noexcept { return PS.pop_back(std::span{ Keys }); }
            void pop_front(mz::endian::read_buffer PS) noexcept { PS.pop_front(std::span{ Keys }); }


            friend bool operator == (blow_key const& L, blow_key const& R) noexcept { 
                return L.ItrIndex == R.ItrIndex && memcmp(L.Keys, R.Keys, sizeof(L.Keys)) == 0; 
            }


        };

    }
}



#endif
