#include "damerau-levenshtein.hpp"

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

  EXPECT(equal(tlo::damerLevenDistance1(""s, ""s), 0U));
  EXPECT(equal(tlo::damerLevenDistance1("GAC"s, ""s), 3U));
  EXPECT(equal(tlo::damerLevenDistance1(""s, "AGCAT"s), 5U));
  EXPECT(equal(tlo::damerLevenDistance1("GAC"s, "AGCAT"s), 3U));
  EXPECT(equal(tlo::damerLevenDistance1("XMJYAUZ"s, "MZJAWXU"s), 6U));
  EXPECT(
      equal(tlo::damerLevenDistance1("___XMJYAUZ___"s, "__MZJAWXU___"s), 7U));
  EXPECT(
      equal(tlo::damerLevenDistance1("__XMJYAUZ___"s, "___MZJAWXU___-"s), 7U));
  EXPECT(equal(tlo::damerLevenDistance1("0123456789"s, "0123456789"s), 0U));
  EXPECT(equal(tlo::damerLevenDistance1("01234567"s, "abcdefghij"s), 10U));
  EXPECT(equal(tlo::damerLevenDistance1("aaabbb"s, "aaabbb___bbbccc"s), 9U));
  EXPECT(equal(tlo::damerLevenDistance1("bbbccc"s, "aaabbb___bbbccc"s), 9U));
  EXPECT(equal(tlo::damerLevenDistance1("aaabbbccc"s, "aaabbb___bbbccc"s), 6U));
  EXPECT(equal(tlo::damerLevenDistance1("sitting"s, "kitten"s), 3U));
  EXPECT(equal(tlo::damerLevenDistance1("Sunday"s, "Saturday"s), 3U));
  EXPECT(equal(tlo::damerLevenDistance1("CA"s, "ABC"s), 2U));

  EXPECT(equal(tlo::damerLevenDistance2(""s, ""s), 0U));
  EXPECT(equal(tlo::damerLevenDistance2("GAC"s, ""s), 3U));
  EXPECT(equal(tlo::damerLevenDistance2(""s, "AGCAT"s), 5U));
  EXPECT(equal(tlo::damerLevenDistance2("GAC"s, "AGCAT"s), 3U));
  EXPECT(equal(tlo::damerLevenDistance2("XMJYAUZ"s, "MZJAWXU"s), 6U));
  EXPECT(
      equal(tlo::damerLevenDistance2("___XMJYAUZ___"s, "__MZJAWXU___"s), 7U));
  EXPECT(
      equal(tlo::damerLevenDistance2("__XMJYAUZ___"s, "___MZJAWXU___-"s), 7U));
  EXPECT(equal(tlo::damerLevenDistance2("0123456789"s, "0123456789"s), 0U));
  EXPECT(equal(tlo::damerLevenDistance2("01234567"s, "abcdefghij"s), 10U));
  EXPECT(equal(tlo::damerLevenDistance2("aaabbb"s, "aaabbb___bbbccc"s), 9U));
  EXPECT(equal(tlo::damerLevenDistance2("bbbccc"s, "aaabbb___bbbccc"s), 9U));
  EXPECT(equal(tlo::damerLevenDistance2("aaabbbccc"s, "aaabbb___bbbccc"s), 6U));
  EXPECT(equal(tlo::damerLevenDistance2("sitting"s, "kitten"s), 3U));
  EXPECT(equal(tlo::damerLevenDistance2("Sunday"s, "Saturday"s), 3U));
  EXPECT(equal(tlo::damerLevenDistance2("CA"s, "ABC"s), 2U));

  if (passed) {
    std::cout << "All tests passed." << std::endl;
    return 0;
  } else {
    std::cout << "Some tests failed." << std::endl;
    return 1;
  }
}
