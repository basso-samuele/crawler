#pragma once

#include "Stream.hpp"

namespace Crawler
{

template <typename T>
class TransactionalStream : public Stream<T>
{
private:
    bool p_End;
    bool p_Bad;

public:
    TransactionalStream()
    : p_End(false), p_Bad(false) { }
    virtual ~TransactionalStream() = default;

    TransactionalStream(const TransactionalStream&) = delete;
    TransactionalStream& operator=(const TransactionalStream&) = delete;

    TransactionalStream(TransactionalStream&&) = delete;
    TransactionalStream& operator=(TransactionalStream&&) = delete;

    bool Put(T&& value) {
        this->p_Base.push_back(std::move(value));
        return true;
    }

    void SetEndFlag() {
        this->p_End = true;
    }

    virtual bool End() const {
        return this->p_End;
    }

    virtual bool Bad() const {
        return this->p_Bad;
    }
};

}