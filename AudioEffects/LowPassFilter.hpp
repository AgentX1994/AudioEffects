//
//  LowPassFilter.hpp
//  AudioEffects
//
//  Created by John Asper on 2016/8/11.
//  Copyright © 2016 John Asper. All rights reserved.
//

#ifndef LowPassFilter_hpp
#define LowPassFilter_hpp

#include <stdio.h>
#include "AudioEffect.hpp"

class LowPassFilter: public AudioEffect {
public:
    
    // Default constructor
    LowPassFilter();
    
    // Constructor
    // Creates a filter with the specified cutoff frequency
    // LowPassFilter(float co);
    
    // Destructor
    // Deallocates any allocated memory
    ~LowPassFilter();
    
    
    // Applies this audio effect to the sample in in_buffer, then
    // calls apply on the next effect in the chain (if it exists);
    //
    // in_buffer must have num_channels sub_buffers, each with room for num_samples floats
    //
    // returns the result once it goes through all the audio effects
    float **apply(float **in_buffer, int num_samples, int num_channels, int sample_rate) override;
    
protected:
private:
    float b;
};

#endif /* LowPassFilter_hpp */
