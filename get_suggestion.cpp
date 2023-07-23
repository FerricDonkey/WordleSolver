
#include <iostream>
#include <stdint.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <iomanip>

#include <atomic>

#include <algorithm>
#include <numeric>
#include <omp.h>

#include "get_suggestion.hpp"
#include "word_restriction.hpp"
#include "common.hpp"

static constexpr float EPSILON = 0.00001;

inline ResponseArray calculate_response(
    const WordArray& guess,
    const WordArray& answer
){
    ResponseArray response = EMPTY_RESPONSE;
    AlphabetArray letter_count = EMPTY_ALPHABET_ARRAY;
    AlphabetArray letter_to_green_count = EMPTY_ALPHABET_ARRAY;

    for (uletter_int index = 0; index < WORD_LENGTH; index++) {
        if (answer[index] == guess[index]) {
            response[index] = 2;
            letter_to_green_count[answer[index]] += 1;
        }
        letter_count[answer[index]]++;
    }

    for (uletter_int index = 0; index < WORD_LENGTH; index++) {
        if (
            guess[index] != answer[index]
            && letter_count[guess[index]] > letter_to_green_count[guess[index]]
        ) {
            response[index] = 1;
            letter_count[guess[index]]--;
        }
    }

    return response;
}

void get_remaining_answers(
    const WordArray& guess,
    const std::vector<WordArray>& possible_answers,
    const WordRestriction& base_restriction,
    std::vector<uint32_t>& num_answers_dest_vec,
    float& mean,
    float& median,
    float& stddev
) {
    std::size_t answer_index = 0;
    uint32_t running_total = 0;
    for (const auto& answer : possible_answers) {
        WordRestriction restriction = base_restriction;
        ResponseArray response = calculate_response(guess, answer);
        restriction.update_from_word_guess(guess, response);

        num_answers_dest_vec[answer_index] = 0;
        for (const auto& possible_answer : possible_answers) {
            num_answers_dest_vec[answer_index] += restriction.is_word_allowed(possible_answer);
        }

        running_total += num_answers_dest_vec[answer_index];
        answer_index++;
    }

    std::sort(num_answers_dest_vec.begin(), num_answers_dest_vec.end());

    mean = (float) running_total / possible_answers.size();
    if (possible_answers.size() % 2) {
        median = num_answers_dest_vec[(possible_answers.size() / 2)];
    } else {
        median = (
            (float) (
                num_answers_dest_vec[(possible_answers.size() / 2) - 1]
                + num_answers_dest_vec[(possible_answers.size() / 2)]
            ) / 2
        );
    }

    stddev = 0;
    for (auto num_answers : num_answers_dest_vec) {
        stddev += (num_answers - mean) * (num_answers - mean);
    }
    stddev = sqrt(stddev / possible_answers.size());
}

static inline bool float_is_less_than(float a, float b) {
    if (
        abs(a - b) >= EPSILON
        && a < b
    ) {
        return true;
    }
    return false;
}

static inline bool guess_comparitor(
    float mean1,
    float median1,
    float mean2,
    float median2,
    const WordArray& guess1,
    const WordArray& guess2,
    const WordRestriction& restriction
) {
    bool guess1_possible = restriction.is_word_allowed(guess1);
    bool guess2_possible = restriction.is_word_allowed(guess2);
    if (float_is_less_than(median1, median2)) {
        return true;
    } else if (abs(median1 - median2) < EPSILON && float_is_less_than(mean1, mean2)) {
        return true;
    } else if (
        abs(median1 - median2) < EPSILON
        && abs(mean1 - mean2) < EPSILON
        && (guess1_possible && !guess2_possible)
    ) {
        return true;
    } else if (
        abs(median1 - median2) < EPSILON
        && abs(mean1 - mean2) < EPSILON
        && (guess1_possible == guess2_possible)
    ) {
        return word_vec_to_string(guess1) < word_vec_to_string(guess2);
    }
    return false;
}


void print_suggestions(
    const std::vector<WordArray>& possible_guesses,
    const std::vector<WordArray>& possible_answers,
    const WordRestriction& restriction
) {
    std::vector<std::vector<uint32_t>> guess_index_to_answer_index_to_num_remaining(
        possible_guesses.size(),
        std::vector<uint32_t>(possible_answers.size())
    );

    std::vector<float> means(possible_guesses.size());
    std::vector<float> medians(possible_guesses.size());
    std::vector<float> stddevs(possible_guesses.size());

    std::atomic_uint32_t num_done = 0;
    std::atomic_uint32_t print_lockish = 0;

    std::cout << "\nChecked 0 of " << possible_guesses.size() << "    " << std::flush;

    // guess_index is signed to make omp happy
    #pragma omp parallel for schedule(dynamic)
    for (int64_t guess_index = 0; guess_index < possible_guesses.size(); guess_index++) {
        get_remaining_answers(
            possible_guesses[guess_index],
            possible_answers,
            restriction,
            guess_index_to_answer_index_to_num_remaining[guess_index],
            means[guess_index],
            medians[guess_index],
            stddevs[guess_index]
        );
        num_done++;
        if (
            !(num_done & 0xf) &&
            !(print_lockish++)
        ) {
            std::cout << "\rChecked " << num_done << " of " << possible_guesses.size()
                << "    " << std::flush;
            print_lockish = 0;
        }
    }

    std::cout << "\rChecked " << possible_guesses.size() << " of " << possible_guesses.size()
        << "    " << std::endl;

    std::vector<std::size_t> sorted_guess_indexes(possible_guesses.size());
    std::iota(sorted_guess_indexes.begin(), sorted_guess_indexes.end(), 0);

    std::sort(
        sorted_guess_indexes.begin(),
        sorted_guess_indexes.end(),
        [&](std::size_t guess_index1, std::size_t guess_index2) {
            return guess_comparitor(
                means[guess_index1],
                medians[guess_index1],
                means[guess_index2],
                medians[guess_index2],
                possible_guesses[guess_index1],
                possible_guesses[guess_index2],
                restriction
            );
        }
    );

    std::size_t num_printed = 0;
    std::cout
        << "SUGGESTED ANSWERS (sorted by decreasing ~remaining answers):\n"
        << "   Word | Median  | Mean    | StdDev  |\n"
        << "  -------------------------------------\n";
    for (auto guess_index : sorted_guess_indexes) {
        std::cout << "  " << word_vec_to_string(possible_guesses[guess_index]) << " | "
            << std::fixed << std::setprecision(2)
            << std::setw(7) << medians[guess_index] << " | "
            << std::setw(7) << means[guess_index]   << " | "
            << std::setw(7) << stddevs[guess_index] << " |"
            << (
                restriction.is_word_allowed(possible_guesses[guess_index])
                ? " (possible answer)" : ""
            )
            << std::endl;
        num_printed++;
        if (num_printed > 35) break;
    }
}
