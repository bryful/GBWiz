#pragma once
#include "blipper.h"
#include <vector>
#include <stdexcept>
#include <cstring>
class Resampler {
    blipper_t* left=nullptr;
    blipper_t* right=nullptr;
public:
    Resampler() {
        left=blipper_new(32,0.85,6.5,64,2048,nullptr);
        right=blipper_new(32,0.85,6.5,64,2048,nullptr);
        if(!left||!right) { blipper_free(left); blipper_free(right); throw std::runtime_error("Audio resampler allocation failed"); }
    }
    ~Resampler() { blipper_free(left); blipper_free(right); }
    Resampler(const Resampler&)=delete;
    Resampler& operator=(const Resampler&)=delete;
    void reset() { blipper_reset(left); blipper_reset(right); }
    std::vector<int16_t> convert(const void* data,unsigned samples) {
        std::vector<int16_t> input(samples*2); std::memcpy(input.data(),data,samples*4);
        blipper_push_samples(left,input.data(),samples,2);
        blipper_push_samples(right,input.data()+1,samples,2);
        unsigned count=blipper_read_avail(left);
        std::vector<int16_t> output(count*2);
        if(count) { blipper_read(left,output.data(),count,2); blipper_read(right,output.data()+1,count,2); }
        return output;
    }
};
