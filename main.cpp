#include <algorithm>
#include <iostream>
#include <omp.h>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <unordered_set>

#include "word_restriction.hpp"
#include "get_suggestion.hpp"
#include "common.hpp"

const std::string WORDS_FILENAME = "words_lenN_466551.txt";

static inline WordArray string_to_word_arr(const std::string& word) {
    WordArray word_vec;
    uletter_int index = 0;
    for (auto c : word) {
        word_vec[index] = c - 'a';
        index++;
    }
    return word_vec;
}

std::vector<std::string> get_words_from_file(const std::string& filename) {
    std::fstream fin(filename, std::ios::in);

    if (!fin.is_open()) {
        std::cout << "Could not open file " << filename << std::endl;
        throw std::runtime_error(std::string("Could not open file ") + filename);
    }

    std::vector<std::string> words;
    std::unordered_set<std::string> seen_words;
    std::string line;
    while (std::getline(fin, line)) {
        if (line.size() < WORD_LENGTH) continue;

        // must be ascii
        if (std::any_of(line.begin(), line.end(), [](unsigned char c) {return c > 127; })) {
            continue;
        }

        std::size_t first = 0;
        std::size_t last = line.size() - 1;
        for (; first < line.size(); first++) {
            if (!std::isspace(line[first])) break;
        }
        for (; last > 0; last--) {
            if (!std::isspace(line[last])) break;
        }

        // Wrong length
        if (
            last < first
            || last - first + 1 != WORD_LENGTH
        ) {
            continue;
        }

        // Trim whitespace
        line = line.substr(first, last - first + 1);

        // Lowercaseify
        std::transform(
            line.begin(),
            line.end(),
            line.begin(),
            [](unsigned char c) { return std::tolower(c); }
        );

        // Must be letters
        if (std::any_of(
                line.begin(),
                line.end(),
                [](unsigned char c) {return c > 'z' || c < 'a'; }
        )){
            continue;
        }

        if (seen_words.find(line) != seen_words.end()) {
            continue;
        }

        // Finally, it's a word.
        words.push_back(line);
        seen_words.insert(line);
    }
    return words;
}

std::vector<WordArray> convert_words(const std::vector<std::string>& words_as_strings) {
    std::vector<WordArray> converted_words(
        words_as_strings.size()
    );

    std::size_t word_index = 0;
    for (const std::string& word : words_as_strings) {
        for (std::size_t letter_index = 0; letter_index < WORD_LENGTH; letter_index++) {
            converted_words[word_index][letter_index] = word[letter_index] - 'a';
        }
        word_index++;
    }

    return converted_words;
}

ResponseArray get_response_from_user() {
    std::string user_input;
    while (true) {
        std::cout << "Enter response (2: green, 1: yellow, 0: gray): " << std::flush;
        std::getline(std::cin, user_input);
        if (
            user_input.size() == WORD_LENGTH
            && std::all_of(
                user_input.begin(),
                user_input.end(),
                [](char c) {return c >= '0' && c <= '2'; }
            )
        ) {
            break;
        }
        std::cout << "BAD INPUT." << std::endl;
    }

    ResponseArray response;
    uletter_int index = 0;
    for (auto c : user_input) {
        response[index] = c - '0';
        index++;
    }
    return response;
}

WordArray get_word_from_user() {
    std::string user_input;
    while (true) {
        std::cout << "Enter word (all lowercase, length "<< WORD_LENGTH << "): " << std::flush;
        std::getline(std::cin, user_input);
        if (
            user_input.size() == WORD_LENGTH
            && std::all_of(
                user_input.begin(),
                user_input.end(),
                [](char c) {return c >= 'a' && c <= 'z'; }
            )
            ) {
            break;
        }
        std::cout << "BAD INPUT." << std::endl;
    }

    return string_to_word_arr(user_input);
}

int get_user_action() {
    std::string user_input;
    while (true) {
        std::cout
            << "Select action:\n"
            << "  1 - Enter a wordle word/response\n"
            << "  2 - Print surviving words\n"
            << "  3 - Get suggested word from all guesses\n"
            << "  4 - Get suggested word from remaining answers\n"
            << "  5 - Quit" << std::endl;

        std::getline(std::cin, user_input);
        if (
            user_input.size() == 1
            && user_input[0] >= '1'
            && user_input[0] <= '5'
        ) {
            return user_input[0] - '0';
        }
        std::cout << "BAD INPUT." << std::endl;
    }
}


