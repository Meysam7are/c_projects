#ifndef MZ_NET_CONFIG_HEADER_FILE
#define MZ_NET_CONFIG_HEADER_FILE
#pragma once

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#endif

#ifndef WINVER
#define WINVER 0x0A00
#endif


#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif


#include <cstdint>
#include <concepts>
#include <memory>
#include <deque>
#include <optional>
#include <vector>
#include <algorithm>
#include <chrono>
#include <format>
#include <array>
#include <span>
#include <bit>

#include "logger.h"
#include "ca_endian.h"
#include "blow_crypt.h"
#include "time_conversions.h"


#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>


namespace mz {
	namespace net2 {


		static constexpr auto net_endian{ std::endian::little };

		using net_duration = std::chrono::duration<std::chrono::microseconds>;
		using net_time = mz::time::steadytime<std::chrono::microseconds>;

		
		using asiocontext = asio::io_context;
		using asioguard = asio::executor_work_guard<asio::io_context::executor_type, void, void>;
		using asiosocket = asio::basic_stream_socket<asio::ip::tcp, asio::io_context::executor_type>;
		using asiostrand = asio::strand<asio::io_context::executor_type>;
		using asiohandle = asio::basic_socket<asio::ip::tcp, asio::io_context::executor_type>::native_handle_type;
		using asioacceptor = asio::basic_socket_acceptor< asio::ip::tcp, asio::io_context::executor_type >;
		using asioendpoint = asio::ip::tcp::endpoint;
		using asioquery = asio::ip::tcp::resolver::results_type;

	

		template <mz::endian::SwapType T>
		inline T to_endian(T t) noexcept {
			if constexpr (std::endian::native != net_endian) {
				t = mz::endian::swap_bytes(t);
			}
			return t;
		}




		struct packet_header;
		class packet;
		class connection;


		class encryptor
		{
		public:

			virtual int encrypt(void*, size_t) noexcept { return 0; }
			virtual int decrypt(void*, size_t) noexcept { return 0; }
		};




	}
}



#endif