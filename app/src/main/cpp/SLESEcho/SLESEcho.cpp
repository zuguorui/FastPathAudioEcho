//
// Created by incus on 2020-06-18.
//

#include "SLESEcho.h"
#include <android/log.h>
#include <chrono>
#include <math.h>




#define MODULE_NAME  "SLESEcho"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__)



SLESEcho::SLESEcho() {


}

SLESEcho::~SLESEcho() {

}

bool SLESEcho::init(int32_t sampleRate, int32_t framesPerBuffer, int32_t micID) {
    this->sampleRate = sampleRate;
    this->framesPerBuffer = framesPerBuffer;

    bool result = true;
    result = result && engine.init();
    if(!result)
    {
        LOGE("init engine error");
        return result;
    }

    result = result && player.init(engine, this, sampleRate, channelCount, framesPerBuffer);
    if(!result)
    {
        LOGE("init player error");
        engine.destroy();
        return result;
    }

    result = result && recorder.init(engine, this, sampleRate, framesPerBuffer);
    if(!result)
    {
        LOGE("init recorder error");
        player.destroy();
        engine.destroy();
        return result;
    }
    return result;
}

void SLESEcho::destroy() {

    recorder.destroy();
    player.destroy();
    engine.destroy();
}



void SLESEcho::start() {
    // 为了降低延迟，先启动播放器，让它的回调函数阻塞，一旦录音器有数据填充进来，可以立刻开始播放。
    player.start();
    recorder.start();

}

void SLESEcho::stop() {
    // 首先调用对应部件的stop方法，该方法是异步的，不会阻塞。但是回调函数可能仍然在阻塞，因此要对buffer进行设置，
    // 解除等待状态，然后再恢复阻塞功能。
    recorder.stop();
    buffer.setWaitPutState(false);

    player.stop();
    buffer.setWaitGetState(false);

    buffer.setWaitGetState(true);
    buffer.setWaitPutState(true);
}

int32_t SLESEcho::writeData(void *audioData, int32_t numFrames) {
    LOGD("writeData, numFrames = %d", numFrames);
    int32_t result = buffer.getRange((int16_t *)audioData, numFrames);
    return result;
}

void SLESEcho::readData(void *audioData, int32_t numFrames) {
    LOGD("readData, numFrames = %d", numFrames);
    int32_t result = buffer.putAll((int16_t *)audioData, numFrames);
}




