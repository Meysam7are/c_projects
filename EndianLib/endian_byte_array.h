#ifndef MZ_ENDIAN_BYTE_ARRAY_HEADER_FILE
#define MZ_ENDIAN_BYTE_ARRAY_HEADER_FILE
#pragma once

/*
*   Simple wrapper around array of bytes to allow for automatic endian conversion
* 
*   Author: Meysam Zare
*   Last Modified: 6/20/24
*/



#include <string>
#include "randomizer.h"
#include "endian_vector.h"


namespace mz {

    namespace endian {

        template <size_t N>
            requires (N > 0)
        class alignas(8) byte_array

        {


        public:


            using value_type = uint8_t;
            using pointer = uint8_t*;
            using const_pointer = uint8_t const*;



            constexpr pointer data() noexcept { return m_bytes; }
            constexpr const_pointer data() const noexcept { return m_bytes; }
            constexpr size_t size() const noexcept { return N; }
            constexpr std::span<value_type, N> span() noexcept { return std::span{ m_bytes }; }
            constexpr std::span<value_type const, N> span() const noexcept { return std::span{ m_bytes }; }

            void generate(mz::randomizer& rengine, bool ReSeed = false) noexcept {
                rengine.randomize(m_bytes, ReSeed);
            }

            uint64_t gen_hash() const noexcept { return gen_hash(0, N); }

            void clear() noexcept { memset(m_bytes, 0, N); }
            void range(int Initial = 0, int Step = 1) noexcept { for (size_t i = 0; i < N; i++) m_bytes[i] = static_cast<uint8_t>(i * Step + Initial); }


            explicit byte_array() noexcept = default;
            explicit byte_array(std::string const& S) noexcept { fill(S); }
            explicit byte_array(std::wstring const& W) noexcept { fill(W); }
            explicit byte_array(mz::randomizer& rengine, bool ReSeed = false) noexcept { generate(rengine, ReSeed); }
            byte_array& operator = (std::string const& S) noexcept { fill(S); return *this; }
            byte_array& operator = (std::wstring const& W) noexcept { fill(W); return *this; }



            void push_back(mz::endian::vector& V) const noexcept { V.push_back(std::span{ m_bytes }); }
            bool pop_back(mz::endian::vector& V) noexcept { return pop_back(V.pop_back_span(N)); }

            bool pop_back(mz::endian::read_buffer Span) noexcept { return Span.pop_back(std::span{ m_bytes }); }
            bool pop_front(mz::endian::read_buffer Span) noexcept { return Span.pop_front(std::span{ m_bytes }); }

            std::string ToString(size_t Count) const noexcept {
                Count = Count < N ? Count : N;
                return std::string{ (char const*)data(), (char const*)data() + Count };
            }

            std::string ToString() const noexcept {
                size_t Count{ 0 };
                while (Count < N && m_bytes[Count]) ++Count;
                return ToString(Count);
            }

            bool empty() const noexcept {
                uint8_t Temp[N]{ 0 };
                memset(Temp, 0, N);
                return memcmp(m_bytes, Temp, N) == 0;
            }


            friend bool operator < (byte_array const& L, byte_array const& R) noexcept { return memcmp(L.m_bytes, R.m_bytes, N) < 0; }
            friend bool operator == (byte_array const& L, byte_array const& R) noexcept { return memcmp(L.m_bytes, R.m_bytes, N) == 0; }

        protected:

            static constexpr uint64_t HashPrime{ 1099511628211ULL };
            static constexpr uint64_t HashInit{ 14695981039346656037ULL };

            constexpr static uint64_t next_hash(uint64_t HashValue, uint64_t NextValue) noexcept {
                HashValue ^= NextValue;
                HashValue *= HashPrime;
                return HashValue;
            }

            uint64_t gen_hash(uint64_t HashValue, uint64_t MaxIndex) const noexcept {
                uint64_t Index{ 0 };
                while (Index != MaxIndex) { HashValue = next_hash(HashValue, m_bytes[Index++]); }
                return HashValue;
            }



            size_t fill_head(std::string const& S) noexcept {
                size_t Index{ 0 };
                for (char c : S) {
                    if (c) {
                        m_bytes[Index] = c;
                        if (++Index == N) break;
                    }
                }
                return Index;
            }

            size_t fill_head(std::wstring const& W) noexcept {
                size_t Index{ 0 };
                for (wchar_t w : W) {
                    if constexpr (mz::endian::endian_mismatch) {
                        w = mz::endian::swap_bytes(w);
                    }
                    if (uint8_t x = (w & 0xff); x) {
                        m_bytes[Index] = x;
                        if (++Index == N) break;
                    }
                    if (uint8_t x = (w >> 8); x) {
                        m_bytes[Index] = x;
                        if (++Index == N) break;
                    }
                }
                return Index;
            }


            uint64_t fill_tail(uint64_t Index) noexcept {
                if (!Index) {
                    memset(m_bytes, 0, N);
                    return 0;
                }
                uint64_t HashValue{ gen_hash(0, Index) };

                if (Index < N) {
                    m_bytes[Index++] = 0;
                }

                while (Index != N) {
                    HashValue = next_hash(HashValue, m_bytes[Index % N]);
                    m_bytes[Index++] = static_cast<uint8_t>(HashValue);
                }
                return HashValue;
            }


            uint64_t fill(std::string const& S) noexcept { memset(m_bytes, 0, N); return fill_tail(fill_head(S)); }
            uint64_t fill(std::wstring const& W) noexcept { memset(m_bytes, 0, N); return fill_tail(fill_head(W)); }

            uint8_t m_bytes[N]{ 0 };

        };


    }




}



#endif
