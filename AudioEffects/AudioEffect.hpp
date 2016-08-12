//
//  AudioEffect.hpp
//  AudioEffects
//
//  Created by John Asper on 2016/8/11.
//  Copyright © 2016 John Asper. All rights reserved.
//

#ifndef AudioEffect_hpp
#define AudioEffect_hpp

#include <stdio.h>

/* AudioEffect Interface (Abstract class)
 *
 * Defines an interface that represents an audio effect in a
 * linked list of audio effects
 *
 * Allows for a modular effect chain
 */

class AudioEffect {
public:
    
    // Applies this audio effect to the sample in in_buffer, then
    // calls apply on the next effect in the chain (if it exists);
    //
    // in_buffer must have num_channels sub_buffers, each with room for num_samples floats
    //
    // returns the result once it goes through all the audio effects
    virtual float **apply(float **in_buffer, int num_samples, int num_channels, int sample_rate) = 0;
    
protected:
    AudioEffect *next; // pointer to the next effect in the list
private:
};

#endif /* AudioEffect_hpp */
