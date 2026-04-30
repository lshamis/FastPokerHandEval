#include <algorithm>
#include <array>
#include <iostream>
#include <utility>
#include <vector>
#include <random>

#define ANKERL_NANOBENCH_IMPLEMENT
#include "third_party/nanobench/nanobench.h"
#include "poker_hand_eval.h"

template <size_t HandSize>
using HandType = std::array<uint32_t, HandSize>;
using DeckType = std::array<uint32_t, 52>;

template <size_t HandSize>
HandType<HandSize> random_hand() {
  static DeckType deck = []() {
    DeckType d;
    for (size_t i = 0; i < 52; i++) { d[i] = i; }
    return d;
  }();
  static std::mt19937 g(42);
  static size_t deal_index = 52;
  if (deal_index + HandSize >= 52) {
    std::shuffle(std::begin(deck), std::end(deck), g);
    deal_index = 0;
  }

  HandType<HandSize> hand;
  std::copy_n(std::next(std::begin(deck), deal_index), HandSize, std::begin(hand));
  deal_index += HandSize;
  return hand;
}

int main() {
  ankerl::nanobench::Bench bench;

  bench.run("Control5", [&]() {
    ankerl::nanobench::doNotOptimizeAway(random_hand<5>());
  });

  {
    PokerHandEval<5> phe("tables/bfs5.phe");
    bench.run("bfs5", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<5>()));
    });
  }

  {
    PokerHandEval<5> phe("tables/dfs5.phe");
    bench.run("dfs5", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<5>()));
    });
  }

  {
    PokerHandEval<5> phe("tables/veb5.phe");
    bench.run("veb5", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<5>()));
    });
  }

  bench.run("Control7", [&]() {
    ankerl::nanobench::doNotOptimizeAway(random_hand<7>());
  });

  {
    PokerHandEval<7> phe("tables/bfs7.phe");
    bench.run("bfs7", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<7>()));
    });
  }

  {
    PokerHandEval<7> phe("tables/dfs7.phe");
    bench.run("dfs7", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<7>()));
    });
  }

  {
    PokerHandEval<7> phe("tables/veb7.phe");
    bench.run("veb7", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<7>()));
    });
  }
}
