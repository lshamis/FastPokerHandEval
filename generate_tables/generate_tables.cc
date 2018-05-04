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

Score eval5(const Hand& hand) {
  const std::vector<int>& ck_deck = deck();
  int ck_hand[5] = {ck_deck[hand.cards[0]],
                    ck_deck[hand.cards[1]],
                    ck_deck[hand.cards[2]],
                    ck_deck[hand.cards[3]],
                    ck_deck[hand.cards[4]]};
  return eval_5hand(ck_hand);
}

Score eval7(const Hand& hand) {
  const std::vector<int> ck_deck = deck();
  int ck_hand[7] = {ck_deck[hand.cards[0]],
                    ck_deck[hand.cards[1]],
                    ck_deck[hand.cards[2]],
                    ck_deck[hand.cards[3]],
                    ck_deck[hand.cards[4]],
                    ck_deck[hand.cards[5]],
                    ck_deck[hand.cards[6]]};
  return eval_7hand(ck_hand);
}

}  // namespace cactus_kev

// Generates tables for 5 and 7-card poker hands, using various layout schemes.
// We use the cactus_kev eval, which uses the following int-to-card matching:
//   0 -> 2c
//   1 -> 3c
//   2 -> 4c
//  ..
//  11 -> Kc
//  12 -> Ac
//  13 -> 2d
//  ..
//  51 -> As
//
// You may choose a different mapping by switching out the eval to one of your
// choice.
// Note that the cards values must be in the range [0, 52).
int main() {
  build_phes<5>(cactus_kev::eval5, {{"tables/bfs5.phe", bfs_memory_order<5>},
                                    {"tables/dfs5.phe", dfs_memory_order<5>},
                                    {"tables/veb5.phe", veb_memory_order<5>}});

  build_phes<7>(cactus_kev::eval7, {{"tables/bfs7.phe", bfs_memory_order<7>},
                                    {"tables/dfs7.phe", dfs_memory_order<7>},
                                    {"tables/veb7.phe", veb_memory_order<7>}});
}
