#include <iostream>

#include "similarity.hpp"

constexpr std::size_t BLOCK_SIZE = 100;  // 100 is fastest so far
constexpr std::size_t NUM_THREADS = 1;

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <file1> <file2>" << std::endl;
    return 1;
  }

  double similarityPercentage =
      checkFileSimilarity(argv[1], argv[2], BLOCK_SIZE, NUM_THREADS);
  std::cout << "The files are about " << similarityPercentage << "% similar."
            << std::endl;
}
