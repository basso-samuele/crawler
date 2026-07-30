#pragma once

#include "Stream.hpp"

namespace Crawler
{

template <typename T>
class TransactionalStream : public Stream<T>
{
public:
    TransactionalStream(const size_t maskBitOffset)
    : Stream<T>(maskBitOffset) { }
    virtual ~TransactionalStream() = default;

    TransactionalStream(const TransactionalStream&) = delete;
    TransactionalStream& operator=(const TransactionalStream&) = delete;

    TransactionalStream(TransactionalStream&&) = delete;
    TransactionalStream& operator=(TransactionalStream&&) = delete;

    void Put(T&& value) {
        size_t fullBufferTailOffset = (this->p_HeadOffset - 1) & this->p_Mask;
        bool isBufferFull = (this->p_TailOffset == fullBufferTailOffset);
        if (isBufferFull) {
            return;
        }

        this->p_Base[this->p_TailOffset] = value;
        this->p_TailOffset = (this->p_TailOffset + 1) & this->p_Mask;
    }

    virtual bool End() const {
        return false;
    }

    virtual bool Bad() const {
        return false;
    }
};

}