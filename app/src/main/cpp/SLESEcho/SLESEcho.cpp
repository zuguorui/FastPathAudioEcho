//
// Created by incus on 2020-06-18.
//

#include "SLESEcho.h"
#include <android/log.h>
#include <chrono>
#include <math.h>




#define MODULE_NAME  "AudioEcho2"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__)



SLESEcho::SLESEcho() {
    bufferQueue = new BlockRecyclerQueue<AudioFrame *>(10);
    memset(emptyBuffer, 0, SAMPLES_PER_FRAME * sizeof(int16_t));

    processStopFlag = false;
//    processThread = new thread(processCallback, this);
#ifdef DEBUG_PCM
    pcmFile = fopen("/sdcard/test.pcm", "wb");
#endif

}

SLESEcho::~SLESEcho() {
    if(bufferQueue)
    {
        AudioFrame *frame = NULL;
        bufferQueue->discardAll(NULL);
        while((frame = bufferQueue->getUsed()) != NULL)
        {
            delete(frame);
            frame = NULL;
        }

        delete(bufferQueue);

    }

    processStopFlag = true;
    if(processThread && processThread->joinable())
    {
        processThread->join();
    }


#ifdef DEBUG_PCM
    if(pcmFile)
    {
        fflush(pcmFile);
        fclose(pcmFile);
    }
#endif
}

bool SLESEcho::init(int32_t micID) {
    bool result = true;
    result = result && initEngine();
    result = result && initPlayer();
    result = result && initRecorder();
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


    return result;
}

void SLESEcho::destroy() {
    releaseRecorder();
    releasePlayer();
    releaseEngine();
#if defined(_INCUS_FRAMEWORK_VER_1)
    ReleaseRealTimeProcessAlgorithms(&algorithmInstance);
#endif
    if(algorithm_memory_page)
    {
        free(algorithm_memory_page);
        algorithm_memory_page = NULL;
    }
}

void SLESEcho::processCallback(void *context) {
    SLESEcho *echo2 = (SLESEcho *)context;
    echo2->processLoop();
}

void SLESEcho::processLoop() {

    while(!processStopFlag)
    {
        AudioFrame *frame = bufferQueue->get();
#ifdef DEBUG_PCM
        fwrite(frame->data, sizeof(int16_t), SAMPLES_PER_FRAME, pcmFile);
#endif
        for(int i = 0; i < SAMPLES_PER_FRAME; i++)
        {
            convertBuffer[i] = (int32_t)(frame->data[i]);
        }


        for(int i = 0; i < SAMPLES_PER_FRAME; i++)
        {
            frame->data[i] = (int16_t)convertBuffer[i];
        }


        (*playerBufferQueue)->Enqueue(playerBufferQueue, frame->data, SAMPLES_PER_FRAME * sizeof(int16_t));
        bufferQueue->putToUsed(frame);

    }
}

void SLESEcho::start() {
    if(playerPlay == NULL || recorderRecord == NULL)
    {
        return;
    }
    SLresult result;
    //memset(emptyBuffer, 0, SAMPLES_PER_FRAME * sizeof(int16_t));
    result = (*playerBufferQueue)->Enqueue(playerBufferQueue, emptyBuffer, SAMPLES_PER_FRAME * sizeof(int16_t));
    result = (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_PLAYING);
    //result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
//    result = (*recorderBufferQueue)->Clear(recorderBufferQueue);
//    result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, audioBuffer, SAMPLES_PER_FRAME * sizeof(int16_t));
    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING);

    return;
}

void SLESEcho::stop() {
    if(playerPlay == NULL || recorderRecord == NULL)
    {
        return;
    }
    SLresult result;
    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
    //(*recorderBufferQueue)->Clear(recorderBufferQueue);
    result = (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_STOPPED);
    return;
}

bool SLESEcho::initEngine() {
    SLresult result;
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }


    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }


    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }
    return true;
}

bool SLESEcho::initPlayer() {
    if(!engineEngine)
    {
        return false;
    }

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
    SLDataFormat_PCM pcmFormat = {SL_DATAFORMAT_PCM, 1, SAMPLE_RATE * 1000, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
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


    result = (*playerBufferQueue)->RegisterCallback(playerBufferQueue, playerCallback, this);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }

//    (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_PAUSED);
    result = (*playerBufferQueue)->Enqueue(playerBufferQueue, emptyBuffer, SAMPLES_PER_FRAME * sizeof(int16_t));
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }



    return true;
}

bool SLESEcho::initRecorder() {

    if(!engineEngine)
    {
        return false;
    }

    SLresult result;
    SLDataLocator_IODevice deviceInputLocator = { SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL };
    SLDataSource inputSource = { &deviceInputLocator, NULL };

    SLDataLocator_AndroidSimpleBufferQueue inputLocator = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
    SLDataFormat_PCM inputFormat = { SL_DATAFORMAT_PCM, 1, SAMPLE_RATE * 1000, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_LEFT, SL_BYTEORDER_LITTLEENDIAN };

    SLDataSink inputSink = { &inputLocator, &inputFormat };

    const SLInterfaceID inputInterfaces[2] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION };

    const SLboolean requireds[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

    // 创建AudioRecorder
    result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject, &inputSource, &inputSink, 2, inputInterfaces, requireds);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }
