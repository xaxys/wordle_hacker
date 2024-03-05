#include <iostream>
#include <array>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <compare>
#include <map>
#include <unordered_map>
#include <optional>
#include <cassert>
#include <memory>
#include <stdexcept>
#include <random>
#include <execution>
#include "guesser.h"

using std::cout;
using std::cin;
using std::endl;
using std::vector;
using String = std::string;

namespace std {
    template <>
    struct hash<CompareResult> {
        std::size_t operator()(const CompareResult& k) const {
            return std::hash<uint32_t>()(k.get_blob());
        }
    };
}

class Counter {
private:
    uint64_t counts = 0;

public:
    Counter(const String &word) {
        for (auto ch : word) increase(ch);
    }
    uint8_t count(char ch) const {
        return (counts >> ((ch - 'a') * 2) ) & 0b11;
    }
    void set(char ch, uint8_t i) {
        assert(i >= 0 && i <= 3);
        counts &= ~( (1ULL << ((ch - 'a') * 2)) | (1ULL << ((ch - 'a') * 2 + 1)) );
        counts |= (uint64_t)i << ((ch - 'a') * 2);
    }
    void decrease(char ch) {
        assert(count(ch));
        counts -= 1ULL << ((ch - 'a') * 2);
    }
    void increase(char ch) {
        assert(count(ch) < 3);
        counts += 1ULL << ((ch - 'a') * 2);
    }
};
// class Counter {
// 	std::array<uint8_t, 26> counts;
// public:
// 	Counter() {
// 		for (auto& count : counts) count = 0;
// 	}
// 	Counter(const String& word) {
// 		for (auto& count : counts) count = 0;
// 		for (auto ch : word) {
// 			counts[ch - 'a']++;
// 		}
// 	}
// 	Counter& operator=(const Counter& other) {
// 		counts = other.counts;
// 		return *this;
// 	}
// 	uint8_t count(char ch) const {
// 		return counts.at(ch - 'a');
// 	}
// 	uint8_t& operator [](char ch) {
// 		return counts[ch - 'a'];
// 	}
// };

CompareResult match(const String& word, const String& splitter) {
    Counter counter(word);
    CompareResult result;
    for (size_t i = 0; i < word.size(); i++) {
        auto ch = word[i];
        auto ref_ch = splitter[i];
        if (ref_ch == ch) {
            counter.decrease(ref_ch);
            result.set_status(Status::green, i);
        }
    }
    for (size_t i = 0; i < word.size(); i++) {
        Status status;
        auto ch = word[i];
        auto ref_ch = splitter[i];
        if (ref_ch == ch)
            continue;
        if (counter.count(ref_ch)) {
            status = Status::yellow;
            counter.decrease(ref_ch);
        } else {
            status = Status::grey;
        }
        result.set_status(status, i);
    }
    return result;
}

int splitCount(const vector<String>& words, const String& splitter) {
    std::unordered_map<CompareResult, int> results;
    for (const auto& word : words) {
        auto key = match(word, splitter);
        results[key]++;
    }
    int max_v = 0;
    for (auto [k, v] : results) {
        max_v = std::max(max_v, v);
    }
    return max_v;
}

std::unordered_map<CompareResult, std::vector<String>> split(const vector<String>& words, const String& splitter) {
    std::unordered_map<CompareResult, std::vector<String>> results;
    for (const auto& word : words) {
        auto key = match(word, splitter);
        results[key].push_back(word);
    }
    return results;
}

