#ifndef MZ_ENDIAN_MUTABLE_BUFFER
#define MZ_ENDIAN_MUTABLE_BUFFER
#pragma once

/*
*   Simple mutable buffer API to allow for reading and writing literal types with automatic endian conversion
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


        class write_buffer {

        public:

            using value_type = uint8_t;
            using pointer = uint8_t*;
            using const_pointer = uint8_t const*;


            explicit constexpr write_buffer(pointer begin, pointer end) noexcept : m_begin{ begin }, m_end{ end } {}
            constexpr write_buffer(pointer begin, size_t Size) noexcept : write_buffer{ begin, begin + Size } {}
            constexpr write_buffer(void* begin, size_t Size) noexcept : write_buffer{ (pointer)begin, Size } {}


            constexpr pointer end() noexcept { return m_end; }
            constexpr pointer data() noexcept { return m_begin; }
            constexpr pointer begin() noexcept { return m_begin; }
            constexpr uint8_t const* end() const noexcept { return m_end; }
            constexpr uint8_t const* data() const noexcept { return m_begin; }
            constexpr uint8_t const* begin() const noexcept { return m_begin; }
            constexpr bool empty() const noexcept { return m_begin == m_end; }
            constexpr bool error() const noexcept { return m_end < m_begin || !m_begin; }
            constexpr size_t size() const noexcept { return m_end - m_begin; }


            constexpr void skip(size_t N) noexcept {
                auto ptr{ m_begin + N };
                m_begin = ptr <= m_end ? ptr : m_end;
            }


            template <typename T> void unsafe_push_back(T) = delete;
            template <typename T> void push_back(T) = delete;



            /*
                UNSAFE PUSH BACK

            */


            template <SwapType T> 
            void unsafe_push_back(T t) noexcept { 
                mz::endian::copy(m_begin, t); 
                m_begin += sizeof(T); 
            }


            template <SwapType T, size_t N>
            void unsafe_push_back(std::span<T, N> SP) noexcept {
                mz::endian::copy(m_begin, SP);
                m_begin += sizeof(T) * SP.size();
            }


            static constexpr size_t size_of(std::string const& NS) noexcept { return NS.size() + 8; }
            void unsafe_push_back(std::string const& NS) noexcept {
                uint32_t Size{ (uint32_t)NS.size() };
                unsafe_push_back(Size);
                unsafe_push_back(std::span{ NS });
                unsafe_push_back(Size);
            }

            static constexpr size_t size_of(std::wstring const& NS) noexcept { return NS.size() * 2 + 8; }
            void unsafe_push_back(std::wstring const& WS) noexcept {
                uint32_t Size{ (uint32_t)WS.size() };
                unsafe_push_back(Size);
                unsafe_push_back(std::span{ WS });
                unsafe_push_back(Size);
            }


            /*
                SAFE PUSH BACK
            */



            template <SwapType T>
            bool push_back(T t) noexcept {
                if (m_begin + sizeof(T) <= m_end) {
                    unsafe_push_back(t);
                    return false;
                }
                return true;
            }



            template <SwapType T, size_t N>
            bool push_back(std::span<T, N> SP) noexcept {
                if (m_begin + sizoef(T) * SP.size() <= m_end) {
                    unsafe_push_back(SP);
                    return false;
                }
                return true;
            }

            bool push_back(std::string const& NS) noexcept {
                if (m_begin + size_of(NS) <= m_end) {
                    unsafe_push_back(NS);
                    return false;
                }
                return true;
            }

            bool push_back(std::wstring const& WS) noexcept {
                if (m_begin + size_of(WS) <= m_end) {
                    unsafe_push_back(WS);
                    return false;
                }
                return true;
            }


            template <SwapType T, size_t N>
            size_t push_back_some(std::span<T, N> SP) noexcept {
                size_t SomeSize = (m_end - m_begin) / sizeof(T);
                SomeSize = SomeSize < SP.size() ? SomeSize : SP.size();
                unsafe_push_back(std::span<T>(SP.data(), SomeSize));
                return SomeSize;
            }


            private:

                pointer m_begin{ nullptr };
                pointer m_end{ nullptr };

        };

    }



    template <endian::SwapType T>
    inline void unsafe_push_back(endian::write_buffer& PS, T t) noexcept { PS.unsafe_push_back(t); }

    template <endian::SwapType T, size_t N>
    inline void unsafe_push_back(endian::write_buffer& PS, std::span<T, N> SP) noexcept { PS.unsafe_push_back(SP); }

    inline void unsafe_push_back(endian::write_buffer& PS, std::string const& NS) noexcept { PS.unsafe_push_back(NS); }
    inline void unsafe_push_back(endian::write_buffer& PS, std::wstring const& WS) noexcept { PS.unsafe_push_back(WS); }


    template <endian::SwapType T>
    inline bool push_back(endian::write_buffer& PS, T t) noexcept { return PS.push_back(t); }

    template <endian::SwapType T, size_t N>
    inline bool push_back(endian::write_buffer& PS, std::span<T, N> SP) noexcept { return PS.push_back(SP); }

    inline bool push_back(endian::write_buffer& PS, std::string const& NS) noexcept { return PS.push_back(NS); }
    inline bool push_back(endian::write_buffer& PS, std::wstring const& WS) noexcept { return PS.push_back(WS); }










}


#endif
