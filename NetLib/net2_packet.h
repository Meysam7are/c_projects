#ifndef NET_PACKET_HEADER_FILE
#define NET_PACKET_HEADER_FILE
#pragma once

#include <string>
#include <format>

#include "net2_config.h"




//#define USE_FIRST_METHOD
namespace mz {
	namespace net2 {



		struct packet_header {

			uint32_t Length{ 0 };
			uint32_t Command{ 0 };
			net_time TimeStamp{ 0 };
			uint64_t Value1{ 0 };
			uint64_t Value2{ 0 };

			std::string string() const noexcept 
			{
				return std::format("{}: #L:{} #C:{} V1:{} V2:{}", TimeStamp.string(), Length, Command, Value1, Value2);
			}
		};



		class packet : public mz::endian::vector {

		public:

			using value_type = uint8_t;
			using pointer = value_type*;
			using const_pointer = value_type const*;


			packet_header Head;
			uint32_t Action{ 0 };

			static constexpr size_t HeadSize{ sizeof(packet_header) };


			packet() noexcept = default;


			packet(packet_header const& Header) : Head{ Header } {}





			auto head_span() noexcept {
				static constexpr size_t SpanSize{ sizeof(packet_header) - 4 };
				return std::span<uint8_t, SpanSize>{ reinterpret_cast<uint8_t*>(&Head) + 4, SpanSize };
			}

			auto tail_span() noexcept { return std::span<value_type>(data(), size()); }


			void SwapNetEndian() noexcept {
				Head.Length = uint32_t(size());
				if constexpr (std::endian::native != net_endian)
				{
					Head.Length = mz::endian::swap_bytes(Head.Length);
					Head.Command = mz::endian::swap_bytes(Head.Command);
					Head.TimeStamp.tsep = mz::endian::swap_bytes(Head.TimeStamp.tsep);
					Head.Value1 = mz::endian::swap_bytes(Head.Value1);
					Head.Value2 = mz::endian::swap_bytes(Head.Value2);
				}
			}

			uint32_t get_encoded_size() const noexcept {
				uint32_t NetEncodedLength{ Head.Length };
				if constexpr (std::endian::native != net_endian) {
					NetEncodedLength = mz::endian::swap_bytes(NetEncodedLength);
				}
				return NetEncodedLength;
			}

			void set_encoded_size() noexcept {
				Head.Length = uint32_t(size());
					if constexpr (std::endian::native != net_endian) {
					Head.Length = mz::endian::swap_bytes(Head.Length);
				}
			}


			asio::const_buffer cbuff_head() const { return asio::const_buffer{ &Head, HeadSize }; }
			asio::const_buffer cbuff_tail() const { return asio::const_buffer{ data(), size() }; }
			asio::mutable_buffer mbuff_head() { return asio::mutable_buffer(&Head, HeadSize); }
			asio::mutable_buffer mbuff_tail() { return asio::mutable_buffer(data(), size()); }
			asio::mutable_buffer mbuff_tail(size_t TailSize) { resize(TailSize); return mbuff_tail(); }
			std::array<asio::const_buffer, 2> send_array() const { return std::array<asio::const_buffer, 2>{ cbuff_head(), cbuff_tail() }; }
			std::array<asio::mutable_buffer, 2> recv_array() { return std::array<asio::mutable_buffer, 2>{mbuff_head(), mbuff_tail()}; }
		};




	}


}



namespace mz {

	namespace net2 {


		class connection;

		struct owned_packet
		{
			std::shared_ptr<connection> remote = nullptr;
			packet msg;
		};

		struct connect_handshake {
			uint32_t BcryptCount{ 0 };
			uint32_t UpdateCount{ 0 };
			mz::crypt::blow_salt Salt{};
			mz::crypt::blow_pass Pass{};

			void generate(mz::randomizer& rengine);
			packet update_fish(mz::crypt::blow_fish&);
			bool pop_back(mz::endian::vector& msg) noexcept;



			friend bool operator == (connect_handshake const& L, connect_handshake const& R) noexcept { return memcmp(&L, &R, sizeof(connect_handshake)) == 0; }
			friend bool operator != (connect_handshake const& L, connect_handshake const& R) noexcept { return memcmp(&L, &R, sizeof(connect_handshake)) != 0; }
		};

	}

}





#endif