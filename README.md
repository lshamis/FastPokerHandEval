# Fast Poker Hand Eval

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

Cards are defined as:
```
   0 -> 2c
   1 -> 3c
   2 -> 4c
  ..
  11 -> Kc
  12 -> Ac
  13 -> 2d
  ..
  51 -> As
```
Lower scores represent better hands.

If you want to change the card mapping, or change the score representations, you'll need to regenerate the `*.phe` file.

# Why is it fast?

Here's the assembly for evaluating 5 and 7 card hands (thanks Godbolt!):
```asm
eval5(PokerHandEval<(unsigned char)5>&, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int): # @eval5(PokerHandEval<(unsigned char)5>&, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int)
  mov eax, r9d
  mov rdi, qword ptr [rdi]
  add r8d, dword ptr [rdi + 4*rax]
  add ecx, dword ptr [rdi + 4*r8]
  add edx, dword ptr [rdi + 4*rcx]
  add esi, dword ptr [rdi + 4*rdx]
  mov eax, dword ptr [rdi + 4*rsi]
  ret

eval7(PokerHandEval<(unsigned char)7>&, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int): # @eval7(PokerHandEval<(unsigned char)7>&, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int)
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

Assembly length is not always a good proxy for runtime performance, but this is also optimized for cache efficiency. More in the "flattening" discussion below, and in benchmarks.

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


# How to change card mapping or scores
## TODO


# How is the finite state machine generated
## TODO


# How is the finite state machine flattened
## TODO

### BFS vs DFS vs VeB ...

# Benchmarks

### 5-card hand evaluation latency...
| relative |             ns/hand |              hand/s |    err% |     total | benchmark
|---------:|--------------------:|--------------------:|--------:|----------:|:----------
|   100.0% |                7.97 |      125,503,532.59 |    0.3% |      0.97 | `control`
|    90.4% |                8.82 |      113,420,133.72 |    0.1% |      0.97 | `bfs`
|    88.7% |                8.98 |      111,306,923.36 |    0.2% |      0.99 | `dfs`
|    89.1% |                8.94 |      111,820,417.58 |    0.3% |      0.98 | `veb`

* net bfs: 0.849 ns/op
* net dfs: 1.02 ns/op
* net veb: 0.975 ns/op


### 7-card hand evaluation latency...
| relative |             ns/hand |              hand/s |    err% |     total | benchmark
|---------:|--------------------:|--------------------:|--------:|----------:|:----------
|   100.0% |               14.63 |       68,357,682.81 |    0.2% |      1.78 | `control`
|    41.4% |               35.34 |       28,295,144.96 |    1.4% |      4.32 | `bfs`
|    38.8% |               37.72 |       26,514,337.58 |    0.8% |      4.71 | `dfs`
|    39.2% |               37.33 |       26,785,317.50 |    1.8% |      4.71 | `veb`

* net bfs: 20.7 ns/op
* net dfs: 23.1 ns/op
* net veb: 22.7 ns/op


### 5-card hand sweep throughput...
|             ns/hand |              hand/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|                0.64 |    1,555,042,717.96 |    1.1% |      0.02 | `bfs`
|                0.70 |    1,421,559,243.12 |    4.6% |      0.02 | `dfs`
|                0.70 |    1,432,212,435.81 |    2.4% |      0.02 | `veb`


### 7-card hand sweep throughput...
|             ns/hand |              hand/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|                0.71 |    1,406,053,168.92 |    1.1% |      1.05 | `bfs`
|                0.94 |    1,066,767,931.48 |    0.7% |      1.39 | `dfs`
|                0.75 |    1,328,699,193.50 |    1.2% |      1.11 | `veb`
