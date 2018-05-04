#pragma once

#include <unordered_map>
#include <vector>

#include "generate_tables/common.h"

namespace poker_eval {

// A finite-state-machine that uses cards, as defined in common.h, as the input
// alphabet. The states are representative hands from the equivalence class of
// possible scores.
// After hand_size hops, the state becomes the score.
//
// fsm[card_0][card_1][card_2]...[card_(max_hand_size-1)] -> score
using FSM = std::unordered_map<EncodedHand, MapCardTo<HandOrScore>>;

// Builds a finite-state-machine for hands of the current size, using the given
// evaluation function.
//
// Note: for efficiency reasons, hand representations are compacted and
// hand_size cannot exceed seven.
template <uint8_t hand_size>
FSM build_fsm(EvalFn eval_fn);

}  // namespace poker_eval

#include "generate_tables/fsm.inl"
