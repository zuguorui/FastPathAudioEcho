//
// Created by incus on 2020-06-24.
//

#ifndef INCUSAUDIOPROCESS_OBOEECHO_H
#define INCUSAUDIOPROCESS_OBOEECHO_H

#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include "BlockRingBuffer.h"
#include "oboe/Oboe.h"
#include "Constants.h"

using namespace std;
using namespace oboe;

class OboeEcho: public oboe::AudioStreamCallback {
public:
    OboeEcho();
    ~OboeEcho();

    bool init(int32_t sampleRate, int32_t framesPerBuffer = 256, int32_t micID = 0);
    void destroy();
    void start();
    void stop();

    DataCallbackResult
    onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) override;

private:
    int32_t sampleRate = 0;
    int32_t framesPerBuffer = 0;
    int32_t micID = -1;

    oboe::AudioStream *recordStream = nullptr;
    oboe::AudioStream *playStream = nullptr;


    BlockRingBuffer<int16_t> *buffer = nullptr;

    bool playFlag = false;




};


#endif //INCUSAUDIOPROCESS_OBOEECHO_H
