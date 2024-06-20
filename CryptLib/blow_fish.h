#ifndef MZ_CRYPT_BLOW_FISH_HEADER_FILE
#define MZ_CRYPT_BLOW_FISH_HEADER_FILE
#pragma once

/*
*   An implementation of a modified Bcrypt algorithm
* 
*   Author: Meysam Zare
*   Last Modified: 6/20/24
*/



#include <cstdint>
#include <memory>

#include "blow_feistel.h"
#include "blow_buffers.h"
#include "blow_key.h"

namespace mz {
    namespace crypt {


        using blow_pass = mz::endian::byte_array<16>;
        using blow_salt = mz::endian::byte_array<32>;



        class blow_fish {

        public:

            blow_key UniqueKeys;
            std::shared_ptr<feistel> sharedFeistel;


            constexpr size_t numKeys() const noexcept { return UniqueKeys.size(); }

            template <size_t NumBits>
            pair32 encryptN(pair32 Pair) const noexcept {
                size_t itr{ 0 };
                size_t const ub{ numKeys() - 2 };
                feistel const& NativeFeistel = const_ref();
                do {
                    Pair[0] ^= UniqueKeys[itr];
                    Pair[1] ^= NativeFeistel.apply<NumBits>(Pair[0]);
                    std::swap(Pair[0], Pair[1]);
                } while (++itr < ub);
                Pair[0] ^= UniqueKeys[itr];
                Pair[1] ^= UniqueKeys[++itr];
                std::swap(Pair[0], Pair[1]);
                return Pair;
            }

            template <size_t NumBits>
            pair32 decryptN(pair32 Pair) const noexcept {
                size_t itr{ numKeys() };
                feistel const& NativeFeistel = const_ref();
                do {
                    Pair[0] ^= UniqueKeys[--itr];
                    Pair[1] ^= NativeFeistel.apply<NumBits>(Pair[0]);
                    std::swap(Pair[0], Pair[1]);
                } while (itr > 2);
                Pair[0] ^= UniqueKeys[1];
                Pair[1] ^= UniqueKeys[0];
                std::swap(Pair[0], Pair[1]);
                return Pair;
            }



            void encrypt(mutable_blow_buffer stream) const noexcept {
                while (stream.has_pair32()) {
                    stream.update_next_pair32(encryptN<32>(stream.peek_next_pair32()));
                }
                if (stream.has_pair16()) {
                    stream.update_next_pair16(encryptN<16>(stream.peek_next_pair16()));
                }
                if (stream.has_pair8()) {
                    stream.update_next_pair8(encryptN<8>(stream.peek_next_pair8()));
                }
                if (stream.has_byte()) {
                    stream.update_next_byte(encryptN<4>(stream.peek_next_byte()));
                }
            }


            void decrypt(mutable_blow_buffer stream) const noexcept {
                while (stream.has_pair32()) {
                    stream.update_next_pair32(decryptN<32>(stream.peek_next_pair32()));
                }
                if (stream.has_pair16()) {
                    stream.update_next_pair16(decryptN<16>(stream.peek_next_pair16()));
                }
                if (stream.has_pair8()) {
                    stream.update_next_pair8(decryptN<8>(stream.peek_next_pair8()));
                }
                if (stream.has_byte()) {
                    stream.update_next_byte(decryptN<4>(stream.peek_next_byte()));
                }
            }

            void encrypt(mutable_blow_buffer stream, size_t N) const noexcept { while (N--) { encrypt(stream); } }
            void decrypt(mutable_blow_buffer stream, size_t N) const noexcept { while (N--) { decrypt(stream); } }
            
            
            void expand_keys(pair32& NativePair) noexcept {
                for (size_t i = 0; i < numKeys(); i += 2) {
                    encrypt(mutable_blow_buffer(NativePair));
                    UniqueKeys[i] = NativePair[0];
                    UniqueKeys[i + 1] = NativePair[1];
                }
            }


            void expand_boxes(pair32& NativePair) noexcept {
                auto matrix = (uint32_t*)sharedFeistel->Matrix;
                for (size_t i = 0; i < 1024; i += 2) {
                    encrypt(mutable_blow_buffer(NativePair));
                    matrix[i] = NativePair[0];
                    matrix[i + 1] = NativePair[1];
                }
            }


            template <size_t N> 
                requires (N > 0)
            void expand_keys(pair32& NativePair, mz::endian::byte_array<N> const& Salt) noexcept {
                const_pair_stream Stream{ Salt };
                for (size_t i = 0; i < numKeys(); i += 2) {
                    NativePair ^= Stream.next_native_pair(UniqueKeys.ItrIndex);
                    encrypt(mutable_blow_buffer(NativePair));
                    UniqueKeys[i] = NativePair[0];
                    UniqueKeys[i + 1] = NativePair[1];
                }
            }

