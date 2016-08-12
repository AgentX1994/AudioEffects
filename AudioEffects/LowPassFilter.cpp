//
//  LowPassFilter.cpp
//  AudioEffects
//
//  Created by John Asper on 2016/8/11.
//  Copyright © 2016 John Asper. All rights reserved.
//

#include <iostream>
#include <cmath>
#include "LowPassFilter.hpp"
#include <algorithm>
#include <iostream>

LowPassFilter::LowPassFilter(){
    min_param = 0.0f;
    max_param = 0.95f;
    auto_period = 5.0f;
    next = NULL;
}

LowPassFilter::~LowPassFilter(){
    // Nothing to dealloc
}

float LowPassFilter::tri_lfo_next(int sample_rate){
    int period_in_samples = roundf(auto_period*sample_rate);
    int half_period = period_in_samples/2;
    float param = ((max_param - min_param)/half_period) * (half_period - abs(cur_sample % (2*half_period) - half_period)) + min_param;
    ++cur_sample;
    return param;
}

float ** LowPassFilter::apply(float **in_buffer, int num_samples, int num_channels, int sample_rate){
    for(int channel = 0; channel < num_channels; ++channel){
        cur_sample = 0;
        for(int sample = 1; sample < num_samples; ++sample){
            float param = tri_lfo_next(sample_rate);
            
            // Simple in place Auto Recursive filtering algorithm
            // y_n = (1-b)*x_n + b*y_{n-1}
            in_buffer[channel][sample] = (1-param)*in_buffer[channel][sample] + param*in_buffer[channel][sample-1];
        }
    }
    
    return (next ? next->apply(in_buffer, num_samples, num_channels, sample_rate) : in_buffer);
}