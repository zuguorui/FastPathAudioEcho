//
// Created by zu on 2020/7/25.
//

#ifndef FASTPATHAUDIOECHO_AAUDIOPLAYER_H
#define FASTPATHAUDIOECHO_AAUDIOPLAYER_H

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include <android/log.h>
#include <aaudio/AAudio.h>

#include "Constants.h"

using namespace std;

class IAAudioPlayerCallback
{
public:
    virtual int32_t writeData(AAudioStream *stream, void *audioData, int32_t numFrames) = 0;
};


class AAudioPlayer {
public:
    AAudioPlayer();
    ~AAudioPlayer();

    static enum PERFORMANCE_MODE{
        NONE = AAUDIO_PERFORMANCE_MODE_NONE,
        POWER_SAVING = AAUDIO_PERFORMANCE_MODE_POWER_SAVING,
        LOW_LATENCY = AAUDIO_PERFORMANCE_MODE_LOW_LATENCY
    };

    bool init(IAAudioPlayerCallback *dataCallback, int32_t sampleRate, int32_t channelCount, PERFORMANCE_MODE mode = NONE, int32_t framesPerBuffer = 256);
    void destroy();
    void start();
    void stop();

    int32_t writeData(int16_t *audioData, int32_t numFrames);



private:
    AAudioStream *outputStream = nullptr;

    static aaudio_data_callback_result_t output_callback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames);

    int16_t *emptyBuffer = nullptr;

    IAAudioPlayerCallback *dataCallback = nullptr;
    int32_t sampleRate;
    int32_t channelCount;
    int32_t framesPerBuffer;

};


#endif //FASTPATHAUDIOECHO_AAUDIOPLAYER_H
