#ifndef MZ_CRYPT_BLOW_STREAM_HEADER_FILE
#define MZ_CRYPT_BLOW_STREAM_HEADER_FILE
#pragma once

/*
*   A modified buffer API to allow encryption of streams of characters of arbitrary byte length
* 
*   Author: Meysam Zare
*   Last Modified: 6/20/24
*/


#include <cstdint>
#include <span>
#include <string>
#include <format>

#include "endian_byte_array.h"



namespace mz {
    namespace crypt {



        struct pair32 {

            uint32_t pair[2]{ 0,0 };

            constexpr uint32_t& operator[](auto i) noexcept { return pair[i]; }

            constexpr pair32() noexcept = default;
            constexpr pair32(uint32_t P0, uint32_t P1) noexcept : pair{ P0, P1 } {}
            constexpr pair32& operator ^= (pair32 rhs) noexcept {
                pair[0] ^= rhs.pair[0];
                pair[1] ^= rhs.pair[1];
                return *this;
            }

        };


        template <size_t N> 
            requires (N > 0)
        class const_pair_stream {

            uint8_t const* m_stream{ nullptr };

            constexpr uint32_t next32(auto& Index) const noexcept { return static_cast<uint32_t>(m_stream[(Index++) % N]); }
            constexpr uint8_t next8(auto& Index) const noexcept { return m_stream[(Index++) % N]; }

        public:

            explicit constexpr const_pair_stream(mz::endian::byte_array<N> const& SARR) noexcept : m_stream{SARR.data()} {}

            constexpr pair32 next_native_pair(auto& Index) const noexcept {

                if constexpr (mz::crypt::server_endian == std::endian::little) {
                    uint32_t P0{ next32(Index) };
                    P0 |= next32(Index) << 8;
                    P0 |= next32(Index) << 16;
                    P0 |= next32(Index) << 24;

                    uint32_t P1{ next32(Index) };
                    P1 |= next32(Index) << 8;
                    P1 |= next32(Index) << 16;
                    P1 |= next32(Index) << 24;
                    return pair32{ P0, P1 };
                }
                else
                {
                    uint32_t P0{ next8(Index) };
                    P0 = (P0 << 8) | next8(Index);
                    P0 = (P0 << 8) | next8(Index);
                    P0 = (P0 << 8) | next8(Index);

                    uint32_t P1{ next8(Index) };
                    P1 = (P1 << 8) | next8(Index);
                    P1 = (P1 << 8) | next8(Index);
                    P1 = (P1 << 8) | next8(Index);
                    return pair32{ P0, P1 };
                }
            }
        };


        class mutable_blow_buffer {

            uint64_t m_size{ 0 };
            uint8_t* m_stream{ nullptr };

            constexpr uint32_t next32(auto& Index) const noexcept { return static_cast<uint32_t>(m_stream[(Index++) % m_size]); }
            constexpr uint8_t next8(auto& Index) const noexcept { return m_stream[(Index++) % m_size]; }



        public:

            template <size_t N>
                requires (N > 0)
            explicit constexpr mutable_blow_buffer(mz::endian::byte_array<N>& SARR) noexcept : m_size{ SARR.size() }, m_stream{SARR.data()} {}
            explicit constexpr mutable_blow_buffer(pair32& P) noexcept : m_size{ 8 }, m_stream{ (uint8_t*)P.pair } {};
            explicit constexpr mutable_blow_buffer(void* Ptr_, size_t Size) noexcept : m_size{ Size }, m_stream{ (uint8_t*)Ptr_ } {};

            template <size_t N>
            explicit constexpr mutable_blow_buffer(std::span<uint8_t, N> SP) noexcept : m_size{ SP.size() }, m_stream{ SP.data() } {}

            constexpr bool has_pair32() const noexcept { return (m_size >> 3); }
            constexpr bool has_pair16() const noexcept { return (m_size >> 2); }
            constexpr bool has_pair8() const noexcept { return (m_size >> 1); }
            constexpr bool has_byte() const noexcept { return m_size; }
            constexpr size_t size() const noexcept { return m_size; }


            constexpr uint8_t* begin() noexcept { return m_stream; }
            constexpr uint8_t* end() noexcept { return m_stream + m_size; }








            pair32 peek_next_pair32() const noexcept { 
                auto Ptr_ = reinterpret_cast<uint32_t const*>(m_stream);
                return pair32{ mz::crypt::stream_to_native(Ptr_[0]), mz::crypt::stream_to_native(Ptr_[1]) };
            }

            pair32 peek_next_pair16() const noexcept {
                auto Ptr_ = reinterpret_cast<uint16_t const*>(m_stream);
                return pair32{ mz::crypt::stream_to_native(Ptr_[0]), mz::crypt::stream_to_native(Ptr_[1]) };
            }

            pair32 peek_next_pair8() const noexcept {
                return pair32{ m_stream[0], m_stream[1] };
            }

            pair32 peek_next_byte() const noexcept {
                return pair32{ uint32_t(m_stream[0] & 0x0f), uint32_t((m_stream[0] >> 4) & 0x0f) };
            }


            void update_next_pair32(pair32 NativePair) noexcept {
                auto Ptr_ = reinterpret_cast<uint32_t*>(m_stream);
                Ptr_[0] = mz::crypt::native_to_stream(NativePair[0]);
                Ptr_[1] = mz::crypt::native_to_stream(NativePair[1]);
                m_size -= 8;
                m_stream += 8;
            }

            void update_next_pair16(pair32 NativePair) noexcept {
                auto Ptr_ = reinterpret_cast<uint16_t*>(m_stream);
                Ptr_[0] = mz::crypt::native_to_stream(uint16_t(NativePair[0]));
                Ptr_[1] = mz::crypt::native_to_stream(uint16_t(NativePair[1]));
                m_size -= 4;
                m_stream += 4;
            }

            void update_next_pair8(pair32 NativePair) noexcept {
                m_stream[0] = uint8_t(NativePair[0]);
                m_stream[1] = uint8_t(NativePair[1]);
                m_size -= 2;
                m_stream += 2;
            }

            void update_next_byte(pair32 NativePair) noexcept {
                *m_stream = uint8_t((NativePair[0] & 0x0f) | (NativePair[1] << 4));
                --m_size;
                ++m_stream;
            }



            std::string ToString() const noexcept {
                std::string result{ " " };
                for (size_t i = 0; i < m_size; i++) { result += std::format("{},", (uint32_t)m_stream[i]); }
                return result;
            }
        };



    }
}






#endif
