#ifndef TLOFS_DAMERAU_LEVENSHTEIN_HPP
#define TLOFS_DAMERAU_LEVENSHTEIN_HPP

#include <algorithm>
#include <cassert>
#include <climits>

#ifdef TLOFS_DEBUG_DAMERAU_LEVENSHTEIN
#include <iostream>
#endif

namespace tlo {
// Calculate max Damerau-Levenshtein distance for a pair of strings with given
// sizes.
std::size_t maxDamerLevenDistance(std::size_t size1, std::size_t size2);

namespace internal {
constexpr std::size_t NUM_CHARS = 1 << CHAR_BIT;
}  // namespace internal

// Returns the Damerau-Levenshtein distance between
// sequence1[startIndex1, startIndex1+size1) and
// sequence2[startIndex2, startIndex2+size2). Takes O(size1 * size2) time.
// Uses O(size1 * size2) memory.
template <class CharSequence>
std::size_t damerLevenDistance1_(const CharSequence &sequence1,
                                 std::size_t startIndex1, std::size_t size1,
                                 const CharSequence &sequence2,
                                 std::size_t startIndex2, std::size_t size2) {
  assert(startIndex1 + size1 <= sequence1.size());
  assert(startIndex2 + size2 <= sequence2.size());

  if (size1 == 0 || size2 == 0) {
    return maxDamerLevenDistance(size1, size2);
  }

  // distances[m + 1][n + 1] will store the Damerau-Levenshtein distance between
  // sequence1[startIndex1, startIndex1+m) and
  // sequence2[startIndex2, startIndex2+n).
  std::vector<std::vector<std::size_t>> distances(
      size1 + 2, std::vector<std::size_t>(size2 + 2, 0));
  const std::size_t maxDistance = maxDamerLevenDistance(size1, size2);
  const std::size_t lastRow = size1 + 1;
  const std::size_t lastCol = size2 + 1;

  distances[0][0] = maxDistance;

  for (std::size_t row = 1; row <= lastRow; ++row) {
    distances[row][0] = maxDistance;
    distances[row][1] = row - 1;
  }

  for (std::size_t col = 1; col <= lastCol; ++col) {
    distances[0][col] = maxDistance;
    distances[1][col] = col - 1;
  }

  std::vector<std::size_t> rowsOfSeq1(internal::NUM_CHARS, 1);

  for (std::size_t i = 0; i < size1; ++i) {
    std::size_t row = i + 2;
    auto charInSeq1 = static_cast<unsigned char>(sequence1[startIndex1 + i]);
    std::size_t colOfSeq2 = 1;

    for (std::size_t j = 0; j < size2; ++j) {
      std::size_t col = j + 2;
      auto charInSeq2 = static_cast<unsigned char>(sequence2[startIndex2 + j]);
      std::size_t rowForCharInSeq2 = rowsOfSeq1[charInSeq2];
      std::size_t colForCharInSeq1 = colOfSeq2;

      std::size_t deletionCost = distances[row - 1][col] + 1;
      std::size_t insertionCost = distances[row][col - 1] + 1;
      std::size_t substitutionCost = distances[row - 1][col - 1];
      std::size_t transpositionCost;

      if (i > 0 && j > 0) {
        transpositionCost =
            distances[rowForCharInSeq2 - 1][colForCharInSeq1 - 1] +
            (row - rowForCharInSeq2 - 1) + 1 + (col - colForCharInSeq1 - 1);
      } else {
        transpositionCost = maxDistance;
      }

      if (charInSeq1 != charInSeq2) {
        substitutionCost++;
      } else {
        colOfSeq2 = col;
      }

      distances[row][col] = std::min(
          {deletionCost, insertionCost, substitutionCost, transpositionCost});
    }

    rowsOfSeq1[charInSeq1] = row;
  }

#ifdef TLOFS_DEBUG_DAMERAU_LEVENSHTEIN
  std::cerr << __func__ << " distances:" << std::endl;
  for (std::size_t i = 0; i <= lastRow; ++i) {
    for (std::size_t j = 0; j <= lastCol; ++j) {
      std::cerr << distances[i][j] << " ";
    }
    std::cerr << std::endl;
  }
#endif

  return distances[lastRow][lastCol];
}

template <class CharSequence>
std::size_t damerLevenDistance1(const CharSequence &sequence1,
                                const CharSequence &sequence2) {
  return damerLevenDistance1_(sequence1, 0, sequence1.size(), sequence2, 0,
                              sequence2.size());
}

// Returns the Damerau-Levenshtein distance between
// sequence1[startIndex1, startIndex1+size1) and
// sequence2[startIndex2, startIndex2+size2). Does additional optimizations on
// top of damerLevenDistance1_.
template <class CharSequence>
std::size_t damerLevenDistance2_(
    const CharSequence &sequence1, std::size_t startIndex1, std::size_t size1,
    const CharSequence &sequence2, std::size_t startIndex2, std::size_t size2,
    std::size_t (*damerLevenDistance)(
        const CharSequence &, std::size_t, std::size_t, const CharSequence &,
        std::size_t, std::size_t) = damerLevenDistance1_<CharSequence>) {
  assert(startIndex1 + size1 <= sequence1.size());
  assert(startIndex2 + size2 <= sequence2.size());

  if (size1 == 0 || size2 == 0) {
    return maxDamerLevenDistance(size1, size2);
  }

  std::size_t prefixLength = 0;

  for (std::size_t i = 0; i < size1 && i < size2; ++i) {
    if (sequence1[startIndex1 + i] != sequence2[startIndex2 + i]) {
      break;
    }

    prefixLength++;
  }

  std::size_t smallerSize = std::min(size1, size2);
  std::size_t largerSize = std::max(size1, size2);

  if (prefixLength == smallerSize) {
    return largerSize - smallerSize;
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

  return damerLevenDistance(sequence1, startIndex1 + prefixLength,
                            size1 - prefixLength - suffixLength, sequence2,
                            startIndex2 + prefixLength,
                            size2 - prefixLength - suffixLength);
}

template <class CharSequence>
std::size_t damerLevenDistance2(
    const CharSequence &sequence1, const CharSequence &sequence2,
    std::size_t (*damerLevenDistance)(
        const CharSequence &, std::size_t, std::size_t, const CharSequence &,
        std::size_t, std::size_t) = damerLevenDistance1_<CharSequence>) {
  return damerLevenDistance2_(sequence1, 0, sequence1.size(), sequence2, 0,
                              sequence2.size(), damerLevenDistance);
}
}  // namespace tlo

#endif  // TLOFS_DAMERAU_LEVENSHTEIN_HPP
