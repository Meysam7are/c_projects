#include <format>
#include <iostream>

#include "multifile_io.h"
#include "time_conversions.h"


namespace mz {
	namespace io {

		bool multifile::open_(std::filesystem::path const& Path, int Flags) noexcept
		{
			NumCopies = 0;
			OpenFlags = Flags;
			return hdl.open_(Path, Flags);
		}



		bool multifile::add(std::filesystem::path const& Path) noexcept
		{
			if (NumCopies < 0 || NumCopies > 5) { return false; }
			if (!copies[NumCopies].open_(Path, OpenFlags)) { return false; }
			++NumCopies;
			return true;
		}


		void multifile::close() noexcept {
			hdl.close();
			for (int i = 0; i < NumCopies; i++) { copies[i].close(); }
		}


		//
		// STATE RETRIEVAL
		//


		bool multifile::bad() const noexcept { 
			bool B{ hdl.bad() };
			for (int i = 0; i < NumCopies; i++) { B |= copies[i].bad(); }
			return B;		
		}

		bool multifile::fail() const noexcept { 
			bool B{ hdl.fail() }; 
			for (int i = 0; i < NumCopies; i++) { B |= copies[i].fail(); }
			return B;
		}

		bool multifile::good() const noexcept { 
			bool B{ hdl.good() }; 
			for (int i = 0; i < NumCopies; i++) { B &= copies[i].good(); }
			return B;
		}

		bool multifile::is_open() const noexcept { 
			bool B{ hdl.is_open() }; 
			for (int i = 0; i < NumCopies; i++) { B &= copies[i].is_open(); }
			return B;
 
		}
		
		bool multifile::is_closed() const noexcept { 
			bool B{ hdl.is_open() }; 
			for (int i = 0; i < NumCopies; i++) { B |= copies[i].is_open(); }
			return B;
		}

		int multifile::eof() const noexcept { 
			int R{ hdl.eof() }; 
			for (int i = 0; i < NumCopies; i++) { R = copies[i].eof() != R ? -1 : R; }
			return R;
		}


		error_flags multifile::eflags() const noexcept { 
			error_flags fl{ hdl.eflags() }; 
			for (int i = 0; i < NumCopies; i++) { fl.value |= copies[i].e_flags.value; }
			return fl;
		}


		//
		// ALL BASIC OPERATIONS
		//

		bool multifile::commit() noexcept
		{
			bool B{ hdl.commit() };
			for (int i = 0; i < NumCopies; i++) { B &= copies[i].commit(); }
			return B;
		}


		int64_t multifile::length() const noexcept
		{
			int64_t L{ hdl.length() };
			for (int i = 0; i < NumCopies; i++) {
				L = copies[i].length() != L ? -1 : L;
			}
			return L;
		}

		int64_t multifile::size() const noexcept { return length(); }


		int64_t multifile::seek(int64_t Pos, int Dir) const noexcept
		{
			int64_t P{ hdl.seek(Pos, Dir) };

			for (int i = 0; i < NumCopies; i++) {
				P = P != copies[i].seek(Pos, Dir) ? -1 : P;
			}
			return P;
		}


		int64_t multifile::tell() const noexcept
		{
			int64_t P{ hdl.tell() };
			for (int i = 0; i < NumCopies; i++) { P = P != copies[i].tell() ? -1 : P; }
			return P;
		}

		bool multifile::chsize(int64_t NewSize) noexcept
		{
			bool Error{ hdl.chsize(NewSize) };
			for (int i = 0; i < NumCopies; i++) {
				Error |= copies[i].chsize(NewSize);
			}
			return Error;
		}



		bool multifile::write(void const* _Buf, size_t Size) noexcept
		{
			bool Error = hdl.write(_Buf, Size);
			for (int i = 0; i < NumCopies; i++) {
				Error = copies[i].write(_Buf, Size) || Error;
			}
			return Error;
		}

		bool multifile::read(void* Ptr_, size_t Size) const noexcept
		{
			buffer.resize(Size);
			void* Buf_ = static_cast<void*>(buffer.data());

			bool Error = hdl.read(Ptr_, Size) || hdl.fail();
			for (int i = 0; i < NumCopies; i++) {
				Error = copies[i].read(Buf_, Size) || copies[i].fail() || Error;
				Error = memcmp(Ptr_, Buf_, Size) != 0 || Error;
			}
			return Error;
		}




		int compare(std::filesystem::path const& P1, std::filesystem::path const& P2) noexcept
		{
			mz::io::ifile F1, F2;
			std::vector<uint8_t> V1(4096);
			std::vector<uint8_t> V2(4096);
			if (!F1.open(P1, O_BINARY)) { return -1; }
			if (!F2.open(P2, O_BINARY)) { return -1; }
			int64_t L1 = F1.length();
			int64_t L2 = F2.length();
			if (L1 < L2) { return -1; }
			if (L1 > L2) { return -1; }
			int64_t cnt{ 0 };
			while (L1)
			{
				int64_t L = L1 > 4096 ? 4096 : L1;
				if (F1.read(V1.data(), L)) { return -1; }
				if (F2.read(V2.data(), L)) { return -1; }
				for (int64_t i = 0; i < L; i++) {
					if (V1[i] != V2[i]) {
						++cnt;
						int64_t j = L2 - L1 + i;
						std::cout << std::format("V1[{}]:{}  V2[{}]:{}  cnt:{}  \n", j, V1[i], j, V2[i], cnt);
						
					}
				}
				L1 -= L;
			}
			return int(cnt);
		}










	}
}


