#ifndef TLOFS_LCS_HPP
#define TLOFS_LCS_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <ostream>
#include <vector>

#ifdef TLOFS_DEBUG_LLCS
#include <iostream>
#endif

struct LCSLengthResult {
  // length of the LCS
  std::size_t lcsLength;

  // position in first sequence of the last character of the first found LCS
  std::size_t lcsEndPosition1;

  // position in second sequence of the last character of the first found LCS
  std::size_t lcsEndPosition2;
};

std::ostream &operator<<(std::ostream &os, const LCSLengthResult &result);
bool operator==(const LCSLengthResult &result1, const LCSLengthResult &result2);

// Returns table[i][j] if i and j are within bounds, otherwise returns 0
std::size_t lookup(const std::vector<std::vector<std::size_t>> &table,
                   std::size_t i, std::size_t j);

// Returns array[i] if i is within bounds, otherwise returns 0
std::size_t lookup(const std::vector<std::size_t> &array, std::size_t i);

// Returns the length of the LCS of sequence1[startIndex1..startIndex1+size1]
// and sequence2[startIndex2..startIndex2+size2]. Takes O(size1 * size2) time.
// Uses O(size1 * size2) memory.
template <class CharSequence>
LCSLengthResult lcsLength1_(const CharSequence &sequence1,
                            std::size_t startIndex1, std::size_t size1,
                            const CharSequence &sequence2,
                            std::size_t startIndex2, std::size_t size2) {
  assert(startIndex1 + size1 <= sequence1.size());
  assert(startIndex2 + size2 <= sequence2.size());

  if (size1 == 0 || size2 == 0) {
    return {0, 0, 0};
  }

  // table[m][n] will store the length of the LCS of
  // sequence1[startIndex1..startIndex1+m+1] and
  // sequence2[startIndex2..startIndex2+n+1]
  std::vector<std::vector<std::size_t>> table(
      size1, std::vector<std::size_t>(size2, 0));

  LCSLengthResult result = {0, 0, 0};

  for (std::size_t i = 0; i < size1; ++i) {
    for (std::size_t j = 0; j < size2; ++j) {
      if (sequence1[startIndex1 + i] == sequence2[startIndex2 + j]) {
        table[i][j] = lookup(table, i - 1, j - 1) + 1;
      } else {
        table[i][j] =
            std::max(lookup(table, i, j - 1), lookup(table, i - 1, j));
      }

      if (result.lcsLength < table[i][j]) {
        result.lcsLength = table[i][j];
        result.lcsEndPosition1 = startIndex1 + i;
        result.lcsEndPosition2 = startIndex2 + j;
      }
    }
  }

#ifdef TLOFS_DEBUG_LLCS
  std::cerr << __func__ << " table:" << std::endl;
  for (std::size_t i = 0; i < size1; ++i) {
    for (std::size_t j = 0; j < size2; ++j) {
      std::cerr << table[i][j] << " ";
    }
    std::cerr << std::endl;
  }
#endif

  return result;
}

template <class CharSequence>
LCSLengthResult lcsLength1(const CharSequence &sequence1,
                           const CharSequence &sequence2) {
  return lcsLength1_(sequence1, 0, sequence1.size(), sequence2, 0,
                     sequence2.size());
}

