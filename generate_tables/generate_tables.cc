#include <array>
#include <numeric>
#include <vector>

#include "generate_tables/memory_layout.h"
#include "generate_tables/phe.h"
#include "third_party/senzee/poker.h"

using namespace poker_eval;

namespace cactus_kev {

const std::vector<int>& deck() {
  static std::vector<int> ck_deck = []() {
    std::vector<int> tmp_deck(52);
    init_deck(tmp_deck.data());
    return tmp_deck;
  }();
  return ck_deck;
}

using IdMap = std::array<Card, 52>;

IdMap suit_major_map() {
  IdMap m{};
  std::iota(m.begin(), m.end(), 0);
  return m;
}

IdMap rank_major_map() {
  IdMap m{};
  for (uint8_t rank = 0; rank < 13; rank++) {
    for (uint8_t suit = 0; suit < 4; suit++) {
      const Card phe_id = static_cast<Card>(rank * 4 + suit);
      const Card ck_id = static_cast<Card>(suit * 13 + rank);
      m[phe_id] = ck_id;
    }
  }
  return m;
}

Score eval5_with_map(const Hand& hand, const IdMap& id_map) {
  const std::vector<int>& ck_deck = deck();
  int ck_hand[5] = {ck_deck[id_map[hand.cards[0]]],
                    ck_deck[id_map[hand.cards[1]]],
                    ck_deck[id_map[hand.cards[2]]],
                    ck_deck[id_map[hand.cards[3]]],
                    ck_deck[id_map[hand.cards[4]]]};
  return eval_5hand(ck_hand);
}

Score eval7_with_map(const Hand& hand, const IdMap& id_map) {
  const std::vector<int> ck_deck = deck();
  int ck_hand[7] = {ck_deck[id_map[hand.cards[0]]],
                    ck_deck[id_map[hand.cards[1]]],
                    ck_deck[id_map[hand.cards[2]]],
                    ck_deck[id_map[hand.cards[3]]],
                    ck_deck[id_map[hand.cards[4]]],
                    ck_deck[id_map[hand.cards[5]]],
                    ck_deck[id_map[hand.cards[6]]]};
  return eval_7hand(ck_hand);
}

}  // namespace cactus_kev

// Generates tables for 5 and 7-card poker hands, using various layout schemes.
// We use the cactus_kev eval, which uses the following int-to-card matching:
//   0 -> 2c
//   1 -> 2d
//   2 -> 2h
//   3 -> 2s
//   4 -> 3c
//  ..
//  51 -> As
//
// Rank-major ordering is used to improve cache efficiency by keeping
// same-rank transitions adjacent in the lookup table.
//
// You may choose a different mapping by switching out the eval to one of your
// choice.
// Note that the cards values must be in the range [0, 52).
int main() {
  const cactus_kev::IdMap id_map = cactus_kev::rank_major_map();

  build_phes<5>([&id_map](const Hand& hand) { return cactus_kev::eval5_with_map(hand, id_map); }, {
                                    {"tables/bfs5.phe", bfs_memory_order<5>},
                                    {"tables/dfs5.phe", dfs_memory_order<5>},
                                    {"tables/veb5.phe", veb_memory_order<5>}});

  build_phes<7>([&id_map](const Hand& hand) { return cactus_kev::eval7_with_map(hand, id_map); }, {
                                    {"tables/bfs7.phe", bfs_memory_order<7>},
                                    {"tables/dfs7.phe", dfs_memory_order<7>},
                                    {"tables/veb7.phe", veb_memory_order<7>}});
}
