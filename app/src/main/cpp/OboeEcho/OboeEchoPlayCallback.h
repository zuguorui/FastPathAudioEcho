//
// Created by incus on 2020-06-24.
//

#ifndef INCUSAUDIOPROCESS_OBOEECHOPLAYCALLBACK_H
#define INCUSAUDIOPROCESS_OBOEECHOPLAYCALLBACK_H


#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include "IAudioFrameProvider.h"
#include "BlockRecyclerQueue.h"
#include "oboe/Oboe.h"
#include "Constants.h"

#if defined(_INCUS_FRAMEWORK_VER_0)
extern "C"
{
#include "algorithm_control.h"
#include "audio_process.h"
}
#elif defined(_INCUS_FRAMEWORK_VER_1)
extern "C"
{
#include "incus_sdk.h"
}
#endif

using namespace std;
using namespace oboe;

class OboeEchoPlayCallback: public oboe::AudioStreamCallback {
public:
    OboeEchoPlayCallback(BlockRecyclerQueue<AudioFrame *> *bufferQueue);
    ~OboeEchoPlayCallback();

    bool init();
    void destroy();

    void updateAlgorithmParams(int32_t algorithm_index, const uint8_t *left_para, const uint8_t *right_para);
    void algorithmStatusSet(int32_t algorithm_index, int32_t status, int32_t channel);

    DataCallbackResult
    onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) override;

private:
    AudioFrame *playingFrame = NULL;
    BlockRecyclerQueue<AudioFrame *> *bufferQueue = NULL;
    int16_t emptyBuffer[SAMPLES_PER_FRAME];

    char *algorithm_memory_page = NULL;
#if defined(_INCUS_FRAMEWORK_VER_1)
    AlgorithmInstance algorithmInstance;
    bool algorithmOn = false;
    int16_t algorithmBuffer[2 * SAMPLES_PER_FRAME];
#endif
};


#endif //INCUSAUDIOPROCESS_OBOEECHOPLAYCALLBACK_H
