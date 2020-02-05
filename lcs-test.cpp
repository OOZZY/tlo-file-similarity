#include "lcs.hpp"

#include <iostream>
#include <string>

template <class T>
bool equal(const T &obj1, const T &obj2) {
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

  EXPECT(equal(tlo::lcsLength1(""s, ""s), {0, 0}));
  EXPECT(equal(tlo::lcsLength1("GAC"s, ""s), {0, 3}));
  EXPECT(equal(tlo::lcsLength1(""s, "AGCAT"s), {0, 5}));
  EXPECT(equal(tlo::lcsLength1("GAC"s, "AGCAT"s), {2, 4}));
  EXPECT(equal(tlo::lcsLength1("XMJYAUZ"s, "MZJAWXU"s), {4, 6}));
  EXPECT(equal(tlo::lcsLength1("___XMJYAUZ___"s, "__MZJAWXU___"s), {9, 7}));
  EXPECT(equal(tlo::lcsLength1("__XMJYAUZ___"s, "___MZJAWXU___-"s), {9, 8}));
  EXPECT(equal(tlo::lcsLength1("0123456789"s, "0123456789"s), {10, 0}));
  EXPECT(equal(tlo::lcsLength1("01234567"s, "abcdefghij"s), {0, 18}));

  EXPECT(equal(tlo::lcsLength2(""s, ""s), {0, 0}));
  EXPECT(equal(tlo::lcsLength2("GAC"s, ""s), {0, 3}));
  EXPECT(equal(tlo::lcsLength2(""s, "AGCAT"s), {0, 5}));
  EXPECT(equal(tlo::lcsLength2("GAC"s, "AGCAT"s), {2, 4}));
  EXPECT(equal(tlo::lcsLength2("XMJYAUZ"s, "MZJAWXU"s), {4, 6}));
  EXPECT(equal(tlo::lcsLength2("___XMJYAUZ___"s, "__MZJAWXU___"s), {9, 7}));
  EXPECT(equal(tlo::lcsLength2("__XMJYAUZ___"s, "___MZJAWXU___-"s), {9, 8}));
  EXPECT(equal(tlo::lcsLength2("0123456789"s, "0123456789"s), {10, 0}));
  EXPECT(equal(tlo::lcsLength2("01234567"s, "abcdefghij"s), {0, 18}));

  const auto &lcsl1_ = tlo::lcsLength1_<std::string>;

  EXPECT(equal(tlo::lcsLength3(""s, ""s, lcsl1_), {0, 0}));
  EXPECT(equal(tlo::lcsLength3("GAC"s, ""s, lcsl1_), {0, 3}));
  EXPECT(equal(tlo::lcsLength3(""s, "AGCAT"s, lcsl1_), {0, 5}));
  EXPECT(equal(tlo::lcsLength3("GAC"s, "AGCAT"s, lcsl1_), {2, 4}));
  EXPECT(equal(tlo::lcsLength3("XMJYAUZ"s, "MZJAWXU"s, lcsl1_), {4, 6}));
  EXPECT(equal(tlo::lcsLength3("___XMJYAUZ___"s, "__MZJAWXU___"s, lcsl1_),
               {9, 7}));
  EXPECT(equal(tlo::lcsLength3("__XMJYAUZ___"s, "___MZJAWXU___-"s, lcsl1_),
               {9, 8}));
  EXPECT(equal(tlo::lcsLength3("0123456789"s, "0123456789"s, lcsl1_), {10, 0}));
  EXPECT(equal(tlo::lcsLength3("01234567"s, "abcdefghij"s, lcsl1_), {0, 18}));
  EXPECT(equal(tlo::lcsLength3("aaabbb"s, "aaabbb___bbbccc"s, lcsl1_), {6, 9}));
  EXPECT(equal(tlo::lcsLength3("bbbccc"s, "aaabbb___bbbccc"s, lcsl1_), {6, 9}));
  EXPECT(
      equal(tlo::lcsLength3("aaabbbccc"s, "aaabbb___bbbccc"s, lcsl1_), {9, 6}));

  EXPECT(equal(tlo::lcsLength3(""s, ""s), {0, 0}));
  EXPECT(equal(tlo::lcsLength3("GAC"s, ""s), {0, 3}));
  EXPECT(equal(tlo::lcsLength3(""s, "AGCAT"s), {0, 5}));
  EXPECT(equal(tlo::lcsLength3("GAC"s, "AGCAT"s), {2, 4}));
  EXPECT(equal(tlo::lcsLength3("XMJYAUZ"s, "MZJAWXU"s), {4, 6}));
  EXPECT(equal(tlo::lcsLength3("___XMJYAUZ___"s, "__MZJAWXU___"s), {9, 7}));
  EXPECT(equal(tlo::lcsLength3("__XMJYAUZ___"s, "___MZJAWXU___-"s), {9, 8}));
  EXPECT(equal(tlo::lcsLength3("0123456789"s, "0123456789"s), {10, 0}));
  EXPECT(equal(tlo::lcsLength3("01234567"s, "abcdefghij"s), {0, 18}));
  EXPECT(equal(tlo::lcsLength3("aaabbb"s, "aaabbb___bbbccc"s), {6, 9}));
  EXPECT(equal(tlo::lcsLength3("bbbccc"s, "aaabbb___bbbccc"s), {6, 9}));
  EXPECT(equal(tlo::lcsLength3("aaabbbccc"s, "aaabbb___bbbccc"s), {9, 6}));

  if (passed) {
    std::cout << "All tests passed." << std::endl;
    return 0;
  } else {
    std::cout << "Some tests failed." << std::endl;
    return 1;
  }
}
