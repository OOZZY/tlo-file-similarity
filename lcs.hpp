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

namespace tlo {
struct LCSLengthResult {
  // Length of the LCS.
  std::size_t lcsLength;

  // LCS distance (edit distance when only insertion and deletion is allowed).
  std::size_t lcsDistance;
};

std::ostream &operator<<(std::ostream &os, const LCSLengthResult &result);
bool operator==(const LCSLengthResult &result1, const LCSLengthResult &result2);

namespace internal {
// Calculate LCS distance for a pair of strings with given sizes and LCS length.
std::size_t lcsDistance(std::size_t size1, std::size_t size2,
                        std::size_t lcsLength);
}  // namespace internal

// Returns the length of the LCS of sequence1[startIndex1, startIndex1+size1)
// and sequence2[startIndex2, startIndex2+size2). Takes O(size1 * size2) time.
// Uses O(size1 * size2) memory.
template <class CharSequence>
LCSLengthResult lcsLength1_(const CharSequence &sequence1,
                            std::size_t startIndex1, std::size_t size1,
                            const CharSequence &sequence2,
                            std::size_t startIndex2, std::size_t size2) {
  assert(startIndex1 + size1 <= sequence1.size());
  assert(startIndex2 + size2 <= sequence2.size());

  if (size1 == 0 || size2 == 0) {
    return {0, internal::lcsDistance(size1, size2, 0)};
  }

  // lcsLengths[m][n] will store the length of the LCS of
  // sequence1[startIndex1, startIndex1+m) and
  // sequence2[startIndex2, startIndex2+n).
  std::vector<std::vector<std::size_t>> lcsLengths(
      size1 + 1, std::vector<std::size_t>(size2 + 1, 0));

  for (std::size_t i = 0; i < size1; ++i) {
    std::size_t row = i + 1;

    for (std::size_t j = 0; j < size2; ++j) {
      std::size_t col = j + 1;

      if (sequence1[startIndex1 + i] == sequence2[startIndex2 + j]) {
        lcsLengths[row][col] = lcsLengths[row - 1][col - 1] + 1;
      } else {
        lcsLengths[row][col] =
            std::max(lcsLengths[row][col - 1], lcsLengths[row - 1][col]);
      }
    }
  }

#ifdef TLOFS_DEBUG_LLCS
  std::cerr << __func__ << " lcsLengths:" << std::endl;
  for (std::size_t i = 0; i <= size1; ++i) {
    for (std::size_t j = 0; j <= size2; ++j) {
      std::cerr << lcsLengths[i][j] << " ";
    }
    std::cerr << std::endl;
  }
#endif

  LCSLengthResult result;

  result.lcsLength = lcsLengths[size1][size2];
  result.lcsDistance = internal::lcsDistance(size1, size2, result.lcsLength);
  return result;
}

template <class CharSequence>
LCSLengthResult lcsLength1(const CharSequence &sequence1,
                           const CharSequence &sequence2) {
  return lcsLength1_(sequence1, 0, sequence1.size(), sequence2, 0,
                     sequence2.size());
}

// Returns the length of the LCS of sequence1[startIndex1, startIndex1+size1)
// and sequence2[startIndex2, startIndex2+size2). Takes O(size1 * size2) time.
// Uses only O(min(size1, size2)) memory.
template <class CharSequence>
LCSLengthResult lcsLength2_(const CharSequence &sequence1,
                            std::size_t startIndex1, std::size_t size1,
                            const CharSequence &sequence2,
                            std::size_t startIndex2, std::size_t size2) {
  if (size1 < size2) {
    return lcsLength2_(sequence2, startIndex2, size2, sequence1, startIndex1,
                       size1);
  }

  assert(startIndex1 + size1 <= sequence1.size());
  assert(startIndex2 + size2 <= sequence2.size());
  assert(size2 <= size1);

  if (size1 == 0 || size2 == 0) {
    return {0, internal::lcsDistance(size1, size2, 0)};
  }

  // lcsLengths[n] will store the length of the LCS of
  // sequence1[startIndex1, startIndex1+size1) and
  // sequence2[startIndex2, startIndex2+n).
  std::vector<std::size_t> lcsLengths(size2 + 1, 0);

  for (std::size_t i = 0; i < size1; ++i) {
    std::size_t valueInPreviousColumnBeforeUpdate = 0;

    for (std::size_t j = 0; j < size2; ++j) {
      std::size_t col = j + 1;
      std::size_t valueInColumnBeforeUpdate = lcsLengths[col];

      // lcsLengths[row - 1][col - 1] -> valueInPreviousColumnBeforeUpdate.
      // lcsLengths[row][col - 1] -> lcsLengths[col - 1].
      // lcsLengths[row - 1][col] -> lcsLengths[col].
      if (sequence1[startIndex1 + i] == sequence2[startIndex2 + j]) {
        lcsLengths[col] = valueInPreviousColumnBeforeUpdate + 1;
      } else {
        lcsLengths[col] = std::max(lcsLengths[col - 1], lcsLengths[col]);
      }

      valueInPreviousColumnBeforeUpdate = valueInColumnBeforeUpdate;
    }
  }

#ifdef TLOFS_DEBUG_LLCS
  std::cerr << __func__ << " lcsLengths:" << std::endl;
  for (std::size_t j = 0; j <= size2; ++j) {
    std::cerr << lcsLengths[j] << " ";
  }
  std::cerr << std::endl;
#endif

  LCSLengthResult result;

  result.lcsLength = lcsLengths[size2];
  result.lcsDistance = internal::lcsDistance(size1, size2, result.lcsLength);
  return result;
}

template <class CharSequence>
LCSLengthResult lcsLength2(const CharSequence &sequence1,
                           const CharSequence &sequence2) {
  return lcsLength2_(sequence1, 0, sequence1.size(), sequence2, 0,
                     sequence2.size());
}

// Returns the length of the LCS of sequence1[startIndex1, startIndex1+size1)
// and sequence2[startIndex2, startIndex2+size2). Does additional optimizations
// on top of one of the other lcsLength functions.
template <class CharSequence>
LCSLengthResult lcsLength3_(
    const CharSequence &sequence1, std::size_t startIndex1, std::size_t size1,
    const CharSequence &sequence2, std::size_t startIndex2, std::size_t size2,
    LCSLengthResult (*lcsLength)(const CharSequence &, std::size_t, std::size_t,
                                 const CharSequence &, std::size_t,
                                 std::size_t) = lcsLength2_<CharSequence>) {
  assert(startIndex1 + size1 <= sequence1.size());
  assert(startIndex2 + size2 <= sequence2.size());

  if (size1 == 0 || size2 == 0) {
    return {0, internal::lcsDistance(size1, size2, 0)};
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
    return {prefixLength, internal::lcsDistance(size1, size2, prefixLength)};
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
  result.lcsDistance = internal::lcsDistance(size1, size2, result.lcsLength);

  return result;
}

template <class CharSequence>
LCSLengthResult lcsLength3(
    const CharSequence &sequence1, const CharSequence &sequence2,
    LCSLengthResult (*lcsLength)(const CharSequence &, std::size_t, std::size_t,
                                 const CharSequence &, std::size_t,
                                 std::size_t) = lcsLength2_<CharSequence>) {
  return lcsLength3_(sequence1, 0, sequence1.size(), sequence2, 0,
                     sequence2.size(), lcsLength);
}

// Calculate max LCS distance for a pair of strings with given sizes.
std::size_t maxLCSDistance(std::size_t size1, std::size_t size2);
}  // namespace tlo

#endif  // TLOFS_LCS_HPP
