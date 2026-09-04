#include "ribo/Runtime.h"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

static int parseInt(const char *text) {
  int value = 0;
  const auto *last = text + std::char_traits<char>::length(text);
  const auto [end, error] = std::from_chars(text, last, value);
  if (error != std::errc{} || end != last) throw std::invalid_argument(text);
  return value;
}

int main(int argc, char **argv) {
  try {
  if (argc > 1 && std::string_view(argv[1]) == "--h-type") {
    const int length = argc > 2 ? std::max(1, parseInt(argv[2])) : 128;
    const int minLoop = argc > 3 ? std::max(0, parseInt(argv[3])) : 3;
    const int stem1 = argc > 4 ? std::max(1, parseInt(argv[4])) : 2;
    const int stem2 = argc > 5 ? std::max(1, parseInt(argv[5])) : stem1;
    const int repeats = argc > 6 ? std::max(1, parseInt(argv[6])) : 1;
    std::string sequence;
    sequence.reserve(length);
    for (int i = 0; i < length; ++i) sequence += i & 1 ? 'U' : 'A';
    const auto begin = std::chrono::steady_clock::now();
    ribo::runtime::FoldResult result;
    for (int run = 0; run < repeats; ++run) result = ribo::runtime::foldHType(sequence, minLoop, minLoop, minLoop, stem1, stem2, 1);
    const auto end = std::chrono::steady_clock::now();
    std::cout << "kernel=h_type_bounded_dp length=" << length << " min_loop=" << minLoop
              << " stem1=" << stem1 << " stem2=" << stem2 << " score=" << result.score << " runs=" << repeats
              << " elapsed_us=" << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "\n";
    return 0;
  }
  if (argc > 1 && std::string_view(argv[1]) == "--sweep") {
    const int first = argc > 2 ? std::max(1, parseInt(argv[2])) : 32;
    const int last = argc > 3 ? std::max(first, parseInt(argv[3])) : 512;
    const int step = argc > 4 ? std::max(1, parseInt(argv[4])) : first;
    std::cout << "length,score,elapsed_us\n";
    for (int length = first;;) {
      std::string sequence;
      sequence.reserve(length);
      for (int i = 0; i < length; ++i) sequence += i & 1 ? 'U' : 'A';
      const auto begin = std::chrono::steady_clock::now();
      const auto result = ribo::runtime::foldNested(sequence, 3, 1);
      const auto end = std::chrono::steady_clock::now();
      std::cout << length << ',' << result.score << ','
                << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << '\n';
      if (length > last - step) break;
      length += step;
    }
    return 0;
  }
  const int length = argc > 1 ? std::max(1, parseInt(argv[1])) : 128;
  const int minLoop = argc > 2 ? std::max(0, parseInt(argv[2])) : 3;
  const int repeats = argc > 3 ? std::max(1, parseInt(argv[3])) : 1;
  std::string sequence;
  sequence.reserve(length);
  for (int i = 0; i < length; ++i) sequence += i & 1 ? 'U' : 'A';
  const auto begin = std::chrono::steady_clock::now();
  ribo::runtime::FoldResult result;
  for (int run = 0; run < repeats; ++run) result = ribo::runtime::foldNested(sequence, minLoop, 1);
  const auto end = std::chrono::steady_clock::now();
  const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
  std::cout << "kernel=nested_triangular_dp length=" << length << " min_loop=" << minLoop
            << " score=" << result.score << " runs=" << repeats << " elapsed_us=" << micros << "\n";
  } catch (const std::exception &error) {
    std::cerr << "ribo-benchmark: invalid argument: " << error.what() << "\n";
    return 2;
  }
}
