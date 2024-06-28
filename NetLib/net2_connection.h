#ifndef MZ_NET_CONNECTION_HEADER_FILE
#define MZ_NET_CONNECTION_HEADER_FILE
#pragma once


#include "net2_config.h"
#include "net2_safe_queue.h"
#include "net2_encryption.h"
#include "net2_process.h"


namespace mz {


	namespace net2 {




		class basic_interface;
		class server_interface;
		class worker_interface;


		// std::enable_shared_from_this used in reginster_incoming_message, read_validation
		class connection : public std::enable_shared_from_this<connection>
		{

			friend class basic_interface;

		public:

			enum class owner
			{
				server,
				client,
				worker,
			};


			// for server_interface, one shared BlowFish
			connection(
				owner OwnerType,
				asiocontext& AsioContext,
				asiosocket AsioSocket,
				safe_queue<owned_packet>& ServerRecvQueue,
				mz::randomizer& RandomEngine)
				: AsioStrand(asio::make_strand(AsioContext))
				, AsioSocket(std::move(AsioSocket))
				, ServerRecvQueue(ServerRecvQueue)
				, randEngine{ RandomEngine }
				, OwnerType{ OwnerType } {}


			virtual ~connection();





			void StartListening();

			void StopListening();


			bool IsConnected() const noexcept { return AsioSocket.is_open(); }

			void Disconnect();


			int Recv(packet&) noexcept;
			void Send(packet&) noexcept;

			int Recv(packet&& P) noexcept { return Recv(P); }
			void Send(packet&& P) noexcept { Send(P); }

			



			int OnMessage(packet&& P) noexcept { return OnMessage(P); }
			virtual int OnMessage(packet& P) noexcept {};



			std::string string() const;


		private:

			void WriteLoop() noexcept;

			void ReadLoop() noexcept; 



		protected:

		public:

			basic_interface* server{ nullptr };

			std::unique_ptr<connection_encryption_interface> encryptor;

			asio::strand<asiocontext::executor_type> AsioStrand;
			asiosocket AsioSocket;
			safe_queue<owned_packet>& ServerRecvQueue;
			mz::randomizer& randEngine;


			owner OwnerType = owner::server;
			packet TempMsg;
			safe_queue<packet> SendQueue;




			std::string Name{ "CONN" };
			int32_t NumOutgoingMessages{ 0 };
			int32_t NumIncomingMessages{ 0 };





			mz::crypt::blow_fish Fish;
			mz::crypt::blow_pass Code;


			net_time now();


		};



	}

}





#endif