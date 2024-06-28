
#include "net2_connection.h"


namespace mz {
	namespace net2 {

		void connection::WriteLoop() noexcept {
			asio::async_write(AsioSocket, SendQueue.front().send_array(),
				[this](std::error_code Ec, size_t)
				{
					if (!Ec) {
						SendQueue.pop_front();
						--NumOutgoingMessages;
						if (!SendQueue.empty()) { WriteLoop(); }
					}
					else {
						mz::ErrLog.ts(Name) << "conn::WriteLoop: Error: " << Ec.message();
						AsioSocket.close(Ec);
					}
				});
		}

		void connection::ReadLoop() noexcept {
			asio::async_read(
				AsioSocket,
				TempMsg.mbuff_head(),
				[this](asio::error_code Ec, std::size_t Length)
				{
					if (!Ec) {
						uint32_t BodyLength{ TempMsg.get_encoded_size() };
						TempMsg.resize(BodyLength);
						if (!TempMsg.empty()) {
							asio::async_read(
								AsioSocket,
								TempMsg.mbuff_tail(),
								[this](std::error_code Ec, std::size_t Length)
								{
									if (!Ec) {
										++NumIncomingMessages;
										ServerRecvQueue.push_back({ this->shared_from_this(), TempMsg });
										ReadLoop();
									}
									else {
										mz::ErrLog.ts(Name) << "conn::read_tail: Connection Error : " << Ec.message();
									}
								});
						}
						else {
							++NumIncomingMessages;
							ServerRecvQueue.push_back({ this->shared_from_this(), TempMsg });
							ReadLoop();
						}
					}
					else {
						if (IsConnected()) {
							mz::ErrLog.ts(Name) << "conn::read_head Error: " << Ec.message();
						}
					}
				});
		}




		void connection::Send(packet& P) noexcept
		{
			++NumOutgoingMessages;
			P.SwapNetEndian();
			if (encryptor) {
				encryptor->encrypt(P);
			}
			asio::post(AsioStrand, [this, P]()
				{
					bool SendQueueIsEmpty{ SendQueue.empty() };
					SendQueue.push_back(P);
					if (SendQueueIsEmpty) { WriteLoop(); }
				});
		}

		int connection::Recv(packet& P) noexcept
		{
			--NumIncomingMessages;
			if (encryptor) {
				encryptor->decrypt(P);
			}
			P.SwapNetEndian();
			return OnMessage(P);
		}



	}
}
