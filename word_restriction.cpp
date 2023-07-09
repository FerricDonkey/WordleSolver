#include<algorithm>
#include<vector>
#include<string>
#include<unordered_map>
#include<stdint.h>
#include<stdexcept>
#include<numeric>
#include<bit>
#include<limits>
#include<iostream>

#include "common.hpp"
#include "word_restriction.hpp"

std::vector<int> WordRestriction::get_surviving_word_indexes(
    const std::vector<std::vector<uint32_t>>& words
) const {
    std::vector<int> allowed_word_indexes;

    if (words.size() > std::numeric_limits<int>::max()) {
        throw std::runtime_error("ERROR: Number of words can't be stored in an int, boo.");
    }

    int word_index = 0;
    for (const auto& word : words) {
        if (is_word_allowed(word)) {
            allowed_word_indexes.push_back(word_index);
        }
        word_index++;
    }
    return allowed_word_indexes;
}

std::vector<std::vector<uint32_t>> WordRestriction::get_surviving_words(
    const std::vector<std::vector<uint32_t>>& words
) const {
    std::vector<std::vector<uint32_t>> surviving_words;

    std::cout << "Initial Words Len: " << words.size() << std::endl;

    for (const auto& word : words) {
        if (is_word_allowed(word)) {
            surviving_words.push_back(word);
        }
    }

    return surviving_words;
}

bool WordRestriction::is_word_allowed(
    const std::vector<uint32_t>& word
) const {
    std::vector<uint32_t> letter_counts(26);
    uint32_t index = 0;
    for (uint32_t letter : word) {
        if (!can_letter_be_at_index(letter, index)) {
            return false;
        }
        letter_counts[letter]++;
        index++;
    }

    for (uint32_t letter = 0; letter < 26; letter++) {
        if (
            letter_counts[letter] > max_possible[letter]
            || letter_counts[letter] < min_possible[letter]
        ) {
            return false;
        }
    }

    return true;
}

bool WordRestriction::can_provide_new_information(
    const std::vector<uint32_t>& word
) const {
    std::unordered_map<uint32_t, uint32_t> letter_counts;
    for (int index = 0; index < WORD_LENGTH; index++) {
        if (
            can_letter_be_at_index(word[index], index)
            && num_possible_letters_at_loc(index) > 1
        ) {
            return true;
        }
        letter_counts[word[index]]++;
    }

    for (const auto& [letter, count] : letter_counts) {
        if (
            count > min_possible[letter]
            && count < max_possible[letter]
        ) {
            return true;
        }
    }
    return false;
}

void WordRestriction::print() const {
    std::cout << "Positional info:";
    for (int index = 0; index < WORD_LENGTH; index++) {
        std::cout << "\n  "<< index <<":";
        for (int letter = 0; letter < 26; letter++) {
            if (can_letter_be_at_index(letter, index)) {
                std::cout << " " << (char) (letter + 'a');
            }
        }
    }
    std::cout << "\n\nCount Info:";
    for (int letter = 0; letter < 26; letter++) {
        std::cout << "\n  " << (char) (letter + 'a') << ": "
            << min_possible[letter] << " to " << max_possible[letter];
    }
    std::cout << std::endl;
}

bool WordRestriction::can_letter_be_at_index(uint32_t letter, int index) const {
    return pos_to_allowed[index] & CHAR_FLAGS[letter];
}

uint32_t WordRestriction::num_possible_letters_at_loc(int index) const {
    return std::popcount(pos_to_allowed[index]);
}

