#include "generate_tables/common.h"

#include <sstream>

namespace poker_eval {

Hand Hand::decode(EncodedHand encoded) {
  static_assert(sizeof(Hand) == sizeof(EncodedHand),
                "EncodedHand cannot be reinterpreted as Hand.");
  return *reinterpret_cast<Hand*>(&encoded);
}

EncodedHand Hand::encode() const {
  static_assert(sizeof(Hand) == sizeof(EncodedHand),
                "Hand cannot be reinterpreted as EncodedHand.");
  return *reinterpret_cast<const EncodedHand*>(this);
}

std::string Hand::debug_string() const {
  std::stringstream ss;
  ss << "{";
  for (size_t i = 0; i < size; i++) {
    if (i > 0) {
      ss << ", ";
    }
    ss << int(cards[i]);
  }
  ss << "}";
  return ss.str();
}

void for_each_hand(uint8_t desired_hand_size,
                   std::function<void(const Hand&)> fn) {
  Hand current_hand;
  current_hand.size = desired_hand_size;

  for (uint8_t i = 0; i < desired_hand_size; ++i) {
    current_hand.cards[i] = i;
  }
  fn(current_hand);

  if (desired_hand_size == 0) {
    return;
  }

  while (true) {
    int i = desired_hand_size - 1;
    ++current_hand.cards[i];

    while (current_hand.cards[i] > 52 + i - desired_hand_size) {
      --i;
      if (i < 0) {
        return;
      }
      ++current_hand.cards[i];
    }

    for (; i < desired_hand_size - 1; ++i) {
      current_hand.cards[i + 1] = current_hand.cards[i] + 1;
    }

    fn(current_hand);
  }
}

}  // namespace poker_eval
