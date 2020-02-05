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

  EXPECT(equal(tlo::levenshteinDistance1(""s, ""s), 0U));
  EXPECT(equal(tlo::levenshteinDistance1("GAC"s, ""s), 3U));
  EXPECT(equal(tlo::levenshteinDistance1(""s, "AGCAT"s), 5U));
  EXPECT(equal(tlo::levenshteinDistance1("GAC"s, "AGCAT"s), 3U));
  EXPECT(equal(tlo::levenshteinDistance1("XMJYAUZ"s, "MZJAWXU"s), 6U));
  EXPECT(
      equal(tlo::levenshteinDistance1("___XMJYAUZ___"s, "__MZJAWXU___"s), 7U));
  EXPECT(
      equal(tlo::levenshteinDistance1("__XMJYAUZ___"s, "___MZJAWXU___-"s), 7U));
  EXPECT(equal(tlo::levenshteinDistance1("0123456789"s, "0123456789"s), 0U));
  EXPECT(equal(tlo::levenshteinDistance1("01234567"s, "abcdefghij"s), 10U));
  EXPECT(equal(tlo::levenshteinDistance1("aaabbb"s, "aaabbb___bbbccc"s), 9U));
  EXPECT(equal(tlo::levenshteinDistance1("bbbccc"s, "aaabbb___bbbccc"s), 9U));
  EXPECT(
      equal(tlo::levenshteinDistance1("aaabbbccc"s, "aaabbb___bbbccc"s), 6U));
  EXPECT(equal(tlo::levenshteinDistance1("sitting"s, "kitten"s), 3U));
  EXPECT(equal(tlo::levenshteinDistance1("Sunday"s, "Saturday"s), 3U));
  EXPECT(equal(tlo::levenshteinDistance1("CA"s, "ABC"s), 3U));

  EXPECT(equal(tlo::levenshteinDistance2(""s, ""s), 0U));
  EXPECT(equal(tlo::levenshteinDistance2("GAC"s, ""s), 3U));
  EXPECT(equal(tlo::levenshteinDistance2(""s, "AGCAT"s), 5U));
  EXPECT(equal(tlo::levenshteinDistance2("GAC"s, "AGCAT"s), 3U));
  EXPECT(equal(tlo::levenshteinDistance2("XMJYAUZ"s, "MZJAWXU"s), 6U));
  EXPECT(
      equal(tlo::levenshteinDistance2("___XMJYAUZ___"s, "__MZJAWXU___"s), 7U));
  EXPECT(
      equal(tlo::levenshteinDistance2("__XMJYAUZ___"s, "___MZJAWXU___-"s), 7U));
  EXPECT(equal(tlo::levenshteinDistance2("0123456789"s, "0123456789"s), 0U));
  EXPECT(equal(tlo::levenshteinDistance2("01234567"s, "abcdefghij"s), 10U));
  EXPECT(equal(tlo::levenshteinDistance2("aaabbb"s, "aaabbb___bbbccc"s), 9U));
  EXPECT(equal(tlo::levenshteinDistance2("bbbccc"s, "aaabbb___bbbccc"s), 9U));
  EXPECT(
      equal(tlo::levenshteinDistance2("aaabbbccc"s, "aaabbb___bbbccc"s), 6U));
  EXPECT(equal(tlo::levenshteinDistance2("sitting"s, "kitten"s), 3U));
  EXPECT(equal(tlo::levenshteinDistance2("Sunday"s, "Saturday"s), 3U));
  EXPECT(equal(tlo::levenshteinDistance2("CA"s, "ABC"s), 3U));

  const auto &ld1_ = tlo::levenshteinDistance1_<std::string>;

  EXPECT(equal(tlo::levenshteinDistance3(""s, ""s, ld1_), 0U));
  EXPECT(equal(tlo::levenshteinDistance3("GAC"s, ""s, ld1_), 3U));
  EXPECT(equal(tlo::levenshteinDistance3(""s, "AGCAT"s, ld1_), 5U));
  EXPECT(equal(tlo::levenshteinDistance3("GAC"s, "AGCAT"s, ld1_), 3U));
  EXPECT(equal(tlo::levenshteinDistance3("XMJYAUZ"s, "MZJAWXU"s, ld1_), 6U));
  EXPECT(equal(
      tlo::levenshteinDistance3("___XMJYAUZ___"s, "__MZJAWXU___"s, ld1_), 7U));
  EXPECT(equal(
      tlo::levenshteinDistance3("__XMJYAUZ___"s, "___MZJAWXU___-"s, ld1_), 7U));
  EXPECT(
      equal(tlo::levenshteinDistance3("0123456789"s, "0123456789"s, ld1_), 0U));
  EXPECT(
      equal(tlo::levenshteinDistance3("01234567"s, "abcdefghij"s, ld1_), 10U));
  EXPECT(equal(tlo::levenshteinDistance3("aaabbb"s, "aaabbb___bbbccc"s, ld1_),
               9U));
  EXPECT(equal(tlo::levenshteinDistance3("bbbccc"s, "aaabbb___bbbccc"s, ld1_),
               9U));
  EXPECT(equal(
      tlo::levenshteinDistance3("aaabbbccc"s, "aaabbb___bbbccc"s, ld1_), 6U));
  EXPECT(equal(tlo::levenshteinDistance3("sitting"s, "kitten"s, ld1_), 3U));
  EXPECT(equal(tlo::levenshteinDistance3("Sunday"s, "Saturday"s, ld1_), 3U));
  EXPECT(equal(tlo::levenshteinDistance3("CA"s, "ABC"s, ld1_), 3U));

  EXPECT(equal(tlo::levenshteinDistance3(""s, ""s), 0U));
  EXPECT(equal(tlo::levenshteinDistance3("GAC"s, ""s), 3U));
  EXPECT(equal(tlo::levenshteinDistance3(""s, "AGCAT"s), 5U));
  EXPECT(equal(tlo::levenshteinDistance3("GAC"s, "AGCAT"s), 3U));
  EXPECT(equal(tlo::levenshteinDistance3("XMJYAUZ"s, "MZJAWXU"s), 6U));
  EXPECT(
      equal(tlo::levenshteinDistance3("___XMJYAUZ___"s, "__MZJAWXU___"s), 7U));
  EXPECT(
      equal(tlo::levenshteinDistance3("__XMJYAUZ___"s, "___MZJAWXU___-"s), 7U));
  EXPECT(equal(tlo::levenshteinDistance3("0123456789"s, "0123456789"s), 0U));
  EXPECT(equal(tlo::levenshteinDistance3("01234567"s, "abcdefghij"s), 10U));
  EXPECT(equal(tlo::levenshteinDistance3("aaabbb"s, "aaabbb___bbbccc"s), 9U));
  EXPECT(equal(tlo::levenshteinDistance3("bbbccc"s, "aaabbb___bbbccc"s), 9U));
  EXPECT(
      equal(tlo::levenshteinDistance3("aaabbbccc"s, "aaabbb___bbbccc"s), 6U));
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
