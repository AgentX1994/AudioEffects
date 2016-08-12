//
//  AudioPlayer.hpp
//  AudioEffects
//
//  Created by John Asper on 2016/8/11.
//  Copyright © 2016 John Asper. All rights reserved.
//

#ifndef AudioPlayer_hpp
#define AudioPlayer_hpp

#include <stdio.h>
#include <AudioToolbox/AudioToolbox.h>
#include "AudioEffect.hpp"
#include "WavFile.hpp"

class AudioPlayer {
public:
    
    AudioPlayer(int num_buffers = 3, float time_between_callbacks = 0.25f);
    
    ~AudioPlayer();
    
    void setEffects(AudioEffect *ae);
    
    void play(std::string path);
    void play(WavFile &wav);
    
protected:
private:
    int num_buffers;
    float time_between_callbacks;
    AudioQueueRef q;
    
    // Internal playing data
    float ** buffer;
    int cur_sample;
    int num_samples;
    int num_channels;
    int packets_per_read;
    int bytes_per_packet;
    
    AudioEffect *effects;
    
    void calculateBufferSize(AudioStreamBasicDescription &asbd,
                             int max_packet_size,
                             float time_to_play,
                             int *outBufferSize,
                             int *outNumPacketsToRead);
    
    // Callback for
    void callback(AudioQueueRef q, AudioQueueBufferRef br);
    
    static void AQcallback(void *ptr, AudioQueueRef queue, AudioQueueBufferRef buf_ref){
        AudioPlayer *ap = (AudioPlayer *)ptr;
        ap->callback(queue, buf_ref);
    }
};


#endif /* AudioPlayer_hpp */
