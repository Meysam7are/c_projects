#ifndef MZ_RANDOMIZER_HEADER_FILE
#define MZ_RANDOMIZER_HEADER_FILE
#pragma once

#include <cstdint>
#include <random>
#include <vector>
#include <span>

/*
*   A wrapper around Mersenne Twister Random Engine.
*   For the conversion of 32 bit pseudo-random number to 8, 16, 64 bit one a simple shift is used
*   More complicated operations are recommended for a more secure application in practice
* 
*   Author: Meysam Zare
*   Last Update: 6/20/24
*/

namespace mz
{
    class randomizer {

    public:

        static constexpr char const AlphaNumeric[] =
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static constexpr size_t NumAlphaNumeric{ sizeof(AlphaNumeric) - 1 };


        using engine_type =
            std::mersenne_twister_engine<
            uint32_t,                               // result type
            32,                                     // word size
            312,                                    // state size
            197,                                    // shift size
            31,                                     // mask bits
            0x9908b0df,                             // xor mask
            11,                                     // tempering u
            0xffffffff,                             // tempering d
            7,                                      // tempering s
            0x9d2c5680,                             // tempering b
            15,                                     // tempering t
            0xefc60000,                             // tempering c
            18,                                     // tempering l
            1812433253>;                            // initialization multiplier


        engine_type engine;
        uint32_t Seed32{ 0 };
        std::random_device rdev;

        void update(uint32_t Shift, bool ReSeed) noexcept {
            if (ReSeed && rdev.entropy()) {
                Seed32 = rdev();
            }
            else {
                Seed32 += Shift;
            }
            engine.seed(Seed32);
        }



        randomizer() noexcept = default;

        explicit randomizer(uint32_t Seed) noexcept : Seed32{ Seed } {}

        uint32_t rd() noexcept {
            if (rdev.entropy()) {
                return rdev();
            }
            return 0;
        }

        void seed() noexcept { Seed32 = 0; update(137, true); }
        void seed(uint32_t Seed) noexcept { Seed32 = 0; update(Seed, false); }


        uint32_t rand32(bool ReSeed = false) noexcept {
            uint32_t res{ engine() };
            update(res, ReSeed);
            return res;
        }



        // unsigned random numbers
        uint8_t rand8(bool ReSeed = false) noexcept { return static_cast<uint8_t>(rand32(ReSeed) >> 13); }
        uint16_t rand16(bool ReSeed = false) noexcept { return static_cast<uint16_t>(rand32(ReSeed) >> 13); }
        uint64_t rand64(bool ReSeed = false) noexcept { return (static_cast<uint64_t>(rand32(ReSeed)) << 32) + rand32(ReSeed); }

        // signed random numbers
        int8_t irand8(bool ReSeed = false) noexcept { return static_cast<int8_t>(rand8(ReSeed)); }
        int16_t irand16(bool ReSeed = false) noexcept { return static_cast<int16_t>(rand16(ReSeed)); }
        int32_t irand32(bool ReSeed = false) noexcept { return static_cast<int32_t>(rand32(ReSeed)); }
        int64_t irand64(bool ReSeed = false) noexcept { return static_cast<int64_t>(rand64(ReSeed)); }

        // signed non zero random numbers
        int8_t i8nz(bool ReSeed = false) noexcept { int8_t x; do { x = irand8(ReSeed); } while (!x); return x; }
        int16_t i16nz(bool ReSeed = false) noexcept { int16_t x; do { x = irand16(ReSeed); } while (!x); return x; }
        int32_t i32nz(bool ReSeed = false) noexcept { int32_t x; do { x = irand32(ReSeed); } while (!x); return x; }
        int64_t i64nz(bool ReSeed = false) noexcept { int64_t x; do { x = irand64(ReSeed); } while (!x); return x; }

