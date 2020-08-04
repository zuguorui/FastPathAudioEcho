//
// Created by zu on 2020/7/27.
//

#include "SLESPlayer.h"
#include <android/log.h>

#define MODULE_NAME  "SLESPlayer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__)

SLESPlayer::SLESPlayer() {

}

SLESPlayer::~SLESPlayer() {

}

bool SLESPlayer::init(SLESEngine &engine, ISLESPlayerCallback *dataCallback, int32_t sampleRate, int32_t channelCount,
                      int32_t framesPerBuffer) {
    this->dataCallback = dataCallback;
    this->sampleRate = sampleRate;
    this->framesPerBuffer = framesPerBuffer;
    this->channelCount = channelCount;

    if(audioBuffer)
    {
        free(audioBuffer);
    }
    audioBuffer = (int16_t *)calloc(framesPerBuffer * channelCount, sizeof(int16_t));

    SLEngineItf engineEngine = engine.getEngine();
    SLresult result;

    SLInterfaceID ids1[1] = {SL_IID_OUTPUTMIX};
    SLboolean reqs1[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, ids1, reqs1);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }


    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }


    // Create player
    SLDataLocator_AndroidSimpleBufferQueue bufferQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM pcmFormat = {SL_DATAFORMAT_PCM, (uint32_t)channelCount, (uint32_t)sampleRate * 1000, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                  SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};

    SLDataSource audioSrc = {&bufferQueue, &pcmFormat};

    SLDataLocator_OutputMix locOutputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSink = {&locOutputMix, NULL};

    SLInterfaceID ids2[2] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
    SLboolean reqs2[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_FALSE};

    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &playerObject, &audioSrc, &audioSink, 2, ids2, reqs2);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }


    result = (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }


    result = (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playerPlay);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }


    result = (*playerObject)->GetInterface(playerObject, SL_IID_BUFFERQUEUE, &playerBufferQueue);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }

    if(dataCallback)
    {
        result = (*playerBufferQueue)->RegisterCallback(playerBufferQueue, playerCallback, this);
        if(result != SL_RESULT_SUCCESS)
        {
            return false;
        }
    }


    result = (*playerBufferQueue)->Enqueue(playerBufferQueue, audioBuffer, framesPerBuffer * channelCount * sizeof(int16_t));
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }



    return true;
}

void SLESPlayer::destroy() {
    if(playerPlay != nullptr)
    {
        (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_STOPPED);
        playerPlay = nullptr;
        (*playerObject)->Destroy(playerObject);
        playerObject = nullptr;
        playerBufferQueue = nullptr;

    }

    if(audioBuffer)
    {
        free(audioBuffer);
        audioBuffer = nullptr;
    }
}

void SLESPlayer::start() {
    if(!playerPlay)
    {
        return;
    }
    SLresult result;
    memset(audioBuffer, 0, framesPerBuffer * channelCount * sizeof(int16_t));
    result = (*playerBufferQueue)->Enqueue(playerBufferQueue, audioBuffer, framesPerBuffer * channelCount * sizeof(int16_t));
    result = (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_PLAYING);
    if(result != SL_RESULT_SUCCESS)
    {
        LOGE("start failed");
    }
}

void SLESPlayer::stop() {
    if(!playerPlay)
    {
        return;
    }

    SLresult result;
    result = (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_STOPPED);
}

int32_t SLESPlayer::writeData(int16_t *audioData, int32_t numFrames) {
    SLresult result = (*playerBufferQueue)->Enqueue(playerBufferQueue, audioData, numFrames * channelCount * sizeof(int16_t));
    return result;
}

void SLESPlayer::playerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    SLESPlayer *player = (SLESPlayer *)context;
    player->processOutput(bq);
}

void SLESPlayer::processOutput(SLAndroidSimpleBufferQueueItf bq) {
    int32_t writeSize = dataCallback->writeData(audioBuffer, framesPerBuffer);
    (*bq)->Enqueue(bq, audioBuffer, writeSize * channelCount * sizeof(int16_t));
}