#ifndef TLOFS_CHRONO_HPP
#define TLOFS_CHRONO_HPP

#include <cassert>
#include <chrono>
#include <ctime>
#include <limits>
#include <string>

namespace tlo {
namespace internal {
template <class Duration, class Rep = typename Duration::rep>
constexpr Duration maxDuration() noexcept {
  return Duration{std::numeric_limits<Rep>::max()};
}

template <class Duration>
constexpr Duration absDuration(const Duration d) noexcept {
  return Duration{(d.count() < 0) ? -d.count() : d.count()};
}
}  // namespace internal

// Convert from one type of std::chrono::time_point to another, even if they
// are on different clocks. The conversion may have some error. Based on code
// from https://stackoverflow.com/questions/35282308/convert-between-c11-clocks
template <typename DstTimePoint, typename SrcTimePoint,
          typename DstDuration = typename DstTimePoint::duration,
          typename SrcDuration = typename SrcTimePoint::duration,
          typename DstClock = typename DstTimePoint::clock,
          typename SrcClock = typename SrcTimePoint::clock>
DstTimePoint convertTimePoint(
    const SrcTimePoint timePoint,
    const SrcDuration tolerance = std::chrono::nanoseconds{100},
    const int limit = 10) {
  assert(limit > 0);

  SrcTimePoint srcNow;
  DstTimePoint dstNow;
  auto epsilon = internal::maxDuration<SrcDuration>();

  for (auto i = 0; epsilon > tolerance && i < limit; ++i) {
    const auto srcBefore = SrcClock::now();
    const auto dstBetween = DstClock::now();
    const auto srcAfter = SrcClock::now();
    const auto srcDiff = srcAfter - srcBefore;
    const auto delta = internal::absDuration(srcDiff);

    if (delta < epsilon) {
      srcNow = srcBefore + srcDiff / 2;
      dstNow = dstBetween;
      epsilon = delta;
    }
  }

  return dstNow + (timePoint - srcNow);
}

// Returns a UTC timestamp in "%Y-%m-%d %H:%M:%S" format. Assumes localTime is
// in local system time. Thread-safe.
std::string toUtcTimestamp(std::time_t localTime);

// Returns a std::time_t in local system time. Assumes utcTimestamp is a UTC
// timestamp in "%Y-%m-%d %H:%M:%S" format. Thread-safe.
std::time_t fromUtcTimestamp(const std::string &utcTimestamp);
}  // namespace tlo

#endif  // TLOFS_CHRONO_HPP
