#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "poker_hand_eval.h"

namespace poker_eval {
namespace {

template <uint8_t hand_size>
bool validate_fsm(const FSM& fsm, EvalFn eval_fn) {
  bool all_good = true;

  for_each_hand(hand_size, [&](const Hand& hand) {
    HandOrScore expected = eval_fn(hand);

    HandOrScore actual = 0;
    for (uint8_t i = 0; i < hand_size; i++) {
      actual = fsm.at(actual)[hand.cards[i]];
    }

    if (expected != actual) {
      printf("Mismatch for %s!\n  expected=%lu\n  actual=%lu\n",
             hand.debug_string().c_str(), expected, actual);
      all_good = false;
    }
  });

  return all_good;
}

template <uint8_t hand_size>
bool validate_phe(const PokerHandEval<hand_size>& phe, EvalFn eval_fn) {
  bool all_good = true;

  for_each_hand(hand_size, [&](const Hand& hand) {
    Score expected = eval_fn(hand);
    Score actual = phe.eval(hand.cards);

    if (expected != actual) {
      printf("Mismatch for %s!\n  expected=%u\n  actual=%u\n",
             hand.debug_string().c_str(), expected, actual);
      all_good = false;
    }
  });

  return all_good;
}

template <typename T>
std::string human_readable_duration(const T& duration) {
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

  auto duration_h = std::chrono::duration_cast<std::chrono::hours>(duration_ms);
  auto duration_m = std::chrono::duration_cast<std::chrono::minutes>(duration_ms -= duration_h);
  auto duration_s = std::chrono::duration_cast<std::chrono::seconds>(duration_ms -= duration_m);
  duration_ms -= duration_s;

  auto h = duration_h.count();
  auto m = duration_m.count();
  auto s = duration_s.count();
  auto ms = duration_ms.count();

  std::stringstream ss;
  if (h >= 1) {
    ss << h << " hr ";
  }
  if (m >= 1) {
    ss << m << " min ";
  }
  if (s >= 1) {
    ss << s << " sec ";
  }
  if (ms >= 1) {
    ss << ms << " ms ";
  }

  return ss.str();
}

std::string human_readable_filesize(size_t num_bytes) {
  static const char* units[] = {"B", "kiB", "MiB", "GiB", "TiB", "PiB", "EiB"};

  if (num_bytes < 1024) {
    return static_cast<std::ostringstream&>(std::ostringstream{} << num_bytes << " B").str();
  }

  double value = num_bytes;
  size_t unit_idx = 0;
  while (value >= 1024) {
    value /= 1024;
    unit_idx++;
  }

  return static_cast<std::ostringstream&>(std::ostringstream{} << std::setprecision(2) << value << " " << units[unit_idx]).str();
}

void save_lookup_table(const std::vector<uint32_t>& lookup_table,
                       const std::string& path) {
  std::ofstream file(path, std::ios::out | std::ios::binary);
  file.write(reinterpret_cast<const char*>(&lookup_table[0]),
             lookup_table.size() * sizeof(uint32_t));
  file.close();
}

template <uint8_t hand_size>
void save_phes(
    const FSM& fsm,
    const std::map<std::string, MemoryLayoutFn<hand_size>>& layout_files,
    EvalFn eval_fn) {
  for (const auto& pair : layout_files) {
    const auto& path = pair.first;
    const auto& layout_fn = pair.second;

    printf("\nProcessing memory layout for %s...\n", path.c_str());

    printf("  Ordering memory...");
    auto table = flatten_fsm<hand_size>(fsm, layout_fn(fsm));
    printf("  Done.\n");

    printf("  Saving table...");
    save_lookup_table(table, path);
    printf("  Done.\n");

    printf("  Validating optimized evaluator...");
    if (validate_phe<hand_size>(PokerHandEval<hand_size>(path), eval_fn)) {
      printf("  Done.\n");
    } else {
      printf("  Failed.\n");
      std::remove(path.c_str());
    }
  }
}

}  // namespace

template <uint8_t hand_size>
void build_phes(
    EvalFn eval_fn,
    const std::map<std::string, MemoryLayoutFn<hand_size>>& layout_files) {
  printf("\nBuilding FSM for hands of size %d...\n", hand_size);
  auto start_time = std::chrono::system_clock::now();
  auto fsm = build_fsm<hand_size>(eval_fn);
  auto end_time = std::chrono::system_clock::now();
  printf("Done.\n");

  auto duration_str = human_readable_duration(end_time - start_time);
  printf("\nTook: %s\n", duration_str.c_str());

  printf("\nNum states: %lu.\n", fsm.size());
  size_t num_bytes = 52 * fsm.size() * sizeof(uint32_t);
  auto filesize_str = human_readable_filesize(num_bytes);
  printf("Table size: %zu bytes (%s).\n", num_bytes, filesize_str.c_str());

  printf("\nValidating FSM... ");
  if (!validate_fsm<hand_size>(fsm, eval_fn)) {
    printf("Failed!\n");
    return;
  }
  printf("Done.\n");

  save_phes<hand_size>(fsm, layout_files, eval_fn);
}

}  // namespace poker_eval
