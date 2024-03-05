#include "guesser.h"
#include <chrono>
#include <execution>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define STR(x) #x
const char* file_location = R"(limited_words.txt)";

#define fucker asdfdsa:/asdfc

#define strify_(x) # x
#define strify(x) strify_(x)

using std::cout;
using std::cin;
using std::endl;
using std::vector;

std::vector<std::string> load(const char* file) {
	std::ifstream fin(file);
	vector<std::string> words;
	std::string line;
	while (std::getline(fin, line)) {
		std::stringstream ss(line);
		std::string word;
		ss >> word;
		auto is_word = std::all_of(word.begin(), word.end(), [](char ch) {
			return 'a' <= ch && ch <= 'z';
		});
		if (!is_word) {
			continue;
		}
		words.emplace_back(word);
	}
	cout << words.size() << " words loaded" << endl;
	return words;
}

String wrap(char ch, Status status) {
    fmt::text_style style;
    switch (status)
    {
        case Status::green:
            style = fmt::bg(fmt::color::green);
            break;
        case Status::yellow:
            style = fmt::bg(fmt::color::yellow);
            break;
        case Status::grey:
            style = fmt::bg(fmt::color::gray);
            break;
        default:
            throw std::logic_error("wtf");
    }
    style = style | fmt::fg(fmt::color::black);
    return fmt::format(style, FMT_STRING("{}"), ch);
}

class Printer {
public:
	Printer(int count, std::ostream& out) : count(count), out(out) {}

	void print_with_color(const Node& node) {
		print_with_color("$", node, "", 0, -1);
	}

	void print_with_color(const Node& node, int max_level) {
		print_with_color("$", node, "", 0, max_level);
	}

	void print_with_color(const std::vector<std::tuple<String, CompareResult, size_t, size_t>>& candidates) {
		if (candidates.empty()) {
			out << "no candidates found" << endl;
			return;
		}
		for (auto& [word, result, _, __] : candidates) {
			String colored_output;
			for (auto i = 0; i < 5; i++) {
				auto status = result.get_status(i);
				colored_output += wrap(word[i], status);
			}
			count++;
			out << fmt::format(FMT_STRING("{:3}: {}"), count, colored_output) << endl;
		}
	}

	void print_with_color_verbose(const std::vector<std::tuple<String, CompareResult, size_t, size_t>>& candidates) {
		if (candidates.empty()) {
			out << "no candidates found" << endl;
			return;
		}
		for (auto& [word, result, possibilities, depth] : candidates) {
			String colored_output;
			for (auto i = 0; i < 5; i++) {
				auto status = result.get_status(i);
				colored_output += wrap(word[i], status);
			}
			count++;
			out << fmt::format(FMT_STRING("{:3}: {} (cnt: {}, dep: {})"), count, colored_output, possibilities, depth) << endl;
		}
	}

private:
	int count = 0;
	std::ostream& out;

	void print_with_color(const String& prefix, const Node& node, const String& last_word, int level, int max_level) {
		assert(!node.words.empty());
		if (max_level >= 0 && level > max_level) {
            ++count;
            out << count << ": " << prefix << endl;
			return;
        }
		if (node.words.size() == 1) {
			auto word = node.words[0];
			String tail;
			if (last_word != word) {
				tail = "->";
				tail += fmt::format(fmt::bg(fmt::color::green) | fmt::fg(fmt::color::black), FMT_STRING("{}"), word);
			}
			++count;
			out << count << ": " << prefix << tail << endl;
			return;
		}
		assert(node.splitter);
		auto split_word = node.splitter.value();
		assert(split_word.size() == 5);
		for (auto& [k, v] : node.children) {
			String colored_output;
			assert(v);
			for (auto i = 0; i < 5; ++i) {
				auto status = k.get_status(i);
				colored_output += wrap(split_word[i], status);
			}
			print_with_color(fmt::format(FMT_STRING("{}->{}"), prefix, colored_output), *v, split_word, level + 1, max_level);
		}
		return;
	}
};

String get_string() {
	char ch;
	String input;
	while (true) {
		ch = (char)getchar();
		if (!std::isprint(ch)) {
			break;
		}
		input += ch;
	}
	return input;
}

