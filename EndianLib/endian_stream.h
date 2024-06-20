#ifndef MZ_ENDIAN_STREAM_BUFFER_HEADER_FILE
#define MZ_ENDIAN_STREAM_BUFFER_HEADER_FILE
#pragma once

/*
*   Simple stream and buffer API to allow for reading and writing literal types with automatic endian conversion
*
*   Author: Meysam Zare
*   Last Modified: 6/20/24
*/


#include <cstdint>
#include <concepts>
#include <type_traits>
#include <span>

#include "endian_conversions.h"
#include "endian_concepts.h"


namespace mz {
    namespace endian {



        class buffer;



        class stream {

        public:

            using value_type = uint8_t;
            using pointer = uint8_t*;
            using const_pointer = uint8_t const*;


            struct block {
                uint32_t Offset{ 0 };
                int32_t Size{ -1 };
                constexpr size_t length() const noexcept { return Size + 8ull; }
                constexpr bool valid() const noexcept { return Size >= 0; }
                constexpr block() noexcept = default;
                constexpr block(size_t Offset, int32_t Size) noexcept : Offset{ static_cast<uint32_t>(Offset) }, Size{ Size } {}
            };


            constexpr stream(pointer begin, pointer end) noexcept : m_begin{ begin }, m_end{ end } {}
            constexpr stream(pointer begin, size_t Size) noexcept : stream{ begin, begin + Size } {}
            constexpr stream(void* begin, size_t Size) noexcept : stream{ (pointer)begin, Size } {}

            constexpr stream(buffer) noexcept;


            constexpr pointer end() noexcept { return m_end; }
            constexpr pointer data() noexcept { return m_begin; }
            constexpr pointer begin() noexcept { return m_begin; }
            constexpr const_pointer end() const noexcept { return m_end; }
            constexpr const_pointer data() const noexcept { return m_begin; }
            constexpr const_pointer begin() const noexcept { return m_begin; }
            constexpr bool empty() const noexcept { return m_begin == m_end; }
            constexpr bool error() const noexcept { return m_end < m_begin || !m_begin; }
            constexpr size_t size() const noexcept { return m_end - m_begin; }


            template <SwapType T>
            static constexpr size_t lengthof(T const&) noexcept { return sizeof(T); }

            template <SwapType T, size_t N>
            static constexpr size_t lengthof(std::span<T, N> const& sp) noexcept { return sizeof(T) * sp.size(); }

            static constexpr size_t lengthof(std::string const& str) noexcept { return str.size() + 8ull; }
            static constexpr size_t lengthof(std::wstring const& str) noexcept { return str.size() * 2ull + 8ull; }



            constexpr void skip(size_t L) noexcept
            {
                auto Ptr{ m_begin + L };
                m_begin = Ptr <= m_end ? Ptr : m_end;
            }

            /*
                SWAP TYPES
            */

            template <SwapTypeNonConst T>
            void unsafe_read(T& t) noexcept 
            {
                copy(t, m_begin); 
                m_begin += sizeof(T);
            }

            template <SwapTypeNonConst T>
            T unsafe_read() noexcept 
            { 
                T t{}; 
                unsafe_read(t); 
                return t; 
            }

            template <SwapTypeNonConst T>
            bool read(T& t) noexcept 
            {
                if (m_begin + sizeof(T) <= m_end) 
                {
                    unsafe_read(t);
                    return false;
                }
                return true;
            };

            template <SwapType T>
            void unsafe_write(T const& t) noexcept
            {
                copy(m_begin, t);
                m_begin += sizeof(T);
            }

            template <SwapType T>
            bool write(T const& t) noexcept
            {
                if (m_begin + sizeof(T) <= m_end)
                {
                    unsafe_write(t);
                    return false;
                }
                return true;
            };


            /*
                SPAN OF SWAP TYPES
            */


            template <SwapTypeNonConst T, size_t N>
            void unsafe_read(std::span<T, N> SP) noexcept {
                copy(SP, m_begin);
                m_begin += sizeof(T) * SP.size();
            }

            template <SwapTypeNonConst T, size_t N>
            bool read(std::span<T, N> SP) noexcept {
                if (m_begin + sizeof(T) * SP.size() <= m_end)
                {
                    unsafe_read(SP);
                    return false;
                }
                return true;
            }


            template <SwapType T, size_t N>
            void unsafe_write(std::span<T, N> SP) noexcept {
                copy(m_begin, SP);
                m_begin += sizeof(T) * SP.size();
            }