// 初始化AudioRecorder
    result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }

    SLAndroidConfigurationItf recordConfig;
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDCONFIGURATION, &recordConfig);
    if(result == SL_RESULT_SUCCESS)
    {
        SLuint32 presentValue = SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION;
        (*recordConfig)->SetConfiguration(recordConfig, SL_ANDROID_KEY_RECORDING_PRESET, &presentValue, sizeof(SLuint32));
    }


    // 获取录制器接口

    (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }


    // 获取音频输入的BufferQueue接口
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &recorderBufferQueue);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }



    result = (*recorderBufferQueue)->RegisterCallback(recorderBufferQueue, recorderCallback, this);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }


    result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, audioBuffer, SAMPLES_PER_FRAME * sizeof(int16_t));
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }
    return true;
}

void SLESEcho::releasePlayer() {
    if(playerPlay != NULL)
    {
        (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_STOPPED);
        playerPlay = NULL;
        (*playerObject)->Destroy(playerObject);
        playerObject = NULL;
        playerBufferQueue = NULL;

    }
}

void SLESEcho::releaseRecorder() {
    if(recorderRecord != NULL)
    {
        (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
        recorderRecord = NULL;
        (*recorderObject)->Destroy(recorderObject);
        recorderObject = NULL;
        recorderBufferQueue = NULL;
    }
}

void SLESEcho::releaseEngine() {
    if(engineEngine != NULL)
    {
        (*engineObject)->Destroy(engineObject);
    }

}

void SLESEcho::algorithmStatusSet(int32_t algorithm_index, int32_t status, int32_t channel) {
    LOGD("algorithmStatusSet, index = %d, status = %d\n", algorithm_index, status);
#if defined(_INCUS_FRAMEWORK_VER_0)
    AlgorithmStatusSet(algorithm_index, status, channel);
#elif defined(_INCUS_FRAMEWORK_VER_1)
    algorithmOn = (status == 1);
#endif

}

void SLESEcho::updateAlgorithmParams(int32_t algorithm_index, const uint8_t *left_para,
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

AudioFrame* SLESEcho::newAudioFrame() {
    AudioFrame *frame = new AudioFrame(SAMPLES_PER_FRAME * sizeof(int16_t));
    return frame;
}

void SLESEcho::playerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    SLESEcho *player = (SLESEcho *)context;
    player->processOutput(bq);
}

void SLESEcho::recorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    SLESEcho *recorder = (SLESEcho *)context;
    recorder->processInput(bq);
}

void SLESEcho::processInput(SLAndroidSimpleBufferQueueItf bq) {
    if(recordingFrame != NULL)
    {
#ifdef DEBUG_PCM
        fwrite(frame->data, sizeof(int16_t), SAMPLES_PER_FRAME, pcmFile);
#endif
#if defined(_INCUS_FRAMEWORK_VER_0)
        for(int i = 0; i < SAMPLES_PER_FRAME; i++)
        {
            convertBuffer[i] = (int32_t)(frame->data[i]);
        }
        AudioProcess_HUAWEI(convertBuffer);
        for(int i = 0; i < SAMPLES_PER_FRAME; i++)
        {
            frame->data[i] = (int16_t)convertBuffer[i];
        }
#elif defined(_INCUS_FRAMEWORK_VER_1)
        if(algorithmOn)
        {
            LOGD("RealTimeProcess");
            for(int i = 0; i < SAMPLES_PER_FRAME; i++)
            {
                int16_t t = recordingFrame->data[i];
                algorithmBuffer[2 * i] = t;
                algorithmBuffer[2 * i + 1] = t;
            }
            RealTimeProcess(&algorithmInstance, algorithmBuffer, 2 * SAMPLES_PER_FRAME);
            for(int i = 0; i < SAMPLES_PER_FRAME; i++)
            {
                recordingFrame->data[i] = algorithmBuffer[2 * i];
            }
        }
#endif
        bufferQueue->put(recordingFrame);
    }
    recordingFrame = bufferQueue->getUsed();

    if(recordingFrame == NULL)
    {
        recordingFrame = newAudioFrame();
    }
    SLresult result = (*bq)->Enqueue(bq, recordingFrame->data, SAMPLES_PER_FRAME * sizeof(int16_t));

}



void SLESEcho::processOutput(SLAndroidSimpleBufferQueueItf bq) {
    //LOGD("processOutput called");
    if(playingFrame != NULL)
    {
        bufferQueue->putToUsed(playingFrame);
    }
    playingFrame = bufferQueue->get();
    if(playingFrame == NULL)
    {
        (*playerBufferQueue)->Enqueue(playerBufferQueue, emptyBuffer, SAMPLES_PER_FRAME * sizeof(int16_t));
    } else
    {
        (*bq)->Enqueue(bq, playingFrame->data, SAMPLES_PER_FRAME * sizeof(int16_t));
#ifdef DEBUG_PCM
//        fwrite(emptyBuffer, sizeof(int16_t), SAMPLES_PER_FRAME, pcmFile);
#endif
    }
}


