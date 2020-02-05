#ifndef TLOFS_LEVENSHTEIN_HPP
#define TLOFS_LEVENSHTEIN_HPP

#include <algorithm>
#include <cassert>

#ifdef TLOFS_DEBUG_LEVENSHTEIN
#include <iostream>
#endif

namespace tlo {
// Calculate max Levenshtein distance for a pair of strings with given sizes.
std::size_t maxLevenshteinDistance(std::size_t size1, std::size_t size2);

// Returns the Levenshtein distance between
// sequence1[startIndex1, startIndex1+size1) and
// sequence2[startIndex2, startIndex2+size2). Takes O(size1 * size2) time.
// Uses O(size1 * size2) memory.
template <class CharSequence>
std::size_t levenshteinDistance1_(const CharSequence &sequence1,
                                  std::size_t startIndex1, std::size_t size1,
                                  const CharSequence &sequence2,
                                  std::size_t startIndex2, std::size_t size2) {
  assert(startIndex1 + size1 <= sequence1.size());
  assert(startIndex2 + size2 <= sequence2.size());

  if (size1 == 0 || size2 == 0) {
    return maxLevenshteinDistance(size1, size2);
  }

  // distances[m][n] will store the Levenshtein distance between
  // sequence1[startIndex1, startIndex1+m) and
  // sequence2[startIndex2, startIndex2+n).
  std::vector<std::vector<std::size_t>> distances(
      size1 + 1, std::vector<std::size_t>(size2 + 1, 0));

  for (std::size_t row = 1; row <= size1; ++row) {
    distances[row][0] = row;
  }

  for (std::size_t col = 1; col <= size2; ++col) {
    distances[0][col] = col;
  }

  for (std::size_t i = 0; i < size1; ++i) {
    for (std::size_t j = 0; j < size2; ++j) {
      std::size_t row = i + 1;
      std::size_t col = j + 1;

      std::size_t deletionCost = distances[row - 1][col] + 1;
      std::size_t insertionCost = distances[row][col - 1] + 1;
      std::size_t substitutionCost = distances[row - 1][col - 1];

      if (sequence1[startIndex1 + i] != sequence2[startIndex2 + j]) {
        substitutionCost++;
      }

      distances[row][col] =
          std::min({deletionCost, insertionCost, substitutionCost});
    }
  }

#ifdef TLOFS_DEBUG_LEVENSHTEIN
  std::cerr << __func__ << " distances:" << std::endl;
  for (std::size_t i = 0; i <= size1; ++i) {
    for (std::size_t j = 0; j <= size2; ++j) {
      std::cerr << distances[i][j] << " ";
    }
    std::cerr << std::endl;
  }
#endif

  return distances[size1][size2];
}

template <class CharSequence>
std::size_t levenshteinDistance1(const CharSequence &sequence1,
                                 const CharSequence &sequence2) {
  return levenshteinDistance1_(sequence1, 0, sequence1.size(), sequence2, 0,
                               sequence2.size());
}

// Returns the Levenshtein distance between
// sequence1[startIndex1, startIndex1+size1) and
// sequence2[startIndex2, startIndex2+size2). Takes O(size1 * size2) time.
// Uses only O(min(size1, size2)) memory.
template <class CharSequence>
std::size_t levenshteinDistance2_(const CharSequence &sequence1,
                                  std::size_t startIndex1, std::size_t size1,
                                  const CharSequence &sequence2,
                                  std::size_t startIndex2, std::size_t size2) {
  if (size1 < size2) {
    return levenshteinDistance2_(sequence2, startIndex2, size2, sequence1,
                                 startIndex1, size1);
  }

  assert(startIndex1 + size1 <= sequence1.size());
  assert(startIndex2 + size2 <= sequence2.size());
  assert(size2 <= size1);

  if (size1 == 0 || size2 == 0) {
    return maxLevenshteinDistance(size1, size2);
  }

  // distances[n] will store the Levenshtein distance between
  // sequence1[startIndex1, startIndex1+size1) and
  // sequence2[startIndex2, startIndex2+n).
  std::vector<std::size_t> distances(size2 + 1, 0);

  for (std::size_t col = 1; col <= size2; ++col) {
    distances[col] = col;
  }

  for (std::size_t i = 0; i < size1; ++i) {
    std::size_t row = i + 1;
    std::size_t valueInPreviousColumnBeforeUpdate;

    distances[0] = row;
    valueInPreviousColumnBeforeUpdate = row - 1;

    for (std::size_t j = 0; j < size2; ++j) {
      std::size_t col = j + 1;
      std::size_t valueInColumnBeforeUpdate = distances[col];

      // distances[row - 1][col - 1] -> valueInPreviousColumnBeforeUpdate.
      // distances[row][col - 1] -> distances[col - 1].
      // distances[row - 1][col] -> distances[col].
      std::size_t deletionCost = distances[col] + 1;
      std::size_t insertionCost = distances[col - 1] + 1;
      std::size_t substitutionCost = valueInPreviousColumnBeforeUpdate;

      if (sequence1[startIndex1 + i] != sequence2[startIndex2 + j]) {
        substitutionCost++;
      }

      distances[col] =
          std::min({deletionCost, insertionCost, substitutionCost});
      valueInPreviousColumnBeforeUpdate = valueInColumnBeforeUpdate;
    }
  }

#ifdef TLOFS_DEBUG_LEVENSHTEIN
  std::cerr << __func__ << " distances:" << std::endl;
  for (std::size_t j = 0; j <= size2; ++j) {
    std::cerr << distances[j] << " ";
  }
  std::cerr << std::endl;
#endif

  return distances[size2];
}

template <class CharSequence>
std::size_t levenshteinDistance2(const CharSequence &sequence1,
                                 const CharSequence &sequence2) {
  return levenshteinDistance2_(sequence1, 0, sequence1.size(), sequence2, 0,
                               sequence2.size());
}

// Returns the Levenshtein distance between
// sequence1[startIndex1, startIndex1+size1) and
// sequence2[startIndex2, startIndex2+size2). Does additional optimizations on
// top of one of the other levenshteinDistance functions.
template <class CharSequence>
std::size_t levenshteinDistance3_(
    const CharSequence &sequence1, std::size_t startIndex1, std::size_t size1,
    const CharSequence &sequence2, std::size_t startIndex2, std::size_t size2,
    std::size_t (*levenshteinDistance)(
        const CharSequence &, std::size_t, std::size_t, const CharSequence &,
        std::size_t, std::size_t) = levenshteinDistance2_<CharSequence>) {
  assert(startIndex1 + size1 <= sequence1.size());
  assert(startIndex2 + size2 <= sequence2.size());

  if (size1 == 0 || size2 == 0) {
    return maxLevenshteinDistance(size1, size2);
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

  return levenshteinDistance(sequence1, startIndex1 + prefixLength,
                             size1 - prefixLength - suffixLength, sequence2,
                             startIndex2 + prefixLength,
                             size2 - prefixLength - suffixLength);
}

template <class CharSequence>
std::size_t levenshteinDistance3(
    const CharSequence &sequence1, const CharSequence &sequence2,
    std::size_t (*levenshteinDistance)(
        const CharSequence &, std::size_t, std::size_t, const CharSequence &,
        std::size_t, std::size_t) = levenshteinDistance2_<CharSequence>) {
  return levenshteinDistance3_(sequence1, 0, sequence1.size(), sequence2, 0,
                               sequence2.size(), levenshteinDistance);
}
}  // namespace tlo

#endif  // TLOFS_LEVENSHTEIN_HPP
