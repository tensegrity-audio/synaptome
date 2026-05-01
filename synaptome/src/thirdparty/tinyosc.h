#pragma once
#include <stdint.h>
#include <string.h>

struct tosc_message {
    const char* address;
    const char* format;
    const uint8_t* data;
    int len;
};

static inline uint32_t tosc_u32be(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}

static inline int tosc_align4(int n) {
    return (n + 3) & ~3;
}

static inline int tosc_readMessage(tosc_message* m, const uint8_t* buf, int len) {
    if (!m || !buf || len <= 0) return -1;
    int offset = 0;
    m->address = (const char*)buf;
    while (offset < len && buf[offset] != '\0') {
        ++offset;
    }
    if (offset >= len) return -1;
    offset = tosc_align4(offset + 1);
    if (offset >= len) return -1;
    m->format = (const char*)(buf + offset);
    while (offset < len && buf[offset] != '\0') {
        ++offset;
    }
    if (offset >= len) return -1;
    offset = tosc_align4(offset + 1);
    if (offset > len) return -1;
    m->data = buf + offset;
    m->len = len - offset;
    return 0;
}

static inline int32_t tosc_getNextInt32(const tosc_message* m, int& offset) {
    int32_t v = (int32_t)tosc_u32be(m->data + offset);
    offset += 4;
    return v;
}

static inline float tosc_getNextFloat(const tosc_message* m, int& offset) {
    uint32_t raw = tosc_u32be(m->data + offset);
    offset += 4;
    float out;
    memcpy(&out, &raw, sizeof(out));
    return out;
}
