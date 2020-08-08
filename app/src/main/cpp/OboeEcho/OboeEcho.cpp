//
// Created by incus on 2020-06-24.
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
    inputBuilder.setPerformanceMode(PerformanceMode::LowLatency);
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
    outputBuilder.setPerformanceMode(PerformanceMode::LowLatency);
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
    SLESJustStart = true;

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

        int32_t readSize = buffer->getRange((int16_t *)audioData, numFrames, !SLESJustStart);
        if(SLESJustStart)
        {
            SLESJustStart = false;
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

