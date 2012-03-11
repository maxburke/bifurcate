#ifndef BIFURCATE_PARSER_H
#define BIFURCATE_PARSER_H

#include "Config.h"

namespace bx
{
    class ParsedString
    {
    public:
        ParsedString(const char *begin, const char *end)
            : mBegin(begin + 1),
              mEnd(end - 1)
        {
            assert(*begin == '"' && *(end - 1) == '"');
        }

        bool Valid() const
        {
            return mBegin != NULL && mEnd != NULL;
        }

        size_t length() const
        {
            assert(mEnd >= mBegin);
            return static_cast<size_t>(mEnd - mBegin);
        }

        const char *mBegin;
        const char *mEnd;
    };

    template<typename T>
    class ParsedNumber
    {
        T mValue;
        bool mValid;

    public:
        ParsedNumber(T value, bool valid)
            : mValue(value),
              mValid(valid)
        {}

        operator T() const
        {
            return mValue;
        }

        bool operator==(T rhs) const
        {
            if (!mValid) return false;
            return mValue == rhs;
        }

        bool operator!=(T rhs) const
        {
            return !(*this == rhs);
        }

        bool Valid() const
        {
            return mValid;
        }

        T Value() const
        {
            return mValue;
        }
    };

    typedef ParsedNumber<int> ParsedInt;
    typedef ParsedNumber<float> ParsedFloat;

    class Parser
    {
        const char *mStreamBegin;
        const char *mStreamEnd;
        const char *mCursor;

    public:
        Parser(const char *begin, const char *end);
        ~Parser() {}

        bool ExpectAndDiscard(const char *token);
        ParsedInt ParseInt();
        ParsedString ParseString();
        ParsedFloat ParseFloat();
    };
}

#endif
