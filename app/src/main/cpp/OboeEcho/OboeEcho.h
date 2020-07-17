//
// Created by incus on 2020-06-24.
//

#ifndef INCUSAUDIOPROCESS_OBOEECHO_H
#define INCUSAUDIOPROCESS_OBOEECHO_H

#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include "IAudioFrameProvider.h"
#include "BlockRecyclerQueue.h"
#include "oboe/Oboe.h"
#include "Constants.h"
#include "OboeEchoPlayCallback.h"
#include "OboeEchoRecordCallback.h"

using namespace std;
using namespace oboe;

class OboeEcho {
public:
    OboeEcho();
    ~OboeEcho();

    bool init(int32_t micID);
    void destroy();
    void start();
    void stop();

    void updateAlgorithmParams(int32_t algorithm_index, const uint8_t *left_para, const uint8_t *right_para);
    void algorithmStatusSet(int32_t algorithm_index, int32_t status, int32_t channel);

private:
    oboe::AudioStream *recordStream = NULL;
    oboe::AudioStream *playStream = NULL;

    OboeEchoRecordCallback *recordCallback = NULL;
    OboeEchoPlayCallback *playCallback = NULL;

    BlockRecyclerQueue<AudioFrame *> *bufferQueue = NULL;

};


#endif //INCUSAUDIOPROCESS_OBOEECHO_H
