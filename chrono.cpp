#include "chrono.hpp"

#include <cstring>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace tlo {
namespace {
std::mutex timeMutex;
constexpr std::string_view TIMESTAMP_FORMAT = "%Y-%m-%d %H:%M:%S %j,%w";
}  // namespace

std::string toLocalTimestamp(std::time_t localTime) {
  std::tm localTimeObjectCopy;

  std::unique_lock<std::mutex> timeUniqueLock(timeMutex);
  std::tm *localTimeObject = std::localtime(&localTime);

  std::memcpy(&localTimeObjectCopy, localTimeObject, sizeof(*localTimeObject));
  timeUniqueLock.unlock();

  std::ostringstream oss;

  oss << std::put_time(&localTimeObjectCopy, TIMESTAMP_FORMAT.data());
  oss << ' ' << localTimeObjectCopy.tm_isdst;
  return oss.str();
}

void toTm(std::tm &localTimeObject, const std::string &localTimestamp) {
  std::istringstream iss(localTimestamp);

  iss >> std::get_time(&localTimeObject, TIMESTAMP_FORMAT.data());
  iss >> localTimeObject.tm_isdst;

  if (iss.fail()) {
    throw std::runtime_error("Error: Failed to parse timestamp \"" +
                             localTimestamp + "\".");
  }
}

std::time_t toTimeT(const std::string &localTimestamp) {
  std::tm localTimeObject;

  toTm(localTimeObject, localTimestamp);
  return std::mktime(&localTimeObject);
}
}  // namespace tlo
