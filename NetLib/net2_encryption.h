#ifndef MZ_NET_ENCRYPTION_HEADER_FILE
#define MZ_NET_ENCRYPTION_HEADER_FILE
#pragma once

#include "blow_crypt.h"
#include "net2_packet.h"
#include "randomizer.h"


namespace mz {
	namespace net2 {

		class server_interface;
		class client_interface;

		class connection_encryption_interface {

		public:

			virtual ~connection_encryption_interface() noexcept {}

			virtual void encrypt(void* Ptr, size_t Size) noexcept { }
			virtual void decrypt(void* Ptr, size_t Size) noexcept { }

			template <size_t N>
			void encrypt(std::span<uint8_t, N> Span) noexcept { encrypt(Span.data(), Span.size()); }

			template <size_t N>
			void decrypt(std::span<uint8_t, N> Span) noexcept { decrypt(Span.data(), Span.size()); }

			virtual int encrypt(packet&) noexcept { return 0; }  
			virtual int decrypt(packet&) noexcept { return 0; }

			virtual int update(packet&) noexcept { return 0; }
			virtual int generate(packet&, mz::randomizer&) noexcept { return 0; }
				
		};


		class server_encryption_interface {

		public:


			server_encryption_interface(server_interface& ServerInterface) noexcept : ServerInterface{ ServerInterface } {}

			virtual void HandshakeWithClient(std::shared_ptr<connection> conn) noexcept;

		protected:

			server_interface& ServerInterface;
		};

		class client_encryption_interface
		{

		public:

			client_encryption_interface(client_interface& ClientInterface) noexcept : ClientInterface{ ClientInterface } {}

			virtual void HandshakeWithServer(std::shared_ptr<connection> conn) noexcept;

		protected:

			client_interface& ClientInterface;

		};



		class connection_bcrypt : public connection_encryption_interface {

		public:

			mz::crypt::blow_fish Fish;

			virtual ~connection_bcrypt() noexcept {}
			connection_bcrypt() noexcept = default;
			connection_bcrypt(mz::crypt::blow_fish& Fish) noexcept : Fish{ Fish } {}

			virtual int encrypt(packet&) noexcept;
			virtual int decrypt(packet&) noexcept;

			virtual void encrypt(void* Ptr, size_t Size) noexcept { Fish.encrypt(Ptr, Size); }
			virtual void decrypt(void* Ptr, size_t Size) noexcept { Fish.decrypt(Ptr, Size); }

			virtual int update(packet&) noexcept;
			virtual int generate(packet&, mz::randomizer&) noexcept;

		};









		class server_bcrypt : public server_encryption_interface {

		public:

			mz::crypt::blow_fish Fish{};
			mz::crypt::blow_pass Text{};
			mz::crypt::blow_pass Code{};
			packet ParamPacket;

			mz::randomizer RandEngine{};

			void generate() noexcept;

			virtual void HandshakeWithClient(std::shared_ptr<connection> conn) noexcept;

			virtual ~server_bcrypt() noexcept {}

			server_bcrypt(server_interface& Server) noexcept : server_encryption_interface{ Server } {}

		};


		class client_bcrypt : public client_encryption_interface {

		public:

			virtual void HandshakeWithServer(std::shared_ptr<connection> conn) noexcept;

			virtual ~client_bcrypt() noexcept {}

			client_bcrypt(client_interface& Client) noexcept : client_encryption_interface{ Client } {}

		};




	}
}



#endif