//
// Created by zu on 2020/7/27.
//

#ifndef FASTPATHAUDIOECHO_SLESRECORDER_H
#define FASTPATHAUDIOECHO_SLESRECORDER_H

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <stdint.h>

#include "Constants.h"
#include "SLESEngine.h"

class ISLESRecorderCallback{
public:
    virtual void readData(void *audioData, int32_t numFrames) = 0;
};
class SLESRecorder {
public:
    SLESRecorder();
    ~SLESRecorder();

    bool init(SLESEngine &engine, ISLESRecorderCallback *dataCallback, int32_t sampleRate, int32_t framesPerBuffer = 256);
    void destroy();

    void start();
    void stop();

    int32_t readData(int16_t *audioData, int32_t numFrames);

private:

    static void recorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
    void processInput(SLAndroidSimpleBufferQueueItf bq);

    int32_t framesPerBuffer = 0;
    int32_t sampleRate = 0;
    SLObjectItf recorderObject = nullptr;
    SLRecordItf recorderRecord = nullptr;

    int16_t *audioBuffer = nullptr;

    ISLESRecorderCallback *dataCallback = nullptr;

    SLAndroidSimpleBufferQueueItf recorderBufferQueue;


};


#endif //FASTPATHAUDIOECHO_SLESRECORDER_H
