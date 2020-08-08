#include <jni.h>
#include <string>
#include "AAudioEcho.h"
#include "SLESEcho.h"
#include "OboeEcho.h"
#include "IEcho.h"



IEcho *echo = nullptr;

int32_t api = -1;


extern "C" JNIEXPORT jboolean JNICALL
Java_com_zu_fastpathaudioecho_MainActivity_nInit(JNIEnv *env, jobject instance, jint sampleRate, jint framesPerBuffer, jint api)
{
    if(echo != nullptr)
    {
        return false;
    }
    if(api == 0)
    {
        echo = new SLESEcho();
        return echo->init(sampleRate, framesPerBuffer);
    } else if(api == 1)
    {
        echo = new AAudioEcho();
        return echo->init(sampleRate, framesPerBuffer);
    } else if(api >= 2)
    {
        echo = new OboeEcho();
        AudioApi audioApi = api == 2 ? AudioApi::OpenSLES : AudioApi::AAudio;
        return ((OboeEcho *)echo)->init(sampleRate, audioApi, framesPerBuffer);
    }
}


extern "C" JNIEXPORT void JNICALL
Java_com_zu_fastpathaudioecho_MainActivity_nDestroy(JNIEnv *env, jobject instance)
{
    if(echo == nullptr)
    {
        return;
    }
    echo->destroy();
    delete(echo);
    echo = nullptr;
}

extern "C" JNIEXPORT void JNICALL
Java_com_zu_fastpathaudioecho_MainActivity_nStart(JNIEnv *env, jobject instance)
{
    if(!echo)
    {
        return;
    }
    echo->start();
}

extern "C" JNIEXPORT void JNICALL
Java_com_zu_fastpathaudioecho_MainActivity_nStop(JNIEnv *env, jobject instance)
{
    if(!echo)
    {
        return;
    }
    echo->stop();
}

