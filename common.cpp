#include<vector>
#include<string>
#include<stdint.h>

#include"common.hpp"

std::string word_vec_to_string(const std::vector<uint32_t> & word_vec) {
    std::string word_string;
    for (auto letter_int : word_vec) {
        word_string.push_back(letter_int + 'a');
    }
    return word_string;
}
