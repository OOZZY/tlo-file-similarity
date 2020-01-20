#include "lcs.hpp"

#include <iostream>
#include <string>

template <class T>
bool equal(const T &obj1, const T &obj2) {
  return obj1 == obj2;
}

bool passed = true;

#define EXPECT(condition)                                        \
  do {                                                           \
    if (!(condition)) {                                          \
      std::cerr << "Expect failed: " << #condition << std::endl; \
      passed = false;                                            \
    }                                                            \
  } while (0)

int main() {
  using namespace std::string_literals;

  EXPECT(equal(lcsLength1(""s, ""s), {0, 0, 0}));
  EXPECT(equal(lcsLength1("GAC"s, ""s), {0, 0, 0}));
  EXPECT(equal(lcsLength1(""s, "AGCAT"s), {0, 0, 0}));
  EXPECT(equal(lcsLength1("GAC"s, "AGCAT"s), {2, 1, 3}));
  EXPECT(equal(lcsLength1("XMJYAUZ"s, "MZJAWXU"s), {4, 5, 6}));
  EXPECT(equal(lcsLength1("___XMJYAUZ___"s, "__MZJAWXU___"s), {9, 12, 11}));
  EXPECT(equal(lcsLength1("__XMJYAUZ___"s, "___MZJAWXU___-"s), {9, 11, 12}));
  EXPECT(equal(lcsLength1("0123456789"s, "0123456789"s), {10, 9, 9}));
  EXPECT(equal(lcsLength1("01234567"s, "abcdefghij"s), {0, 0, 0}));

  EXPECT(equal(lcsLength2(""s, ""s), {0, 0, 0}));
  EXPECT(equal(lcsLength2("GAC"s, ""s), {0, 0, 0}));
  EXPECT(equal(lcsLength2(""s, "AGCAT"s), {0, 0, 0}));
  EXPECT(equal(lcsLength2("GAC"s, "AGCAT"s), {2, 2, 2}));
  EXPECT(equal(lcsLength2("XMJYAUZ"s, "MZJAWXU"s), {4, 5, 6}));
  EXPECT(equal(lcsLength2("___XMJYAUZ___"s, "__MZJAWXU___"s), {9, 12, 11}));
  EXPECT(equal(lcsLength2("__XMJYAUZ___"s, "___MZJAWXU___-"s), {9, 11, 12}));
  EXPECT(equal(lcsLength2("0123456789"s, "0123456789"s), {10, 9, 9}));
  EXPECT(equal(lcsLength2("01234567"s, "abcdefghij"s), {0, 0, 0}));

  const auto &lcsLength1s = lcsLength1_<std::string>;
  EXPECT(equal(lcsLength3(""s, ""s, lcsLength1s), {0, 0, 0}));
  EXPECT(equal(lcsLength3("GAC"s, ""s, lcsLength1s), {0, 0, 0}));
  EXPECT(equal(lcsLength3(""s, "AGCAT"s, lcsLength1s), {0, 0, 0}));
  EXPECT(equal(lcsLength3("GAC"s, "AGCAT"s, lcsLength1s), {2, 1, 3}));
  EXPECT(equal(lcsLength3("XMJYAUZ"s, "MZJAWXU"s, lcsLength1s), {4, 5, 6}));
  EXPECT(equal(lcsLength3("___XMJYAUZ___"s, "__MZJAWXU___"s, lcsLength1s),
               {9, 12, 11}));
  EXPECT(equal(lcsLength3("__XMJYAUZ___"s, "___MZJAWXU___-"s, lcsLength1s),
               {9, 11, 12}));
  EXPECT(
      equal(lcsLength3("0123456789"s, "0123456789"s, lcsLength1s), {10, 9, 9}));
  EXPECT(equal(lcsLength3("01234567"s, "abcdefghij"s, lcsLength1s), {0, 0, 0}));
  EXPECT(
      equal(lcsLength3("aaabbb"s, "aaabbb___bbbccc"s, lcsLength1s), {6, 5, 5}));
  EXPECT(equal(lcsLength3("bbbccc"s, "aaabbb___bbbccc"s, lcsLength1s),
               {6, 5, 14}));
  EXPECT(equal(lcsLength3("aaabbbccc"s, "aaabbb___bbbccc"s, lcsLength1s),
               {9, 8, 14}));

  const auto &lcsLength2s = lcsLength2_<std::string>;
  EXPECT(equal(lcsLength3(""s, ""s, lcsLength2s), {0, 0, 0}));
  EXPECT(equal(lcsLength3("GAC"s, ""s, lcsLength2s), {0, 0, 0}));
  EXPECT(equal(lcsLength3(""s, "AGCAT"s, lcsLength2s), {0, 0, 0}));
  EXPECT(equal(lcsLength3("GAC"s, "AGCAT"s, lcsLength2s), {2, 2, 2}));
  EXPECT(equal(lcsLength3("XMJYAUZ"s, "MZJAWXU"s, lcsLength2s), {4, 5, 6}));
  EXPECT(equal(lcsLength3("___XMJYAUZ___"s, "__MZJAWXU___"s, lcsLength2s),
               {9, 12, 11}));
  EXPECT(equal(lcsLength3("__XMJYAUZ___"s, "___MZJAWXU___-"s, lcsLength2s),
               {9, 11, 12}));
  EXPECT(
      equal(lcsLength3("0123456789"s, "0123456789"s, lcsLength2s), {10, 9, 9}));
  EXPECT(equal(lcsLength3("01234567"s, "abcdefghij"s, lcsLength2s), {0, 0, 0}));
  EXPECT(
      equal(lcsLength3("aaabbb"s, "aaabbb___bbbccc"s, lcsLength2s), {6, 5, 5}));
  EXPECT(equal(lcsLength3("bbbccc"s, "aaabbb___bbbccc"s, lcsLength2s),
               {6, 5, 14}));
  EXPECT(equal(lcsLength3("aaabbbccc"s, "aaabbb___bbbccc"s, lcsLength2s),
               {9, 8, 14}));

  if (passed) {
    std::cout << "All tests passed." << std::endl;
    return 0;
  } else {
    std::cout << "Some tests failed." << std::endl;
    return 1;
  }
}
