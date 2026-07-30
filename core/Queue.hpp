#pragma once

#include <queue>
#include <mutex>
#include <utility>

namespace Crawler
{

template <typename T>
class Sink
{
public:
    virtual ~Sink() = default;
    virtual void Push(T&& source) = 0;
};

template <typename T>
class Source
{
public:
    virtual ~Source() = default;
    virtual bool Pop(T& destination) = 0;
};

template <typename T>
class Queue : public Sink<T>, public Source<T>
{
private:
    std::queue<T> p_Queue;

public:
    void Push(T&& source) override {
        this->p_Queue.push(std::move(source));
    }

    bool Pop(T& destination) override {
        if (!this->p_Queue.empty()) {
            destination = std::move(this->p_Queue.front());
            this->p_Queue.pop();
            return true;
        } else {
            return false;
        }
    }
};

}