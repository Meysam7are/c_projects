#include "net2_encryption.h"
#include "net2_server_interface.h"
#include "net2_client_interface.h"

namespace mz {
	namespace net2 {

		void server_encryption_interface::HandshakeWithClient(std::shared_ptr<connection> conn) noexcept
		{
			ServerInterface.OnHandshakeSuccess(std::move(conn));
		}

		void client_encryption_interface::HandshakeWithServer(std::shared_ptr<connection> conn) noexcept
		{
			ClientInterface.OnHandshakeSuccess(std::move(conn));
		}


		int connection_bcrypt::encrypt(packet& P) noexcept {
			Fish.encrypt(P.head_span());
			Fish.encrypt(P.tail_span());
			return 0;
		}

		int connection_bcrypt::decrypt(packet& P) noexcept {
			Fish.decrypt(P.head_span());
			Fish.decrypt(P.tail_span());
			return 0;
		}

		int connection_bcrypt::update(packet& Msg) noexcept
		{
			mz::crypt::blow_fish::bcrypt_parameters Par;
			if (Msg.pop_back(Par.Count) ||
				Msg.pop_back(Par.Salt.span()) ||
				Msg.pop_back(Par.Pass.span()) ||
				Msg.size()) {
				return 1;
			}
			if (Par.Count > 100000) { return 2; }
			Fish.bcrypt(Par.Pass, Par.Salt, Par.Count);
			return 0;
		}





		void server_bcrypt::generate() noexcept
		{
			mz::crypt::blow_fish::bcrypt_parameters Params;
			Params.Count = 400;
			RandEngine.randomize(Params.Pass.span(), true);
			RandEngine.randomize(Params.Salt.span(), true);
			RandEngine.randomize(Text.span(), true);
			
			packet ParamPacket{};
			ParamPacket.push_back(Params.Pass.span());
			ParamPacket.push_back(Params.Salt.span());
			ParamPacket.push_back(Params.Count);
			ParamPacket.push_back(Text.span());
			
			Fish.bcrypt(Params);
			Fish.encrypt(Text.span());
		}


		void server_bcrypt::HandshakeWithClient(std::shared_ptr<connection> conn) noexcept
		{
			conn->encryptor = std::make_unique<connection_bcrypt>(Fish);
			asio::async_write(
				conn->AsioSocket,
				ParamPacket.send_array(),
				[this, conn](std::error_code Ec, size_t)
				{
					if (!Ec)
					{
						conn->TempMsg.resize(Text.size());
						asio::async_read(
							conn->AsioSocket,
							conn->TempMsg.recv_array(),
							[this, conn](std::error_code Ec, size_t) 
							{
								if (!Ec)
								{
									if (!memcmp(conn->TempMsg.data(), Text.data(), Text.size()))
									{
										ServerInterface.OnHandshakeSuccess(std::move(conn));
									}
									else
									{
										mz::ErrLog.ts() << "login_server::ConnectToClient: incoming message inconsistent";
										conn->Disconnect();
									}
								}
								else
								{
									mz::ErrLog.ts() << "read accept handshake reply Ec: " << Ec.message();
									conn->Disconnect();
								}
							}
						);

					}
					else
					{
						mz::ErrLog.ts() << "write accept handshake Ec: " << Ec.message();
						conn->Disconnect();
					}
				});

		}


		void client_bcrypt::HandshakeWithServer(std::shared_ptr<connection> conn) noexcept
		{
			conn->TempMsg.resize(sizeof(mz::crypt::blow_fish::bcrypt_parameters) + sizeof(mz::crypt::blow_pass));
			conn->encryptor = std::make_unique<connection_bcrypt>();
			asio::async_read(
				conn->AsioSocket,
				conn->TempMsg.recv_array(),
				[this, conn](std::error_code Ec, size_t)
				{
					if (!Ec)
					{
						mz::crypt::blow_pass Code;
						conn->TempMsg.pop_back(Code.span());
						if (int Error = conn->encryptor->update(conn->TempMsg); !Error)
						{
							conn->TempMsg.push_back(Code.span());
							conn->Send(conn->TempMsg);
							ClientInterface.OnHandshakeSuccess(std::move(conn));
						}
						else
						{
							mz::ErrLog.ts() << "client_connect_handshake: Error = ", Error;
							conn->Disconnect();
						}
					}
					else
					{
						mz::ErrLog.ts() << "client_connect_handshake::async_read Ec: " << Ec.message();
						conn->Disconnect();
					}
				}
			);
		}


	}
}