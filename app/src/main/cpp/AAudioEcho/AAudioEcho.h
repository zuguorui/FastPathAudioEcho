//
// Created by 祖国瑞 on 2020-06-06.
//

#ifndef AAUDIOTEST_AAUDIOECHO_H
#define AAUDIOTEST_AAUDIOECHO_H

#include <stdlib.h>
#include <iostream>
#include <android/log.h>
#include <aaudio/AAudio.h>
#include "Constants.h"
#include "BlockRecyclerQueue.h"
#include "thread"
#include "IAudioFrameProvider.h"

#if defined(_INCUS_FRAMEWORK_VER_0)
extern "C"
{
#include "algorithm_control.h"
#include "audio_process.h"
}
#elif defined(_INCUS_FRAMEWORK_VER_1)
extern "C"
{
#include "incus_sdk.h"
}
#endif

using namespace std;

class AAudioEcho {
public:
    AAudioEcho();
    ~AAudioEcho();
    bool init(int32_t micID);
    void destroy();
    void start();
    void stop();

    void updateAlgorithmParams(int32_t algorithm_index, const int32_t *left_para, const int32_t *right_para);
    void algorithmStatusSet(int32_t algorithm_index, int32_t status, int32_t channel);

private:
    AAudioStream *inputStream = NULL;
    AAudioStream *outputStream = NULL;

    static aaudio_data_callback_result_t input_callback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames);
    aaudio_data_callback_result_t process_input(AAudioStream *stream, void *audioData, int32_t numFrames);

    static aaudio_data_callback_result_t output_callback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames);
    aaudio_data_callback_result_t process_output(AAudioStream *stream, void *audioData, int32_t numFrames);

    int16_t emptyBuffer[SAMPLES_PER_FRAME];
    int32_t convertBuffer[SAMPLES_PER_FRAME];
    int16_t buffer[SAMPLES_PER_FRAME];


    AudioFrame *playingFrame = NULL;
    int32_t playingFrameIndex = 0;

    char *algorithm_memory_page = NULL;


    BlockRecyclerQueue<AudioFrame *> *inputQueue = NULL;

    static void recordCallback(void *context);
    thread *recordThread = NULL;
    void recordLoop();
    bool stopRecordFlag = false;

#if defined(_INCUS_FRAMEWORK_VER_1)
    AlgorithmInstance algorithmInstance;
    bool algorithmOn = false;
    int16_t algorithmBuffer[2 * SAMPLES_PER_FRAME];
#endif

#ifdef DEBUG_PCM
    FILE *pcmFile = NULL;
#endif

};


#endif //AAUDIOTEST_AAUDIOECHO_H
