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
#include "BlockQueue.h"
#include "thread"
#include "AAudioPlayer.h"
#include "AAudioRecorder.h"
#include "IEcho.h"


using namespace std;

class AAudioEcho: public IEcho, public IAAudioPlayerCallback, public IAAudioRecorderCallback {
public:
    AAudioEcho();
    ~AAudioEcho();
    bool init(int32_t sampleRate, int32_t framesPerBuffer = 256, int32_t micID = 0) override;
    void destroy() override;
    void start() override;
    void stop() override;

    int32_t writeData(AAudioStream *stream, void *audioData, int32_t numFrames) override;

    int32_t readData(AAudioStream *stream, void *audioData, int32_t numFrames) override;

private:
    AAudioPlayer *player = nullptr;
    AAudioRecorder *recorder = nullptr;

    int32_t sampleRate;
    int32_t micID;

    int32_t framesPerBuffer = 0;

    AudioFrame *recordingFrame = nullptr;
    int32_t recordingFrameIndex = 0;

    AudioFrame *playingFrame = nullptr;
    int32_t playingFrameIndex = 0;

    BlockQueue<AudioFrame *> dataQueue{10};
    BlockQueue<AudioFrame *> junkQueue{10};

    inline AudioFrame *getFreeNode();

    bool newRecorder();
    void deleteRecorder();
    bool newPlayer();
    void deletePlayer();

    bool playFlag = false;




};


#endif //AAUDIOTEST_AAUDIOECHO_H
