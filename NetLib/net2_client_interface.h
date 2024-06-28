#ifndef MZ_NET_BASIC_CLIENT_INTERFACE
#define MZ_NET_BASIC_CLIENT_INTERFACE

#include "net2_basic_interface.h"
#include "net2_process.h"
#include "net2_encryption.h"

namespace mz {
	namespace net2 {



		// Client
		class client_interface : public basic_interface
		{
		public:

			client_interface() : basic_interface{ "USER" } 
			{
				clientEncryptor = std::make_unique<client_encryption_interface>(*this);
			}

			void Disconnect() { Stop(); }


			void Connect(const std::string& host, const uint16_t port)
			{
				try
				{
					// Resolve hostname/ip-address into tangiable physical address
					asio::error_code Ec;
					asio::ip::tcp::resolver resolver(AsioContext);
					asioquery endpoints = resolver.resolve(host, std::to_string(port), Ec);
					if (!Ec) {
						// Create connection

						std::shared_ptr<connection> client =
							std::make_shared<connection>(
								connection::owner::client,
								AsioContext,
								asiosocket{ AsioContext },
								IncomingMessages,
								RandEngine);

						asio::async_connect(
							client->AsioSocket,
							endpoints,
							[this, client](asio::error_code Ec, asioendpoint Ep)
							{
								if (!Ec) {
									asio::post(
										client->AsioStrand,
										[this, client]() {
											clientEncryptor->HandshakeWithServer(std::move(client));
										});

								}
								else {

									mz::ErrLog.ts() << "client_net::ConnectToServer Ec: " << Ec.message();
								}
							}
						);
						Connections.push_back(std::move(client));
					}
					else {
						mz::ErrLog.ts() << "client_interface::connect Ec: " << Ec.message();
					}
				}
				catch (std::exception& e)
				{
					mz::ErrLog.ts() << "client_interface::connect Exception: ", std::string{ e.what() };
				}
			}


















			std::unique_ptr<client_encryption_interface> clientEncryptor;





		};





	}
}



#endif

