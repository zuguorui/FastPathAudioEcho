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

class IAAudioRecorderCallback{
public:
    virtual int32_t readData(AAudioStream *stream, void *audioData, int32_t numFrames) = 0;
};

class AAudioRecorder {
public:
    AAudioRecorder();
    ~AAudioRecorder();

    bool init(IAAudioRecorderCallback *dataCallback, int32_t sampleRate, int32_t framesPerBuffer = 256, int32_t micID = -1);
    void destroy();

    void start();
    void stop();

    int32_t readData(int16_t *audioData, int32_t numFrames);

private:

    static aaudio_data_callback_result_t input_callback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames);

    int32_t sampleRate = 0;
    int32_t framesPerBuffer = 0;
    int32_t micID = -1;
    AAudioStream *inputStream = nullptr;
    IAAudioRecorderCallback *dataCallback = nullptr;


};


#endif //FASTPATHAUDIOECHO_AAUDIORECORDER_H
