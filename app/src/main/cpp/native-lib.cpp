#include <jni.h>
#include <string>
#include "AAudioEcho.h"
#include "SLESEcho.h"
#include "OboeEcho.h"


//OboeEcho *echo = nullptr;
AAudioEcho *echo = nullptr;
//SLESEcho *echo = nullptr;


extern "C" JNIEXPORT jboolean JNICALL
Java_com_zu_fastpathaudioecho_MainActivity_nInit(JNIEnv *env, jobject instance, jint sampleRate, jint framesPerBuffer)
{
    if(!echo)
    {
        //echo = new OboeEcho();
        echo = new AAudioEcho();
//        echo = new SLESEcho();
    }
    return echo->init(sampleRate, framesPerBuffer);
}


extern "C" JNIEXPORT void JNICALL
Java_com_zu_fastpathaudioecho_MainActivity_nDestroy(JNIEnv *env, jobject instance)
{
    if(!echo)
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

