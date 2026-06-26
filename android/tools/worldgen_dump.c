/* Standalone Worldgen dump harness.
   Build from repo root:
     cc -std=c99 -O2 tools/worldgen_dump.c -lm -o worldgen_dump
   Usage:
     ./worldgen_dump <seed> <dimension> <chunkX> <chunkZ>
   Dimensions: 0 Overworld, -1 Nether debug generator, 1 End debug generator. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define ExitProcess(code) exit(code)
#define PEXCRAFT_WORLDGEN_ARRAYS_ONLY 1
#include "../src/worldgen/level.c"

typedef struct Sha1Ctx { uint32_t h[5]; uint64_t len; unsigned char buf[64]; size_t used; } Sha1Ctx;
static uint32_t sha1_rol(uint32_t v, int n) { return (v << n) | (v >> (32 - n)); }
static void sha1_block(Sha1Ctx *c, const unsigned char b[64]) {
    uint32_t w[80];
    for (int i = 0; i < 16; ++i) w[i] = ((uint32_t)b[i*4] << 24) | ((uint32_t)b[i*4+1] << 16) | ((uint32_t)b[i*4+2] << 8) | (uint32_t)b[i*4+3];
    for (int i = 16; i < 80; ++i) w[i] = sha1_rol(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    uint32_t a=c->h[0], d=c->h[3], e=c->h[4], f, k, temp, b2=c->h[1], cc=c->h[2];
    for (int i = 0; i < 80; ++i) {
        if (i < 20) { f = (b2 & cc) | ((~b2) & d); k = 0x5a827999u; }
        else if (i < 40) { f = b2 ^ cc ^ d; k = 0x6ed9eba1u; }
        else if (i < 60) { f = (b2 & cc) | (b2 & d) | (cc & d); k = 0x8f1bbcdcu; }
        else { f = b2 ^ cc ^ d; k = 0xca62c1d6u; }
        temp = sha1_rol(a,5) + f + e + k + w[i];
        e = d; d = cc; cc = sha1_rol(b2,30); b2 = a; a = temp;
    }
    c->h[0]+=a; c->h[1]+=b2; c->h[2]+=cc; c->h[3]+=d; c->h[4]+=e;
}
static void sha1_init(Sha1Ctx *c) { c->h[0]=0x67452301u; c->h[1]=0xefcdab89u; c->h[2]=0x98badcfeu; c->h[3]=0x10325476u; c->h[4]=0xc3d2e1f0u; c->len=0; c->used=0; }
static void sha1_update(Sha1Ctx *c, const void *data, size_t n) {
    const unsigned char *p=(const unsigned char*)data; c->len += (uint64_t)n * 8u;
    while (n) { size_t take = 64 - c->used; if (take > n) take = n; memcpy(c->buf + c->used, p, take); c->used += take; p += take; n -= take; if (c->used == 64) { sha1_block(c, c->buf); c->used = 0; } }
}
static void sha1_final(Sha1Ctx *c, unsigned char out[20]) {
    c->buf[c->used++] = 0x80;
    if (c->used > 56) { while (c->used < 64) c->buf[c->used++] = 0; sha1_block(c, c->buf); c->used = 0; }
    while (c->used < 56) c->buf[c->used++] = 0;
    for (int i = 7; i >= 0; --i) c->buf[c->used++] = (unsigned char)((c->len >> (i*8)) & 0xffu);
    sha1_block(c, c->buf);
    for (int i = 0; i < 5; ++i) { out[i*4]=(unsigned char)(c->h[i]>>24); out[i*4+1]=(unsigned char)(c->h[i]>>16); out[i*4+2]=(unsigned char)(c->h[i]>>8); out[i*4+3]=(unsigned char)c->h[i]; }
}
static void sha1_hex(const void *data, size_t n, char hex[41]) {
    Sha1Ctx c; unsigned char d[20]; sha1_init(&c); sha1_update(&c, data, n); sha1_final(&c, d);
    static const char *h="0123456789abcdef";
    for (int i=0;i<20;i++){ hex[i*2]=h[d[i]>>4]; hex[i*2+1]=h[d[i]&15]; } hex[40]=0;
}

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "usage: %s <seed> <dimension> <chunkX> <chunkZ>\n", argv[0]);
        return 2;
    }
    long long seed = strtoll(argv[1], NULL, 10);
    int dim = atoi(argv[2]);
    int cx = atoi(argv[3]);
    int cz = atoi(argv[4]);
    unsigned char *blocks=(unsigned char*)calloc(32768,1), *data=(unsigned char*)calloc(16384,1), *height=(unsigned char*)calloc(256,1), *biomes=(unsigned char*)calloc(256,1);
    if (!blocks || !data || !height || !biomes) return 4;
    if (!worldgen_generate_dimension_chunk_arrays(seed, dim, cx, cz, blocks, data, height, biomes)) return 5;
    char hb[41], hd[41], hh[41], hbio[41];
    sha1_hex(blocks, 32768, hb); sha1_hex(data, 16384, hd); sha1_hex(height, 256, hh); sha1_hex(biomes, 256, hbio);
    int sx[3], sz[3]; worldgen_get_stronghold_coords(seed, sx, sz);
    printf("seed=%lld dim=%d chunk=%d,%d blocks_sha1=%s data_sha1=%s heightmap_sha1=%s biomes_sha1=%s\n", seed, dim, cx, cz, hb, hd, hh, hbio);
    printf("strongholds=%d,%d;%d,%d;%d,%d village_here=%d mineshaft_here=%d\n", sx[0],sz[0],sx[1],sz[1],sx[2],sz[2], worldgen_village_can_spawn(seed,cx,cz), worldgen_mineshaft_can_spawn(seed,cx,cz));
    free(blocks); free(data); free(height); free(biomes);
    return 0;
}
