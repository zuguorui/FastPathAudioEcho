//
// Created by zu on 2020/7/27.
//

#include "SLESRecorder.h"
#include <android/log.h>


#define MODULE_NAME  "SLESRecorder"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__)\

void SLESRecorder::recorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    SLESRecorder *recorder = (SLESRecorder *)context;
    recorder->processInput(bq);
}

void SLESRecorder::processInput(SLAndroidSimpleBufferQueueItf bq) {
    SLresult result = (*bq)->Enqueue(bq, audioBuffer, framesPerBuffer * sizeof(int16_t));
    if(result == SL_RESULT_SUCCESS)
    {
        dataCallback->readData(audioBuffer, framesPerBuffer);
    } else
    {
        LOGE("Error when read data from recorder");
    }
}

SLESRecorder::SLESRecorder() {

}

SLESRecorder::~SLESRecorder() {

}

bool SLESRecorder::init(SLESEngine &engine, ISLESRecorderCallback *dataCallback, int32_t sampleRate, int32_t framesPerBuffer) {
    this->dataCallback = dataCallback;
    this->sampleRate = sampleRate;
    this->framesPerBuffer = framesPerBuffer;

    if(audioBuffer)
    {
        free(audioBuffer);
    }
    audioBuffer = (int16_t *)calloc(framesPerBuffer, sizeof(int16_t));

    const SLEngineItf engineEngine = engine.getEngine();

    if(!engineEngine)
    {
        LOGE("engineEngine null");
        return false;
    }

    SLresult result;
    SLDataLocator_IODevice deviceInputLocator = { SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL };
    SLDataSource inputSource = { &deviceInputLocator, NULL };

    SLDataLocator_AndroidSimpleBufferQueue inputLocator = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
    SLDataFormat_PCM inputFormat = { SL_DATAFORMAT_PCM, 1, (SLuint32)sampleRate * 1000, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_LEFT, SL_BYTEORDER_LITTLEENDIAN };

    SLDataSink inputSink = { &inputLocator, &inputFormat };

    const SLInterfaceID inputInterfaces[2] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION };

    const SLboolean requireds[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

    // 创建AudioRecorder
    result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject, &inputSource, &inputSink, 2, inputInterfaces, requireds);
    if(result != SL_RESULT_SUCCESS)
    {
        LOGE("create recorder error");
        return false;
    }

    // 设置recorder的config为SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION，这是开启录音器低延迟的方法。
    SLAndroidConfigurationItf recordConfig;
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDCONFIGURATION, &recordConfig);
    if(result == SL_RESULT_SUCCESS)
    {
        SLuint32 presentValue = SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION;
        (*recordConfig)->SetConfiguration(recordConfig, SL_ANDROID_KEY_RECORDING_PRESET, &presentValue, sizeof(SLuint32));
    }

// 初始化AudioRecorder
    result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
    if(result != SL_RESULT_SUCCESS)
    {
        LOGE("realise recorder object error");
        return false;
    }




    // 获取录制器接口

    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
    if(result != SL_RESULT_SUCCESS)
    {
        LOGE("get interface error");
        return false;
    }


    // 获取音频输入的BufferQueue接口
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &recorderBufferQueue);
    if(result != SL_RESULT_SUCCESS)
    {
        LOGE("get buffer queue error");
        return false;
    }


    if(dataCallback)
    {
        result = (*recorderBufferQueue)->RegisterCallback(recorderBufferQueue, recorderCallback, this);
        if(result != SL_RESULT_SUCCESS)
        {
            LOGE("register callback error");
            return false;
        }
    }



    // 要注意，OpenSLES在创建好播放器或者录音器后，需要手动Enqueue一次，才能触发主动回调。
    result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, audioBuffer, framesPerBuffer * sizeof(int16_t));
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }

    (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
    return true;
}

void SLESRecorder::destroy() {
    if(recorderRecord != nullptr)
    {
        (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
        recorderRecord = nullptr;
        (*recorderObject)->Destroy(recorderObject);
        recorderObject = nullptr;
        recorderBufferQueue = nullptr;
    }

    if(audioBuffer)
    {
        free(audioBuffer);
        audioBuffer = nullptr;
    }
}

void SLESRecorder::start() {
    if(recorderRecord == nullptr)
    {
        return;
    }
    SLresult result;
    SLuint32 recordState;
    (*recorderRecord)->GetRecordState(recorderRecord, &recordState);
    if(recordState == SL_RECORDSTATE_RECORDING)
    {
        LOGE("recorder already recording");
        return;
    }
    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING);
}

void SLESRecorder::stop() {
    SLresult result;
    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
}

int32_t SLESRecorder::readData(int16_t *audioData, int32_t numFrames) {
    SLresult result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, audioData, numFrames * sizeof(int16_t));
    return result;
}


