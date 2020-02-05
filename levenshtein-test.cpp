#include "levenshtein.hpp"

#include <iostream>
#include <string>

template <class T1, class T2>
bool equal(const T1 &obj1, const T2 &obj2) {
  return obj1 == obj2;
}

namespace {
bool passed = true;
}  // namespace

#define EXPECT(condition)                                        \
  do {                                                           \
    if (!(condition)) {                                          \
      std::cout << "Expect failed: " << #condition << std::endl; \
      passed = false;                                            \
    }                                                            \
  } while (0)

int main() {
  using namespace std::string_literals;

  EXPECT(equal(tlo::levenshteinDistance1("sitting"s, "kitten"s), 3U));
  EXPECT(equal(tlo::levenshteinDistance1("Sunday"s, "Saturday"s), 3U));
  EXPECT(equal(tlo::levenshteinDistance1("CA"s, "ABC"s), 3U));

  EXPECT(equal(tlo::levenshteinDistance2("sitting"s, "kitten"s), 3U));
  EXPECT(equal(tlo::levenshteinDistance2("Sunday"s, "Saturday"s), 3U));
  EXPECT(equal(tlo::levenshteinDistance2("CA"s, "ABC"s), 3U));

  const auto &ld1_ = tlo::levenshteinDistance1_<std::string>;

  EXPECT(equal(tlo::levenshteinDistance3("sitting"s, "kitten"s, ld1_), 3U));
  EXPECT(equal(tlo::levenshteinDistance3("Sunday"s, "Saturday"s, ld1_), 3U));
  EXPECT(equal(tlo::levenshteinDistance3("CA"s, "ABC"s, ld1_), 3U));

  EXPECT(equal(tlo::levenshteinDistance3("sitting"s, "kitten"s), 3U));
  EXPECT(equal(tlo::levenshteinDistance3("Sunday"s, "Saturday"s), 3U));
  EXPECT(equal(tlo::levenshteinDistance3("CA"s, "ABC"s), 3U));

  if (passed) {
    std::cout << "All tests passed." << std::endl;
    return 0;
  } else {
    std::cout << "Some tests failed." << std::endl;
    return 1;
  }
}
