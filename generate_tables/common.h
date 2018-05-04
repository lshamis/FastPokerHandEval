#pragma once

#include <array>
#include <cstdint>
#include <functional>

namespace poker_eval {

// Cards are represented as integers in the range [0, 52).
// The interpretation of those values are at the discretion of the
// EvalFn, defined below.
using Card = uint8_t;

// For simplicity and efficiency, a hand of cards is defined to have seven
// or fewer cards.
// Since each card takes one byte (with one byte reserved for size), a hand can
// be encoded into an eight byte structure.
using EncodedHand = uint64_t;

// A score is the valuation of a complete hand of cards, as provided by the
// EvalFn, defined below.
// Following previous convention and to more easily fit into a flattened
// finite-state-machine, scores are defined to be 32-bit unsigned integers.
using Score = uint32_t;

// A union of EncodedHand and Score, used to label nodes in the
// finite-state-machine.
// EncodedHand are used for non-terminal nodes.
// Scores are used for terminal nodes.
//
// Lucky for us, both are defined as uint64_t, so we don't need to delve into
// real union shenanigans.
// If we decide to switch Score to be a double, this might need to become a real
// union.
using HandOrScore = uint64_t;

// Hand is a container of up-to seven Cards.
struct Hand {
  // The number of Cards in the `cards` field that are meaningfully populated.
  // The remaining entries are garbage memory.
  uint8_t size = 0;
  // The set of cards in the hand.
  // These should be kept sorted. This struct does not guarantee this invariant.
  Card cards[7] = {0, 0, 0, 0, 0, 0, 0};

  // Constructs a Hand object from an encoded version of the hand.
  static Hand decode(EncodedHand encoded);

  // Returns an encoded version of this hand.
  EncodedHand encode() const;

  // Returns a human-readable string describing the content of this hand.
  std::string debug_string() const;
};

// Efficient associative container, keyed off cards.
template <typename T>
using MapCardTo = std::array<T, 52>;

// Function that returns the valuation of a completed hand of cards.
// This is used as a bootstrap to construct a more efficient evaluator.
// This is the only place where card values, integers in the range [0, 52),
// are given an interpretation.
using EvalFn = std::function<Score(const Hand&)>;

// Utility method that executes a given callback for each valid hand of a given
// size.
void for_each_hand(uint8_t desired_hand_size,
                   std::function<void(const Hand&)> fn);

}  // namespace poker_eval
