#ifndef BIFURCATE_SSEMATH_H
#define BIFURCATE_SSEMATH_H

#include <emmintrin.h>

union ConversionUnion
{
    unsigned int mUnsigned;
    int mSigned;
    float mFloat;
};

__forceinline __m128 _mm_dotps_ss(__m128 a, __m128 b)
{
    // NOTE NOTE NOTE
    // This function only returns a valid dot product value in
    // the low order element (word 0/x)! If you need the dot
    // product splatted, you need to shuffle yourself! 
    // (ie, _mm_shuffle_ps(x, x, 0); )
    __m128 x0 = _mm_mul_ps(a, b);
    __m128 x1 = _mm_movehl_ps(x0, x0);
    __m128 x2 = _mm_add_ps(x1, x0);
    __m128 x3 = _mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(x2), 4));
    __m128 x4 = _mm_add_ps(x2, x3);
    return x4;
}

__forceinline __m128 _mm_dot_ps(__m128 a, __m128 b)
{
    __m128 dot = _mm_dotps_ss(a, b);
    return _mm_shuffle_ps(dot, dot, 0);
}

__forceinline __m128 _mm_abs_ps(__m128 a)
{
    ConversionUnion conversionUnion;
    conversionUnion.mUnsigned = 0x7ffffffful;
    const __m128 absoluteValueMask = _mm_load1_ps(&conversionUnion.mFloat);
    return _mm_and_ps(a, absoluteValueMask);
}

__forceinline __m128 _mm_neg_ps(__m128 a)
{
    ConversionUnion conversionUnion;
    conversionUnion.mUnsigned = 0x80000000ul;
    const __m128 signBit = _mm_load1_ps(&conversionUnion.mFloat);
    return _mm_xor_ps(a, signBit);
}

__forceinline __m128 _mm_madd_ps(__m128 a, __m128 b, __m128 c)
{
    return _mm_add_ps(c, _mm_mul_ps(a, b));
}

__forceinline __m128 _mm_sel_ps(__m128 f, __m128 t, __m128 mask)
{
    return _mm_or_ps(_mm_and_ps(mask, t), _mm_andnot_ps(mask, f));
}

__forceinline __m128 _mm_nmsub_ps(__m128 a, __m128 b, __m128 c)
{
    return _mm_sub_ps(c, _mm_mul_ps(a, b));
}

__forceinline __m128 _mm_acos_ps(__m128 a)
{
    // Many thanks to Bullet Physics for this.
    const __m128 abs = _mm_abs_ps(a);
    const __m128 mask = _mm_cmplt_ps(a, _mm_setzero_ps());
    const __m128 t1 = _mm_sqrt_ps(_mm_sub_ps(_mm_set1_ps(1.0f), abs));

    const __m128 abs2 = _mm_mul_ps(abs, abs);
    const __m128 abs4 = _mm_mul_ps(abs2, abs2);
    const __m128 hi = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_set1_ps(-0.0012624911f),
        abs, _mm_set1_ps(0.0066700901f)),
        abs, _mm_set1_ps(-0.0170881256f)),
        abs, _mm_set1_ps(0.0308918810f));
    const __m128 lo = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_set1_ps(-0.0501743046f),
        abs, _mm_set1_ps(0.0889789874f)),
        abs, _mm_set1_ps(-0.2145988016f)),
        abs, _mm_set1_ps(1.5707963050f));

    const __m128 result = _mm_madd_ps(hi, abs4, lo);
    return _mm_sel_ps(_mm_mul_ps(t1, result), _mm_nmsub_ps(t1, result, _mm_set1_ps(3.1415926535898f)), mask);
}

