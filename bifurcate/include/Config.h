#ifndef BIFURCATE_CONFIG_H
#define BIFURCATE_CONFIG_H

#if defined(_MSC_VER)
#   pragma warning(disable:4514)
#   pragma warning(disable:4820)
#   define B_WIN32 1

// Currently only supporting SSE
#   define SIMD_SIZE 4
#   define SIMD_ALIGNMENT size_t(16)

#   define RESTRICT __restrict

typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
typedef          __int8  int8_t;
typedef          __int16 int16_t;
typedef          __int32 int32_t;
typedef          __int64 int64_t;

#ifdef _M_IX86
typedef uint32_t uintptr_t;
typedef int32_t intptr_t;
typedef int32_t ssize_t;
#endif

#endif

#if !defined(UNUSED)
#define UNUSED(x) (void)x
#endif

#if !defined(NULL)
#define NULL 0
#endif

#ifdef NDEBUG
#define assert(x) (void)0
#define verify(x) \
    __pragma(warning(push)) \
    __pragma(warning(disable: 4127)) \
    do { \
        if (!(x)) __asm int 3 \
    } while(false) \
    __pragma(warning(pop))
#else
#define assert(x) \
    __pragma(warning(push)) \
    __pragma(warning(disable: 4127)) \
    do { \
        if (!(x)) __asm int 3 \
    } while(false) \
    __pragma(warning(pop))
#define verify(x) assert(x)
#endif

namespace bc 
{
    namespace detail
    {
        template<size_t size>
        struct type_of_size
        {
            typedef char type[size];
        };

        template<typename T, size_t size>
        typename type_of_size<size>::type &sizeof_array_helper(T(&)[size]);

        #define array_sizeof(ARRAY) sizeof(bc::detail::sizeof_array_helper(ARRAY))
    }

    class Uncopyable
    {
        Uncopyable(const Uncopyable &);
        Uncopyable &operator =(const Uncopyable &);
    };
}


#endif
