//
// Created by zu on 2020/7/26.
//

#ifndef FASTPATHAUDIOECHO_SLESENGINE_H
#define FASTPATHAUDIOECHO_SLESENGINE_H

#include <stdio.h>
#include <stdlib.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

using namespace std;

/**
 * 封装OpenSL ES engine的类。
 * */
class SLESEngine {
public:
    SLESEngine();
    ~SLESEngine();
    bool init();
    void destroy();
    const SLEngineItf& getEngine();
private:
    SLObjectItf engineObject = nullptr;
    SLEngineItf engineEngine = nullptr;
};


#endif //FASTPATHAUDIOECHO_SLESENGINE_H
