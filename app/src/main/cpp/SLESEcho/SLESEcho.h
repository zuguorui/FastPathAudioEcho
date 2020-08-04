//
// Created by incus on 2020-06-18.
//

#ifndef INCUSAUDIOPROCESS_AUDIOECHO2_H
#define INCUSAUDIOPROCESS_AUDIOECHO2_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "Constants.h"
#include "SLESPlayer.h"
#include "SLESRecorder.h"
#include "BlockRingBuffer.h"


using namespace std;

class SLESEcho: public ISLESPlayerCallback, public ISLESRecorderCallback{
public:
    SLESEcho();
    ~SLESEcho();

    bool init(int32_t sampleRate, int32_t framesPerBuffer = 256, int32_t micID = -1);
    void destroy();
    void start();
    void stop();

    int32_t writeData(void *audioData, int32_t numFrames) override;

    void readData(void *audioData, int32_t numFrames) override;

private:

    int32_t framesPerBuffer = 0;
    int32_t sampleRate = 0;
    int32_t channelCount = 1;

    BlockRingBuffer<int16_t> buffer{2048};

    SLESEngine engine;
    SLESPlayer player;
    SLESRecorder recorder;
};


#endif //INCUSAUDIOPROCESS_AUDIOECHO2_H