// TOO LONG - break up into multiple functions probably.
void WordRestriction::update_from_word_guess(
    const std::vector<uint32_t>& guess,
    const std::vector<int>& response
){
    std::unordered_map<uint32_t, uint32_t> submitted_letter_counts;
    std::unordered_map<uint32_t, uint32_t> response_letter_counts;
    std::vector<uint32_t> green_counts(26);

    // Handle individual index knowledge, and count how many letters
    // were in the submitted word / are in the solution word.
    for (int index = 0; index < 5; index++) {
        uint32_t this_letter = guess[index];
        submitted_letter_counts[this_letter]++;

        switch (response[index]) {
            case 1:
                response_letter_counts[this_letter]++;
                [[fallthrough]];
            case 0:
                _remove_char_possibility(this_letter, index);
                break;

            case 2:
                response_letter_counts[this_letter]++;
                green_counts[this_letter]++;
                _set_only_char_possibility(this_letter, index);
                break;
        }
    }

    // Adjust min/max possible of each letter according to above counts.
    // AND removing gray letters from other locations, as possible
    for (const auto& [letter, submitted_count] : submitted_letter_counts) {
        // Note: response of 21000 to aabbb only tells us that there is at least 2 a's, there
        //       may be more. but response of 21000 to aaabb tells us that there's exactly 2 a's
        min_possible[letter] = std::max(min_possible[letter], response_letter_counts[letter]);

        // If we submitted more a's than the response colored green/yellow, then we know
        // that number green a's + number_yellow a's (ie response_letter_counts[letter])
        // is exactly the number of a's
        if (submitted_count > response_letter_counts[letter]) {
            max_possible[letter] = response_letter_counts[letter];
        }

        // Handle removing gray letters from other locations, if possible.
        if (
            submitted_count > response_letter_counts[letter] // there was a gray <letter>
            && response_letter_counts[letter] == green_counts[letter] // we know where any/all such <letters> are
        ){
            for (uint32_t letter_index = 0; letter_index < WORD_LENGTH; letter_index++) {
                // If response was 2, no need to change it
                if (response[letter_index] != 2) {
                    _remove_char_possibility(letter, letter_index);
                }
            }
        }
    }


    // If I know that a word as at least 2 os, then I know that it can't have more than
    // 3 of anything else. Make those adjustments
    uint32_t sum_of_mins = std::accumulate(min_possible.begin(), min_possible.end(), 0);
    if (sum_of_mins > WORD_LENGTH) {
        throw InvalidRestriction(
            std::string("ERROR: Sum of minimum counts of letters is ")
            + std::to_string(sum_of_mins)
            + " which is greater than word length of "
            + std::to_string(WORD_LENGTH)
        );
    }

    if (sum_of_mins > 0) {
        for (uint32_t letter = 0; letter < 26; letter++) {
            uint32_t max_from_loc_data = 0;
            for (uint32_t letter_index = 0; letter_index < WORD_LENGTH; letter_index++) {
                max_from_loc_data += can_letter_be_at_index(letter, letter_index);
            }
            max_possible[letter] = std::min({
                max_possible[letter],
                WORD_LENGTH - sum_of_mins + min_possible[letter],
                max_from_loc_data
            });
        }
    }

    // Glean additional information from maxes
    uint32_t num_possible_letters = 0;
    int sum_of_maxes = 0;
    std::vector<uint32_t> possible_letters;
    std::vector<uint32_t> impossible_letters;
    for (uint32_t letter = 0; letter < 26; letter++) {
        if (max_possible[letter] > 0) {
            sum_of_maxes += max_possible[letter];
            num_possible_letters++;
            possible_letters.push_back(letter);
        } else {
            impossible_letters.push_back(letter);
        }
    }
    if (sum_of_maxes < WORD_LENGTH) {
        throw InvalidRestriction(
            std::string("ERROR: Sum of maximum counts of letters is ")
            + std::to_string(sum_of_maxes)
            + " which is less than word length of "
            + std::to_string(WORD_LENGTH)
        );
    }

    // If the maxes add up exactly to word_length, then the maxes are also the mins
    if (sum_of_maxes == WORD_LENGTH){
        min_possible = max_possible;
    }

    // Anything with max 0 should be removed from all positions
    for (auto letter : impossible_letters) {
        for (int index = 0; index < WORD_LENGTH; index++) {
            _remove_char_possibility(letter, index);
        }
    }

    // Any letters that must be in exactly 1 place and are only possible
    // in one place are the only choice for that place
    for (uint32_t letter = 0; letter < 26; letter++) {
        if (min_possible[letter] == 1 && max_possible[letter] == 1) {
            int first_seen = -1;
            int last_seen = -1;
            for (int letter_index = 0; letter_index < WORD_LENGTH; letter_index++) {
                if (can_letter_be_at_index(letter, letter_index)) {
                    last_seen = letter_index;
                    if (first_seen == -1) {
                        first_seen = letter_index;
                    }
                }
            }
            if (first_seen >= 0 && first_seen == last_seen) { // only seen once
                _set_only_char_possibility(letter, first_seen);
            }
        }
    }

    // If any position cannot have any letters, that's invalid
    if (std::any_of(
        possible_letters.begin(),
        possible_letters.end(),
        [](uint32_t letter_flags) {return letter_flags == 0; }
    )) {
        std::string error_str = "ERROR: The following location(s) have no allowed letters:";
        bool found_one = false;
        for (uint32_t letter_index = 0; letter_index < WORD_LENGTH; letter_index++) {
            if (!possible_letters[letter_index]) {
                if (found_one) error_str += ", ";
                error_str += " ";
                found_one = true;
                error_str += std::to_string(letter_index);
            }
        }
        error_str += ".\n";
        throw InvalidRestriction(error_str);
    }
}

void WordRestriction::_remove_char_possibility(
    uint32_t to_remove,
    int index
) {
    pos_to_allowed[index] &= ~CHAR_FLAGS[to_remove];
}

void WordRestriction::_set_only_char_possibility(
    uint32_t to_set,
    int index
) {
    pos_to_allowed[index] = CHAR_FLAGS[to_set];
}
