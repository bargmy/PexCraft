#include "packet_codec.h"
#include <string.h>
#include <stdlib.h>

void pex_mcpe_buffer_init(PexMcpeBuffer *buffer, uint8_t *data, size_t size) {
    if (!buffer) return;
    buffer->data = data;
    buffer->size = size;
    buffer->offset = 0;
}

void pex_mcpe_read_buffer_init(PexMcpeReadBuffer *buffer, const uint8_t *data, size_t size) {
    if (!buffer) return;
    buffer->data = data;
    buffer->size = size;
    buffer->offset = 0;
}

size_t pex_mcpe_buffer_remaining(const PexMcpeReadBuffer *buffer) {
    if (!buffer || buffer->offset > buffer->size) return 0;
    return buffer->size - buffer->offset;
}

static int pex_mcpe_can_write(PexMcpeBuffer *buffer, size_t n) {
    return buffer && buffer->data && buffer->offset <= buffer->size && n <= buffer->size - buffer->offset;
}

static int pex_mcpe_can_read(PexMcpeReadBuffer *buffer, size_t n) {
    return buffer && buffer->data && buffer->offset <= buffer->size && n <= buffer->size - buffer->offset;
}

int pex_mcpe_write_u8(PexMcpeBuffer *buffer, uint8_t value) {
    if (!pex_mcpe_can_write(buffer, 1)) return 0;
    buffer->data[buffer->offset++] = value;
    return 1;
}

int pex_mcpe_write_bytes(PexMcpeBuffer *buffer, const void *data, size_t size) {
    if (size == 0) return 1;
    if (!data || !pex_mcpe_can_write(buffer, size)) return 0;
    memcpy(buffer->data + buffer->offset, data, size);
    buffer->offset += size;
    return 1;
}

int pex_mcpe_write_i16_be(PexMcpeBuffer *buffer, int16_t value) {
    uint16_t v = (uint16_t)value;
    return pex_mcpe_write_u8(buffer, (uint8_t)((v >> 8) & 0xff)) &&
           pex_mcpe_write_u8(buffer, (uint8_t)(v & 0xff));
}

int pex_mcpe_write_i32_be(PexMcpeBuffer *buffer, int32_t value) {
    uint32_t v = (uint32_t)value;
    return pex_mcpe_write_u8(buffer, (uint8_t)((v >> 24) & 0xff)) &&
           pex_mcpe_write_u8(buffer, (uint8_t)((v >> 16) & 0xff)) &&
           pex_mcpe_write_u8(buffer, (uint8_t)((v >> 8) & 0xff)) &&
           pex_mcpe_write_u8(buffer, (uint8_t)(v & 0xff));
}

int pex_mcpe_write_i32_le(PexMcpeBuffer *buffer, int32_t value) {
    uint32_t v = (uint32_t)value;
    return pex_mcpe_write_u8(buffer, (uint8_t)(v & 0xff)) &&
           pex_mcpe_write_u8(buffer, (uint8_t)((v >> 8) & 0xff)) &&
           pex_mcpe_write_u8(buffer, (uint8_t)((v >> 16) & 0xff)) &&
           pex_mcpe_write_u8(buffer, (uint8_t)((v >> 24) & 0xff));
}

int pex_mcpe_write_i64_be(PexMcpeBuffer *buffer, int64_t value) {
    uint64_t v = (uint64_t)value;
    for (int i = 7; i >= 0; --i) {
        if (!pex_mcpe_write_u8(buffer, (uint8_t)((v >> (i * 8)) & 0xff))) return 0;
    }
    return 1;
}

int pex_mcpe_write_f32_be(PexMcpeBuffer *buffer, float value) {
    union { float f; uint32_t u; } u;
    u.f = value;
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return pex_mcpe_write_bytes(buffer, &u.u, 4);
#else
    return pex_mcpe_write_u8(buffer, (uint8_t)((u.u >> 24) & 0xff)) &&
           pex_mcpe_write_u8(buffer, (uint8_t)((u.u >> 16) & 0xff)) &&
           pex_mcpe_write_u8(buffer, (uint8_t)((u.u >> 8) & 0xff)) &&
           pex_mcpe_write_u8(buffer, (uint8_t)(u.u & 0xff));
#endif
}

int pex_mcpe_write_string(PexMcpeBuffer *buffer, const char *value) {
    size_t len = value ? strlen(value) : 0;
    if (len > 32767) return 0;
    return pex_mcpe_write_i16_be(buffer, (int16_t)len) && pex_mcpe_write_bytes(buffer, value ? value : "", len);
}

