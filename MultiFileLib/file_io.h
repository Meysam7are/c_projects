#ifndef IO_FILE_HEADER_FILE
#define IO_FILE_HEADER_FILE
#pragma once


#ifdef _MSC_VER
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

#include <span>
#include <filesystem>

#include "endian_concepts.h"

namespace mz {
	namespace io {


		struct error_flags {
			union {
				struct {
					uint16_t open : 1;
					uint16_t reopen : 1;
					uint16_t eacces : 1;
					uint16_t eexists : 1;
					uint16_t einval : 1;
					uint16_t emfile : 1;
					uint16_t enoent : 1;
					uint16_t invalid : 1;
					//
					uint16_t commit : 1;
					uint16_t seek : 1;
					uint16_t tell : 1;
					uint16_t read : 1;
					uint16_t write : 1;
					uint16_t corrupt : 1;
				};
				uint16_t value{ 0 };
			};

			constexpr bool good() const noexcept { return !value; }
			constexpr bool fail() const noexcept { return value; }
			constexpr bool bad() const noexcept { return value & 0xff; }
			constexpr bool bad(int fh) noexcept { open = (fh == -1); return bad(); }
			constexpr void reset() noexcept { value &= 0xff; /* clears fail bits. not bad bits. */ }

			constexpr error_flags() noexcept : value{ 0 } {}
			constexpr error_flags(auto Value) noexcept : value{ static_cast<uint16_t>(Value) } {}
			constexpr void fill_open_errors(errno_t err) noexcept {
				value = 0;
				eacces = err & EACCES;		// path is a directory, or open-for-writing a read-only file
				eexists = err & EEXIST;		// _O_CREAT and _O_EXCL, but filename already exists.
				einval = err & EINVAL;		// Invalid oflag, shflag, pmode, or pfh or filename was a null pointer. in chage size if size < 0
				emfile = err & EMFILE;		// No more file descriptors available.
				enoent = err & ENOENT;		// File or path not found
				//eacces = err & EBADF;		// in change size when the file is read only
				//eacces = err & ENOSPC;		// in change size when no space left on disk

			}
		};


		class mfile;
		class multifile;

		template <size_t N>
		class basic_file {

			friend class multifile;
			friend class mfile;

			static constexpr bool READER{ bool(N & 1) };
			static constexpr bool WRITER{ bool(N & 2) };







		public:



			constexpr error_flags eflags() const noexcept { return e_flags; }
			constexpr bool bad() const noexcept { return e_flags.bad(f_handle); }
			constexpr bool fail() const noexcept { return e_flags.fail(); }
			constexpr bool good() const noexcept { return e_flags.good(); }

			constexpr int fd() const noexcept { return f_handle; }
			constexpr bool is_open() const noexcept { return f_handle != -1; }



			//
			// ALL BASIC OPERATIONS
			//

			int64_t length() const noexcept {
				int64_t res = os_size();
				if (res == -1) {
					e_flags.invalid = 1;
				}
				return  res;
			}

			int64_t size() const noexcept { return length(); }

			int eof() const noexcept {
				int res{ os_eof() };
				if (res == -1) {
					e_flags.invalid = 1;
				}
				return res;
			}


			bool commit() noexcept requires (WRITER)
			{
				if (is_open() && good()) {
					int res = os_commit();
					if (res) {
						e_flags.invalid = 1;
						return true;
					}
					return false;
				}
				return true;
			}

			bool chsize(int64_t NewSize) noexcept requires (WRITER)
			{
				auto err = os_chsize(NewSize);
				if (err) {
					e_flags.invalid = 1;
					e_flags.eacces |= err & (EBADF | ENOSPC);
					e_flags.einval |= err & EINVAL;
					return true;
				}
				return false;
			}






			int64_t seek(int64_t Pos, int Dir) const noexcept
			{
				int64_t res = os_seek(Pos, Dir);
				if (res == -1) {
					e_flags.seek = 1;
					e_flags.invalid = 1;
				}
				return res;
			}

			int64_t tell() const noexcept
			{
				int64_t res = os_tell();
				if (res == -1) {
					e_flags.invalid = 1;
					e_flags.tell = 1;
				}
				return res;
			}





