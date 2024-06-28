#ifndef MZ_BASIC_SERVER_HEADER_FILE
#define MZ_BASIC_SERVER_HEADER_FILE
#pragma once

#include "net2_connection.h"
#include "net2_encryption.h"
#include <atomic>

namespace mz {
	namespace net2 {


		class basic_interface
		{
		public:

			// Create a server, ready to listen on specified port
			basic_interface(std::string const& Name)
				: Name{ Name }
				, AsioGuard{ asio::make_work_guard(AsioContext) } {}

			virtual ~basic_interface() { Stop(); }


			virtual bool Start() noexcept
			{
				try
				{
					// Run asio io context in another thread
					AsioThread = std::thread([this]() { AsioContext.run(); });
				}
				catch (std::exception& e)
				{
					mz::ErrLog.ts(Name) << "Start: Except: {}" << e.what();
					return true;
				}
				return false;
			}

			virtual void Stop() noexcept
			{
				if (!Stopped) {
					Stopped = true;
					// Stop all connections to remove server, 
					AsioContext.post([this]() { Connections.clear(); });

					// Stop asio context
					AsioContext.post([this]() { AsioContext.stop(); });

					// Stop asio thread
					if (AsioThread.joinable()) { AsioThread.join(); }
				}


			}




			virtual void RemoveConnection(std::shared_ptr<connection> client) noexcept
			{
				client.reset();
				Connections.erase(
					std::remove(Connections.begin(), Connections.end(), client), Connections.end());
			}



			void Update(size_t nMaxMessages, bool bWait) noexcept
			{
				if (!Stopped)
				{
					if (bWait)
					{
						IncomingMessages.wait();
					}

					// Process as many messages as you can up to the value
					// specified
					size_t nMessageCount = 0;
					while (nMessageCount < nMaxMessages && !IncomingMessages.empty())
					{
						auto msg = IncomingMessages.pop_front();
						msg.remote->NumIncomingMessages--;
						//OnMessage(std::move(msg.remote), msg.msg);
						nMessageCount++;
					}
				}
			}



			virtual void OnHandshakeSuccess(std::shared_ptr<connection> conn) {}



			safe_queue<owned_packet> IncomingMessages;
			std::deque<std::shared_ptr<connection>> Connections;
			asiocontext AsioContext;
			std::thread AsioThread;
			asioguard AsioGuard;


			std::atomic<bool> Stopped{ false };


			std::string Name{};
			mz::randomizer RandEngine;






		};







	}
}


#endif