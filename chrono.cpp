#include "chrono.hpp"

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace tlo {
namespace {
std::mutex timeMutex;
constexpr int BASE_YEAR = 1900;
}  // namespace

std::string toLocalTimestamp(std::time_t localTime) {
  std::tm localTimeObjectCopy;

  std::unique_lock<std::mutex> timeUniqueLock(timeMutex);
  std::tm *localTimeObject = std::localtime(&localTime);

  std::memcpy(&localTimeObjectCopy, localTimeObject, sizeof(*localTimeObject));
  timeUniqueLock.unlock();

  localTimeObjectCopy.tm_year += BASE_YEAR;
  localTimeObjectCopy.tm_mon++;

  std::ostringstream oss;

  oss.fill('0');
  oss << std::setw(4) << localTimeObjectCopy.tm_year;
  oss << std::setw(1) << '-';
  oss << std::setw(2) << localTimeObjectCopy.tm_mon;
  oss << std::setw(1) << '-';
  oss << std::setw(2) << localTimeObjectCopy.tm_mday;
  oss << std::setw(1) << ' ';
  oss << std::setw(2) << localTimeObjectCopy.tm_hour;
  oss << std::setw(1) << ':';
  oss << std::setw(2) << localTimeObjectCopy.tm_min;
  oss << std::setw(1) << ':';
  oss << std::setw(2) << localTimeObjectCopy.tm_sec;
  oss << std::setw(1) << ' ';
  oss << std::setw(3) << localTimeObjectCopy.tm_yday;
  oss << std::setw(1) << ' ';
  oss << std::setw(1) << localTimeObjectCopy.tm_wday;
  oss << std::setw(1) << ' ';
  oss << std::setw(1) << localTimeObjectCopy.tm_isdst;
  return oss.str();
}

void toTm(std::tm &localTimeObject, const std::string &localTimestamp) {
  std::istringstream iss(localTimestamp);

  iss >> localTimeObject.tm_year;
  iss.ignore(1);
  iss >> localTimeObject.tm_mon;
  iss.ignore(1);
  iss >> localTimeObject.tm_mday;
  iss.ignore(1);
  iss >> localTimeObject.tm_hour;
  iss.ignore(1);
  iss >> localTimeObject.tm_min;
  iss.ignore(1);
  iss >> localTimeObject.tm_sec;
  iss.ignore(1);
  iss >> localTimeObject.tm_yday;
  iss.ignore(1);
  iss >> localTimeObject.tm_wday;
  iss.ignore(1);
  iss >> localTimeObject.tm_isdst;

  if (iss.fail()) {
    throw std::runtime_error("Error: Failed to parse timestamp \"" +
                             localTimestamp + "\".");
  }

  localTimeObject.tm_year -= BASE_YEAR;
  localTimeObject.tm_mon--;
}

std::time_t toTimeT(const std::string &localTimestamp) {
  std::tm localTimeObject;

  toTm(localTimeObject, localTimestamp);
  return std::mktime(&localTimeObject);
}

namespace {
constexpr int SECONDS_IN_MINUTE = 60;
constexpr int SECONDS_IN_HOUR = 60 * SECONDS_IN_MINUTE;
constexpr long SECONDS_IN_DAY = 24 * SECONDS_IN_HOUR;
constexpr long SECONDS_IN_YEAR = 365 * SECONDS_IN_DAY;
constexpr int YEARS_SINCE_BASE_DURING_EPOCH = 70;

// Calculates the number of seconds since the Epoch as defined by POSIX.
std::intmax_t secondsSinceTheEpoch(const std::tm &localTimeObject) {
  return localTimeObject.tm_sec + localTimeObject.tm_min * SECONDS_IN_MINUTE +
         localTimeObject.tm_hour * SECONDS_IN_HOUR +
         localTimeObject.tm_yday * SECONDS_IN_DAY +
         (localTimeObject.tm_year - YEARS_SINCE_BASE_DURING_EPOCH) *
             SECONDS_IN_YEAR +
         ((localTimeObject.tm_year - 69) / 4) * SECONDS_IN_DAY -
         ((localTimeObject.tm_year - 1) / 100) * SECONDS_IN_DAY +
         ((localTimeObject.tm_year + 299) / 400) * SECONDS_IN_DAY;
}
}  // namespace

bool equalTimestamps(const std::string &localTimestamp1,
                     const std::string &localTimestamp2,
                     int maxSecondDifference) {
  if (localTimestamp1 == localTimestamp2) {
    return true;
  }

  std::tm localTimeObject1;
  std::tm localTimeObject2;

  toTm(localTimeObject1, localTimestamp1);
  toTm(localTimeObject2, localTimestamp2);

  std::intmax_t seconds1 = secondsSinceTheEpoch(localTimeObject1);
  std::intmax_t seconds2 = secondsSinceTheEpoch(localTimeObject2);

  if (seconds1 > seconds2) {
    return (seconds1 - seconds2) <= maxSecondDifference;
  } else if (seconds2 > seconds1) {
    return (seconds2 - seconds1) <= maxSecondDifference;
  } else {
    return true;
  }
}
}  // namespace tlo
