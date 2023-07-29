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
    const std::vector<WordArray>& words
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

std::vector<WordArray> WordRestriction::get_surviving_words(
    const std::vector<WordArray>& words
) const {
    std::vector<WordArray> surviving_words;

    std::cout << "Initial Words Len: " << words.size() << std::endl;

    for (const auto& word : words) {
        if (is_word_allowed(word)) {
            surviving_words.push_back(word);
        }
    }

    return surviving_words;
}


bool WordRestriction::is_word_allowed(
    const WordArray& word
) const {
    static AlphabetArray letter_counts;
    #pragma omp threadprivate(letter_counts)
    letter_counts.fill(0);

    uint32_t index = 0;
    for (uint32_t letter : word) {
        if (!can_letter_be_at_index(letter, index)) {
            return false;
        }
        letter_counts[letter]++;
        index++;
    }

    for (uint32_t letter = 0; letter < ALPHABET_LENGTH; letter++) {
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
    const WordArray& word
) const {
    static AlphabetArray letter_counts;
    #pragma omp threadprivate(letter_counts)
    letter_counts.fill(0);

    for (uletter_int index = 0; index < WORD_LENGTH; index++) {
        if (
            can_letter_be_at_index(word[index], index)
            && num_possible_letters_at_loc(index) > 1
        ) {
            return true;
        }
        letter_counts[word[index]]++;
    }

    uint32_t seen_letters = 0;
    for (uletter_int index = 0; index < WORD_LENGTH; index++) {
        uletter_int letter = word[index];
        if (seen_letters & CHAR_FLAGS[letter]) {
            continue;
        }
        seen_letters ^= CHAR_FLAGS[letter];
        if (
            letter_counts[letter] > min_possible[letter]
            && letter_counts[letter] < max_possible[letter]
        ) {
            return true;
        }
    }
    return false;
}

void WordRestriction::print() const {
    std::cout << "Positional info:";
    for (uletter_int index = 0; index < WORD_LENGTH; index++) {
        std::cout << "\n  "<< (int) index <<":";
        for (uletter_int letter = 0; letter < ALPHABET_LENGTH; letter++) {
            if (can_letter_be_at_index(letter, index)) {
                std::cout << " " << (char) (letter + 'a');
            }
        }
    }
    std::cout << "\n\nCount Info:";
    for (uletter_int letter = 0; letter < ALPHABET_LENGTH; letter++) {
        std::cout << "\n  " << (char) (letter + 'a') << ": "
            << (int) min_possible[letter] << " to " << (int) max_possible[letter];
    }
    std::cout << std::endl;
}

bool WordRestriction::can_letter_be_at_index(uletter_int letter, uletter_int index) const {
    return pos_to_allowed[index] & CHAR_FLAGS[letter];
}

uletter_int WordRestriction::num_possible_letters_at_loc(uletter_int index) const {
    return std::popcount(pos_to_allowed[index]);
}

// TOO LONG - break up into multiple functions probably.
void WordRestriction::update_from_word_guess(
    const WordArray& guess,
    const ResponseArray& response
){
    AlphabetArray submitted_letter_counts = EMPTY_ALPHABET_ARRAY;
    AlphabetArray response_letter_counts = EMPTY_ALPHABET_ARRAY;
    AlphabetArray green_counts = EMPTY_ALPHABET_ARRAY;

    // Handle individual index knowledge, and count how many letters
    // were in the submitted word / are in the solution word.
    for (uletter_int index = 0; index < WORD_LENGTH; index++) {
        uletter_int this_letter = guess[index];
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
    uint32_t seen_letters = 0;
    for (uletter_int letter: guess) {
        if (seen_letters & CHAR_FLAGS[letter]) {
            continue;
        }
        seen_letters ^= CHAR_FLAGS[letter];  // xor is fine, only will do once per letter
        // Note: response of 21000 to aabbb only tells us that there is at least 2 a's, there
        //       may be more. but response of 21000 to aaabb tells us that there's exactly 2 a's
        min_possible[letter] = std::max(min_possible[letter], response_letter_counts[letter]);

        // If we submitted more a's than the response colored green/yellow, then we know
        // that number green a's + number_yellow a's (ie response_letter_counts[letter])
        // is exactly the number of a's
        if (submitted_letter_counts[letter] > response_letter_counts[letter]) {
            max_possible[letter] = response_letter_counts[letter];
        }

        // Handle removing gray letters from other locations, if possible.
        if (
            submitted_letter_counts[letter] > response_letter_counts[letter] // there was a gray <letter>
            && response_letter_counts[letter] == green_counts[letter] // we know where any/all such <letters> are
        ){
            for (uletter_int letter_index = 0; letter_index < WORD_LENGTH; letter_index++) {
                // If response was 2, no need to change it
                if (response[letter_index] != 2) {
                    _remove_char_possibility(letter, letter_index);
                }
            }
        }
    }


    // If I know that a word as at least 2 os, then I know that it can't have more than
    // 3 of anything else. Make those adjustments
    uletter_int sum_of_mins = std::accumulate(min_possible.begin(), min_possible.end(), 0);
    if (sum_of_mins > WORD_LENGTH) {
        std::string error_msg = (
            std::string("ERROR: Sum of minimum counts of letters is ")
            + std::to_string(sum_of_mins)
            + " which is greater than word length of "
            + std::to_string(WORD_LENGTH)
        );
        std::cerr << error_msg << std::endl;
        throw InvalidRestriction(error_msg);
    }

    if (sum_of_mins > 0) {
        for (uletter_int letter = 0; letter < ALPHABET_LENGTH; letter++) {
            uletter_int max_from_loc_data = 0;
            for (uletter_int letter_index = 0; letter_index < WORD_LENGTH; letter_index++) {
                max_from_loc_data += can_letter_be_at_index(letter, letter_index);
            }
            max_possible[letter] = std::min({
                max_possible[letter],
                (uletter_int) (WORD_LENGTH - sum_of_mins + min_possible[letter]),
                max_from_loc_data
            });
        }
    }

    // Glean additional information from maxes
    uletter_int num_possible_letters = 0;
    uletter_int sum_of_maxes = 0;
    static std::vector<uletter_int> possible_letters;
    static std::vector<uletter_int> impossible_letters;
    #pragma omp threadprivate(possible_letters)
    #pragma omp threadprivate(impossible_letters)
    possible_letters.clear();
    impossible_letters.clear();

    for (uletter_int letter = 0; letter < ALPHABET_LENGTH; letter++) {
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
    for (uletter_int letter = 0; letter < ALPHABET_LENGTH; letter++) {
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
        pos_to_allowed.begin(),
        pos_to_allowed.end(),
        [](uint32_t letter_flags) {return letter_flags == 0; }
    )) {
        std::string error_str = "ERROR: The following location(s) have no allowed letters:";
        bool found_one = false;
        for (uletter_int letter_index = 0; letter_index < WORD_LENGTH; letter_index++) {
            if (!pos_to_allowed[letter_index]) {
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
    uletter_int to_remove,
    uletter_int index
) {
    pos_to_allowed[index] &= ~CHAR_FLAGS[to_remove];
}

void WordRestriction::_set_only_char_possibility(
    uletter_int to_set,
    uletter_int index
) {
    pos_to_allowed[index] = CHAR_FLAGS[to_set];
}
