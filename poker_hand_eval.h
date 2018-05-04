#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <vector>

// Main class for evaluating poker hands.
//
// Example usage:
//   PokerHandEval<7> phe("/path/to/table7.phe");
//   auto score = phe.eval(37, 0, 48, 26, 7, 5, 8);
//
// Also works with containers:
//   std::vector<int> hand1{37, 0, 48, 26, 7, 5, 8};
//   std::array<uint32_t, 7> hand2{37, 0, 48, 26, 7, 5, 8};
//   phe.eval(hand1) == phe.eval(hand2);
template <uint8_t hand_size>
class PokerHandEval {
 public:
  PokerHandEval(const std::string& path);
  PokerHandEval(const PokerHandEval&) = delete;
  PokerHandEval(PokerHandEval&&) = default;

  template <typename... CardType>
  uint32_t eval(CardType... hand) const;

  template <typename Container>
  uint32_t eval(const Container& hand) const;

 private:
  std::vector<uint32_t> table_;
};

//////////////////////////////////
// Implementation details below //
//////////////////////////////////

namespace details {

template <uint8_t hand_size>
struct EvalHelper;

template <>
struct EvalHelper<0> {
  static constexpr uint32_t eval_cards(const std::vector<uint32_t>&) {
    return 0;
  }

  template <typename Iterator>
  static constexpr uint32_t eval_iterator(const std::vector<uint32_t>&, Iterator) {
    return 0;
  }
};

template <uint8_t hand_size>
struct EvalHelper {
  template <typename CardType, typename... Tail>
  static constexpr uint32_t eval_cards(const std::vector<uint32_t>& table,
                                       CardType first,
                                       Tail... rest) {
    static_assert(sizeof...(rest) + 1 == hand_size, "Wrong number of arguments.");
    return table[first + EvalHelper<hand_size - 1>::eval_cards(table, rest...)];
  }

  template <typename Iterator>
  static constexpr uint32_t eval_iterator(const std::vector<uint32_t>& table,
                                          Iterator it) {
    return table[*it + EvalHelper<hand_size - 1>::eval_iterator(table, std::next(it))];
  }
};

}  // namespace details

template <uint8_t hand_size>
PokerHandEval<hand_size>::PokerHandEval(const std::string& path) {
  std::ifstream file(path, std::ios::in | std::ifstream::binary);

  file.seekg(0, std::ios::end);
  auto num_bytes = file.tellg();
  file.seekg(0, std::ios::beg);

  table_.resize(num_bytes / sizeof(uint32_t));

  file.read(reinterpret_cast<char*>(table_.data()), num_bytes);
  file.close();
}

template <uint8_t hand_size>
template <typename... CardType>
uint32_t PokerHandEval<hand_size>::eval(CardType... hand) const {
  return details::EvalHelper<hand_size>::eval_cards(table_, hand...);
}

template <uint8_t hand_size>
template <typename Container>
uint32_t PokerHandEval<hand_size>::eval(const Container& hand) const {
  return details::EvalHelper<hand_size>::eval_iterator(table_, std::begin(hand));
}
