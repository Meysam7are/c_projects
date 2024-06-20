#ifndef MZ_TIME_CONVERSIONS_HEADER_FILE
#define MZ_TIME_CONVERSIONS_HEADER_FILE
#pragma once
#include <string>
#include <format>
#include <cstdint>

#include <type_traits>
#include <concepts>
#include <chrono>

#include "encode64.h"

#include <limits>

/*
*	Helper functions to use std::chrono
* 
*	Author: Meysam Zare
*	Last Modified: 6/20/24
*/


namespace mz {

	namespace time {

		using namespace std::chrono;
		using syspoint = time_point<system_clock, system_clock::duration>;
		using secpoint = time_point<system_clock, seconds>;
		using millipoint = time_point<system_clock, milliseconds>;
		using micropoint = time_point<system_clock, microseconds>;

		inline constexpr secpoint SecPoint(int64_t TimeSinceEpoch) noexcept { return secpoint{ seconds{TimeSinceEpoch} }; }
		inline constexpr millipoint MilliPoint(int64_t TimeSinceEpoch) noexcept { return millipoint{ milliseconds{TimeSinceEpoch} }; }
		inline constexpr micropoint MicroPoint(int64_t TimeSinceEpoch) noexcept { return micropoint{ microseconds{TimeSinceEpoch} }; }

		template <class CL_, class DUR_>
		constexpr syspoint ToSystemTimePoint(time_point<CL_, DUR_> DateTime) noexcept { return clock_cast<system_clock, CL_, DUR_>(DateTime); }

		template <class CL_, class DUR_>
		constexpr secpoint ToSecondTimePoint(time_point<CL_, DUR_> DateTime) noexcept { return time_point_cast<seconds>(ToSystemTimePoint(DateTime)); }

		template <class CL_, class DUR_>
		constexpr millipoint ToMillisecondTimePoint(time_point<CL_, DUR_> DateTime) noexcept { return time_point_cast<milliseconds>(ToSystemTimePoint(DateTime)); }

		template <class CL_, class DUR_>
		constexpr micropoint ToMicrosecondTimePoint(time_point<CL_, DUR_> DateTime) noexcept { return time_point_cast<microseconds>(ToSystemTimePoint(DateTime)); }

		template <class CL_, class DUR_>
		constexpr int64_t SecondSinceEpoch(time_point<CL_, DUR_> DateTime) noexcept { return time_point_cast<seconds>(ToSystemTimePoint(DateTime)).time_since_epoch().count(); }

		template <class CL_, class DUR_>
		constexpr int64_t MillisecondSinceEpoch(time_point<CL_, DUR_> DateTime) noexcept { return time_point_cast<milliseconds>(ToSystemTimePoint(DateTime)).time_since_epoch().count(); }

		template <class CL_, class DUR_>
		constexpr int64_t MicrosecondSinceEpoch(time_point<CL_, DUR_> DateTime) noexcept { return time_point_cast<microseconds>(ToSystemTimePoint(DateTime)).time_since_epoch().count(); }



		template <class CL_, class  DUR_>
		[[nodiscard]] std::string DateString(time_point<CL_, DUR_> DateTime) noexcept { return std::format("{:%Y-%m-%d}", SecPoint(DateTime)); }

		template <class CL_, class  DUR_>
		[[nodiscard]] std::string TimeString(time_point<CL_, DUR_> DateTime) noexcept { return std::format("{:%Y-%m-%d %H:%M:%S}", SecPoint(DateTime)); }

		template <class CL_, class  DUR_>
		[[nodiscard]] std::string MilliString(time_point<CL_, DUR_> DateTime) noexcept { return std::format("{:%Y-%m-%d %H:%M:%S}", MilliPoint(DateTime)); }

		template <class CL_, class  DUR_>
		[[nodiscard]] std::string MicroString(time_point<CL_, DUR_> DateTime) noexcept { return std::format("{:%Y-%m-%d %H:%M:%S}", MicroPoint(DateTime)); }

		template <class CL_, class  DUR_>
		[[nodiscard]] std::string SysString(time_point<CL_, DUR_> DateTime) noexcept { return std::format("{:%Y-%m-%d %H:%M:%S}", SysPoint(DateTime)); }



		inline int64_t now2hours() { return time_point_cast<hours>(system_clock::now()).time_since_epoch().count(); }
		inline int64_t now2minutes() { return time_point_cast<minutes>(system_clock::now()).time_since_epoch().count(); }
		inline int64_t now2seconds() { return time_point_cast<seconds>(system_clock::now()).time_since_epoch().count(); }
		inline int64_t now2milliseconds() { return time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count(); }
		inline int64_t now2microseconds() { return time_point_cast<microseconds>(system_clock::now()).time_since_epoch().count(); }


