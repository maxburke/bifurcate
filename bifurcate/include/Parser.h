#ifndef BIFURCATE_PARSER_H
#define BIFURCATE_PARSER_H

#include "Config.h"

namespace bx
{
    class parsed_string
    {
    public:
        parsed_string(const char *begin, const char *end)
            : mBegin(begin + 1),
              mEnd(end + 1)
        {
            assert(*begin == '"' && *(end - 1)== '"');
        }

        bool valid() const
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
    class parsed_number
    {
        T mValue;
        bool mValid;

    public:
        parsed_number(T value, bool valid)
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

        bool valid() const
        {
            return mValid;
        }
    };

    typedef parsed_number<int> parsed_int;
    typedef parsed_number<float> parsed_float;

    class parser
    {
        const char *mStreamBegin;
        const char *mStreamEnd;
        const char *mCursor;

    public:
        parser(const char *begin, const char *end);
        ~parser() {}

        bool expect_and_discard(const char *token);
        parsed_int parse_int();
        parsed_string parse_string();
        parsed_float parse_float();
    };
}

#endif
