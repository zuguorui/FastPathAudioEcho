//
// Created by zu on 2020/8/8.
//

#ifndef FASTPATHAUDIOECHO_IECHO_H
#define FASTPATHAUDIOECHO_IECHO_H

#include <iostream>
#include <stdlib.h>
#include <stdint.h>

using namespace std;

class IEcho
{
public:
    virtual bool init(int32_t sampleRate, int32_t framesPerBuffer, int32_t micID = 0) = 0;
    virtual void destroy() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
};
#endif //FASTPATHAUDIOECHO_IECHO_H