            template <size_t N>
                requires (N > 0)
            void expand_boxes(pair32& NativePair, mz::endian::byte_array<N> const& Salt) noexcept {
                const_pair_stream Stream{ Salt };
                auto matrix = (uint32_t*)sharedFeistel->Matrix;
                for (size_t i = 0; i < 1024; i += 2) {
                    NativePair ^= Stream.next_native_pair(UniqueKeys.ItrIndex);
                    encrypt(mutable_blow_buffer(NativePair));
                    matrix[i] = NativePair[0];
                    matrix[i + 1] = NativePair[1];
                }
            }

            feistel& ref() noexcept { return *static_cast<feistel*>(sharedFeistel.get()); }
            feistel const& const_ref() const noexcept { return *static_cast<feistel const*>(sharedFeistel.get()); }


            template <size_t N>
            void encrypt(std::span<uint8_t,N> ARR) const noexcept { encrypt(mutable_blow_buffer{ ARR }); }

            template <size_t N>
            void decrypt(std::span<uint8_t,N> ARR) const noexcept { decrypt(mutable_blow_buffer{ ARR }); }



            template <size_t N>
                requires (N > 0)
            void encrypt(mz::endian::byte_array<N>& ARR) const noexcept { encrypt(mutable_blow_buffer{ ARR }); }

            template <size_t N>
                requires (N > 0)
            void decrypt(mz::endian::byte_array<N>& ARR) const noexcept { decrypt(mutable_blow_buffer{ ARR }); }

            template <size_t N>
                requires (N > 0)
            void encrypt(mz::endian::byte_array<N>& ARR, size_t Count) const noexcept { while (Count--) { encrypt(ARR); } }

            template <size_t N>
                requires (N > 0)
            void decrypt(mz::endian::byte_array<N>& ARR, size_t Count) const noexcept { while (Count--) { decrypt(ARR); } }

            template <size_t N>
                requires (N > 0)
            [[nodiscard]] mz::endian::byte_array<N> encrypted(mz::endian::byte_array<N> const& ARR, size_t Count = 1) const noexcept { 
                mz::endian::byte_array<N> Res{ ARR };
                encrypt(Res, Count);
                return Res;
            }

            template <size_t N>
                requires (N > 0)
            [[nodiscard]] mz::endian::byte_array<N> decrypted(mz::endian::byte_array<N> const& ARR, size_t Count = 1) const noexcept {
                mz::endian::byte_array<N> Res{ ARR };
                decrypt(Res, Count);
                return Res;
            }


            template <size_t N>
                requires (N > 0)
            void update_keys(mz::endian::byte_array<N> const& Pass, size_t Count) noexcept {
                pair32 Block;
                for (size_t i = 0; i < Count; i++) {
                    UniqueKeys.xor_with(Pass);
                    expand_keys(Block, Pass);
                }
            }

            template <size_t N, size_t M>
                requires (N > 0 && M > 0)
            void update_keys(mz::endian::byte_array<N> const& Pass, mz::endian::byte_array<M> const& Salt, size_t Count) noexcept {
                pair32 Block;
                for (size_t i = 0; i < Count; i++)
                {
                    UniqueKeys.xor_with(Pass);
                    expand_keys(Block, Salt);

                    UniqueKeys.xor_with(Salt);
                    expand_keys(Block, Pass);
                }
            }

            template <size_t N, size_t M>
                requires (N > 0 && M > 0)
            void scramble_and_update_keys(mz::endian::byte_array<N>& Pass, mz::endian::byte_array<M>& Salt, size_t Count) noexcept {
                pair32 Block{ 0,0 };
                for (size_t i = 0; i < Count; i++)
                {
                    UniqueKeys.xor_with(Salt);
                    expand_keys(Block, Pass);
                    encrypt(Salt);

                    UniqueKeys.xor_with(Pass);
                    expand_keys(Block, Salt);
                    encrypt(Pass);
                }
            }



            template <size_t N, size_t M>
                requires (N > 0 && M > 0)
            void bcrypt(mz::endian::byte_array<N> const& Pass, mz::endian::byte_array<M> const& Salt, size_t Count) noexcept {
                if (Count) {
                    pair32 Block;
                    UniqueKeys.xor_with(Pass);
                    expand_keys(Block, Salt);
                    expand_boxes(Block, Salt);
                    for (size_t i = 0; i < Count; i++)
                    {
                        UniqueKeys.xor_with(Pass);
                        expand_keys(Block);
                        expand_boxes(Block);

                        UniqueKeys.xor_with(Salt);
                        expand_keys(Block);
                        expand_boxes(Block);
                    }
                }
            }


