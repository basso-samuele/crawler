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
    TransactionalStream(const size_t maskBitOffset)
    : Stream<T>(maskBitOffset), p_End(false), p_Bad(false) { }
    virtual ~TransactionalStream() = default;

    TransactionalStream(const TransactionalStream&) = delete;
    TransactionalStream& operator=(const TransactionalStream&) = delete;

    TransactionalStream(TransactionalStream&&) = delete;
    TransactionalStream& operator=(TransactionalStream&&) = delete;

    bool Put(T&& value) {
        size_t fullBufferTailOffset = (this->p_HeadOffset - 1) & this->p_Mask;
        bool isBufferFull = (this->p_TailOffset == fullBufferTailOffset);
        if (isBufferFull) {
            return false;
        }

        this->p_Base[this->p_TailOffset] = value;
        this->p_TailOffset = (this->p_TailOffset + 1) & this->p_Mask;

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