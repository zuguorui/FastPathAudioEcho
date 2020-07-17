//
// Created by incus on 2020-06-18.
//

#ifndef INCUSAUDIOPROCESS_AUDIOECHO2_H
#define INCUSAUDIOPROCESS_AUDIOECHO2_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>


#include "BlockRecyclerQueue.h"
#include "Constants.h"


using namespace std;

class SLESEcho {
public:
    SLESEcho();
    ~SLESEcho();

    bool init(int32_t micID);
    void destroy();
    void start();
    void stop();



private:
    bool initEngine();

    bool initPlayer();

    bool initRecorder();

    void releaseEngine();
    void releasePlayer();
    void releaseRecorder();

    AudioFrame *newAudioFrame();



    static void playerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
    static void recorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);

    void processInput(SLAndroidSimpleBufferQueueItf bq);
    void processOutput(SLAndroidSimpleBufferQueueItf bq);

    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine;

    SLObjectItf recorderObject = NULL;
    SLRecordItf recorderRecord;

    SLObjectItf outputMixObject = NULL;

    SLObjectItf playerObject = NULL;
    SLPlayItf playerPlay;

    SLAndroidSimpleBufferQueueItf recorderBufferQueue;
    SLAndroidSimpleBufferQueueItf playerBufferQueue;


    BlockRecyclerQueue<AudioFrame *> *bufferQueue = NULL;

    AudioFrame *recordingFrame = NULL;
    AudioFrame *playingFrame = NULL;

    int16_t emptyBuffer[SAMPLES_PER_FRAME];
    int16_t audioBuffer[SAMPLES_PER_FRAME];
    int32_t convertBuffer[SAMPLES_PER_FRAME];

    bool processStopFlag = false;
    thread *processThread = NULL;
    static void processCallback(void *context);
    void processLoop();



};


#endif //INCUSAUDIOPROCESS_AUDIOECHO2_H
