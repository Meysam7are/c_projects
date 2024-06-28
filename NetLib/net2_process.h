#ifndef MZ_NET_PROCESS_HEADER_FILE
#define MZ_NET_PROCESS_HEADER_FILE
#pragma once

#include "net2_packet.h"

namespace mz {
	namespace net2 {

		class basic_server;

		class basic_process {

			std::shared_ptr<connection> conn{};


		public:

			basic_process(std::shared_ptr<connection> conn) noexcept : conn{ std::move(conn) } {}
			virtual ~basic_process() noexcept = 0;
			
			virtual void OnMessage(packet&) noexcept {};

		};


		class basic_server_handshake : basic_process
		{

			basic_server* server{ nullptr };

		public:

			basic_server_handshake(std::shared_ptr<connection> conn, basic_server* server) noexcept;



		};




	}
}



#endif