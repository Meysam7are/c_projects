#ifndef MZ_ENDIAN_BYTESWAP_CONVERSIONS_HEADER_FILE
#define MZ_ENDIAN_BYTESWAP_CONVERSIONS_HEADER_FILE
#pragma once

/*
*   Helper functions for endian conversions 
* 
*   Author: Meysam Zare
*   Last Modified: 6/20/24
*/


#include <cstdint>
#include <concepts>
#include <type_traits>

#ifdef _MSC_VER

#include <stdlib.h>
#define mz_temp_bswap_16(x) _byteswap_ushort(x)
#define mz_temp_bswap_32(x) _byteswap_ulong(x)
#define mz_temp_bswap_64(x) _byteswap_uint64(x)

#elif defined(__APPLE__)

// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define mz_temp_bswap_16(x) OSSwapInt16(x)
#define mz_temp_bswap_32(x) OSSwapInt32(x)
#define mz_temp_bswap_64(x) OSSwapInt64(x)

#elif defined(__sun) || defined(sun)

#include <sys/byteorder.h>
#define mz_temp_bswap_16(x) BSWAP_16(x)
#define mz_temp_bswap_32(x) BSWAP_32(x)
#define mz_temp_bswap_64(x) BSWAP_64(x)

#elif defined(__FreeBSD__)

#include <sys/endian.h>
#define mz_temp_bswap_16(x) bswap16(x)
#define mz_temp_bswap_32(x) bswap32(x)
#define mz_temp_bswap_64(x) bswap64(x)

#elif defined(__OpenBSD__)

#include <sys/types.h>
#define mz_temp_bswap_16(x) swap16(x)
#define mz_temp_bswap_32(x) swap32(x)
#define mz_temp_bswap_64(x) swap64(x)

#elif defined(__NetBSD__)

#include <sys/types.h>
#include <machine/bswap.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define mz_temp_bswap_16(x) bswap16(x)
#define mz_temp_bswap_32(x) bswap32(x)
#define mz_temp_bswap_64(x) bswap64(x)
#endif

#else

#include <byteswap.h>
#define mz_temp_bswap_16(x) bswap_16(x)
#define mz_temp_bswap_32(x) bswap_32(x)
#define mz_temp_bswap_64(x) bswap_64(x)


#endif


namespace mz {
    namespace endian {


        template <std::integral T>
        T swap_bytes(T t) noexcept {
            if constexpr (sizeof(t) == 1) {
                return t;
            }
            else if constexpr (sizeof(t) == 2) {
                return mz_temp_bswap_16(t);
            }
            else if constexpr (sizeof(t) == 4) {
                return mz_temp_bswap_32(t);
            }
            else {
                return mz_temp_bswap_64(t);
            }
        }

        template <typename E> requires std::is_enum_v<E>
        E swap_bytes(E e) noexcept {
            if constexpr (sizeof(e) == 1) {
                return e;
            }
            else {
                return static_cast<E>(swap_bytes(static_cast<std::underlying_type_t<E>>(e)));
            }
        }

    }
}

#undef mz_temp_bswap_16
#undef mz_temp_bswap_32
#undef mz_temp_bswap_64





#endif
