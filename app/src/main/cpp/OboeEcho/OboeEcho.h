//
// Created by zu on 2020-06-24.
//

#ifndef _OBOE_ECHO_H_
#define _OBOE_ECHO_H_

#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include "BlockRingBuffer.h"
#include "oboe/Oboe.h"
#include "Constants.h"
#include "IEcho.h"

using namespace std;
using namespace oboe;
/**
 * 基于Oboe的echo
 * */
class OboeEcho: public IEcho, public oboe::AudioStreamCallback {
public:
    OboeEcho();
    ~OboeEcho();

    /**
     * 初始化
     * sampleRate：采样率
     * api：指定使用OpenSLES或者是AAudio。不设置时由系统自行确定。
     * framesPerBuffer：每个buffer包含多少帧。
     * micID：仅当Oboe使用AAudio进行echo时才有效。
     * */
    bool init(int32_t sampleRate, AudioApi api = AudioApi::Unspecified, int32_t framesPerBuffer = 256, int32_t micID = 0);

    bool init(int32_t sampleRate, int32_t framesPerBuffer = 256, int32_t micID = 0) override;
    void destroy() override;
    void start() override;
    void stop() override;

    DataCallbackResult
    onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) override;

private:
    int32_t sampleRate = 0;
    int32_t framesPerBuffer = 0;
    int32_t micID = -1;
    AudioApi inputApi = AudioApi::Unspecified;
    AudioApi outputApi = AudioApi::Unspecified;

    oboe::AudioStream *recordStream = nullptr;
    oboe::AudioStream *playStream = nullptr;


    BlockRingBuffer<int16_t> *buffer = nullptr;

    bool playFlag = false;


    bool justStart = false;




};


#endif