__forceinline __m128 _mm_sin_ps(__m128 x)
{
    const float CC0 = -0.0013602249f;
    const float CC1 = 0.0416566950f;
    const float CC2 = -0.4999990225f;
    const float SC0 = -0.0001950727f;
    const float SC1 = 0.0083320758f;
    const float SC2 = -0.1666665247f;
    const float KC1 = 1.57079625129f;
    const float KC2 = 7.54978995489e-8f;

    const __m128i one = _mm_set1_epi32(1);
    const __m128i two = _mm_set1_epi32(2);
    const __m128i zero = _mm_setzero_si128();

    const __m128 x1 = _mm_mul_ps(x, _mm_set1_ps(0.63661977236f));
    const __m128i q = _mm_cvtps_epi32(x1);
    const __m128i offsetSin = _mm_and_si128(q, _mm_set1_epi32(3));
    const __m128 qf = _mm_cvtepi32_ps(q);
    const __m128 x11 = _mm_nmsub_ps(qf, _mm_set1_ps(KC2), _mm_nmsub_ps(qf, _mm_set1_ps(KC1), x));
    const __m128 x12 = _mm_mul_ps(x11, x11);
    const __m128 x13 = _mm_mul_ps(x12, x11);

    const __m128 cx = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_set1_ps(CC0), x12, _mm_set1_ps(CC1)), x12, _mm_set1_ps(CC2)), x12, _mm_set1_ps(1.0f));
    const __m128 sx = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_set1_ps(SC0), x12, _mm_set1_ps(SC1)), x12, _mm_set1_ps(SC2)), x13, x11);

    const __m128i sinMask = _mm_cmpeq_epi32(_mm_and_si128(offsetSin, one), zero);
    const __m128 s1 = _mm_sel_ps(cx, sx, _mm_castsi128_ps(sinMask));

    const __m128i sinMask2 = _mm_cmpeq_epi32(_mm_and_si128(offsetSin, two), zero);

    const __m128 highBitMask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
    return _mm_sel_ps(_mm_xor_ps(highBitMask, s1), s1, _mm_castsi128_ps(sinMask2));
}

__forceinline void _mm_sincos_ps(__m128 * RESTRICT sin, __m128 * RESTRICT cos, __m128 x)
{
    const float CC0 = -0.0013602249f;
    const float CC1 = 0.0416566950f;
    const float CC2 = -0.4999990225f;
    const float SC0 = -0.0001950727f;
    const float SC1 = 0.0083320758f;
    const float SC2 = -0.1666665247f;
    const float KC1 = 1.57079625129f;
    const float KC2 = 7.54978995489e-8f;

    const __m128i one = _mm_set1_epi32(1);
    const __m128i two = _mm_set1_epi32(2);
    const __m128i zero = _mm_setzero_si128();

    const __m128 x1 = _mm_mul_ps(x, _mm_set1_ps(0.63661977236f));
    const __m128i q = _mm_cvtps_epi32(x1);
    const __m128i offsetSin = _mm_and_si128(q, _mm_set1_epi32(3));
    const __m128i temp = _mm_add_epi32(one, offsetSin);
    const __m128i offsetCos = temp;
    const __m128 qf = _mm_cvtepi32_ps(q);
    const __m128 x11 = _mm_nmsub_ps(qf, _mm_set1_ps(KC2), _mm_nmsub_ps(qf, _mm_set1_ps(KC1), x));
    const __m128 x12 = _mm_mul_ps(x11, x11);
    const __m128 x13 = _mm_mul_ps(x12, x11);

    const __m128 cx = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_set1_ps(CC0), x12, _mm_set1_ps(CC1)), x12, _mm_set1_ps(CC2)), x12, _mm_set1_ps(1.0f));
    const __m128 sx = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_set1_ps(SC0), x12, _mm_set1_ps(SC1)), x12, _mm_set1_ps(SC2)), x13, x11);

    const __m128i sinMask = _mm_cmpeq_epi32(_mm_and_si128(offsetSin, one), zero);
    const __m128i cosMask = _mm_cmpeq_epi32(_mm_and_si128(offsetCos, one), zero);
    const __m128 s1 = _mm_sel_ps(cx, sx, _mm_castsi128_ps(sinMask));
    const __m128 c1 = _mm_sel_ps(cx, sx, _mm_castsi128_ps(cosMask));

    const __m128i sinMask2 = _mm_cmpeq_epi32(_mm_and_si128(offsetSin, two), zero);
    const __m128i cosMask2 = _mm_cmpeq_epi32(_mm_and_si128(offsetCos, two), zero);

    const __m128 highBitMask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
    const __m128 s = _mm_sel_ps(_mm_xor_ps(highBitMask, s1), s1, _mm_castsi128_ps(sinMask2));
    const __m128 c = _mm_sel_ps(_mm_xor_ps(highBitMask, c1), c1, _mm_castsi128_ps(cosMask2));
    *sin = s;
    *cos = c;
}

__forceinline __m128 _mm_clamp_ps(__m128 val, __m128 min, __m128 max)
{
    return _mm_max_ps(_mm_min_ps(val, max), min);
}

#endif