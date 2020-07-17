//
// Created by incus on 2020-06-24.
//

#ifndef INCUSAUDIOPROCESS_OBOEECHORECORDCALLBACK_H
#define INCUSAUDIOPROCESS_OBOEECHORECORDCALLBACK_H

#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include "IAudioFrameProvider.h"
#include "BlockRecyclerQueue.h"
#include "oboe/Oboe.h"
#include "Constants.h"

using namespace std;
using namespace oboe;

class OboeEchoRecordCallback: public oboe::AudioStreamCallback {
public:
    OboeEchoRecordCallback(BlockRecyclerQueue<AudioFrame *> *bufferQueue);
    ~OboeEchoRecordCallback();

    DataCallbackResult onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) override;

private:
    AudioFrame *recordingFrame = NULL;
    BlockRecyclerQueue<AudioFrame *> *bufferQueue = NULL;
};


#endif //INCUSAUDIOPROCESS_OBOEECHORECORDCALLBACK_H