			int64_t seek_set(int64_t Pos) const noexcept { return seek(Pos, SEEK_SET); }
			int64_t seek_cur(int64_t Pos) const noexcept { return seek(Pos, SEEK_CUR); }
			int64_t seek_end(int64_t Pos) const noexcept { return seek(Pos, SEEK_END); }

			int64_t bseek_set(int64_t Pos) const noexcept { return seek(Pos, SEEK_SET) == -1; }
			int64_t bseek_cur(int64_t Pos) const noexcept { return seek(Pos, SEEK_CUR) == -1; }
			int64_t bseek_end(int64_t Pos) const noexcept { return seek(Pos, SEEK_END) == -1; }


			bool read(void* _Buf, size_t Size) const noexcept requires (READER)
			{
				auto res = os_read(_Buf, unsigned(Size));
				if (res == Size) {
					return false;
				}
				else {
					e_flags.read = 1;
					if (res == -1) {
						e_flags.invalid = 1;
					}
					return true;
				}
			}


			bool write(void const* _Buf, size_t Size) noexcept requires (WRITER) {
				auto res = os_write(_Buf, unsigned(Size));
				if (res == Size) {
					return false;
				}
				else {
					e_flags.write = 1;
					if (res == -1) {
						e_flags.invalid = 1;
					}
					return true;
				}
			}


			//
			// ALL READ OPERATIONS
			//

			template <mz::endian::TrivialTypeNonConst T>
			bool read(T& t) const noexcept requires (READER)
			{
				return read((void*)&t, sizeof(T));
			}

			template <mz::endian::SwapTypeNonConst T>
			bool read_endian(T& t) const noexcept requires (READER)
			{
				bool Err{ read(t) };
				t = mz::endian::as_endian(t);
				return Err;
			}

			//

			template <mz::endian::TrivialTypeNonConst T>
			bool read(T* P, size_t N) const noexcept requires (READER)
			{
				return read((void*)P, sizeof(T) * N);
			}

			template <mz::endian::SwapTypeNonConst T>
			bool read_endian(T* P, size_t N) const noexcept requires (READER)
			{
				if constexpr (mz::endian::endian_mismatch)
				{
					for (size_t i = 0; i < N && good(); i++) { read_endian(P[i]); }
					return fail();
				}
				else { return read(P, N); }
			}

			template <mz::endian::TrivialTypeNonConst T, size_t N>
			bool read(std::span<T, N> SP) const noexcept requires (READER)
			{
				return read(SP.data(), SP.size());
			}

			template <mz::endian::SwapTypeNonConst T, size_t N>
			bool read_endian(std::span<T, N> SP) const noexcept requires (READER)
			{
				return read_endian(SP.data(), SP.size());
			}


			bool read(std::string& S) const noexcept requires (READER)
			{
				uint32_t Len{ 0 };
				if (read(Len)) return true;
				S.resize(Len);
				if (read(S.data(), Len) || read(Len)) { return true; }
				if (Len != S.size()) { e_flags.corrupt = 1; return true; }
				return false;
			}

			bool read_endian(std::string& S) const noexcept requires (READER)
			{
				uint32_t Len{ 0 };
				if (read_endian(Len)) return true;
				S.resize(Len);
				if (read_endian(S.data(), Len) || read_endian(Len)) { return true; }
				if (Len != S.size()) { e_flags.corrupt = 1; return true; }
				return false;
			}

			//

			bool read(std::wstring& W) const noexcept requires (READER)
			{
				uint32_t Len{ 0 };
				if (read(Len)) return true;
				W.resize(Len);
				if (read(W.data(), Len) || read(Len)) { return true; }
				if (Len != W.size()) { e_flags.corrupt = 1; return true; }
				return false;
			}


			bool read_endian(std::wstring& W) const noexcept requires (READER)
			{
				uint32_t Len{ 0 };
				if (read_endian(Len)) return true;
				W.resize(Len);
				if (read_endian(W.data(), Len) || read_endian(Len)) { return true; }
				if (Len != W.size()) { e_flags.corrupt = 1; return true; }
				return false;
			}


			//
			// ALL WRITE OPERATIONS
			//