        // signed positive random numbers
        int8_t i8pos(bool ReSeed = false) noexcept { int8_t x; do { x = irand8(ReSeed); } while (x <= 0); return x; }
        int16_t i16pos(bool ReSeed = false) noexcept { int16_t x; do { x = irand16(ReSeed); } while (x <= 0); return x; }
        int32_t i32pos(bool ReSeed = false) noexcept { int32_t x; do { x = irand32(ReSeed); } while (x <= 0); return x; }
        int64_t i64pos(bool ReSeed = false) noexcept { int64_t x; do { x = irand64(ReSeed); } while (x <= 0); return x; }

        // signed negative random numbers
        int8_t i8neg(bool ReSeed = false) noexcept { int8_t x; do { x = irand8(ReSeed); } while (x <= 0); return x; }
        int16_t i16neg(bool ReSeed = false) noexcept { int16_t x; do { x = irand16(ReSeed); } while (x >= 0); return x; }
        int32_t i32neg(bool ReSeed = false) noexcept { int32_t x; do { x = irand32(ReSeed); } while (x >= 0); return x; }
        int64_t i64neg(bool ReSeed = false) noexcept { int64_t x; do { x = irand64(ReSeed); } while (x >= 0); return x; }


        uint32_t operator()(bool ReSeed = false) noexcept { return rand32(ReSeed); }

        template <std::integral T>
        void operator()(T& t, bool ReSeed = false) noexcept { t = static_cast<T>(rand32(ReSeed)); }



        template <std::integral T, size_t N>
        void randomize(std::span<T, N> sp, bool ReSeed = false) noexcept {
            if constexpr (sizeof(T) == 1) {
                uint32_t R{ 0 };
                for (auto& x : sp) {
                    if (!R) { R = rand32(ReSeed); }
                    x = static_cast<T>(R & 0xff);
                    R >>= 8;
                }
            }
            else if constexpr (sizeof(T) == 2) {
                uint32_t R{ 0 };
                for (auto& x : sp) {
                    if (!R) { R = rand32(ReSeed); }
                    x = static_cast<T>(R & 0xffff);
                    R >>= 16;
                }
            }
            else if constexpr (sizeof(T) == 8) {
                for (auto& x : sp) { x = static_cast<T>(rand64(ReSeed)); }
            }
            else {
                for (auto& x : sp) { x = static_cast<T>(rand32(ReSeed)); }
            }
        }

        template<std::integral T, size_t N>
        void randomize(T(&ARR)[N], bool ReSeed = false) noexcept { randomize(std::span(ARR), ReSeed); }



        void alphanumeric(std::span<char> sp, bool ReSeed) {
            uint32_t R{ rand32(ReSeed) };
            for (auto& a : sp) {
                a = AlphaNumeric[(R >>= 6) % NumAlphaNumeric];
                if (!R) { R = rand32(ReSeed); }
            }
        }

        void alphanumeric(std::span<wchar_t> sp, bool ReSeed) {
            uint32_t R{ rand32(ReSeed) };
            for (auto& a : sp) {
                a = AlphaNumeric[(R >>= 6) % NumAlphaNumeric];
                if (!R) { R = rand32(ReSeed); }
            }
        }


        template<size_t N> void alphanumeric(char(&ARR)[N], bool ReSeed = false) noexcept { alphanumeric(std::span(ARR)); }
        template<size_t N> void alphanumeric(wchar_t(&ARR)[N], bool ReSeed = false) noexcept { alphanumeric(std::span(ARR)); }


        std::string string(size_t NumCharacters, bool ReSeed = false) noexcept {
            std::string Result(NumCharacters, 0);
            alphanumeric(std::span{ Result }, ReSeed);
            return Result;
        };

        std::wstring wstring(size_t NumCharacters, bool ReSeed = false) noexcept {
            std::wstring Result(NumCharacters, 0);
            alphanumeric(std::span{ Result }, ReSeed);
            return Result;
        };

        void alphanumeric(std::string& STR, bool ReSeed = false) noexcept {
            alphanumeric(std::span{ STR }, ReSeed);
        }

        // random number within the range [LB,UB]
        uint32_t range32(uint32_t LB, uint32_t UB, bool ReSeed = false) noexcept {
            uint32_t Res{ LB };
            if (LB < UB) {
                Res += rand32(ReSeed) % (UB - LB);
            }
            return Res;
        }
    };



}

#endif
