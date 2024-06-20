#ifndef MZ_ENDIAN_VECTOR_HEADER_FILE
#define MZ_ENDIAN_VECTOR_HEADER_FILE
#pragma once

/*
*   Simple wrapper around std::vector<uint8_t> to allow for reading and writing literal types with automatic endian conversion
* 
*   Author: Meysam Zare
*   Last Modified: 6/20/24
*/



#include <cstdint>
#include <vector>
#include <concepts>
#include <type_traits>
#include <span>

#include "endian_read_buffer.h"
#include "endian_write_buffer.h"
#include "endian_stream.h"

namespace mz {
    namespace endian {

        class vector {






        public:

            using value_type = uint8_t;
            using pointer = uint8_t*;
            using const_pointer = uint8_t const*;





            template <typename T>
            struct place_holder {
                mutable size_t Offset{ 0 };
                T Value{};
            };



            explicit constexpr vector() noexcept : m_size{ 0 }, m_data{} {}
            explicit constexpr vector(size_t Size) noexcept : m_size{ Size }, m_data(Size) {}

            vector(vector const& RHS) noexcept : m_size{ RHS.m_size }
            {
                m_data.resize(size());
                memcpy(m_data.data(), RHS.m_data.data(), size());
            }

            vector& operator = (vector const& RHS) noexcept {
                if (this != &RHS) {
                    m_size = RHS.m_size;
                    m_data.resize(m_size);
                    memcpy(m_data.data(), RHS.data(), m_size);
                }
                return *this;
            }

            vector(vector&& RHS) noexcept
                : m_size{ RHS.m_size }
                , m_data{ std::move(RHS.m_data) }
            {
                RHS.m_size = 0;
            }

            vector& operator = (vector&& RHS) noexcept {
                if (this != &RHS) {
                    std::swap(m_size, RHS.m_size);
                    std::swap(m_data, RHS.m_data);
                }
                return *this;
            }


            constexpr bool empty() const noexcept { return !m_size; }
            constexpr size_t size() const noexcept { return m_size; }
            constexpr uint8_t* data() noexcept { return m_data.data(); }
            constexpr uint8_t const* data() const noexcept { return m_data.data(); }


            constexpr int error() const noexcept { return m_data.size() < m_size ? 1 : 0; }


            void clear() noexcept {
                m_size = 0;
                m_data.resize(0);
            }

            void reserve(size_t NewCapacity) noexcept {
                if (m_data.size() < NewCapacity) {
                    m_data.resize(NewCapacity);
                }
            }

            void resize(size_t NewSize) noexcept {
                if (m_data.size() < NewSize) {
                    m_data.resize(NewSize);
                }
                m_size = NewSize;
            }

            size_t expand_to_capacity() noexcept {
                size_t Capacity{ m_data.capacity() };
                m_data.resize(Capacity);
                m_size = Capacity;
                return m_size;
            }


            void expand_by(size_t Length) noexcept {
                reserve_extra(Length);
                m_size += Length;
            }

            size_t shrink_by(size_t Length) noexcept {
                Length = Length < m_size ? Length : m_size;
                m_size -= Length;
                return Length;
            }





            template <typename T> void push_back(T) = delete;


            constexpr std::span<uint8_t> span() noexcept { return std::span<uint8_t>{data(), size()}; }
            constexpr std::span<uint8_t> back_span(size_t FrontOffset) noexcept {
                FrontOffset = FrontOffset < size() ? FrontOffset : size();
                return std::span<uint8_t>{data() + FrontOffset, size() - FrontOffset};
            }

            read_buffer as_pop_buffer() const noexcept { return read_buffer{ data(), size() }; }
            write_buffer as_push_buffer() noexcept { return write_buffer{ data(), size() }; }

            read_buffer pop_back_span(size_t Size) noexcept {
                Size = shrink_by(Size);
                return read_buffer{ data() + size(), Size };
            }


            write_buffer push_back_span(size_t Size) noexcept {
                size_t OldLength{ size() };
                expand_by(Size);
                return write_buffer{ data() + OldLength, Size };
            }

            template <SwapType T>
            void unsafe_push_back(T t) noexcept {
                copy(data() + size(), t);
                m_size += sizeof(T);
            }

            template <SwapType T>
            void push_back(T t) noexcept {
                reserve_extra(sizeof(T));
                unsafe_push_back(t);
            }

            template<SwapType T>
            void push_back(place_holder<T>& ph) noexcept {
                ph.Offset = size();
                push_back(ph.Value);
            }

            template <SwapType T>
            bool update(place_holder<T>& ph) noexcept {
                if (size() >= ph.Offset + sizeof(T)) {
                    copy(data() + ph.Offset, ph.Value);
                    return false;
                }
                return true;
            }

