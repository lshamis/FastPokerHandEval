namespace poker_eval {

// Whether the given card-keyed associative container contains the given card.
template <typename T>
inline bool has_card(const MapCardTo<T>& c, Card k) {
  return c[k] != 0;
}

// Execute a callback function for each legal next hand.
// The next hand will have all cards from the given hand, plus an additional
// card not already in the hand.
// The initial hand is assumed to be sorted, and the hands provided to the
// callback are guaranteed be sorted.
inline void for_each_next_hand(const Hand& hand,
                               std::function<void(Card, const Hand&)> fn) {
  Hand next_hand = hand;
  // Create an empty slot for a new card at the end of the list.
  next_hand.size++;
  // Iterate over each card that can go in the currently empty slot, then shift
  // the empty slot over by one.
  for (int i = static_cast<int>(hand.size); i >= 0; i--) {
    // The start of the valid card range is the value of the card in the slot
    // before the empty slot (+1 to exclude that card value).
    // If the empty slot is at the very beginning, the valid card lower bound
    // is 0.
    Card start_range = (i == 0 ? 0 : hand.cards[i - 1] + 1);
    // The start of the valid card range is the value of the card in the slot
    // after the empty slot.
    // If the empty slot is at the very end, the valid card upper bound is 52.
    Card end_range = (i == static_cast<int>(hand.size) ? 52 : hand.cards[i]);
    // Iterate over the range of valid cards for the empty slot.
    for (Card card = start_range; card < end_range; card++) {
      // Populate the empty slot and execute the callback.
      next_hand.cards[i] = card;
      fn(card, next_hand);
    }
    // Shift the empty slot, if there is still space,
    if (i != 0) {
      next_hand.cards[i] = next_hand.cards[i - 1];
    }
  }
}

// Mapping from a hand to a representative of the equivalence class.
// For example, if there are two seven-card hands that both use the same five
// cards for evaluation (and the other two cards don't matter), one may be
// arbitrarily set as a common representative for both.
// This is used to help collapse multiple states in the finite-state-machine.
using ToRepresentativeHand = absl::flat_hash_map<EncodedHand, EncodedHand>;

// Edges are the transitions emitting from a state. Each state has 52 out-edges
// (less for repeated cards) that point to another state.
using Edges = MapCardTo<HandOrScore>;

// Two edge sets are compatible if they have no disagreements.
// More precisely: For the sets to be compatible, each card-transition must
// result in the same target state.
// Missing card-transition do not effect compatible.
inline bool edges_compatible(const Edges& edge_set_1, const Edges& edge_set_2) {
  for (Card card = 0; card < 52; card++) {
    if (has_card(edge_set_1, card) &&
        has_card(edge_set_2, card) &&
        edge_set_1[card] != edge_set_2[card]) {
      return false;
    }
  }
  return true;
}

// An equivalence class, here, is a collections of hands that have a compatible
// set of edges, e.g. hands that react identically to every future card.
struct EquivalenceClass {
  absl::flat_hash_set<EncodedHand> hands;
  Edges edges;
};

// EquivalenceClass objects are maintained in a master std::vector.
// This is a convenience type used to reference equivalence classes by their
// index within the master list.
using EquivalenceClassIndex = int32_t;
// Named constant for a non-existent equivalence class index, within a master
// list.
const EquivalenceClassIndex EquivalenceClassNotFound = -1;

// Efficient set implementation for small sets of integers with
// frequent iteration, rare insert, and nothing else.
template <typename T>
struct FlatSet {
  std::vector<T> items;

  void insert(T item) {
    if (std::find(begin(), end(), item) == end()) {
      items.push_back(item);
    }
  }

  typename std::vector<T>::iterator begin() { return items.begin(); }
  typename std::vector<T>::iterator end() { return items.end(); }
};

// Used to help find the appropriate equivalence class for a given state.
// Naively, we could iterate through all equivalence classes and see if any
// match. This helps us avoid iterating through all equivalence classes by
// allowing us to query for equivalence classes with certain edges.
//
// This is part of the inner-most loop of the code, and is critical to runtime
// performance.
// google::dense_hash_map provides efficient lookup.
// FlatSet provides efficient iteration.
using EquivalenceClassHintMap = MapCardTo<absl::flat_hash_map<HandOrScore, FlatSet<EquivalenceClassIndex>>>;

// Find a equivalence class index where the equivalence_class's edges are
// compatible with the given edges.
// For efficiency, a hint table `equivalence_class_counts` is required, which
// provides a mapping from equivalence class index to the number of matching
// edges.
inline EquivalenceClassIndex find_matching_equivalence_class(
    uint8_t hand_size,
    const Edges& edges,
    const std::vector<EquivalenceClass>& equivalence_classes,
    const absl::flat_hash_map<EquivalenceClassIndex, size_t>& equivalence_class_counts) {
  for (auto&& pair : equivalence_class_counts) {
    EquivalenceClassIndex equivalence_class_idx = pair.first;
    size_t count = pair.second;
    // Ideally (and impossibly), the equivalence class would match for all cards
    // and have a count of 52. There are two things that reduce the count:
    //
    //   1) A card would result in a different states. This invalidates the
    //      equivalence_class match.
    //
    //   2) A card is not a legal fsm transition, because it was already seen.
    //      For example, assume flushes have been ruled out for two hands. One
    //      of the hands was used to construct the equivalence class, and has no
    //      defined transition for the two of spades (because the two of spades
    //      is already in the hand). The second hand might have a two of clubs
    //      (thereby not having a defined transition for it). But the hands are
    //      equivalent. In all, the equivalent hands have their count reduced by
    //      two.
    //
    // Decrements due to (2) are ok. Decrements due to (1) are not ok.
    // Decrements due to (2) are limited to the number of cards across both
    // hands. This limit might even be tighter.
    // edges_compatible is a final decider, but a cheap filter is added to
    // quickly check whether the number of decrements exceeds that possible by
    // case (2).
    if (count >= 52u - (2u * hand_size) &&
        edges_compatible(equivalence_classes[equivalence_class_idx].edges, edges)) {
      return equivalence_class_idx;
    }
  }

  return EquivalenceClassNotFound;
}

// Populates a given equivalence_class with the given edges.
// Excludes not-defined transitions and updates the hint map.
inline void populate_equivalence_class_edges(const Edges& edges,
                                             EquivalenceClass* equivalence_class,
                                             EquivalenceClassIndex equivalence_class_idx,
                                             EquivalenceClassHintMap* equivalence_class_hints) {
  for (Card card = 0; card < 52; card++) {
    if (has_card(edges, card)) {
      HandOrScore target = edges[card];
      equivalence_class->edges[card] = target;
      (*equivalence_class_hints)[card][target].insert(equivalence_class_idx);
    }
  }
}

// Selects a representative hand to describe the equivalence class and populates
// associated data structures.
inline EncodedHand collapse_equivalence_class(
    const EquivalenceClass& equivalence_class,
    ToRepresentativeHand* representative_hand_map) {
  // Choose an arbitrary hand to act as the representative.
  EncodedHand representative_hand = *equivalence_class.hands.begin();

  // Update the representative hand map, so that all hands in the equivalence
  // class point to the same representative hand.
  for (EncodedHand hand : equivalence_class.hands) {
    (*representative_hand_map)[hand] = representative_hand;
  }

  return representative_hand;
}

// Add the equivalence_class to the final finite-state-machine.
inline void add_equivalence_class_to_fsm(const EquivalenceClass& equivalence_class,
                                         EncodedHand representative_hand,
                                         FSM* fsm) {
  // Add the representative hand and the equivalence class's collective edges to
  // the final finite-state-machine.
  for (Card card = 0; card < 52; card++) {
    if (has_card(equivalence_class.edges, card)) {
      (*fsm)[representative_hand][card] = equivalence_class.edges[card];
    }
  }
}

// Populates the representative hand map and finite-state-machine with the
// equivalence classes for hands of the given size.
//
// This requires that equivalence classes have been already been built up for
// hands of size hand_size+1. Hands with hand_size == max_hand_size are
// implicitly collapsed based on the given eval_fn.
inline void build_hands_of_size(uint8_t hand_size,
                                uint8_t max_hand_size,
                                EvalFn eval_fn,
                                ToRepresentativeHand* representative_hand_map,
                                FSM* fsm) {
  std::vector<EquivalenceClass> equivalence_classes;
  EquivalenceClassHintMap equivalence_class_hints;

  // For each hand, we collect the out edges and try to find a valid matching
  // equivalence class.
  // If one is found, the hand is added to the equivalence class and the class's
  // out edges are updated. The update is because out edges may contain
  // `don't-care` connections that are collapsed into a single state.
  // Otherwise, a new equivalence class is created.
  for_each_hand(hand_size, [&](const Hand& hand) {
    Edges edges;
    edges.fill(0);

    // An edge may be part of many equivalence classes. We're looking for
    // equivalence classes that show up for as many edges as possible.
    // This counts the number of times an equivalence class has been seen.
    absl::flat_hash_map<EquivalenceClassIndex, size_t> equivalence_class_counts;

    // Populate out edges.
    for_each_next_hand(hand, [&](Card card, const Hand& next_hand) {
      if (next_hand.size == max_hand_size) {
        // Hands of max size have an implicit state based on their evaluated
        // score.
        edges[card] = eval_fn(next_hand);
      } else {
        edges[card] = (*representative_hand_map)[next_hand.encode()];
      }

      // Increment possible equivalence classes.
      for (auto&& equivalence_class_idx : equivalence_class_hints[card][edges[card]]) {
        equivalence_class_counts[equivalence_class_idx]++;
      }
    });

    // Choose a definitive equivalence class for the hand.
    EquivalenceClassIndex equivalence_class_idx =
        find_matching_equivalence_class(hand_size, edges, equivalence_classes, equivalence_class_counts);

    // If no valid equivalence class exists, make a new one.
    // We add the current hand to the equivalence class later.
    if (equivalence_class_idx == EquivalenceClassNotFound) {
      equivalence_classes.push_back({{}, edges});
      equivalence_class_idx = equivalence_classes.size() - 1;
    }

    // Add the hand to the equivalence class.
    EquivalenceClass* matched_equivalence_class = &equivalence_classes[equivalence_class_idx];
    matched_equivalence_class->hands.insert(hand.encode());

    // Update the equivalence class.
    populate_equivalence_class_edges(edges, matched_equivalence_class, equivalence_class_idx, &equivalence_class_hints);
  });

  // Add all equivalence classes, for the current hand size, to the
  // finite-state-machine.
  for (auto&& equivalence_class : equivalence_classes) {
    EncodedHand representative_hand = collapse_equivalence_class(equivalence_class, representative_hand_map);
    add_equivalence_class_to_fsm(equivalence_class, representative_hand, fsm);
  }
  printf("  found %zu equivalence classes.\n", equivalence_classes.size());
}

template <uint8_t max_hand_size>
FSM build_fsm(EvalFn eval_fn) {
  FSM fsm;
  ToRepresentativeHand representative_hand_map;

  for (int hand_size = max_hand_size - 1; hand_size >= 0; hand_size--) {
    printf("  Processing hands of size: %d...", hand_size);
    build_hands_of_size(hand_size, max_hand_size, eval_fn, &representative_hand_map, &fsm);
  }

  return fsm;
}

}  // namespace poker_eval
