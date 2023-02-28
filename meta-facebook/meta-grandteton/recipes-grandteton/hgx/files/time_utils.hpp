// This file was orginally part of openbmc/bmcweb.
// https://github.com/openbmc/bmcweb/blob/master/redfish-core/include/utils/time_utils.hpp
// SPDX-License-Identifier: Apache-2.0
#include <chrono>
#include <tuple>
#include <string>


namespace hgx::time {

template <class IntType>
constexpr std::tuple<IntType, unsigned, unsigned>
    civilFromDays(IntType z) noexcept {
  z += 719468;
  IntType era = (z >= 0 ? z : z - 146096) / 146097;
  unsigned doe = static_cast<unsigned>(z - era * 146097); // [0, 146096]
  unsigned yoe =
      (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
  IntType y = static_cast<IntType>(yoe) + era * 400;
  unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); // [0, 365]
  unsigned mp = (5 * doy + 2) / 153;                      // [0, 11]
  unsigned d = doy - (153 * mp + 2) / 5 + 1;              // [1, 31]
  unsigned m = mp < 10 ? mp + 3 : mp - 9;                 // [1, 12]

  return std::tuple<IntType, unsigned, unsigned>(y + (m <= 2), m, d);
}

inline std::string padZeros(int64_t value, size_t pad)
{
  std::string result(pad, '0');
  for (int64_t val = value; pad > 0; pad--)
  {
      result[pad - 1] = static_cast<char>('0' + val % 10);
      val /= 10;
  }
  return result;
}

template <typename IntType, typename Period>
std::string toISO8061ExtendedStr(std::chrono::duration<IntType, Period> t)
{
  using seconds = std::chrono::duration<int>;
  using minutes = std::chrono::duration<int, std::ratio<60>>;
  using hours = std::chrono::duration<int, std::ratio<3600>>;
  using days = std::chrono::duration<
      IntType, std::ratio_multiply<hours::period, std::ratio<24>>>;

  // d is days since 1970-01-01
  days d = std::chrono::duration_cast<days>(t);

  // t is now time duration since midnight of day d
  t -= d;

  // break d down into year/month/day
  int year = 0;
  int month = 0;
  int day = 0;
  std::tie(year, month, day) = civilFromDays(d.count());
  // Check against limits.  Can't go above year 9999, and can't go below epoch
  // (1970)
  if (year >= 10000) {
    year = 9999;
    month = 12;
    day = 31;
    t = days(1) - std::chrono::duration<IntType, Period>(1);
  } else if (year < 1970) {
    year = 1970;
    month = 1;
    day = 1;
    t = std::chrono::duration<IntType, Period>::zero();
  }

  std::string out;
  out += padZeros(year, 4);
  out += '-';
  out += padZeros(month, 2);
  out += '-';
  out += padZeros(day, 2);
  out += 'T';
  hours hr = std::chrono::duration_cast<hours>(t);
  out += padZeros(hr.count(), 2);
  t -= hr;
  out += ':';

  minutes mt = std::chrono::duration_cast<minutes>(t);
  out += padZeros(mt.count(), 2);
  t -= mt;
  out += ':';

  seconds se = std::chrono::duration_cast<seconds>(t);
  out += padZeros(se.count(), 2);
  t -= se;

  if constexpr (std::is_same_v<typename decltype(t)::period, std::milli>) {
    out += '.';
    using MilliDuration = std::chrono::duration<int, std::milli>;
    MilliDuration subsec = std::chrono::duration_cast<MilliDuration>(t);
    out += padZeros(subsec.count(), 3);
  } else if constexpr (std::is_same_v<typename decltype(t)::period, std::micro>) {
    out += '.';

    using MicroDuration = std::chrono::duration<int, std::micro>;
    MicroDuration subsec = std::chrono::duration_cast<MicroDuration>(t);
    out += padZeros(subsec.count(), 6);
  }

  out += "-08:00"; // PST
  return out;
}

inline std::string now() {
  using DurationType = std::chrono::duration<std::time_t>;
  DurationType sinceEpoch(std::time(nullptr));
  return toISO8061ExtendedStr(sinceEpoch);
}

} // namespace hgx
