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

/**
 * AAudioPlayer获取数据的接口回调。
 * */
class IAAudioPlayerCallback
{
public:
    /**
     * Player通过这个函数向客户端获取数据。
     * stream：AAudio音频流，一般不需要对其进行操作。
     * audioData：客户端向此处填充音频数据。
     * numFrames：audioData可以容纳多少帧数据。
     * return：AAUDIO_CALLBACK_RESULT_CONTINUE或者AAUDIO_CALLBACK_RESULT_STOP。但是仍然需要手动调用start或play来开始停止播放。
     * 但是需要注意的是，一旦你从这里返回了STOP，下次再次start时，也不会调用回调函数。因此建议你总是返回CONTINUE，而停止启动状态通过start和stop
     * 来进行控制。
     * */
    virtual int32_t writeData(AAudioStream *stream, void *audioData, int32_t numFrames) = 0;
};

/**
 * 基于AAudio的播放器
 * */
class AAudioPlayer {
public:
    AAudioPlayer();
    ~AAudioPlayer();

    /**
     * 性能模式
     * */
    static enum PERFORMANCE_MODE{
        NONE = AAUDIO_PERFORMANCE_MODE_NONE,
        POWER_SAVING = AAUDIO_PERFORMANCE_MODE_POWER_SAVING,
        LOW_LATENCY = AAUDIO_PERFORMANCE_MODE_LOW_LATENCY
    };

    /**
     * 初始化。
     * dataCallback：获取数据的回调接口
     * sampleRate：采样率
     * channelCount：声道数
     * mode：性能模式
     * framesPerBuffer：每个buffer所容纳的帧数。
     * return：是否初始化成功
     * 如果需要启用低延迟模式，就要设置mode = LOW_LATENCY，并且sampleRate和framesPerBuffer要通过查询得知系统的最佳值。
     * */
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
