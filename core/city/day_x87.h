/* Imhotep - integer reproduction of the original's x87 expressions (53-bit precision).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The CRT startup sets the x87 precision to 53 bits (@bin 0x005640EC, docs/formats/map.md) and
 * round-to-nearest-even; __ftol (0x0056133C) truncates toward zero into a 64-bit integer and
 * returns its low 32 bits. Every operation below is exact on integers: a value is kept as an
 * unsigned mantissa of at most 53 bits, a binary exponent and a sign, and each x87 operation is
 * rounded to 53 bits as the FPU does. */
#ifndef IMHOTEP_CITY_DAY_X87_H
#define IMHOTEP_CITY_DAY_X87_H

#include <stdint.h>

typedef struct {
    int neg;
    uint64_t m; /* < 2^53 */
    int e;      /* value = m * 2^e */
} x87_val;

static inline int x87_bits(unsigned __int128 v)
{
    int n = 0;
    while (v) {
        n++;
        v >>= 1;
    }
    return n;
}

/* round v * 2^e (v exact) to 53 bits, nearest-even */
static inline x87_val x87_round(int neg, unsigned __int128 v, int e, int sticky)
{
    x87_val r = {neg, 0, 0};
    int n = x87_bits(v);
    if (n > 53) {
        int s = n - 53;
        unsigned __int128 rem = v & ((((unsigned __int128) 1) << s) - 1);
        unsigned __int128 half = ((unsigned __int128) 1) << (s - 1);
        v >>= s;
        e += s;
        if (rem > half || (rem == half && (sticky || (v & 1)))) {
            v++;
        }
        if (v >> 53) {
            v >>= 1;
            e++;
        }
    }
    r.m = (uint64_t) v;
    r.e = e;
    return r;
}

/* the 53-bit rounding of a / b (fild a; fidiv b) */
static inline x87_val x87_div(int32_t a, int32_t b)
{
    int neg = (a < 0) != (b < 0);
    uint64_t p = a < 0 ? (uint64_t) -(int64_t) a : (uint64_t) a;
    uint64_t q = b < 0 ? (uint64_t) -(int64_t) b : (uint64_t) b;
    if (p == 0) {
        x87_val z = {neg, 0, 0};
        return z;
    }
    /* a quotient of 56..57 bits plus a sticky remainder rounds like the exact value */
    int k = 56 - x87_bits(p) + x87_bits(q);
    unsigned __int128 num = ((unsigned __int128) p) << k;
    unsigned __int128 quo = num / q;
    int sticky = (num % q) != 0;
    return x87_round(neg, quo, -k, sticky);
}

/* the 53-bit rounding of x * c (fimul c) */
static inline x87_val x87_mul_int(x87_val x, int32_t c)
{
    int neg = x.neg != (c < 0);
    uint64_t u = c < 0 ? (uint64_t) -(int64_t) c : (uint64_t) c;
    return x87_round(neg, (unsigned __int128) x.m * u, x.e, 0);
}

/* fild a; fmul dword [float]: the 53-bit rounding of a * f (f a normal float, as its bits) */
static inline x87_val x87_int_mul_float(int32_t a, uint32_t fbits)
{
    int neg = (a < 0) != ((fbits >> 31) != 0);
    uint64_t u = a < 0 ? (uint64_t) -(int64_t) a : (uint64_t) a;
    uint64_t m = (fbits & 0x7FFFFFu) | 0x800000u;
    int e = (int) ((fbits >> 23) & 0xFF) - 127 - 23;
    return x87_round(neg, (unsigned __int128) u * m, e, 0);
}


/* the value of an integer (fild) */
static inline x87_val x87_from_int(int32_t a)
{
    x87_val r = {a < 0, a < 0 ? (uint64_t) -(int64_t) a : (uint64_t) a, 0};
    return r;
}

/* the value of a double given by its bits (fld qword; normal numbers and zero) */
static inline x87_val x87_from_double(uint64_t bits)
{
    x87_val r = {(int) (bits >> 63), 0, 0};
    uint32_t exp = (uint32_t) (bits >> 52) & 0x7FF;
    if (exp == 0) {
        return r; /* zero (no denormals in the game's constants) */
    }
    r.m = (bits & 0xFFFFFFFFFFFFFull) | (1ull << 52);
    r.e = (int) exp - 1075;
    return r;
}

/* the same value with a 53-bit mantissa (zero stays zero) */
static inline x87_val x87_norm(x87_val x)
{
    if (x.m == 0) {
        return x;
    }
    int n = x87_bits(x.m);
    if (n < 53) {
        x.m <<= 53 - n;
        x.e -= 53 - n;
    }
    return x;
}

/* fmul: the 53-bit rounding of a * b */
static inline x87_val x87_mul(x87_val a, x87_val b)
{
    return x87_round(a.neg != b.neg, (unsigned __int128) a.m * b.m, a.e + b.e, 0);
}

/* fdiv: the 53-bit rounding of a / b (b non-zero) */
static inline x87_val x87_divv(x87_val a, x87_val b)
{
    if (a.m == 0) {
        x87_val z = {a.neg != b.neg, 0, 0};
        return z;
    }
    a = x87_norm(a);
    b = x87_norm(b);
    /* 53-bit mantissas: a quotient of 56..57 bits */
    unsigned __int128 num = ((unsigned __int128) a.m) << 56;
    unsigned __int128 quo = num / b.m;
    int sticky = (num % b.m) != 0;
    return x87_round(a.neg != b.neg, quo, a.e - b.e - 56, sticky);
}

/* fadd: the 53-bit rounding of a + b */
static inline x87_val x87_add(x87_val a, x87_val b)
{
    if (a.m == 0) {
        return b;
    }
    if (b.m == 0) {
        return a;
    }
    a = x87_norm(a);
    b = x87_norm(b);
    if (a.e < b.e) {
        x87_val t = a;
        a = b;
        b = t;
    }
    int d = a.e - b.e;
    if (d > 60) {
        return a; /* b is below half an ulp of a */
    }
    unsigned __int128 am = ((unsigned __int128) a.m) << d, bm = b.m;
    if (a.neg == b.neg) {
        return x87_round(a.neg, am + bm, b.e, 0);
    }
    if (am >= bm) {
        return x87_round(a.neg, am - bm, b.e, 0);
    }
    return x87_round(b.neg, bm - am, b.e, 0);
}

/* fsub: a - b */
static inline x87_val x87_sub(x87_val a, x87_val b)
{
    b.neg = !b.neg;
    return x87_add(a, b);
}

/* the sign of a - b: -1, 0 or 1 (fcom) */
static inline int x87_cmp(x87_val a, x87_val b)
{
    x87_val d = x87_sub(a, b);
    return d.m == 0 ? 0 : d.neg ? -1 : 1;
}

/* __ftol: truncation toward zero, low 32 bits of the 64-bit result */
static inline int32_t x87_ftol(x87_val x)
{
    uint64_t v;
    if (x.e >= 0) {
        /* beyond 2^63 fistp stores the indefinite integer 0x8000000000000000: low half 0 */
        if (x.m != 0 && (x.e >= 63 || (x.m >> (63 - x.e)) != 0)) {
            return 0;
        }
        v = x.m << x.e;
    } else {
        v = x.e <= -64 ? 0 : x.m >> -x.e;
    }
    return (int32_t) (uint32_t) (x.neg ? (uint64_t) -(int64_t) v : v);
}

#endif
