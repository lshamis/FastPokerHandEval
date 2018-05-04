#pragma once

#include <functional>
#include <vector>

#include "generate_tables/fsm.h"

namespace poker_eval {

// Family of functions that produce a state ordering, used to flatten
// a finite-state-machine.
template <uint8_t hand_size>
using MemoryLayoutFn = std::function<std::vector<EncodedHand>(const FSM&)>;

// Lay's out the states in the order visited by breadth-first search.
template <uint8_t hand_size>
std::vector<EncodedHand> bfs_memory_order(const FSM& fsm);

// Lay's out the states in the order visited by depth-first search.
template <uint8_t hand_size>
std::vector<EncodedHand> dfs_memory_order(const FSM& fsm);

// Lay's out the states in the Van Emde Boas order.
template <uint8_t hand_size>
std::vector<EncodedHand> veb_memory_order(const FSM& fsm);

// Flattens a finite-state-machine, given the ordering of states.
// Use the above functions to create a state-ordering.
template <uint8_t hand_size>
std::vector<uint32_t> flatten_fsm(const FSM& fsm,
                                  const std::vector<EncodedHand>& order);

}  // namespace poker_eval

#include "generate_tables/memory_layout.inl"
