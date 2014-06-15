#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "Parser.h"

namespace bx
{
    static const char *ConsumeWhitespace(const char *start, const char *end)
    {
        while ((start < end) && isspace(*start))
            ++start;

        return start;
    }

    static const char *ConsumeComment(const char *start, const char *end)
    {
        assert(start[0] == '/' && start[1] == '*');
        assert(start + 2 < end);
        start += 2;

        while ((start < end) && (start[0] != '*' && start[1] != '/'))
            ++start;

        return start + 2;
    }

    enum TokenType
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
    
    struct Token
    {
        TokenType mTokenType;
        const char *mBegin;
        const char *mEnd;
    };

    static Token NextToken(const char *start, const char * const end)
    {
        Token invalidtoken = { kTokenTypeInvalid, NULL, NULL };

        Token currentToken = {};
#define VALIDATE_BOUNDS(START, END) if (START >= END) { return invalidtoken; } else (void)0

        start = ConsumeWhitespace(start, end);
        VALIDATE_BOUNDS(start, end);

        if (start[0] == '/' && start[1] == '*')
            start = ConsumeWhitespace(ConsumeComment(start, end), end);

        if (start[0] == '/' && start[1] == '/')
        {
            while (*start++ != '\n')
                ;
            start = ConsumeWhitespace(start, end);
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
            const TokenType delimiterTypes[] = { kTokenTypeBlockBegin, kTokenTypeBlockEnd, kTokenTypeTupleBegin, kTokenTypeTupleEnd };
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

    Parser::Parser(const char *begin, const char *end)
        : mStreamBegin(begin),
          mStreamEnd(end),
          mCursor(begin)
    {
    }

    bool Parser::ExpectAndDiscard(const char *string)
    {
        Token t = NextToken(mCursor, mStreamEnd);
        mCursor = t.mEnd;
        return string_equal(t.mBegin, string);
    }

    ParsedInt Parser::ParseInt()
    {
        Token t = NextToken(mCursor, mStreamEnd);
        mCursor = t.mEnd;

        if (t.mTokenType == kTokenTypeNumber)
            return ParsedInt(strtol(t.mBegin, NULL, 0), true);

        return ParsedInt(0, false);
    }

    ParsedString Parser::ParseString()
    {
        Token t = NextToken(mCursor, mStreamEnd);
        mCursor = t.mEnd;

        if (t.mTokenType == kTokenTypeString)
            return ParsedString(t.mBegin, t.mEnd);

        return ParsedString(NULL, NULL);
    }

    ParsedFloat Parser::ParseFloat()
    {
        Token t = NextToken(mCursor, mStreamEnd);
        mCursor = t.mEnd;

        if (t.mTokenType == kTokenTypeNumber)
            return ParsedFloat(static_cast<float>(strtod(t.mBegin, NULL)), true);

        return ParsedFloat(0, false);
    }
}
