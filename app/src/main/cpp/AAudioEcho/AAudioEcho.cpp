//
// Created by 祖国瑞 on 2020-06-06.
//

#include "AAudioEcho.h"



#define MODULE_NAME  "AAudioEcho"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__)

aaudio_data_callback_result_t AAudioEcho::input_callback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames)
{

    AAudioEcho *context = (AAudioEcho *)userData;
    if(context->outputStream == NULL)
    {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }
    return context->process_input(stream, audioData, numFrames);

}

aaudio_data_callback_result_t AAudioEcho::process_input(AAudioStream *stream, void *audioData,
                                                        int32_t numFrames) {

    if(numFrames != SAMPLES_PER_FRAME)
    {
        LOGE("input numFrames not equals SAMPLES_PER_FRAME: %d", numFrames);
    }
    int i = 0;
    int16_t *inputData = (int16_t *)audioData;
    while(i < numFrames)
    {
        AudioFrame *frame = inputQueue->getUsed();
        if(frame == NULL)
        {
            frame = new AudioFrame(SAMPLES_PER_FRAME * sizeof(int16_t));
        }
        int count = min(SAMPLES_PER_FRAME, numFrames - i);

        memcpy(frame->data, inputData + i, count * sizeof(int16_t));
        frame->sampleCount = count;
        inputQueue->put(frame);
        i += count;
    }

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

aaudio_data_callback_result_t AAudioEcho::output_callback(AAudioStream *stream, void *userData,
                                                          void *audioData, int32_t numFrames)
{
    AAudioEcho *context = (AAudioEcho *)userData;
    if(context->outputStream == NULL)
    {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }
    return context->process_output(stream, audioData, numFrames);
}

aaudio_data_callback_result_t AAudioEcho::process_output(AAudioStream *stream, void *audioData,
                                                         int32_t numFrames) {
    if(numFrames != SAMPLES_PER_FRAME)
    {
        //LOGE("numFrames not equals SAMPLES_PER_FRAME: %d", numFrames);
    }
//    AudioFrame *frame = inputQueue->get();
//    if(frame == NULL)
//    {
//        memcpy(audioData, emptyBuffer, SAMPLES_PER_FRAME * sizeof(int16_t));
//    } else
//    {
//        memcpy(audioData, frame->data, frame->sampleCount * sizeof(int16_t));
//        inputQueue->putToUsed(frame);
//    }

    int16_t *playData = (int16_t *)audioData;
    int32_t dstIndex = 0;
    while(dstIndex < numFrames)
    {
        if(playingFrame == NULL)
        {
            playingFrame = inputQueue->get();
            if(playingFrame == NULL)
            {
                return AAUDIO_CALLBACK_RESULT_STOP;
            }
            playingFrameIndex = 0;
        }
        int32_t srcLeftCount = playingFrame->sampleCount - playingFrameIndex;
        int32_t dstLeftCount = numFrames - dstIndex;
        int count = min(srcLeftCount, dstLeftCount);
        memcpy(playData + dstIndex, playingFrame->data + playingFrameIndex, count * sizeof(int16_t));
        dstIndex += count;
        playingFrameIndex += count;
        if(playingFrameIndex >= playingFrame->sampleCount)
        {
            inputQueue->putToUsed(playingFrame);
            playingFrame = NULL;
            playingFrameIndex = 0;
        }
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}




AAudioEcho::AAudioEcho() {

#ifdef DEBUG_PCM
    pcmFile = fopen("/sdcard/test.pcm", "wb");
#endif

    inputQueue = new BlockRecyclerQueue<AudioFrame *>(10);

}

AAudioEcho::~AAudioEcho() {



    LOGD("process thread over");
    if(inputQueue)
    {
        AudioFrame *frame = NULL;
        inputQueue->discardAll(NULL);

        while((frame = inputQueue->getUsed()) != NULL)
        {
            delete(frame);
            frame = NULL;
        }
        delete(inputQueue);
    }
#ifdef DEBUG_PCM
    if(pcmFile)
    {
        fflush(pcmFile);
        fclose(pcmFile);
    }
#endif
}

bool AAudioEcho::init(int32_t micID) {
    aaudio_result_t result;
    AAudioStreamBuilder *inputBuilder, *outputBuilder;
    result = AAudio_createStreamBuilder(&inputBuilder);
    if(result != AAUDIO_OK)
    {
        LOGE("create input stream builder error");
        return false;
    }

    //AAudioStreamBuilder_setBufferCapacityInFrames(inputBuilder, SAMPLES_PER_FRAME);
    AAudioStreamBuilder_setDeviceId(inputBuilder, micID);
    AAudioStreamBuilder_setFormat(inputBuilder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setSamplesPerFrame(inputBuilder, SAMPLES_PER_FRAME);
    AAudioStreamBuilder_setSampleRate(inputBuilder, SAMPLE_RATE);

    AAudioStreamBuilder_setChannelCount(inputBuilder, 1);
    AAudioStreamBuilder_setDirection(inputBuilder, AAUDIO_DIRECTION_INPUT);
//    AAudioStreamBuilder_setDataCallback(inputBuilder, input_callback, this);
    result = AAudioStreamBuilder_openStream(inputBuilder, &inputStream);
    if(result != AAUDIO_OK)
    {
        LOGE("open record stream failed");
        return false;
    }


    result = AAudio_createStreamBuilder(&outputBuilder);
    if(result != AAUDIO_OK)
    {
        LOGE("create output stream builder error");
        return false;
    }
    //AAudioStreamBuilder_setBufferCapacityInFrames(outputBuilder, SAMPLES_PER_FRAME);
    AAudioStreamBuilder_setFormat(outputBuilder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setSamplesPerFrame(outputBuilder, SAMPLES_PER_FRAME);
    AAudioStreamBuilder_setSampleRate(outputBuilder, SAMPLE_RATE);
    AAudioStreamBuilder_setChannelCount(outputBuilder, 1);
    AAudioStreamBuilder_setDataCallback(outputBuilder, output_callback, this);
    AAudioStreamBuilder_setDirection(outputBuilder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setPerformanceMode(outputBuilder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    result = AAudioStreamBuilder_openStream(outputBuilder, &outputStream);
    if(result != AAUDIO_OK)
    {
        LOGE("open play stream failed");
        return false;
    }

    AAudioStreamBuilder_delete(inputBuilder);
    AAudioStreamBuilder_delete(outputBuilder);

    int32_t framesPerBurst = AAudioStream_getFramesPerBurst(outputStream);
    int32_t wantedFrames = min(SAMPLES_PER_FRAME, framesPerBurst);
    int32_t optimalBufferSize = wantedFrames * 2;
    result = AAudioStream_setBufferSizeInFrames(outputStream, optimalBufferSize);
    if(optimalBufferSize != result)
    {
        LOGE("can not use low latency, result = %d", result);
    }

//    framesPerBurst = AAudioStream_getFramesPerBurst(inputStream);
//    wantedFrames = min(SAMPLES_PER_FRAME, framesPerBurst);
//    optimalBufferSize = wantedFrames * 2;
//    result = AAudioStream_setBufferSizeInFrames(inputStream, optimalBufferSize);

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


    return true;

}

void AAudioEcho::destroy() {
    stop();
    if(inputStream)
    {
        AAudioStream_close(inputStream);
    }

    if(outputStream)
    {
        AAudioStream_close(outputStream);
    }
#if defined(_INCUS_FRAMEWORK_VER_1)
    ReleaseRealTimeProcessAlgorithms(&algorithmInstance);
#endif
    if(algorithm_memory_page)
    {
        free(algorithm_memory_page);
        algorithm_memory_page = NULL;
    }
}

void AAudioEcho::start() {
    if(!inputStream)
    {
        LOGE("input stream not created");
        return;
    }
    if(!outputStream)
    {
        LOGE("output stream not created");
        return;
    }

    if(AAudioStream_getState(inputStream) == AAUDIO_STREAM_STATE_STARTED)
    {
        LOGE("stream already started");
    }
    aaudio_result_t result;

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STARTING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;

    result = AAudioStream_requestStart(outputStream);
    AAudioStream_waitForStateChange(outputStream, inputState, &nextState, 100 * NANO_SEC_IN_MILL_SEC);
    if(nextState != AAUDIO_STREAM_STATE_STARTED)
    {
        LOGE("open output stream state = %d", nextState);
    }
    if(result != AAUDIO_OK)
    {
        LOGE("request start output stream error");
        return;
    }

    nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;

    result = AAudioStream_requestStart(inputStream);
    AAudioStream_waitForStateChange(inputStream, inputState, &nextState, 100 * NANO_SEC_IN_MILL_SEC);
    if(nextState != AAUDIO_STREAM_STATE_STARTED)
    {
        LOGE("open input stream state = %d", nextState);
    }
    if(result != AAUDIO_OK)
    {
        LOGE("request start input stream error");
        return;
    }
    stopRecordFlag = false;
    recordThread = new thread(recordCallback, this);

}

void AAudioEcho::stop() {
    stopRecordFlag = true;
    if(recordThread && recordThread->joinable())
    {
        inputQueue->notifyWaitPut();
        recordThread->join();
        delete(recordThread);
        recordThread = NULL;
    }

    aaudio_result_t result;

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STOPPING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
    result = AAudioStream_requestStop(inputStream);
    AAudioStream_waitForStateChange(inputStream, inputState, &nextState, 100 * NANO_SEC_IN_MILL_SEC);
    if(nextState != AAUDIO_STREAM_STATE_STOPPED)
    {
        LOGE("close input stream state = %d", nextState);
    }
    if(result != AAUDIO_OK)
    {
        LOGE("request close input stream error");
        return;
    }
    nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;

    inputQueue->notifyWaitGet();
    result = AAudioStream_requestStop(outputStream);

    AAudioStream_waitForStateChange(outputStream, inputState, &nextState,  100 * NANO_SEC_IN_MILL_SEC);
    if(nextState != AAUDIO_STREAM_STATE_STOPPED)
    {
        LOGE("close output stream state = %d", nextState);
    }
    if(result != AAUDIO_OK)
    {
        LOGE("request output input stream error");
        return;
    }

    inputQueue->discardAll(NULL);

}

void AAudioEcho::recordCallback(void *context) {
    AAudioEcho *echo = (AAudioEcho *)context;
    echo->recordLoop();
}

void AAudioEcho::recordLoop() {
    while(!stopRecordFlag)
    {
        AudioFrame *frame = inputQueue->getUsed();
        if(frame == NULL)
        {
            frame = new AudioFrame(SAMPLES_PER_FRAME * sizeof(int16_t));
        }
        frame->sampleCount = SAMPLES_PER_FRAME;
        aaudio_result_t result = AAudioStream_read(inputStream, frame->data, SAMPLES_PER_FRAME, 1000 * NANO_SEC_IN_MILL_SEC);
        if(result != SAMPLES_PER_FRAME)
        {
            LOGE("record error, result = %d", result);
            continue;
        }
#if defined(_INCUS_FRAMEWORK_VER_1)
//        if(algorithmOn)
//        {
//            for(int i = 0; i < SAMPLES_PER_FRAME; i++)
//            {
//                int16_t t = frame->data[i];
//                algorithmBuffer[2 * i] = t;
//                algorithmBuffer[2 * i + 1] = t;
//            }
//            RealTimeProcess(&algorithmInstance, algorithmBuffer, 2 * SAMPLES_PER_FRAME);
//            for(int i = 0; i < SAMPLES_PER_FRAME; i++)
//            {
//                frame->data[i] = algorithmBuffer[2 * i];
//            }
//
//        }

        for(int i = 0; i < SAMPLES_PER_FRAME; i++)
        {
            int16_t t = frame->data[i];
            algorithmBuffer[2 * i] = t;
            algorithmBuffer[2 * i + 1] = t;
        }
        RealTimeProcess(&algorithmInstance, algorithmBuffer, 2 * SAMPLES_PER_FRAME);
        for(int i = 0; i < SAMPLES_PER_FRAME; i++)
        {
            frame->data[i] = algorithmBuffer[2 * i];
        }
#endif
        inputQueue->put(frame);
        //LOGD("buffer size is %d", inputQueue->getSize());

    }
}

void AAudioEcho::algorithmStatusSet(int32_t algorithm_index, int32_t status, int32_t channel) {
    LOGD("algorithmStatusSet, index = %d, status = %d\n", algorithm_index, status);
#if defined(_INCUS_FRAMEWORK_VER_0)
    AlgorithmStatusSet(algorithm_index, status, channel);
#elif defined(_INCUS_FRAMEWORK_VER_1)
    algorithmOn = (status == 1);
    if(algorithmOn)
    {
        SetRealTimeProcessStatue(1);
    } else
    {
        SetRealTimeProcessStatue(0);
    }
#endif

}

void AAudioEcho::updateAlgorithmParams(int32_t algorithm_index, const int32_t *left_para,
                                       const int32_t *right_para) {
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



