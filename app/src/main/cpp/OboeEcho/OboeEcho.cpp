//
// Created by incus on 2020-06-24.
//

#include "OboeEcho.h"
#include <android/log.h>

#define MODULE_NAME  "OboeEcho"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__)

OboeEcho::OboeEcho() {

}

OboeEcho::~OboeEcho() {

}

bool OboeEcho::init(int32_t micID) {
    bufferQueue = new BlockRecyclerQueue<AudioFrame *>(10);

    playCallback = new OboeEchoPlayCallback(bufferQueue);
    playCallback->init();
    recordCallback = new OboeEchoRecordCallback(bufferQueue);

    oboe::Result result;
    AudioStreamBuilder inputBuilder;
    inputBuilder.setAudioApi(AudioApi::AAudio);
    inputBuilder.setCallback(recordCallback);
    inputBuilder.setDirection(Direction::Input);
    inputBuilder.setChannelCount(ChannelCount::Mono);
    inputBuilder.setPerformanceMode(PerformanceMode::LowLatency);
    inputBuilder.setSharingMode(SharingMode::Shared);
    inputBuilder.setFormat(AudioFormat::I16);
    inputBuilder.setSampleRate(SAMPLE_RATE);
    inputBuilder.setFramesPerCallback(SAMPLES_PER_FRAME);
    inputBuilder.setDeviceId(micID);

    result = inputBuilder.openStream(&recordStream);
    if(result != Result::OK)
    {
        LOGE("create input stream failed");
        return false;
    }

    AudioStreamBuilder outputBuilder;
    outputBuilder.setAudioApi(AudioApi::AAudio);
    outputBuilder.setCallback(playCallback);
    outputBuilder.setDirection(Direction::Output);
    outputBuilder.setChannelCount(ChannelCount::Mono);
    outputBuilder.setPerformanceMode(PerformanceMode::LowLatency);
    outputBuilder.setSharingMode(SharingMode::Shared);
    outputBuilder.setFormat(AudioFormat::I16);
    outputBuilder.setSampleRate(SAMPLE_RATE);
    outputBuilder.setFramesPerCallback(SAMPLES_PER_FRAME);

    result = outputBuilder.openStream(&playStream);
    if(result != Result::OK)
    {
        LOGE("create output stream failed");
        return false;
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

    playCallback->destroy();
}

void OboeEcho::start() {
    if(!recordStream || !playStream)
    {
        LOGE("player or recorder not prepared");
        return;
    }
    playStream->start(10 * NANO_SEC_IN_MILL_SEC);
    recordStream->start(10 * NANO_SEC_IN_MILL_SEC);

//    if(playStream->getState() != StreamState::Started)
//    {
//        playStream->requestStart();
//    }
//    if(recordStream->getState() != StreamState::Started)
//    {
//        recordStream->requestStart();
//    }

}

void OboeEcho::stop() {
    if(!recordStream || !playStream)
    {
        LOGE("player or recorder not prepared");
        return;
    }

    bufferQueue->notifyWaitGet();
    playStream->stop(10 * NANO_SEC_IN_MILL_SEC);
    bufferQueue->notifyWaitPut();
    recordStream->stop(10 * NANO_SEC_IN_MILL_SEC);

//    playStream->requestStop();
//    recordStream->requestStop();

    bufferQueue->discardAll(NULL);
}

void OboeEcho::updateAlgorithmParams(int32_t algorithm_index, const uint8_t *left_para,
                                     const uint8_t *right_para) {
    if(playCallback != NULL)
    {
        playCallback->updateAlgorithmParams(algorithm_index, left_para, right_para);
    }
}

void OboeEcho::algorithmStatusSet(int32_t algorithm_index, int32_t status, int32_t channel) {
    if(playCallback != NULL)
    {
        playCallback->algorithmStatusSet(algorithm_index, status, channel);
    }
}