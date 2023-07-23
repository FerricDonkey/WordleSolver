#include<vector>
#include<array>
#include<string>
#include<stdint.h>

#include <iostream>

#include"common.hpp"

std::string word_vec_to_string(const WordArray & word_arr) {
    std::string word_string;
    for (auto letter_int : word_arr) {
        word_string.push_back(letter_int + 'a');
    }
    return word_string;
}
