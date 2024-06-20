#ifndef MZ_ENDIAN_TYPES_HEADER_FILE
#define MZ_ENDIAN_TYPES_HEADER_FILE
#pragma once


/*
*   Concepts and helper functions for literal types that allow automating endian conversions
* 
*   Author: Meysam Zare
*   Last Modified: 6/20/24
*/

#include <bit>
#include "endian_conversions.h"


namespace mz {

    namespace endian {

        static constexpr auto stream_endian{ std::endian::little };
        static constexpr auto native_endian{ std::endian::native };
        static constexpr bool endian_mismatch{ native_endian != stream_endian };

        template <typename T>
        concept TrivialType = std::is_trivially_copyable_v<T>;

        template <typename T>
        concept TrivialTypeNonConst = std::is_trivially_copyable_v<T> && !std::is_const_v<T>;

        template <typename T>
        concept IntType = std::is_integral_v<T> && std::has_unique_object_representations_v<T>;

        template <typename T>
        concept EnumType = std::is_enum_v<T> && std::has_unique_object_representations_v<T>;

        template <typename T>
        concept SafeEnumType = std::is_enum_v<T> && std::has_unique_object_representations_v<T>
            && requires(T e) {
            e = T::none;
            e = T::invalid;
        };


        template <typename T>
        concept SwapType = std::has_unique_object_representations_v<T> && (std::is_enum_v<T> || std::is_integral_v<T>);

        template <typename T>
        concept SwapTypeNonConst = (!std::is_const_v<T>) && std::has_unique_object_representations_v<T> && (std::is_enum_v<T> || std::is_integral_v<T>);



        template <SwapType T>
        inline T as_endian(T t) noexcept {
            if constexpr (endian_mismatch) { t = swap_bytes(t); }
            return t;
        }


        template <SwapType T>
        inline T as_little(T t) noexcept {
            if constexpr (std::endian::native != std::endian::little) { t = swap_bytes(t); }
            return t;
        }

        template <SwapType T>
        inline T as_big(T t) noexcept {
            if constexpr (std::endian::native != std::endian::big) { t = swap_bytes(t); }
            return t;
        }



        template <typename T> void copy(void* copy, T x) noexcept = delete;


        template <SafeEnumType S>
        constexpr bool valid(S s) noexcept {
            using T = std::underlying_type_t<S>;
            return static_cast<T>(s) > static_cast<T>(S::none)
                && static_cast<T>(s) < static_cast<T>(S::invalid);
        }

        template<SafeEnumType S>
        constexpr bool invalid(S s) noexcept { return !valid(s); }



        template<SwapType T>
        void copy(void* Ptr_, T t) noexcept {
            memcpy(Ptr_, &t, sizeof(T));
            if constexpr (endian_mismatch) {
                T* ptr{ static_cast<T*>(Ptr_) };
                ptr[0] = swap_bytes(ptr[0]);
            }
        }


        template <SwapType T, size_t N>
        void copy(void* Ptr_, std::span<T, N> SP) noexcept {
            using PT = std::remove_const_t<std::remove_const_t<T>*>;
            memcpy(Ptr_, SP.data(), sizeof(T) * SP.size());
            if constexpr (endian_mismatch) {
                if constexpr (sizeof(T) > 1) {
                    PT ptr{ static_cast<PT>(Ptr_) };
                    for (size_t i = 0; i < SP.size(); i++) {
                        ptr[i] = swap_bytes(ptr[i]);
                    }
                }
            }
        }



        template<SwapType T> requires (!std::is_const_v<T>)
        void copy(T& t, void const* Cptr_) noexcept {
            memcpy(&t, Cptr_, sizeof(T));
            if constexpr (endian_mismatch) { t = swap_bytes(t); }
        }



        template <SwapType T, size_t N> requires (!std::is_const_v<T>)
            void copy(std::span<T, N> SP, void const* Cptr_) noexcept {
            memcpy(SP.data(), Cptr_, sizeof(T) * SP.size());
            if constexpr (endian_mismatch) {
                if constexpr (sizeof(T) > 1) {
                    for (auto& t : SP) {
                        t = swap_bytes(t);
                    }
                }
            }
        }



        template<typename T>
        concept streamable =
            requires (void* Ptr_, T & t, T const& ct) {
            copy(t, Ptr_);
            copy(Ptr_, ct);
        };



    }

}


#endif