int test(
    std::vector<WordArray>& possible_answers,
    std::vector<WordArray>& possible_guesses,
    WordRestriction& restriction
) {
    auto answer = string_to_word_arr("piano");
    auto guess1 = string_to_word_arr("tares");
    auto guess2 = string_to_word_arr("deter");

    auto eliminated = string_to_word_arr("below");
    if (!restriction.is_word_allowed(answer)) {
        std::cerr << "BUG: Restriction is flunking words before being updated" << std::endl;
        return 1;
    }

    for (const auto& guess : {guess1, guess2}) {
        auto response = calculate_response(guess, answer);
        std::cout << word_vec_to_string(guess) << " ";
        for (auto r : response) std::cout << (int) r << " ";
        std::cout << std::endl;

        restriction.update_from_word_guess(
            guess,
            response
        );
        restriction.print();
        possible_answers = restriction.get_surviving_words(possible_answers);
        if (std::find(possible_answers.begin(), possible_answers.end(), eliminated) != possible_answers.end()) {
            std::cerr << "CRAPPPPP" << std::endl;
            std::cerr << restriction.is_word_allowed(eliminated) << std::endl;
            return 1;
        }
        std::vector<WordArray> new_possible_guesses;
        new_possible_guesses.reserve(possible_guesses.size());
        for (auto& guess : possible_guesses) {
            if (restriction.can_provide_new_information(guess))
                new_possible_guesses.push_back(std::move(guess));
        }
        possible_guesses = std::move(new_possible_guesses);
        std::cout << std::endl;
    }

    print_suggestions(
        possible_guesses,
        possible_answers,
        restriction
    );
    return 0;
}


class CommandLineParser {
public:
    bool recieved_help_arg = false;
    bool do_test = false;
    bool do_big_search = false;
    const std::string* words_file_p = nullptr;

    CommandLineParser() = default;
    CommandLineParser(int argc, char** argv) {
        for (int arg_i = 1; arg_i < argc; arg_i++) {
            if (std::string("--help") == argv[arg_i]) {
                recieved_help_arg = true;
                return;
            } else if (std::string("--test") == argv[arg_i]) {
                do_test = true;
            } else if (std::string("--search") == argv[arg_i]){
                do_big_search = true;
            } else {
                if (words_file_p != nullptr) {
                    throw std::invalid_argument("File supplied twice");
                }
                words_file_p = new std::string(argv[arg_i]);
            }
        }

        if (do_big_search && do_test) {
            throw std::invalid_argument("Cannot use --test with --search.");
        }

        if (words_file_p == nullptr) {
            words_file_p = &WORDS_FILENAME;
        }
    }
    ~CommandLineParser() {
        if (words_file_p != nullptr and words_file_p != &WORDS_FILENAME)
            delete words_file_p;
    }

    void print_help(const std::string& prog_name) {
        std::cout << "Usage: " << prog_name << "[word_list] [--test] [--help]\n"
            << "    word_list  - Filename of wordlist to use (one per line).\n"
            << "                 Default: pwd/" << WORDS_FILENAME << "\n"
            << "    --search   - Run a non-interactive search for the best starting word.\n"
            << "    --test     - Run a basic non-interactive test.\n"
            << "    --help     - Print this message and exit."
            << std::endl;
    }
};


int main(int argc, char** argv) {
    CommandLineParser args;
    try {
        args = CommandLineParser(argc, argv);
    } catch (const std::exception& exc) {
        std::cerr << exc.what() << std::endl;
        return 1;
    }

    if (args.recieved_help_arg) {
        args.print_help(argv[0]);
        return 0;
    }

    std::vector<WordArray> possible_guesses = convert_words(
        get_words_from_file(
            *args.words_file_p
        )
    );
    std::vector<WordArray> possible_answers(possible_guesses);
    WordRestriction restriction;

    if (args.do_test) {
        return test(possible_answers, possible_guesses, restriction);
    } else if (args.do_big_search) {
        print_suggestions(
            possible_guesses,
            possible_answers,
            restriction
        );
        return 0;
    }

    while (true) {
        std::cout << "\nRemaining Solutions: " << possible_answers.size() << "\n" << std::endl;
        int user_action = get_user_action();
        switch (user_action) {
            case 1: {// enter new
                WordArray word = get_word_from_user();
                ResponseArray response = get_response_from_user();
                restriction.update_from_word_guess(word, response);
                possible_answers = restriction.get_surviving_words(possible_answers);

                // Along the lines of std::remove_if, except that requires nonsense
                // and almost as much code
                std::vector<WordArray> new_possible_guesses;
                new_possible_guesses.reserve(possible_guesses.size());
                for (auto& guess : possible_guesses) {
                    if (restriction.can_provide_new_information(guess))
                        new_possible_guesses.push_back(std::move(guess));
                }
                possible_guesses = std::move(new_possible_guesses);

                restriction.print();
                break;
            }
            case 2: // print
                std::cout << "\nRemaining Answers:\n";
                for (const auto& word : possible_answers) {
                    std::cout << "  - " << word_vec_to_string(word) << "\n";
                }
                std::cout << std::endl;
                break;
            case 3: // get suggestion
                print_suggestions(
                    possible_guesses,
                    possible_answers,
                    restriction
                );
                break;
            case 4:
                print_suggestions(
                    possible_answers,
                    possible_answers,
                    restriction
                );
                break;
            case 5:
                return 0;
                break;
        }
    }
    return 0;
}