		inline time_point<system_clock> hours2now(int64_t MS) { return time_point<system_clock>{hours{ MS }}; }
		inline time_point<system_clock> minutes2now(int64_t MS) { return time_point<system_clock>{minutes{ MS }}; }
		inline time_point<system_clock> seconds2now(int64_t MS) { return time_point<system_clock>{seconds{ MS }}; }
		inline time_point<system_clock> milliseconds2now(int64_t MS) { return time_point<system_clock>{milliseconds{ MS }}; }
		inline time_point<system_clock> microseconds2now(int64_t MS) { return time_point<system_clock>{microseconds{ MS }}; }


		inline int64_t steady2hour() { return time_point_cast<hours>(steady_clock::now()).time_since_epoch().count(); }
		inline int64_t steady2minutes() { return time_point_cast<minutes>(steady_clock::now()).time_since_epoch().count(); }
		inline int64_t steady2seconds() { return time_point_cast<seconds>(steady_clock::now()).time_since_epoch().count(); }
		inline int64_t steady2milliseconds() { return time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count(); }
		inline int64_t steady2microseconds() { return time_point_cast<microseconds>(steady_clock::now()).time_since_epoch().count(); }


		inline time_point<steady_clock> hours2steady(int64_t MS) { return time_point<steady_clock>{hours{ MS }}; }
		inline time_point<steady_clock> minutes2steady(int64_t MS) { return time_point<steady_clock>{minutes{ MS }}; }
		inline time_point<steady_clock> seconds2steady(int64_t MS) { return time_point<steady_clock>{seconds{ MS }}; }
		inline time_point<steady_clock> milliseconds2steady(int64_t MS) { return time_point<steady_clock>{milliseconds{ MS }}; }
		inline time_point<steady_clock> microseconds2steady(int64_t MS) { return time_point<steady_clock>{microseconds{ MS }}; }




		template <typename _Duration>
		struct systime {

			static_assert(std::chrono::_Is_duration_v<_Duration>,
				"N4950 [time.point.general]/1 mandates Duration to be a specialization of chrono::duration.");


			static constexpr int64_t MIN{ LLONG_MIN };
			static constexpr int64_t MAX{ LLONG_MAX };

			using duration = _Duration;
			using clock = std::chrono::system_clock;
			using clock_tp = std::chrono::time_point<clock, duration>;

			template <class _Clock, class _Dur>
			static constexpr int64_t count(std::chrono::time_point<_Clock, _Dur> const& tp)
			{
				return std::chrono::time_point_cast<duration>(std::chrono::clock_cast<clock, _Clock, _Dur>(tp)).time_since_epoch().count();
			}


			constexpr systime() noexcept = default;

			constexpr systime(int64_t TSEP) noexcept : tsep{ TSEP } {};
			constexpr systime& operator = (int64_t TSEP) noexcept { tsep = TSEP; return *this; }

			template <class _Clock, class _Dur>
			explicit constexpr systime(std::chrono::time_point<_Clock, _Dur> const& t) noexcept : tsep{ count(t) } {}

			template <class _Dur>
			explicit constexpr systime(systime<_Dur> const& t) noexcept : tsep{ count(t.stp()) } {}


			template <class _Clock, class _Dur>
			constexpr systime& operator = (std::chrono::time_point<_Clock, _Dur> const& t) noexcept { tsep = count(t); return *this; }


			static constexpr systime now() noexcept { return systime{ std::chrono::time_point_cast<duration>(clock::now()) }; }
			static constexpr systime TimeFromNow(int64_t Delay) noexcept { return systime{ now().tsep + Delay }; }

			constexpr void clear() noexcept { tsep = MIN; }
			constexpr bool exists() const noexcept { return tsep != MIN; }

			constexpr clock_tp stp() const noexcept { return clock_tp{ duration{ tsep } }; }
			constexpr time_t to_time_t() const noexcept { return std::chrono::system_clock::to_time_t(stp()); }

			constexpr std::string encode64() const noexcept { return mz::encoder64::ToString(tsep); }


			std::string string() const noexcept
			{
				return std::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::time_point_cast<duration>(stp()));
			}




			std::wstring wstring() const noexcept
			{
				return std::format(L"{:%Y-%m-%d %H:%M:%S}", std::chrono::time_point_cast<duration>(stp()));
			}

			std::string string_underlined() const noexcept
			{
				return std::format("{:%Y_%m_%d__%H_%M_%S}", std::chrono::time_point_cast<duration>(stp()));
			}

			std::wstring wstring_underlined() const noexcept {
				return std::format(L"{:%Y_%m_%d__%H_%M_%S}"
					, std::chrono::time_point_cast<duration>(stp()));
			}

			int64_t tsep{ MIN };

			template <typename _Dur>
			constexpr systime& operator += (_Dur d) noexcept {
				tsep = systime{ clock_tp{ duration{ tsep } + d } }.tsep;
				return *this;
			}

			template <typename _Dur>
			constexpr systime& operator -= (_Dur d) noexcept {
				tsep = systime{ clock_tp{ duration{ tsep } - d } }.tsep;
				return *this;
			}

