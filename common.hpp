#ifndef common_hpp
#define common_hpp

#include<array>
#include<stdint.h>

typedef uint8_t uletter_int;
typedef int8_t  letter_int;

constexpr uletter_int WORD_LENGTH = 5;
constexpr uletter_int ALPHABET_LENGTH = 26;

typedef std::array<uletter_int, WORD_LENGTH> WordArray;
typedef std::array<uletter_int, ALPHABET_LENGTH> AlphabetArray;
typedef std::array<letter_int, WORD_LENGTH> ResponseArray;

constexpr ResponseArray EMPTY_RESPONSE = {};
constexpr AlphabetArray EMPTY_ALPHABET_ARRAY = {};

// This is stupid. There has to be a better way that would not have to be
// modified if ALPHABET_LENGTH changed
constexpr AlphabetArray MAX_VAL_ALPHABET_ARRAY = {  // Ugh
    ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH,
    ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH,
    ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH,
    ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH,
    ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH, ALPHABET_LENGTH,
    ALPHABET_LENGTH,
};

constexpr AlphabetArray WORD_LEN_ALPHABET_ARRAY = {  // Ugh
    WORD_LENGTH, WORD_LENGTH, WORD_LENGTH, WORD_LENGTH, WORD_LENGTH,
    WORD_LENGTH, WORD_LENGTH, WORD_LENGTH, WORD_LENGTH, WORD_LENGTH,
    WORD_LENGTH, WORD_LENGTH, WORD_LENGTH, WORD_LENGTH, WORD_LENGTH,
    WORD_LENGTH, WORD_LENGTH, WORD_LENGTH, WORD_LENGTH, WORD_LENGTH,
    WORD_LENGTH, WORD_LENGTH, WORD_LENGTH, WORD_LENGTH, WORD_LENGTH,
    WORD_LENGTH
};

std::string word_vec_to_string(const WordArray& word_arr);

#endif