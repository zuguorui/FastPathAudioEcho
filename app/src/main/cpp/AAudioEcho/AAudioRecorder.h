//
// Created by zu on 2020/7/25.
//

#ifndef FASTPATHAUDIOECHO_AAUDIORECORDER_H
#define FASTPATHAUDIOECHO_AAUDIORECORDER_H

#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <thread>
#include <android/log.h>
#include <aaudio/AAudio.h>
#include "Constants.h"

/**
 * AAudioRecorder的数据回调接口
 * */
class IAAudioRecorderCallback{
public:
    /**
     * 客户端通过该函数获取录音器的数据
     * stream：AAudio的音频流，一般不需要对此进行操作。
     * audioData：音频数据
     * numFrames：audioData中包含多少帧。
     * return：AAUDIO_CALLBACK_RESULT_CONTINUE或者AAUDIO_CALLBACK_RESULT_STOP。但是仍然需要手动调用start或play来开始停止播放。
     * 但是需要注意的是，一旦你从这里返回了STOP，下次再次start时，也不会调用回调函数。因此建议你总是返回CONTINUE，而停止启动状态通过start和stop
     * 来进行控制。
     * */
    virtual int32_t readData(AAudioStream *stream, void *audioData, int32_t numFrames) = 0;
};

/**
 * 基于AAudio的录音器。
 * 固定单声道
 * */
class AAudioRecorder {
public:
    AAudioRecorder();
    ~AAudioRecorder();

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
     * dataCallback：客户端获取数据的回调接口
     * sampleRate：采样率
     * framesPerBuffer：每个buffer所容纳的帧数。
     * micID：指定mic的ID。
     * return：是否初始化成功
     * */
    bool init(IAAudioRecorderCallback *dataCallback, int32_t sampleRate, PERFORMANCE_MODE mode = NONE, int32_t framesPerBuffer = 256, int32_t micID = 0);
    void destroy();

    void start();
    void stop();

    int32_t readData(int16_t *audioData, int32_t numFrames);

private:

    static aaudio_data_callback_result_t input_callback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames);

    int32_t sampleRate = 0;
    int32_t framesPerBuffer = 0;
    int32_t micID = -1;
    PERFORMANCE_MODE mode;
    AAudioStream *inputStream = nullptr;
    IAAudioRecorderCallback *dataCallback = nullptr;


};


#endif //FASTPATHAUDIOECHO_AAUDIORECORDER_H
