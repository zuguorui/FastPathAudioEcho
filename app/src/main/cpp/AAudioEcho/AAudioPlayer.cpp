//
// Created by zu on 2020/7/25.
//

#include "AAudioPlayer.h"

#define MODULE_NAME  "AAudioPlayer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__)

AAudioPlayer::AAudioPlayer() {

}

AAudioPlayer::~AAudioPlayer() {

}

bool AAudioPlayer::init(IAAudioPlayerCallback *dataCallback, int32_t sampleRate, int32_t channelCount, PERFORMANCE_MODE mode, int32_t framesPerBuffer) {
    this->dataCallback = dataCallback;
    this->sampleRate = sampleRate;
    this->channelCount = channelCount;
    this->framesPerBuffer = framesPerBuffer;

    emptyBuffer = (int16_t *)calloc(framesPerBuffer * channelCount, sizeof(int16_t));

    aaudio_result_t result;

    AAudioStreamBuilder *outputBuilder;
    result = AAudio_createStreamBuilder(&outputBuilder);
    if(result != AAUDIO_OK)
    {
        LOGE("create output stream builder error");
        AAudioStreamBuilder_delete(outputBuilder);
        return false;
    }
    AAudioStreamBuilder_setDirection(outputBuilder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setFormat(outputBuilder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setSamplesPerFrame(outputBuilder, framesPerBuffer);
    AAudioStreamBuilder_setSampleRate(outputBuilder, sampleRate);
    AAudioStreamBuilder_setChannelCount(outputBuilder, channelCount);
    if(dataCallback)
    {
        AAudioStreamBuilder_setDataCallback(outputBuilder, output_callback, this);
    }

    AAudioStreamBuilder_setPerformanceMode(outputBuilder, mode);
    result = AAudioStreamBuilder_openStream(outputBuilder, &outputStream);

    AAudioStreamBuilder_delete(outputBuilder);
    if(result != AAUDIO_OK)
    {
        LOGE("open play stream failed");
        return false;
    }

    return true;
}

void AAudioPlayer::destroy() {
    if(emptyBuffer)
    {
        free(emptyBuffer);
        emptyBuffer = nullptr;
    }


    if(outputStream)
    {
        AAudioStream_close(outputStream);
        outputStream = nullptr;
    }
}

aaudio_data_callback_result_t AAudioPlayer::output_callback(AAudioStream *stream, void *userData,
                                                            void *audioData, int32_t numFrames) {
    AAudioPlayer *player = (AAudioPlayer *)userData;
    aaudio_data_callback_result_t result = player->dataCallback->writeData(stream, audioData, numFrames);
    LOGD("output_callback, result = %d", result);
    return result;
}

void AAudioPlayer::start() {
    if(!outputStream)
    {
        LOGE("output stream not created");
        return;
    }

    if(AAudioStream_getState(outputStream) == AAUDIO_STREAM_STATE_STARTED)
    {
        LOGE("player already started");
        return;
    }

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STARTING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;

    aaudio_result_t result;
    result = AAudioStream_requestStart(outputStream);
//    AAudioStream_waitForStateChange(outputStream, inputState, &nextState, 100 * NANO_SEC_IN_MILL_SEC);
//    if(nextState != AAUDIO_STREAM_STATE_STARTED)
//    {
//        LOGE("open output stream state = %d", nextState);
//    }
    if(result != AAUDIO_OK)
    {
        LOGE("request start output stream error");
        return;
    }
}

void AAudioPlayer::stop() {
    if(!outputStream)
    {
        LOGE("output stream not created");
        return;
    }

    if(AAudioStream_getState(outputStream) == AAUDIO_STREAM_STATE_STOPPED)
    {
        LOGE("player already stopped");
        return;
    }

    aaudio_result_t result;

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STOPPING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;

    result = AAudioStream_requestStop(outputStream);

//    AAudioStream_waitForStateChange(outputStream, inputState, &nextState,  100 * NANO_SEC_IN_MILL_SEC);
//    if(nextState != AAUDIO_STREAM_STATE_STOPPED)
//    {
//        LOGE("close output stream state = %d", nextState);
//    }
    if(result != AAUDIO_OK)
    {
        LOGE("request output input stream error");
        return;
    }

}

int32_t AAudioPlayer::writeData(int16_t *audioData, int32_t numFrames) {


    if(!outputStream)
    {
        LOGE("outputStream is nullptr when feedData");
        return -1;
    }

    return AAudioStream_write(outputStream, audioData, numFrames, 100 * NANO_SEC_IN_MILL_SEC);

}