std::unique_ptr<Node> construct(vector<String> words, const vector<String>& vocabulary, std::function<void(const Node&)> progress_callback = nullptr) {
    assert(words.size() > 0);

    if (words.size() == 1) {
        auto node = std::make_unique<Node>();
        node->count = 1;
        node->words = words;
        if (progress_callback != nullptr) {
            progress_callback(*node);
        }
        return node;
    }

    using PR = std::pair<int, String>;
    auto init = PR{ std::numeric_limits<int>::max(), "" };

    PR min_split = std::transform_reduce(
            std::execution::par_unseq,
            words.begin(),
            words.end(),
            init,
            [](const PR& a, const PR& b) {
                return std::min(a, b);
            },
            [&](const String& word) -> PR{
                auto local_max_size = splitCount(words, word);
                return std::make_pair(local_max_size, word);
            }
    );

    if (vocabulary.size() != words.size()) {
        auto local_min = std::transform_reduce(
                std::execution::par_unseq,
                vocabulary.begin(),
                vocabulary.end(),
                init,
                [](const PR& a, const PR& b) {
                    return std::min(a, b);
                },
                [&](const String& word) -> PR {
                    auto local_max_size = splitCount(words, word);
                    return std::make_pair(local_max_size, word);
                }
        );
        if (local_min.first < min_split.first) min_split = local_min;
    }

    auto min_split_result = split(words, min_split.second);
    auto node = std::make_unique<Node>();
    node->count = 0;
    node->words = std::move(words);
    node->splitter = min_split.second;

    for (auto& [k, vec] : min_split_result) {
        auto sub_node = construct(std::move(vec), vocabulary, progress_callback);
        node->count += sub_node->count;
        node->children[k] = std::move(sub_node);
    }

    return node;
}

bool Guesser::finished() const {
    return cur == nullptr || cur->words.size() == 1;
}

void Guesser::init(const std::vector<String> &words) {
    this->words = words;
    this->candidates = words;
    build();
}

void Guesser::build() {
    std::function<void(const Node &)> callback;
    if (build_progress_callback) {
        callback = [&]() {
            volatile size_t count = 0;
            return [&](const Node &node) mutable {
                build_progress_callback(node, count, candidates.size());
                count += 1;
            };
        }();
    }
    node = construct(candidates, words, callback);
    cur = node.get();
}

void Guesser::reset() {
    cur = node.get();
    if (candidates.size() != words.size()) {
        candidates = words;
        build();
    }
}

void Guesser::visit(std::function<void(const Node &)> visitor) {
    visitor(*cur);
}

void Guesser::filter(const String &word, const CompareResult &result) {
    assert(word.size() == 5);
    Counter counter("");
    Counter neg_counter("");
    for (size_t i = 0; i < word.size(); i++) {
        auto status = result.get_status(i);
        auto ch = word[i];
        if (status == Status::green || status == Status::yellow) {
            counter.increase(ch);
        }
        if (status == Status::grey) {
            neg_counter.set(ch, 1);
        }
    }
    auto it = std::remove_if(candidates.begin(), candidates.end(), [&](const String &candidate) {
        for (size_t i = 0; i < word.size(); i++) {
            auto ch = word[i];
            auto ref_ch = candidate[i];
            auto status = result.get_status(i);
            if (status == Status::green && ref_ch != ch) {
                return true;
            }
            if (status == Status::yellow && ref_ch == ch) {
                return true;
            }
            if (!counter.count(ref_ch) && neg_counter.count(ref_ch)) {
                return true;
            }
        }
        Counter candidate_counter(candidate);
        for (uint8_t i = 'a'; i <= 'z'; i++) {
            if (counter.count(i)) {
                if (neg_counter.count(i) && candidate_counter.count(i) != counter.count(i)) {
                    return true;
                }
                if (candidate_counter.count(i) < counter.count(i)) {
                    return true;
                }
            }
        }
        return false;
    });
    candidates.erase(it, candidates.end());
}

void Guesser::guess(const String &word, const CompareResult &result) {
    if (finished()) return;
    if (cur->splitter.value() == word) {
        for (auto& [sub_result, sub_node] : cur->children) {
            if (sub_result == result) {
                filter(word, result);
                cur = sub_node.get();
                return;
            }
        }
    }
    filter(word, result);
    build();
}

std::pair<String, int> Guesser::current() const {
    if (finished()) return {cur->words[0], 1};
    return {cur->splitter.value(), cur->count};
}

std::vector<std::pair<String, CompareResult>> Guesser::current_list() const {
    std::vector<std::pair<String, CompareResult>> result;
    if (finished()) {
        static constexpr CompareResult green = []() {
            CompareResult result;
            for (uint8_t i = 0; i < 5; i++) {
                result.set_status(Status::green, i);
            }
            return result;
        }();
        return {{cur->words[0], green}};
    }
    for (auto& [k, v] : cur->children) {
        result.push_back({cur->splitter.value(), k});
    }
    return result;
}
