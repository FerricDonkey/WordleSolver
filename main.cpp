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

static inline std::vector<uint32_t> string_to_word_vec(const std::string& word) {
    std::vector<uint32_t> word_vec;
    for (auto c : word) {
        word_vec.push_back(c - 'a');
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

std::vector<std::vector<uint32_t>> convert_words(const std::vector<std::string>& words_as_strings) {
    std::vector<std::vector<uint32_t>> converted_words(
        words_as_strings.size(),
        std::vector<uint32_t>(WORD_LENGTH)
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

std::vector<int> get_response_from_user() {
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

    std::vector<int> response;
    for (auto c : user_input) {
        response.push_back(c - '0');
    }
    return response;
}

std::vector<uint32_t> get_word_from_user() {
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

    return string_to_word_vec(user_input);
}

int get_user_action() {
    std::string user_input;
    while (true) {
        std::cout
            << "Select action:\n"
            << "  1 - Enter a wordle word/response\n"
            << "  2 - Print surviving words\n"
            << "  3 - Get suggested word\n"
            << "  4 - Quit" << std::endl;

        std::getline(std::cin, user_input);
        if (
            user_input.size() == 1
            && user_input[0] >= '1'
            && user_input[0] <= '4'
        ) {
            return user_input[0] - '0';
        }
        std::cout << "BAD INPUT." << std::endl;
    }
}


void test(
    std::vector<std::vector<uint32_t>>& possible_answers,
    std::vector<std::vector<uint32_t>>& possible_guesses,
    WordRestriction& restriction
) {
    auto answer = string_to_word_vec("cower");
    auto guess1 = string_to_word_vec("raise");
    auto guess2 = string_to_word_vec("deter");

    for (const auto guess : {guess1, guess2}) {
        auto response = calculate_response(guess, answer);
        std::cout << word_vec_to_string(guess) << " ";
        for (auto r : response) std::cout << r << " ";
        std::cout << std::endl;

        restriction.update_from_word_guess(
            guess,
            calculate_response(guess, answer)
        );
        restriction.print();
        possible_answers = restriction.get_surviving_words(possible_answers);
        std::vector<std::vector<uint32_t>> new_possible_guesses;
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
}

int main(int argc, char** argv) {
    const std::string* words_file_p = nullptr;
    if (
        argc > 2
        || (argc == 2 && std::string(argv[1]) == "--help")
    ) {
        std::cout << argv[0] << " takes 0 or 1 positional arguments. If 1, "
            << "path to file containing one word per line." << std::endl;
        return 0;
    } else if (argc == 2) {
        words_file_p = new std::string(argv[1]);
    } else {
        words_file_p = &WORDS_FILENAME;
    }

    std::vector<std::vector<uint32_t>> possible_guesses = convert_words(
        get_words_from_file(
            *words_file_p
        )
    );
    std::vector<std::vector<uint32_t>> possible_answers(possible_guesses);
    if (argc == 2) {
        delete words_file_p;
    }

    WordRestriction restriction;

    // test(possible_answers, possible_guesses, restriction);
    // return 0;
    //print_suggestions(
    //    possible_guesses,
    //    possible_answers,
    //    restriction
    //);
    //return 0;

    while (true) {
        std::cout << "\nRemaining Solutions: " << possible_answers.size() << "\n" << std::endl;
        int user_action = get_user_action();
        switch (user_action) {
            case 1: {// enter new
                std::vector<std::uint32_t> word = get_word_from_user();
                std::vector<int> response = get_response_from_user();
                restriction.update_from_word_guess(word, response);
                possible_answers = restriction.get_surviving_words(possible_answers);

                // Along the lines of std::remove_if, except that requires nonsense
                // and almost as much code
                std::vector<std::vector<uint32_t>> new_possible_guesses;
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
                return 0;
                break;
        }
    }
    return 0;
}