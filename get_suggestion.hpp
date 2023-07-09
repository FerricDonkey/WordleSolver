#ifndef get_suggestion_hpp
#define get_suggestion_hpp

#include <stdint.h>
#include <vector>
#include <string>
#include <algorithm>

#include "word_restriction.hpp"
#include "common.hpp"

void print_suggestions(
    const std::vector<std::vector<uint32_t>>& possible_guesses,
    const std::vector<std::vector<uint32_t>>& possible_answers,
    const WordRestriction& restriction
);

inline std::vector<int> calculate_response(
    const std::vector<uint32_t>& guess,
    const std::vector<uint32_t>& answer
);

#endif