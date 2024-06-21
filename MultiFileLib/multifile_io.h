#ifndef IO_MULTIFILE_HEADER_FILE
#define IO_MULTIFILE_HEADER_FILE
#pragma once

#include <vector>
#include <filesystem>
#include "file_io.h"


namespace mz {
	namespace io {

		int compare(std::filesystem::path const&, std::filesystem::path const&) noexcept;

		class multifile {

			file hdl;
			file copies[5];
			int NumCopies{ 0 };
			int OpenFlags{ 0 };
			
			inline static thread_local std::vector<uint8_t> buffer{};


		public:


			multifile() noexcept = default;
			~multifile() noexcept { close(); }

			bool open(std::filesystem::path const& Path, int flags) noexcept { return open_(Path, file::filter_open(flags)); }
			bool excl(std::filesystem::path const& Path, int flags) noexcept { return open_(Path, file::filter_excl(flags)); }
			bool create(std::filesystem::path const& Path, int flags) noexcept { return open_(Path, file::filter_create(flags)); }

			bool add(std::filesystem::path const& Path) noexcept;


			void close() noexcept;


			//
			// STATE RETRIEVAL
			//


			bool bad() const noexcept;
			bool fail() const noexcept;
			bool good() const noexcept;
			bool is_open() const noexcept;
			bool is_closed() const noexcept;

			int eof() const noexcept;


			error_flags eflags() const noexcept;


			//
			// ALL BASIC OPERATIONS
			//

			bool commit() noexcept;


			int64_t length() const noexcept;

			int64_t size() const noexcept;


			int64_t seek(int64_t Pos, int Dir) const noexcept;

			int64_t tell() const noexcept;


			bool chsize(int64_t NewSize) noexcept;



			int64_t seek_set(int64_t Pos) const noexcept { return seek(Pos, SEEK_SET); }
			int64_t seek_cur(int64_t Pos) const noexcept { return seek(Pos, SEEK_CUR); }
			int64_t seek_end(int64_t Pos) const noexcept { return seek(Pos, SEEK_END); }

			bool bseek_set(int64_t Pos) const noexcept { return seek(Pos, SEEK_SET) < 0; }
			bool bseek_cur(int64_t Pos) const noexcept { return seek(Pos, SEEK_CUR) < 0; }
			bool bseek_end(int64_t Pos) const noexcept { return seek(Pos, SEEK_END) < 0; }





			//
			// WRITE API
			//

			bool write(void const* _Buf, size_t Size) noexcept;


			template <mz::endian::TrivialType T>
			bool write(T const& t) noexcept
			{
				void const* Ptr_{ static_cast<void const*>(&t) };
				return write(Ptr_, sizeof(t));
			}

			template <mz::endian::TrivialType T>
			bool write(T const* P, size_t N) noexcept
			{
				void const* Ptr_{ static_cast<void const*>(P) };
				return write(Ptr_, sizeof(T) * N);
			}

			template <mz::endian::TrivialType T, size_t N>
			bool write(std::span<T, N>  SP) noexcept
			{
				return write(SP.data(), SP.size());
			}

			//
			// ENDIAN WRITE API
			// 

			template <mz::endian::SwapType T>
			bool write_endian(T t) noexcept
			{
				return write(mz::endian::as_endian(t));
			}

			template <mz::endian::SwapType T>
			bool write_endian(T const* Ptr_, size_t N) noexcept
			{
				if constexpr (mz::endian::endian_mismatch)
				{
					buffer.resize(sizeof(T) * N);
					memcpy(buffer.data(), Ptr_, sizeof(T) * N);

					T* P = reinterpret_cast<T*>(buffer.data());
					for (size_t i = 0; i < N; i++) {
						P[i] = mz::endian::as_endian(P[i]);
					}
					return write(P, N);
				}
				else {
					return write(Ptr_, N);
				}
			}

			template <mz::endian::SwapType T, size_t N>
			bool write_endian(std::span<T, N>  SP) noexcept
			{
				return write_endian(SP.data(), SP.size());
			}


			//
			// READ API
			//

			bool read(void* Ptr_, size_t Size) const noexcept;


			template <mz::endian::TrivialTypeNonConst T>
			bool read(T& t) const noexcept
			{
				return read((void*)&t, sizeof(T));
			}

			template <mz::endian::TrivialTypeNonConst T>
			bool read(T* P, size_t N) const noexcept
			{
				return read((void*)P, sizeof(T) * N);
			}

			template <mz::endian::TrivialTypeNonConst T, size_t CNT>
			bool read(std::span<T, CNT> SP) const noexcept
			{
				return read(SP.data(), SP.size());
			}


			//
			// ENDIAN READ API
			//

			template <mz::endian::SwapTypeNonConst T>
			bool read_endian(T& t) const noexcept
			{
				bool Error{ read(t) };
				t = mz::endian::as_endian(t);
				return Error;
			}

			template <mz::endian::SwapTypeNonConst T>
			bool read_endian(T* P, size_t N) const noexcept
			{
				bool Error = read(P, N);
				if (!Error) {
					for (size_t i = 0; i < N; i++) { P[i] = mz::endian::as_endian(P[i]); }
				}
				return Error;
			}


			template <mz::endian::TrivialTypeNonConst T, size_t CNT>
			bool read_endian(std::span<T, CNT> SP) const noexcept
			{
				return read_endian(SP.data(), SP.size());
			}

		private:


			// this operations set bad bits;
			bool open_(std::filesystem::path const& Path, int Flags) noexcept;


		};



	}
}




#endif


