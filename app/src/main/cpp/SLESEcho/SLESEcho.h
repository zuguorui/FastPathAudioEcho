//
// Created by zu on 2020-06-18.
//

#ifndef _SLES_ECHO_H_
#define _SLES_ECHO_H_

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
#include "IEcho.h"


using namespace std;

/**
 * 基于OpenSLES的echo功能。采用的是被动方式，也就是由OpenSLES自己回调函数来读写数据。
 * */
class SLESEcho: public IEcho, public ISLESPlayerCallback, public ISLESRecorderCallback{
public:
    SLESEcho();
    ~SLESEcho();

    /**
     * 初始化。
     * sampleRate：采样率。该采样率不能自由设置，需要设置为OpenSLES支持的采样率。但常用的16k、44.1k、48k皆有支持。
     * framesPerBuffer：每buffer所包含的帧数。
     * micID：录音所使用的micID。但是OpenSLES是不支持指定mic的，所以这个实际上没有用。
     * 返回：是否初始化成功
     * */
    bool init(int32_t sampleRate, int32_t framesPerBuffer = 256, int32_t micID = 0) override;
    /**
     * 销毁资源
     * */
    void destroy() override;
    /**
     * 开始回放
     * */
    void start() override;
    /**
     * 停止回放。
     * */
    void stop() override;

    // player的回调函数
    int32_t writeData(void *audioData, int32_t numFrames) override;

    // recorder的回调函数
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


#endif
