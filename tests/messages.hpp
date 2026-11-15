#pragma once

// Test helpers: build ITCH 5.0 messages byte by byte.
// Field layouts and sizes come from the ITCH 5.0 spec, not from netrx.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace test {

// Appends value to out, most significant byte first.
template <typename T> void putBigEndian(std::vector<char> &out, T value) {
  for (int shift = (sizeof(T) - 1) * 8; shift >= 0; shift -= 8) {
    out.push_back(static_cast<char>((value >> shift) & 0xFF));
  }
}

// Appends text to out, one byte per character.
inline void putText(std::vector<char> &out, const std::string &text) {
  for (char c : text) {
    out.push_back(c);
  }
}

// Returns a new vector holding exactly bytes [0, length) of src.
// A fresh vector built from a range has capacity == size, so ASan
// catches any read past the end. If length > src.size(), pads with 0.
inline std::vector<char> exactCopy(const std::vector<char> &src,
                                   std::size_t length) {
  std::vector<char> padded = src;
  padded.resize(length, 0);
  return std::vector<char>(padded.begin(), padded.end());
}

// Full message sizes from the ITCH 5.0 spec, header included.
struct TypeSize {
  char type;
  std::size_t size;
};
inline const std::vector<TypeSize> kSpecSizes = {
    {'S', 12}, {'R', 39}, {'A', 36}, {'F', 40}, {'E', 31},
    {'C', 36}, {'X', 23}, {'D', 19}, {'U', 35},
};

// Header: type(1), stock locate(2), tracking number(2), timestamp(6).
inline void putHeader(std::vector<char> &out, char type) {
  out.push_back(type);
  putBigEndian<uint16_t>(out, 13);         // stock locate
  putBigEndian<uint16_t>(out, 0);          // tracking number
  putBigEndian<uint16_t>(out, 0x0000);     // timestamp, high 2 bytes
  putBigEndian<uint32_t>(out, 0x12345678); // timestamp, low 4 bytes
}

// S: event code(1)
inline std::vector<char> systemEvent(char eventCode) {
  std::vector<char> m;
  putHeader(m, 'S');
  m.push_back(eventCode);
  return exactCopy(m, m.size());
}

// R: stock(8), market category(1), financial status(1), round lot size(4),
// round lots only(1), issue classification(1), issue sub-type(2),
// authenticity(1), short sale threshold(1), IPO flag(1), LULD tier(1),
// ETP flag(1), ETP leverage factor(4), inverse indicator(1)
inline std::vector<char> stockDirectory() {
  std::vector<char> m;
  putHeader(m, 'R');
  putText(m, "AAPL    ");
  m.push_back('Q');
  m.push_back('N');
  putBigEndian<uint32_t>(m, 100);
  m.push_back('N');
  m.push_back('C');
  putText(m, "Z ");
  m.push_back('P');
  m.push_back('N');
  m.push_back('N');
  m.push_back('1');
  m.push_back('N');
  putBigEndian<uint32_t>(m, 0);
  m.push_back('N');
  return exactCopy(m, m.size());
}

// A: order ref(8), buy/sell(1), shares(4), stock(8), price(4)
inline std::vector<char> addOrder(uint64_t orderRef) {
  std::vector<char> m;
  putHeader(m, 'A');
  putBigEndian<uint64_t>(m, orderRef);
  m.push_back('B');
  putBigEndian<uint32_t>(m, 100);
  putText(m, "AAPL    ");
  putBigEndian<uint32_t>(m, 1'500'000);
  return exactCopy(m, m.size());
}

// F: same as A, then attribution(4)
inline std::vector<char> addOrderMpid(uint64_t orderRef) {
  std::vector<char> m;
  putHeader(m, 'F');
  putBigEndian<uint64_t>(m, orderRef);
  m.push_back('B');
  putBigEndian<uint32_t>(m, 100);
  putText(m, "AAPL    ");
  putBigEndian<uint32_t>(m, 1'500'000);
  putText(m, "MPID");
  return exactCopy(m, m.size());
}

// E: order ref(8), executed shares(4), match number(8)
inline std::vector<char> orderExecuted(uint64_t orderRef) {
  std::vector<char> m;
  putHeader(m, 'E');
  putBigEndian<uint64_t>(m, orderRef);
  putBigEndian<uint32_t>(m, 50);
  putBigEndian<uint64_t>(m, 777);
  return exactCopy(m, m.size());
}

// C: order ref(8), executed shares(4), match number(8), printable(1),
// execution price(4)
inline std::vector<char> orderExecutedWithPrice(uint64_t orderRef) {
  std::vector<char> m;
  putHeader(m, 'C');
  putBigEndian<uint64_t>(m, orderRef);
  putBigEndian<uint32_t>(m, 50);
  putBigEndian<uint64_t>(m, 778);
  m.push_back('Y');
  putBigEndian<uint32_t>(m, 1'500'000);
  return exactCopy(m, m.size());
}

// X: order ref(8), cancelled shares(4)
inline std::vector<char> orderCancel(uint64_t orderRef) {
  std::vector<char> m;
  putHeader(m, 'X');
  putBigEndian<uint64_t>(m, orderRef);
  putBigEndian<uint32_t>(m, 10);
  return exactCopy(m, m.size());
}

// D: order ref(8)
inline std::vector<char> orderDelete(uint64_t orderRef) {
  std::vector<char> m;
  putHeader(m, 'D');
  putBigEndian<uint64_t>(m, orderRef);
  return exactCopy(m, m.size());
}

// U: original order ref(8), new order ref(8), shares(4), price(4)
inline std::vector<char> orderReplace(uint64_t originalRef, uint64_t newRef) {
  std::vector<char> m;
  putHeader(m, 'U');
  putBigEndian<uint64_t>(m, originalRef);
  putBigEndian<uint64_t>(m, newRef);
  putBigEndian<uint32_t>(m, 100);
  putBigEndian<uint32_t>(m, 1'600'000);
  return exactCopy(m, m.size());
}

// A valid message of the given handled type.
inline std::vector<char> validMessage(char type) {
  switch (type) {
  case 'S': return systemEvent('O');
  case 'R': return stockDirectory();
  case 'A': return addOrder(1);
  case 'F': return addOrderMpid(1);
  case 'E': return orderExecuted(1);
  case 'C': return orderExecutedWithPrice(1);
  case 'X': return orderCancel(1);
  case 'D': return orderDelete(1);
  case 'U': return orderReplace(1, 2);
  default: return {};
  }
}

// Appends a 2-byte big-endian length prefix, then the message.
inline void putFramed(std::vector<char> &stream, uint16_t length,
                      const std::vector<char> &message) {
  putBigEndian<uint16_t>(stream, length);
  for (char c : message) {
    stream.push_back(c);
  }
}

} // namespace test
