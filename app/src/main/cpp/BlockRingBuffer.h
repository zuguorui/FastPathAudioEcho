//
// Created by zu on 2020/8/1.
//

#ifndef FASTPATHAUDIOECHO_BLOCKRINGBUFFER_H
#define FASTPATHAUDIOECHO_BLOCKRINGBUFFER_H

#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <mutex>
#include <condition_variable>

using namespace std;

template <typename T>
class BlockRingBuffer {
public:
    BlockRingBuffer(int32_t capacity);
    ~BlockRingBuffer();

    bool put(T t, bool wait = true);
    T get(bool wait = true);

    int32_t putAll(T *t, int32_t size, bool wait = true);

    int32_t getRange(T *t, int32_t size, bool wait = true);

    void clear();

    int32_t size();

    void setWaitPutState(bool enable);
    void setWaitGetState(bool enable);

    bool getWaitPutState();
    bool getWaitGetState();

private:
    int32_t capacity = 0;

    int32_t getPtr = 0;
    int32_t putPtr = 0;

    bool waitPutFlag = true;
    bool waitGetFlag = true;


    T *buffer = nullptr;

    mutex bufferMu;
    condition_variable notEmptySignal;
    condition_variable notFullSignal;

};

template <typename T>
BlockRingBuffer<T>::BlockRingBuffer(int32_t capacity): capacity{capacity} {
    buffer = (T *)calloc(capacity, sizeof(T));
    getPtr = 0;
    putPtr = 0;
}

template <typename T>
BlockRingBuffer<T>::~BlockRingBuffer() {
    notFullSignal.notify_all();
    notEmptySignal.notify_all();
    if(buffer)
    {
        free(buffer);
        buffer = nullptr;
    }
}

template <typename T>
T BlockRingBuffer<T>::get(bool wait) {
    unique_lock<mutex> lock(bufferMu);

    T result = nullptr;
    if(getPtr == putPtr)
    {
        if(wait && waitGetFlag)
        {
            while(getPtr == putPtr)
            {
                if(!waitGetFlag)
                {
                    break;
                }
                notEmptySignal.wait(lock);
            }
            if(getPtr < putPtr)
            {
                result = buffer[getPtr % capacity];
                getPtr++;
            }
        }
    }else
    {
        result = buffer[getPtr % capacity];
        getPtr++;

    }
    int32_t c = capacity * (getPtr / capacity);
    getPtr -= c;
    putPtr -= c;
    if(putPtr - capacity < getPtr)
    {
        notFullSignal.notify_all();
    }
    lock.unlock();
    return result;
}

template <typename T>
bool BlockRingBuffer<T>::put(T t, bool wait) {
    unique_lock<mutex> lock(bufferMu);
    bool result = true;
    if(putPtr - capacity == getPtr)
    {
        if(wait && waitPutFlag)
        {
            while(putPtr - capacity == getPtr)
            {
                if(!waitPutFlag)
                {
                    break;
                }
                notFullSignal.wait(lock);
            }
            if(putPtr - capacity < getPtr)
            {
                buffer[putPtr % capacity] = t;
                putPtr++;
            } else
            {
                result = false;
            }

        } else {
            result = false;
        }
    }
    else{
        buffer[putPtr % capacity] = t;
        putPtr++;
    }
    int32_t c = capacity * (getPtr / capacity);
    getPtr -= c;
    putPtr -= c;
    if(putPtr > getPtr)
    {
        notEmptySignal.notify_all();
    }
    lock.unlock();
    return result;
}

template <typename T>
int32_t BlockRingBuffer<T>::getRange(T *t, int32_t size, bool wait) {
    unique_lock<mutex> lock(bufferMu);

    int32_t copyCount = 0;
    while(copyCount < size)
    {
        if(getPtr == putPtr)
        {
            if(wait && waitGetFlag)
            {
                while(getPtr == putPtr)
                {
                    if(!waitGetFlag)
                    {
                        break;
                    }
                    notEmptySignal.wait(lock);
                }
                continue;
            } else
            {
                break;
            }
        }
        else
        {
            int32_t leftCount = min(putPtr - getPtr, size - copyCount);
            memcpy(t + copyCount, buffer + (getPtr % capacity), leftCount * sizeof(T));
            getPtr += leftCount;
            copyCount += leftCount;
            if(!wait || !waitGetFlag)
            {
                break;
            }
        }
    }
    int32_t c = capacity * (getPtr / capacity);
    getPtr -= c;
    putPtr -= c;
    if(putPtr - capacity < getPtr)
    {
        notFullSignal.notify_all();
    }
    lock.unlock();
    return copyCount;
}

template <typename T>
int32_t BlockRingBuffer<T>::putAll(T *t, int32_t size, bool wait) {
    unique_lock<mutex> lock(bufferMu);
    int32_t copyCount = 0;

    while(copyCount < size)
    {
        if(putPtr - capacity == getPtr)
        {
            if(wait && waitPutFlag)
            {
                while(putPtr - capacity == getPtr)
                {
                    if(!waitPutFlag)
                    {
                        break;
                    }
                    notFullSignal.wait(lock);
                }
                continue;
            } else
            {
                break;
            }
        } else
        {
            int32_t leftCount = min(getPtr + capacity - putPtr, size - copyCount);
            memcpy(buffer + (putPtr % capacity), t + copyCount, leftCount * sizeof(T));
            putPtr += leftCount;
            copyCount += leftCount;
            if(!wait || !waitPutFlag)
            {
                break;
            }
        }
    }
    int32_t c = capacity * (getPtr / capacity);
    getPtr -= c;
    putPtr -= c;
    if(putPtr > getPtr)
    {
        notEmptySignal.notify_all();
    }
    lock.unlock();
    return copyCount;
}

template <typename T>
void BlockRingBuffer<T>::clear() {
    unique_lock<mutex> lock(bufferMu);
    putPtr = 0;
    getPtr = 0;
    lock.unlock();
}

template <typename T>
int32_t BlockRingBuffer<T>::size() {
    int32_t tempGetPtr = getPtr;
    int32_t tempPutPtr = putPtr;

    int32_t size = abs(tempPutPtr = tempGetPtr);
    if(tempGetPtr > tempPutPtr)
    {
        size = capacity - size;
    }
    return size;
}

template <typename T>
void BlockRingBuffer<T>::setWaitGetState(bool enable) {
    unique_lock<mutex> lock(bufferMu);
    waitGetFlag = enable;
    if(!enable)
    {
        notFullSignal.notify_all();
        notEmptySignal.notify_all();
    }
    lock.unlock();
}

template <typename T>
void BlockRingBuffer<T>::setWaitPutState(bool enable) {
    unique_lock<mutex> lock(bufferMu);
    waitPutFlag = enable;
    notFullSignal.notify_all();
    notEmptySignal.notify_all();
    lock.unlock();
}

template <typename T>
bool BlockRingBuffer<T>::getWaitGetState() {
    return waitGetFlag;
}

template <typename T>
bool BlockRingBuffer<T>::getWaitPutState() {
    return waitPutFlag;
}

#endif //FASTPATHAUDIOECHO_BLOCKRINGBUFFER_H