int pex_mcpe_read_u8(PexMcpeReadBuffer *buffer, uint8_t *out_value) {
    if (!pex_mcpe_can_read(buffer, 1)) return 0;
    if (out_value) *out_value = buffer->data[buffer->offset];
    buffer->offset++;
    return 1;
}

int pex_mcpe_read_bytes(PexMcpeReadBuffer *buffer, const uint8_t **out_data, size_t size) {
    if (!pex_mcpe_can_read(buffer, size)) return 0;
    if (out_data) *out_data = buffer->data + buffer->offset;
    buffer->offset += size;
    return 1;
}

int pex_mcpe_read_i16_be(PexMcpeReadBuffer *buffer, int16_t *out_value) {
    const uint8_t *p;
    if (!pex_mcpe_read_bytes(buffer, &p, 2)) return 0;
    if (out_value) *out_value = (int16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
    return 1;
}

int pex_mcpe_read_i32_be(PexMcpeReadBuffer *buffer, int32_t *out_value) {
    const uint8_t *p;
    if (!pex_mcpe_read_bytes(buffer, &p, 4)) return 0;
    uint32_t v = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
    if (out_value) *out_value = (int32_t)v;
    return 1;
}

int pex_mcpe_read_i32_le(PexMcpeReadBuffer *buffer, int32_t *out_value) {
    const uint8_t *p;
    if (!pex_mcpe_read_bytes(buffer, &p, 4)) return 0;
    uint32_t v = ((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16) | ((uint32_t)p[1] << 8) | (uint32_t)p[0];
    if (out_value) *out_value = (int32_t)v;
    return 1;
}

int pex_mcpe_read_i64_be(PexMcpeReadBuffer *buffer, int64_t *out_value) {
    const uint8_t *p;
    if (!pex_mcpe_read_bytes(buffer, &p, 8)) return 0;
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | (uint64_t)p[i];
    if (out_value) *out_value = (int64_t)v;
    return 1;
}

int pex_mcpe_read_f32_be(PexMcpeReadBuffer *buffer, float *out_value) {
    const uint8_t *p;
    if (!pex_mcpe_read_bytes(buffer, &p, 4)) return 0;
    union { uint32_t u; float f; } u;
    u.u = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
    if (out_value) *out_value = u.f;
    return 1;
}

int pex_mcpe_read_string(PexMcpeReadBuffer *buffer, char *out_value, size_t out_size) {
    int16_t len16;
    if (out_value && out_size) out_value[0] = '\0';
    if (!pex_mcpe_read_i16_be(buffer, &len16) || len16 < 0) return 0;
    const uint8_t *p;
    size_t len = (size_t)len16;
    if (!pex_mcpe_read_bytes(buffer, &p, len)) return 0;
    if (out_value && out_size) {
        size_t n = len < out_size - 1 ? len : out_size - 1;
        memcpy(out_value, p, n);
        out_value[n] = '\0';
    }
    return 1;
}

int pex_mcpe_read_string_bytes(PexMcpeReadBuffer *buffer, uint8_t **out_data, size_t *out_size) {
    int16_t len16;
    if (out_data) *out_data = NULL;
    if (out_size) *out_size = 0;
    if (!pex_mcpe_read_i16_be(buffer, &len16) || len16 < 0) return 0;
    size_t len = (size_t)len16;
    const uint8_t *p;
    if (!pex_mcpe_read_bytes(buffer, &p, len)) return 0;
    uint8_t *copy = NULL;
    if (len > 0) {
        copy = (uint8_t *)malloc(len);
        if (!copy) return 0;
        memcpy(copy, p, len);
    }
    if (out_data) *out_data = copy;
    else if (copy) free(copy);
    if (out_size) *out_size = len;
    return 1;
}

void pex_mcpe_free_string_bytes(uint8_t *data) {
    if (data) free(data);
}

int pex_mcpe_write_varint(PexMcpeBuffer *buffer, int32_t value) {
    uint32_t v = (uint32_t)value;
    do {
        uint8_t temp = (uint8_t)(v & 0x7f);
        v >>= 7;
        if (v != 0) temp |= 0x80;
        if (!pex_mcpe_write_u8(buffer, temp)) return 0;
    } while (v != 0);
    return 1;
}

int pex_mcpe_read_varint(PexMcpeBuffer *buffer, int32_t *out_value) {
    if (!buffer || !buffer->data) return 0;
    uint32_t result = 0;
    int shift = 0;
    while (shift < 35 && buffer->offset < buffer->size) {
        uint8_t b = buffer->data[buffer->offset++];
        result |= (uint32_t)(b & 0x7f) << shift;
        if ((b & 0x80) == 0) {
            if (out_value) *out_value = (int32_t)result;
            return 1;
        }
        shift += 7;
    }
    return 0;
}