			template <mz::endian::TrivialType T>
			bool write(T t) noexcept requires (WRITER)
			{
				return write((void const*)&t, sizeof(t));
			}

			template <mz::endian::SwapType T>
			bool write_endian(T t) noexcept requires (WRITER)
			{
				return write(mz::endian::as_endian(t));
			}

			template <mz::endian::TrivialType T>
			bool write(T const* P, size_t N) noexcept requires (WRITER)
			{
				return write((void const*)P, sizeof(T) * N);
			}


			template <mz::endian::SwapType T>
			bool write_endian(T const* P, size_t N) noexcept requires (WRITER)
			{
				if constexpr (mz::endian::endian_mismatch)
				{
					for (size_t i = 0; i < N && good(); i++) { write_endian(P[i]); }
					return fail();
				}
				else {
					return write(P, N);
				}
			}

			template <mz::endian::TrivialType T, size_t N>
			bool write(std::span<T, N>  SP) noexcept requires (WRITER)
			{
				return write(SP.data(), SP.size());
			}

			template <mz::endian::SwapType T, size_t N>
			bool write_endian(std::span<T, N>  SP) noexcept requires (WRITER)
			{
				return write_endian(SP.data(), SP.size());
			}


			bool write(std::string const& S) noexcept requires (WRITER)
			{
				uint32_t Len = uint32_t(S.size());
				return write(Len) || write(S.data(), S.size()) || write(Len);
			}

			bool write_endian(std::string const& S) noexcept requires (WRITER)
			{
				uint32_t Len = uint32_t(S.size());
				return write_endian(Len) || write_endian(S.data(), S.size()) || write_endian(Len);
			}

			bool write(std::wstring const& W) noexcept requires (WRITER)
			{
				uint32_t Len = uint32_t(W.size());
				return write(Len) || write(W.data(), W.size()) || write(Len);
			}

			bool write_endian(std::wstring const& W) noexcept requires (WRITER)
			{
				uint32_t Len = uint32_t(W.size());
				return write_endian(Len) || write_endian(W.data(), W.size()) || write_endian(Len);
			}

			// these operations set no error bits;
			void close() noexcept requires (WRITER) {
				if (is_open()) {
					if (good()) {
						os_commit();
					}
					os_close();
					f_handle = -1;
				}
			}

			void close() noexcept requires (!WRITER) {
				if (is_open()) {
					os_close();
					f_handle = -1;
				}
			}


		protected:



			basic_file() noexcept : f_handle{ -1 }, e_flags{ 1 } {}
			~basic_file() noexcept { close(); }

			// this operations set bad bits;
			int open(std::filesystem::path const& Path, int Flags) noexcept
			{
				e_flags.value = 0;
				if (f_handle != -1) {
					e_flags.reopen = 1;
					close();
				}
				else {
					auto err = os_open(Path.string().c_str(), Flags);

					e_flags.fill_open_errors(err);
					if (f_handle == -1) {
						e_flags.open = 1;
					}
					else if (err) {
						e_flags.open = 1;
						_close(f_handle);
						f_handle = -1;
					}
				}
				return !good();
			}


		private:
			int f_handle{ -1 };
			mutable error_flags e_flags;



#ifdef _MSC_VER

			int os_eof() const noexcept { return _eof(f_handle); }
			long long os_size() const noexcept { return _filelengthi64(f_handle); }
			long long os_tell() const noexcept { return _telli64(f_handle); }
			long long os_seek(long long Offset, int Origin) const noexcept { return _lseeki64(f_handle, Offset, Origin);  }
			
			int os_chsize(long long Size) noexcept { return _chsize_s(f_handle, Size); }
			int os_commit() noexcept { return _commit(f_handle); }
			int os_close() noexcept { return _close(f_handle); }
			int os_open(std::filesystem::path const& Path, int OperationFlags) noexcept { return _sopen_s(&f_handle, Path.string().c_str(), OperationFlags, _SH_DENYNO, _S_IREAD | _S_IWRITE); }
			
