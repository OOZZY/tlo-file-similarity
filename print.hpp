#ifndef TLOFS_PRINT_HPP
#define TLOFS_PRINT_HPP

#include <ostream>
#include <unordered_map>
#include <vector>

namespace tlo {
template <class Type>
std::ostream &print(std::ostream &ostream, const Type &value) {
  return ostream << value;
}

namespace internal {
template <class SequenceContainer>
std::ostream &printSequenceContainer(std::ostream &ostream,
                                     const SequenceContainer &container) {
  bool first = true;

  ostream << '{';

  for (const auto &item : container) {
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

template <class Type>
std::ostream &print(std::ostream &ostream, const std::vector<Type> &vector) {
  return internal::printSequenceContainer(ostream, vector);
}

namespace internal {
template <class AssociativeContainer>
std::ostream &printAssociativeContainer(std::ostream &ostream,
                                        const AssociativeContainer &container) {
  bool first = true;

  ostream << '{';

  for (const auto &pair : container) {
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

template <class KeyType, class ValueType>
std::ostream &print(std::ostream &ostream,
                    const std::unordered_map<KeyType, ValueType> &map) {
  return internal::printAssociativeContainer(ostream, map);
}
}  // namespace tlo

#endif  // TLOFS_PRINT_HPP
