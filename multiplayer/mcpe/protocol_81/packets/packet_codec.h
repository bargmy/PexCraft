#ifndef PEXCRAFT_MCPE_PROTOCOL_81_PACKET_CODEC_H
#define PEXCRAFT_MCPE_PROTOCOL_81_PACKET_CODEC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PexMcpeBuffer {
    uint8_t *data;
    size_t size;
    size_t offset;
} PexMcpeBuffer;

typedef struct PexMcpeReadBuffer {
    const uint8_t *data;
    size_t size;
    size_t offset;
} PexMcpeReadBuffer;

void pex_mcpe_buffer_init(PexMcpeBuffer *buffer, uint8_t *data, size_t size);
void pex_mcpe_read_buffer_init(PexMcpeReadBuffer *buffer, const uint8_t *data, size_t size);
size_t pex_mcpe_buffer_remaining(const PexMcpeReadBuffer *buffer);

int pex_mcpe_write_u8(PexMcpeBuffer *buffer, uint8_t value);
int pex_mcpe_write_bytes(PexMcpeBuffer *buffer, const void *data, size_t size);
int pex_mcpe_write_i16_be(PexMcpeBuffer *buffer, int16_t value);
int pex_mcpe_write_i32_be(PexMcpeBuffer *buffer, int32_t value);
int pex_mcpe_write_i32_le(PexMcpeBuffer *buffer, int32_t value);
int pex_mcpe_write_i64_be(PexMcpeBuffer *buffer, int64_t value);
int pex_mcpe_write_f32_be(PexMcpeBuffer *buffer, float value);
int pex_mcpe_write_string(PexMcpeBuffer *buffer, const char *value);

int pex_mcpe_read_u8(PexMcpeReadBuffer *buffer, uint8_t *out_value);
int pex_mcpe_read_bytes(PexMcpeReadBuffer *buffer, const uint8_t **out_data, size_t size);
int pex_mcpe_read_i16_be(PexMcpeReadBuffer *buffer, int16_t *out_value);
int pex_mcpe_read_i32_be(PexMcpeReadBuffer *buffer, int32_t *out_value);
int pex_mcpe_read_i32_le(PexMcpeReadBuffer *buffer, int32_t *out_value);
int pex_mcpe_read_i64_be(PexMcpeReadBuffer *buffer, int64_t *out_value);
int pex_mcpe_read_f32_be(PexMcpeReadBuffer *buffer, float *out_value);
int pex_mcpe_read_string(PexMcpeReadBuffer *buffer, char *out_value, size_t out_size);
int pex_mcpe_read_string_bytes(PexMcpeReadBuffer *buffer, uint8_t **out_data, size_t *out_size);
void pex_mcpe_free_string_bytes(uint8_t *data);

/* Kept for old placeholder callers; MCPE 0.15.4 Genisys packets mostly use fixed ints. */
int pex_mcpe_write_varint(PexMcpeBuffer *buffer, int32_t value);
int pex_mcpe_read_varint(PexMcpeBuffer *buffer, int32_t *out_value);

#ifdef __cplusplus
}
#endif

#endif /* PEXCRAFT_MCPE_PROTOCOL_81_PACKET_CODEC_H */
