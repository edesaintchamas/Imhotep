/* Imhotep - the direction from one tile to another, computed by the original in x87 doubles.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x0051CAE0 (called by the missile creation 0x004B45C0) computes, in x87 code:
 *   dx = a3 - a1, dy = a2 - a4                         (int32, fild)
 *   r  = fsqrt(dy*dy + dx*dx); r == 0: return al = 0
 *   q  = dy / r
 *   t  = asin(q)  (CRT 0x00562D00: fpatan(q, fsqrt((1 + q) * (1 - q))); q = +-1: +-pi/2 as an
 *                  80-bit constant at 0x006050AA)
 *   dx <= 0: t = 3.1429 - t                            (0x00570148)
 *   t -= 5.500075                                      (0x00570140)
 *   while t < 0: t += 6.2858                           (0x00570138)
 *   return __ftol((6.2858 - t) * 5.090839670368132)    (0x00570138, 0x00570130; 0x0056133C)
 * with the x87 precision control at 53 bits (CRT startup 0x005640EC; 0x00562D00 keeps it) and
 * round-to-nearest-even: every fadd/fsub/fmul/fdiv/fsqrt result is the 53-bit rounding of the exact
 * result. Each one is reproduced below exactly on integers (a value is m * 2^e).
 *
 * fpatan is the one transcendental step; it ignores the precision control and returns a 64-bit
 * extended result on a real x87. Here atan(q / s) is computed by CORDIC (vectoring mode) in signed
 * Q124 fixed point and rounded to 64 bits, then carried exactly into the next 53-bit fsub/fsubr.
 * Error bound: each of the 124 iterations loses at most 2^-124 in x and y (arithmetic shifts),
 * which turns the vector by at most 2^-123.5 since |(x, y)| >= |(q, s)| ~ 1; each table entry is
 * within 2^-125; the residual angle after the last iteration is below 2^-122: total below 2^-115
 * (measured: below 2^-120). The 64-bit ulp of the angle is >= 2^-90 (|angle| >= 2^-27 for
 * |dy| < 2^26), so the rounding is decided correctly unless the angle lies within 2^-115 of a
 * 64-bit rounding midpoint; over the game's domain (|dx|, |dy| <= 227) the closest is 2^-83.5.
 * The function oracle (Unicorn, QEMU 5 x87 helpers) computes fpatan as the host's double atan2
 * instead (not correctly rounded: 0.56 ulp off for dx = -227, dy = -214). On that domain every
 * final result is the same with the 64-bit, the correctly rounded 53-bit and the oracle's atan2
 * (exact rational model, offline), and every input was checked against the oracle: 207025
 * inputs, 0 differences. */
#include "sim/x87_angle.h"

#include "state/layout.h"
#include "state/ported.h"

typedef unsigned __int128 u128;
typedef __int128 s128;

typedef struct {
    int neg;
    uint64_t m; /* value = (-1)^neg * m * 2^e */
    int e;
} xv;

static int bits128(u128 v)
{
    int n = 0;
    while (v) {
        n++;
        v >>= 1;
    }
    return n;
}

/* the p-bit nearest-even rounding (p <= 64) of (-1)^neg * (v + sticky fraction) * 2^e */
static xv xround_p(int neg, u128 v, int e, int sticky, int p)
{
    xv r = {neg, 0, 0};
    if (sticky && bits128(v) < p + 3) {
        int k = p + 3 - bits128(v); /* guard bits below the rounding position for the sticky part */
        v <<= k;
        e -= k;
    }
    int n = bits128(v);
    if (n > p) {
        int s = n - p;
        u128 rem = v & ((((u128) 1) << s) - 1);
        u128 half = ((u128) 1) << (s - 1);
        v >>= s;
        e += s;
        if (rem > half || (rem == half && (sticky || (v & 1)))) {
            v++;
        }
        if (v >> p) {
            v >>= 1;
            e++;
        }
    }
    r.m = (uint64_t) v;
    r.e = e;
    return r;
}

/* the 53-bit rounding (precision control 53 bits) */
static xv xround(int neg, u128 v, int e, int sticky)
{
    return xround_p(neg, v, e, sticky, 53);
}

static xv xdouble(uint64_t bits)
{
    /* a normal double from its bits */
    xv r = {(int) (bits >> 63), (bits & 0x000FFFFFFFFFFFFFull) | 0x0010000000000000ull,
            (int) ((bits >> 52) & 0x7FF) - 1075};
    return r;
}

