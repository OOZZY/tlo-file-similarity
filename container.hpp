#ifndef TLOFS_CONTAINER_HPP
#define TLOFS_CONTAINER_HPP

#include <ostream>
#include <unordered_map>
#include <vector>

namespace tlo {
template <class Value>
std::ostream &print(std::ostream &ostream, const Value &value) {
  return ostream << value;
}

namespace internal {
template <class NonMap>
std::ostream &printNonMap(std::ostream &ostream, const NonMap &nonMap) {
  bool first = true;

  ostream << '{';

  for (const auto &item : nonMap) {
    if (!first) {
      ostream << ", ";
    }

    print(ostream, item);

    if (first) {
      first = false;
    }
  }

  ostream << '}';

  return ostream;
}
}  // namespace internal

template <class Value>
std::ostream &print(std::ostream &ostream, const std::vector<Value> &vector) {
  return internal::printNonMap(ostream, vector);
}

namespace internal {
template <class Map>
std::ostream &printMap(std::ostream &ostream, const Map &map) {
  bool first = true;

  ostream << '{';

  for (const auto &pair : map) {
    if (!first) {
      ostream << ", ";
    }

    print(ostream, pair.first);
    ostream << ": ";
    print(ostream, pair.second);

    if (first) {
      first = false;
    }
  }

  ostream << '}';

  return ostream;
}
}  // namespace internal

template <class Key, class Value>
std::ostream &print(std::ostream &ostream,
                    const std::unordered_map<Key, Value> &map) {
  return internal::printMap(ostream, map);
}
}  // namespace tlo

#endif  // TLOFS_CONTAINER_HPP
