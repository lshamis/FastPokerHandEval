#include <queue>
#include <stack>

namespace poker_eval {

template <typename Collection, typename T>
bool has_key(const Collection& c, const T& k) {
  return c.find(k) != c.end();
}

template <uint8_t hand_size>
std::vector<EncodedHand> dfs_memory_order(const FSM& fsm) {
  std::unordered_map<EncodedHand, uint8_t> depth = {{-1, 0}};
  std::vector<EncodedHand> order;

  std::stack<std::pair<HandOrScore /* parent */, HandOrScore /* child */>> dfs;
  dfs.push({-1, 0});

  while (!dfs.empty()) {
    auto pc = dfs.top();
    auto parent = pc.first;
    auto child = pc.second;
    dfs.pop();

    if (!has_key(fsm, child) || has_key(depth, child) || depth[parent] >= hand_size) {
      continue;
    }
    order.push_back(child);
    depth[child] = depth[parent] + 1;

    for (Card card = 0; card < 52; card++) {
      dfs.push({child, fsm.at(child)[card]});
    }
  }

  return order;
}

template <uint8_t>
std::vector<EncodedHand> bfs_memory_order(const FSM& fsm) {
  std::unordered_set<EncodedHand> seen_hands;
  std::vector<EncodedHand> order;

  std::queue<HandOrScore> bfs;
  bfs.push(0);

  while (!bfs.empty()) {
    HandOrScore hand = bfs.front();
    bfs.pop();

    if (!has_key(fsm, hand) || has_key(seen_hands, hand)) {
      continue;
    }
    order.push_back(hand);
    seen_hands.insert(hand);

    for (Card card = 0; card < 52; card++) {
      bfs.push(fsm.at(hand)[card]);
    }
  }

  return order;
}

namespace {

template <uint8_t hand_size>
std::pair<std::vector<EncodedHand>, std::vector<EncodedHand>> veb_memory_order_helper(
    const FSM& fsm,
    HandOrScore root,
    std::unordered_set<EncodedHand>& seen_hands) {
  if (has_key(seen_hands, root)) {
    return {{}, {}};
  }

  auto order_next = veb_memory_order_helper<hand_size / 2>(fsm, root, seen_hands);

  auto order = order_next.first;
  std::vector<EncodedHand> next;

  for (auto lower_root : order_next.second) {
    auto lower_order_next = veb_memory_order_helper<hand_size - (hand_size / 2)>(fsm, lower_root, seen_hands);
    auto& lower_order = lower_order_next.first;
    auto& lower_next = lower_order_next.second;
    order.insert(order.end(), lower_order.begin(), lower_order.end());
    next.insert(next.end(), lower_next.begin(), lower_next.end());
  }

  return {order, next};
}

template <>
std::pair<std::vector<EncodedHand>, std::vector<EncodedHand>> veb_memory_order_helper<1>(
    const FSM& fsm,
    HandOrScore root,
    std::unordered_set<EncodedHand>& seen_hands) {
  if (has_key(seen_hands, root)) {
    return {{}, {}};
  }

  seen_hands.insert(root);
  std::vector<EncodedHand> next;
  for (Card card = 0; card < 52; card++) {
    next.push_back(fsm.at(root)[card]);
  }
  return {{root}, next};
}

}  // namespace

template <uint8_t hand_size>
std::vector<EncodedHand> veb_memory_order(const FSM& fsm) {
  std::unordered_set<EncodedHand> seen_hands;
  return veb_memory_order_helper<hand_size>(fsm, 0, seen_hands).first;
}

template <uint8_t max_hand_size>
std::vector<uint32_t> flatten_fsm(const FSM& fsm,
                                  const std::vector<EncodedHand>& order) {
  assert(fsm.size() == order.size());
  assert(order[0] == 0);

  std::unordered_map<uint32_t, EncodedHand> idx_to_hand;
  std::unordered_map<EncodedHand, uint32_t> hand_to_idx;
  uint32_t next_idx = 0;
  for (EncodedHand hand : order) {
    idx_to_hand[next_idx] = hand;
    hand_to_idx[hand] = next_idx;
    next_idx += 52;
  }

  std::vector<uint32_t> memory(next_idx);

  for (auto&& pair : hand_to_idx) {
    EncodedHand hand = pair.first;
    uint32_t idx = pair.second;

    if (Hand::decode(hand).size + 1u == max_hand_size) {
      for (Card card = 0; card < 52; card++) {
        Score score = fsm.at(hand)[card];
        memory[idx + card] = score;
      }
    } else {
      for (Card card = 0; card < 52; card++) {
        EncodedHand next_hand = fsm.at(hand)[card];
        memory[idx + card] = hand_to_idx[next_hand];
      }
    }
  }

  return memory;
}

}  // namespace poker_eval
