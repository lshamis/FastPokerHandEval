#include <algorithm>
#include <array>
#include <iostream>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

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
  static size_t deal_index = 52;
  if (deal_index + HandSize >= 52) {
    std::random_shuffle(std::begin(deck), std::end(deck));
    deal_index = 0;
  }

  HandType<HandSize> hand;
  std::copy_n(std::next(std::begin(deck), deal_index), HandSize, std::begin(hand));
  deal_index += HandSize;
  return hand;
}

void BM_Control5(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(random_hand<5>());
  }
}
BENCHMARK(BM_Control5);

void BM_phe_dfs5(benchmark::State& state) {
  PokerHandEval<5> phe("tables/dfs5.phe");
  for (auto _ : state) {
    benchmark::DoNotOptimize(phe.eval(random_hand<5>()));
  }
}
BENCHMARK(BM_phe_dfs5);

void BM_phe_bfs5(benchmark::State& state) {
  PokerHandEval<5> phe("tables/bfs5.phe");
  for (auto _ : state) {
    benchmark::DoNotOptimize(phe.eval(random_hand<5>()));
  }
}
BENCHMARK(BM_phe_bfs5);

void BM_phe_veb5(benchmark::State& state) {
  PokerHandEval<5> phe("tables/veb5.phe");
  for (auto _ : state) {
    benchmark::DoNotOptimize(phe.eval(random_hand<5>()));
  }
}
BENCHMARK(BM_phe_veb5);

void BM_Control7(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(random_hand<7>());
  }
}
BENCHMARK(BM_Control7);

void BM_phe_dfs7(benchmark::State& state) {
  PokerHandEval<7> phe("tables/dfs7.phe");
  for (auto _ : state) {
    benchmark::DoNotOptimize(phe.eval(random_hand<7>()));
  }
}
BENCHMARK(BM_phe_dfs7);

void BM_phe_bfs7(benchmark::State& state) {
  PokerHandEval<7> phe("tables/bfs7.phe");
  for (auto _ : state) {
    benchmark::DoNotOptimize(phe.eval(random_hand<7>()));
  }
}
BENCHMARK(BM_phe_bfs7);

void BM_phe_veb7(benchmark::State& state) {
  PokerHandEval<7> phe("tables/veb7.phe");
  for (auto _ : state) {
    benchmark::DoNotOptimize(phe.eval(random_hand<7>()));
  }
}
BENCHMARK(BM_phe_veb7);

BENCHMARK_MAIN();
