//
// Created by zu on 2020/7/27.
//

#ifndef FASTPATHAUDIOECHO_SLESRECORDER_H
#define FASTPATHAUDIOECHO_SLESRECORDER_H

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <stdint.h>

#include "Constants.h"
#include "SLESEngine.h"

/**
 * SLESRecorder的数据回调接口。
 * */
class ISLESRecorderCallback{
public:
    /**
     * 录音器通过此方法回调客户端，获取录音数据。
     * audioData：录音数据buffer
     * numFrames: audioData有多少帧数据，注意，一帧的意思是所有声道的一个数据。比如单声道，一帧就是一个int16。双声道则是左右各一个int16，一帧两个int16。
     * */
    virtual void readData(void *audioData, int32_t numFrames) = 0;
};
class SLESRecorder {
public:
    SLESRecorder();
    ~SLESRecorder();

    /**
     * 初始化。
     * engine：引擎
     * dataCallback：输出录音数据的回调。为空时，需要客户端主动从录音器读取数据
     * sampleRate：采样率
     * framesPerBuffer：buffer可以容纳多少帧数据。对于录音器来说，该选项并不会影响延迟。录音器总是以尽可能快的方式进行。
     * return：是否成功初始化
     * */
    bool init(SLESEngine &engine, ISLESRecorderCallback *dataCallback, int32_t sampleRate, int32_t framesPerBuffer = 256);
    /**
     * 销毁资源
     * */
    void destroy();

    /**
     * 开始录音
     * */
    void start();
    /**
     * 停止录音。
     * */
    void stop();

    /**
     * 客户端主动从录音器读取数据的接口。该方法为阻塞的，录音器总是会填充指定数量的数据。
     * audioData：容纳录音数据的buffer。
     * numFrames：audioData可以容纳多少帧数据。
     * return：返回值为SL_RESULT_*
     * */
    int32_t readData(int16_t *audioData, int32_t numFrames);

private:

    static void recorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
    void processInput(SLAndroidSimpleBufferQueueItf bq);

    int32_t framesPerBuffer = 0;
    int32_t sampleRate = 0;
    SLObjectItf recorderObject = nullptr;
    SLRecordItf recorderRecord = nullptr;

    int16_t *audioBuffer = nullptr;

    ISLESRecorderCallback *dataCallback = nullptr;

    SLAndroidSimpleBufferQueueItf recorderBufferQueue;


};


#endif //FASTPATHAUDIOECHO_SLESRECORDER_H