            stream::block begin_block() noexcept
            {
                int32_t BlockSize{ 0 };
                size_t BlockOffset{ size() };
                push_back(BlockSize);
                return stream::block{ BlockOffset, BlockSize };
            }

            bool end_block(stream::block& Block) noexcept
            {
                if (size() >= (Block.Offset + 4ull)) {
                    Block.Size = static_cast<int32_t>(size() - (Block.Offset + 4ull));
                    copy(data() + Block.Offset, Block.Size);
                    push_back(-Block.Size);
                    return false;
                }
                Block.Size = -1;
                return true;
            }

            stream pop_back(stream::block& Block) noexcept
            {
                auto old_size = m_size;
                Block.Size = 0;
                if (size() >= Block.length())
                {
                    int32_t NSize{ 0 };
                    unsafe_pop_back(NSize);
                    if (NSize <= 0)
                    {
                        NSize = -NSize;
                        if (size() >= NSize + 4ull)
                        {
                            m_size -= NSize;
                            unsafe_pop_back(Block.Size);
                            if (Block.Size == NSize) {
                                Block.Offset = static_cast<uint32_t>(m_size);
                                return stream{ data() + Block.Offset + 4, size_t(Block.Size) };
                            }
                        }
                    }
                }
                Block.Size = -1;
                m_size = old_size;
                return stream{ nullptr, nullptr };
            }


            template <SwapTypeNonConst T>
            void unsafe_pop_back(T& t) noexcept {
                m_size -= (sizeof(T));
                copy(t, data() + size());
            }

            template <SwapTypeNonConst T>
            bool pop_back(T& t) noexcept {
                if (sizeof(T) <= m_size) {
                    unsafe_pop_back(t);
                    return false;
                }
                return true;
            }



            template <SwapType T, size_t N>
            void push_back(std::span<T, N> SP) noexcept {
                size_t ByteSize{ sizeof(T) * SP.size() };
                reserve_extra(ByteSize);
                copy(data() + size(), SP);
                m_size += ByteSize;
            }

            template <SwapTypeNonConst T, size_t N>
            bool pop_back(std::span<T, N> SP) noexcept {
                size_t ByteSize{ sizeof(T) * SP.size() };
                if (ByteSize <= m_size) {
                    m_size -= ByteSize;
                    read_buffer{ data() + size(), ByteSize }.unsafe_pop_front(SP);
                    return false;
                }
                return true;
            }


            void push_back(std::string const& NS) noexcept {
                size_t ByteSize{ write_buffer::size_of(NS) };
                push_back_span(ByteSize).unsafe_push_back(NS);
            }

            void push_back(std::wstring const& WS) noexcept {
                size_t ByteSize{ write_buffer::size_of(WS) };
                push_back_span(ByteSize).unsafe_push_back(WS);
            }




            bool pop_back(std::string& NS) noexcept {
                read_buffer PS = as_pop_buffer();
                if (!PS.pop_back(NS)) {
                    m_size = PS.size();
                    return false;
                }
                return true;
            }


            bool pop_back(std::wstring& WS) noexcept {
                read_buffer PS = as_pop_buffer();
                if (!PS.pop_back(WS)) {
                    m_size = PS.size();
                    return false;
                }
                return true;
            }



            template <TrivialTypeNonConst T>
            bool pop_back_raw(T& t) noexcept {
                if (sizeof(T) <= m_size) {
                    m_size -= (sizeof(T));
                    memcpy(&t, data() + size(), sizeof(T));
                    return false;
                }
                return true;
            }

            template <TrivialType T>
            void push_back_raw(T const& t) noexcept {
                size_t OldLength{ size() };
                expand_by(sizeof(T));
                memcpy(data() + OldLength, &t, sizeof(T));
            }


            void push_back_stream(void const* _Src, size_t Size)
            {
                size_t OldLength{ size() };
                expand_by(Size);
                memcpy(data() + OldLength, _Src, Size);
            }

            bool pop_back_stream(void* _Dst, size_t Size)
            {
                if (m_size >= Size)
                {
                    m_size -= Size;
                    memcpy(_Dst, data() + m_size, Size);
                    return false;
                }
                else { return true; }
            }


        private:

            size_t m_size{ 0 };
            std::vector<uint8_t> m_data;


            void reserve_extra(size_t ExtraSize) noexcept {
                if (m_data.size() < m_size + ExtraSize) {
                    m_data.resize(m_size + ExtraSize);
                }
            }


            friend bool operator == (vector const& L, vector const& R) noexcept {
                return L.m_size == R.m_size
                    && memcmp(L.m_data.data(), R.m_data.data(), L.m_size) == 0;
            }


        };


    }
}


#endif
