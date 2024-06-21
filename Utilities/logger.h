#ifndef MZ_LOGGER_HEADER_FILE
#define MZ_LOGGER_HEADER_FILE
#pragma once
#include <string>
#include <format>
#include <chrono>
#include <fstream>
#include <filesystem>


namespace mz {



	class logger
	{
		std::ofstream FS{};
		std::string Hdr{};
	public:

		int Start(std::filesystem::path const& Path, int Mode = std::ios::app) noexcept {
			if (FS.is_open()) { return 1; }
			FS.open(Path.string(), std::ios::out | std::ios::binary | Mode);
			if (FS.is_open()) { return 0; }
			return -1;
		}

		logger() noexcept {};
		logger(size_t Indent) noexcept : Hdr{ std::format("{:>{}}", "", Indent) } {};
		logger(std::string Header) noexcept : Hdr{ Header } {};

		logger& operator << (std::string const& Msg) noexcept {
			FS << std::format("{:%Y-%m-%d %H:%M:%S} ", std::chrono::system_clock::now()) << Hdr << Msg << '\n';
			return *this;
		}

	};

	inline static logger ErrLog{};
	inline static logger WarLog{};
	inline static logger EvtLog{};
};


#endif