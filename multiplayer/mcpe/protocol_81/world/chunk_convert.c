#include "chunk_convert.h"

/* Genisys sends old 128-high MCPE chunks as:
 * block ids:      16*16*128 bytes
 * block data:     16*16*128 / 2 nibbles
 * sky light:      16*16*128 / 2 nibbles
 * block light:    16*16*128 / 2 nibbles
 * height map:     256 bytes
 * biome colors:   256 big-endian ints
 * extras/tiles:   trailing payload, currently ignored by PexCraft
 */
int pex_mcpe_convert_chunk_to_pexcraft(const uint8_t *mcpe_chunk,
                                       size_t mcpe_chunk_size,
                                       int chunk_x,
                                       int chunk_z,
                                       int order) {
    (void)order;
    const int mcpe_height = 128;
    const size_t ids_size = 16u * 16u * 128u;
    const size_t nibble_size = ids_size / 2u;
    const size_t min_size = ids_size + nibble_size * 3u;
    if (!mcpe_chunk || mcpe_chunk_size < min_size) return 0;

    const uint8_t *ids = mcpe_chunk;
    const uint8_t *meta = ids + ids_size;
    const uint8_t *sky = meta + nibble_size;
    const uint8_t *light = sky + nibble_size;
    (void)sky;

    int base_x = chunk_x * 16;
    int base_z = chunk_z * 16;

    for (int y = 0; y < mcpe_height; ++y) {
        for (int z = 0; z < 16; ++z) {
            int wz = base_z + z;
            for (int x = 0; x < 16; ++x) {
                int wx = base_x + x;
                int idx = (y << 8) | (z << 4) | x;
                uint8_t id = ids[idx];
                uint8_t mbyte = meta[idx >> 1];
                uint8_t lbyte = light[idx >> 1];
                uint8_t m = (idx & 1) ? (uint8_t)((mbyte >> 4) & 15) : (uint8_t)(mbyte & 15);
                uint8_t bl = (idx & 1) ? (uint8_t)((lbyte >> 4) & 15) : (uint8_t)(lbyte & 15);
                if (!flat_in_bounds(wx, y, wz)) continue;
                g_flat_blocks[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = id;
                g_flat_block_light[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = bl;
                g_flat_meta[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = block_is_liquid(id) ? (m & 15) : m;
                g_flat_levels[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = block_is_liquid(id) ? (m & 15) : 0;
            }
        }
    }

    for (int y = mcpe_height; y < PEX_NET_WORLD_HEIGHT; ++y) {
        for (int z = 0; z < 16; ++z) {
            int wz = base_z + z;
            for (int x = 0; x < 16; ++x) {
                int wx = base_x + x;
                if (!flat_in_bounds(wx, y, wz)) continue;
                g_flat_blocks[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = 0;
                g_flat_meta[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = 0;
                g_flat_levels[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = 0;
                g_flat_block_light[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = 0;
            }
        }
    }
    return 1;
}
