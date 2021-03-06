//
// Created by zu on 2020-07-02.
//

#ifndef FASTPATHAUDIOECHO_CONSTANTS_H
#define FASTPATHAUDIOECHO_CONSTANTS_H

#include <iostream>
#include <stdlib.h>
#include <string.h>

#define NANO_SEC_IN_MILL_SEC 100000
struct AudioFrame{
    int64_t pts;
    int16_t *data;
    int32_t sampleCount;

    int32_t maxDataSizeInByte = 0;

    AudioFrame(int32_t dataLenInByte)
    {
        this->maxDataSizeInByte = dataLenInByte;
        pts = 0;
        sampleCount = 0;
        data = (int16_t *)malloc(maxDataSizeInByte);
        memset(data, 0, maxDataSizeInByte);
    }

    ~AudioFrame(){
        if(data != NULL)
        {
            free(data);
        }
    }
};
#endif //FASTPATHAUDIOECHO_CONSTANTS_H
