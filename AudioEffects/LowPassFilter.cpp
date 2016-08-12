//
//  LowPassFilter.cpp
//  AudioEffects
//
//  Created by John Asper on 2016/8/11.
//  Copyright © 2016 John Asper. All rights reserved.
//

#include <iostream>
#include "LowPassFilter.hpp"

LowPassFilter::LowPassFilter(){
    b = 0.95f;
    next = NULL;
}

LowPassFilter::~LowPassFilter(){
    // Nothing to dealloc
}

float ** LowPassFilter::apply(float **in_buffer, int num_samples, int num_channels, int sample_rate){
    
    for(int channel = 0; channel < num_channels; ++channel){
        for(int sample = 1; sample < num_samples; ++sample){
            in_buffer[channel][sample] = (1-b)*in_buffer[channel][sample] + b*in_buffer[channel][sample-1];
        }
    }
    
    return (next ? next->apply(in_buffer, num_samples, num_channels, sample_rate) : in_buffer);
}