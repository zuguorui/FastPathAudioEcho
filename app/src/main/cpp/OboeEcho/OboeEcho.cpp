//
// Created by zu on 2020-06-24.
//

#include "OboeEcho.h"
#include <android/log.h>
#include <chrono>

#define MODULE_NAME  "OboeEcho"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__)

OboeEcho::OboeEcho() {

}

OboeEcho::~OboeEcho() {

}

bool OboeEcho::init(int32_t sampleRate, int32_t framesPerBuffer, int32_t micID) {
    return init(sampleRate, AudioApi::Unspecified, framesPerBuffer, micID);
}

bool OboeEcho::init(int32_t sampleRate, AudioApi api, int32_t framesPerBuffer, int32_t micID) {

    this->sampleRate = sampleRate;
    this->framesPerBuffer = framesPerBuffer;
    this->micID = micID;

    buffer = new BlockRingBuffer<int16_t>(3 * framesPerBuffer);



    oboe::Result result;
    AudioStreamBuilder inputBuilder;
    inputBuilder.setAudioApi(api);
    inputBuilder.setCallback(this);
    inputBuilder.setDirection(Direction::Input);
    inputBuilder.setChannelCount(ChannelCount::Mono);
    inputBuilder.setPerformanceMode(PerformanceMode::LowLatency); // 指定为低延迟
    inputBuilder.setSharingMode(SharingMode::Shared);
    inputBuilder.setFormat(AudioFormat::I16);
    inputBuilder.setSampleRate(sampleRate);
    inputBuilder.setFramesPerCallback(framesPerBuffer);
    inputBuilder.setDeviceId(micID);

    result = inputBuilder.openStream(&recordStream);
    if(result != Result::OK)
    {
        LOGE("create input stream failed");
        return false;
    }

    AudioStreamBuilder outputBuilder;
    outputBuilder.setAudioApi(api);
    outputBuilder.setCallback(this);
    outputBuilder.setDirection(Direction::Output);
    outputBuilder.setChannelCount(ChannelCount::Mono);
    outputBuilder.setPerformanceMode(PerformanceMode::LowLatency); // 指定为低延迟
    outputBuilder.setSharingMode(SharingMode::Shared);
    outputBuilder.setFormat(AudioFormat::I16);
    outputBuilder.setSampleRate(sampleRate);
    outputBuilder.setFramesPerCallback(framesPerBuffer);

    result = outputBuilder.openStream(&playStream);
    if(result != Result::OK)
    {
        LOGE("create output stream failed");
        return false;
    }

    this->inputApi = recordStream->getAudioApi();
    if(inputApi == AudioApi::OpenSLES)
    {
        LOGD("oboe recorder use api OpenSLES");
    }
    else if(inputApi == AudioApi::AAudio)
    {
        LOGD("oboe recorder use api AAudio");
    } else
    {
        LOGD("oboe recorder use api UNKNOWN");
    }

    this->outputApi = playStream->getAudioApi();
    if(inputApi == AudioApi::OpenSLES)
    {
        LOGD("oboe player use api OpenSLES");
    }
    else if(inputApi == AudioApi::AAudio)
    {
        LOGD("oboe player use api AAudio");
    } else
    {
        LOGD("oboe player use api UNKNOWN");
    }


    return true;

}

void OboeEcho::destroy() {
    if(recordStream)
    {
        recordStream->close();
    }

    if(playStream)
    {
        playStream->close();
    }

    if(buffer)
    {
        delete(buffer);
        buffer = nullptr;
    }
}

void OboeEcho::start() {
    if(!recordStream || !playStream)
    {
        LOGE("player or recorder not prepared");
        return;
    }
    playFlag = true;
    /*
     * 如果指定OpenSLES，则该标识为客户端是否刚启动播放。因为OpenSLES在启动时需要手动Enqueue一个空buffer。
     * 并且这个Enqueue的过程不是异步的，它会阻塞start方法。
     * 而由于我们希望尽可能降低播放延迟，因此都是首先启动play流。此时音频数据buffer是空的，play方法就会阻塞在
     * 回调函数里，等待录音流放入数据。但是play的start方法一直阻塞导致无法进行到recorder的start方法，就会发生死锁。
     * */
    justStart = true;

    playStream->start(10 * NANO_SEC_IN_MILL_SEC);
    recordStream->start(10 * NANO_SEC_IN_MILL_SEC);

}

void OboeEcho::stop() {
    playFlag = false;
    if(!recordStream || !playStream)
    {
        LOGE("player or recorder not prepared");
        return;
    }

    buffer->setWaitPutState(false);
    recordStream->stop(10 * NANO_SEC_IN_MILL_SEC);
    buffer->setWaitGetState(false);
    playStream->stop(10 * NANO_SEC_IN_MILL_SEC);


    buffer->setWaitGetState(true);
    buffer->setWaitPutState(true);

}

DataCallbackResult
OboeEcho::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) {
    if(!playFlag)
    {
        return DataCallbackResult::Continue;
    }
    if(oboeStream == playStream)
    {
        LOGD("player callback called");

        int32_t readSize = buffer->getRange((int16_t *)audioData, numFrames, !justStart);
        if(justStart)
        {
            justStart = false;
        }
//        int32_t readSize = 0;
//        sleep(2);

        LOGD("player get data, readSize = %d", readSize);
        if(readSize != numFrames)
        {
            return DataCallbackResult::Continue;
        }
    } else
    {
        LOGD("recorder callback called");
        int32_t writeSize = buffer->putAll((int16_t *)audioData, numFrames);
        LOGD("recorder write data, writeSize = %d", writeSize);
        if(writeSize != numFrames)
        {
            return DataCallbackResult::Continue;
        }
    }
    return DataCallbackResult::Continue;
}