// Returns the length of the LCS of sequence1[startIndex1..startIndex1+size1]
// and sequence2[startIndex2..startIndex2+size2]. Takes O(size1 * size2) time.
// Uses only O(min(size1, size2)) memory.
template <class CharSequence>
LCSLengthResult lcsLength2_(const CharSequence &sequence1,
                            std::size_t startIndex1, std::size_t size1,
                            const CharSequence &sequence2,
                            std::size_t startIndex2, std::size_t size2) {
  if (size1 < size2) {
    LCSLengthResult result = lcsLength2_(sequence2, startIndex2, size2,
                                         sequence1, startIndex1, size1);
    std::swap(result.lcsEndPosition1, result.lcsEndPosition2);
    return result;
  }

  assert(startIndex1 + size1 <= sequence1.size());
  assert(startIndex2 + size2 <= sequence2.size());
  assert(size2 <= size1);

  if (size1 == 0 || size2 == 0) {
    return {0, 0, 0};
  }

  // array[n] will store the length of the LCS of
  // sequence1[startIndex1..startIndex1+size1] and
  // sequence2[startIndex2..startIndex2+n+1]
  std::vector<std::size_t> array(size2, 0);

  LCSLengthResult result = {0, 0, 0};

  for (std::size_t i = 0; i < size1; ++i) {
    std::size_t jMinus1thValueBeforeUpdate = 0;

    for (std::size_t j = 0; j < size2; ++j) {
      std::size_t jthValueBeforeUpdate = array[j];

      // jMinus1thValueBeforeUpdate equivalent to table[i - 1][j - 1].
      // array[j - 1] equivalent to table[i][j - 1].
      // array[j] equivalent to table[i - 1][j].
      if (sequence1[startIndex1 + i] == sequence2[startIndex2 + j]) {
        array[j] = jMinus1thValueBeforeUpdate + 1;
      } else {
        array[j] = std::max(lookup(array, j - 1), lookup(array, j));
      }

      jMinus1thValueBeforeUpdate = jthValueBeforeUpdate;

      if (result.lcsLength < array[j]) {
        result.lcsLength = array[j];
        result.lcsEndPosition1 = startIndex1 + i;
        result.lcsEndPosition2 = startIndex2 + j;
      }
    }
  }

#ifdef TLOFS_DEBUG_LLCS
  std::cerr << __func__ << " array:" << std::endl;
  for (std::size_t j = 0; j < size2; ++j) {
    std::cerr << array[j] << " ";
  }
  std::cerr << std::endl;
#endif

  return result;
}

template <class CharSequence>
LCSLengthResult lcsLength2(const CharSequence &sequence1,
                           const CharSequence &sequence2) {
  return lcsLength2_(sequence1, 0, sequence1.size(), sequence2, 0,
                     sequence2.size());
}

// Returns the length of the LCS of sequence1[startIndex1..startIndex1+size1]
// and sequence2[startIndex2..startIndex2+size2]. Does additional optimizations
// on top of one of the other lcsLength functions.
template <class CharSequence, class LengthOfLCSFunction>
LCSLengthResult lcsLength3_(const CharSequence &sequence1,
                            std::size_t startIndex1, std::size_t size1,
                            const CharSequence &sequence2,
                            std::size_t startIndex2, std::size_t size2,
                            LengthOfLCSFunction lcsLength) {
  assert(startIndex1 + size1 <= sequence1.size());
  assert(startIndex2 + size2 <= sequence2.size());

  if (size1 == 0 || size2 == 0) {
    return {0, 0, 0};
  }

  std::size_t prefixLength = 0;
  for (std::size_t i = 0; i < size1 && i < size2; ++i) {
    if (sequence1[startIndex1 + i] != sequence2[startIndex2 + i]) {
      break;
    }

    prefixLength++;
  }

  std::size_t smallerSize = std::min(size1, size2);
  if (prefixLength == smallerSize) {
    return {prefixLength, startIndex1 + prefixLength - 1,
            startIndex2 + prefixLength - 1};
  }

  std::size_t remainingLength = smallerSize - prefixLength;
  std::size_t suffixLength = 0;
  for (std::size_t i = size1 - 1, j = size2 - 1; i < size1 && j < size2;
       --i, --j) {
    if (sequence1[startIndex1 + i] != sequence2[startIndex2 + j]) {
      break;
    }

    if (suffixLength == remainingLength) {
      break;
    }

    suffixLength++;
  }

  LCSLengthResult result = lcsLength(sequence1, startIndex1 + prefixLength,
                                     size1 - prefixLength - suffixLength,
                                     sequence2, startIndex2 + prefixLength,
                                     size2 - prefixLength - suffixLength);
  result.lcsLength += prefixLength + suffixLength;

  if (suffixLength > 0) {
    result.lcsEndPosition1 = startIndex1 + size1 - 1;
    result.lcsEndPosition2 = startIndex2 + size2 - 1;
  }

  return result;
}

template <class CharSequence, class LengthOfLCSFunction>
LCSLengthResult lcsLength3(const CharSequence &sequence1,
                           const CharSequence &sequence2,
                           LengthOfLCSFunction lcsLength) {
  return lcsLength3_(sequence1, 0, sequence1.size(), sequence2, 0,
                     sequence2.size(), lcsLength);
}

#endif  // TLOFS_LCS_HPP
