# Fast Poker Hand Eval

A high-performance C++ implementation of a poker hand evaluator, based on a variant of the famous 2+2 (Ray Wotton) algorithm. This version supports both 5 and 7 card hands and features various memory layout optimizations to maximize cache efficiency.

Required files:
* `poker_hand_eval.h`, from the code
* `bfs5.phe` or `bfs7.phe` from the releases page

Example usage:
```c++
#include "poker_hand_eval.h"
...
PokerHandEval<7> phe("/path/to/table7.phe");
uint32_t score = phe.eval(37, 0, 48, 26, 7, 5, 8);
...
std::vector<uint32_t> hand = ...
uint32_t score = phe.eval(hand);
```

Cards are defined as rank-major integers (0-51):
```
   0 -> 2c
   1 -> 2d
   2 -> 2h
   3 -> 2s
   4 -> 3c
  ..
  48 -> Ac
  49 -> Ad
  50 -> Ah
  51 -> As
```
Lower scores represent better hands.

If you want to change the card mapping, or change the score representations, you'll need to regenerate the `*.phe` file.

# Why is it fast?

The evaluator uses a precomputed finite state machine (FSM) stored in a flat array. Evaluating a hand is simply a series of array lookups, which the compiler can optimize into a tight chain of `add` and `mov` instructions.

Here's the assembly for evaluating 5 and 7 card hands (thanks Godbolt!):
```asm
eval5(PokerHandEval<(unsigned char)5>&, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int):
  mov eax, r9d
  mov rdi, qword ptr [rdi]
  add r8d, dword ptr [rdi + 4*rax]
  add ecx, dword ptr [rdi + 4*r8]
  add edx, dword ptr [rdi + 4*rcx]
  add esi, dword ptr [rdi + 4*rdx]
  mov eax, dword ptr [rdi + 4*rsi]
  ret

eval7(PokerHandEval<(unsigned char)7>&, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int):
  mov eax, dword ptr [rsp + 16]
  mov rdi, qword ptr [rdi]
  mov eax, dword ptr [rdi + 4*rax]
  add eax, dword ptr [rsp + 8]
  add r9d, dword ptr [rdi + 4*rax]
  add r8d, dword ptr [rdi + 4*r9]
  add ecx, dword ptr [rdi + 4*r8]
  add edx, dword ptr [rdi + 4*rcx]
  add esi, dword ptr [rdi + 4*rdx]
  mov eax, dword ptr [rdi + 4*rsi]
  ret
```

Beyond minimal instruction count, this implementation is optimized for **cache efficiency**. By controlling the memory layout of the FSM (Breadth-First, Depth-First, or Van Emde Boas), we can significantly reduce cache misses during common operations like Monte Carlo simulations or full-deck sweeps.

# What on Earth does that asm do?

The `*.phe` file is a finite state machine laid out as an array. Each state has 52 slots in the array. The value in those slots is the next state index, unless the state is a terminal. Terminal states' slots contain scores.

The evaluation code is effectively:
```c++
auto index = table[hand[0]];
index = table[index + hand[1]];
index = table[index + hand[2]];
...
index = table[index + hand[n]];
auto value = index;
```

# How is the finite state machine generated?

The FSM is built by iterating through all possible hands of size $N-1$ down to 0. At each step, we identify "equivalence classes" of hands. Two hands are considered equivalent if they react identically to every future card (i.e., they have compatible "out-edges").

This implementation uses a **greedy finite state minimization** with **"don't-care" transitions**. Unlike traditional FSM minimization, which accounts for every possible transition and usually routes invalid inputs to a "failure" state, this approach treats repeated cards (which are impossible in a valid hand) as **"don't-care" transitions**.

By allowing nodes to merge even if their transitions for already-seen cards differ (since those transitions will never be taken during a valid evaluation), we can collapse the state space much more aggressively. This results in a significantly smaller table size that still provides 100% correct results for all legal poker hands. The minimization is "greedy" in that it uses a hinted search to find and merge compatible states efficiently.

# How is the finite state machine flattened?

Once the FSM states are identified, they must be assigned a location in the final lookup array. The order of these states determines the memory access pattern during evaluation.

### BFS vs DFS vs VeB

We support three different memory layouts:
*   **BFS (Breadth-First Search):** States are laid out in the order they are visited in a level-order traversal. This is exceptionally fast for **sweeps** (iterating through all possible hands) because the access pattern is highly sequential.
*   **DFS (Depth-First Search):** States are laid out following a depth-first traversal. This can provide better locality for evaluations that share common prefixes.
*   **VeB (Van Emde Boas):** A recursive partitioning layout designed to be cache-oblivious. It aims to provide good locality regardless of the cache size.

Benchmarks show that **BFS** is generally the winner for throughput, while all three perform similarly for random latency, fitting well within modern L3 caches.

# How to change card mapping or scores

The card mapping and evaluation logic are decoupled from the FSM generator. To change them:
1.  Modify `generate_tables/generate_tables.cc` to use a different `IdMap` or `EvalFn`.
2.  Run `make generate_tables` to build the generator.
3.  Execute the generator to produce new `*.phe` files.

The provided generator uses `cactus_kev` and `senzee` evaluators as a bootstrap to populate the FSM with correct poker hand rankings.

### Profile-Guided Optimization (PGO)

The included `Makefile` supports PGO to further squeeze out performance. To run benchmarks with PGO:
```bash
make pgo-bench
```
This will compile the benchmarks with profiling enabled, run them to collect data, and then recompile using that data to optimize branches and inlining based on actual execution patterns.

# Benchmarks

Benchmarks were performed using [nanobench](https://github.com/martinus/nanobench). Latency measures random hand evaluation, while throughput measures a full sweep of all possible hands.

### 5-card hand evaluation latency
| relative |             ns/hand |              hand/s |    err% | benchmark
|---------:|--------------------:|--------------------:|--------:|:----------
|   100.0% |                7.97 |      125,503,532.59 |    0.3% | `control` (random gen)
|    90.4% |                8.82 |      113,420,133.72 |    0.1% | `bfs`
|    88.7% |                8.98 |      111,306,923.36 |    0.2% | `dfs`
|    89.1% |                8.94 |      111,820,417.58 |    0.3% | `veb`

### 7-card hand evaluation latency
| relative |             ns/hand |              hand/s |    err% | benchmark
|---------:|--------------------:|--------------------:|--------:|:----------
|   100.0% |               14.63 |       68,357,682.81 |    0.2% | `control` (random gen)
|    41.4% |               35.34 |       28,295,144.96 |    1.4% | `bfs`
|    38.8% |               37.72 |       26,514,337.58 |    0.8% | `dfs`
|    39.2% |               37.33 |       26,785,317.50 |    1.8% | `veb`

### 5-card hand sweep throughput
|             ns/hand |              hand/s |    err% | benchmark
|--------------------:|--------------------:|--------:|:----------
|                0.64 |    1,555,042,717.96 |    1.1% | `bfs`
|                0.70 |    1,421,559,243.12 |    4.6% | `dfs`
|                0.70 |    1,432,212,435.81 |    2.4% | `veb`

### 7-card hand sweep throughput
|             ns/hand |              hand/s |    err% | benchmark
|--------------------:|--------------------:|--------:|:----------
|                0.71 |    1,406,053,168.92 |    1.1% | `bfs`
|                0.94 |    1,066,767,931.48 |    0.7% | `dfs`
|                0.75 |    1,328,699,193.50 |    1.2% | `veb`
