#ifndef MZ_NET_BASIC_SERVER_HEADER_FILE
#define MZ_NET_BASIC_SERVER_HEADER_FILE
#pragma once

#include "net2_basic_interface.h"
#include "net2_process.h"

namespace mz {
	namespace net2 {


		// Server
		class server_interface : public basic_interface
		{
		public:

			server_interface(uint16_t port) noexcept
				: basic_interface{ "[LOGIN]" }
				, AsioAcceptor(AsioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) 
			{
				serverEncryptor = std::make_unique<server_encryption_interface>(*this);
			}


			virtual bool Start() noexcept
			{
				try
				{
					// if there was no guard, this line should go after AcceptorLoop
					AsioThread = std::thread([this]() { AsioContext.run(); });
					AcceptorLoop();
				}
				catch (std::exception& e)
				{
					mz::ErrLog.ts(Name) << "Start: Exception: " << e.what();
					return true;
				}
				return false;
			}

			virtual void Stop() noexcept
			{
				AsioContext.post([this]()
					{
						asio::error_code Ec;
						AsioAcceptor.close(Ec);
					});
				basic_interface::Stop();
			}


			// ASYNC - Instruct asio to wait for connection
			void AcceptorLoop()
			{
				// Prime context with an instruction to wait until a socket connects. This
				// is the purpose of an "acceptor" object. It will provide a unique socket
				// for each incoming connection attempt
				//asiosocket socket{ AsioContext };
				AsioAcceptor.async_accept(
					[this](std::error_code Ec, asiosocket socket)
					{
						if (!Stopped) {
							if (!Ec)
							{
								std::shared_ptr<connection> conn =
									std::make_shared<connection>(
										connection::owner::server,
										AsioContext,
										std::move(socket),
										IncomingMessages,
										RandEngine);

								conn->Name = Name;

								asio::post(
									conn->AsioStrand,
									[this, conn]()
									{
										serverEncryptor->HandshakeWithClient(std::move(conn));
									});
							}
							else
							{
								// Error has occurred during acceptance
								mz::ErrLog.ts(Name) << "New Connection Error: " << Ec.message();
							}

							// Prime the asio context with more work - again simply wait for
							// another connection...
							AcceptorLoop();

						}

					});
			}


			virtual void AuthenticateClient(std::shared_ptr<connection> conn) noexcept {}


			virtual void OnHandshakeSuccess(std::shared_ptr<connection> conn) {
  				conn->StartListening();
				Connections.push_back(std::move(conn));
			}



		protected:


			virtual void OnClientValidated(std::shared_ptr<connection> client) { }
			virtual void OnClientInvalidated(std::shared_ptr<connection> client) { }




		protected:

			asioacceptor AsioAcceptor;

			std::unique_ptr<server_encryption_interface> serverEncryptor;
		};





	}
}


#endif
