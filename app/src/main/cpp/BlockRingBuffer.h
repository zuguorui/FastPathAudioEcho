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

/**
 * 线程安全的环形buffer。
 * 采用如下方式实现环形buffer：
 * 申请一块T的数组内存，大小为capacity * sizeof(T)
 * 设置putPtr和getPtr作为指针。
 * 当putPtr == getPtr时，buffer为空。
 * 当putPtr - capacity == getPtr时，buffer满。
 * 注意每次对buffer进行下标操作时，都要取模。即putPtr % capacity和getPtr % capacity。
 * 为了防止putPtr和getPtr溢出，每次进行完get操作和put操作后，要对其进行规整。由于getPtr总是小于等于putPtr，
 * 因此是基于getPtr进行的。保证getPtr范围为[0, capacity - 1]，而putPtr范围为[getPtr, getPtr + capacity - 1]。
 * */
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

    // get指针
    int32_t getPtr = 0;
    // put指针
    int32_t putPtr = 0;

    // put操作是否会被阻塞
    bool waitPutFlag = true;
    // get操作是否会被阻塞
    bool waitGetFlag = true;

    // buffer
    T *buffer = nullptr;

    mutex bufferMu;
    condition_variable notEmptySignal;
    condition_variable notFullSignal;

};

/**
 * capacity为该buffer容量。
 * */
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

/**
 * 获取一个元素
 * wait = true时，如果buffer当前为空，则会阻塞直到buffer不为空。如果wait为false，buffer为空则返回nullptr。
 * */
template <typename T>
T BlockRingBuffer<T>::get(bool wait) {
    unique_lock<mutex> lock(bufferMu);

    T result = nullptr;
    if(getPtr == putPtr)
    {
        // 只有wait和waitGetFlag同时为真时才会等待。对应setWaitGetState()功能
        if(wait && waitGetFlag)
        {
            // getPtr == putPtr时，buffer为空。
            while(getPtr == putPtr)
            {
                // 如果在等待过程中调用了setWaitGetState(false)，那么代表此时客户端不希望我们陷入死等待，那就跳出等待。
                if(!waitGetFlag)
                {
                    break;
                }
                notEmptySignal.wait(lock);
            }
            // 出来之后，检查一遍是否可以获取元素。因为走到这一步可能是因为buffer中有可供获取的元素，或者客户端取消了我们的等待。
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
    // 对put和get指针规整。
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

/**
 * 向buffer中放入一个元素。
 * 如果wait为true，当buffer满时，则会等待到buffer不为满时将元素放入。如果wait为false，当buffer满时则不会等待，并且返回false。
 * 返回：是否成功将元素放入。false代表元素并没有被放入buffer中。
 * */
template <typename T>
bool BlockRingBuffer<T>::put(T t, bool wait) {
    unique_lock<mutex> lock(bufferMu);
    bool result = true;
    // putPtr - capacity == getPtr时，buffer满，等待
    if(putPtr - capacity == getPtr)
    {
        // 只有wait和waitPutFlag同时为真时才会等待。
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

/**
 * 获取多个元素
 * *t：存放输出元素的buffer。
 * size：要获取的元素个数
 * wait：是否等待直到获取size个元素
 * 返回值：实际获取了多少个元素
 * */
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
                // 如果跳出了等待循环，可能是因为buffer中新放入了可供获取的元素，或者是客户端调用setWaitGetState(false)
                // 导致的。那么重新开始循环，检查条件。
                continue;
            } else
            {
                // wait和waitGetFlag不同时为真，代表不等待，则直接跳出该复制循环。
                break;
            }
        }
        else
        {
            // 复制可供获取的元素。
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
    // 对getPtr和putPtr进行规整。
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

/**
 * 放入多个元素
 * *t：要放入的元素buffer。
 * size：要放入的元素个数。
 * wait：是否等待直到所有size个元素都被放入。
 * 返回值：实际被成功放入buffer的元素个数。
 * */
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

/**
 * 清空buffer。要注意，该操作只是重置了put和get指针，并没有对其中的元素有任何操作，请自行管理元素的内存。
 * */
template <typename T>
void BlockRingBuffer<T>::clear() {
    unique_lock<mutex> lock(bufferMu);
    putPtr = 0;
    getPtr = 0;
    lock.unlock();
}

/**
 * 获取当前存储的元素个数
 * 该方法并没有加锁，所以无法保证完全准确。
 * */
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

/**
 * 设置get操作的等待状态。
 * enable：是否允许get操作在元素不够时进行等待。如果为true，get操作正常按照对应方法中的wait决定是否等待。如果为false，那么无论对应
 * 方法中的wait是何值，都不会阻塞。效果等同于wait=false。
 * 如果设置enable=false时有在阻塞的get方法，那么此方法会解除阻塞，get方法会返回null_ptr，getRange会返回实际已获取的元素数。用以解除死等待。
 * */
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

/**
 * 设置put操作的等待状态。
 * enable：是否允许put操作在buffer满时进行等待。如果为true，put操作正常对应方法中的wait决定是否等待。如果为false，那么无论
 * 对应方法中的wait为何值，都不会阻塞，效果等同于wait=false。
 * 如果设置enable=false时由阻塞的put方法，那么此方法会解除阻塞，put方法会返回false，putAll会返回实际已放入buffer的元素个数。
 * 用以解除死等待。
 * */
template <typename T>
void BlockRingBuffer<T>::setWaitPutState(bool enable) {
    unique_lock<mutex> lock(bufferMu);
    waitPutFlag = enable;
    if(!enable)
    {
        notFullSignal.notify_all();
        notEmptySignal.notify_all();
    }

    lock.unlock();
}

/**
 * 获取当前get状态。
 * */
template <typename T>
bool BlockRingBuffer<T>::getWaitGetState() {
    return waitGetFlag;
}

/**
 * 获取当前put状态。
 * */
template <typename T>
bool BlockRingBuffer<T>::getWaitPutState() {
    return waitPutFlag;
}

#endif //FASTPATHAUDIOECHO_BLOCKRINGBUFFER_H
