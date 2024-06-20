#pragma once

#include <bit>
#include <cstdint>
#include "endian_concepts.h"


namespace mz {
    namespace crypt {

        inline static constexpr auto const native_endian{ mz::endian::native_endian };
        inline static constexpr auto const server_endian{ std::endian::big };

        inline uint32_t stream_to_native(uint32_t x) noexcept {
            if constexpr (native_endian != server_endian) { x = mz::endian::swap_bytes(x); }
            return x;
        }

        inline uint16_t stream_to_native(uint16_t x) noexcept {
            if constexpr (native_endian != server_endian) { x = mz::endian::swap_bytes(x); }
            return x;
        }

        inline uint8_t stream_to_native(uint8_t x) noexcept {
            return x;
        }


        inline uint32_t native_to_stream(uint32_t x) noexcept {
            if constexpr (native_endian != server_endian) { x = mz::endian::swap_bytes(x); }
            return x;
        }

        inline uint16_t native_to_stream(uint16_t x) noexcept {
            if constexpr (native_endian != server_endian) { x = mz::endian::swap_bytes(x); }
            return x;
        }

        inline uint8_t native_to_stream(uint8_t x) noexcept {
            return x;
        }

    }
}
