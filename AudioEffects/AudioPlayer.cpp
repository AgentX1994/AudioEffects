//
//  AudioPlayer.cpp
//  AudioEffects
//
//  Created by John Asper on 2016/8/11.
//  Copyright © 2016 John Asper. All rights reserved.
//

#include "AudioPlayer.hpp"
#include <functional> // for std::bind

AudioPlayer::AudioPlayer(int n_buffers, float time_callbacks){
    num_buffers = n_buffers;
    time_between_callbacks = time_callbacks;
}

AudioPlayer::~AudioPlayer(){
    AudioQueueDispose(q, true);
}

void AudioPlayer::setEffects(AudioEffect *ae){
    effects = ae;
}

void AudioPlayer::play(std::string path){
    WavFile w(path);
    play(w);
}

void AudioPlayer::play(WavFile &wav){
    AudioStreamBasicDescription asbd;
    
    AudioQueueRef queue;
    
    cur_sample = 0;
    num_samples = wav.getNumSamples();
    num_channels = wav.getNumChannels();

    buffer = effects->apply(wav.getData(), num_samples, num_channels, wav.getSampleRate());
    
    // Set up the Audio Stream Basic Description using the WavFile
    asbd.mSampleRate = wav.getSampleRate();
    asbd.mFormatID = kAudioFormatLinearPCM;
    asbd.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    asbd.mFramesPerPacket = 1;
    asbd.mChannelsPerFrame = wav.getNumChannels();
    asbd.mBytesPerPacket = asbd.mBytesPerFrame = bytes_per_packet = sizeof(float)*wav.getNumChannels();
    asbd.mBitsPerChannel = sizeof(float)*8;
    
    AudioQueueNewOutput(&asbd,
                        AQcallback,
                        this, CFRunLoopGetCurrent(),
                        kCFRunLoopCommonModes, 0, &queue);
    
    int buffer_size;
    AudioQueueBufferRef buf_refs[num_buffers];
    AudioQueueBuffer *bufs[num_buffers];
    
    // Determine best size for buffers and packets
    calculateBufferSize(asbd, bytes_per_packet, time_between_callbacks, &buffer_size, &packets_per_read);
    
    // Create and init buffers
    for (int i = 0; i < num_buffers; ++i) {
        AudioQueueAllocateBuffer(queue, buffer_size, &(buf_refs[i]));
        bufs[i] = buf_refs[i];
        bufs[i]->mAudioDataByteSize = buffer_size;
        AQcallback(this, queue, buf_refs[i]);
    }
    
    // Set desired volume
    AudioQueueSetParameter (queue, kAudioQueueParam_Volume, 1.0f);
    
    // Start playback
    AudioQueueStart (queue, NULL);
    
    // Run the callback in a loop
    while (cur_sample <= num_samples){
        CFRunLoopRunInMode (
                            kCFRunLoopDefaultMode,
                            time_between_callbacks, // seconds
                            false // don't return after source handled
                            );
    }
    
    // Make sure that all audio is finished playing
    CFRunLoopRunInMode ( kCFRunLoopDefaultMode,
                        time_between_callbacks,
                        false);
    
    // Dispose of the audio queue
    AudioQueueDispose(queue, true);
}

void AudioPlayer::callback(AudioQueueRef q, AudioQueueBufferRef br){
    AudioQueueBuffer *buf = br;
    float *samp = (float *)buf->mAudioData;
    
    if(cur_sample > num_samples){
        AudioQueueStop(q, false);
        return;
    }
    
    int sample = 0;
    while(sample < packets_per_read * num_channels){
        for (int channel = 0; channel < num_channels; ++channel) {
            if (cur_sample > num_samples) {
                samp[sample] = 0;
            } else {
                samp[sample] = buffer[channel][cur_sample];
            }
            ++sample;
        }
        ++cur_sample;
    }
    AudioQueueEnqueueBuffer (q, br, 0, NULL);
}

// Figures out the proper buffer size for the a specific length of audio
void AudioPlayer::calculateBufferSize(AudioStreamBasicDescription &asbd,
                                      int max_packet_size,
                                      float time_to_play,
                                      int *out_buffer_size,
                                      int *out_num_packets_to_read){
    static const int maxBufferSize = 0x50000;
    static const int minBufferSize = 0x4000;
    
    if (asbd.mFramesPerPacket != 0) {
        float numPacketsForTime =
        asbd.mSampleRate / asbd.mFramesPerPacket * time_to_play;
        *out_buffer_size = numPacketsForTime * max_packet_size;
    } else {
        *out_buffer_size =
        maxBufferSize > max_packet_size ?
        maxBufferSize : max_packet_size;
    }
    
    if (
        *out_buffer_size > maxBufferSize &&
        *out_buffer_size > max_packet_size
        )
        *out_buffer_size = maxBufferSize;
    else {
        if (*out_buffer_size < minBufferSize)
            *out_buffer_size = minBufferSize;
    }
    
    *out_num_packets_to_read = *out_buffer_size / max_packet_size;
}