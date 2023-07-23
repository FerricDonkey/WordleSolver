#ifndef get_suggestion_hpp
#define get_suggestion_hpp

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

inline ResponseArray calculate_response(
    const WordArray& guess,
    const WordArray& answer
);

#endif