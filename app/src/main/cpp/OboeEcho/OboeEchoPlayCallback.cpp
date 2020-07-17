//
// Created by incus on 2020-06-24.
//

#include "OboeEchoPlayCallback.h"
#include <android/log.h>

#define MODULE_NAME  "OboeEcho"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__)

OboeEchoPlayCallback::OboeEchoPlayCallback(BlockRecyclerQueue<AudioFrame *> *bufferQueue) {
    this->bufferQueue = bufferQueue;
    playingFrame = NULL;
}

OboeEchoPlayCallback::~OboeEchoPlayCallback() {
    if(playingFrame != NULL)
    {
        bufferQueue->putToUsed(playingFrame);
        playingFrame = NULL;
    }
}

bool OboeEchoPlayCallback::init() {
#if defined(_INCUS_FRAMEWORK_VER_0)
    int32_t total_memory_size = GetSystemTotalMemory();
    algorithm_memory_page = (char *)calloc(total_memory_size, sizeof(char));
    SetSystemMemory(algorithm_memory_page, total_memory_size);
    InitAudioProcessAlgorithms();
#elif defined(_INCUS_FRAMEWORK_VER_1)
    int32_t total_memory_size = IncusAlgorithmSize(ALGORITHM_ID);
    algorithm_memory_page = (char *)calloc(total_memory_size, sizeof(char));
    InitRealTimeProcessAlgorithms(&algorithmInstance, algorithm_memory_page, ALGORITHM_ID);
#endif
    return true;
}

void OboeEchoPlayCallback::destroy() {
#if defined(_INCUS_FRAMEWORK_VER_1)
    ReleaseRealTimeProcessAlgorithms(&algorithmInstance);
#endif
    if(algorithm_memory_page)
    {
        free(algorithm_memory_page);
        algorithm_memory_page = NULL;
    }
}

void OboeEchoPlayCallback::algorithmStatusSet(int32_t algorithm_index, int32_t status,
                                              int32_t channel) {
    LOGD("algorithmStatusSet, index = %d, status = %d\n", algorithm_index, status);
#if defined(_INCUS_FRAMEWORK_VER_0)
    AlgorithmStatusSet(algorithm_index, status, channel);
#elif defined(_INCUS_FRAMEWORK_VER_1)
    algorithmOn = (status == 1);
#endif
}

void OboeEchoPlayCallback::updateAlgorithmParams(int32_t algorithm_index, const uint8_t *left_para,
                                                 const uint8_t *right_para) {
    LOGD("updateAlgorithmParams");
#if defined(_INCUS_FRAMEWORK_VER_0)
    UpdateAlgorithmParams(algorithm_index, left_para, right_para);
#elif defined(_INCUS_FRAMEWORK_VER_1)
    int32_t params[PARAM_LEN];
    for(int i = 0; i < PARAM_LEN; i++)
    {
        params[i] = (int32_t)(left_para[i]);
    }
    UpdateRealTimeCompParameter(&algorithmInstance, ALGORITHM_ID, params, PARAM_LEN);
#endif
}

DataCallbackResult
OboeEchoPlayCallback::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) {
    if(numFrames != SAMPLES_PER_FRAME)
    {
        LOGE("play, numFrames != SAMPLES_PER_FRAME, %d", numFrames);
    }
    if(audioData == NULL)
    {
        return DataCallbackResult::Stop;
    }
    if(playingFrame != NULL)
    {
        bufferQueue->putToUsed(playingFrame);
    }
    playingFrame = NULL;
    playingFrame = bufferQueue->get();

    int16_t *outputData = (int16_t *)audioData;

    if(playingFrame == NULL)
    {
        memcpy(audioData, emptyBuffer, playingFrame->sampleCount * sizeof(int16_t));
    } else
    {
        if(algorithmOn)
        {
            for(int i = 0; i < SAMPLES_PER_FRAME; i++)
            {
                int16_t t = playingFrame->data[i];
                algorithmBuffer[2 * i] = t;
                algorithmBuffer[2 * i + 1] = t;
            }
            RealTimeProcess(&algorithmInstance, algorithmBuffer, 2 * SAMPLES_PER_FRAME);
            for(int i = 0; i < SAMPLES_PER_FRAME; i++)
            {
                outputData[i] = algorithmBuffer[2 * i];
            }

        } else
        {
            memcpy(audioData, playingFrame->data, playingFrame->sampleCount * sizeof(int16_t));
        }
    }
    return DataCallbackResult::Continue;
}
