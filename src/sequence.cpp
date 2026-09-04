#include "ribo/Sequence.h"

namespace ribo::sequence {
std::string normalize(std::string_view input) {
  std::string result;
  result.reserve(input.size());
  bool header = false;
  for (const char raw : input) {
    char c = raw;
    if (result.empty() && c == '>') { header = true; continue; }
    if (header) { if (c == '\n' || c == '\r') header = false; continue; }
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    result.push_back(c);
  }
  return result;
}
}
