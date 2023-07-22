#include<vector>
#include<array>
#include<string>
#include<stdint.h>

#include"common.hpp"

std::string word_vec_to_string(const WordArray & word_vec) {
    std::string word_string;
    for (auto letter_int : word_vec) {
        word_string.push_back(letter_int + 'a');
    }
    return word_string;
}
