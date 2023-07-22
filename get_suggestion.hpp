#ifndef get_suggestion_hpp
#define get_suggestion_hpp

#include <stdint.h>
#include <vector>
#include <string>
#include <algorithm>

#include "word_restriction.hpp"
#include "common.hpp"

void print_suggestions(
    const std::vector<WordArray>& possible_guesses,
    const std::vector<WordArray>& possible_answers,
    const WordRestriction& restriction
);

inline std::array<int, WORD_LENGTH> calculate_response(
    const WordArray& guess,
    const WordArray& answer
);

#endif