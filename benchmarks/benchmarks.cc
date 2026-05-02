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
  ankerl::nanobench::Bench latency_bench_5;
  latency_bench_5
      .title("5-cards latency")
      .unit("hand")
      .relative(true)
      .performanceCounters(true)
      .minEpochIterations(1000000);

  latency_bench_5.run("control5", [&]() {
    ankerl::nanobench::doNotOptimizeAway(random_hand<5>());
  });

  {
    PokerHandEval<5> phe("tables/bfs5.phe");
    latency_bench_5.run("bfs5", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<5>()));
    });
  }

  {
    PokerHandEval<5> phe("tables/dfs5.phe");
    latency_bench_5.run("dfs5", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<5>()));
    });
  }

  {
    PokerHandEval<5> phe("tables/veb5.phe");
    latency_bench_5.run("veb5", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<5>()));
    });
  }

  ankerl::nanobench::Bench latency_bench_7;
  latency_bench_7
      .title("7-cards latency")
      .unit("hand")
      .relative(true)
      .performanceCounters(true)
      .minEpochIterations(1000000);

  latency_bench_7.run("control7", [&]() {
    ankerl::nanobench::doNotOptimizeAway(random_hand<7>());
  });

  {
    PokerHandEval<7> phe("tables/bfs7.phe");
    latency_bench_7.run("bfs7", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<7>()));
    });
  }

  {
    PokerHandEval<7> phe("tables/dfs7.phe");
    latency_bench_7.run("dfs7", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<7>()));
    });
  }

  {
    PokerHandEval<7> phe("tables/veb7.phe");
    latency_bench_7.run("veb7", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<7>()));
    });
  }

  ankerl::nanobench::Bench throughput_bench_5;
  throughput_bench_5
      .title("5-cards throughput")
      .unit("hand")
      .epochs(1)
      .epochIterations(1)
      .batch(2598960)  // Number of 5-card hands.
      .performanceCounters(true);

  {
    PokerHandEval<5> phe("tables/bfs5.phe");
    throughput_bench_5.run("bfs5", [&]() {
      phe.sweep([](auto hand, auto score) { ankerl::nanobench::doNotOptimizeAway(score); });
    });
  }

  {
    PokerHandEval<5> phe("tables/dfs5.phe");
    throughput_bench_5.run("dfs5", [&]() {
      phe.sweep([](auto hand, auto score) { ankerl::nanobench::doNotOptimizeAway(score); });
    });
  }

  {
    PokerHandEval<5> phe("tables/veb5.phe");
    throughput_bench_5.run("veb5", [&]() {
      phe.sweep([](auto hand, auto score) { ankerl::nanobench::doNotOptimizeAway(score); });
    });
  }

  ankerl::nanobench::Bench throughput_bench_7;
  throughput_bench_7
      .title("7-cards throughput")
      .unit("hand")
      .epochs(1)
      .epochIterations(1)
      .batch(133784560)  // Number of 7-card hands.
      .performanceCounters(true);

  {
    PokerHandEval<7> phe("tables/bfs7.phe");
    throughput_bench_7.run("bfs7", [&]() {
      phe.sweep([](auto hand, auto score) { ankerl::nanobench::doNotOptimizeAway(score); });
    });
  }

  {
    PokerHandEval<7> phe("tables/dfs7.phe");
    throughput_bench_7.run("dfs7", [&]() {
      phe.sweep([](auto hand, auto score) { ankerl::nanobench::doNotOptimizeAway(score); });
    });
  }

  {
    PokerHandEval<7> phe("tables/veb7.phe");
    throughput_bench_7.run("veb7", [&]() {
      phe.sweep([](auto hand, auto score) { ankerl::nanobench::doNotOptimizeAway(score); });
    });
  }
}
