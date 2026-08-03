#include "Pipeline.hpp"

#include "Definitions.hpp"
#include "ServiceLocator.hpp"

namespace Crawler
{

bool Preprocessor::p_Sanify(char32_t currentCodePoint) {
    return this->p_Verify(this->p_Surrogates, currentCodePoint) ||
           this->p_Verify(this->p_NonCharacters, currentCodePoint) ||
           this->p_Verify(this->p_Control, currentCodePoint);
}

bool Preprocessor::p_Verify(Sequence::Sequence<char32_t>& s, char32_t currentCodePoint) {
    if (s.Match(currentCodePoint) == Sequence::Result::TRUE) {
        ServiceLocator::GetInstance().GetLogger()->Error("parse error, ignoring code point");
        this->p_In.Drop();
        return true;
    }
    return false;
}

void Preprocessor::Process() {
    for (char32_t c; this->p_In.Peek(&c);) {
        if (this->p_Sanify(c)) continue;

        switch (this->p_CRLF.Match(c)) {
        case Sequence::Result::TRUE:
            this->p_Out.Put(0x000A);
            this->p_In.Drop();
            continue;
        case Sequence::Result::FALSE:
            break;
        case Sequence::Result::PENDING:
            continue;
        }

        switch (this->p_CR.Match(c)) {
        case Sequence::Result::TRUE:
            this->p_Out.Put(0x000A);
            this->p_In.Drop();
            break;
        case Sequence::Result::FALSE:
            this->p_Out.Put(std::move(c));
            this->p_In.Drop();
            break;
        case Sequence::Result::PENDING:
            return;
        }
    }

    if (this->p_In.End()) {
        this->p_Out.SetEndFlag();
    }
}

}