#include "text_packet.h"
#include "packet_codec.h"
#include "packet_ids.h"
#include <stdio.h>
#include <string.h>

static const char *pex_mcpe_known_translation(const char *key) {
    if (!key) return NULL;
    if (key[0] == '%') key++;
    if (strcmp(key, "multiplayer.player.joined") == 0) return "%s joined the game";
    if (strcmp(key, "multiplayer.player.left") == 0) return "%s left the game";
    if (strcmp(key, "chat.type.text") == 0) return "<%s> %s";
    if (strcmp(key, "chat.type.announcement") == 0) return "[%s] %s";
    if (strcmp(key, "commands.message.display.incoming") == 0) return "%s whispers to you: %s";
    if (strcmp(key, "commands.message.display.outgoing") == 0) return "You whisper to %s: %s";
    return NULL;
}

static void pex_mcpe_format_translation(char *out, size_t out_size,
                                        const char *key, char params[][256], int param_count) {
    if (!out || out_size == 0) return;
    out[0] = 0;
    const char *fmt = pex_mcpe_known_translation(key);
    if (fmt) {
        if (param_count <= 0) snprintf(out, out_size, "%s", fmt);
        else if (param_count == 1) snprintf(out, out_size, fmt, params[0]);
        else snprintf(out, out_size, fmt, params[0], params[1]);
        return;
    }
    if (key && key[0] == '%' && param_count > 0) {
        snprintf(out, out_size, "%s", params[0]);
        for (int i = 1; i < param_count; ++i) {
            size_t n = strlen(out);
            if (n + 2 < out_size) snprintf(out + n, out_size - n, " %s", params[i]);
        }
        return;
    }
    snprintf(out, out_size, "%s", key ? key : "");
}

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
    if (type == 2) {
        char key[256] = "";
        uint8_t count = 0;
        char params[8][256];
        memset(params, 0, sizeof(params));
        if (!pex_mcpe_read_string(&b, key, sizeof(key))) return 0;
        if (!pex_mcpe_read_u8(&b, &count)) count = 0;
        if (count > 8) count = 8;
        for (uint8_t i = 0; i < count; ++i) {
            if (!pex_mcpe_read_string(&b, params[i], sizeof(params[i]))) return 0;
        }
        if (out_text && out_text_size) pex_mcpe_format_translation(out_text, out_text_size, key, params, count);
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
