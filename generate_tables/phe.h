#pragma once

#include <map>

#include "generate_tables/common.h"
#include "generate_tables/memory_layout.h"

namespace poker_eval {

// Generates a set of files that can be used by poker_hand_eval.h. The files
// contain a lookup table that produce evaluations matching the evaluations
// produced by the eval_fn provided here.
// layout_files is a mapping from filename to state-layout-order.
template <uint8_t hand_size>
void build_phes(
    EvalFn eval_fn,
    const std::map<std::string, MemoryLayoutFn<hand_size>>& layout_files);

}  // namespace poker_eval

#include "generate_tables/phe.inl"
