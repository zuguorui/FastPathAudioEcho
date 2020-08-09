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

/**
 * SLESPlayer获取数据的回调接口。
 * audioData：用于填充数据的buffer。
 * numFrames：需要多少帧数据，注意，一帧的意思是所有声道的一个数据。比如单声道，一帧就是一个int16。双声道则是左右各一个int16，一帧两个int16。
 * return：实际填充的帧数。
 * */
class ISLESPlayerCallback{
public:
    virtual int32_t writeData(void *audioData, int32_t numFrames) = 0;
};

/**
 * 基于OpenSLES的播放器。
 * */
class SLESPlayer {
public:
    SLESPlayer();
    ~SLESPlayer();

    /**
     * 初始化播放器。
     * engine：OpenSLES引擎。
     * dataCallback：数据回调接口。为空时，需要客户端自行向播放器填充数据。
     * sampleRate：采样率
     * channelCount：声道数
     * framesPerBuffer：一个buffer包含多少帧，通常这是在使用低延迟音频时会设置。
     * return：是否成功初始化。
     * */
    bool init(SLESEngine &engine, ISLESPlayerCallback *dataCallback, int32_t sampleRate, int32_t channelCount, int32_t framesPerBuffer = 256);

    /**
     * 销毁资源
     * */
    void destroy();

    /**
     * 开始播放
     * */
    void start();

    /**
     * 停止播放。
     * */
    void stop();

    /**
     * 客户端自行向播放器填充数据的接口。
     * audioData：音频数据。
     * numFrames：audioData里包含多少帧。
     * */
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
