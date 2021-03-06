//
//  main.cpp
//  AudioEffects
//
//  Created by John Asper on 2016/8/11.
//  Copyright © 2016 John Asper. All rights reserved.
//

#include <iostream>
#include "WavFile.hpp"
#include "AudioPlayer.hpp"
#include "LowPassFilter.hpp"

int main(int argc, const char * argv[]) {
    AudioPlayer player;
    LowPassFilter lp;
    player.setEffects(&lp);
    
    WavFile w("/Users/john/Documents/Xcode Projects/AudioEffects/test.wav");
    player.play(w);
    w.save("/Users/john/Documents/Xcode Projects/AudioEffects/save.wav");
}
