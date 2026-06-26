#include "text_packet.h"
#include "packet_codec.h"
#include "packet_ids.h"
#include <stdio.h>
#include <string.h>

int pex_mcpe_decode_text_packet(const uint8_t *data, size_t size, char *out_text, size_t out_text_size) {
    if (out_text && out_text_size) out_text[0] = 0;
    PexMcpeReadBuffer b;
    uint8_t type;
    char source[128] = "";
    char msg[512] = "";
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_u8(&b, &type)) return 0;
    if (type == 1 || type == 3) {
        if (!pex_mcpe_read_string(&b, source, sizeof(source))) return 0;
        if (!pex_mcpe_read_string(&b, msg, sizeof(msg))) return 0;
        if (out_text && out_text_size) snprintf(out_text, out_text_size, source[0] ? "<%s> %s" : "%s%s", source, msg);
        return 1;
    }
    if (!pex_mcpe_read_string(&b, msg, sizeof(msg))) return 0;
    if (out_text && out_text_size) snprintf(out_text, out_text_size, "%s", msg);
    return 1;
}

int pex_mcpe_encode_chat_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size, const char *source, const char *message) {
    if (out_size) *out_size = 0;
    PexMcpeBuffer b;
    pex_mcpe_buffer_init(&b, out_data, out_capacity);
    if (!source) source = "";
    if (!message) message = "";
    if (!pex_mcpe_write_u8(&b, PEX_MCPE_RAKLIB_GAME_PACKET) ||
        !pex_mcpe_write_u8(&b, PEX_MCPE_PACKET_TEXT) ||
        !pex_mcpe_write_u8(&b, 1) ||
        !pex_mcpe_write_string(&b, source) ||
        !pex_mcpe_write_string(&b, message)) return 0;
    if (out_size) *out_size = b.offset;
    return 1;
}