            template <SwapType T, size_t N>
            bool write(std::span<T, N> SP) noexcept {
                if (m_begin + sizeof(T) * SP.size() <= m_end)
                {
                    unsafe_write(SP);
                    return false;
                }
                return true;
            }


            /*
                STRING TYPES
            */

            template <SwapTypeNonConst CharT>
            bool read(std::basic_string<CharT>& STR) noexcept 
            {
                auto old_begin{ m_begin };
                if (m_begin + 8ull <= m_end) 
                {
                    int32_t Length{ 0 };
                    unsafe_read(Length);
                    if (m_begin + 4ull + Length * sizeof(CharT) <= m_end)
                    {
                        STR.resize(Length);
                        unsafe_read(std::span{ STR });
                        unsafe_read(Length);
                        if (Length == STR.size()) 
                        {
                            return false;
                        }
                    }
                }
                STR.clear();
                m_begin = old_begin;
                return true;
            }


            template <SwapType CharT>
            bool write(std::basic_string<CharT> const& STR) noexcept
            {
                int32_t Length{ static_cast<int32_t>(STR.size()) };
                if (m_begin + 8ull + Length * sizeof(CharT) <= m_end)
                {
                    unsafe_write(Length);
                    unsafe_write(std::span{ STR });
                    unsafe_write(Length);
                    return false;
                }
                return true;
            }



            stream read(block& Block) noexcept
            {
                auto old_begin{ m_begin };
                if (m_begin + 8ull <= m_end)
                {
                    copy(Block.Size, m_begin);
                    if (Block.valid() && m_begin + Block.length() <= m_end)
                    {
                        int32_t NSize{ 0 };
                        copy(NSize, m_begin + 4ull + Block.Size);
                        if (NSize + Block.Size == 0) {
                            m_begin += Block.length();
                            return stream{ old_begin + 4, m_begin - 4 };
                        }
                    }
                }
                return stream{ nullptr, nullptr };
            }



            protected:

                pointer m_begin{ nullptr };
                pointer m_end{ nullptr };

        };



        class buffer : public stream
        {

        public:

            using stream::value_type;
            using stream::pointer;
            using stream::const_pointer;

            constexpr buffer(pointer begin, pointer end) noexcept : stream{ begin, end } {}
            constexpr buffer(pointer begin, size_t Size) noexcept : stream{ begin, Size } {}
            constexpr buffer(void* begin, size_t Size) noexcept : stream{ begin, Size } {}
            constexpr buffer(stream ss) noexcept : stream{ ss } {}
            

            constexpr void shrinkby(size_t L) noexcept
            {
                m_end = m_begin + L < m_end ? (m_end - L) : m_end;
            }

            template <SwapTypeNonConst T>
            inline void upsafe_pop_back(T& t) noexcept
            {
                m_end -= sizeof(T);
                copy(t, m_end);
            }

            template <SwapTypeNonConst T>
            bool pop_back(T& t) noexcept
            {
                if (m_begin + sizeof(T) <= m_end) {
                    upsafe_pop_back(t);
                    return false;
                }
                return true;
            }

            template <SwapTypeNonConst T, size_t N>
            void unsafe_pop_back(std::span<T, N> sp) noexcept
            {
                m_end -= sizeof(T) * sp.size();
                copy(sp, m_end);
            }

            template <SwapTypeNonConst T, size_t N>
            bool pop_back(std::span<T, N> sp) noexcept
            {
                if (m_begin + sizeof(T) * sp.size() <= m_end)
                {
                    unsafe_pop_back(sp);
                    return false;
                }
                return true;
            }


            template <SwapTypeNonConst CharT>
            bool pop_back(std::basic_string<CharT>& STR) noexcept
            {
                auto old_end{ m_end };
                if (m_begin + 8ull <= m_end)
                {
                    int32_t Length{ 0 };
                    unsafe_pop_back(Length);
                    if (m_begin + 4ull + Length * sizeof(CharT) <= m_end)
                    {
                        STR.resize(Length);
                        unsafe_pop_back(std::span{ STR });
                        unsafe_pop_back(Length);
                        if (Length == STR.size()) {
                            return false;
                        }
                    }
                }
                STR.clear();
                m_end = old_end;
                return true;
            }










        };


        constexpr stream::stream(buffer bf) noexcept : stream{ bf.begin(), bf.end() } {}


    }
}


#endif


// PCSP7077