			int os_read(void* _Buf, unsigned Size) const noexcept { return _read(f_handle, _Buf, Size); }
			int os_write(void const* _Buf, unsigned Size) const noexcept { return _write(f_handle, _Buf, Size); }



#else
			long long os_tell() const noexcept { return lseek64(f_handle, 0, SEEK_CUR); }
			long long os_seek(long long Offset, int Origin) const noexcept { return lseek64(f_handle, Offset, Origin); }

			int os_chsize(long long Size) noexcept { return ftruncate(f_handle, Size); }
			int os_commit() noexcept { return fsync(f_handle); }
			int os_close() noexcept { return close(f_handle); }
			int os_open(std::filesystem::path const& Path, int OperationFlags) noexcept { return open(&f_handle, Path.string().c_str(), OperationFlags); }

			int os_read(void* _Buf, unsigned Size) const noexcept { return read(f_handle, _Buf, Size); }
			int os_write(void const* _Buf, unsigned Size) const noexcept { return write(f_handle, _Buf, Size); }

			long long os_size() const noexcept { 
				struct stat buf;
				if (fstat(fd, &buf)) return -1;
				return buf.st_size;
			}

			int os_eof() const noexcept {
				uint8_t dummy;
				ssize_t bytes_read = read(fd, &dummy, 1);
				if (bytes_read == 0) { return 1; }
				if (bytes_read == -1) { return -1; }
				if (lseek64(fd, -1, SEEK_CUR) == -1) { return -1; }
				return 0;
			}
#endif


		};




		class ifile : public basic_file<1> {

		public:

			ifile() noexcept = default;
			ifile(std::filesystem::path const& Path, int flags) { basic_file::open(Path, filter_open(flags)); }
			bool open(std::filesystem::path const& Path, int flags) noexcept { return basic_file::open(Path, filter_open(flags)) == 0; }

		private:

			static constexpr int filter_open(int flags) noexcept {
				return (flags | O_RDONLY) & ~(O_RDWR | _O_TRUNC | O_WRONLY | _O_APPEND | O_CREAT | O_EXCL);
			}

		};



		class ofile : public basic_file<2> {

		public:

			ofile() noexcept = default;

			bool open(std::filesystem::path const& Path, int flags) noexcept { return basic_file::open(Path, filter_open(flags)) == 0; }
			bool excl(std::filesystem::path const& Path, int flags) noexcept { return basic_file::open(Path, filter_excl(flags)) == 0; }
			bool create(std::filesystem::path const& Path, int flags) noexcept { return basic_file::open(Path, filter_create(flags)) == 0; }

		private:

			static constexpr int filter_open(int flags) noexcept {
				return (flags | O_WRONLY) & ~(O_RDWR | O_CREAT | O_EXCL);
			}

			static constexpr int filter_excl(int flags) noexcept {
				return (flags | O_WRONLY | O_CREAT | O_EXCL) & ~O_RDWR;
			}

			static constexpr int filter_create(int flags) noexcept {
				return (flags | O_WRONLY | O_CREAT) & ~(O_RDWR | O_EXCL);
			}


		};




		class file : public basic_file<3> {

			friend class mfile;
			friend class multifile;
			bool open_(std::filesystem::path const& Path, int flags) noexcept { return basic_file::open(Path, flags) == 0; }


		public:

			bool open(std::filesystem::path const& Path, int flags) noexcept { return basic_file::open(Path, filter_open(flags)) == 0; }
			bool excl(std::filesystem::path const& Path, int flags) noexcept { return basic_file::open(Path, filter_excl(flags)) == 0; }
			bool create(std::filesystem::path const& Path, int flags) noexcept { return basic_file::open(Path, filter_create(flags)) == 0; }

			file() noexcept = default;

		private:

			static constexpr int filter_open(int flags) noexcept {
				return (flags | O_RDWR) & ~(_O_RDONLY | O_WRONLY & O_CREAT & O_EXCL);
			}

			static constexpr int filter_excl(int flags) noexcept {
				return (flags | O_RDWR | O_CREAT | O_EXCL) & ~(_O_RDONLY | O_WRONLY);
			}

			static constexpr int filter_create(int flags) noexcept {
				return (flags | O_RDWR | O_CREAT) & ~(_O_RDONLY | O_WRONLY & O_EXCL);
			}
		};







	}
}






#endif


