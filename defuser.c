#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

static uint32_t rotl32(uint32_t x, unsigned n) {
    return (uint32_t)((x << n) | (x >> (32u - n)));
}

static uint32_t rotr32(uint32_t x, unsigned n) {
    return (uint32_t)((x >> n) | (x << (32u - n)));
}

static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

static uint32_t p3_apply(uint32_t idx, uint32_t x) {
    uint32_t r;
    switch (idx) {
        case 0: r = (x ^ 0x8f7cu) + (x >> 1); break;
        case 1: r = 3u * x + 0xeac7u; break;
        case 2: r = 5u * x + 0x3d540u; break;
        case 3: r = (2u * x) ^ 0xd412u; break;
        case 4: r = (x ^ 0x466du) + (x >> 1); break;
        case 5: r = 5u * x + 0x1555eu; break;
        case 6: r = 5u * x + 0x6b5cu; break;
        case 7: r = (2u * x) ^ 0xa9f4u; break;
        default: r = 0; break;
    }
    return r;
}

static uint32_t p4_walk(uint32_t target, uint32_t lo, uint32_t hi, uint32_t key) {
    uint32_t mid = lo + ((hi - lo) >> 1);
    if (mid == target) return (mid + 0xa1u) ^ key;
    uint32_t base = mid + key;
    if (target < mid) return ((p4_walk(target, lo, mid - 1u, key) * 3u) ^ base) ^ 0xau;
    return ((p4_walk(target, mid + 1u, hi, key) * 3u) ^ base) ^ 0x2u;
}

static uint32_t p6_apply(uint32_t idx, uint32_t x) {
    uint32_t r;
    switch (idx) {
        case 0: r = rotr32(x, 3); break;
        case 1: r = rotl32(x, 5) ^ 0x2a9bu; break;
        case 2: r = (x + 0x7a76u) ^ (x >> 1); break;
        case 3: r = (x ^ 0x14dbu) + 2u * x; break;
        case 4: r = ((x >> 3) ^ x) + 0x3707u; break;
        case 5: r = rotr32(x, 3); break;
        case 6: r = ((x >> 3) ^ x) + 0x3707u; break;
        case 7: r = ((x << 5) + 0x31bau) ^ (5u * x); break;
        default: r = 0; break;
    }
    return r;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;

    uint32_t lo = (uint32_t)challenge;
    uint32_t hi = (uint32_t)(challenge >> 32);
    uint32_t mid = (uint32_t)(challenge >> 16);

    uint32_t s2 = (hi + 8U * lo) ^ 0x780dU;
    uint32_t s0 = (3U * lo + mid) ^ 0x8092U;
    values[0] = 3U * (s0 - s2) + 0x90abU;

    count = 1;
    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[6];
    uint32_t t = (uint32_t)(challenge >> 37);
    t = t + t;
    t &= 0x1eu;
    values[0] = t + 0x16u;
    for (uint32_t i = 1; i < 6; ++i) {
        uint32_t sh = 17u + 5u * (i - 1u);
        uint32_t bits = (uint32_t)((challenge >> sh) & 0xfu);
        values[i] = (bits ^ i) + 2u * values[i - 1u];
    }
    return write_answer(out, cap, values, 6);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t idx = (uint32_t)challenge & 7u;
    uint32_t x = (uint32_t)(challenge >> 32) ^ (uint32_t)(challenge >> 3);
    uint32_t values[2];
    values[0] = idx;
    values[1] = p3_apply(idx, x);
    return write_answer(out, cap, values, 2);
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t target = (uint32_t)challenge & 0x7fu;
    uint32_t key = (uint32_t)(challenge >> 32) ^ (uint32_t)(challenge >> 7);
    uint32_t values[1];
    values[0] = p4_walk(target, 0u, 0x7fu, key);
    return write_answer(out, cap, values, 1);
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    static const uint32_t node_f0[12] = {0x6d60u, 0x1b0cu, 0x2b8fu, 0x93e7u,
                                         0xd4e9u, 0x8c4bu, 0x5effu, 0x18d9u,
                                         0xd637u, 0x062cu, 0x9eb9u, 0xc37au};
    static const uint32_t node_f4[12] = {0x3bu, 0xb71u, 0x165u, 0x216u,
                                         0x931u, 0x319u, 0x412u, 0xa28u,
                                         0x83fu, 0x674u, 0x76fu, 0x57au};
    static const int node_next[12] = {5, 0, 1, 4, 11, 7, 3, 6, -1, 2, 9, 8};
    static const int heads[8] = {10, 9, 2, 1, 0, 5, 7, 6};

    uint32_t idx = (uint32_t)challenge & 7u;
    uint32_t steps = ((uint32_t)(challenge >> 3)) & 3u;
    int n = heads[idx];

    uint32_t acc = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    acc ^= node_f0[n];
    acc ^= 0x7276u;
    acc += node_f4[n];
    uint32_t last = node_f4[n];

    for (uint32_t i = 1; i <= steps; ++i) {
        n = node_next[n];
        acc ^= node_f0[n];
        acc += node_f4[n];
        last = node_f4[n];
    }

    uint32_t values[2] = {acc, last};
    return write_answer(out, cap, values, 2);
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t x = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t t = p6_apply((uint32_t)challenge & 7u, x);
    t ^= 0x6052u;
    uint32_t r = p6_apply(((uint32_t)(challenge >> 3)) & 7u, t) + 0xe634u;
    uint32_t values[1] = {r};
    return write_answer(out, cap, values, 1);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t c_lo = (uint32_t)challenge;
    uint32_t c_hi = (uint32_t)(challenge >> 32);
    uint32_t t_lo = (uint32_t)context;
    uint32_t t_hi = (uint32_t)(context >> 32);

    uint32_t pick = t_lo & 3u;
    uint32_t x = c_lo ^ c_hi ^ t_lo ^ t_hi;
    uint32_t mixed = rotl32(x, 1) + 0x5502u;
    mixed ^= 0x4447u;
    uint32_t v1 = mixed ^ ((0x5502u * pick) ^ (uint32_t)(context >> 16));
    uint32_t v2 = v1 ^ ((t_lo ^ t_hi) + pick + 0x4447u + ((c_lo ^ c_hi) >> 1));

    int n = snprintf(out, cap, "%32sOPEN %" PRIu32 " %" PRIu32 " %" PRIu32, "",
                     pick, v1, v2);
    if (n < 0 || (size_t)n >= cap) return 1;
    return 0;
}