			friend constexpr bool operator < (systime L, systime R) noexcept { return L.tsep < R.tsep; }
			friend constexpr bool operator == (systime L, systime R) noexcept { return L.tsep == R.tsep; }
			friend constexpr duration operator - (systime L, systime R) noexcept { return duration{ L.tsep - R.tsep }; }



			template <typename _Dur>
			friend constexpr systime operator + (systime L, _Dur R) noexcept {
				return systime{ clock_tp{ duration{ L.tsep } + R } };
			}

			template <typename _Dur>
			friend constexpr systime operator - (systime L, _Dur R) noexcept {
				return systime{ clock_tp{ duration{ L.tsep } - R } };
			}

		};

		using secondtime = systime<std::chrono::seconds>;
		using millitime = systime<std::chrono::milliseconds>;
		using microtime = systime<std::chrono::microseconds>;
		using nanotime = systime<std::chrono::nanoseconds>;









		template <typename _Duration>
		struct steadytime {

			static_assert(std::chrono::_Is_duration_v<_Duration>,
				"N4950 [time.point.general]/1 mandates Duration to be a specialization of chrono::duration.");


			static constexpr int64_t MIN{ LLONG_MIN };
			static constexpr int64_t MAX{ LLONG_MAX };


			using duration = _Duration;
			using clock = std::chrono::steady_clock;
			using clock_tp = std::chrono::time_point<clock, duration>;
			using sysclock = std::chrono::system_clock;
			using sysclock_tp = std::chrono::time_point<sysclock, duration>;


			template <class _Clock, class _Dur>
			static constexpr int64_t count(std::chrono::time_point<_Clock, _Dur> const& tp)
			{
				return std::chrono::time_point_cast<duration>(std::chrono::clock_cast<clock, _Clock, _Dur>(tp)).time_since_epoch().count();
			}


			constexpr steadytime() noexcept = default;
			explicit constexpr steadytime(int64_t TSEP) noexcept : tsep{ TSEP } {};
			constexpr steadytime& operator = (int64_t TSEP) noexcept { tsep = TSEP; return *this; }


			template <class _Clock, class _Dur>
			explicit constexpr steadytime(std::chrono::time_point<_Clock, _Dur> const& t) noexcept : tsep{ count(t) } {}

			template <class _Clock, class _Dur>
			constexpr steadytime& operator = (std::chrono::time_point<_Clock, _Dur> const& t) noexcept { tsep = count(t); return *this; }


			static constexpr steadytime Now() noexcept { return steadytime{ std::chrono::time_point_cast<duration>(clock::now()) }; }
			//static constexpr steadytime Min() noexcept { return steadytime{ clock_tp::min() }; }
			//static constexpr steadytime Max() noexcept { return steadytime{ clock_tp::max() }; }

			constexpr void clear() noexcept { tsep = MIN; }
			constexpr bool exists() const noexcept { return tsep != MIN; }

			constexpr sysclock_tp stp() const noexcept { return sysclock_tp{ duration{ tsep } }; }

			constexpr time_t to_time_t() const noexcept { return std::chrono::system_clock::to_time_t(stp()); }

			constexpr std::string encode64() const noexcept { return mz::encoder64::ToString(tsep); }

			std::string string() const noexcept {
				return std::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::time_point_cast<duration>(stp()));
			}

			std::wstring wstring() const noexcept {
				return std::format(L"{:%Y-%m-%d %H:%M:%S}"
					, std::chrono::time_point_cast<duration>(stp()));
			}

			std::string string_name() const noexcept {

				return std::format("{:%Y_%m_%d__%H_%M_%S}"
					, std::chrono::time_point_cast<duration>(stp()));
			}

			std::wstring wstring_name() const noexcept {
				return std::format(L"{:%Y_%m_%d__%H_%M_%S}"
					, std::chrono::time_point_cast<duration>(stp()));
			}

			int64_t tsep{ 0 };

			template <typename _Dur>
			constexpr steadytime& operator += (_Dur d) noexcept {
				tsep = steadytime{ clock_tp{ duration{ tsep } + d } }.tsep;
				return *this;
			}

			template <typename _Dur>
			constexpr steadytime& operator -= (_Dur d) noexcept {
				tsep = steadytime{ clock_tp{ duration{ tsep } - d } }.tsep;
				return *this;
			}

			friend constexpr bool operator < (steadytime L, steadytime R) noexcept { return L.tsep < R.tsep; }
			friend constexpr bool operator == (steadytime L, steadytime R) noexcept { return L.tsep == R.tsep; }
			friend constexpr duration operator - (steadytime L, steadytime R) noexcept { return duration{ L.tsep - R.tsep }; }


			template <typename _Dur>
			friend constexpr steadytime operator + (steadytime L, _Dur R) noexcept {
				return steadytime{ clock_tp{ duration{ L.tsep } + R } };
			}

			template <typename _Dur>
			friend constexpr steadytime operator - (steadytime L, _Dur R) noexcept {
				return steadytime{ clock_tp{ duration{ L.tsep } - R } };
			}



		};




	}
};








#endif