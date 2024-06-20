#ifndef MZ_ENDIAN_CONST_BUFFER_HEADER_FILE
#define MZ_ENDIAN_CONST_BUFFER_HEADER_FILE
#pragma once

/*
*   Simple const buffer API to allow for reading and writing literal types with automatic endian conversion
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

        class read_buffer {

        public:

            using value_type = uint8_t;
            using pointer = uint8_t*;
            using const_pointer = uint8_t const*;



            constexpr read_buffer(const_pointer begin, const_pointer end) noexcept : m_begin{begin}, m_end{end} {}
            constexpr read_buffer(const_pointer begin, size_t Size) noexcept : read_buffer{ begin, begin + Size } {}
            constexpr read_buffer(void const* begin, size_t Size) noexcept : read_buffer{(const_pointer)begin, Size} {}

            constexpr const_pointer end() const noexcept { return m_end; }
            constexpr const_pointer data() const noexcept { return m_begin; }
            constexpr const_pointer begin() const noexcept { return m_begin; }
            constexpr bool empty() const noexcept { return m_begin == m_end; }
            constexpr bool error() const noexcept { return m_end < m_begin || !m_begin; }
            constexpr size_t size() const noexcept { return m_end - m_begin; }



            /*
                UNSAFE POP FRONT
            
            */

            template <SwapType T> requires (!std::is_const_v<T>)
            void unsafe_pop_front(T& t) noexcept { copy(t, m_begin); m_begin += sizeof(T); }


            template <SwapType T> requires (!std::is_const_v<T>)
                bool pop_front(T& t) noexcept {
                if (m_begin + sizeof(T) <= m_end) {
                    unsafe_pop_front(t);
                    return false;
                }
                return true;
            }


            //-------------------------------------------------------//


            template <SwapType T, size_t N> requires (!std::is_const_v<T>)
                void unsafe_pop_front(std::span<T, N> SP) noexcept { copy(SP, m_begin); m_begin += sizeof(T) * SP.size(); }

            template <SwapType T, size_t N> requires (!std::is_const_v<T>)
                bool pop_front(std::span<T, N> SP) noexcept {
                if (m_begin + sizeof(T) * SP.size() <= m_end) {
                    unsafe_pop_front(SP);
                    return false;
                }
                return true;
            }


            //-------------------------------------------------------//

            constexpr void skip_front(size_t L) noexcept {
                auto Ptr{ m_begin + L };
                m_begin = Ptr <= m_end ? Ptr : m_end;
            }

            constexpr void skip_back(size_t L) noexcept {
                auto Ptr{ m_end - L };
                m_end = Ptr >= m_begin ? Ptr : m_begin;
            }

            bool pop_front(std::string& NS) noexcept {
                auto old_begin{ m_begin };
                if (m_begin + 8ull <= m_end) {
                    uint32_t Size{ 0 };
                    unsafe_pop_front(Size);
                    if (m_begin + 4ull + Size <= m_end) {
                        NS.resize(Size);
                        unsafe_pop_front(std::span{ NS });
                        unsafe_pop_front(Size);
                        if (Size == NS.size()) {
                            return false;
                        }
                    }
                }
                NS.clear();
                m_begin = old_begin;
                return true;
            }

            bool pop_front(std::wstring& WS) noexcept {
                auto old_begin{ m_begin };
                if (m_begin + 8ull <= m_end) {
                    uint32_t Size{ 0 };
                    unsafe_pop_front(Size);
                    if (m_begin + 4ull + Size * 2ull <= m_end) {
                        WS.resize(Size);
                        unsafe_pop_front(std::span{ WS });
                        unsafe_pop_front(Size);
                        if (Size == WS.size()) {
                            return false;
                        }
                    }
                }
                WS.clear();
                m_begin = old_begin;
                return true;
            }




            /*
                UNSAFE POP BACK
            
            */




            template <SwapType T> requires (!std::is_const_v<T>)
            void unsafe_pop_back(T& t) noexcept { m_end -= sizeof(T);  copy(t, m_end); }

            template <SwapType T> requires (!std::is_const_v<T>)
                bool pop_back(T& t) noexcept {
                if (m_begin + sizeof(T) <= m_end) {
                    unsafe_pop_back(t);
                    return false;
                }
                return true;
            }

            //-------------------------------------------------------//


            template <SwapType T, size_t N> requires (!std::is_const_v<T>)
                void unsafe_pop_back(std::span<T, N> SP) noexcept {
                m_end -= sizeof(T) * SP.size();
                copy(SP, m_end);
            }

            template <SwapType T, size_t N> requires (!std::is_const_v<T>)
                bool pop_back(std::span<T, N> SP) noexcept {
                if (m_begin + sizeof(T) * SP.size() <= m_end) {
                    unsafe_pop_back(SP);
                    return false;
                }
                return true;
            }


            //-------------------------------------------------------//



            bool pop_back(std::string& NS) noexcept {
                auto old_end{ m_end };
                if (m_begin + 8ull <= m_end) {
                    uint32_t Size{ 0 };
                    unsafe_pop_back(Size);
                    if (m_begin + 4ull + Size <= m_end) {
                        NS.resize(Size);
                        unsafe_pop_back(std::span{ NS });
                        unsafe_pop_back(Size);
                        if (Size == NS.size()) {
                            return false;
                        }
                    }
                }
                NS.clear();
                m_end = old_end;
                return true;
            }

            bool pop_back(std::wstring& WS) noexcept {
                auto old_end{ m_end };
                if (m_begin + 8ull <= m_end) {
                    uint32_t Size{ 0 };
                    unsafe_pop_back(Size);
                    if (m_begin + 4ull + Size * 2ull <= m_end) {
                        WS.resize(Size);
                        unsafe_pop_back(std::span{ WS });
                        unsafe_pop_back(Size);
                        if (Size == WS.size()) {
                            return false;
                        }
                    }
                }
                WS.clear();
                m_end = old_end;
                return true;
            }



            template <SwapType T> 
            T unsafe_pop_front() noexcept { T t{}; unsafe_pop_front(t); return t; }

            template <SwapType T>
            T unsafe_pop_back() noexcept { T t{}; unsafe_pop_front(t); return t; }



            private:

                const_pointer m_begin{ nullptr };
                const_pointer m_end{ nullptr };

        };
    }



    template <endian::SwapType T> requires (!::std::is_const_v<T>)
        inline void unsafe_pop_front(endian::read_buffer& PS, T& t) noexcept { PS.unsafe_pop_front(t); }

    template <endian::SwapType T, size_t N> requires (!::std::is_const_v<T>)
        inline void unsafe_pop_front(endian::read_buffer& PS, ::std::span<T, N> SP) noexcept { PS.unsafe_pop_front(SP); }

    template <endian::SwapType T> requires (!::std::is_const_v<T>)
        inline void unsafe_pop_back(endian::read_buffer& PS, T& t) noexcept { PS.unsafe_pop_back(t); }

    template <endian::SwapType T, size_t N> requires (!::std::is_const_v<T>)
        inline void unsafe_pop_back(endian::read_buffer& PS, ::std::span<T, N> SP) noexcept { PS.unsafe_pop_back(SP); }



    template <endian::SwapType T> requires (!std::is_const_v<T>)
        inline bool pop_front(endian::read_buffer& PS, T& t) noexcept { return PS.pop_front(t); }

    template <endian::SwapType T> requires (!std::is_const_v<T>)
        inline bool pop_back(endian::read_buffer& PS, T& t) noexcept { return PS.pop_back(t); }

    template <endian::SwapType T, size_t N> requires (!std::is_const_v<T>)
        inline bool pop_front(endian::read_buffer& PS, std::span<T, N> SP) noexcept { return PS.pop_front(SP); }

    template <endian::SwapType T, size_t N> requires (!std::is_const_v<T>)
        inline bool pop_back(endian::read_buffer& PS, std::span<T, N> SP) noexcept { return PS.pop_back(SP); }

    inline bool pop_front(endian::read_buffer& PS, std::string& NS) noexcept { return PS.pop_front(NS); }
    inline bool pop_front(endian::read_buffer& PS, std::wstring& WS) noexcept { return PS.pop_front(WS); }
    inline bool pop_back(endian::read_buffer& PS, std::string& NS) noexcept { return PS.pop_back(NS); }
    inline bool pop_back(endian::read_buffer& PS, std::wstring& WS) noexcept { return PS.pop_back(WS); }


}


#endif
