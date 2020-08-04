//
// Created by zu on 2020/7/25.
//

#include "AAudioRecorder.h"

#define MODULE_NAME  "AAudioRecorder"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__)

AAudioRecorder::AAudioRecorder() {

}

AAudioRecorder::~AAudioRecorder() {

}

bool AAudioRecorder::init(IAAudioRecorderCallback *dataCallback, int32_t sampleRate, int32_t framesPerBuffer, int32_t micID) {
    this->sampleRate = sampleRate;
    this->framesPerBuffer = framesPerBuffer;
    this->micID = micID;
    this->dataCallback = dataCallback;

    aaudio_result_t result;
    AAudioStreamBuilder *inputBuilder;

    result = AAudio_createStreamBuilder(&inputBuilder);
    if(result != AAUDIO_OK)
    {
        LOGE("create input stream builder error");
        return false;
    }

    if(micID != -1)
    {
        AAudioStreamBuilder_setDeviceId(inputBuilder, micID);
    }

    AAudioStreamBuilder_setFormat(inputBuilder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setSamplesPerFrame(inputBuilder, framesPerBuffer);
    AAudioStreamBuilder_setSampleRate(inputBuilder, sampleRate);

    AAudioStreamBuilder_setChannelCount(inputBuilder, 1);
    AAudioStreamBuilder_setDirection(inputBuilder, AAUDIO_DIRECTION_INPUT);

    if(dataCallback)
    {
        AAudioStreamBuilder_setDataCallback(inputBuilder, input_callback, this);
    }

    result = AAudioStreamBuilder_openStream(inputBuilder, &inputStream);
    if(result != AAUDIO_OK)
    {
        LOGE("open record stream failed");
        return false;
    }

    return true;

}

void AAudioRecorder::destroy() {
    if(inputStream)
    {
        AAudioStream_close(inputStream);
        inputStream = nullptr;
    }
}

aaudio_data_callback_result_t AAudioRecorder::input_callback(AAudioStream *stream, void *userData,
                                                             void *audioData, int32_t numFrames) {
    AAudioRecorder *recorder = (AAudioRecorder *)userData;
    LOGD("input_callback");
    return recorder->dataCallback->readData(stream, audioData, numFrames);
}

void AAudioRecorder::start() {
    if(!inputStream)
    {
        LOGE("input stream not created");
        return;
    }

    if(AAudioStream_getState(inputStream) == AAUDIO_STREAM_STATE_STARTED)
    {
        LOGE("recorder already started");
        return;
    }

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STARTING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;

    aaudio_result_t result;

    result = AAudioStream_requestStart(inputStream);
//    AAudioStream_waitForStateChange(inputStream, inputState, &nextState, 100 * NANO_SEC_IN_MILL_SEC);
//    if(nextState != AAUDIO_STREAM_STATE_STARTED)
//    {
//        LOGE("open input stream state = %d", nextState);
//    }
    if(result != AAUDIO_OK)
    {
        LOGE("request start input stream error");
        return;
    }
}

void AAudioRecorder::stop() {
    if(!inputStream)
    {
        return;
    }
    aaudio_result_t result;

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STOPPING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;

    result = AAudioStream_requestStop(inputStream);
//    AAudioStream_waitForStateChange(inputStream, inputState, &nextState, 100 * NANO_SEC_IN_MILL_SEC);
//    if(nextState != AAUDIO_STREAM_STATE_STOPPED)
//    {
//        LOGE("close input stream state = %d", nextState);
//    }
    if(result != AAUDIO_OK)
    {
        LOGE("request close input stream error");
        return;
    }
}

int32_t AAudioRecorder::readData(int16_t *audioData, int32_t numFrames) {
    if(!inputStream)
    {
        LOGE("input stream is nullptr when read data");
        return -1;
    }
    return AAudioStream_read(inputStream, audioData, numFrames, 100 * NANO_SEC_IN_MILL_SEC);
}