int main(int argc, char** argv) {
    if (argc > 1) file_location = argv[1];
	cout << fmt::format(FMT_STRING("file location: {}"), file_location) << endl;
	cout << fmt::format(fmt::fg(fmt::color::yellow), FMT_STRING("loading words..."), file_location) << endl;
	auto words = load(file_location);
	Guesser guesser;
	auto start = std::chrono::high_resolution_clock::now();
	auto start_callback = [&]() {
		cout << fmt::format(fmt::fg(fmt::color::yellow), "constructing...") << endl;
		start = std::chrono::high_resolution_clock::now();
	};
	auto end_callback = [&](const Node &node) {
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		cout << "\r" << fmt::format(FMT_STRING("constructed in {:.3f} s"), duration.count() / 1000.0) << endl;
	};
    auto progress_callback = [&](const Node &node, size_t progress, size_t total) {
		auto one_percent = total / 100;
		if (one_percent <= 0) one_percent = 1;
        if (progress % one_percent == 0) {
            cout << "\r" << progress * 100 / total << "%";
        }
    };
	guesser.build_start_callback = start_callback;
	guesser.build_end_callback = end_callback;
	guesser.build_progress_callback = progress_callback;
    guesser.init(words);

	auto candidates = guesser.current_list();
	auto print_candidates = [&]() {
		auto [word, possibilities, depth] = guesser.current();
		if (guesser.finished()) {
			word = fmt::format(fmt::bg(fmt::color::green) | fmt::fg(fmt::color::black), FMT_STRING("{}"), word);
			cout << "you win! the word is " << word << ". type 'restart' to restart" << endl;
		} else {
			cout << "possibilities: " << possibilities << ", max depth: " << depth - 1 << endl;
			Printer(0, cout).print_with_color(candidates);
		}
	};
	print_candidates();

    while (true) {
        cout << "command> ";
        String command = get_string();
        if (command == "exit") {
			break;
		}
		if (command == "help") {
			cout << "type a number to select a word" << endl;
			cout << "type a word to guess" << endl;
			cout << "type 'exit' to exit" << endl;
			cout << "type 'reload' to reload words from file" << endl;
			cout << "type 'restart' to restart" << endl;
			cout << "type 'choices' to print all candidate choices" << endl;
			cout << "type 'choicesv' to print all candidate choices with verbose information" << endl;
			cout << "type 'routes' to print all possible routes" << endl;
			continue;
		}
		if (command == "reload") {
			words = load(file_location);
			guesser.init(words);
			continue;
		}
		if (command == "choices") {
            print_candidates();
            continue;
		}
		if (command == "choicesv") {
			Printer(0, cout).print_with_color_verbose(candidates);
			continue;
		}
		if (command == "routes") {
			auto printer = Printer(0, cout);
			auto visitor = [&](const Node& node) { printer.print_with_color(node); };
			guesser.visit(visitor);
			continue;
		}
		if (command == "restart") {
			guesser.reset();
            candidates = guesser.current_list();
            print_candidates();
            continue;
		}
		// select words
		try {
			int num = std::stoi(command);
			if (num > 0 && num <= candidates.size()) {
				auto& [word, result, _, __] = candidates[num - 1];
				guesser.guess(word, result);
                candidates = guesser.current_list();
                print_candidates();
                continue;
			}
		} catch (std::invalid_argument& e) {
			// do nothing
		}
		// guess word
		auto it = std::find(words.begin(), words.end(), command);
		if (it != words.end()) {
			cout << "input color status (g for green, y for yellow, x for grey)" << endl;
			cout << "color> ";
			String input = get_string();
			CompareResult color;
			int index = 0;
			for (auto ch : input) {
				switch (ch) {
				case 'g':
					color.set_status(Status::green, index++);
					break;
				case 'y':
					color.set_status(Status::yellow, index++);
					break;
				case 'x':
					color.set_status(Status::grey, index++);
					break;
				default:
					break;
				}
				if (index >= 5) break;
			}
			if (index < 5) {
				cout << fmt::format(fmt::fg(fmt::color::red), FMT_STRING("invalid input")) << endl;
				continue;
			}
			guesser.guess(command, color);
            candidates = guesser.current_list();
            print_candidates();
            continue;
		}
		// print help
		cout << "unknown command, type 'help' for help" << endl;
	}
	return 0;
}
