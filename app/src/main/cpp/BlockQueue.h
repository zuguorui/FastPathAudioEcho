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

template <typename T>
class BlockQueue {
public:

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

template <typename T>
BlockQueue<T>::BlockQueue(int32_t capacity): capacity{capacity}, notifyPush{false}, notifyPull{false} {

}

template <typename T>
BlockQueue<T>::~BlockQueue<T>() {

}

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

template <typename T>
void BlockQueue<T>::notifyWaitPull() {
    notifyPull = true;
    notEmptySignal.notify_all();
}

template <typename T>
void BlockQueue<T>::notifyWaitPush() {
    notifyPush = true;
    notFullSignal.notify_all();
}

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

template <typename T>
int32_t BlockQueue<T>::getSize()
{
    unique_lock<mutex> lock(listMu);
    int32_t result = queue.size();
    lock.unlock();
    return result;
}

#endif //FASTPATHAUDIOECHO_BLOCKQUEUE_H
