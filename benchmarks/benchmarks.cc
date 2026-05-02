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

uint64_t choose(uint64_t n, uint64_t k) {
    if (k > n) return 0;
    if (k == 0 || k == n) return 1;

    if (k > n / 2) k = n - k;

    uint64_t result = 1;
    for (uint64_t i = 1; i <= k; ++i) {
        result *= (n - i + 1);
        result /= i;
    }
    return result;
}


template <size_t HandSize>
void bench_latency() {
  std::cout << "\n\nBenchmarking " << HandSize << "-card hand evaluation latency...\n";

  ankerl::nanobench::Bench b;
  b
      .unit("hand")
      .warmup(10000)
      .relative(true)
      .performanceCounters(true)
      .minEpochIterations(10000000);

  b.run("control", [&]() {
    ankerl::nanobench::doNotOptimizeAway(random_hand<HandSize>());
  });

  {
    PokerHandEval<HandSize> phe("tables/bfs" + std::to_string(HandSize) + ".phe");
    b.run("bfs", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<HandSize>()));
    });
  }

  {
    PokerHandEval<HandSize> phe("tables/dfs" + std::to_string(HandSize) + ".phe");
    b.run("dfs", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<HandSize>()));
    });
  }

  {
    PokerHandEval<HandSize> phe("tables/veb" + std::to_string(HandSize) + ".phe");
    b.run("veb", [&]() {
      ankerl::nanobench::doNotOptimizeAway(phe.eval(random_hand<HandSize>()));
    });
  }

  const auto& results = b.results();

  auto bfs_net = results[1].median(ankerl::nanobench::Result::Measure::elapsed) - results[0].median(ankerl::nanobench::Result::Measure::elapsed);
  std::cout << "net bfs: " << std::setprecision(3) << (bfs_net * 1e9) << " ns/op\n";

  auto dfs_net = results[2].median(ankerl::nanobench::Result::Measure::elapsed) - results[0].median(ankerl::nanobench::Result::Measure::elapsed);
  std::cout << "net dfs: " << std::setprecision(3) << (dfs_net * 1e9) << " ns/op\n";

  auto veb_net = results[3].median(ankerl::nanobench::Result::Measure::elapsed) - results[0].median(ankerl::nanobench::Result::Measure::elapsed);
  std::cout << "net veb: " << std::setprecision(3) << (veb_net * 1e9) << " ns/op\n";
}

template <size_t HandSize>
void bench_throughput() {
  std::cout << "\n\nBenchmarking " << HandSize << "-card hand sweep throughput...\n";

  ankerl::nanobench::Bench b;
  b
      .unit("hand")
      .warmup(10)
      .epochIterations(1)
      .batch(choose(52, HandSize))
      .performanceCounters(true);

  {
    PokerHandEval<HandSize> phe("tables/bfs" + std::to_string(HandSize) + ".phe");
    b.run("bfs", [&]() {
      phe.sweep([](auto, auto score) { ankerl::nanobench::doNotOptimizeAway(score); });
    });
  }

  {
    PokerHandEval<HandSize> phe("tables/dfs" + std::to_string(HandSize) + ".phe");
    b.run("dfs", [&]() {
      phe.sweep([](auto, auto score) { ankerl::nanobench::doNotOptimizeAway(score); });
    });
  }

  {
    PokerHandEval<HandSize> phe("tables/veb" + std::to_string(HandSize) + ".phe");
    b.run("veb", [&]() {
      phe.sweep([](auto, auto score) { ankerl::nanobench::doNotOptimizeAway(score); });
    });
  }
}

int main() {
  bench_latency<5>();
  bench_latency<7>();
  bench_throughput<5>();
  bench_throughput<7>();
}