/* fadd: the 53-bit rounding of a + b (operands of up to 64 bits) */
static xv xadd(xv a, xv b)
{
    if (b.m == 0) {
        return xround(a.neg, a.m, a.e, 0);
    }
    if (a.m == 0) {
        return xround(b.neg, b.m, b.e, 0);
    }
    if (a.e < b.e) {
        xv t = a;
        a = b;
        b = t;
    }
    int d = a.e - b.e;
    u128 A, B;
    int e;
    if (d <= 62) {
        A = ((u128) a.m) << d;
        B = b.m;
        e = b.e;
    } else {
        /* b is below a's last bit by more than 62 bits: shifted with its lost bits jammed into the
         * lowest bit, far below the rounding position (>= 62 guard bits) */
        int sh = d - 62;
        A = ((u128) a.m) << 62;
        B = sh >= 64 ? 0 : b.m >> sh;
        if (sh >= 64 || (b.m & ((1ull << sh) - 1))) {
            B |= 1;
        }
        e = a.e - 62;
    }
    if (a.neg == b.neg) {
        return xround(a.neg, A + B, e, 0);
    }
    if (A >= B) {
        return xround(a.neg, A - B, e, 0);
    }
    return xround(b.neg, B - A, e, 0);
}

static xv xneg(xv a)
{
    a.neg = !a.neg;
    return a;
}

/* fmul: the 53-bit rounding of a * b (operands of up to 64 bits) */
static xv xmul(xv a, xv b)
{
    return xround(a.neg != b.neg, (u128) a.m * b.m, a.e + b.e, 0);
}

