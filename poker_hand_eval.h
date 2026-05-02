#pragma once

#include <algorithm>
#include <array>
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

  template <typename Fn>
  void sweep(Fn fn) const;

  template <typename Container, typename Fn>
  void sweep(const Container& prefix, Fn fn) const;

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

template <uint8_t hand_size>
template <typename Fn>
void PokerHandEval<hand_size>::sweep(Fn fn) const {
  sweep(std::array<uint32_t, 0>{}, fn);
}

template <uint8_t hand_size>
template <typename Container, typename Fn>
void PokerHandEval<hand_size>::sweep(const Container& prefix, Fn fn) const {
  uint32_t stack[hand_size + 1] = {};
  std::array<uint32_t, hand_size> hand;

  // Populate with prefix cards.
  uint32_t seen_cards[52] = {};
  for (uint32_t i = 0; i < prefix.size(); i++) {
    hand[i] = prefix[i];
    stack[i + 1] = table_[stack[i] + hand[i]];

    seen_cards[hand[i]] = true;
  }

  // Build deck with remaining cards.
  uint32_t deck_size = 52 - prefix.size();
  uint32_t deck[deck_size];
  uint32_t deck_lookup[52];
  for (uint32_t di = 0, c = 0; c < 52; c++) {
    if (!seen_cards[c]) {
      deck[di] = c;
      deck_lookup[c] = di;
      di++;
    }
  }

  // Create first legal hand.
  for (uint32_t hand_idx = prefix.size(); hand_idx < hand_size; hand_idx++) {
    hand[hand_idx] = deck[hand_idx - prefix.size()];
    stack[hand_idx + 1] = table_[stack[hand_idx] + hand[hand_idx]];
  }
  fn(hand, stack[hand_size]);

  // Generate all remaining hands.
  while (true) {
    uint32_t start_idx = hand_size - 1;
    while (start_idx >= prefix.size()) {
      uint32_t deck_idx = deck_lookup[hand[start_idx]];
      uint32_t max_deck_idx = deck_size - (hand_size - start_idx);

      if (deck_idx < max_deck_idx) {
        // Found the rightmost position that can be incremented.
        break;
      }
      if (start_idx == prefix.size()) {
        // Cannot advance any further (last combination).
        return;
      }
      start_idx--;
    }
    if (start_idx < prefix.size()) {
      return;
    }

    // Advance the pivot and refill the tail with the smallest possible cards.
    uint32_t deck_idx = deck_lookup[hand[start_idx]] + 1;
    for (uint32_t hand_idx = start_idx; hand_idx < hand_size; hand_idx++, deck_idx++) {
      hand[hand_idx] = deck[deck_idx];
      stack[hand_idx + 1] = table_[stack[hand_idx] + hand[hand_idx]];
    }
    fn(hand, stack[hand_size]);
  }
}
