//
// Created by zu on 2020/7/25.
//

#ifndef FASTPATHAUDIOECHO_BLOCKQUEUE_H
#define FASTPATHAUDIOECHO_BLOCKQUEUE_H

#include <iostream>
#include <thread>
#include <list>
#include <mutex>
#include <condition_variable>

using namespace std;

/**
 * 线程安全的queue。
 * */
template <typename T>
class BlockQueue {
public:

    //线程当前的状态
    enum State{
        RUNNING,
        WAIT_PULL,
        WAIT_PUSH
    };



    BlockQueue(int32_t capacity);
    ~BlockQueue();


    int32_t getSize();

    T pull_front(bool wait = true);
    void push_back(T t, bool wait = true);

    // to avoid dead lock
    void notifyWaitPush();
    void notifyWaitPull();

    State getState();

private:
    int32_t capacity = 0;
    mutex listMu;
    condition_variable notFullSignal;
    condition_variable notEmptySignal;

    bool notifyPull = false;
    bool notifyPush = false;

    list<T> queue;

    bool waitPush = false;
    bool waitPull = false;
};

/**
 * 构造函数，如果capacity == -1，那么queue的大小将没有限制。所有的put操作都不会被阻塞。
 * @param capacity queue的预期容量，当queue的size >= capacity时，put操作会阻塞。
 * */
template <typename T>
BlockQueue<T>::BlockQueue(int32_t capacity): capacity{capacity}, notifyPush{false}, notifyPull{false} {

}

/**
 * 析构函数
 * */
template <typename T>
BlockQueue<T>::~BlockQueue<T>() {

}

/**
 * 向queue的尾部添加一个元素。当queue的size已经达到capacity时，该操作会阻塞。
 * t：元素
 * wait：是否等待queue的size小于capacity。如果wait = true，那么该操作会阻塞直到queue不为满时将元素放入。
 * 如果wait = false，那么该操作会无视capacity，直接将元素放入。默认为true
 * */
template <typename T>
void BlockQueue<T>::push_back(T t, bool wait) {
    unique_lock<mutex> queueLock(listMu);
    if(this->capacity == -1 || !wait)
    {
        queue.push_back(t);
    } else
    {
        waitPush = false;
        while(queue.size() >= this->capacity)
        {
            waitPush = true;
            cout << "queue is full, wait" << endl;
            notFullSignal.wait(queueLock);
            if(notifyPush)
            {
                notifyPush = false;
                break;
            }
        }
        queue.push_back(t);
        waitPush = false;
    }
    queueLock.unlock();
    notEmptySignal.notify_all();
}

/**
 * 从queue头部拉取一个元素。
 * wait：是否一直等到到有可用元素。如果wait = true（默认情况），那么当queue为空时，该操作会一直阻塞直到queue不为空。
 * 如果wait为false，那么就会返回NULL。
 * */
template <typename T>
T BlockQueue<T>::pull_front(bool wait) {
    unique_lock<mutex> lock(listMu);
    waitPull = false;
    if(wait)
    {
        while(queue.empty())
        {
            waitPull = true;
            cout << "queue is empty, wait" << endl;
            notEmptySignal.wait(lock);
            if(notifyPull)
            {
                notifyPull = false;
                break;
            }
        }
        waitPull = false;
    }

    T element = NULL;
    if(!queue.empty())
    {
        element = queue.front();
        queue.pop_front();
    }
    notFullSignal.notify_all();
    lock.unlock();
    return element;
}

/**
 * 释放被阻塞的拉取操作。
 * 为了避免死锁，需要有一种方式来释放被阻塞的pull操作。这会导致pull_front返回NULL，即使wait = true。
 * */
template <typename T>
void BlockQueue<T>::notifyWaitPull() {
    notifyPull = true;
    notEmptySignal.notify_all();
}

/**
 * 释放被阻塞的推入操作。
 * 为了避免死锁，需要有一种方式来释放被阻塞的push操作。这会导致push_back忽略capacity和wait，直接将元素放入queue中。
 * */
template <typename T>
void BlockQueue<T>::notifyWaitPush() {
    notifyPush = true;
    notFullSignal.notify_all();
}

/**
 * 获取queue当前的状态，RUN为正常，WAIT_PUSH为push操作正在被阻塞，WAIT_PULL为pull操作被阻塞。
 * */
template <typename T>
typename BlockQueue<T>::State BlockQueue<T>::getState() {
    if(waitPull)
    {
        return WAIT_PULL;
    }
    else if(waitPush)
    {
        return WAIT_PUSH;
    }
    else
    {
        return RUNNING;
    }

}

/**
 * 获取当前元素个数，有可能大于capacity。
 * */
template <typename T>
int32_t BlockQueue<T>::getSize()
{
    unique_lock<mutex> lock(listMu);
    int32_t result = queue.size();
    lock.unlock();
    return result;
}

#endif //FASTPATHAUDIOECHO_BLOCKQUEUE_H
