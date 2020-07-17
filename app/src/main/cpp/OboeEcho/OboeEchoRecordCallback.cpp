//
// Created by incus on 2020-06-24.
//

#include "OboeEchoRecordCallback.h"
#include <android/log.h>

#define MODULE_NAME  "OboeEcho"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__)

OboeEchoRecordCallback::OboeEchoRecordCallback(BlockRecyclerQueue<AudioFrame *> *bufferQueue) {
    this->bufferQueue = bufferQueue;
    recordingFrame = NULL;
}

OboeEchoRecordCallback::~OboeEchoRecordCallback() {
    if(recordingFrame != NULL)
    {
        bufferQueue->putToUsed(recordingFrame);
        recordingFrame = NULL;
    }
}



DataCallbackResult OboeEchoRecordCallback::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) {
    if(numFrames != SAMPLES_PER_FRAME)
    {
        LOGE("record, numFrames != SAMPLES_PER_FRAME, %d", numFrames);
    }

    recordingFrame = bufferQueue->getUsed();
    if(recordingFrame == NULL)
    {
        recordingFrame = new AudioFrame(SAMPLES_PER_FRAME * sizeof(int16_t));
        recordingFrame->sampleCount = SAMPLES_PER_FRAME;
    }
    memcpy(recordingFrame->data, audioData, SAMPLES_PER_FRAME * sizeof(int16_t));
    bufferQueue->put(recordingFrame);
    return DataCallbackResult::Continue;
}
