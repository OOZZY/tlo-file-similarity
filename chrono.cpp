#include "chrono.hpp"

#include <cstring>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace tlo {
namespace {
std::mutex timeMutex;
const char *TIMESTAMP_FORMAT = "%Y-%m-%d %H:%M:%S";
}  // namespace

std::string toUtcTimestamp(std::time_t localTime) {
  std::tm utcTimeObjectCopy;
  std::unique_lock<std::mutex> timeUniqueLock(timeMutex);

  std::tm *utcTimeObject = std::gmtime(&localTime);
  std::memcpy(&utcTimeObjectCopy, utcTimeObject, sizeof(*utcTimeObject));
  timeUniqueLock.unlock();

  std::ostringstream oss;

  oss << std::put_time(&utcTimeObjectCopy, TIMESTAMP_FORMAT);
  return oss.str();
}

std::time_t fromUtcTimestamp(const std::string &utcTimestamp) {
  std::istringstream iss(utcTimestamp);
  std::tm utcTimeObject;

  iss >> std::get_time(&utcTimeObject, TIMESTAMP_FORMAT);

  if (iss.fail()) {
    throw std::runtime_error("Error: Failed to parse timestamp \"" +
                             utcTimestamp + "\".");
  }

  utcTimeObject.tm_isdst = 0;

  std::time_t utcTime = std::mktime(&utcTimeObject);
  std::time_t localNow = std::time(nullptr);

  std::unique_lock<std::mutex> timeUniqueLock(timeMutex);

  std::time_t utcNow = std::mktime(std::gmtime(&localNow));
  timeUniqueLock.unlock();

  if (utcNow > localNow) {
    return utcTime - (utcNow - localNow);
  } else if (utcNow < localNow) {
    return utcTime + (localNow - utcNow);
  } else {
    return utcTime;
  }
}
}  // namespace tlo