static u128 isqrt128(u128 n, int *exact)
{
    u128 res = 0;
    u128 bit = ((u128) 1) << 126;
    while (bit > n) {
        bit >>= 2;
    }
    while (bit) {
        if (n >= res + bit) {
            n -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    *exact = n == 0;
    return res;
}

/* fsqrt: the 53-bit rounding of sqrt(a), a > 0 */
static xv xsqrt(xv a)
{
    u128 m = a.m;
    int e = a.e;
    if (e & 1) {
        m <<= 1;
        e -= 1;
    }
    int k2 = 112 - bits128(m); /* scale to 112..113 bits: a root of 56..57 bits */
    if (k2 & 1) {
        k2++;
    }
    if (k2 > 0) {
        m <<= k2;
    } else {
        k2 = 0;
    }
    int exact;
    u128 root = isqrt128(m, &exact);
    return xround(0, root, (e - k2) / 2, !exact);
}

/* fild n; fdiv d: the 53-bit rounding of n / d, 0 < |n| < 2^26, d > 0 */
static xv xdiv_int(int64_t n, xv d)
{
    uint64_t u = n < 0 ? (uint64_t) -n : (uint64_t) n;
    int k = 110 - bits128(u);
    u128 num = ((u128) u) << k;
    u128 quo = num / d.m;
    return xround(n < 0, quo, -k - d.e, (num % d.m) != 0);
}

/* sign of |a| - 1 */
static int xcmp_one(xv a)
{
    if (a.m == 0) {
        return -1;
    }
    int top = bits128(a.m) - 1 + a.e; /* |a| in [2^top, 2^(top+1)) */
    if (top != 0) {
        return top > 0 ? 1 : -1;
    }
    return (a.m & (a.m - 1)) ? 1 : 0;
}

/* atan(2^-i) in Q124, rounded; for i >= 42 it is 2^-i to within 2^-126 (2^-3i / 3) */
static const uint64_t ATAN_TABLE[42][2] = {
    {0x0C90FDAA22168C23ULL, 0x4C4C6628B80DC1CDULL}, {0x076B19C1586ED3DAULL, 0x2B7F222F65E1D468ULL},
    {0x03EB6EBF25901BACULL, 0x55B71E7BD7DE8860ULL}, {0x01FD5BA9AAC2F6DCULL, 0x65912F313E7D111EULL},
    {0x00FFAADDB967EF4EULL, 0x36CB2792DC0E2E0DULL}, {0x007FF556EEA5D892ULL, 0xA13BCEBBB6ED4631ULL},
    {0x003FFEAAB776E535ULL, 0x6EF9E31590057DD8ULL}, {0x001FFFD555BBBA97ULL, 0x2D00C46A3F77CC16ULL},
    {0x000FFFFAAAADDDDBULL, 0x94BB12AFB6B6D4F8ULL}, {0x0007FFFF55556EEEULL, 0xEA5CA6ADEAB02252ULL},
    {0x0003FFFFEAAAAB77ULL, 0x776E52E5A019FBCFULL}, {0x0001FFFFFD55555BULL, 0xBBBBA97297625625ULL},
    {0x0000FFFFFFAAAAAAULL, 0xDDDDDDB94B94D5BDULL}, {0x00007FFFFFF55555ULL, 0x56EEEEEEA5CA5CB4ULL},
    {0x00003FFFFFFEAAAAULL, 0xAAB7777776E52E53ULL}, {0x00001FFFFFFFD555ULL, 0x5555BBBBBBBA9729ULL},
    {0x00000FFFFFFFFAAAULL, 0xAAAAADDDDDDDDB95ULL}, {0x000007FFFFFFFF55ULL, 0x5555556EEEEEEEEAULL},
    {0x000003FFFFFFFFEAULL, 0xAAAAAAAB77777777ULL}, {0x000001FFFFFFFFFDULL, 0x555555555BBBBBBCULL},
    {0x000000FFFFFFFFFFULL, 0xAAAAAAAAAADDDDDEULL}, {0x0000007FFFFFFFFFULL, 0xF55555555556EEEFULL},
    {0x0000003FFFFFFFFFULL, 0xFEAAAAAAAAAAB777ULL}, {0x0000001FFFFFFFFFULL, 0xFFD55555555555BCULL},
    {0x0000000FFFFFFFFFULL, 0xFFFAAAAAAAAAAAAEULL}, {0x00000007FFFFFFFFULL, 0xFFFF555555555555ULL},
    {0x00000003FFFFFFFFULL, 0xFFFFEAAAAAAAAAABULL}, {0x00000001FFFFFFFFULL, 0xFFFFFD5555555555ULL},
    {0x00000000FFFFFFFFULL, 0xFFFFFFAAAAAAAAABULL}, {0x000000007FFFFFFFULL, 0xFFFFFFF555555555ULL},
    {0x000000003FFFFFFFULL, 0xFFFFFFFEAAAAAAABULL}, {0x000000001FFFFFFFULL, 0xFFFFFFFFD5555555ULL},
    {0x000000000FFFFFFFULL, 0xFFFFFFFFFAAAAAABULL}, {0x0000000007FFFFFFULL, 0xFFFFFFFFFF555555ULL},
    {0x0000000003FFFFFFULL, 0xFFFFFFFFFFEAAAABULL}, {0x0000000001FFFFFFULL, 0xFFFFFFFFFFFD5555ULL},
    {0x0000000000FFFFFFULL, 0xFFFFFFFFFFFFAAABULL}, {0x00000000007FFFFFULL, 0xFFFFFFFFFFFFF555ULL},
    {0x00000000003FFFFFULL, 0xFFFFFFFFFFFFFEABULL}, {0x00000000001FFFFFULL, 0xFFFFFFFFFFFFFFD5ULL},
    {0x00000000000FFFFFULL, 0xFFFFFFFFFFFFFFFBULL}, {0x000000000007FFFFULL, 0xFFFFFFFFFFFFFFFFULL},
};

#define FRAC 124

static s128 to_q124(xv a)
{
    u128 m = a.m;
    int sh = a.e + FRAC;
    m = sh >= 0 ? m << sh : m >> -sh; /* values in use: |a| <= 1 and a.e >= -124 */
    return a.neg ? -(s128) m : (s128) m;
}

/* atan(y / x) in Q124 within 2^-115 (CORDIC, vectoring mode), x > 0 */
static s128 cordic_atan(xv y, xv x)
{
    s128 cx = to_q124(x), cy = to_q124(y), z = 0;
    for (int i = 0; i < FRAC; i++) {
        s128 t = i < 42 ? (s128) (((u128) ATAN_TABLE[i][0] << 64) | ATAN_TABLE[i][1])
                        : (s128) (((u128) 1) << (FRAC - i));
        s128 xs = cx >> i, ys = cy >> i;
        if (cy >= 0) {
            cx += ys;
            cy -= xs;
            z += t;
        } else {
            cx -= ys;
            cy += xs;
            z -= t;
        }
    }
    return z;
}

/* fpatan: atan(y / x), x > 0, y != 0, rounded to the x87's 64-bit extended precision (fpatan
 * ignores the precision control) */
static xv xatan(xv y, xv x)
{
    s128 z = cordic_atan(y, x);
    return xround_p(z < 0, z < 0 ? (u128) -z : (u128) z, -FRAC, 0, 64);
}

/* @bin 0x00562D00 CRT asin(q) for |q| <= 1: fpatan(q, fsqrt((1 + q) * (1 - q))), each step at
 * 53 bits; |q| == 1: +-pi/2 as the 80-bit constant at 0x006050AA (mantissa 0xC90FDAA22168C235,
 * exponent 0x3FFF), loaded without rounding */
static xv crt_asin(xv q)
{
    if (q.m == 0) {
        xv z = {q.neg, 0, 0}; /* fpatan(+-0, 1) = +-0 */
        return z;
    }
    if (xcmp_one(q) == 0) {
        xv h = {q.neg, 0xC90FDAA22168C235ull, -63};
        return h;
    }
    xv one = {0, 1, 0};
    xv t1 = xadd(one, q);
    xv t2 = xadd(one, xneg(q));
    xv s = xsqrt(xmul(t1, t2));
    return xatan(q, s);
}

int32_t x87_direction(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int *ok)
{
    /* @bin 0x0051CAE0 sub eax, ecx: int32 differences */
    int32_t dx = (int32_t) ((uint32_t) x2 - (uint32_t) x1);
    int32_t dy = (int32_t) ((uint32_t) y1 - (uint32_t) y2);
    const int32_t lim = 1 << 26; /* squares and their sum exact in 53 bits, |q| <= 1 */
    *ok = dx > -lim && dx < lim && dy > -lim && dy < lim;
    if (!*ok) {
        return 0;
    }
    /* @bin 0x0051CB0B dy*dy + dx*dx, exact here; fsqrt; == 0.0 (0x0056F5A0): al = 0 */
    int64_t sum = (int64_t) dx * dx + (int64_t) dy * dy;
    if (sum == 0) {
        return 0;
    }
    xv sq = {0, (uint64_t) sum, 0};
    xv r = xsqrt(sq);
    xv q;
    if (dy == 0) {
        q.neg = 0;
        q.m = 0;
        q.e = 0;
    } else {
        q = xdiv_int(dy, r); /* @bin 0x0051CB3A fld dy; fdiv r */
    }
    xv t = crt_asin(q);
    /* @bin 0x0051CB40 test ah, 0x41 on fcomp dx, 0.0: dx <= 0 -> 3.1429 - t (0x00570148) */
    if (dx <= 0) {
        t = xadd(xdouble(0x400924A8C154C986ull), xneg(t));
    }
    /* @bin 0x0051CB57 t - 5.500075 (0x00570140); while t < 0: t + 6.2858 (0x00570138) */
    xv full = xdouble(0x401924A8C154C986ull);
    t = xadd(t, xneg(xdouble(0x40160013A92A3055ull)));
    while (t.neg && t.m != 0) {
        t = xadd(t, full);
    }
    /* @bin 0x0051CB7D (6.2858 - t) * 5.090839670368132 (0x00570130); __ftol 0x0056133C truncates */
    xv v = xmul(xadd(full, xneg(t)), xdouble(0x40145D051315A466ull));
    int64_t n = v.e >= 0 ? (int64_t) (v.m << v.e) : v.e <= -64 ? 0 : (int64_t) (v.m >> -v.e);
    return (int32_t) (v.neg ? -n : n);
}

int32_t x87_direction_port(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    int ok;
    int32_t r = x87_direction(x1, y1, x2, y2, &ok);
    if (!ok) {
        state_unported(st, FN_X87_DIRECTION);
        return 0;
    }
    return r;
}

static int32_t run_x87_direction(game_state *st, const int32_t *a)
{
    return x87_direction_port(st, a[0], a[1], a[2], a[3]);
}

const ported_function x87_ports[] = {
    /* al only on the null-vector path (ah keeps the FPU status): the caller 0x004B45C0 stores al */
    {0x0051CAE0u, "x87_direction", 4, 8, 0, run_x87_direction, PORT_PARTIAL},
    /* the argument and result are in st(0): no stack entry point, inlined in its callers */
    {0x00562D00u, "crt_asin (crt_asin in sim/x87_angle.c)", 0, 0, 0, NULL, 0},
    /* its parts: the exponent test of the argument (eax) and the body (control word 0x27F kept,
     * fpatan, |q| == 1) */
    {0x005675E8u, "crt_asin exponent test (crt_asin)", 0, 0, 0, NULL, 0},
    {0x00562D1Du, "crt_asin body (crt_asin)", 0, 0, 0, NULL, 0},
    /* its exits (tail jumps), by 0x012663C0: control word restore, nothing to restore at 0x27F */
    {0x0056760Bu, "crt_asin exit (crt_asin)", 0, 0, 0, NULL, 0},
    {0x005675FEu, "crt_asin exit 2 (crt_asin)", 0, 0, 0, NULL, 0},
};
const int x87_ports_count = sizeof(x87_ports) / sizeof(x87_ports[0]);
