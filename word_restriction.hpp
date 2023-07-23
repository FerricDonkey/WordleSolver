#ifndef word_restriction_hpp
#define word_restriction_hpp

#include <stdexcept>
#include <vector>
#include <string>
#include <stdint.h>

#include "common.hpp"

// binary number with 1 in bits 0-2WORD_LENGTH, inclusive
static const uint32_t ANY_CHAR = 0x3ffffff;

constexpr AlphabetArray CHAR_FLAGS = {
    0x1,
    0x2,
    0x4,
    0x8,
    0x10,
    0x20,
    0x40,
    0x80,
    0x100,
    0x200,
    0x400,
    0x800,
    0x1000,
    0x2000,
    0x4000,
    0x8000,
    0x10000,
    0x20000,
    0x40000,
    0x80000,
    0x100000,
    0x200000,
    0x400000,
    0x800000,
    0x1000000,
    0x2000000
};

class InvalidRestriction: public std::exception {
private:
    const char* message;

public:
    InvalidRestriction() : message("") {}
    InvalidRestriction(char* msg) : message(msg) {}
    InvalidRestriction(std::string msg): message(msg.c_str()) {}
    const char* what () {
        return message;
    }
};

class WordRestriction {
public:
    AlphabetArray min_possible = EMPTY_ALPHABET_ARRAY;
    AlphabetArray max_possible = WORD_LEN_ALPHABET_ARRAY;
    std::array<uint32_t, WORD_LENGTH> pos_to_allowed = {  // Ugh - this is gross initialization
        ANY_CHAR, ANY_CHAR, ANY_CHAR, ANY_CHAR, ANY_CHAR
    };


    void update_from_word_guess(
        const WordArray& guess,
        const ResponseArray& response
    );

    bool can_provide_new_information(const WordArray& word) const;
    bool can_letter_be_at_index(uint32_t letter, int index) const;
    uint32_t num_possible_letters_at_loc(int index) const;

    bool is_word_allowed(const WordArray& word) const;
    std::vector<int> get_surviving_word_indexes(
        const std::vector<WordArray>& words
    ) const;
    std::vector<WordArray> get_surviving_words(
        const std::vector<WordArray>& words
    ) const;

    void print() const;

private:
    void _remove_char_possibility(uint32_t to_remove, int index);
    void _set_only_char_possibility(uint32_t to_set, int index);
};

#endif
