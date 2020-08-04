//
// Created by zu on 2020/7/26.
//

#include "SLESEngine.h"
#include <android/log.h>
#define MODULE_NAME  "SLESEngine"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__)
SLESEngine::SLESEngine() {

}

SLESEngine::~SLESEngine() {

}

bool SLESEngine::init() {
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

void SLESEngine::destroy() {
    if(engineEngine != nullptr)
    {
        (*engineObject)->Destroy(engineObject);
    }
}

const SLEngineItf & SLESEngine::getEngine() {
    return engineEngine;
}