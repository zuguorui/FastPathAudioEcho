//
// Created by zu on 2020/7/27.
//

#ifndef FASTPATHAUDIOECHO_SLESPLAYER_H
#define FASTPATHAUDIOECHO_SLESPLAYER_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "SLESEngine.h"

#include <stdint.h>

using namespace std;

class ISLESPlayerCallback{
public:
    virtual int32_t writeData(void *audioData, int32_t numFrames) = 0;
};

class SLESPlayer {
public:
    SLESPlayer();
    ~SLESPlayer();

    bool init(SLESEngine &engine, ISLESPlayerCallback *dataCallback, int32_t sampleRate, int32_t channelCount, int32_t framesPerBuffer = 256);
    void destroy();

    void start();
    void stop();

    int32_t writeData(int16_t *audioData, int32_t numFrames);

private:
    static void playerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
    void processOutput(SLAndroidSimpleBufferQueueItf bq);

    SLAndroidSimpleBufferQueueItf playerBufferQueue = nullptr;

    SLObjectItf outputMixObject = nullptr;

    SLObjectItf playerObject = nullptr;
    SLPlayItf playerPlay = nullptr;

    int32_t framesPerBuffer = 0;
    int32_t sampleRate = 0;
    int32_t channelCount = 0;

    ISLESPlayerCallback *dataCallback = nullptr;

    int16_t *audioBuffer = nullptr;



};


#endif //FASTPATHAUDIOECHO_SLESPLAYER_H
