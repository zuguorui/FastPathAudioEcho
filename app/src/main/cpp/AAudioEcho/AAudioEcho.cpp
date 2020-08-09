//
// Created by 祖国瑞 on 2020-06-06.
//

#include "AAudioEcho.h"



#define MODULE_NAME  "AAudioEcho"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__)


AAudioEcho::AAudioEcho() {

}

AAudioEcho::~AAudioEcho() {

}

bool AAudioEcho::init(int32_t sampleRate, int32_t framesPerBuffer, int32_t micID) {

    this->sampleRate = sampleRate;
    this->framesPerBuffer = framesPerBuffer;
    this->micID = micID;

    playFlag = false;

    deleteRecorder();
    if(!newRecorder())
    {
        deleteRecorder();
        return false;
    }

    deletePlayer();
    if(!newPlayer())
    {
        deleteRecorder();
        deletePlayer();
        return false;
    }

    return true;
}

bool AAudioEcho::newRecorder() {

    recorder = new AAudioRecorder();
    if(!recorder->init(this, sampleRate, AAudioRecorder::PERFORMANCE_MODE::LOW_LATENCY, framesPerBuffer, micID))
    {
        recorder->destroy();
        delete(recorder);
        LOGE("init recorder failed");
        return false;
    }
    return true;
}

void AAudioEcho::deleteRecorder() {
    if(recorder)
    {
        recorder->stop();
        recorder->destroy();
        delete(recorder);
    }
}

bool AAudioEcho::newPlayer() {
    player = new AAudioPlayer();

    if(!player->init(this, sampleRate, 1,  AAudioPlayer::PERFORMANCE_MODE::LOW_LATENCY, framesPerBuffer))
    {
        LOGE("init player failed");
        player->destroy();
        delete(player);
        return false;
    }
    return true;
}

void AAudioEcho::deletePlayer() {
    if(player)
    {
        player->stop();
        player->destroy();
        delete(player);
    }
}
void AAudioEcho::destroy() {
    stop();
    deleteRecorder();
    deletePlayer();
}

void AAudioEcho::start() {
    playFlag = true;
    // 先启动player使其处于等待状态。

    player->start();
    recorder->start();

}

void AAudioEcho::stop() {
    playFlag = false;
    recorder->stop();
    dataQueue.notifyWaitPush();
    player->stop();
    dataQueue.notifyWaitPull();

    AudioFrame *frame;
    while((frame = dataQueue.pull_front(false)) != nullptr)
    {
        junkQueue.push_back(frame, false);
    }

}

int32_t AAudioEcho::writeData(AAudioStream *stream, void *audioData, int32_t numFrames) {

    // 对于AAudio来说，它并不像OpenSLES那样会返回指定帧数的数据，而是会变化的。由于这里使用AudioFrame为单位存储一段数据，因此需要手动拼接。
    LOGD("writeData, numFrames = %d", numFrames);
    if(!player)
    {
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }
    if(!playFlag)
    {
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }

    int dstIndex = 0;
    int16_t *playBuffer = (int16_t *)audioData;
    while(dstIndex < numFrames)
    {
        if(playingFrame == nullptr)
        {
            playingFrame = dataQueue.pull_front();

            playingFrameIndex = 0;
            if(playingFrame == nullptr)
            {
                return AAUDIO_CALLBACK_RESULT_CONTINUE;
            }
        } else
        {
            if(playingFrameIndex >= playingFrame->sampleCount)
            {
                junkQueue.push_back(playingFrame);
                playingFrame = nullptr;
                continue;
            }
        }
        playBuffer[dstIndex++] = playingFrame->data[playingFrameIndex++];
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

int32_t AAudioEcho::readData(AAudioStream *stream, void *audioData, int32_t numFrames) {
    if(!recorder)
    {
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }
    if(!playFlag)
    {
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }
    LOGD("readData, numFrames = %d", numFrames);
    int srcIndex = 0;

    int16_t *recordBuffer = (int16_t *)audioData;
    while(srcIndex < numFrames)
    {
        //LOGD("srcIndex = %d, recordingFrameIndex = %d", srcIndex, recordingFrameIndex);
        if(recordingFrame == nullptr)
        {
            recordingFrame = getFreeNode();
            recordingFrameIndex = 0;
        } else
        {
            if(recordingFrameIndex >= recordingFrame->sampleCount)
            {
                dataQueue.push_back(recordingFrame);
                recordingFrame = nullptr;
                continue;
            }
        }
        recordingFrame->data[recordingFrameIndex++] = recordBuffer[srcIndex++];
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

inline AudioFrame* AAudioEcho::getFreeNode() {
    // 获取一个废弃的AudioFrame用以填充新数据。如果junkQueue里没有，那么新建一个。所有AudioFrame的大小都是
    // framesPerBuffer * sizeof(int16)。
    AudioFrame *frame = junkQueue.pull_front(false);
    if(!frame)
    {
        frame = new AudioFrame(framesPerBuffer * sizeof(int16_t));
        frame->sampleCount = framesPerBuffer;
    }
    return frame;
}
