#ifndef WORDLE_HACKER_GUESSER_H
#define WORDLE_HACKER_GUESSER_H

#endif //WORDLE_HACKER_GUESSER_H

#include <optional>
#include <map>
#include <functional>
#include <cassert>
#include <compare>

using String = std::string;

enum class Status : uint8_t {
    invalid,
    green,
    yellow,
    grey
};

class CompareResult {
private:
    uint32_t bin = 0;

public:
    auto constexpr operator<=>(const CompareResult& right) const = default;
    auto constexpr get_status(int index) const {
        return (Status)((bin >> (8 - index * 2) ) & 0b11);
    }
    auto constexpr set_status(Status status, int index) {
        bin |= (uint32_t)status << (8 - index * 2);
    }
    auto constexpr get_blob() const {
        return bin;
    }
};

struct Node {
    size_t count;
    std::vector<String> words;
    std::optional<String> splitter;
    std::map<CompareResult, std::unique_ptr<Node>> children;
};

class Guesser {
private:
    std::vector<String> words;
    std::vector<String> candidates;
    std::unique_ptr<Node> node;
    Node *cur = nullptr;

public:
    std::function<void(const Node &, size_t, size_t)> build_progress_callback;
    bool finished() const;
    void init(const std::vector<String>& words);
    void build();
    void reset();
    void visit(std::function<void(const Node&)> visitor);
    void filter(const String& word, const CompareResult& result);
    void guess(const String& word, const CompareResult& result);
    std::pair<String, int> current() const;
    std::vector<std::pair<String, CompareResult>> current_list() const;
};