            template <size_t N, size_t M>
                requires (N > 0 && M > 0)
            void scramble_and_bcrypt(mz::endian::byte_array<N>& Pass, mz::endian::byte_array<M>& Salt, size_t Count) noexcept 
            {
                if (Count) {
                    pair32 Block;
                    UniqueKeys.xor_with(Pass);
                    expand_keys(Block, Salt);
                    expand_boxes(Block, Salt);
                    for (size_t i = 0; i < Count; i++)
                    {
                        UniqueKeys.xor_with(Pass);
                        expand_keys(Block);
                        expand_boxes(Block);
                        encrypt(Salt);

                        UniqueKeys.xor_with(Salt);
                        expand_keys(Block);
                        expand_boxes(Block);
                        encrypt(Pass);
                    }
                }
            }

            template <size_t N, size_t M>
                requires (N > 0 && M > 0)
            blow_fish(mz::endian::byte_array<N> const& Pass, mz::endian::byte_array<M> const& Salt, size_t Count) noexcept : blow_fish{} { bcrypt(Pass, Salt, Count); }



            void detach() { sharedFeistel = std::make_shared<feistel>(*sharedFeistel); }



            blow_fish() noexcept : sharedFeistel{ std::make_shared<feistel>() } {}
            blow_fish(std::shared_ptr<feistel> Ptr_) noexcept : sharedFeistel{ std::move(Ptr_) } {}
            blow_fish(std::shared_ptr<feistel> Ptr_, blow_key const& Keys) noexcept
                : sharedFeistel{std::move(Ptr_)}
                , UniqueKeys{ Keys } {}

            blow_fish(feistel const& Feistel, blow_key const& Keys) noexcept
                : sharedFeistel{ std::make_shared<feistel>(Feistel) }
                , UniqueKeys{ Keys } {}



            
            bool has_same_box(blow_fish const& R) const noexcept { return (sharedFeistel == R.sharedFeistel); }
            bool has_equal_key(blow_fish const& R) const noexcept { return UniqueKeys == R.UniqueKeys; }
            bool has_equal_box(blow_fish const& R) const noexcept { return *sharedFeistel == *R.sharedFeistel; }
            bool has_equal_data(blow_fish const& R) const noexcept { return has_equal_key(R) && has_equal_box(R); }
            
            bool is_same(blow_fish const& R) const noexcept { return has_same_box(R) && has_equal_key(R); }
            bool is_copy(blow_fish const& R) const noexcept { return !has_same_box(R) && has_equal_box(R) && has_equal_key(R); }
            bool is_equal(blow_fish const& R) const noexcept { return has_equal_key(R) && (has_same_box(R) || has_equal_box(R)); }

            bool is_deep_same(blow_fish const& R) const noexcept { return UniqueKeys.ItrIndex == R.UniqueKeys.ItrIndex && is_same(R); }
            bool is_deep_copy(blow_fish const& R) const noexcept { return UniqueKeys.ItrIndex == R.UniqueKeys.ItrIndex && is_copy(R); }
            bool is_deep_equal(blow_fish const& R) const noexcept { return UniqueKeys.ItrIndex == R.UniqueKeys.ItrIndex && is_equal(R); }


            friend bool operator == (blow_fish const& L, blow_fish const& R) noexcept { return L.is_copy(R); }


            void reset() noexcept {
                UniqueKeys = blow_key{};
                if (sharedFeistel) {
                    *sharedFeistel = feistel{};
                }
                else {
                    sharedFeistel = std::make_shared<feistel>();
                }
            }

            void clear() noexcept {
                blow_fish Temp;
                std::swap(UniqueKeys, Temp.UniqueKeys);
                std::swap(sharedFeistel, Temp.sharedFeistel);
            }

        };



        class common_crypt : public blow_fish {

        public:
            blow_pass Pass{};
            blow_salt Salt{};
            uint32_t BcryptCount{ 0 };
            uint32_t UpdateCount{ 0 };

            common_crypt() noexcept : blow_fish{} {  }
            void bcrypt() noexcept {
                if (!sharedFeistel) {
                    sharedFeistel = std::make_shared<feistel>();
                }
                blow_fish::bcrypt(Pass, Salt, BcryptCount);
            }

        };



    }
}



#endif
//#endif