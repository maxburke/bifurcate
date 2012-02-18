#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "Parser.h"

namespace bx
{
    static const char *consume_whitespace(const char *start, const char *end)
    {
        while ((start < end) && isspace(*start))
            ++start;

        return start;
    }

    static const char *consume_comment(const char *start, const char *end)
    {
        assert(start[0] == '/' && start[1] == '*');
        assert(start + 2 < end);
        start += 2;

        while ((start < end) && (start[0] != '*' && start[1] != '/'))
            ++start;

        return start + 2;
    }

    enum token_type
    {
        kTokenTypeInvalid,
        kTokenTypeSymbol,
        kTokenTypeBlockBegin,
        kTokenTypeBlockEnd,
        kTokenTypeTupleBegin,
        kTokenTypeTupleEnd,
        kTokenTypeString,
        kTokenTypeNumber
    };
    
    struct token
    {
        token_type mTokenType;
        const char *mBegin;
        const char *mEnd;
    };

    static token next_token(const char *start, const char * const end)
    {
        token invalidtoken = { kTokenTypeInvalid, NULL, NULL };

        token currentToken = {};
#define VALIDATE_BOUNDS(START, END) if (START >= END) { return invalidtoken; } else (void)0

        start = consume_whitespace(start, end);
        VALIDATE_BOUNDS(start, end);

        if (start[0] == '/' && start[1] == '*')
            start = consume_whitespace(consume_comment(start, end), end);

        if (start[0] == '/' && start[1] == '/')
        {
            while (*start++ != '\n')
                ;
            start = consume_whitespace(start, end);
        }
        VALIDATE_BOUNDS(start, end);

        const char *ptr = start;
        currentToken.mBegin = start;
        char startChar = *start;

        if (startChar == '"')
        {
            ++ptr;
            while (*ptr != '"')
                ++ptr;
            VALIDATE_BOUNDS(ptr, end);

            currentToken.mEnd = ptr + 1;
            currentToken.mTokenType = kTokenTypeString;
            return currentToken;
        }

        const char *blockDelimiters = "{}()";
        if (const char *delimiterPtr = strchr(blockDelimiters, startChar))
        {
            const token_type delimiterTypes[] = { kTokenTypeBlockBegin, kTokenTypeBlockEnd, kTokenTypeTupleBegin, kTokenTypeTupleEnd };
            int delimiterIdx = delimiterPtr - blockDelimiters;
            currentToken.mEnd = start + 1;
            currentToken.mTokenType = delimiterTypes[delimiterIdx];
            return currentToken;
        }

        if ((startChar >= '0' && startChar <= '9') || startChar == '-')
        {
            while (!isspace(*ptr))
                ++ptr;
            VALIDATE_BOUNDS(ptr, end);

            currentToken.mEnd = ptr;
            currentToken.mTokenType = kTokenTypeNumber;
            return currentToken;
        }

        while (!isspace(*ptr))
            ++ptr;
        currentToken.mEnd = ptr;

        VALIDATE_BOUNDS(ptr, end);

        currentToken.mTokenType = kTokenTypeSymbol;
        return currentToken;

#undef VALIDATE_BOUNDS
    }
    
    static bool string_equal(const char *lhs, const char *rhs)
    {
        while (*lhs == *rhs)
            ++lhs, ++rhs;
        return *lhs == 0 || *rhs == 0;
    }

    parser::parser(const char *begin, const char *end)
        : mStreamBegin(begin),
          mStreamEnd(end),
          mCursor(begin)
    {
    }

    bool parser::expect_and_discard(const char *string)
    {
        token t = next_token(mCursor, mStreamEnd);
        mCursor = t.mEnd;
        return string_equal(t.mBegin, string);
    }

    parsed_int parser::parse_int()
    {
        token t = next_token(mCursor, mStreamEnd);
        mCursor = t.mEnd;

        if (t.mTokenType == kTokenTypeNumber)
            return parsed_int(strtol(t.mBegin, NULL, 0), true);

        return parsed_int(0, false);
    }

    parsed_string parser::parse_string()
    {
        token t = next_token(mCursor, mStreamEnd);
        mCursor = t.mEnd;

        if (t.mTokenType == kTokenTypeString)
            return parsed_string(t.mBegin, t.mEnd);

        return parsed_string(NULL, NULL);
    }

    parsed_float parser::parse_float()
    {
        token t = next_token(mCursor, mStreamEnd);
        mCursor = t.mEnd;

        if (t.mTokenType == kTokenTypeNumber)
            return parsed_float(static_cast<float>(strtod(t.mBegin, NULL)), true);

        return parsed_float(0, false);
    }
}
