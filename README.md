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

| relative |             ns/hand |              hand/s |    err% |     total | 5-cards latency
|---------:|--------------------:|--------------------:|--------:|----------:|:----------------
|   100.0% |                8.92 |      112,116,633.72 |    0.8% |      0.12 | `control5`
|    89.6% |                9.95 |      100,473,460.38 |    0.5% |      0.12 | `bfs5`
|    88.8% |               10.05 |       99,523,239.93 |    0.3% |      0.12 | `dfs5`
|    89.7% |                9.94 |      100,620,820.40 |    0.4% |      0.12 | `veb5`

| relative |             ns/hand |              hand/s |    err% |     total | 7-cards latency
|---------:|--------------------:|--------------------:|--------:|----------:|:----------------
|   100.0% |               15.58 |       64,189,431.20 |    0.6% |      0.19 | `control7`
|    41.2% |               37.82 |       26,443,838.70 |    3.4% |      0.47 | `bfs7`
|    39.5% |               39.48 |       25,328,719.95 |    1.1% |      0.49 | `dfs7`
|    39.8% |               39.16 |       25,534,294.47 |    2.5% |      0.48 | `veb7`

|             ns/hand |              hand/s |    err% |     total | 5-cards throughput
|--------------------:|--------------------:|--------:|----------:|:-------------------
|                2.30 |      434,325,657.68 |    0.0% |      0.01 | `bfs5`
|                2.30 |      435,033,835.74 |    0.0% |      0.01 | `dfs5`
|                2.32 |      431,856,441.33 |    0.0% |      0.01 | `veb5`

|             ns/hand |              hand/s |    err% |     total | 7-cards throughput
|--------------------:|--------------------:|--------:|----------:|:-------------------
|                2.02 |      496,261,024.31 |    0.0% |      0.27 | `bfs7`
|                2.28 |      437,779,919.02 |    0.0% |      0.31 | `dfs7`
|                2.06 |      484,987,451.10 |    0.0% |      0.28 | `veb7`

