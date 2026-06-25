/* Split from original monolithic main.c. Included by src/main.c unity build. */
#if defined(PEXCRAFT_WORLDGEN_ARRAYS_ONLY)
static int g_world_map_features = 1;
#endif

#define BLK_AIR       0
#define BLK_STONE     1
#define BLK_GRASS     2
#define BLK_DIRT      3
#define BLK_COBBLESTONE 4
#define BLK_WOOD_PLANKS 5
#define BLK_SAPLING   6
#define BLK_WEB       30
#define BLK_WOOL      35
#define BLK_BRICK     45
#define BLK_BOOKSHELF 47
#define BLK_TORCH     50
#define BLK_FIRE 51
#define BLK_MOB_SPAWNER 52
#define BLK_WOOD_STAIRS 53
#define BLK_CHEST 54
#define BLK_WORKBENCH 58
#define BLK_FURNACE 61
#define BLK_CROPS     59
#define BLK_FARMLAND  60
#define BLK_WOOD_DOOR 64
#define BLK_LADDER    65
#define BLK_RAILS     66
#define BLK_COBBLE_STAIRS 67
#define BLK_IRON_DOOR 71
#define BLK_CACTUS 81
#define BLK_BEDROCK   7
#define BLK_WATER     8
#define BLK_STILL_WATER 9
#define BLK_LAVA      10
#define BLK_STILL_LAVA 11
#define BLK_SAND      12
#define BLK_GRAVEL    13
#define BLK_GOLD_ORE  14
#define BLK_IRON_ORE  15
#define BLK_COAL_ORE  16
#define BLK_LOG       17
#define BLK_LEAVES    18
#define BLK_GLASS     20
#define BLK_LAPIS_ORE 21
#define BLK_LAPIS_BLOCK 22
#define BLK_SANDSTONE 24
#define BLK_TALL_GRASS 31
#define BLK_DEAD_BUSH 32
#define BLK_SILVERFISH 97
#define BLK_STONE_BRICK 98
#define BLK_MUSHROOM_CAP_BROWN 99
#define BLK_MUSHROOM_CAP_RED 100
#define BLK_IRON_BARS 101
#define BLK_GLASS_PANE 102
#define BLK_BRICK_STAIRS 108
#define BLK_STONE_BRICK_STAIRS 109
#define BLK_END_PORTAL 119
#define BLK_END_PORTAL_FRAME 120
#define BLK_DIAMOND_ORE 56
#define BLK_REDSTONE_ORE 73
#define BLK_ICE       79
#define BLK_CLAY      82
#define BLK_SNOW_LAYER 78
#define BLK_YELLOW_FLOWER 37
#define BLK_RED_ROSE 38
#define BLK_BROWN_MUSHROOM 39
#define BLK_RED_MUSHROOM 40
#define BLK_SLAB      44
#define BLK_DOUBLE_SLAB 43
#define BLK_REEDS     83
#define BLK_FENCE     85
#define BLK_PUMPKIN   86
#define BLK_VINE      106
#define BLK_LILY_PAD  111
#define BLK_NETHER_WART 115
#define BLK_NETHERRACK 87
#define BLK_SOUL_SAND 88
#define BLK_GLOWSTONE 89
#define BLK_NETHER_BRICK 112
#define BLK_NETHER_BRICK_FENCE 113
#define BLK_NETHER_BRICK_STAIRS 114
#define BLK_END_STONE 121
#define BLK_OBSIDIAN 49
#define BLK_MYCELIUM  110

#ifndef BLOCK_META_WOOD_OAK
#define BLOCK_META_WOOD_OAK 0
#define BLOCK_META_WOOD_SPRUCE 1
#define BLOCK_META_WOOD_BIRCH 2
#define BLOCK_META_WOOD_JUNGLE 3
#define BLOCK_META_SANDSTONE_NORMAL 0
#define BLOCK_META_SANDSTONE_CHISELED 1
#define BLOCK_META_SANDSTONE_SMOOTH 2
#define BLOCK_META_STONE_BRICK_NORMAL 0
#define BLOCK_META_STONE_BRICK_MOSSY 1
#define BLOCK_META_STONE_BRICK_CRACKED 2
#define BLOCK_META_STONE_BRICK_CHISELED 3
#define BLOCK_META_TALL_GRASS_SHRUB 0
#define BLOCK_META_TALL_GRASS_GRASS 1
#define BLOCK_META_TALL_GRASS_FERN 2
#endif

#define JAVA_RAND_MULT UINT64_C(0x5DEECE66D)
#define JAVA_RAND_ADD  UINT64_C(0xB)
#define JAVA_RAND_MASK ((UINT64_C(1) << 48) - 1)

typedef struct JavaRandom { uint64_t seed; } JavaRandom;

static void jr_set_seed(JavaRandom *r, int64_t seed) { r->seed = (((uint64_t)seed) ^ JAVA_RAND_MULT) & JAVA_RAND_MASK; }
static int32_t jr_next(JavaRandom *r, int bits) { r->seed = (r->seed * JAVA_RAND_MULT + JAVA_RAND_ADD) & JAVA_RAND_MASK; return (int32_t)(r->seed >> (48 - bits)); }
static int32_t jr_next_int(JavaRandom *r) { return jr_next(r, 32); }
static int32_t jr_next_int_bound(JavaRandom *r, int32_t bound) {
    if (bound <= 0) return 0;
    if ((bound & -bound) == bound) return (int32_t)((bound * (int64_t)jr_next(r, 31)) >> 31);
    int32_t bits, val;
    do { bits = jr_next(r, 31); val = bits % bound; } while (bits - val + (bound - 1) < 0);
    return val;
}
static int64_t jr_next_long(JavaRandom *r) { return ((int64_t)jr_next(r, 32) << 32) + (int64_t)jr_next(r, 32); }
static float jr_next_float(JavaRandom *r) { return (float)jr_next(r, 24) / (float)(1 << 24); }
static double jr_next_double(JavaRandom *r) { return (((int64_t)jr_next(r, 26) << 27) + (int64_t)jr_next(r, 27)) / (double)(INT64_C(1) << 53); }

static float beta_sin_table[65536];
static int beta_sin_ready = 0;
static void beta_init_sin(void) {
    if (beta_sin_ready) return;
    for (int i = 0; i < 65536; i++) beta_sin_table[i] = (float)sin(i * M_PI * 2.0 / 65536.0);
    beta_sin_ready = 1;
}
static float beta_sin(float v) { beta_init_sin(); return beta_sin_table[((int)(v * 10430.378f)) & 65535]; }
static float beta_cos(float v) { beta_init_sin(); return beta_sin_table[((int)(v * 10430.378f + 16384.0f)) & 65535]; }
static int beta_floor_double(double v) { int i = (int)v; return v < (double)i ? i - 1 : i; }

static int chunk_index(int x, int y, int z) { return (x << 11) | (z << 7) | y; }
static void set_nibble(unsigned char *arr, int idx, int val) { int bi = idx >> 1; if (idx & 1) arr[bi] = (unsigned char)((arr[bi] & 0x0F) | ((val & 15) << 4)); else arr[bi] = (unsigned char)((arr[bi] & 0xF0) | (val & 15)); }
static unsigned char get_block_local(const unsigned char *blocks, int x, int y, int z) { if (x<0||x>=16||y<0||y>=128||z<0||z>=16) return 0; return blocks[chunk_index(x,y,z)]; }
static void set_block_local(unsigned char *blocks, int x, int y, int z, unsigned char id) { if (x>=0&&x<16&&y>=0&&y<128&&z>=0&&z<16) blocks[chunk_index(x,y,z)] = id; }

/* aa.java */
typedef struct PerlinNoise {
    int p[512];
    double xo, yo, zo;
} PerlinNoise;

static double pn_lerp(double t, double a, double b) { return a + t * (b - a); }
static double pn_grad2(int h, double x, double z) {
    int g = h & 15;
    double a = (1 - ((g & 8) >> 3)) * x;
    double b = g < 4 ? 0.0 : (g != 12 && g != 14 ? z : x);
    return ((g & 1) == 0 ? a : -a) + ((g & 2) == 0 ? b : -b);
}
static double pn_grad3(int h, double x, double y, double z) {
    int g = h & 15;
    double a = g < 8 ? x : y;
    double b = g < 4 ? y : (g != 12 && g != 14 ? z : x);
    return ((g & 1) == 0 ? a : -a) + ((g & 2) == 0 ? b : -b);
}
static void perlin_init(PerlinNoise *n, JavaRandom *r) {
    n->xo = jr_next_double(r) * 256.0;
    n->yo = jr_next_double(r) * 256.0;
    n->zo = jr_next_double(r) * 256.0;
    for (int i = 0; i < 256; i++) n->p[i] = i;
    for (int i = 0; i < 256; i++) {
        int j = jr_next_int_bound(r, 256 - i) + i;
        int t = n->p[i]; n->p[i] = n->p[j]; n->p[j] = t;
        n->p[i + 256] = n->p[i];
    }
}
static double perlin_noise2(PerlinNoise *n, double x, double z) {
    double X = x + n->xo;
    double Y = 0.0 + n->yo;
    double Z = z + n->zo;
    int ix = beta_floor_double(X), iy = beta_floor_double(Y), iz = beta_floor_double(Z);
    int xi = ix & 255, yi = iy & 255, zi = iz & 255;
    X -= ix; Y -= iy; Z -= iz;
    double u = X*X*X*(X*(X*6.0-15.0)+10.0);
    double v = Y*Y*Y*(Y*(Y*6.0-15.0)+10.0);
    double w = Z*Z*Z*(Z*(Z*6.0-15.0)+10.0);
    int A = n->p[xi] + yi, AA = n->p[A] + zi, AB = n->p[A+1] + zi;
    int B = n->p[xi+1] + yi, BA = n->p[B] + zi, BB = n->p[B+1] + zi;
    return pn_lerp(w,
        pn_lerp(v,
            pn_lerp(u, pn_grad3(n->p[AA], X, Y, Z), pn_grad3(n->p[BA], X-1.0, Y, Z)),
            pn_lerp(u, pn_grad3(n->p[AB], X, Y-1.0, Z), pn_grad3(n->p[BB], X-1.0, Y-1.0, Z))),
        pn_lerp(v,
            pn_lerp(u, pn_grad3(n->p[AA+1], X, Y, Z-1.0), pn_grad3(n->p[BA+1], X-1.0, Y, Z-1.0)),
            pn_lerp(u, pn_grad3(n->p[AB+1], X, Y-1.0, Z-1.0), pn_grad3(n->p[BB+1], X-1.0, Y-1.0, Z-1.0))));
}
static void perlin_fill(PerlinNoise *n, double *out, double x0, double y0, double z0, int xs, int ys, int zs, double sx, double sy, double sz, double amp) {
    if (ys == 1) {
        int idx = 0;
        double inv = 1.0 / amp;
        for (int x = 0; x < xs; x++) {
            double X = (x0 + x) * sx + n->xo;
            int ix = beta_floor_double(X); int xi = ix & 255; X -= ix;
            double u = X*X*X*(X*(X*6.0-15.0)+10.0);
            for (int z = 0; z < zs; z++) {
                double Z = (z0 + z) * sz + n->zo;
                int iz = beta_floor_double(Z); int zi = iz & 255; Z -= iz;
                double w = Z*Z*Z*(Z*(Z*6.0-15.0)+10.0);
                int a = n->p[xi] + 0;
                int aa = n->p[a] + zi;
                int b = n->p[xi + 1] + 0;
                int ba = n->p[b] + zi;
                double v1 = pn_lerp(u, pn_grad2(n->p[aa], X, Z), pn_grad3(n->p[ba], X - 1.0, 0.0, Z));
                double v2 = pn_lerp(u, pn_grad3(n->p[aa + 1], X, 0.0, Z - 1.0), pn_grad3(n->p[ba + 1], X - 1.0, 0.0, Z - 1.0));
                out[idx++] += pn_lerp(w, v1, v2) * inv;
            }
        }
    } else {
        int idx = 0;
        double inv = 1.0 / amp;
        int last_yi = -1;
        int A=0,AA=0,AB=0,B=0,BA=0,BB=0;
        double x1=0,x2=0,x3=0,x4=0;
        for (int x = 0; x < xs; x++) {
            double X = (x0 + x) * sx + n->xo;
            int ix = beta_floor_double(X); int xi = ix & 255; X -= ix;
            double u = X*X*X*(X*(X*6.0-15.0)+10.0);
            for (int z = 0; z < zs; z++) {
                double Z = (z0 + z) * sz + n->zo;
                int iz = beta_floor_double(Z); int zi = iz & 255; Z -= iz;
                double w = Z*Z*Z*(Z*(Z*6.0-15.0)+10.0);
                for (int y = 0; y < ys; y++) {
                    double Y = (y0 + y) * sy + n->yo;
                    int iy = beta_floor_double(Y); int yi = iy & 255; Y -= iy;
                    double v = Y*Y*Y*(Y*(Y*6.0-15.0)+10.0);
                    if (y == 0 || yi != last_yi) {
                        last_yi = yi;
                        A = n->p[xi] + yi; AA = n->p[A] + zi; AB = n->p[A + 1] + zi;
                        B = n->p[xi + 1] + yi; BA = n->p[B] + zi; BB = n->p[B + 1] + zi;
                        x1 = pn_lerp(u, pn_grad3(n->p[AA], X, Y, Z), pn_grad3(n->p[BA], X-1.0, Y, Z));
                        x2 = pn_lerp(u, pn_grad3(n->p[AB], X, Y-1.0, Z), pn_grad3(n->p[BB], X-1.0, Y-1.0, Z));
                        x3 = pn_lerp(u, pn_grad3(n->p[AA+1], X, Y, Z-1.0), pn_grad3(n->p[BA+1], X-1.0, Y, Z-1.0));
                        x4 = pn_lerp(u, pn_grad3(n->p[AB+1], X, Y-1.0, Z-1.0), pn_grad3(n->p[BB+1], X-1.0, Y-1.0, Z-1.0));
                    }
                    out[idx++] += pn_lerp(w, pn_lerp(v, x1, x2), pn_lerp(v, x3, x4)) * inv;
                }
            }
        }
    }
}

typedef struct OctaveNoise { int octaves; PerlinNoise p[16]; } OctaveNoise;
static void octave_init(OctaveNoise *o, JavaRandom *r, int count) { o->octaves = count; for (int i = 0; i < count && i < 16; i++) perlin_init(&o->p[i], r); }
static double octave_noise2(OctaveNoise *o, double x, double z) { double v=0.0, f=1.0; for(int i=0;i<o->octaves;i++){ v += perlin_noise2(&o->p[i], x*f, z*f) / f; f /= 2.0; } return v; }
static double *octave_fill3(OctaveNoise *o, double *out, int *cap, double x,double y,double z,int xs,int ys,int zs,double sx,double sy,double sz) {
    int n = xs*ys*zs;
    if (!out || *cap < n) { free(out); out = (double*)malloc(sizeof(double)*n); *cap = n; }
    for (int i=0;i<n;i++) out[i]=0.0;
    double amp=1.0;
    for(int i=0;i<o->octaves;i++){ perlin_fill(&o->p[i], out, x,y,z,xs,ys,zs, sx*amp, sy*amp, sz*amp, amp); amp/=2.0; }
    return out;
}
static double *octave_fill2_bridge(OctaveNoise *o, double *out, int *cap, int x,int z,int xs,int zs,double sx,double sz,double dummy) {
    (void)dummy;
    return octave_fill3(o, out, cap, (double)x, 10.0, (double)z, xs, 1, zs, sx, 1.0, sz);
}

/* ay.java / nu.java */
typedef struct SimplexNoise { int p[512]; double xo, yo, zo; } SimplexNoise;
static const int simplex_grad[12][3] = {{1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},{1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},{0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}};
static int simplex_floor(double v) { return v > 0.0 ? (int)v : (int)v - 1; }
static double simplex_dot(const int *g, double x, double y) { return g[0]*x + g[1]*y; }
static void simplex_init(SimplexNoise *n, JavaRandom *r) {
    n->xo = jr_next_double(r) * 256.0;
    n->yo = jr_next_double(r) * 256.0;
    n->zo = jr_next_double(r) * 256.0;
    for (int i=0;i<256;i++) n->p[i]=i;
    for (int i=0;i<256;i++) { int j=jr_next_int_bound(r,256-i)+i; int t=n->p[i]; n->p[i]=n->p[j]; n->p[j]=t; n->p[i+256]=n->p[i]; }
}
static void simplex_fill(SimplexNoise *n, double *out, double x0, double z0, int xs, int zs, double sx, double sz, double amp) {
    const double F2 = 0.5 * (1.7320508075688772935 - 1.0);
    const double G2 = (3.0 - 1.7320508075688772935) / 6.0;
    int idx=0;
    for (int x=0;x<xs;x++) {
        double X=(x0+x)*sx+n->xo;
        for(int z=0;z<zs;z++) {
            double Y=(z0+z)*sz+n->yo;
            double s=(X+Y)*F2;
            int i=simplex_floor(X+s), j=simplex_floor(Y+s);
            double t=(i+j)*G2;
            double X0=i-t, Y0=j-t;
            double x0p=X-X0, y0p=Y-Y0;
            int i1, j1; if(x0p>y0p){i1=1;j1=0;} else {i1=0;j1=1;}
            double x1=x0p-i1+G2, y1=y0p-j1+G2;
            double x2=x0p-1.0+2.0*G2, y2=y0p-1.0+2.0*G2;
            int ii=i&255, jj=j&255;
            int gi0=n->p[ii+n->p[jj]]%12;
            int gi1=n->p[ii+i1+n->p[jj+j1]]%12;
            int gi2=n->p[ii+1+n->p[jj+1]]%12;
            double n0,n1,n2;
            double tt=0.5-x0p*x0p-y0p*y0p;
            if(tt<0.0)n0=0.0; else {tt*=tt; n0=tt*tt*simplex_dot(simplex_grad[gi0],x0p,y0p);} 
            tt=0.5-x1*x1-y1*y1;
            if(tt<0.0)n1=0.0; else {tt*=tt; n1=tt*tt*simplex_dot(simplex_grad[gi1],x1,y1);} 
            tt=0.5-x2*x2-y2*y2;
            if(tt<0.0)n2=0.0; else {tt*=tt; n2=tt*tt*simplex_dot(simplex_grad[gi2],x2,y2);} 
            out[idx++] += 70.0 * (n0+n1+n2) * amp;
        }
    }
}
typedef struct SimplexOctaves { int octaves; SimplexNoise s[4]; } SimplexOctaves;
static void simplex_octaves_init(SimplexOctaves *o, int64_t seed, int count) { JavaRandom r; jr_set_seed(&r, seed); o->octaves=count; for(int i=0;i<count && i<4;i++) simplex_init(&o->s[i], &r); }
static double *simplex_octaves_fill(SimplexOctaves *o, double *out, int *cap, double x,double z,int xs,int zs,double sx,double sz,double persistence,double scaleMul) {
    sx /= 1.5; sz /= 1.5;
    int n=xs*zs; if(!out||*cap<n){free(out); out=(double*)malloc(sizeof(double)*n); *cap=n;}
    for(int i=0;i<n;i++) out[i]=0.0;
    double amp=1.0, freq=1.0;
    for(int i=0;i<o->octaves;i++){ simplex_fill(&o->s[i],out,x,z,xs,zs,sx*freq,sz*freq,0.55/amp); freq*=persistence; amp*=scaleMul; }
    return out;
}


/* -------------------------------------------------------------------------
   Minecraft Java 1.2.5 biome pipeline and biome data.

   This replaces the legacy Beta temperature/humidity lookup for base terrain
   generation.  Layer order, seeds, biome IDs, and biome min/max heights match
   the 1.2.5 GenLayer + WorldChunkManager + ChunkProviderGenerate path.
   ------------------------------------------------------------------------- */

typedef enum WorldBiomeId {
    BIOME_OCEAN = 0,
    BIOME_PLAINS = 1,
    BIOME_DESERT = 2,
    BIOME_EXTREME_HILLS = 3,
    BIOME_FOREST = 4,
    BIOME_TAIGA = 5,
    BIOME_SWAMPLAND = 6,
    BIOME_RIVER = 7,
    BIOME_HELL = 8,
    BIOME_SKY = 9,
    BIOME_FROZEN_OCEAN = 10,
    BIOME_FROZEN_RIVER = 11,
    BIOME_ICE_PLAINS = 12,
    BIOME_ICE_MOUNTAINS = 13,
    BIOME_MUSHROOM_ISLAND = 14,
    BIOME_MUSHROOM_ISLAND_SHORE = 15,
    BIOME_BEACH = 16,
    BIOME_DESERT_HILLS = 17,
    BIOME_FOREST_HILLS = 18,
    BIOME_TAIGA_HILLS = 19,
    BIOME_EXTREME_HILLS_EDGE = 20,
    BIOME_JUNGLE = 21,
    BIOME_JUNGLE_HILLS = 22
} WorldBiomeId;

typedef struct WorldBiome {
    int id;
    unsigned char top;
    unsigned char filler;
    float minHeight;
    float maxHeight;
    float temperature;
    float rainfall;
    unsigned char enableSnow;
    int treesPerChunk;
    int flowersPerChunk;
    int grassPerChunk;
    int deadBushPerChunk;
    int reedsPerChunk;
    int cactiPerChunk;
    int sandPerChunk;
    int sandPerChunk2;
    int clayPerChunk;
    int waterlilyPerChunk;
    int mushroomsPerChunk;
    int bigMushroomsPerChunk;
} WorldBiome;

static WorldBiome world_biome_list[256];
static int world_biomes_ready = 0;

static void world_biome_set(int id, unsigned char top, unsigned char filler,
                            float minH, float maxH, float temp, float rain,
                            int snow) {
    WorldBiome *b = &world_biome_list[id];
    memset(b, 0, sizeof(*b));
    b->id = id;
    b->top = top;
    b->filler = filler;
    b->minHeight = minH;
    b->maxHeight = maxH;
    b->temperature = temp;
    b->rainfall = rain;
    b->enableSnow = (unsigned char)(snow ? 1 : 0);
    b->treesPerChunk = 0;
    b->flowersPerChunk = 2;
    b->grassPerChunk = 1;
    b->sandPerChunk = 1;
    b->sandPerChunk2 = 3;
    b->clayPerChunk = 1;
}

static void world_biomes_init(void) {
    if(world_biomes_ready) return;
    for(int i=0;i<256;i++) world_biome_set(i, BLK_GRASS, BLK_DIRT, 0.1f, 0.3f, 0.5f, 0.5f, 0);

    world_biome_set(BIOME_OCEAN, BLK_GRASS, BLK_DIRT, -1.0f, 0.4f, 0.5f, 0.5f, 0);
    world_biome_set(BIOME_PLAINS, BLK_GRASS, BLK_DIRT, 0.1f, 0.3f, 0.8f, 0.4f, 0);
    world_biome_set(BIOME_DESERT, BLK_SAND, BLK_SAND, 0.1f, 0.2f, 2.0f, 0.0f, 0);
    world_biome_set(BIOME_EXTREME_HILLS, BLK_GRASS, BLK_DIRT, 0.2f, 1.3f, 0.2f, 0.3f, 0);
    world_biome_set(BIOME_FOREST, BLK_GRASS, BLK_DIRT, 0.1f, 0.3f, 0.7f, 0.8f, 0);
    world_biome_set(BIOME_TAIGA, BLK_GRASS, BLK_DIRT, 0.1f, 0.4f, 0.05f, 0.8f, 1);
    world_biome_set(BIOME_SWAMPLAND, BLK_GRASS, BLK_DIRT, -0.2f, 0.1f, 0.8f, 0.9f, 0);
    world_biome_set(BIOME_RIVER, BLK_GRASS, BLK_DIRT, -0.5f, 0.0f, 0.5f, 0.5f, 0);
    world_biome_set(BIOME_HELL, BLK_GRASS, BLK_DIRT, 0.1f, 0.3f, 2.0f, 0.0f, 0);
    world_biome_set(BIOME_SKY, BLK_DIRT, BLK_DIRT, 0.1f, 0.3f, 0.5f, 0.5f, 0);
    world_biome_set(BIOME_FROZEN_OCEAN, BLK_GRASS, BLK_DIRT, -1.0f, 0.5f, 0.0f, 0.5f, 1);
    world_biome_set(BIOME_FROZEN_RIVER, BLK_GRASS, BLK_DIRT, -0.5f, 0.0f, 0.0f, 0.5f, 1);
    world_biome_set(BIOME_ICE_PLAINS, BLK_GRASS, BLK_DIRT, 0.1f, 0.3f, 0.0f, 0.5f, 1);
    world_biome_set(BIOME_ICE_MOUNTAINS, BLK_GRASS, BLK_DIRT, 0.2f, 1.2f, 0.0f, 0.5f, 1);
    world_biome_set(BIOME_MUSHROOM_ISLAND, BLK_MYCELIUM, BLK_DIRT, 0.2f, 1.0f, 0.9f, 1.0f, 0);
    world_biome_set(BIOME_MUSHROOM_ISLAND_SHORE, BLK_MYCELIUM, BLK_DIRT, -1.0f, 0.1f, 0.9f, 1.0f, 0);
    world_biome_set(BIOME_BEACH, BLK_SAND, BLK_SAND, 0.0f, 0.1f, 0.8f, 0.4f, 0);
    world_biome_set(BIOME_DESERT_HILLS, BLK_SAND, BLK_SAND, 0.2f, 0.7f, 2.0f, 0.0f, 0);
    world_biome_set(BIOME_FOREST_HILLS, BLK_GRASS, BLK_DIRT, 0.2f, 0.6f, 0.7f, 0.8f, 0);
    world_biome_set(BIOME_TAIGA_HILLS, BLK_GRASS, BLK_DIRT, 0.2f, 0.7f, 0.05f, 0.8f, 1);
    world_biome_set(BIOME_EXTREME_HILLS_EDGE, BLK_GRASS, BLK_DIRT, 0.2f, 0.8f, 0.2f, 0.3f, 0);
    world_biome_set(BIOME_JUNGLE, BLK_GRASS, BLK_DIRT, 0.2f, 0.4f, 1.2f, 0.9f, 0);
    world_biome_set(BIOME_JUNGLE_HILLS, BLK_GRASS, BLK_DIRT, 1.8f, 0.2f, 1.2f, 0.9f, 0);

    world_biome_list[BIOME_DESERT].treesPerChunk = -999;
    world_biome_list[BIOME_DESERT].deadBushPerChunk = 2;
    world_biome_list[BIOME_DESERT].reedsPerChunk = 50;
    world_biome_list[BIOME_DESERT].cactiPerChunk = 10;
    world_biome_list[BIOME_DESERT_HILLS] = world_biome_list[BIOME_DESERT];
    world_biome_list[BIOME_DESERT_HILLS].id = BIOME_DESERT_HILLS;
    world_biome_list[BIOME_DESERT_HILLS].minHeight = 0.2f;
    world_biome_list[BIOME_DESERT_HILLS].maxHeight = 0.7f;

    world_biome_list[BIOME_PLAINS].treesPerChunk = -999;
    world_biome_list[BIOME_PLAINS].flowersPerChunk = 4;
    world_biome_list[BIOME_PLAINS].grassPerChunk = 10;
    world_biome_list[BIOME_FOREST].treesPerChunk = 10;
    world_biome_list[BIOME_FOREST].grassPerChunk = 2;
    world_biome_list[BIOME_FOREST_HILLS].treesPerChunk = 10;
    world_biome_list[BIOME_FOREST_HILLS].grassPerChunk = 2;
    world_biome_list[BIOME_TAIGA].treesPerChunk = 10;
    world_biome_list[BIOME_TAIGA].grassPerChunk = 1;
    world_biome_list[BIOME_TAIGA_HILLS].treesPerChunk = 10;
    world_biome_list[BIOME_TAIGA_HILLS].grassPerChunk = 1;
    world_biome_list[BIOME_SWAMPLAND].treesPerChunk = 2;
    world_biome_list[BIOME_SWAMPLAND].flowersPerChunk = -999;
    world_biome_list[BIOME_SWAMPLAND].deadBushPerChunk = 1;
    world_biome_list[BIOME_SWAMPLAND].mushroomsPerChunk = 8;
    world_biome_list[BIOME_SWAMPLAND].reedsPerChunk = 10;
    world_biome_list[BIOME_SWAMPLAND].clayPerChunk = 1;
    world_biome_list[BIOME_SWAMPLAND].waterlilyPerChunk = 4;
    world_biome_list[BIOME_JUNGLE].treesPerChunk = 50;
    world_biome_list[BIOME_JUNGLE].grassPerChunk = 25;
    world_biome_list[BIOME_JUNGLE].flowersPerChunk = 4;
    world_biome_list[BIOME_JUNGLE_HILLS].treesPerChunk = 50;
    world_biome_list[BIOME_JUNGLE_HILLS].grassPerChunk = 25;
    world_biome_list[BIOME_JUNGLE_HILLS].flowersPerChunk = 4;
    world_biome_list[BIOME_MUSHROOM_ISLAND].treesPerChunk = -100;
    world_biome_list[BIOME_MUSHROOM_ISLAND].flowersPerChunk = -100;
    world_biome_list[BIOME_MUSHROOM_ISLAND].grassPerChunk = -100;
    world_biome_list[BIOME_MUSHROOM_ISLAND].mushroomsPerChunk = 1;
    world_biome_list[BIOME_MUSHROOM_ISLAND].bigMushroomsPerChunk = 1;
    world_biome_list[BIOME_BEACH].treesPerChunk = -999;
    world_biome_list[BIOME_BEACH].deadBushPerChunk = 0;
    world_biomes_ready = 1;
}

typedef enum GenLayerType {
    GL_ISLAND,
    GL_FUZZY_ZOOM,
    GL_ZOOM,
    GL_ADD_ISLAND,
    GL_ADD_SNOW,
    GL_BIOME,
    GL_HILLS,
    GL_RIVER_INIT,
    GL_RIVER,
    GL_RIVER_MIX,
    GL_SHORE,
    GL_SMOOTH,
    GL_SWAMP_RIVERS,
    GL_VORONOI_ZOOM,
    GL_ADD_MUSHROOM_ISLAND
} GenLayerType;

typedef struct GenLayer GenLayer;
struct GenLayer {
    GenLayerType type;
    GenLayer *parent;
    GenLayer *parent2;
    int64_t worldGenSeed;
    int64_t chunkSeed;
    int64_t baseSeed;
};

static int64_t genlayer_step_seed(int64_t seed, int64_t add) {
    uint64_t s = (uint64_t)seed;
    uint64_t v = s * (s * UINT64_C(6364136223846793005) + UINT64_C(1442695040888963407));
    v += (uint64_t)add;
    return (int64_t)v;
}
static int64_t genlayer_base_seed(int64_t seed) {
    int64_t s = seed;
    s = genlayer_step_seed(s, seed);
    s = genlayer_step_seed(s, seed);
    s = genlayer_step_seed(s, seed);
    return s;
}
static void genlayer_ctor(GenLayer *g, GenLayerType type, int64_t seed, GenLayer *p1, GenLayer *p2) {
    memset(g, 0, sizeof(*g));
    g->type = type;
    g->parent = p1;
    g->parent2 = p2;
    g->baseSeed = genlayer_base_seed(seed);
}
static void genlayer_init_world_seed(GenLayer *g, int64_t seed) {
    if(!g) return;
    if(g->type == GL_RIVER_MIX) {
        genlayer_init_world_seed(g->parent, seed);
        genlayer_init_world_seed(g->parent2, seed);
    } else if(g->parent) {
        genlayer_init_world_seed(g->parent, seed);
    }
    g->worldGenSeed = seed;
    g->worldGenSeed = genlayer_step_seed(g->worldGenSeed, g->baseSeed);
    g->worldGenSeed = genlayer_step_seed(g->worldGenSeed, g->baseSeed);
    g->worldGenSeed = genlayer_step_seed(g->worldGenSeed, g->baseSeed);
}
static void genlayer_init_chunk_seed(GenLayer *g, int64_t x, int64_t z) {
    g->chunkSeed = g->worldGenSeed;
    g->chunkSeed = genlayer_step_seed(g->chunkSeed, x);
    g->chunkSeed = genlayer_step_seed(g->chunkSeed, z);
    g->chunkSeed = genlayer_step_seed(g->chunkSeed, x);
    g->chunkSeed = genlayer_step_seed(g->chunkSeed, z);
}
static int genlayer_next_int(GenLayer *g, int bound) {
    int64_t shifted = g->chunkSeed >> 24;
    int v = (int)(shifted % (int64_t)bound);
    if(v < 0) v += bound;
    g->chunkSeed = genlayer_step_seed(g->chunkSeed, g->worldGenSeed);
    return v;
}
static int *genlayer_alloc(int n) {
    int *p = (int*)malloc(sizeof(int) * (size_t)n);
    if(!p) ExitProcess(2);
    return p;
}
static int genlayer_choose2(GenLayer *g, int a, int b) { return genlayer_next_int(g,2) == 0 ? a : b; }
static int genlayer_choose4_fuzzy(GenLayer *g, int a, int b, int c, int d) {
    int v = genlayer_next_int(g,4);
    return v == 0 ? a : (v == 1 ? b : (v == 2 ? c : d));
}
static int genlayer_choose4_zoom(GenLayer *g, int a, int b, int c, int d) {
    if(b == c && c == d) return b;
    if(a == b && a == c) return a;
    if(a == b && a == d) return a;
    if(a == c && a == d) return a;
    if(a == b && c != d) return a;
    if(a == c && b != d) return a;
    if(a == d && b != c) return a;
    if(b == a && c != d) return b;
    if(b == c && a != d) return b;
    if(b == d && a != c) return b;
    if(c == a && b != d) return c;
    if(c == b && a != d) return c;
    if(c == d && a != b) return c;
    if(d == a && b != c) return c;
    if(d == b && a != c) return c;
    if(d == c && a != b) return c;
    return genlayer_choose4_fuzzy(g,a,b,c,d);
}

static int *genlayer_get_ints(GenLayer *g, int x, int z, int w, int h);

static int *genlayer_island(GenLayer *g, int x, int z, int w, int h) {
    int *out = genlayer_alloc(w*h);
    for(int dz=0; dz<h; dz++) for(int dx=0; dx<w; dx++) {
        genlayer_init_chunk_seed(g, (int64_t)(x + dx), (int64_t)(z + dz));
        out[dx + dz*w] = genlayer_next_int(g,10) == 0 ? 1 : 0;
    }
    if(x > -w && x <= 0 && z > -h && z <= 0) out[-x + -z * w] = 1;
    return out;
}
static int *genlayer_zoom_common(GenLayer *g, int x, int z, int w, int h, int fuzzy) {
    int px = x >> 1, pz = z >> 1;
    int pw = (w >> 1) + 3, ph = (h >> 1) + 3;
    int *par = genlayer_get_ints(g->parent, px, pz, pw, ph);
    int tw = pw << 1, th = ph << 1;
    int *tmp = genlayer_alloc(tw * th);
    for(int row=0; row<ph-1; row++) {
        int ty = row << 1;
        int ti = ty * tw;
        int v00 = par[(row + 0) * pw];
        int v01 = par[(row + 1) * pw];
        for(int col=0; col<pw-1; col++) {
            genlayer_init_chunk_seed(g, (int64_t)((col + px) << 1), (int64_t)((row + pz) << 1));
            int v10 = par[col + 1 + row * pw];
            int v11 = par[col + 1 + (row + 1) * pw];
            tmp[ti] = v00;
            tmp[ti++ + tw] = genlayer_choose2(g, v00, v01);
            tmp[ti] = genlayer_choose2(g, v00, v10);
            tmp[ti++ + tw] = fuzzy ? genlayer_choose4_fuzzy(g, v00, v10, v01, v11) : genlayer_choose4_zoom(g, v00, v10, v01, v11);
            v00 = v10;
            v01 = v11;
        }
    }
    int *out = genlayer_alloc(w*h);
    for(int row=0; row<h; row++) memcpy(out + row*w, tmp + (row + (z & 1)) * tw + (x & 1), sizeof(int)*(size_t)w);
    free(tmp); free(par);
    return out;
}
static int *genlayer_add_island(GenLayer *g, int x, int z, int w, int h) {
    int px=x-1, pz=z-1, pw=w+2, ph=h+2;
    int *par=genlayer_get_ints(g->parent,px,pz,pw,ph);
    int *out=genlayer_alloc(w*h);
    for(int dz=0; dz<h; dz++) for(int dx=0; dx<w; dx++) {
        int nw=par[dx + (dz+0)*pw], ne=par[dx+2 + (dz+0)*pw], sw=par[dx + (dz+2)*pw], se=par[dx+2 + (dz+2)*pw], c=par[dx+1 + (dz+1)*pw];
        genlayer_init_chunk_seed(g, (int64_t)(dx+x), (int64_t)(dz+z));
        if(c != 0 || (nw == 0 && ne == 0 && sw == 0 && se == 0)) {
            if(c > 0 && (nw == 0 || ne == 0 || sw == 0 || se == 0)) {
                if(genlayer_next_int(g,5) == 0) out[dx+dz*w] = (c == BIOME_ICE_PLAINS) ? BIOME_FROZEN_OCEAN : 0;
                else out[dx+dz*w] = c;
            } else out[dx+dz*w] = c;
        } else {
            int choices=1, chosen=1;
            if(nw != 0 && genlayer_next_int(g, choices++) == 0) chosen = nw;
            if(ne != 0 && genlayer_next_int(g, choices++) == 0) chosen = ne;
            if(sw != 0 && genlayer_next_int(g, choices++) == 0) chosen = sw;
            if(se != 0 && genlayer_next_int(g, choices++) == 0) chosen = se;
            if(genlayer_next_int(g,3) == 0) out[dx+dz*w] = chosen;
            else out[dx+dz*w] = (chosen == BIOME_ICE_PLAINS) ? BIOME_FROZEN_OCEAN : 0;
        }
    }
    free(par); return out;
}
static int *genlayer_add_snow(GenLayer *g, int x, int z, int w, int h) {
    int px=x-1, pz=z-1, pw=w+2, ph=h+2;
    int *par=genlayer_get_ints(g->parent,px,pz,pw,ph);
    int *out=genlayer_alloc(w*h);
    for(int dz=0; dz<h; dz++) for(int dx=0; dx<w; dx++) {
        int c=par[dx+1 + (dz+1)*pw];
        genlayer_init_chunk_seed(g, (int64_t)(dx+x), (int64_t)(dz+z));
        if(c == 0) out[dx+dz*w] = 0;
        else { int v=genlayer_next_int(g,5); out[dx+dz*w] = (v == 0) ? BIOME_ICE_PLAINS : 1; }
    }
    free(par); return out;
}
static int *genlayer_biome_layer(GenLayer *g, int x, int z, int w, int h) {
    static const int allowed[] = {BIOME_DESERT, BIOME_FOREST, BIOME_EXTREME_HILLS, BIOME_SWAMPLAND, BIOME_PLAINS, BIOME_TAIGA, BIOME_JUNGLE};
    int *par=genlayer_get_ints(g->parent,x,z,w,h);
    int *out=genlayer_alloc(w*h);
    for(int dz=0; dz<h; dz++) for(int dx=0; dx<w; dx++) {
        genlayer_init_chunk_seed(g, (int64_t)(dx+x), (int64_t)(dz+z));
        int v=par[dx+dz*w];
        if(v == 0) out[dx+dz*w]=0;
        else if(v == BIOME_MUSHROOM_ISLAND) out[dx+dz*w]=v;
        else if(v == 1) out[dx+dz*w]=allowed[genlayer_next_int(g, (int)(sizeof(allowed)/sizeof(allowed[0])))];
        else out[dx+dz*w]=BIOME_ICE_PLAINS;
    }
    free(par); return out;
}
static int *genlayer_hills(GenLayer *g, int x, int z, int w, int h) {
    int pw=w+2, ph=h+2;
    int *par=genlayer_get_ints(g->parent,x-1,z-1,pw,ph);
    int *out=genlayer_alloc(w*h);
    for(int dz=0; dz<h; dz++) for(int dx=0; dx<w; dx++) {
        genlayer_init_chunk_seed(g,(int64_t)(dx+x),(int64_t)(dz+z));
        int c=par[dx+1 + (dz+1)*pw];
        if(genlayer_next_int(g,3)==0) {
            int hill=c;
            if(c==BIOME_DESERT) hill=BIOME_DESERT_HILLS;
            else if(c==BIOME_FOREST) hill=BIOME_FOREST_HILLS;
            else if(c==BIOME_TAIGA) hill=BIOME_TAIGA_HILLS;
            else if(c==BIOME_PLAINS) hill=BIOME_FOREST;
            else if(c==BIOME_ICE_PLAINS) hill=BIOME_ICE_MOUNTAINS;
            else if(c==BIOME_JUNGLE) hill=BIOME_JUNGLE_HILLS;
            if(hill != c) {
                int n=par[dx+1 + dz*pw], e=par[dx+2 + (dz+1)*pw], ww=par[dx + (dz+1)*pw], s=par[dx+1 + (dz+2)*pw];
                out[dx+dz*w] = (n==c && e==c && ww==c && s==c) ? hill : c;
            } else out[dx+dz*w]=c;
        } else out[dx+dz*w]=c;
    }
    free(par); return out;
}
static int *genlayer_river_init(GenLayer *g, int x, int z, int w, int h) {
    int *par=genlayer_get_ints(g->parent,x,z,w,h);
    int *out=genlayer_alloc(w*h);
    for(int dz=0; dz<h; dz++) for(int dx=0; dx<w; dx++) { genlayer_init_chunk_seed(g,(int64_t)(dx+x),(int64_t)(dz+z)); out[dx+dz*w] = par[dx+dz*w] > 0 ? genlayer_next_int(g,2)+2 : 0; }
    free(par); return out;
}
static int *genlayer_river(GenLayer *g, int x, int z, int w, int h) {
    int px=x-1,pz=z-1,pw=w+2,ph=h+2;
    int *par=genlayer_get_ints(g->parent,px,pz,pw,ph);
    int *out=genlayer_alloc(w*h);
    for(int dz=0; dz<h; dz++) for(int dx=0; dx<w; dx++) {
        int west=par[dx + (dz+1)*pw], east=par[dx+2 + (dz+1)*pw], north=par[dx+1 + dz*pw], south=par[dx+1 + (dz+2)*pw], c=par[dx+1 + (dz+1)*pw];
        if(c != 0 && west != 0 && east != 0 && north != 0 && south != 0) out[dx+dz*w] = (c==west && c==north && c==east && c==south) ? -1 : BIOME_RIVER;
        else out[dx+dz*w] = BIOME_RIVER;
    }
    free(par); return out;
}
static int *genlayer_river_mix(GenLayer *g, int x, int z, int w, int h) {
    int *biome=genlayer_get_ints(g->parent,x,z,w,h);
    int *river=genlayer_get_ints(g->parent2,x,z,w,h);
    int *out=genlayer_alloc(w*h);
    for(int i=0;i<w*h;i++) {
        if(biome[i] == BIOME_OCEAN) out[i] = biome[i];
        else if(river[i] >= 0) {
            if(biome[i] == BIOME_ICE_PLAINS) out[i] = BIOME_FROZEN_RIVER;
            else if(biome[i] != BIOME_MUSHROOM_ISLAND && biome[i] != BIOME_MUSHROOM_ISLAND_SHORE) out[i] = river[i];
            else out[i] = BIOME_MUSHROOM_ISLAND_SHORE;
        } else out[i] = biome[i];
    }
    free(biome); free(river); return out;
}
static int *genlayer_shore(GenLayer *g, int x, int z, int w, int h) {
    int pw=w+2, ph=h+2;
    int *par=genlayer_get_ints(g->parent,x-1,z-1,pw,ph);
    int *out=genlayer_alloc(w*h);
    for(int dz=0; dz<h; dz++) for(int dx=0; dx<w; dx++) {
        genlayer_init_chunk_seed(g,(int64_t)(dx+x),(int64_t)(dz+z));
        int c=par[dx+1 + (dz+1)*pw];
        if(c == BIOME_MUSHROOM_ISLAND) {
            int n=par[dx+1 + dz*pw], e=par[dx+2 + (dz+1)*pw], ww=par[dx + (dz+1)*pw], s=par[dx+1 + (dz+2)*pw];
            out[dx+dz*w] = (n != BIOME_OCEAN && e != BIOME_OCEAN && ww != BIOME_OCEAN && s != BIOME_OCEAN) ? c : BIOME_MUSHROOM_ISLAND_SHORE;
        } else if(c != BIOME_OCEAN && c != BIOME_RIVER && c != BIOME_SWAMPLAND && c != BIOME_EXTREME_HILLS) {
            int n=par[dx+1 + dz*pw], e=par[dx+2 + (dz+1)*pw], ww=par[dx + (dz+1)*pw], s=par[dx+1 + (dz+2)*pw];
            out[dx+dz*w] = (n != BIOME_OCEAN && e != BIOME_OCEAN && ww != BIOME_OCEAN && s != BIOME_OCEAN) ? c : BIOME_BEACH;
        } else if(c == BIOME_EXTREME_HILLS) {
            int n=par[dx+1 + dz*pw], e=par[dx+2 + (dz+1)*pw], ww=par[dx + (dz+1)*pw], s=par[dx+1 + (dz+2)*pw];
            out[dx+dz*w] = (n == BIOME_EXTREME_HILLS && e == BIOME_EXTREME_HILLS && ww == BIOME_EXTREME_HILLS && s == BIOME_EXTREME_HILLS) ? c : BIOME_EXTREME_HILLS_EDGE;
        } else out[dx+dz*w] = c;
    }
    free(par); return out;
}
static int *genlayer_smooth(GenLayer *g, int x, int z, int w, int h) {
    int px=x-1,pz=z-1,pw=w+2,ph=h+2;
    int *par=genlayer_get_ints(g->parent,px,pz,pw,ph);
    int *out=genlayer_alloc(w*h);
    for(int dz=0; dz<h; dz++) for(int dx=0; dx<w; dx++) {
        int west=par[dx + (dz+1)*pw], east=par[dx+2 + (dz+1)*pw], north=par[dx+1 + dz*pw], south=par[dx+1 + (dz+2)*pw], c=par[dx+1 + (dz+1)*pw];
        if(west == east && north == south) { genlayer_init_chunk_seed(g,(int64_t)(dx+x),(int64_t)(dz+z)); c = genlayer_next_int(g,2)==0 ? west : north; }
        else { if(west == east) c = west; if(north == south) c = north; }
        out[dx+dz*w] = c;
    }
    free(par); return out;
}
static int *genlayer_swamp_rivers(GenLayer *g, int x, int z, int w, int h) {
    int pw=w+2, ph=h+2;
    int *par=genlayer_get_ints(g->parent,x-1,z-1,pw,ph);
    int *out=genlayer_alloc(w*h);
    for(int dz=0; dz<h; dz++) for(int dx=0; dx<w; dx++) {
        genlayer_init_chunk_seed(g,(int64_t)(dx+x),(int64_t)(dz+z));
        int c=par[dx+1 + (dz+1)*pw];
        if(c == BIOME_SWAMPLAND && genlayer_next_int(g,6)==0) out[dx+dz*w] = BIOME_RIVER;
        else if((c == BIOME_JUNGLE || c == BIOME_JUNGLE_HILLS) && genlayer_next_int(g,8)==0) out[dx+dz*w] = BIOME_RIVER;
        else out[dx+dz*w] = c;
    }
    free(par); return out;
}
static int *genlayer_voronoi(GenLayer *g, int x, int z, int w, int h) {
    x -= 2; z -= 2;
    const int shift = 2, scale = 1 << shift;
    int px = x >> shift, pz = z >> shift;
    int pw = (w >> shift) + 3, ph = (h >> shift) + 3;
    int *par = genlayer_get_ints(g->parent, px, pz, pw, ph);
    int tw = pw << shift, th = ph << shift;
    int *tmp = genlayer_alloc(tw * th);
    for(int row=0; row<ph-1; row++) {
        int v00 = par[(row+0)*pw];
        int v01 = par[(row+1)*pw];
        for(int col=0; col<pw-1; col++) {
            double jitter = (double)scale * 0.9;
            genlayer_init_chunk_seed(g,(int64_t)((col + px) << shift),(int64_t)((row + pz) << shift));
            double x00=((double)genlayer_next_int(g,1024)/1024.0 - 0.5) * jitter;
            double z00=((double)genlayer_next_int(g,1024)/1024.0 - 0.5) * jitter;
            genlayer_init_chunk_seed(g,(int64_t)((col + px + 1) << shift),(int64_t)((row + pz) << shift));
            double x10=((double)genlayer_next_int(g,1024)/1024.0 - 0.5) * jitter + (double)scale;
            double z10=((double)genlayer_next_int(g,1024)/1024.0 - 0.5) * jitter;
            genlayer_init_chunk_seed(g,(int64_t)((col + px) << shift),(int64_t)((row + pz + 1) << shift));
            double x01=((double)genlayer_next_int(g,1024)/1024.0 - 0.5) * jitter;
            double z01=((double)genlayer_next_int(g,1024)/1024.0 - 0.5) * jitter + (double)scale;
            genlayer_init_chunk_seed(g,(int64_t)((col + px + 1) << shift),(int64_t)((row + pz + 1) << shift));
            double x11=((double)genlayer_next_int(g,1024)/1024.0 - 0.5) * jitter + (double)scale;
            double z11=((double)genlayer_next_int(g,1024)/1024.0 - 0.5) * jitter + (double)scale;
            int v10 = par[col+1 + row*pw];
            int v11 = par[col+1 + (row+1)*pw];
            for(int dz=0; dz<scale; dz++) {
                int ti = ((row << shift) + dz) * tw + (col << shift);
                for(int dx=0; dx<scale; dx++) {
                    double d00=((double)dz-z00)*((double)dz-z00)+((double)dx-x00)*((double)dx-x00);
                    double d10=((double)dz-z10)*((double)dz-z10)+((double)dx-x10)*((double)dx-x10);
                    double d01=((double)dz-z01)*((double)dz-z01)+((double)dx-x01)*((double)dx-x01);
                    double d11=((double)dz-z11)*((double)dz-z11)+((double)dx-x11)*((double)dx-x11);
                    if(d00 < d10 && d00 < d01 && d00 < d11) tmp[ti++] = v00;
                    else if(d10 < d00 && d10 < d01 && d10 < d11) tmp[ti++] = v10;
                    else if(d01 < d00 && d01 < d10 && d01 < d11) tmp[ti++] = v01;
                    else tmp[ti++] = v11;
                }
            }
            v00 = v10; v01 = v11;
        }
    }
    int *out=genlayer_alloc(w*h);
    for(int row=0; row<h; row++) memcpy(out + row*w, tmp + (row + (z & (scale-1))) * tw + (x & (scale-1)), sizeof(int)*(size_t)w);
    free(tmp); free(par); return out;
}
static int *genlayer_add_mushroom(GenLayer *g, int x, int z, int w, int h) {
    int px=x-1,pz=z-1,pw=w+2,ph=h+2;
    int *par=genlayer_get_ints(g->parent,px,pz,pw,ph);
    int *out=genlayer_alloc(w*h);
    for(int dz=0; dz<h; dz++) for(int dx=0; dx<w; dx++) {
        int nw=par[dx + dz*pw], ne=par[dx+2 + dz*pw], sw=par[dx + (dz+2)*pw], se=par[dx+2 + (dz+2)*pw], c=par[dx+1 + (dz+1)*pw];
        genlayer_init_chunk_seed(g,(int64_t)(dx+x),(int64_t)(dz+z));
        out[dx+dz*w] = (c == 0 && nw == 0 && ne == 0 && sw == 0 && se == 0 && genlayer_next_int(g,100)==0) ? BIOME_MUSHROOM_ISLAND : c;
    }
    free(par); return out;
}
static int *genlayer_get_ints(GenLayer *g, int x, int z, int w, int h) {
    switch(g->type) {
        case GL_ISLAND: return genlayer_island(g,x,z,w,h);
        case GL_FUZZY_ZOOM: return genlayer_zoom_common(g,x,z,w,h,1);
        case GL_ZOOM: return genlayer_zoom_common(g,x,z,w,h,0);
        case GL_ADD_ISLAND: return genlayer_add_island(g,x,z,w,h);
        case GL_ADD_SNOW: return genlayer_add_snow(g,x,z,w,h);
        case GL_BIOME: return genlayer_biome_layer(g,x,z,w,h);
        case GL_HILLS: return genlayer_hills(g,x,z,w,h);
        case GL_RIVER_INIT: return genlayer_river_init(g,x,z,w,h);
        case GL_RIVER: return genlayer_river(g,x,z,w,h);
        case GL_RIVER_MIX: return genlayer_river_mix(g,x,z,w,h);
        case GL_SHORE: return genlayer_shore(g,x,z,w,h);
        case GL_SMOOTH: return genlayer_smooth(g,x,z,w,h);
        case GL_SWAMP_RIVERS: return genlayer_swamp_rivers(g,x,z,w,h);
        case GL_VORONOI_ZOOM: return genlayer_voronoi(g,x,z,w,h);
        case GL_ADD_MUSHROOM_ISLAND: return genlayer_add_mushroom(g,x,z,w,h);
    }
    return genlayer_alloc(w*h);
}

typedef struct BiomeManager {
    GenLayer layers[64];
    int layer_count;
    GenLayer *gen_biomes;
    GenLayer *gen_voronoi;
    WorldBiome biomes[256];
    WorldBiome genBiomes[128];
    double *temp;
    int tempCap;
} BiomeManager;

static GenLayer *biome_manager_new_layer(BiomeManager *bm, GenLayerType type, int64_t seed, GenLayer *p1, GenLayer *p2) {
    if(bm->layer_count >= (int)(sizeof(bm->layers)/sizeof(bm->layers[0]))) ExitProcess(2);
    GenLayer *g = &bm->layers[bm->layer_count++];
    genlayer_ctor(g, type, seed, p1, p2);
    return g;
}
static GenLayer *biome_manager_zoom_many(BiomeManager *bm, int64_t seed, GenLayer *parent, int count) {
    GenLayer *g = parent;
    for(int i=0; i<count; i++) g = biome_manager_new_layer(bm, GL_ZOOM, seed + (int64_t)i, g, NULL);
    return g;
}
static void biome_manager_init(BiomeManager *bm, int64_t worldSeed) {
    world_biomes_init();
    memset(bm, 0, sizeof(*bm));
    GenLayer *var3 = biome_manager_new_layer(bm, GL_ISLAND, 1, NULL, NULL);
    GenLayer *var9 = biome_manager_new_layer(bm, GL_FUZZY_ZOOM, 2000, var3, NULL);
    GenLayer *var10 = biome_manager_new_layer(bm, GL_ADD_ISLAND, 1, var9, NULL);
    GenLayer *var11 = biome_manager_new_layer(bm, GL_ZOOM, 2001, var10, NULL);
    var10 = biome_manager_new_layer(bm, GL_ADD_ISLAND, 2, var11, NULL);
    GenLayer *var12 = biome_manager_new_layer(bm, GL_ADD_SNOW, 2, var10, NULL);
    var11 = biome_manager_new_layer(bm, GL_ZOOM, 2002, var12, NULL);
    var10 = biome_manager_new_layer(bm, GL_ADD_ISLAND, 3, var11, NULL);
    var11 = biome_manager_new_layer(bm, GL_ZOOM, 2003, var10, NULL);
    var10 = biome_manager_new_layer(bm, GL_ADD_ISLAND, 4, var11, NULL);
    GenLayer *var15 = biome_manager_new_layer(bm, GL_ADD_MUSHROOM_ISLAND, 5, var10, NULL);
    int var4 = 4;
    GenLayer *var5 = biome_manager_zoom_many(bm, 1000, var15, 0);
    GenLayer *var13 = biome_manager_new_layer(bm, GL_RIVER_INIT, 100, var5, NULL);
    var5 = biome_manager_zoom_many(bm, 1000, var13, var4 + 2);
    GenLayer *var14 = biome_manager_new_layer(bm, GL_RIVER, 1, var5, NULL);
    GenLayer *var16 = biome_manager_new_layer(bm, GL_SMOOTH, 1000, var14, NULL);
    GenLayer *var6 = biome_manager_zoom_many(bm, 1000, var15, 0);
    GenLayer *var17 = biome_manager_new_layer(bm, GL_BIOME, 200, var6, NULL);
    var6 = biome_manager_zoom_many(bm, 1000, var17, 2);
    GenLayer *var18 = biome_manager_new_layer(bm, GL_HILLS, 1000, var6, NULL);
    for(int var7=0; var7<var4; var7++) {
        var18 = biome_manager_new_layer(bm, GL_ZOOM, 1000 + var7, var18, NULL);
        if(var7 == 0) var18 = biome_manager_new_layer(bm, GL_ADD_ISLAND, 3, var18, NULL);
        if(var7 == 1) var18 = biome_manager_new_layer(bm, GL_SHORE, 1000, var18, NULL);
        if(var7 == 1) var18 = biome_manager_new_layer(bm, GL_SWAMP_RIVERS, 1000, var18, NULL);
    }
    GenLayer *var19 = biome_manager_new_layer(bm, GL_SMOOTH, 1000, var18, NULL);
    GenLayer *var20 = biome_manager_new_layer(bm, GL_RIVER_MIX, 100, var19, var16);
    GenLayer *var8 = biome_manager_new_layer(bm, GL_VORONOI_ZOOM, 10, var20, NULL);
    genlayer_init_world_seed(var20, worldSeed);
    genlayer_init_world_seed(var8, worldSeed);
    bm->gen_biomes = var20;
    bm->gen_voronoi = var8;
}
static WorldBiome *biome_manager_get_generation(BiomeManager *bm, int x, int z, int xs, int zs) {
    int n=xs*zs;
    int *ids = genlayer_get_ints(bm->gen_biomes, x, z, xs, zs);
    WorldBiome *dst = n <= (int)(sizeof(bm->genBiomes)/sizeof(bm->genBiomes[0])) ? bm->genBiomes : bm->biomes;
    for(int i=0;i<n;i++) dst[i] = world_biome_list[(ids[i] >= 0 && ids[i] < 256) ? ids[i] : 0];
    free(ids);
    return dst;
}
static WorldBiome *biome_manager_get(BiomeManager *bm, int x, int z, int xs, int zs) {
    int n=xs*zs;
    int *ids = genlayer_get_ints(bm->gen_voronoi, x, z, xs, zs);
    for(int i=0;i<n && i<256;i++) bm->biomes[i] = world_biome_list[(ids[i] >= 0 && ids[i] < 256) ? ids[i] : 0];
    free(ids);
    return bm->biomes;
}
static double *biome_manager_temperatures(BiomeManager *bm, double *out, int *cap, int x, int z, int xs, int zs) {
    int n=xs*zs;
    if(!out || *cap < n) { free(out); out=(double*)malloc(sizeof(double)*(size_t)n); if(!out) ExitProcess(2); *cap=n; }
    int *ids = genlayer_get_ints(bm->gen_voronoi, x, z, xs, zs);
    for(int i=0;i<n;i++) out[i] = world_biome_list[(ids[i] >= 0 && ids[i] < 256) ? ids[i] : 0].temperature;
    free(ids);
    return out;
}
static void biome_manager_write_biome_array(BiomeManager *bm, int cx, int cz, unsigned char *out256) {
    int *ids = genlayer_get_ints(bm->gen_voronoi, cx*16, cz*16, 16, 16);
    for(int i=0;i<256;i++) out256[i] = (unsigned char)(ids[i] & 255);
    free(ids);
}

static int worldgen_is_stronghold_biome(int id) {
    return id == BIOME_DESERT || id == BIOME_FOREST || id == BIOME_EXTREME_HILLS || id == BIOME_SWAMPLAND ||
           id == BIOME_TAIGA || id == BIOME_ICE_PLAINS || id == BIOME_ICE_MOUNTAINS ||
           id == BIOME_DESERT_HILLS || id == BIOME_FOREST_HILLS || id == BIOME_EXTREME_HILLS_EDGE ||
           id == BIOME_JUNGLE || id == BIOME_JUNGLE_HILLS;
}
static int worldgen_is_village_biome(int id) { return id == BIOME_PLAINS || id == BIOME_DESERT; }

static int biome_manager_find_biome_position(BiomeManager *bm, int x, int z, int radius, int (*allowed)(int), JavaRandom *rand, int *outX, int *outZ) {
    int minX = (x - radius) >> 2;
    int minZ = (z - radius) >> 2;
    int maxX = (x + radius) >> 2;
    int maxZ = (z + radius) >> 2;
    int w = maxX - minX + 1;
    int h = maxZ - minZ + 1;
    int *ids = genlayer_get_ints(bm->gen_biomes, minX, minZ, w, h);
    int found = 0, count = 0, rx = 0, rz = 0;
    for (int i = 0; i < w * h; ++i) {
        int wx = (minX + i % w) << 2;
        int wz = (minZ + i / w) << 2;
        int id = ids[i];
        if (allowed(id) && (!found || jr_next_int_bound(rand, count + 1) == 0)) {
            rx = wx;
            rz = wz;
            found = 1;
            ++count;
        }
    }
    free(ids);
    if (found) { if (outX) *outX = rx; if (outZ) *outZ = rz; }
    return found;
}

int worldgen_get_stronghold_coords(long long seed, int out_chunk_x[3], int out_chunk_z[3]) {
    BiomeManager bm;
    memset(&bm, 0, sizeof(bm));
    world_biomes_init();
    biome_manager_init(&bm, (int64_t)seed);
    JavaRandom rand;
    jr_set_seed(&rand, (int64_t)seed);
    double angle = jr_next_double(&rand) * M_PI * 2.0;
    for (int i = 0; i < 3; ++i) {
        double dist = (1.25 + jr_next_double(&rand)) * 32.0;
        int cx = beta_floor_double(cos(angle) * dist + 0.5);
        int cz = beta_floor_double(sin(angle) * dist + 0.5);
        int bx, bz;
        if (biome_manager_find_biome_position(&bm, (cx << 4) + 8, (cz << 4) + 8, 112, worldgen_is_stronghold_biome, &rand, &bx, &bz)) {
            cx = bx >> 4;
            cz = bz >> 4;
        }
        if (out_chunk_x) out_chunk_x[i] = cx;
        if (out_chunk_z) out_chunk_z[i] = cz;
        angle += M_PI * 2.0 / 3.0;
    }
    free(bm.temp);
    return 3;
}

static JavaRandom worldgen_set_random_seed(long long seed, int x, int z, int salt) {
    JavaRandom r;
    int64_t s = (int64_t)x * INT64_C(341873128712) + (int64_t)z * INT64_C(132897987541) + (int64_t)seed + (int64_t)salt;
    jr_set_seed(&r, s);
    return r;
}

int worldgen_village_can_spawn(long long seed, int chunkX, int chunkZ) {
    int spacing = 32, separation = 8;
    int ox = chunkX, oz = chunkZ;
    if (chunkX < 0) chunkX -= spacing - 1;
    if (chunkZ < 0) chunkZ -= spacing - 1;
    int gridX = chunkX / spacing;
    int gridZ = chunkZ / spacing;
    JavaRandom r = worldgen_set_random_seed(seed, gridX, gridZ, 10387312);
    int candX = gridX * spacing + jr_next_int_bound(&r, spacing - separation);
    int candZ = gridZ * spacing + jr_next_int_bound(&r, spacing - separation);
    if (ox != candX || oz != candZ) return 0;
    BiomeManager bm;
    memset(&bm, 0, sizeof(bm));
    world_biomes_init();
    biome_manager_init(&bm, (int64_t)seed);
    int qx = (ox * 16 + 8) >> 2;
    int qz = (oz * 16 + 8) >> 2;
    int *ids = genlayer_get_ints(bm.gen_biomes, qx, qz, 1, 1);
    int ok = ids && worldgen_is_village_biome(ids[0]);
    free(ids);
    free(bm.temp);
    return ok;
}

int worldgen_mineshaft_can_spawn(long long seed, int chunkX, int chunkZ) {
    JavaRandom r;
    jr_set_seed(&r, (int64_t)seed);
    int64_t mulX = jr_next_long(&r);
    int64_t mulZ = jr_next_long(&r);
    int64_t s = (int64_t)chunkX * mulX ^ (int64_t)chunkZ * mulZ ^ (int64_t)seed;
    jr_set_seed(&r, s);
    jr_next_int(&r); /* MapGenStructure.recursiveGenerate discard */
    int dist = abs(chunkX) > abs(chunkZ) ? abs(chunkX) : abs(chunkZ);
    return jr_next_int_bound(&r, 100) == 0 && jr_next_int_bound(&r, 80) < dist;
}

typedef struct TerrainProvider {
    int64_t worldSeed;
    JavaRandom rand;
    OctaveNoise k,l,m,n,o,a,b,c;
    BiomeManager biomeManager;
    WorldBiome *densityBiomes;
    double *q,*r,*s,*t,*d,*e,*f,*g,*h,*snow;
    float biomeWeights[25];
    int qCap,rCap,sCap,tCap,dCap,eCap,fCap,gCap,hCap,snowCap;
} TerrainProvider;

static void terrain_provider_init(TerrainProvider *tp, int64_t seed) {
    memset(tp, 0, sizeof(*tp));
    tp->worldSeed = seed;
    jr_set_seed(&tp->rand, seed);
    octave_init(&tp->k, &tp->rand, 16);
    octave_init(&tp->l, &tp->rand, 16);
    octave_init(&tp->m, &tp->rand, 8);
    octave_init(&tp->n, &tp->rand, 4);
    octave_init(&tp->o, &tp->rand, 4);
    octave_init(&tp->a, &tp->rand, 10);
    octave_init(&tp->b, &tp->rand, 16);
    octave_init(&tp->c, &tp->rand, 8);
    for(int dz=-2; dz<=2; dz++) for(int dx=-2; dx<=2; dx++) tp->biomeWeights[dx+2 + (dz+2)*5] = 10.0f / sqrtf((float)(dx*dx + dz*dz) + 0.2f);
    biome_manager_init(&tp->biomeManager, seed);
}
static void terrain_provider_free(TerrainProvider *tp) {
    free(tp->q); free(tp->r); free(tp->s); free(tp->t); free(tp->d); free(tp->e); free(tp->f); free(tp->g); free(tp->h); free(tp->snow);
    free(tp->biomeManager.temp);
}
static double *qm_density(TerrainProvider *tp, int x, int y, int z, int xs, int ys, int zs) {
    int n = xs*ys*zs;
    if(!tp->q || tp->qCap<n){ free(tp->q); tp->q=(double*)malloc(sizeof(double)*n); if(!tp->q) ExitProcess(2); tp->qCap=n; }
    double scaleX = 684.412, scaleY = 684.412;
    tp->g = octave_fill2_bridge(&tp->a, tp->g, &tp->gCap, x,z,xs,zs,1.121,1.121,0.5);
    tp->h = octave_fill2_bridge(&tp->b, tp->h, &tp->hCap, x,z,xs,zs,200.0,200.0,0.5);
    tp->d = octave_fill3(&tp->m, tp->d, &tp->dCap, x,y,z,xs,ys,zs,scaleX/80.0,scaleY/160.0,scaleX/80.0);
    tp->e = octave_fill3(&tp->k, tp->e, &tp->eCap, x,y,z,xs,ys,zs,scaleX,scaleY,scaleX);
    tp->f = octave_fill3(&tp->l, tp->f, &tp->fCap, x,y,z,xs,ys,zs,scaleX,scaleY,scaleX);
    int outIdx=0, noise2dIdx=0;
    for(int xi=0; xi<xs; xi++) {
        for(int zi=0; zi<zs; zi++) {
            float maxHeight = 0.0f, minHeight = 0.0f, weightSum = 0.0f;
            const WorldBiome *center = &tp->densityBiomes[xi + 2 + (zi + 2) * (xs + 5)];
            for(int bx=-2; bx<=2; bx++) {
                for(int bz=-2; bz<=2; bz++) {
                    const WorldBiome *b = &tp->densityBiomes[xi + bx + 2 + (zi + bz + 2) * (xs + 5)];
                    float weight = tp->biomeWeights[bx + 2 + (bz + 2) * 5] / (b->minHeight + 2.0f);
                    if(b->minHeight > center->minHeight) weight /= 2.0f;
                    maxHeight += b->maxHeight * weight;
                    minHeight += b->minHeight * weight;
                    weightSum += weight;
                }
            }
            maxHeight /= weightSum;
            minHeight /= weightSum;
            maxHeight = maxHeight * 0.9f + 0.1f;
            minHeight = (minHeight * 4.0f - 1.0f) / 8.0f;
            double noiseHeight = tp->h[noise2dIdx] / 8000.0;
            if(noiseHeight < 0.0) noiseHeight = -noiseHeight * 0.3;
            noiseHeight = noiseHeight * 3.0 - 2.0;
            if(noiseHeight < 0.0) {
                noiseHeight /= 2.0;
                if(noiseHeight < -1.0) noiseHeight = -1.0;
                noiseHeight /= 1.4;
                noiseHeight /= 2.0;
            } else {
                if(noiseHeight > 1.0) noiseHeight = 1.0;
                noiseHeight /= 8.0;
            }
            noise2dIdx++;
            for(int yi=0; yi<ys; yi++) {
                double terrainOffset = (double)minHeight;
                double terrainScale = (double)maxHeight;
                terrainOffset += noiseHeight * 0.2;
                terrainOffset = terrainOffset * (double)ys / 16.0;
                double mid = (double)ys / 2.0 + terrainOffset * 4.0;
                double density = 0.0;
                double falloff = ((double)yi - mid) * 12.0 * 128.0 / 128.0 / terrainScale;
                if(falloff < 0.0) falloff *= 4.0;
                double n1 = tp->e[outIdx] / 512.0;
                double n2 = tp->f[outIdx] / 512.0;
                double blend = (tp->d[outIdx] / 10.0 + 1.0) / 2.0;
                if(blend < 0.0) density = n1;
                else if(blend > 1.0) density = n2;
                else density = n1 + (n2 - n1) * blend;
                density -= falloff;
                if(yi > ys - 4) {
                    double topBlend = (double)((float)(yi - (ys - 4)) / 3.0f);
                    density = density * (1.0 - topBlend) + -10.0 * topBlend;
                }
                tp->q[outIdx++] = density;
            }
        }
    }
    return tp->q;
}

static void qm_generate_base(TerrainProvider *tp, int cx, int cz, unsigned char *blocks) {
    int cell = 4;
    int xCells = cell + 1, yCells = 17, zCells = cell + 1;
    tp->densityBiomes = biome_manager_get_generation(&tp->biomeManager, cx*4 - 2, cz*4 - 2, xCells + 5, zCells + 5);
    double *dens = qm_density(tp, cx*cell, 0, cz*cell, xCells, yCells, zCells);
    for(int x=0; x<cell; x++) for(int z=0; z<cell; z++) for(int ycell=0; ycell<16; ycell++) {
        double v000 = dens[((x+0)*zCells + z+0)*yCells + ycell+0];
        double v001 = dens[((x+0)*zCells + z+1)*yCells + ycell+0];
        double v100 = dens[((x+1)*zCells + z+0)*yCells + ycell+0];
        double v101 = dens[((x+1)*zCells + z+1)*yCells + ycell+0];
        double yd000 = (dens[((x+0)*zCells + z+0)*yCells + ycell+1] - v000) * 0.125;
        double yd001 = (dens[((x+0)*zCells + z+1)*yCells + ycell+1] - v001) * 0.125;
        double yd100 = (dens[((x+1)*zCells + z+0)*yCells + ycell+1] - v100) * 0.125;
        double yd101 = (dens[((x+1)*zCells + z+1)*yCells + ycell+1] - v101) * 0.125;
        for(int yy=0; yy<8; yy++) {
            double vx0=v000, vx1=v001;
            double xd0=(v100-v000)*0.25, xd1=(v101-v001)*0.25;
            for(int xx=0; xx<4; xx++) {
                int idx = ((xx + x*4) << 11) | ((z*4) << 7) | (ycell*8 + yy);
                double vz=vx0, zd=(vx1-vx0)*0.25;
                for(int zz=0; zz<4; zz++) {
                    int id = 0;
                    if(vz > 0.0) id = BLK_STONE;
                    else if(ycell*8 + yy < 63) id = BLK_STILL_WATER;
                    blocks[idx] = (unsigned char)id;
                    idx += 128;
                    vz += zd;
                }
                vx0 += xd0; vx1 += xd1;
            }
            v000 += yd000; v001 += yd001; v100 += yd100; v101 += yd101;
        }
    }
}

static void qm_replace_surface(TerrainProvider *tp, int cx, int cz, unsigned char *blocks, WorldBiome *biomes) {
    int sea=63;
    tp->t = octave_fill3(&tp->o, tp->t, &tp->tCap, cx*16, cz*16, 0.0, 16,16,1,0.0625,0.0625,0.0625);
    for(int x=0;x<16;x++) for(int z=0;z<16;z++) {
        WorldBiome *bio = &biomes[z + x*16];
        float temp = bio->temperature;
        int depth = (int)(tp->t[x+z*16] / 3.0 + 3.0 + jr_next_double(&tp->rand) * 0.25);
        int run = -1;
        unsigned char top = bio->top, filler = bio->filler;
        for(int y=127;y>=0;y--) {
            int idx = chunk_index(x,y,z);
            if(y <= jr_next_int_bound(&tp->rand,5)) { blocks[idx] = BLK_BEDROCK; continue; }
            unsigned char id = blocks[idx];
            if(id == 0) run = -1;
            else if(id == BLK_STONE) {
                if(run == -1) {
                    if(depth <= 0) { top = 0; filler = BLK_STONE; }
                    else if(y >= sea-4 && y <= sea+1) { top = bio->top; filler = bio->filler; }
                    if(y < sea && top == 0) top = (temp < 0.15f) ? BLK_ICE : BLK_STILL_WATER;
                    run = depth;
                    blocks[idx] = (y >= sea-1) ? top : filler;
                } else if(run > 0) {
                    run--;
                    blocks[idx] = filler;
                    if(run == 0 && filler == BLK_SAND) { run = jr_next_int_bound(&tp->rand, 4); filler = BLK_SANDSTONE; }
                }
            }
        }
    }
}

/* mp.java / dr.java */
static void cave_carve_node(JavaRandom *globalRand, int chunkX, int chunkZ, unsigned char *blocks, double x, double y, double z, float radius, float yaw, float pitch, int step, int maxStep, double heightScale) {
    double centerX = chunkX * 16 + 8;
    double centerZ = chunkZ * 16 + 8;
    float yawVel=0.0f, pitchVel=0.0f;
    JavaRandom rand; jr_set_seed(&rand, jr_next_long(globalRand));
    if(maxStep <= 0) { int base = 8 * 16 - 16; maxStep = base - jr_next_int_bound(&rand, base/4); }
    int mainBranch = 0;
    if(step == -1) { step = maxStep / 2; mainBranch = 1; }
    int splitStep = jr_next_int_bound(&rand, maxStep/2) + maxStep/4;
    int extraTall = jr_next_int_bound(&rand, 6) == 0;
    while(step < maxStep) {
        double hRadius = 1.5 + beta_sin(step * (float)M_PI / maxStep) * radius;
        double vRadius = hRadius * heightScale;
        float cp = beta_cos(pitch), sp = beta_sin(pitch);
        x += beta_cos(yaw) * cp;
        y += sp;
        z += beta_sin(yaw) * cp;
        pitch *= extraTall ? 0.92f : 0.7f;
        pitch += pitchVel * 0.1f;
        yaw += yawVel * 0.1f;
        pitchVel *= 0.9f; yawVel *= 0.75f;
        pitchVel += (jr_next_float(&rand)-jr_next_float(&rand))*jr_next_float(&rand)*2.0f;
        yawVel += (jr_next_float(&rand)-jr_next_float(&rand))*jr_next_float(&rand)*4.0f;
        if(!mainBranch && step == splitStep && radius > 1.0f) {
            cave_carve_node(globalRand,chunkX,chunkZ,blocks,x,y,z,jr_next_float(&rand)*0.5f+0.5f,yaw-(float)(M_PI/2.0),pitch/3.0f,step,maxStep,1.0);
            cave_carve_node(globalRand,chunkX,chunkZ,blocks,x,y,z,jr_next_float(&rand)*0.5f+0.5f,yaw+(float)(M_PI/2.0),pitch/3.0f,step,maxStep,1.0);
            return;
        }
        if(mainBranch || jr_next_int_bound(&rand,4) != 0) {
            double dx=x-centerX, dz=z-centerZ, remain=maxStep-step, reach=radius+2.0f+16.0f;
            if(dx*dx + dz*dz - remain*remain > reach*reach) return;
            if(!(x < centerX-16.0-hRadius*2.0) && !(z < centerZ-16.0-hRadius*2.0) && !(x > centerX+16.0+hRadius*2.0) && !(z > centerZ+16.0+hRadius*2.0)) {
                int minX=beta_floor_double(x-hRadius)-chunkX*16-1, maxX=beta_floor_double(x+hRadius)-chunkX*16+1;
                int minY=beta_floor_double(y-vRadius)-1, maxY=beta_floor_double(y+vRadius)+1;
                int minZ=beta_floor_double(z-hRadius)-chunkZ*16-1, maxZ=beta_floor_double(z+hRadius)-chunkZ*16+1;
                if(minX<0)minX=0;
                if(maxX>16)maxX=16;
                if(minY<1)minY=1;
                if(maxY>120)maxY=120;
                if(minZ<0)minZ=0;
                if(maxZ>16)maxZ=16;
                int foundLiquid=0;
                for(int xx=minX; !foundLiquid && xx<maxX; xx++) for(int zz=minZ; !foundLiquid && zz<maxZ; zz++) for(int yy=maxY+1; !foundLiquid && yy>=minY-1; yy--) {
                    int idx=chunk_index(xx,yy,zz);
                    if(yy>=0 && yy<128) {
                        if(blocks[idx] == BLK_WATER || blocks[idx] == BLK_STILL_WATER || blocks[idx] == BLK_LAVA || blocks[idx] == BLK_STILL_LAVA) foundLiquid=1;
                        if(yy != minY-1 && xx != minX && xx != maxX-1 && zz != minZ && zz != maxZ-1) yy = minY;
                    }
                }
                if(!foundLiquid) {
                    for(int xx=minX; xx<maxX; xx++) {
                        double nx=(xx+chunkX*16+0.5-x)/hRadius;
                        for(int zz=minZ; zz<maxZ; zz++) {
                            double nz=(zz+chunkZ*16+0.5-z)/hRadius;
                            int idx=chunk_index(xx,maxY,zz);
                            int hitGrass=0;
                            for(int yy=maxY-1; yy>=minY; yy--) {
                                double ny=(yy+0.5-y)/vRadius;
                                if(ny > -0.7 && nx*nx + ny*ny + nz*nz < 1.0) {
                                    unsigned char id=blocks[idx];
                                    if(id == BLK_GRASS) hitGrass=1;
                                    if(id == BLK_STONE || id == BLK_DIRT || id == BLK_GRASS) {
                                        if(yy < 10) blocks[idx] = BLK_STILL_LAVA;
                                        else { blocks[idx] = 0; if(hitGrass && blocks[idx-1] == BLK_DIRT) blocks[idx-1] = BLK_GRASS; }
                                    }
                                }
                                idx--;
                            }
                        }
                    }
                    if(mainBranch) break;
                }
            }
        }
        step++;
    }
}
static void cave_big_room(JavaRandom *globalRand, int chunkX, int chunkZ, unsigned char *blocks, double x, double y, double z) {
    cave_carve_node(globalRand, chunkX, chunkZ, blocks, x, y, z, 1.0f + jr_next_float(globalRand) * 6.0f, 0.0f, 0.0f, -1, -1, 0.5);
}
static void caves_for_source(JavaRandom *rand, int srcX, int srcZ, int chunkX, int chunkZ, unsigned char *blocks) {
    int count = jr_next_int_bound(rand, jr_next_int_bound(rand, jr_next_int_bound(rand, 40) + 1) + 1);
    if(jr_next_int_bound(rand, 15) != 0) count = 0;
    for(int i=0;i<count;i++) {
        double x = srcX*16 + jr_next_int_bound(rand,16);
        double y = jr_next_int_bound(rand, jr_next_int_bound(rand,120)+8);
        double z = srcZ*16 + jr_next_int_bound(rand,16);
        int tunnels=1;
        if(jr_next_int_bound(rand,4) == 0) { cave_big_room(rand, chunkX, chunkZ, blocks, x,y,z); tunnels += jr_next_int_bound(rand,4); }
        for(int j=0;j<tunnels;j++) {
            float yaw = jr_next_float(rand) * (float)M_PI * 2.0f;
            float pitch = (jr_next_float(rand) - 0.5f) * 2.0f / 8.0f;
            float rad = jr_next_float(rand) * 2.0f + jr_next_float(rand);
            cave_carve_node(rand, chunkX, chunkZ, blocks, x,y,z,rad,yaw,pitch,0,0,1.0);
        }
    }
}
static void qm_generate_caves(TerrainProvider *tp, int cx, int cz, unsigned char *blocks) {
    JavaRandom rand; jr_set_seed(&rand, tp->worldSeed);
    int64_t s1 = jr_next_long(&rand);
    int64_t s2 = jr_next_long(&rand);
    for(int sx=cx-8; sx<=cx+8; sx++) for(int sz=cz-8; sz<=cz+8; sz++) {
        int64_t seed = (int64_t)(((uint64_t)(int64_t)sx * (uint64_t)s1) ^ ((uint64_t)(int64_t)sz * (uint64_t)s2) ^ (uint64_t)tp->worldSeed);
        jr_set_seed(&rand, seed);
        caves_for_source(&rand, sx, sz, cx, cz, blocks);
    }
}


static void ravine_carve_node(TerrainProvider *tp, JavaRandom *seedRand, int chunkX, int chunkZ, unsigned char *blocks,
                              double x, double y, double z, float radius, float yaw, float pitch,
                              int step, int maxStep, double heightScale) {
    JavaRandom rand;
    jr_set_seed(&rand, jr_next_long(seedRand));
    double centerX = chunkX * 16 + 8;
    double centerZ = chunkZ * 16 + 8;
    float yawVel = 0.0f, pitchVel = 0.0f;
    if(maxStep <= 0) {
        int base = 8 * 16 - 16;
        maxStep = base - jr_next_int_bound(&rand, base / 4);
    }
    int mainBranch = 0;
    if(step == -1) { step = maxStep / 2; mainBranch = 1; }
    float widthScale = 1.0f;
    float widths[128];
    for(int i=0; i<128; i++) {
        if(i == 0 || jr_next_int_bound(&rand, 3) == 0) widthScale = 1.0f + jr_next_float(&rand) * jr_next_float(&rand);
        widths[i] = widthScale * widthScale;
    }
    WorldBiome *chunkBiomes = biome_manager_get(&tp->biomeManager, chunkX*16, chunkZ*16, 16, 16);
    for(; step < maxStep; step++) {
        double hRadius = 1.5 + (double)(beta_sin((float)step * (float)M_PI / (float)maxStep) * radius);
        double vRadius = hRadius * heightScale;
        hRadius *= (double)jr_next_float(&rand) * 0.25 + 0.75;
        vRadius *= (double)jr_next_float(&rand) * 0.25 + 0.75;
        float cp = beta_cos(pitch), sp = beta_sin(pitch);
        x += (double)(beta_cos(yaw) * cp);
        y += (double)sp;
        z += (double)(beta_sin(yaw) * cp);
        pitch *= 0.7f;
        pitch += pitchVel * 0.05f;
        yaw += yawVel * 0.05f;
        pitchVel *= 0.8f;
        yawVel *= 0.5f;
        pitchVel += (jr_next_float(&rand) - jr_next_float(&rand)) * jr_next_float(&rand) * 2.0f;
        yawVel += (jr_next_float(&rand) - jr_next_float(&rand)) * jr_next_float(&rand) * 4.0f;
        if(mainBranch || jr_next_int_bound(&rand,4) != 0) {
            double dx = x - centerX, dz = z - centerZ;
            double remain = (double)(maxStep - step);
            double reach = (double)(radius + 2.0f + 16.0f);
            if(dx*dx + dz*dz - remain*remain > reach*reach) return;
            if(x >= centerX - 16.0 - hRadius*2.0 && z >= centerZ - 16.0 - hRadius*2.0 && x <= centerX + 16.0 + hRadius*2.0 && z <= centerZ + 16.0 + hRadius*2.0) {
                int minX = beta_floor_double(x - hRadius) - chunkX*16 - 1;
                int maxX = beta_floor_double(x + hRadius) - chunkX*16 + 1;
                int minY = beta_floor_double(y - vRadius) - 1;
                int maxY = beta_floor_double(y + vRadius) + 1;
                int minZ = beta_floor_double(z - hRadius) - chunkZ*16 - 1;
                int maxZ = beta_floor_double(z + hRadius) - chunkZ*16 + 1;
                if(minX < 0) minX = 0;
                if(maxX > 16) maxX = 16;
                if(minY < 1) minY = 1;
                if(maxY > 120) maxY = 120;
                if(minZ < 0) minZ = 0;
                if(maxZ > 16) maxZ = 16;
                int foundWater = 0;
                for(int xx=minX; !foundWater && xx<maxX; xx++) for(int zz=minZ; !foundWater && zz<maxZ; zz++) for(int yy=maxY+1; !foundWater && yy>=minY-1; yy--) {
                    if(yy >= 0 && yy < 128) {
                        unsigned char id = blocks[chunk_index(xx,yy,zz)];
                        if(id == BLK_WATER || id == BLK_STILL_WATER) foundWater = 1;
                        if(yy != minY-1 && xx != minX && xx != maxX-1 && zz != minZ && zz != maxZ-1) yy = minY;
                    }
                }
                if(!foundWater) {
                    for(int xx=minX; xx<maxX; xx++) {
                        double nx = ((double)(xx + chunkX*16) + 0.5 - x) / hRadius;
                        for(int zz=minZ; zz<maxZ; zz++) {
                            double nz = ((double)(zz + chunkZ*16) + 0.5 - z) / hRadius;
                            int idx = chunk_index(xx,maxY,zz);
                            int hitGrass = 0;
                            if(nx*nx + nz*nz < 1.0) {
                                for(int yy=maxY-1; yy>=minY; yy--) {
                                    double ny = ((double)yy + 0.5 - y) / vRadius;
                                    if((nx*nx + nz*nz) * (double)widths[yy] + ny*ny / 6.0 < 1.0) {
                                        unsigned char id = blocks[idx];
                                        if(id == BLK_GRASS) hitGrass = 1;
                                        if(id == BLK_STONE || id == BLK_DIRT || id == BLK_GRASS) {
                                            if(yy < 10) blocks[idx] = BLK_LAVA;
                                            else {
                                                blocks[idx] = 0;
                                                if(hitGrass && idx > 0 && blocks[idx-1] == BLK_DIRT) blocks[idx-1] = chunkBiomes[zz + xx*16].top;
                                            }
                                        }
                                    }
                                    idx--;
                                }
                            }
                        }
                    }
                    if(mainBranch) break;
                }
            }
        }
    }
}
static void ravine_for_source(TerrainProvider *tp, JavaRandom *rand, int srcX, int srcZ, int chunkX, int chunkZ, unsigned char *blocks) {
    if(jr_next_int_bound(rand,50) == 0) {
        double x = (double)(srcX * 16 + jr_next_int_bound(rand,16));
        double y = (double)(jr_next_int_bound(rand, jr_next_int_bound(rand,40) + 8) + 20);
        double z = (double)(srcZ * 16 + jr_next_int_bound(rand,16));
        float yaw = jr_next_float(rand) * (float)M_PI * 2.0f;
        float pitch = (jr_next_float(rand) - 0.5f) * 2.0f / 8.0f;
        float radius = (jr_next_float(rand) * 2.0f + jr_next_float(rand)) * 2.0f;
        ravine_carve_node(tp, rand, chunkX, chunkZ, blocks, x, y, z, radius, yaw, pitch, 0, 0, 3.0);
    }
}
static void qm_generate_ravines(TerrainProvider *tp, int cx, int cz, unsigned char *blocks) {
    JavaRandom rand; jr_set_seed(&rand, tp->worldSeed);
    int64_t s1 = jr_next_long(&rand);
    int64_t s2 = jr_next_long(&rand);
    for(int sx=cx-8; sx<=cx+8; sx++) for(int sz=cz-8; sz<=cz+8; sz++) {
        int64_t seed = (int64_t)(((uint64_t)(int64_t)sx * (uint64_t)s1) ^ ((uint64_t)(int64_t)sz * (uint64_t)s2) ^ (uint64_t)tp->worldSeed);
        jr_set_seed(&rand, seed);
        ravine_for_source(tp, &rand, sx, sz, cx, cz, blocks);
    }
}

static int highest_solid(unsigned char *blocks, int x, int z) { for(int y=127;y>=0;y--) if(get_block_local(blocks,x,y,z)!=0) return y; return 0; }
static int can_tree_grow(unsigned char *blocks, int x, int y, int z, int h) {
    if(y < 1 || y+h+1 > 128) return 0;
    unsigned char soil = get_block_local(blocks,x,y-1,z);
    if(soil != BLK_GRASS && soil != BLK_DIRT) return 0;
    for(int yy=y; yy<=y+1+h; yy++) {
        int rad = (yy == y) ? 0 : (yy >= y+h-1 ? 2 : 1);
        for(int xx=x-rad; xx<=x+rad; xx++) for(int zz=z-rad; zz<=z+rad; zz++) {
            if(xx<0||xx>=16||zz<0||zz>=16||yy<0||yy>=128) return 0;
            unsigned char id=get_block_local(blocks,xx,yy,zz);
            if(id != 0 && id != BLK_LEAVES) return 0;
        }
    }
    return 1;
}
static void place_beta_tree(unsigned char *blocks, JavaRandom *r, int x, int y, int z) {
    int h = jr_next_int_bound(r,3) + 4;
    if(!can_tree_grow(blocks,x,y,z,h)) return;
    set_block_local(blocks,x,y-1,z,BLK_DIRT);
    for(int yy=y-3+h; yy<=y+h; yy++) {
        int layer = yy - (y+h);
        int rad = 1 - layer/2;
        for(int xx=x-rad; xx<=x+rad; xx++) for(int zz=z-rad; zz<=z+rad; zz++) {
            int ax = xx-x, az = zz-z;
            if(abs(ax) != rad || abs(az) != rad || (jr_next_int_bound(r,2) != 0 && layer != 0)) {
                if(get_block_local(blocks,xx,yy,zz)==0) set_block_local(blocks,xx,yy,zz,BLK_LEAVES);
            }
        }
    }
    for(int yy=0; yy<h; yy++) {
        unsigned char id=get_block_local(blocks,x,y+yy,z);
        if(id==0 || id==BLK_LEAVES) set_block_local(blocks,x,y+yy,z,BLK_LOG);
    }
}
static void place_ore_vein(unsigned char *blocks, JavaRandom *r, int id, int count, int x, int y, int z) {
    float angle = jr_next_float(r) * (float)M_PI;
    double x1 = x + 8 + beta_sin(angle) * count / 8.0f;
    double x2 = x + 8 - beta_sin(angle) * count / 8.0f;
    double z1 = z + 8 + beta_cos(angle) * count / 8.0f;
    double z2 = z + 8 - beta_cos(angle) * count / 8.0f;
    double y1 = y + jr_next_int_bound(r,3) + 2;
    double y2 = y + jr_next_int_bound(r,3) + 2;
    for(int i=0;i<=count;i++) {
        double px=x1+(x2-x1)*i/count, py=y1+(y2-y1)*i/count, pz=z1+(z2-z1)*i/count;
        double rnd = jr_next_double(r) * count / 16.0;
        double sx=(sin(i*M_PI/count)+1.0)*rnd + 1.0;
        double sy=(sin(i*M_PI/count)+1.0)*rnd + 1.0;
        int minX=(int)(px-sx/2.0), minY=(int)(py-sy/2.0), minZ=(int)(pz-sx/2.0);
        int maxX=(int)(px+sx/2.0), maxY=(int)(py+sy/2.0), maxZ=(int)(pz+sx/2.0);
        for(int xx=minX;xx<=maxX;xx++) for(int yy=minY;yy<=maxY;yy++) for(int zz=minZ;zz<=maxZ;zz++) {
            if(xx>=0&&xx<16&&yy>=0&&yy<128&&zz>=0&&zz<16) {
                double dx=(xx+0.5-px)/(sx/2.0), dy=(yy+0.5-py)/(sy/2.0), dz=(zz+0.5-pz)/(sx/2.0);
                if(dx*dx+dy*dy+dz*dz < 1.0 && get_block_local(blocks,xx,yy,zz)==BLK_STONE) set_block_local(blocks,xx,yy,zz,(unsigned char)id);
            }
        }
    }
}
static void place_flower(unsigned char *blocks, JavaRandom *r, int id, int x, int y, int z) {
    for(int i=0;i<64;i++) {
        int xx=x+jr_next_int_bound(r,8)-jr_next_int_bound(r,8), yy=y+jr_next_int_bound(r,4)-jr_next_int_bound(r,4), zz=z+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        if(xx>=0&&xx<16&&yy>0&&yy<128&&zz>=0&&zz<16 && get_block_local(blocks,xx,yy,zz)==0 && get_block_local(blocks,xx,yy-1,zz)==BLK_GRASS) set_block_local(blocks,xx,yy,zz,(unsigned char)id);
    }
}

/* 3x3 population canvas. Java populate() intentionally starts features at x/z+8,
   so a chunk can be modified by its north/west neighbor.  The canvas lets the
   target chunk receive those cross-border writes instead of clipping them. */
typedef struct GenCanvas {
    int minCx, minCz, chunks;
    unsigned char *blocks;
    /* Byte-per-block staging metadata. It is packed into Java NBT Data[]
       nibbles when the target chunk is extracted. Keeping this parallel to
       blocks makes later 1.2.5 tree/structure ports able to write log, leaf,
       stair, chest, vine, frame, and crop metadata without changing the canvas
       API again. */
    unsigned char *meta;
} GenCanvas;
static int wg_floor_div16(int v) { return v >= 0 ? v / 16 : -(((-v) + 15) / 16); }
static int canvas_chunk_slot(GenCanvas *cv, int cx, int cz) { return (cz - cv->minCz) * cv->chunks + (cx - cv->minCx); }
static int canvas_block_index(GenCanvas *cv, int wx, int y, int wz) {
    int cx = wg_floor_div16(wx), cz = wg_floor_div16(wz);
    if(cx < cv->minCx || cz < cv->minCz || cx >= cv->minCx + cv->chunks || cz >= cv->minCz + cv->chunks || y < 0 || y >= 128) return -1;
    int lx = wx - cx * 16; if(lx < 0) lx += 16;
    int lz = wz - cz * 16; if(lz < 0) lz += 16;
    return canvas_chunk_slot(cv, cx, cz) * 32768 + chunk_index(lx, y, lz);
}
static unsigned char canvas_get(GenCanvas *cv, int wx, int y, int wz) {
    int idx = canvas_block_index(cv, wx, y, wz);
    return idx >= 0 ? cv->blocks[idx] : 0;
}
static void canvas_set_with_meta(GenCanvas *cv, int wx, int y, int wz, unsigned char id, unsigned char meta) {
    int idx = canvas_block_index(cv, wx, y, wz);
    if(idx >= 0) {
        cv->blocks[idx] = id;
        if(cv->meta) cv->meta[idx] = (unsigned char)(meta & 15);
    }
}
static void canvas_set(GenCanvas *cv, int wx, int y, int wz, unsigned char id) {
    canvas_set_with_meta(cv, wx, y, wz, id, 0);
}
static unsigned char canvas_get_meta(GenCanvas *cv, int wx, int y, int wz) {
    int idx = canvas_block_index(cv, wx, y, wz);
    return (idx >= 0 && cv->meta) ? (unsigned char)(cv->meta[idx] & 15) : 0;
}
static int cv_is_liquid_id(int id) { return id == BLK_STILL_WATER || id == BLK_WATER || id == BLK_LAVA || id == BLK_STILL_LAVA; }
static int cv_is_water_id(int id) { return id == BLK_STILL_WATER || id == BLK_WATER; }
static int cv_is_air(GenCanvas *cv, int x, int y, int z) { return canvas_get(cv,x,y,z) == 0; }
static int cv_is_solid_id(int id) {
    return id != 0 && id != BLK_STILL_WATER && id != BLK_WATER && id != BLK_LAVA && id != BLK_STILL_LAVA && id != BLK_LEAVES &&
           id != BLK_SNOW_LAYER && id != BLK_YELLOW_FLOWER && id != BLK_RED_ROSE && id != BLK_BROWN_MUSHROOM &&
           id != BLK_RED_MUSHROOM && id != BLK_REEDS && id != BLK_TALL_GRASS &&
           id != BLK_DEAD_BUSH && id != BLK_VINE && id != BLK_LILY_PAD;
}

/* WorldGenTrees uses Block.opaqueCubeLookup, while World.getHeightValue uses
   Block.lightOpacity.  Java BlockLeavesBase.isOpaqueCube() is false, but leaves
   still have non-zero light opacity.  Keep these Java rules separate from the
   C renderer/material helpers so population stays aligned with Source B without
   changing unrelated gameplay checks. */
static int cv_tree_opaque_lookup_id(int id) {
    if(id == 0) return 0;
    if(id == BLK_LEAVES) return 0;
    if(cv_is_liquid_id(id) || id == BLK_GLASS || id == BLK_ICE || id == BLK_SNOW_LAYER ||
       id == BLK_YELLOW_FLOWER || id == BLK_RED_ROSE || id == BLK_BROWN_MUSHROOM ||
       id == BLK_RED_MUSHROOM || id == BLK_REEDS) return 0;
    return 1;
}
static int cv_heightmap_light_blocks_id(int id) {
    if(id == 0) return 0;
    if(id == BLK_SNOW_LAYER || id == BLK_YELLOW_FLOWER || id == BLK_RED_ROSE ||
       id == BLK_BROWN_MUSHROOM || id == BLK_RED_MUSHROOM || id == BLK_REEDS ||
       id == BLK_TALL_GRASS || id == BLK_DEAD_BUSH || id == BLK_VINE || id == BLK_LILY_PAD) return 0;
    return 1; /* leaves, liquids and ice all have non-zero Java lightOpacity */
}
static int cv_opaque_id(int id) { return cv_tree_opaque_lookup_id(id); }
static int canvas_top_solid(GenCanvas *cv, int wx, int wz) {
    for(int y=127;y>=0;y--) {
        if(cv_heightmap_light_blocks_id(canvas_get(cv, wx, y, wz))) return y + 1;
    }
    return 0;
}

static int cv_blocks_movement_for_top_id(int id) {
    if(id == 0 || id == BLK_LEAVES || cv_is_liquid_id(id) || id == BLK_SNOW_LAYER ||
       id == BLK_YELLOW_FLOWER || id == BLK_RED_ROSE || id == BLK_BROWN_MUSHROOM ||
       id == BLK_RED_MUSHROOM || id == BLK_REEDS || id == BLK_TALL_GRASS ||
       id == BLK_DEAD_BUSH || id == BLK_VINE || id == BLK_LILY_PAD) return 0;
    return 1;
}

static int canvas_top_solid_or_liquid(GenCanvas *cv, int wx, int wz) {
    /* Java 1.2.5 World.getTopSolidOrLiquidBlock actually returns the first
       y above a movement-blocking block, excluding leaves.  Under shallow
       water this is the water block directly above the seabed, which is what
       WorldGenSand/WorldGenClay expect. */
    for(int y=127; y>0; --y) {
        if(cv_blocks_movement_for_top_id(canvas_get(cv, wx, y, wz))) return y + 1;
    }
    return -1;
}

static int canvas_snow_surface_y(GenCanvas *cv, int wx, int wz) {
    /* World.findTopSolidBlock from b1.0 returns the first solid-or-liquid
       material plus one.  Leaves count as solid material there, so snow may
       settle on tree canopies in cold biomes. */
    for(int y=127; y>0; y--) {
        int id = canvas_get(cv, wx, y, wz);
        if(id != 0 && (cv_is_liquid_id(id) || id == BLK_LEAVES || cv_is_solid_id(id))) return y + 1;
    }
    return -1;
}

static int canvas_can_snow_stay_at(GenCanvas *cv, int wx, int y, int wz) {
    int below = canvas_get(cv, wx, y - 1, wz);
    if(below == 0 || below == BLK_ICE || cv_is_liquid_id(below)) return 0;
    if(below == BLK_LEAVES) return 1;
    return cv_is_solid_id(below);
}
static int canvas_can_flower_stay(GenCanvas *cv, int id, int x, int y, int z) {
    int below = canvas_get(cv, x, y - 1, z);
    if(id == BLK_BROWN_MUSHROOM || id == BLK_RED_MUSHROOM) return cv_is_solid_id(below);
    return below == BLK_GRASS || below == BLK_DIRT;
}
static int canvas_can_reed_stay(GenCanvas *cv, int x, int y, int z) {
    int below = canvas_get(cv, x, y - 1, z);
    if(below == BLK_REEDS) return 1;
    if(below != BLK_GRASS && below != BLK_DIRT && below != BLK_SAND) return 0;
    return cv_is_water_id(canvas_get(cv,x-1,y-1,z)) || cv_is_water_id(canvas_get(cv,x+1,y-1,z)) ||
           cv_is_water_id(canvas_get(cv,x,y-1,z-1)) || cv_is_water_id(canvas_get(cv,x,y-1,z+1));
}
static int canvas_can_cactus_stay(GenCanvas *cv, int x, int y, int z) {
    if(canvas_get(cv,x-1,y,z) || canvas_get(cv,x+1,y,z) || canvas_get(cv,x,y,z-1) || canvas_get(cv,x,y,z+1)) return 0;
    int below = canvas_get(cv, x, y - 1, z);
    return below == BLK_CACTUS || below == BLK_SAND;
}
static void canvas_place_minable(GenCanvas *cv, JavaRandom *r, int id, int count, int x, int y, int z, int clay) {
    if(clay && !cv_is_water_id(canvas_get(cv,x,y,z))) return;
    float angle = jr_next_float(r) * (float)M_PI;
    double x1 = x + 8 + beta_sin(angle) * count / 8.0f;
    double x2 = x + 8 - beta_sin(angle) * count / 8.0f;
    double z1 = z + 8 + beta_cos(angle) * count / 8.0f;
    double z2 = z + 8 - beta_cos(angle) * count / 8.0f;
    double y1 = y + jr_next_int_bound(r,3) + 2;
    double y2 = y + jr_next_int_bound(r,3) + 2;
    for(int i=0;i<=count;i++) {
        double px=x1+(x2-x1)*i/count, py=y1+(y2-y1)*i/count, pz=z1+(z2-z1)*i/count;
        double rnd = jr_next_double(r) * count / 16.0;
        double sx=(sin(i*M_PI/count)+1.0)*rnd + 1.0;
        double sy=(sin(i*M_PI/count)+1.0)*rnd + 1.0;
        int minX=(int)(px-sx/2.0), minY=(int)(py-sy/2.0), minZ=(int)(pz-sx/2.0);
        int maxX=(int)(px+sx/2.0), maxY=(int)(py+sy/2.0), maxZ=(int)(pz+sx/2.0);
        for(int xx=minX;xx<=maxX;xx++) for(int yy=minY;yy<=maxY;yy++) for(int zz=minZ;zz<=maxZ;zz++) {
            double dx=(xx+0.5-px)/(sx/2.0), dy=(yy+0.5-py)/(sy/2.0), dz=(zz+0.5-pz)/(sx/2.0);
            if(dx*dx+dy*dy+dz*dz < 1.0) {
                int cur = canvas_get(cv,xx,yy,zz);
                if((clay && cur == BLK_SAND) || (!clay && cur == BLK_STONE)) canvas_set(cv,xx,yy,zz,(unsigned char)id);
            }
        }
    }
}

static int canvas_place_sand_or_gravel_patch(GenCanvas *cv, JavaRandom *r, int blockId, int radiusLimit, int x, int y, int z) {
    if(!cv_is_water_id(canvas_get(cv, x, y, z))) return 0;
    int radius = jr_next_int_bound(r, radiusLimit - 2) + 2;
    int yRadius = 2;
    for(int xx = x - radius; xx <= x + radius; ++xx) {
        for(int zz = z - radius; zz <= z + radius; ++zz) {
            int dx = xx - x, dz = zz - z;
            if(dx * dx + dz * dz <= radius * radius) {
                for(int yy = y - yRadius; yy <= y + yRadius; ++yy) {
                    int cur = canvas_get(cv, xx, yy, zz);
                    if(cur == BLK_DIRT || cur == BLK_GRASS) canvas_set(cv, xx, yy, zz, (unsigned char)blockId);
                }
            }
        }
    }
    return 1;
}

static int canvas_place_clay_patch(GenCanvas *cv, JavaRandom *r, int count, int x, int y, int z) {
    if(!cv_is_water_id(canvas_get(cv, x, y, z))) return 0;
    int radius = jr_next_int_bound(r, count - 2) + 2;
    int yRadius = 1;
    for(int xx = x - radius; xx <= x + radius; ++xx) {
        for(int zz = z - radius; zz <= z + radius; ++zz) {
            int dx = xx - x, dz = zz - z;
            if(dx * dx + dz * dz <= radius * radius) {
                for(int yy = y - yRadius; yy <= y + yRadius; ++yy) {
                    int cur = canvas_get(cv, xx, yy, zz);
                    if(cur == BLK_DIRT || cur == BLK_CLAY) canvas_set(cv, xx, yy, zz, BLK_CLAY);
                }
            }
        }
    }
    return 1;
}
static void canvas_place_vine_column(GenCanvas *cv, int x, int y, int z, int meta) {
    int n = 4;
    while(y >= 0 && canvas_get(cv, x, y, z) == 0 && n > 0) {
        canvas_set_with_meta(cv, x, y, z, BLK_VINE, (unsigned char)meta);
        --y;
        --n;
    }
}

static int canvas_place_tree_custom(GenCanvas *cv, JavaRandom *r, int x, int y, int z, int baseHeight, int woodMeta, int leavesMeta, int vines) {
    int h = jr_next_int_bound(r,3) + baseHeight;
    int ok = 1;
    if(y < 1 || y + h + 1 > 128) ok = 0;
    for(int yy=y; ok && yy<=y+1+h; yy++) {
        int rad = (yy == y) ? 0 : (yy >= y+1+h-2 ? 2 : 1);
        for(int xx=x-rad; ok && xx<=x+rad; xx++) for(int zz=z-rad; ok && zz<=z+rad; zz++) {
            if(yy < 0 || yy >= 128) { ok=0; break; }
            int id = canvas_get(cv,xx,yy,zz);
            if(id != 0 && id != BLK_LEAVES && id != BLK_GRASS && id != BLK_DIRT && id != BLK_LOG) ok = 0;
        }
    }
    int soil = canvas_get(cv,x,y-1,z);
    if(!ok || (soil != BLK_GRASS && soil != BLK_DIRT) || y >= 128 - h - 1) return 0;
    canvas_set(cv,x,y-1,z,BLK_DIRT);
    for(int yy=y-3+h; yy<=y+h; yy++) {
        int layer = yy - (y+h);
        int rad = 1 - layer/2;
        for(int xx=x-rad; xx<=x+rad; xx++) for(int zz=z-rad; zz<=z+rad; zz++) {
            int ax=xx-x, az=zz-z;
            if((abs(ax) != rad || abs(az) != rad || (jr_next_int_bound(r,2) != 0 && layer != 0)) && !cv_opaque_id(canvas_get(cv,xx,yy,zz))) {
                canvas_set_with_meta(cv,xx,yy,zz,BLK_LEAVES,(unsigned char)(leavesMeta & 3));
            }
        }
    }
    for(int yy=0; yy<h; yy++) {
        int id = canvas_get(cv,x,y+yy,z);
        if(id == 0 || id == BLK_LEAVES) {
            canvas_set_with_meta(cv,x,y+yy,z,BLK_LOG,(unsigned char)(woodMeta & 3));
            if(vines && yy > 0) {
                if(jr_next_int_bound(r,3) > 0 && canvas_get(cv,x-1,y+yy,z)==0) canvas_set_with_meta(cv,x-1,y+yy,z,BLK_VINE,8);
                if(jr_next_int_bound(r,3) > 0 && canvas_get(cv,x+1,y+yy,z)==0) canvas_set_with_meta(cv,x+1,y+yy,z,BLK_VINE,2);
                if(jr_next_int_bound(r,3) > 0 && canvas_get(cv,x,y+yy,z-1)==0) canvas_set_with_meta(cv,x,y+yy,z-1,BLK_VINE,1);
                if(jr_next_int_bound(r,3) > 0 && canvas_get(cv,x,y+yy,z+1)==0) canvas_set_with_meta(cv,x,y+yy,z+1,BLK_VINE,4);
            }
        }
    }
    if(vines) {
        for(int yy=y-3+h; yy<=y+h; yy++) {
            int layer = yy - (y+h);
            int rad = 2 - layer/2;
            for(int xx=x-rad; xx<=x+rad; xx++) for(int zz=z-rad; zz<=z+rad; zz++) {
                if(canvas_get(cv,xx,yy,zz)==BLK_LEAVES) {
                    if(jr_next_int_bound(r,4)==0 && canvas_get(cv,xx-1,yy,zz)==0) canvas_place_vine_column(cv,xx-1,yy,zz,8);
                    if(jr_next_int_bound(r,4)==0 && canvas_get(cv,xx+1,yy,zz)==0) canvas_place_vine_column(cv,xx+1,yy,zz,2);
                    if(jr_next_int_bound(r,4)==0 && canvas_get(cv,xx,yy,zz-1)==0) canvas_place_vine_column(cv,xx,yy,zz-1,1);
                    if(jr_next_int_bound(r,4)==0 && canvas_get(cv,xx,yy,zz+1)==0) canvas_place_vine_column(cv,xx,yy,zz+1,4);
                }
            }
        }
    }
    return 1;
}

static void canvas_place_tree_typed(GenCanvas *cv, JavaRandom *r, int x, int y, int z, int woodMeta) {
    canvas_place_tree_custom(cv, r, x, y, z, 4, woodMeta, woodMeta, 0);
}
static void canvas_place_tree(GenCanvas *cv, JavaRandom *r, int x, int y, int z) {
    canvas_place_tree_typed(cv, r, x, y, z, 0);
}
static void canvas_place_swamp_tree(GenCanvas *cv, JavaRandom *r, int x, int y, int z) {
    int h = jr_next_int_bound(r, 4) + 5;
    while (y > 1 && cv_is_water_id(canvas_get(cv, x, y - 1, z))) --y;

    int ok = 1;
    if (y < 1 || y + h + 1 > 128) ok = 0;
    for (int yy = y; ok && yy <= y + 1 + h; ++yy) {
        int rad = 1;
        if (yy == y) rad = 0;
        if (yy >= y + 1 + h - 2) rad = 3;
        for (int xx = x - rad; ok && xx <= x + rad; ++xx) {
            for (int zz = z - rad; ok && zz <= z + rad; ++zz) {
                if (yy < 0 || yy >= 128) { ok = 0; break; }
                int id = canvas_get(cv, xx, yy, zz);
                if (id != 0 && id != BLK_LEAVES) {
                    if (!cv_is_water_id(id)) ok = 0;
                    else if (yy > y) ok = 0;
                }
            }
        }
    }
    if (!ok) return;

    int soil = canvas_get(cv, x, y - 1, z);
    if ((soil != BLK_GRASS && soil != BLK_DIRT) || y >= 128 - h - 1) return;
    canvas_set(cv, x, y - 1, z, BLK_DIRT);

    for (int yy = y - 3 + h; yy <= y + h; ++yy) {
        int layer = yy - (y + h);
        int rad = 2 - layer / 2;
        for (int xx = x - rad; xx <= x + rad; ++xx) {
            int dx = xx - x;
            for (int zz = z - rad; zz <= z + rad; ++zz) {
                int dz = zz - z;
                if ((abs(dx) != rad || abs(dz) != rad || (jr_next_int_bound(r, 2) != 0 && layer != 0)) &&
                    !cv_opaque_id(canvas_get(cv, xx, yy, zz))) {
                    canvas_set(cv, xx, yy, zz, BLK_LEAVES);
                }
            }
        }
    }

    for (int yy = 0; yy < h; ++yy) {
        int id = canvas_get(cv, x, y + yy, z);
        if (id == 0 || id == BLK_LEAVES || cv_is_water_id(id)) canvas_set(cv, x, y + yy, z, BLK_LOG);
    }

    for (int yy = y - 3 + h; yy <= y + h; ++yy) {
        int layer = yy - (y + h);
        int rad = 2 - layer / 2;
        for (int xx = x - rad; xx <= x + rad; ++xx) {
            for (int zz = z - rad; zz <= z + rad; ++zz) {
                if (canvas_get(cv, xx, yy, zz) == BLK_LEAVES) {
                    if (jr_next_int_bound(r, 4) == 0 && canvas_get(cv, xx - 1, yy, zz) == 0) canvas_place_vine_column(cv, xx - 1, yy, zz, 8);
                    if (jr_next_int_bound(r, 4) == 0 && canvas_get(cv, xx + 1, yy, zz) == 0) canvas_place_vine_column(cv, xx + 1, yy, zz, 2);
                    if (jr_next_int_bound(r, 4) == 0 && canvas_get(cv, xx, yy, zz - 1) == 0) canvas_place_vine_column(cv, xx, yy, zz - 1, 1);
                    if (jr_next_int_bound(r, 4) == 0 && canvas_get(cv, xx, yy, zz + 1) == 0) canvas_place_vine_column(cv, xx, yy, zz + 1, 4);
                }
            }
        }
    }
}

static int canvas_place_forest_tree(GenCanvas *cv, JavaRandom *r, int x, int y, int z) {
    int h = jr_next_int_bound(r,3) + 5;
    int ok = 1;
    if(y < 1 || y + h + 1 > 128) ok = 0;
    for(int yy=y; ok && yy<=y+1+h; yy++) {
        int rad = (yy == y) ? 0 : (yy >= y+1+h-2 ? 2 : 1);
        for(int xx=x-rad; ok && xx<=x+rad; xx++) for(int zz=z-rad; ok && zz<=z+rad; zz++) {
            if(yy < 0 || yy >= 128) { ok=0; break; }
            int id = canvas_get(cv,xx,yy,zz);
            if(id != 0 && id != BLK_LEAVES) ok = 0;
        }
    }
    int soil = canvas_get(cv,x,y-1,z);
    if(!ok || (soil != BLK_GRASS && soil != BLK_DIRT) || y >= 128 - h - 1) return 0;
    canvas_set(cv,x,y-1,z,BLK_DIRT);
    for(int yy=y-3+h; yy<=y+h; yy++) {
        int layer = yy - (y+h);
        int rad = 1 - layer/2;
        for(int xx=x-rad; xx<=x+rad; xx++) for(int zz=z-rad; zz<=z+rad; zz++) {
            int ax=xx-x, az=zz-z;
            if((abs(ax) != rad || abs(az) != rad || (jr_next_int_bound(r,2) != 0 && layer != 0)) && !cv_opaque_id(canvas_get(cv,xx,yy,zz))) {
                canvas_set_with_meta(cv,xx,yy,zz,BLK_LEAVES,BLOCK_META_WOOD_BIRCH);
            }
        }
    }
    for(int yy=0; yy<h; yy++) {
        int id = canvas_get(cv,x,y+yy,z);
        if(id == 0 || id == BLK_LEAVES) canvas_set_with_meta(cv,x,y+yy,z,BLK_LOG,BLOCK_META_WOOD_BIRCH);
    }
    return 1;
}

static int canvas_place_taiga1(GenCanvas *cv, JavaRandom *r, int x, int y, int z) {
    int h = jr_next_int_bound(r,5) + 7;
    int trunkClear = h - jr_next_int_bound(r,2) - 3;
    int leafHeight = h - trunkClear;
    int maxRadius = 1 + jr_next_int_bound(r, leafHeight + 1);
    int ok = 1;
    if(y < 1 || y + h + 1 > 128) ok = 0;
    for(int yy=y; ok && yy<=y+1+h; yy++) {
        int rad = (yy - y < trunkClear) ? 0 : maxRadius;
        for(int xx=x-rad; ok && xx<=x+rad; xx++) for(int zz=z-rad; ok && zz<=z+rad; zz++) {
            if(yy < 0 || yy >= 128) { ok=0; break; }
            int id = canvas_get(cv,xx,yy,zz);
            if(id != 0 && id != BLK_LEAVES) ok = 0;
        }
    }
    int soil = canvas_get(cv,x,y-1,z);
    if(!ok || (soil != BLK_GRASS && soil != BLK_DIRT) || y >= 128 - h - 1) return 0;
    canvas_set(cv,x,y-1,z,BLK_DIRT);
    int radius = 0;
    for(int yy=y+h; yy>=y+trunkClear; --yy) {
        for(int xx=x-radius; xx<=x+radius; ++xx) for(int zz=z-radius; zz<=z+radius; ++zz) {
            int dx=xx-x, dz=zz-z;
            if((abs(dx) != radius || abs(dz) != radius || radius <= 0) && !cv_opaque_id(canvas_get(cv,xx,yy,zz))) canvas_set_with_meta(cv,xx,yy,zz,BLK_LEAVES,BLOCK_META_WOOD_SPRUCE);
        }
        if(radius >= 1 && yy == y + trunkClear + 1) --radius;
        else if(radius < maxRadius) ++radius;
    }
    for(int yy=0; yy<h-1; ++yy) {
        int id = canvas_get(cv,x,y+yy,z);
        if(id == 0 || id == BLK_LEAVES) canvas_set_with_meta(cv,x,y+yy,z,BLK_LOG,BLOCK_META_WOOD_SPRUCE);
    }
    return 1;
}

static int canvas_place_taiga2(GenCanvas *cv, JavaRandom *r, int x, int y, int z) {
    int h = jr_next_int_bound(r,4) + 6;
    int trunkBare = 1 + jr_next_int_bound(r,2);
    int coneHeight = h - trunkBare;
    int maxRadius = 2 + jr_next_int_bound(r,2);
    int ok = 1;
    if(y < 1 || y + h + 1 > 128) ok = 0;
    for(int yy=y; ok && yy<=y+1+h; yy++) {
        int rad = (yy - y < trunkBare) ? 0 : maxRadius;
        for(int xx=x-rad; ok && xx<=x+rad; xx++) for(int zz=z-rad; ok && zz<=z+rad; zz++) {
            if(yy < 0 || yy >= 128) { ok=0; break; }
            int id = canvas_get(cv,xx,yy,zz);
            if(id != 0 && id != BLK_LEAVES) ok = 0;
        }
    }
    int soil = canvas_get(cv,x,y-1,z);
    if(!ok || (soil != BLK_GRASS && soil != BLK_DIRT) || y >= 128 - h - 1) return 0;
    canvas_set(cv,x,y-1,z,BLK_DIRT);
    int radius = jr_next_int_bound(r,2);
    int nextRadiusAt = 1;
    int resetRadius = 0;
    for(int i=0; i<=coneHeight; ++i) {
        int yy = y + h - i;
        for(int xx=x-radius; xx<=x+radius; ++xx) for(int zz=z-radius; zz<=z+radius; ++zz) {
            int dx=xx-x, dz=zz-z;
            if((abs(dx) != radius || abs(dz) != radius || radius <= 0) && !cv_opaque_id(canvas_get(cv,xx,yy,zz))) canvas_set_with_meta(cv,xx,yy,zz,BLK_LEAVES,BLOCK_META_WOOD_SPRUCE);
        }
        if(radius >= nextRadiusAt) {
            radius = resetRadius;
            resetRadius = 1;
            ++nextRadiusAt;
            if(nextRadiusAt > maxRadius) nextRadiusAt = maxRadius;
        } else ++radius;
    }
    int shorten = jr_next_int_bound(r,3);
    for(int yy=0; yy<h-shorten; ++yy) {
        int id = canvas_get(cv,x,y+yy,z);
        if(id == 0 || id == BLK_LEAVES) canvas_set_with_meta(cv,x,y+yy,z,BLK_LOG,BLOCK_META_WOOD_SPRUCE);
    }
    return 1;
}

static int canvas_place_shrub(GenCanvas *cv, JavaRandom *r, int x, int y, int z, int woodMeta, int leavesMeta) {
    while(y > 0) {
        int id = canvas_get(cv,x,y,z);
        if(id != 0 && id != BLK_LEAVES) break;
        --y;
    }
    int soil = canvas_get(cv,x,y,z);
    if(soil == BLK_DIRT || soil == BLK_GRASS) {
        ++y;
        canvas_set_with_meta(cv,x,y,z,BLK_LOG,(unsigned char)(woodMeta & 3));
        for(int yy=y; yy<=y+2; ++yy) {
            int layer = yy - y;
            int rad = 2 - layer;
            for(int xx=x-rad; xx<=x+rad; ++xx) for(int zz=z-rad; zz<=z+rad; ++zz) {
                int dx=xx-x, dz=zz-z;
                if((abs(dx) != rad || abs(dz) != rad || jr_next_int_bound(r,2) != 0) && !cv_opaque_id(canvas_get(cv,xx,yy,zz))) canvas_set_with_meta(cv,xx,yy,zz,BLK_LEAVES,(unsigned char)(leavesMeta & 3));
            }
        }
    }
    return 1;
}

static void canvas_place_huge_tree_leaf_blob(GenCanvas *cv, JavaRandom *r, int x, int z, int y, int radiusAdd, int leavesMeta) {
    int verticalRadius = 2;
    for (int yy = y - verticalRadius; yy <= y; ++yy) {
        int layer = yy - y;
        int radius = radiusAdd + 1 - layer;
        for (int xx = x - radius; xx <= x + radius + 1; ++xx) {
            int dx = xx - x;
            for (int zz = z - radius; zz <= z + radius + 1; ++zz) {
                int dz = zz - z;
                if ((dx >= 0 || dz >= 0 || dx * dx + dz * dz <= radius * radius) &&
                    ((dx <= 0 && dz <= 0) || dx * dx + dz * dz <= (radius + 1) * (radius + 1)) &&
                    (jr_next_int_bound(r, 4) != 0 || dx * dx + dz * dz <= (radius - 1) * (radius - 1)) &&
                    !cv_opaque_id(canvas_get(cv, xx, yy, zz))) {
                    canvas_set_with_meta(cv, xx, yy, zz, BLK_LEAVES, (unsigned char)(leavesMeta & 3));
                }
            }
        }
    }
}

static int canvas_place_huge_jungle_tree(GenCanvas *cv, JavaRandom *r, int x, int y, int z, int heightBase) {
    int h = jr_next_int_bound(r, 3) + heightBase;
    int ok = 1;
    if (y < 1 || y + h + 1 > 128) ok = 0;
    for (int yy = y; ok && yy <= y + 1 + h; ++yy) {
        int rad = 2;
        if (yy == y) rad = 1;
        if (yy >= y + 1 + h - 2) rad = 2;
        for (int xx = x - rad; ok && xx <= x + rad; ++xx) {
            for (int zz = z - rad; ok && zz <= z + rad; ++zz) {
                if (yy >= 0 && yy < 128) {
                    int id = canvas_get(cv, xx, yy, zz);
                    if (id != 0 && id != BLK_LEAVES && id != BLK_GRASS && id != BLK_DIRT && id != BLK_LOG) ok = 0;
                } else ok = 0;
            }
        }
    }
    if (!ok) return 0;

    int soil = canvas_get(cv, x, y - 1, z);
    if ((soil != BLK_GRASS && soil != BLK_DIRT) || y >= 128 - h - 1) return 0;

    canvas_set(cv, x, y - 1, z, BLK_DIRT);
    canvas_set(cv, x + 1, y - 1, z, BLK_DIRT);
    canvas_set(cv, x, y - 1, z + 1, BLK_DIRT);
    canvas_set(cv, x + 1, y - 1, z + 1, BLK_DIRT);

    canvas_place_huge_tree_leaf_blob(cv, r, x, z, y + h, 2, BLOCK_META_WOOD_JUNGLE);

    for (int by = y + h - 2 - jr_next_int_bound(r, 4); by > y + h / 2; by -= 2 + jr_next_int_bound(r, 4)) {
        float angle = jr_next_float(r) * (float)M_PI * 2.0f;
        int bx = x + (int)(0.5f + beta_cos(angle) * 4.0f);
        int bz = z + (int)(0.5f + beta_sin(angle) * 4.0f);
        canvas_place_huge_tree_leaf_blob(cv, r, bx, bz, by, 0, BLOCK_META_WOOD_JUNGLE);
        for (int i = 0; i < 5; ++i) {
            bx = x + (int)(1.5f + beta_cos(angle) * (float)i);
            bz = z + (int)(1.5f + beta_sin(angle) * (float)i);
            canvas_set_with_meta(cv, bx, by - 3 + i / 2, bz, BLK_LOG, BLOCK_META_WOOD_JUNGLE);
        }
    }

    for (int yy = 0; yy < h; ++yy) {
        int id = canvas_get(cv, x, y + yy, z);
        if (id == 0 || id == BLK_LEAVES) {
            canvas_set_with_meta(cv, x, y + yy, z, BLK_LOG, BLOCK_META_WOOD_JUNGLE);
            if (yy > 0) {
                if (jr_next_int_bound(r, 3) > 0 && canvas_get(cv, x - 1, y + yy, z) == 0) canvas_set_with_meta(cv, x - 1, y + yy, z, BLK_VINE, 8);
                if (jr_next_int_bound(r, 3) > 0 && canvas_get(cv, x, y + yy, z - 1) == 0) canvas_set_with_meta(cv, x, y + yy, z - 1, BLK_VINE, 1);
            }
        }

        if (yy < h - 1) {
            id = canvas_get(cv, x + 1, y + yy, z);
            if (id == 0 || id == BLK_LEAVES) {
                canvas_set_with_meta(cv, x + 1, y + yy, z, BLK_LOG, BLOCK_META_WOOD_JUNGLE);
                if (yy > 0) {
                    if (jr_next_int_bound(r, 3) > 0 && canvas_get(cv, x + 2, y + yy, z) == 0) canvas_set_with_meta(cv, x + 2, y + yy, z, BLK_VINE, 2);
                    if (jr_next_int_bound(r, 3) > 0 && canvas_get(cv, x + 1, y + yy, z - 1) == 0) canvas_set_with_meta(cv, x + 1, y + yy, z - 1, BLK_VINE, 1);
                }
            }

            id = canvas_get(cv, x + 1, y + yy, z + 1);
            if (id == 0 || id == BLK_LEAVES) {
                canvas_set_with_meta(cv, x + 1, y + yy, z + 1, BLK_LOG, BLOCK_META_WOOD_JUNGLE);
                if (yy > 0) {
                    if (jr_next_int_bound(r, 3) > 0 && canvas_get(cv, x + 2, y + yy, z + 1) == 0) canvas_set_with_meta(cv, x + 2, y + yy, z + 1, BLK_VINE, 2);
                    if (jr_next_int_bound(r, 3) > 0 && canvas_get(cv, x + 1, y + yy, z + 2) == 0) canvas_set_with_meta(cv, x + 1, y + yy, z + 2, BLK_VINE, 4);
                }
            }

            id = canvas_get(cv, x, y + yy, z + 1);
            if (id == 0 || id == BLK_LEAVES) {
                canvas_set_with_meta(cv, x, y + yy, z + 1, BLK_LOG, BLOCK_META_WOOD_JUNGLE);
                if (yy > 0) {
                    if (jr_next_int_bound(r, 3) > 0 && canvas_get(cv, x - 1, y + yy, z + 1) == 0) canvas_set_with_meta(cv, x - 1, y + yy, z + 1, BLK_VINE, 8);
                    if (jr_next_int_bound(r, 3) > 0 && canvas_get(cv, x, y + yy, z + 2) == 0) canvas_set_with_meta(cv, x, y + yy, z + 2, BLK_VINE, 4);
                }
            }
        }
    }
    return 1;
}

static void canvas_place_flowers(GenCanvas *cv, JavaRandom *r, int id, int x, int y, int z) {
    for(int i=0;i<64;i++) {
        int xx=x+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        int yy=y+jr_next_int_bound(r,4)-jr_next_int_bound(r,4);
        int zz=z+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        if(yy>0 && yy<128 && canvas_get(cv,xx,yy,zz)==0 && canvas_can_flower_stay(cv,id,xx,yy,zz)) canvas_set(cv,xx,yy,zz,(unsigned char)id);
    }
}
static void canvas_place_reeds(GenCanvas *cv, JavaRandom *r, int x, int y, int z) {
    for(int i=0;i<20;i++) {
        int xx=x+jr_next_int_bound(r,4)-jr_next_int_bound(r,4), yy=y, zz=z+jr_next_int_bound(r,4)-jr_next_int_bound(r,4);
        if(canvas_get(cv,xx,yy,zz)==0 && canvas_can_reed_stay(cv,xx,yy,zz)) {
            int h=2+jr_next_int_bound(r,jr_next_int_bound(r,3)+1);
            for(int j=0;j<h;j++) if(canvas_can_reed_stay(cv,xx,yy+j,zz)) canvas_set(cv,xx,yy+j,zz,BLK_REEDS);
        }
    }
}
static void canvas_place_tall_grass(GenCanvas *cv, JavaRandom *r, int meta, int x, int y, int z) {
    while(y > 0) { int cur = canvas_get(cv,x,y,z); if(cur != 0 && cur != BLK_LEAVES) break; --y; }
    for(int i=0;i<128;i++) {
        int xx=x+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        int yy=y+jr_next_int_bound(r,4)-jr_next_int_bound(r,4);
        int zz=z+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        int below = canvas_get(cv, xx, yy - 1, zz);
        if(yy>0 && yy<128 && canvas_get(cv,xx,yy,zz)==0 && (below==BLK_GRASS || below==BLK_DIRT)) {
            canvas_set_with_meta(cv,xx,yy,zz,BLK_TALL_GRASS,(unsigned char)(meta & 15));
        }
    }
}
static void canvas_place_dead_bush(GenCanvas *cv, JavaRandom *r, int x, int y, int z) {
    while(y > 0) { int cur = canvas_get(cv,x,y,z); if(cur != 0 && cur != BLK_LEAVES) break; --y; }
    for(int i=0;i<4;i++) {
        int xx=x+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        int yy=y+jr_next_int_bound(r,4)-jr_next_int_bound(r,4);
        int zz=z+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        if(yy>0 && yy<128 && canvas_get(cv,xx,yy,zz)==0 && canvas_get(cv,xx,yy-1,zz)==BLK_SAND) canvas_set(cv,xx,yy,zz,BLK_DEAD_BUSH);
    }
}
static void canvas_place_waterlily(GenCanvas *cv, JavaRandom *r, int x, int y, int z) {
    for(int i=0;i<10;i++) {
        int xx=x+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        int yy=y+jr_next_int_bound(r,4)-jr_next_int_bound(r,4);
        int zz=z+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        if(yy>0 && yy<128 && canvas_get(cv,xx,yy,zz)==0 && cv_is_water_id(canvas_get(cv,xx,yy-1,zz))) canvas_set(cv,xx,yy,zz,BLK_LILY_PAD);
    }
}
static int canvas_vine_can_place_side(GenCanvas *cv, int x, int y, int z, int side) {
    if(side == 2) return cv_opaque_id(canvas_get(cv,x,y,z+1));
    if(side == 3) return cv_opaque_id(canvas_get(cv,x,y,z-1));
    if(side == 4) return cv_opaque_id(canvas_get(cv,x+1,y,z));
    if(side == 5) return cv_opaque_id(canvas_get(cv,x-1,y,z));
    return 0;
}
static int canvas_vine_meta_for_side(int side) {
    if(side == 2) return 1;
    if(side == 3) return 4;
    if(side == 4) return 8;
    if(side == 5) return 2;
    return 0;
}
static void canvas_place_jungle_vines(GenCanvas *cv, JavaRandom *r, int chunkX, int chunkZ) {
    for(int i=0;i<50;i++) {
        int x0 = chunkX + jr_next_int_bound(r,16) + 8;
        int z0 = chunkZ + jr_next_int_bound(r,16) + 8;
        int x = x0, z = z0;
        for(int y=64; y<128; ++y) {
            if(canvas_get(cv,x,y,z) == 0) {
                for(int side=2; side<=5; ++side) {
                    if(canvas_vine_can_place_side(cv,x,y,z,side)) { canvas_set_with_meta(cv,x,y,z,BLK_VINE,(unsigned char)canvas_vine_meta_for_side(side)); break; }
                }
            } else {
                x = x0 + jr_next_int_bound(r,4) - jr_next_int_bound(r,4);
                z = z0 + jr_next_int_bound(r,4) - jr_next_int_bound(r,4);
            }
        }
    }
}
static void canvas_place_pumpkin(GenCanvas *cv, JavaRandom *r, int x, int y, int z) {
    for(int i=0;i<64;i++) {
        int xx=x+jr_next_int_bound(r,8)-jr_next_int_bound(r,8), yy=y+jr_next_int_bound(r,4)-jr_next_int_bound(r,4), zz=z+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        if(canvas_get(cv,xx,yy,zz)==0 && canvas_get(cv,xx,yy-1,zz)==BLK_GRASS) canvas_set_with_meta(cv,xx,yy,zz,BLK_PUMPKIN,(unsigned char)jr_next_int_bound(r,4));
    }
}
static void canvas_place_cactus(GenCanvas *cv, JavaRandom *r, int x, int y, int z) {
    for(int i=0;i<10;i++) {
        int xx=x+jr_next_int_bound(r,8)-jr_next_int_bound(r,8), yy=y+jr_next_int_bound(r,4)-jr_next_int_bound(r,4), zz=z+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        if(canvas_get(cv,xx,yy,zz)==0) {
            int h=1+jr_next_int_bound(r,jr_next_int_bound(r,3)+1);
            for(int j=0;j<h;j++) if(canvas_can_cactus_stay(cv,xx,yy+j,zz)) canvas_set(cv,xx,yy+j,zz,BLK_CACTUS);
        }
    }
}

static int canvas_place_big_mushroom(GenCanvas *cv, JavaRandom *r, int x, int y, int z) {
    int type = jr_next_int_bound(r,2);
    int h = jr_next_int_bound(r,3) + 4;
    if(y < 1 || y + h + 1 >= 128) return 0;
    for(int yy=y; yy<=y+1+h; ++yy) {
        int rad = (yy == y) ? 0 : 3;
        for(int xx=x-rad; xx<=x+rad; ++xx) for(int zz=z-rad; zz<=z+rad; ++zz) {
            int id = canvas_get(cv,xx,yy,zz);
            if(id != 0 && id != BLK_LEAVES) return 0;
        }
    }
    int soil = canvas_get(cv,x,y-1,z);
    if(soil != BLK_DIRT && soil != BLK_GRASS && soil != BLK_MYCELIUM) return 0;
    canvas_set(cv,x,y-1,z,BLK_DIRT);
    int capStart = y + h;
    if(type == 1) capStart = y + h - 3;
    for(int yy=capStart; yy<=y+h; ++yy) {
        int rad = 1;
        if(yy < y+h) ++rad;
        if(type == 0) rad = 3;
        for(int xx=x-rad; xx<=x+rad; ++xx) for(int zz=z-rad; zz<=z+rad; ++zz) {
            int meta = 5;
            if(xx == x-rad) --meta;
            if(xx == x+rad) ++meta;
            if(zz == z-rad) meta -= 3;
            if(zz == z+rad) meta += 3;
            if(type == 0 || yy < y+h) {
                if((xx == x-rad || xx == x+rad) && (zz == z-rad || zz == z+rad)) continue;
                if(xx == x-(rad-1) && zz == z-rad) meta = 1;
                if(xx == x-rad && zz == z-(rad-1)) meta = 1;
                if(xx == x+(rad-1) && zz == z-rad) meta = 3;
                if(xx == x+rad && zz == z-(rad-1)) meta = 3;
                if(xx == x-(rad-1) && zz == z+rad) meta = 7;
                if(xx == x-rad && zz == z+(rad-1)) meta = 7;
                if(xx == x+(rad-1) && zz == z+rad) meta = 9;
                if(xx == x+rad && zz == z+(rad-1)) meta = 9;
            }
            if(meta == 5 && yy < y+h) meta = 0;
            if((meta != 0 || y >= y+h-1) && !cv_opaque_id(canvas_get(cv,xx,yy,zz))) canvas_set_with_meta(cv,xx,yy,zz,(unsigned char)(type == 0 ? BLK_MUSHROOM_CAP_BROWN : BLK_MUSHROOM_CAP_RED),(unsigned char)meta);
        }
    }
    for(int yy=0; yy<h; ++yy) {
        if(!cv_opaque_id(canvas_get(cv,x,y+yy,z))) canvas_set_with_meta(cv,x,y+yy,z,(unsigned char)(type == 0 ? BLK_MUSHROOM_CAP_BROWN : BLK_MUSHROOM_CAP_RED),10);
    }
    return 1;
}
static int canvas_place_desert_well(GenCanvas *cv, int x, int y, int z) {
    while(canvas_get(cv,x,y,z) == 0 && y > 2) --y;
    if(canvas_get(cv,x,y,z) != BLK_SAND) return 0;
    for(int dx=-2; dx<=2; ++dx) for(int dz=-2; dz<=2; ++dz) {
        if(canvas_get(cv,x+dx,y-1,z+dz)==0 && canvas_get(cv,x+dx,y-2,z+dz)==0) return 0;
    }
    for(int dy=-1; dy<=0; ++dy) for(int dx=-2; dx<=2; ++dx) for(int dz=-2; dz<=2; ++dz) canvas_set(cv,x+dx,y+dy,z+dz,BLK_SANDSTONE);
    canvas_set(cv,x,y,z,BLK_WATER);
    canvas_set(cv,x-1,y,z,BLK_WATER);
    canvas_set(cv,x+1,y,z,BLK_WATER);
    canvas_set(cv,x,y,z-1,BLK_WATER);
    canvas_set(cv,x,y,z+1,BLK_WATER);
    for(int dx=-2; dx<=2; ++dx) for(int dz=-2; dz<=2; ++dz) if(dx==-2 || dx==2 || dz==-2 || dz==2) canvas_set(cv,x+dx,y+1,z+dz,BLK_SANDSTONE);
    canvas_set_with_meta(cv,x+2,y+1,z,BLK_SLAB,1);
    canvas_set_with_meta(cv,x-2,y+1,z,BLK_SLAB,1);
    canvas_set_with_meta(cv,x,y+1,z+2,BLK_SLAB,1);
    canvas_set_with_meta(cv,x,y+1,z-2,BLK_SLAB,1);
    for(int dx=-1; dx<=1; ++dx) for(int dz=-1; dz<=1; ++dz) {
        if(dx==0 && dz==0) canvas_set(cv,x+dx,y+4,z+dz,BLK_SANDSTONE);
        else canvas_set_with_meta(cv,x+dx,y+4,z+dz,BLK_SLAB,1);
    }
    for(int dy=1; dy<=3; ++dy) {
        canvas_set(cv,x-1,y+dy,z-1,BLK_SANDSTONE);
        canvas_set(cv,x-1,y+dy,z+1,BLK_SANDSTONE);
        canvas_set(cv,x+1,y+dy,z-1,BLK_SANDSTONE);
        canvas_set(cv,x+1,y+dy,z+1,BLK_SANDSTONE);
    }
    return 1;
}
static void canvas_place_liquid_spring(GenCanvas *cv, int id, int x, int y, int z) {
    if(canvas_get(cv,x,y+1,z)!=BLK_STONE || canvas_get(cv,x,y-1,z)!=BLK_STONE) return;
    int cur=canvas_get(cv,x,y,z); if(cur!=0 && cur!=BLK_STONE) return;
    int stone=0, air=0;
    if(canvas_get(cv,x-1,y,z)==BLK_STONE) stone++;
    if(canvas_get(cv,x+1,y,z)==BLK_STONE) stone++;
    if(canvas_get(cv,x,y,z-1)==BLK_STONE) stone++;
    if(canvas_get(cv,x,y,z+1)==BLK_STONE) stone++;
    if(canvas_get(cv,x-1,y,z)==0) air++;
    if(canvas_get(cv,x+1,y,z)==0) air++;
    if(canvas_get(cv,x,y,z-1)==0) air++;
    if(canvas_get(cv,x,y,z+1)==0) air++;
    if(stone==3 && air==1) canvas_set(cv,x,y,z,(unsigned char)id);
}
static int canvas_lake(GenCanvas *cv, JavaRandom *r, int liquid, int x, int y, int z) {
    x -= 8; z -= 8;
    while(y > 0 && canvas_get(cv,x,y,z)==0) y--;
    y -= 4;
    if(y <= 0) return 0;
    unsigned char carve[2048]; memset(carve,0,sizeof(carve));
    int blobs = jr_next_int_bound(r,4) + 4;
    for(int b=0;b<blobs;b++) {
        double sx=jr_next_double(r)*6.0+3.0, sy=jr_next_double(r)*4.0+2.0, sz=jr_next_double(r)*6.0+3.0;
        double cx=jr_next_double(r)*(16.0-sx-2.0)+1.0+sx/2.0, cy=jr_next_double(r)*(8.0-sy-4.0)+2.0+sy/2.0, cz=jr_next_double(r)*(16.0-sz-2.0)+1.0+sz/2.0;
        for(int xx=1;xx<15;xx++) for(int zz=1;zz<15;zz++) for(int yy=1;yy<7;yy++) {
            double dx=(xx-cx)/(sx/2.0), dy=(yy-cy)/(sy/2.0), dz=(zz-cz)/(sz/2.0);
            if(dx*dx+dy*dy+dz*dz < 1.0) carve[(xx*16+zz)*8+yy]=1;
        }
    }
    for(int xx=0;xx<16;xx++) for(int zz=0;zz<16;zz++) for(int yy=0;yy<8;yy++) {
        int edge=!carve[(xx*16+zz)*8+yy] && ((xx<15&&carve[((xx+1)*16+zz)*8+yy])||(xx>0&&carve[((xx-1)*16+zz)*8+yy])||(zz<15&&carve[(xx*16+zz+1)*8+yy])||(zz>0&&carve[(xx*16+zz-1)*8+yy])||(yy<7&&carve[(xx*16+zz)*8+yy+1])||(yy>0&&carve[(xx*16+zz)*8+yy-1]));
        if(edge) { int id=canvas_get(cv,x+xx,y+yy,z+zz); if(yy>=4 && cv_is_liquid_id(id)) return 0; if(yy<4 && !cv_is_solid_id(id) && id != liquid) return 0; }
    }
    for(int xx=0;xx<16;xx++) for(int zz=0;zz<16;zz++) for(int yy=0;yy<8;yy++) if(carve[(xx*16+zz)*8+yy]) canvas_set(cv,x+xx,y+yy,z+zz,(unsigned char)(yy>=4?0:liquid));
    for(int xx=0;xx<16;xx++) for(int zz=0;zz<16;zz++) for(int yy=4;yy<8;yy++) if(carve[(xx*16+zz)*8+yy] && canvas_get(cv,x+xx,y+yy-1,z+zz)==BLK_DIRT) canvas_set(cv,x+xx,y+yy-1,z+zz,BLK_GRASS);
    if(liquid == BLK_LAVA || liquid == BLK_STILL_LAVA) {
        for(int xx=0;xx<16;xx++) for(int zz=0;zz<16;zz++) for(int yy=0;yy<8;yy++) {
            int edge=!carve[(xx*16+zz)*8+yy] && ((xx<15&&carve[((xx+1)*16+zz)*8+yy])||(xx>0&&carve[((xx-1)*16+zz)*8+yy])||(zz<15&&carve[(xx*16+zz+1)*8+yy])||(zz>0&&carve[(xx*16+zz-1)*8+yy])||(yy<7&&carve[(xx*16+zz)*8+yy+1])||(yy>0&&carve[(xx*16+zz)*8+yy-1]));
            if(edge && (yy<4 || jr_next_int_bound(r,2)!=0) && cv_is_solid_id(canvas_get(cv,x+xx,y+yy,z+zz))) canvas_set(cv,x+xx,y+yy,z+zz,BLK_STONE);
        }
    }
    return 1;
}
static void canvas_consume_dungeon_loot(JavaRandom *r) {
    int k = jr_next_int_bound(r, 11);
    if(k == 1 || k == 3 || k == 4 || k == 5) jr_next_int_bound(r,4);
    else if(k == 7) jr_next_int_bound(r,100);
    else if(k == 8) { if(jr_next_int_bound(r,2)==0) jr_next_int_bound(r,4); }
    else if(k == 9) { if(jr_next_int_bound(r,10)==0) jr_next_int_bound(r,2); }
}
static int canvas_dungeon(GenCanvas *cv, JavaRandom *r, int x, int y, int z) {
    int h=3, rx=jr_next_int_bound(r,2)+2, rz=jr_next_int_bound(r,2)+2, openings=0;
    for(int xx=x-rx-1;xx<=x+rx+1;xx++) for(int yy=y-1;yy<=y+h+1;yy++) for(int zz=z-rz-1;zz<=z+rz+1;zz++) {
        int mat=canvas_get(cv,xx,yy,zz);
        if(yy==y-1 && !cv_is_solid_id(mat)) return 0;
        if(yy==y+h+1 && !cv_is_solid_id(mat)) return 0;
        if((xx==x-rx-1||xx==x+rx+1||zz==z-rz-1||zz==z+rz+1) && yy==y && canvas_get(cv,xx,yy,zz)==0 && canvas_get(cv,xx,yy+1,zz)==0) openings++;
    }
    if(openings < 1 || openings > 5) return 0;
    for(int xx=x-rx-1;xx<=x+rx+1;xx++) for(int yy=y+h;yy>=y-1;yy--) for(int zz=z-rz-1;zz<=z+rz+1;zz++) {
        if(xx != x-rx-1 && yy != y-1 && zz != z-rz-1 && xx != x+rx+1 && yy != y+h+1 && zz != z+rz+1) canvas_set(cv,xx,yy,zz,0);
        else if(yy >= 0 && !cv_is_solid_id(canvas_get(cv,xx,yy-1,zz))) canvas_set(cv,xx,yy,zz,0);
        else if(cv_is_solid_id(canvas_get(cv,xx,yy,zz))) canvas_set(cv,xx,yy,zz,(yy==y-1 && jr_next_int_bound(r,4)!=0) ? 48 : 4);
    }
    for(int c=0;c<2;c++) {
        for(int a=0;a<3;a++) {
            int xx=x+jr_next_int_bound(r,rx*2+1)-rx, zz=z+jr_next_int_bound(r,rz*2+1)-rz;
            if(canvas_get(cv,xx,y,zz)==0) {
                int solid=0; if(cv_is_solid_id(canvas_get(cv,xx-1,y,zz)))solid++; if(cv_is_solid_id(canvas_get(cv,xx+1,y,zz)))solid++; if(cv_is_solid_id(canvas_get(cv,xx,y,zz-1)))solid++; if(cv_is_solid_id(canvas_get(cv,xx,y,zz+1)))solid++;
                if(solid==1) { canvas_set(cv,xx,y,zz,54); for(int k=0;k<8;k++) canvas_consume_dungeon_loot(r); break; }
            }
        }
    }
    canvas_set(cv,x,y,z,52);
    jr_next_int_bound(r,4);
    return 1;
}

typedef struct BigTreeGen {
    JavaRandom rand;
    int base[3];
    int height;
    int trunkHeight;
    double heightAttenuation;
    double branchDensity;
    double branchSlope;
    double scaleWidth;
    double leafDensity;
    int trunkSize;
    int heightLimit;
    int leafDistanceLimit;
    int leaves[512][4];
    int leafCount;
} BigTreeGen;
static const unsigned char bigtree_axis_map[6] = {2,0,0,1,2,1};
static void bigtree_init(BigTreeGen *bt) {
    memset(bt, 0, sizeof(*bt));
    bt->heightAttenuation = 0.618;
    bt->branchDensity = 1.0;
    bt->branchSlope = 0.381;
    bt->scaleWidth = 1.0;
    bt->leafDensity = 1.0;
    bt->trunkSize = 1;
    bt->heightLimit = 12;
    bt->leafDistanceLimit = 4;
}
static float bigtree_layer_size(BigTreeGen *bt, int y) {
    if ((double)y < (double)((float)bt->height) * 0.3) return -1.618f;
    float half = (float)bt->height / 2.0f;
    float d = half - (float)y;
    float r;
    if (d == 0.0f) r = half;
    else if (fabsf(d) >= half) r = 0.0f;
    else r = (float)sqrt(pow((double)fabsf(half), 2.0) - pow((double)fabsf(d), 2.0));
    r *= 0.5f;
    return r;
}
static float bigtree_leaf_size(BigTreeGen *bt, int y) {
    return y >= 0 && y < bt->leafDistanceLimit ? (y != 0 && y != bt->leafDistanceLimit - 1 ? 3.0f : 2.0f) : -1.0f;
}
static void bigtree_leaf_disk(GenCanvas *cv, int x, int y, int z, float radius, unsigned char axis, int blockId) {
    int rad = (int)((double)radius + 0.618);
    unsigned char ax1 = bigtree_axis_map[axis];
    unsigned char ax2 = bigtree_axis_map[axis + 3];
    int center[3] = {x, y, z};
    int pos[3] = {0, 0, 0};
    for (int a = -rad; a <= rad; a++) {
        pos[axis] = center[axis];
        pos[ax1] = center[ax1] + a;
        for (int b = -rad; b <= rad; b++) {
            double dist = sqrt(pow((double)abs(a) + 0.5, 2.0) + pow((double)abs(b) + 0.5, 2.0));
            if (dist <= (double)radius) {
                pos[ax2] = center[ax2] + b;
                int cur = canvas_get(cv, pos[0], pos[1], pos[2]);
                if (cur == 0 || cur == BLK_LEAVES) canvas_set(cv, pos[0], pos[1], pos[2], (unsigned char)blockId);
            }
        }
    }
}
static void bigtree_leaf_node(GenCanvas *cv, BigTreeGen *bt, int x, int y, int z) {
    for (int yy = y; yy < y + bt->leafDistanceLimit; yy++) {
        float r = bigtree_leaf_size(bt, yy - y);
        bigtree_leaf_disk(cv, x, yy, z, r, 1, BLK_LEAVES);
    }
}
static void bigtree_place_line(GenCanvas *cv, int a[3], int b[3], int blockId) {
    int delta[3] = {0,0,0};
    int major = 0;
    for (int i=0;i<3;i++) { delta[i] = b[i] - a[i]; if (abs(delta[i]) > abs(delta[major])) major = i; }
    if (delta[major] == 0) return;
    unsigned char ax1 = bigtree_axis_map[major];
    unsigned char ax2 = bigtree_axis_map[major + 3];
    int step = delta[major] > 0 ? 1 : -1;
    double slope1 = (double)delta[ax1] / (double)delta[major];
    double slope2 = (double)delta[ax2] / (double)delta[major];
    int pos[3] = {0,0,0};
    for (int t = 0, end = delta[major] + step; t != end; t += step) {
        pos[major] = beta_floor_double((double)(a[major] + t) + 0.5);
        pos[ax1] = beta_floor_double((double)a[ax1] + (double)t * slope1 + 0.5);
        pos[ax2] = beta_floor_double((double)a[ax2] + (double)t * slope2 + 0.5);
        canvas_set(cv, pos[0], pos[1], pos[2], (unsigned char)blockId);
    }
}
static int bigtree_line_check(GenCanvas *cv, int a[3], int b[3]) {
    int delta[3] = {0,0,0};
    int major = 0;
    for (int i=0;i<3;i++) { delta[i] = b[i] - a[i]; if (abs(delta[i]) > abs(delta[major])) major = i; }
    if (delta[major] == 0) return -1;
    unsigned char ax1 = bigtree_axis_map[major];
    unsigned char ax2 = bigtree_axis_map[major + 3];
    int step = delta[major] > 0 ? 1 : -1;
    double slope1 = (double)delta[ax1] / (double)delta[major];
    double slope2 = (double)delta[ax2] / (double)delta[major];
    int pos[3] = {0,0,0};
    int t = 0;
    int end = delta[major] + step;
    for (; t != end; t += step) {
        pos[major] = a[major] + t;
        pos[ax1] = beta_floor_double((double)a[ax1] + (double)t * slope1);
        pos[ax2] = beta_floor_double((double)a[ax2] + (double)t * slope2);
        int id = canvas_get(cv, pos[0], pos[1], pos[2]);
        if (id != 0 && id != BLK_LEAVES) break;
    }
    return t == end ? -1 : abs(t);
}
static void bigtree_generate_leaf_nodes(GenCanvas *cv, BigTreeGen *bt) {
    bt->trunkHeight = (int)((double)bt->height * bt->heightAttenuation);
    if (bt->trunkHeight >= bt->height) bt->trunkHeight = bt->height - 1;
    int nodesPerLayer = (int)(1.382 + pow(bt->leafDensity * (double)bt->height / 13.0, 2.0));
    if (nodesPerLayer < 1) nodesPerLayer = 1;
    int tmp[512][4];
    int topY = bt->base[1] + bt->height - bt->leafDistanceLimit;
    int count = 1;
    int trunkTopY = bt->base[1] + bt->trunkHeight;
    int relY = topY - bt->base[1];
    tmp[0][0] = bt->base[0]; tmp[0][1] = topY; tmp[0][2] = bt->base[2]; tmp[0][3] = trunkTopY;
    --topY;
    while (relY >= 0) {
        int n = 0;
        float layer = bigtree_layer_size(bt, relY);
        if (layer < 0.0f) { --topY; --relY; continue; }
        for (double half = 0.5; n < nodesPerLayer; n++) {
            double dist = bt->scaleWidth * (double)layer * ((double)jr_next_float(&bt->rand) + 0.328);
            double angle = (double)jr_next_float(&bt->rand) * 2.0 * 3.14159;
            int x = (int)(dist * sin(angle) + (double)bt->base[0] + half);
            int z = (int)(dist * cos(angle) + (double)bt->base[2] + half);
            int node[3] = {x, topY, z};
            int nodeTop[3] = {x, topY + bt->leafDistanceLimit, z};
            if (bigtree_line_check(cv, node, nodeTop) == -1) {
                int trunk[3] = {bt->base[0], bt->base[1], bt->base[2]};
                double horiz = sqrt(pow((double)abs(bt->base[0] - node[0]), 2.0) + pow((double)abs(bt->base[2] - node[2]), 2.0));
                double slopeY = horiz * bt->branchSlope;
                if ((double)node[1] - slopeY > (double)trunkTopY) trunk[1] = trunkTopY;
                else trunk[1] = (int)((double)node[1] - slopeY);
                if (bigtree_line_check(cv, trunk, node) == -1 && count < 512) {
                    tmp[count][0] = x; tmp[count][1] = topY; tmp[count][2] = z; tmp[count][3] = trunk[1];
                    count++;
                }
            }
        }
        --topY;
        --relY;
    }
    bt->leafCount = count;
    for (int i=0;i<count;i++) for (int j=0;j<4;j++) bt->leaves[i][j] = tmp[i][j];
}
static void bigtree_make_leaf_nodes(GenCanvas *cv, BigTreeGen *bt) {
    for (int i=0; i<bt->leafCount; i++) bigtree_leaf_node(cv, bt, bt->leaves[i][0], bt->leaves[i][1], bt->leaves[i][2]);
}
static int bigtree_branch_reaches(BigTreeGen *bt, int y) { return (double)y >= (double)bt->height * 0.2; }
static void bigtree_make_trunk(GenCanvas *cv, BigTreeGen *bt) {
    int start[3] = {bt->base[0], bt->base[1], bt->base[2]};
    int end[3] = {bt->base[0], bt->base[1] + bt->trunkHeight, bt->base[2]};
    bigtree_place_line(cv, start, end, BLK_LOG);
    if (bt->trunkSize == 2) {
        start[0]++; end[0]++; bigtree_place_line(cv, start, end, BLK_LOG);
        start[2]++; end[2]++; bigtree_place_line(cv, start, end, BLK_LOG);
        start[0]--; end[0]--; bigtree_place_line(cv, start, end, BLK_LOG);
    }
}
static void bigtree_make_branches(GenCanvas *cv, BigTreeGen *bt) {
    int base[3] = {bt->base[0], bt->base[1], bt->base[2]};
    for (int i=0;i<bt->leafCount;i++) {
        int *leaf = bt->leaves[i];
        int node[3] = {leaf[0], leaf[1], leaf[2]};
        base[1] = leaf[3];
        int dy = base[1] - bt->base[1];
        if (bigtree_branch_reaches(bt, dy)) bigtree_place_line(cv, base, node, BLK_LOG);
    }
}
static int bigtree_valid(GenCanvas *cv, BigTreeGen *bt) {
    int start[3] = {bt->base[0], bt->base[1], bt->base[2]};
    int end[3] = {bt->base[0], bt->base[1] + bt->height - 1, bt->base[2]};
    int soil = canvas_get(cv, bt->base[0], bt->base[1] - 1, bt->base[2]);
    if (soil != BLK_GRASS && soil != BLK_DIRT) return 0;
    int check = bigtree_line_check(cv, start, end);
    if (check == -1) return 1;
    if (check < 6) return 0;
    bt->height = check;
    return 1;
}
static int canvas_place_big_tree(GenCanvas *cv, JavaRandom *mainRand, BigTreeGen *bt, int x, int y, int z) {
    int64_t seed = jr_next_long(mainRand);
    jr_set_seed(&bt->rand, seed);
    bt->base[0] = x; bt->base[1] = y; bt->base[2] = z;
    if (bt->height == 0) bt->height = 5 + jr_next_int_bound(&bt->rand, bt->heightLimit);
    if (!bigtree_valid(cv, bt)) return 0;
    bigtree_generate_leaf_nodes(cv, bt);
    bigtree_make_leaf_nodes(cv, bt);
    bigtree_make_trunk(cv, bt);
    bigtree_make_branches(cv, bt);
    return 1;
}

static void canvas_place_biome_tree(GenCanvas *cv, JavaRandom *r, WorldBiome biome, int x, int y, int z) {
    BigTreeGen bigTree;
    if(biome.id == BIOME_SWAMPLAND) {
        canvas_place_swamp_tree(cv, r, x, y, z);
        return;
    }
    if(biome.id == BIOME_TAIGA || biome.id == BIOME_TAIGA_HILLS) {
        if(jr_next_int_bound(r,3) == 0) canvas_place_taiga1(cv, r, x, y, z);
        else canvas_place_taiga2(cv, r, x, y, z);
        return;
    }
    if(biome.id == BIOME_FOREST || biome.id == BIOME_FOREST_HILLS) {
        if(jr_next_int_bound(r,5) == 0) canvas_place_forest_tree(cv, r, x, y, z);
        else if(jr_next_int_bound(r,10) == 0) { bigtree_init(&bigTree); canvas_place_big_tree(cv, r, &bigTree, x, y, z); }
        else canvas_place_tree(cv, r, x, y, z);
        return;
    }
    if(biome.id == BIOME_JUNGLE || biome.id == BIOME_JUNGLE_HILLS) {
        if(jr_next_int_bound(r,10) == 0) { bigtree_init(&bigTree); canvas_place_big_tree(cv, r, &bigTree, x, y, z); }
        else if(jr_next_int_bound(r,2) == 0) canvas_place_shrub(cv, r, x, y, z, BLOCK_META_WOOD_JUNGLE, BLOCK_META_WOOD_OAK);
        else if(jr_next_int_bound(r,3) == 0) canvas_place_huge_jungle_tree(cv, r, x, y, z, 10 + jr_next_int_bound(r,20));
        else canvas_place_tree_custom(cv, r, x, y, z, 4 + jr_next_int_bound(r,7), BLOCK_META_WOOD_JUNGLE, BLOCK_META_WOOD_JUNGLE, 1);
        return;
    }
    if(jr_next_int_bound(r,10) == 0) { bigtree_init(&bigTree); canvas_place_big_tree(cv, r, &bigTree, x, y, z); }
    else canvas_place_tree(cv, r, x, y, z);
}


/* -------------------------------------------------------------------------
   Java 1.2.5 structure block-placement pass.

   This is a deterministic, metadata-aware placement layer for the generated
   blocks used by the three Overworld structure families.  It now implements
   a larger component-inspired graph for strongholds, villages and mineshafts:
   portal rooms, libraries, prisons, crossings, village wells/roads/houses/
   farms/blacksmith/church/lamp posts, and mineshaft rooms/corridors/branches.
   The exact Java component graph still needs line-by-line parity testing, but
   the generated chunks now contain the important blocks, metadata and tile
   entities needed for in-game testing.
   ------------------------------------------------------------------------- */
static JavaRandom worldgen_structure_random(long long seed, int sourceChunkX, int sourceChunkZ) {
    JavaRandom r;
    jr_set_seed(&r, (int64_t)seed);
    int64_t mulX = jr_next_long(&r);
    int64_t mulZ = jr_next_long(&r);
    int64_t s = (int64_t)(((uint64_t)(int64_t)sourceChunkX * (uint64_t)mulX) ^
                          ((uint64_t)(int64_t)sourceChunkZ * (uint64_t)mulZ) ^
                          (uint64_t)(int64_t)seed);
    jr_set_seed(&r, s);
    jr_next_int(&r); /* MapGenStructure.recursiveGenerate consumes nextInt(). */
    return r;
}

static void struct_set(GenCanvas *cv, int x, int y, int z, int id, int meta) {
    if(y < 0 || y >= 128) return;
    canvas_set_with_meta(cv, x, y, z, (unsigned char)id, (unsigned char)(meta & 15));
}

static void struct_clear_box(GenCanvas *cv, int x1, int y1, int z1, int x2, int y2, int z2) {
    for(int y=y1; y<=y2; ++y) for(int z=z1; z<=z2; ++z) for(int x=x1; x<=x2; ++x) struct_set(cv,x,y,z,BLK_AIR,0);
}

static void struct_fill_box(GenCanvas *cv, int x1, int y1, int z1, int x2, int y2, int z2, int id, int meta) {
    for(int y=y1; y<=y2; ++y) for(int z=z1; z<=z2; ++z) for(int x=x1; x<=x2; ++x) struct_set(cv,x,y,z,id,meta);
}

static void struct_hollow_box_random_stone(GenCanvas *cv, JavaRandom *r, int x1, int y1, int z1, int x2, int y2, int z2) {
    for(int y=y1; y<=y2; ++y) for(int z=z1; z<=z2; ++z) for(int x=x1; x<=x2; ++x) {
        int wall = (x==x1 || x==x2 || y==y1 || y==y2 || z==z1 || z==z2);
        if(!wall) { struct_set(cv,x,y,z,BLK_AIR,0); continue; }
        int id = BLK_STONE_BRICK, meta = BLOCK_META_STONE_BRICK_NORMAL;
        float f = jr_next_float(r);
        if(f < 0.20f) meta = BLOCK_META_STONE_BRICK_CRACKED;
        else if(f < 0.50f) meta = BLOCK_META_STONE_BRICK_MOSSY;
        else if(f < 0.55f) { id = BLK_SILVERFISH; meta = 2; }
        struct_set(cv,x,y,z,id,meta);
    }
}

static void struct_floor_random_stone(GenCanvas *cv, JavaRandom *r, int x1, int y, int z1, int x2, int z2) {
    for(int z=z1; z<=z2; ++z) for(int x=x1; x<=x2; ++x) {
        int id = BLK_STONE_BRICK, meta = BLOCK_META_STONE_BRICK_NORMAL;
        float f = jr_next_float(r);
        if(f < 0.20f) meta = BLOCK_META_STONE_BRICK_CRACKED;
        else if(f < 0.50f) meta = BLOCK_META_STONE_BRICK_MOSSY;
        else if(f < 0.55f) { id = BLK_SILVERFISH; meta = 2; }
        struct_set(cv,x,y,z,id,meta);
    }
}

static void struct_line_x(GenCanvas *cv, int x1, int x2, int y, int z, int id, int meta) {
    if(x2 < x1) { int t=x1; x1=x2; x2=t; }
    for(int x=x1; x<=x2; ++x) struct_set(cv,x,y,z,id,meta);
}
static void struct_line_z(GenCanvas *cv, int x, int y, int z1, int z2, int id, int meta) {
    if(z2 < z1) { int t=z1; z1=z2; z2=t; }
    for(int z=z1; z<=z2; ++z) struct_set(cv,x,y,z,id,meta);
}

static void struct_place_stronghold_portal_room(GenCanvas *cv, JavaRandom *r, int x0, int y0, int z0) {
    /* Shape follows ComponentStrongholdPortalRoom dimensions: 11 x 8 x 16. */
    struct_hollow_box_random_stone(cv,r,x0,y0,z0,x0+10,y0+7,z0+15);
    struct_clear_box(cv,x0+1,y0+1,z0+1,x0+9,y0+5,z0+14);
    struct_floor_random_stone(cv,r,x0+1,y0+1,z0+1,x0+9,z0+14);
    struct_fill_box(cv,x0+1,y0+1,z0+1,x0+1,y0+1,z0+3,BLK_LAVA,0);
    struct_fill_box(cv,x0+9,y0+1,z0+1,x0+9,y0+1,z0+3,BLK_LAVA,0);
    struct_fill_box(cv,x0+4,y0+1,z0+9,x0+6,y0+1,z0+11,BLK_LAVA,0);
    for(int zz=3; zz<14; zz+=2) { struct_fill_box(cv,x0,y0+3,z0+zz,x0,y0+4,z0+zz,BLK_IRON_BARS,0); struct_fill_box(cv,x0+10,y0+3,z0+zz,x0+10,y0+4,z0+zz,BLK_IRON_BARS,0); }
    for(int xx=2; xx<9; xx+=2) struct_fill_box(cv,x0+xx,y0+3,z0+15,x0+xx,y0+4,z0+15,BLK_IRON_BARS,0);
    for(int yy=1; yy<=3; ++yy) for(int xx=4; xx<=6; ++xx) struct_set(cv,x0+xx,y0+yy,z0+3+yy,BLK_STONE_BRICK_STAIRS,3);
    int metaNorth = 0, metaSouth = 2, metaWest = 3, metaEast = 1;
    for(int xx=4; xx<=6; ++xx) {
        struct_set(cv,x0+xx,y0+3,z0+8,BLK_END_PORTAL_FRAME,metaNorth + (jr_next_float(r) > 0.9f ? 4 : 0));
        struct_set(cv,x0+xx,y0+3,z0+12,BLK_END_PORTAL_FRAME,metaSouth + (jr_next_float(r) > 0.9f ? 4 : 0));
    }
    for(int zz=9; zz<=11; ++zz) {
        struct_set(cv,x0+3,y0+3,z0+zz,BLK_END_PORTAL_FRAME,metaWest + (jr_next_float(r) > 0.9f ? 4 : 0));
        struct_set(cv,x0+7,y0+3,z0+zz,BLK_END_PORTAL_FRAME,metaEast + (jr_next_float(r) > 0.9f ? 4 : 0));
    }
    struct_set(cv,x0+5,y0+3,z0+6,BLK_MOB_SPAWNER,0);
    struct_set(cv,x0+4,y0+4,z0+0,BLK_IRON_BARS,0);
    struct_set(cv,x0+5,y0+4,z0+0,BLK_IRON_BARS,0);
    struct_set(cv,x0+6,y0+4,z0+0,BLK_IRON_BARS,0);
}

static void struct_place_stronghold_library(GenCanvas *cv, JavaRandom *r, int x0, int y0, int z0) {
    struct_hollow_box_random_stone(cv,r,x0,y0,z0,x0+13,y0+6,z0+10);
    struct_clear_box(cv,x0+1,y0+1,z0+1,x0+12,y0+5,z0+9);
    for(int z=z0+2; z<=z0+8; z+=3) {
        struct_line_x(cv,x0+2,x0+11,y0+1,z,BLK_BOOKSHELF,0);
        struct_line_x(cv,x0+2,x0+11,y0+2,z,BLK_BOOKSHELF,0);
    }
    struct_set(cv,x0+7,y0+1,z0+5,BLK_CHEST,2);
    for(int x=x0+2; x<=x0+11; ++x) if((x-x0)%3==0) struct_set(cv,x,y0+4,z0+5,BLK_TORCH,5);
}


static void struct_place_stronghold_prison(GenCanvas *cv, JavaRandom *r, int x0, int y0, int z0) {
    struct_hollow_box_random_stone(cv,r,x0,y0,z0,x0+8,y0+5,z0+10);
    struct_clear_box(cv,x0+1,y0+1,z0+1,x0+7,y0+4,z0+9);
    for(int z=z0+2; z<=z0+8; z+=3) {
        struct_fill_box(cv,x0+1,y0+1,z,x0+1,y0+3,z,BLK_IRON_BARS,0);
        struct_fill_box(cv,x0+4,y0+1,z,x0+4,y0+3,z,BLK_IRON_BARS,0);
        struct_fill_box(cv,x0+7,y0+1,z,x0+7,y0+3,z,BLK_IRON_BARS,0);
        struct_set(cv,x0+2,y0+1,z,BLK_IRON_DOOR,1); struct_set(cv,x0+2,y0+2,z,BLK_IRON_DOOR,8);
        struct_set(cv,x0+5,y0+1,z,BLK_IRON_DOOR,1); struct_set(cv,x0+5,y0+2,z,BLK_IRON_DOOR,8);
    }
    struct_set(cv,x0+4,y0+2,z0+1,BLK_TORCH,4);
}

static void struct_place_stronghold_crossing(GenCanvas *cv, JavaRandom *r, int x0, int y0, int z0) {
    struct_hollow_box_random_stone(cv,r,x0,y0,z0,x0+12,y0+6,z0+12);
    struct_clear_box(cv,x0+1,y0+1,z0+1,x0+11,y0+5,z0+11);
    struct_floor_random_stone(cv,r,x0+1,y0+1,z0+1,x0+11,z0+11);
    struct_line_x(cv,x0+1,x0+11,y0+2,z0+6,BLK_AIR,0);
    struct_line_z(cv,x0+6,y0+2,z0+1,z0+11,BLK_AIR,0);
    struct_fill_box(cv,x0+5,y0+1,z0+5,x0+7,y0+1,z0+7,BLK_STONE_BRICK,0);
    struct_set(cv,x0+6,y0+2,z0+6,BLK_TORCH,5);
    for(int i=0;i<4;i++) {
        int dx = (i==0) ? 1 : (i==1 ? 11 : 6);
        int dz = (i==2) ? 1 : (i==3 ? 11 : 6);
        struct_set(cv,x0+dx,y0+3,z0+dz,BLK_TORCH,5);
    }
}

static void struct_place_stronghold_fountain(GenCanvas *cv, JavaRandom *r, int x0, int y0, int z0) {
    struct_hollow_box_random_stone(cv,r,x0,y0,z0,x0+9,y0+5,z0+9);
    struct_clear_box(cv,x0+1,y0+1,z0+1,x0+8,y0+4,z0+8);
    struct_fill_box(cv,x0+3,y0+1,z0+3,x0+6,y0+1,z0+6,BLK_STONE_BRICK,0);
    struct_fill_box(cv,x0+4,y0+1,z0+4,x0+5,y0+1,z0+5,BLK_STILL_WATER,0);
    struct_fill_box(cv,x0+4,y0+2,z0+4,x0+5,y0+2,z0+5,BLK_STONE_BRICK,0);
    struct_set(cv,x0+4,y0+3,z0+4,BLK_TORCH,5);
    struct_set(cv,x0+5,y0+3,z0+5,BLK_TORCH,5);
}

static void struct_place_stronghold_stairs(GenCanvas *cv, JavaRandom *r, int x0, int y0, int z0) {
    struct_hollow_box_random_stone(cv,r,x0,y0,z0,x0+6,y0+8,z0+6);
    struct_clear_box(cv,x0+1,y0+1,z0+1,x0+5,y0+7,z0+5);
    for(int i=0;i<8;i++) {
        int x=x0+1+(i%5), z=z0+1+((i/2)%5);
        struct_set(cv,x,y0+1+i,z,BLK_STONE_BRICK_STAIRS,(i&3));
        struct_set(cv,x,y0+i,z,BLK_STONE_BRICK,0);
    }
    struct_set(cv,x0+3,y0+5,z0+3,BLK_TORCH,5);
}

static void struct_place_simplified_stronghold(GenCanvas *cv, long long seed, int scx, int scz) {
    JavaRandom r = worldgen_structure_random(seed, scx, scz);
    int baseX = (scx << 4) + 2;
    int baseZ = (scz << 4) + 2;
    int y = 26 + jr_next_int_bound(&r, 16);
    int dir = jr_next_int_bound(&r, 4);
    (void)dir;

    /* ComponentStrongholdStairs2-inspired start. */
    struct_place_stronghold_stairs(cv,&r,baseX,y,baseZ);

    /* Primary spine and portal-room branch. */
    struct_hollow_box_random_stone(cv,&r,baseX+2,y+1,baseZ+5,baseX+2,y+5,baseZ+28);
    struct_clear_box(cv,baseX+1,y+2,baseZ+6,baseX+3,y+4,baseZ+27);
    for(int z=baseZ+7; z<=baseZ+27; z+=5) { struct_set(cv,baseX+1,y+3,z,BLK_TORCH,1); struct_set(cv,baseX+3,y+3,z,BLK_TORCH,2); }
    struct_place_stronghold_portal_room(cv,&r,baseX-3,y,baseZ+28);

    /* Secondary rooms approximate the common Java piece pool, with deterministic
       offsets so neighboring chunks receive repeatable partial components. */
    struct_place_stronghold_library(cv,&r,baseX+8,y,baseZ+10);
    struct_place_stronghold_prison(cv,&r,baseX-14,y,baseZ+7);
    struct_place_stronghold_crossing(cv,&r,baseX-6,y,baseZ-14);
    struct_place_stronghold_fountain(cv,&r,baseX+22,y,baseZ+2);

    /* Corridors connecting the graph. */
    struct_hollow_box_random_stone(cv,&r,baseX+5,y+1,baseZ+13,baseX+22,y+5,baseZ+13);
    struct_clear_box(cv,baseX+6,y+2,baseZ+12,baseX+21,y+4,baseZ+14);
    struct_hollow_box_random_stone(cv,&r,baseX-14,y+1,baseZ+11,baseX-1,y+5,baseZ+11);
    struct_clear_box(cv,baseX-13,y+2,baseZ+10,baseX-2,y+4,baseZ+12);
    struct_hollow_box_random_stone(cv,&r,baseX-1,y+1,baseZ-2,baseX-1,y+5,baseZ+5);
    struct_clear_box(cv,baseX-2,y+2,baseZ-1,baseX,y+4,baseZ+4);
}

static int struct_surface_y(GenCanvas *cv, int x, int z) {
    int y = canvas_top_solid_or_liquid(cv, x, z);
    while(y > 1 && cv_is_water_id(canvas_get(cv,x,y-1,z))) --y;
    return y;
}

static void struct_surface_fill(GenCanvas *cv, int x, int z, int targetY, int block, int meta) {
    int top = struct_surface_y(cv,x,z);
    if(top < targetY) {
        for(int y=top; y<=targetY; ++y) struct_set(cv,x,y,z,block,meta);
    } else {
        for(int y=top; y>targetY; --y) struct_set(cv,x,y,z,BLK_AIR,0);
        struct_set(cv,x,targetY,z,block,meta);
    }
}

static void struct_place_door(GenCanvas *cv, int x, int y, int z, int metaLower) {
    struct_set(cv,x,y,z,BLK_WOOD_DOOR,metaLower & 15);
    struct_set(cv,x,y+1,z,BLK_WOOD_DOOR,8);
}

static void struct_place_village_well(GenCanvas *cv, int x0, int y, int z0) {
    struct_fill_box(cv,x0-3,y-1,z0-3,x0+3,y-1,z0+3,BLK_COBBLESTONE,0);
    struct_fill_box(cv,x0-2,y,z0-2,x0+2,y,z0+2,BLK_COBBLESTONE,0);
    struct_fill_box(cv,x0-1,y,z0-1,x0+1,y,z0+1,BLK_STILL_WATER,0);
    for(int dx=-2; dx<=2; dx+=4) for(int dz=-2; dz<=2; dz+=4) struct_fill_box(cv,x0+dx,y+1,z0+dz,x0+dx,y+3,z0+dz,BLK_FENCE,0);
    struct_fill_box(cv,x0-2,y+4,z0-2,x0+2,y+4,z0+2,BLK_WOOD_PLANKS,0);
    struct_set(cv,x0,y+5,z0,BLK_FENCE,0);
}

static void struct_place_village_house(GenCanvas *cv, int x0, int y, int z0, int desert) {
    int wall = desert ? BLK_SANDSTONE : BLK_WOOD_PLANKS;
    int roof = desert ? BLK_SANDSTONE : BLK_WOOD_STAIRS;
    struct_fill_box(cv,x0,y-1,z0,x0+6,y-1,z0+6,wall,0);
    for(int yy=y; yy<=y+3; ++yy) for(int zz=z0; zz<=z0+6; ++zz) for(int xx=x0; xx<=x0+6; ++xx) {
        int edge = xx==x0 || xx==x0+6 || zz==z0 || zz==z0+6;
        if(edge) struct_set(cv,xx,yy,zz,wall,0); else struct_set(cv,xx,yy,zz,BLK_AIR,0);
    }
    struct_fill_box(cv,x0,y+4,z0,x0+6,y+4,z0+6,roof,0);
    struct_clear_box(cv,x0+3,y,z0,x0+3,y+1,z0);
    struct_place_door(cv,x0+3,y,z0,1);
    struct_set(cv,x0+1,y+2,z0,BLK_GLASS_PANE,0); struct_set(cv,x0+5,y+2,z0,BLK_GLASS_PANE,0);
    struct_set(cv,x0+1,y+1,z0+5,BLK_CHEST,2);
    struct_set(cv,x0+5,y+2,z0+3,BLK_TORCH,5);
}

static void struct_place_village_farm(GenCanvas *cv, int x0, int y, int z0) {
    struct_fill_box(cv,x0,y-1,z0,x0+8,y-1,z0+6,BLK_DIRT,0);
    for(int z=0; z<=6; ++z) for(int x=0; x<=8; ++x) {
        if(x==4) struct_set(cv,x0+x,y,z0+z,BLK_STILL_WATER,0);
        else { struct_set(cv,x0+x,y,z0+z,BLK_FARMLAND,7); if(z>0 && z<6) struct_set(cv,x0+x,y+1,z0+z,BLK_CROPS,7); }
    }
    struct_line_z(cv,x0-1,y,z0-1,z0+7,BLK_WOOD_PLANKS,0);
    struct_line_z(cv,x0+9,y,z0-1,z0+7,BLK_WOOD_PLANKS,0);
}

static void struct_place_village_roads(GenCanvas *cv, int cx, int cz, int y) {
    for(int dx=-24; dx<=24; ++dx) {
        for(int w=-1; w<=1; ++w) struct_surface_fill(cv,cx+dx,cz+w,y,BLK_GRAVEL,0);
    }
    for(int dz=-24; dz<=24; ++dz) {
        for(int w=-1; w<=1; ++w) struct_surface_fill(cv,cx+w,cz+dz,y,BLK_GRAVEL,0);
    }
}


static void struct_place_village_blacksmith(GenCanvas *cv, int x0, int y, int z0, int desert) {
    int wall = desert ? BLK_SANDSTONE : BLK_COBBLESTONE;
    int roof = desert ? BLK_SANDSTONE : BLK_WOOD_PLANKS;
    struct_fill_box(cv,x0,y-1,z0,x0+9,y-1,z0+6,BLK_COBBLESTONE,0);
    struct_fill_box(cv,x0,y,z0,x0+9,y+3,z0+6,BLK_AIR,0);
    for(int yy=y; yy<=y+3; ++yy) for(int zz=z0; zz<=z0+6; ++zz) for(int xx=x0; xx<=x0+9; ++xx) {
        int edge = xx==x0 || xx==x0+9 || zz==z0 || zz==z0+6;
        if(edge) struct_set(cv,xx,yy,zz,wall,0);
    }
    struct_fill_box(cv,x0,y+4,z0,x0+9,y+4,z0+6,roof,0);
    struct_clear_box(cv,x0+4,y,z0,x0+4,y+1,z0);
    struct_place_door(cv,x0+4,y,z0,1);
    struct_set(cv,x0+7,y+1,z0+1,BLK_FURNACE,2);
    struct_set(cv,x0+7,y+1,z0+2,BLK_FURNACE,2);
    struct_set(cv,x0+2,y+1,z0+5,BLK_CHEST,3);
    struct_set(cv,x0+5,y+1,z0+5,BLK_LAVA,0);
    struct_fill_box(cv,x0+4,y+1,z0+4,x0+6,y+1,z0+6,BLK_IRON_BARS,0);
    struct_set(cv,x0+2,y+2,z0,BLK_GLASS_PANE,0); struct_set(cv,x0+7,y+2,z0,BLK_GLASS_PANE,0);
}

static void struct_place_village_church(GenCanvas *cv, int x0, int y, int z0, int desert) {
    int wall = desert ? BLK_SANDSTONE : BLK_COBBLESTONE;
    struct_fill_box(cv,x0,y-1,z0,x0+5,y-1,z0+8,wall,0);
    for(int yy=y; yy<=y+5; ++yy) for(int zz=z0; zz<=z0+8; ++zz) for(int xx=x0; xx<=x0+5; ++xx) {
        int edge = xx==x0 || xx==x0+5 || zz==z0 || zz==z0+8;
        if(edge) struct_set(cv,xx,yy,zz,wall,0); else struct_set(cv,xx,yy,zz,BLK_AIR,0);
    }
    struct_fill_box(cv,x0+1,y+6,z0+1,x0+4,y+6,z0+7,wall,0);
    struct_fill_box(cv,x0+1,y,z0+6,x0+4,y+9,z0+8,wall,0);
    struct_clear_box(cv,x0+2,y,z0,x0+3,y+1,z0);
    struct_place_door(cv,x0+2,y,z0,1);
    struct_set(cv,x0+1,y+3,z0+4,BLK_GLASS_PANE,0); struct_set(cv,x0+4,y+3,z0+4,BLK_GLASS_PANE,0);
    for(int yy=y+1; yy<=y+8; ++yy) struct_set(cv,x0+2,yy,z0+7,BLK_LADDER,4);
    struct_set(cv,x0+2,y+10,z0+7,BLK_TORCH,5);
}

static void struct_place_village_lamp(GenCanvas *cv, int x, int y, int z) {
    struct_fill_box(cv,x,y,z,x,y+3,z,BLK_FENCE,0);
    struct_set(cv,x,y+4,z,BLK_WOOL,0);
    struct_set(cv,x,y+5,z,BLK_TORCH,5);
}

static void struct_place_simplified_village(GenCanvas *cv, long long seed, int vcx, int vcz) {
    JavaRandom r = worldgen_structure_random(seed, vcx, vcz);
    int cx = (vcx << 4) + 8;
    int cz = (vcz << 4) + 8;
    int y = struct_surface_y(cv,cx,cz);
    if(y < 2 || y > 110) return;
    WorldBiome b = world_biome_list[BIOME_PLAINS];
    (void)b;
    int desert = 0;
    /* Querying exact biome here avoids desert villages being made from wood. */
    int topId = canvas_get(cv,cx,y-1,cz);
    if(topId == BLK_SAND || topId == BLK_SANDSTONE) desert = 1;
    struct_place_village_roads(cv,cx,cz,y);
    struct_place_village_well(cv,cx,y,cz);
    struct_place_village_house(cv,cx+8,y,cz-4,desert);
    struct_place_village_house(cv,cx-15,y,cz+6,desert);
    struct_place_village_farm(cv,cx-6,y,cz-15);
    struct_place_village_blacksmith(cv,cx+12,y,cz+10,desert);
    struct_place_village_church(cv,cx-24,y,cz-12,desert);
    for(int i=0;i<4;i++) {
        int sx = cx + (i<2 ? -18 : 18);
        int sz = cz + (i&1 ? -18 : 18);
        int ty = struct_surface_y(cv,sx,sz);
        struct_place_village_lamp(cv,sx,ty,sz);
    }
    /* Consume some random calls to keep future expansion closer to StructureVillageStart. */
    for(int i=0;i<8;i++) jr_next_int_bound(&r, 16);
}

static void struct_place_mineshaft_corridor_x(GenCanvas *cv, JavaRandom *r, int x1, int x2, int y, int z) {
    if(x2 < x1) { int t=x1; x1=x2; x2=t; }
    for(int x=x1; x<=x2; ++x) {
        for(int yy=y; yy<=y+3; ++yy) for(int zz=z-1; zz<=z+1; ++zz) {
            if(yy==y || yy==y+3 || zz==z-1 || zz==z+1) {
                if(canvas_get(cv,x,yy,zz)==BLK_AIR) struct_set(cv,x,yy,zz,BLK_WOOD_PLANKS,0);
            } else struct_set(cv,x,yy,zz,BLK_AIR,0);
        }
        if((x-x1)%5==0) { struct_set(cv,x,y+1,z-1,BLK_FENCE,0); struct_set(cv,x,y+1,z+1,BLK_FENCE,0); struct_set(cv,x,y+2,z-1,BLK_FENCE,0); struct_set(cv,x,y+2,z+1,BLK_FENCE,0); }
        if((x-x1)%3==0) struct_set(cv,x,y+1,z,BLK_RAILS,1);
        if(jr_next_int_bound(r, 23) == 0) struct_set(cv,x,y+2,z+(jr_next_int_bound(r,2)?-1:1),BLK_WEB,0);
    }
}

static void struct_place_mineshaft_corridor_z(GenCanvas *cv, JavaRandom *r, int x, int y, int z1, int z2) {
    if(z2 < z1) { int t=z1; z1=z2; z2=t; }
    for(int z=z1; z<=z2; ++z) {
        for(int yy=y; yy<=y+3; ++yy) for(int xx=x-1; xx<=x+1; ++xx) {
            if(yy==y || yy==y+3 || xx==x-1 || xx==x+1) {
                if(canvas_get(cv,xx,yy,z)==BLK_AIR) struct_set(cv,xx,yy,z,BLK_WOOD_PLANKS,0);
            } else struct_set(cv,xx,yy,z,BLK_AIR,0);
        }
        if((z-z1)%5==0) { struct_set(cv,x-1,y+1,z,BLK_FENCE,0); struct_set(cv,x+1,y+1,z,BLK_FENCE,0); struct_set(cv,x-1,y+2,z,BLK_FENCE,0); struct_set(cv,x+1,y+2,z,BLK_FENCE,0); }
        if((z-z1)%3==0) struct_set(cv,x,y+1,z,BLK_RAILS,0);
        if(jr_next_int_bound(r, 23) == 0) struct_set(cv,x+(jr_next_int_bound(r,2)?-1:1),y+2,z,BLK_WEB,0);
    }
}


static void struct_place_mineshaft_crossing(GenCanvas *cv, JavaRandom *r, int cx, int y, int cz) {
    struct_clear_box(cv,cx-4,y,cz-4,cx+4,y+4,cz+4);
    struct_fill_box(cv,cx-4,y,cz-4,cx+4,y,cz+4,BLK_WOOD_PLANKS,0);
    struct_line_x(cv,cx-4,cx+4,y+1,cz,BLK_RAILS,1);
    struct_line_z(cv,cx,y+1,cz-4,cz+4,BLK_RAILS,0);
    for(int dx=-3; dx<=3; dx+=6) for(int dz=-3; dz<=3; dz+=6) {
        struct_fill_box(cv,cx+dx,y+1,cz+dz,cx+dx,y+3,cz+dz,BLK_FENCE,0);
        struct_set(cv,cx+dx,y+4,cz+dz,BLK_WOOD_PLANKS,0);
    }
    if(jr_next_int_bound(r,3)==0) struct_set(cv,cx+1,y+1,cz+1,BLK_CHEST,2);
}

static void struct_place_mineshaft_side_room(GenCanvas *cv, JavaRandom *r, int x0, int y, int z0) {
    struct_clear_box(cv,x0,y,z0,x0+6,y+4,z0+6);
    struct_fill_box(cv,x0,y,z0,x0+6,y,z0+6,BLK_WOOD_PLANKS,0);
    for(int x=x0; x<=x0+6; x+=6) for(int z=z0; z<=z0+6; z+=6) struct_fill_box(cv,x,y+1,z,x,y+3,z,BLK_FENCE,0);
    for(int i=0;i<18;i++) {
        int x=x0+jr_next_int_bound(r,7), z=z0+jr_next_int_bound(r,7), yy=y+1+jr_next_int_bound(r,3);
        if(jr_next_int_bound(r,4)==0) struct_set(cv,x,yy,z,BLK_WEB,0);
    }
    struct_set(cv,x0+3,y+1,z0+3,BLK_MOB_SPAWNER,0);
}

static void struct_place_simplified_mineshaft(GenCanvas *cv, long long seed, int mcx, int mcz) {
    JavaRandom r = worldgen_structure_random(seed, mcx, mcz);
    int cx = (mcx << 4) + 8;
    int cz = (mcz << 4) + 8;
    int y = 18 + jr_next_int_bound(&r, 28);
    struct_hollow_box_random_stone(cv,&r,cx-4,y,cz-4,cx+4,y+5,cz+4);
    struct_clear_box(cv,cx-3,y+1,cz-3,cx+3,y+4,cz+3);
    struct_place_mineshaft_corridor_x(cv,&r,cx-34,cx+34,y+1,cz);
    struct_place_mineshaft_corridor_z(cv,&r,cx,y+1,cz-34,cz+34);
    struct_place_mineshaft_crossing(cv,&r,cx,y+1,cz);
    struct_place_mineshaft_corridor_x(cv,&r,cx-20,cx+8,y+1,cz+18);
    struct_place_mineshaft_corridor_z(cv,&r,cx+18,y+1,cz-20,cz+8);
    struct_place_mineshaft_side_room(cv,&r,cx-12,y+1,cz+12);
    struct_set(cv,cx+2,y+1,cz+2,BLK_CHEST,2);
    if(jr_next_int_bound(&r,4)==0) struct_set(cv,cx-2,y+1,cz-2,BLK_MOB_SPAWNER,0);
}

static int structure_chunk_in_canvas_reach(GenCanvas *cv, int chunkX, int chunkZ, int reachChunks) {
    int min = cv->minCx - reachChunks, max = cv->minCx + cv->chunks - 1 + reachChunks;
    int minz = cv->minCz - reachChunks, maxz = cv->minCz + cv->chunks - 1 + reachChunks;
    return chunkX >= min && chunkX <= max && chunkZ >= minz && chunkZ <= maxz;
}

static void worldgen_place_structure_blocks(TerrainProvider *tp, GenCanvas *cv, int centerCx, int centerCz) {
    if (!g_world_map_features) return;
    int sx[3], sz[3];
    worldgen_get_stronghold_coords((long long)tp->worldSeed, sx, sz);
    for(int i=0;i<3;i++) if(structure_chunk_in_canvas_reach(cv,sx[i],sz[i],4)) struct_place_simplified_stronghold(cv,(long long)tp->worldSeed,sx[i],sz[i]);
    /* MapGenStructure.generate scans source chunks within range 8 for starts. */
    for(int x=centerCx-8; x<=centerCx+8; ++x) for(int z=centerCz-8; z<=centerCz+8; ++z) {
        if(worldgen_mineshaft_can_spawn((long long)tp->worldSeed,x,z) && structure_chunk_in_canvas_reach(cv,x,z,4)) struct_place_simplified_mineshaft(cv,(long long)tp->worldSeed,x,z);
        if(worldgen_village_can_spawn((long long)tp->worldSeed,x,z) && structure_chunk_in_canvas_reach(cv,x,z,4)) struct_place_simplified_village(cv,(long long)tp->worldSeed,x,z);
    }
}

static void generate_canvas_chunk(TerrainProvider *tp, GenCanvas *cv, int cx, int cz) {
    unsigned char *tmp=(unsigned char*)calloc(32768,1); if(!tmp) return;
    jr_set_seed(&tp->rand, (int64_t)((uint64_t)(int64_t)cx * UINT64_C(341873128712) + (uint64_t)(int64_t)cz * UINT64_C(132897987541)));
    WorldBiome *biomes=biome_manager_get(&tp->biomeManager,cx*16,cz*16,16,16);
    qm_generate_base(tp,cx,cz,tmp);
    qm_replace_surface(tp,cx,cz,tmp,biomes);
    qm_generate_caves(tp,cx,cz,tmp);
    qm_generate_ravines(tp,cx,cz,tmp);
    int slot=canvas_chunk_slot(cv,cx,cz);
    if(slot>=0 && slot<cv->chunks*cv->chunks) {
        memcpy(cv->blocks + slot*32768, tmp, 32768);
        if(cv->meta) memset(cv->meta + slot*32768, 0, 32768);
    }
    free(tmp);
}
static void qm_populate_canvas(TerrainProvider *tp, GenCanvas *cv, int cx, int cz) {
    int baseX=cx*16, baseZ=cz*16;
    WorldBiome centerBiome = biome_manager_get(&tp->biomeManager, baseX+16, baseZ+16, 1, 1)[0];
    JavaRandom *r=&tp->rand;
    jr_set_seed(r, tp->worldSeed);
    int64_t s1=jr_next_long(r)/2LL*2LL+1LL, s2=jr_next_long(r)/2LL*2LL+1LL;
    jr_set_seed(r, ((int64_t)((uint64_t)(int64_t)cx*(uint64_t)s1 + (uint64_t)(int64_t)cz*(uint64_t)s2)) ^ tp->worldSeed);

    int villageGenerated = 0;
    for(int vx=cx-8; vx<=cx+8 && !villageGenerated; ++vx) for(int vz=cz-8; vz<=cz+8 && !villageGenerated; ++vz) {
        if(worldgen_village_can_spawn((long long)tp->worldSeed, vx, vz)) {
            int vwx=(vx<<4)+8, vwz=(vz<<4)+8;
            if(vwx >= baseX-48 && vwx <= baseX+64 && vwz >= baseZ-48 && vwz <= baseZ+64) villageGenerated = 1;
        }
    }
    if(!villageGenerated && jr_next_int_bound(r,4)==0) canvas_lake(cv,r,BLK_STILL_WATER,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16)+8);
    if(!villageGenerated && jr_next_int_bound(r,8)==0) { int x=baseX+jr_next_int_bound(r,16)+8, y=jr_next_int_bound(r,jr_next_int_bound(r,120)+8), z=baseZ+jr_next_int_bound(r,16)+8; if(y<63 || jr_next_int_bound(r,10)==0) canvas_lake(cv,r,BLK_STILL_LAVA,x,y,z); }
    for(int i=0;i<8;i++) canvas_dungeon(cv,r,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16)+8);

    /* BiomeDecorator.generateOres(), in exact Java order. */
    for(int i=0;i<20;i++) canvas_place_minable(cv,r,BLK_DIRT,32,baseX+jr_next_int_bound(r,16),jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16),0);
    for(int i=0;i<10;i++) canvas_place_minable(cv,r,BLK_GRAVEL,32,baseX+jr_next_int_bound(r,16),jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16),0);
    for(int i=0;i<20;i++) canvas_place_minable(cv,r,BLK_COAL_ORE,16,baseX+jr_next_int_bound(r,16),jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16),0);
    for(int i=0;i<20;i++) canvas_place_minable(cv,r,BLK_IRON_ORE,8,baseX+jr_next_int_bound(r,16),jr_next_int_bound(r,64),baseZ+jr_next_int_bound(r,16),0);
    for(int i=0;i<2;i++) canvas_place_minable(cv,r,BLK_GOLD_ORE,8,baseX+jr_next_int_bound(r,16),jr_next_int_bound(r,32),baseZ+jr_next_int_bound(r,16),0);
    for(int i=0;i<8;i++) canvas_place_minable(cv,r,BLK_REDSTONE_ORE,7,baseX+jr_next_int_bound(r,16),jr_next_int_bound(r,16),baseZ+jr_next_int_bound(r,16),0);
    for(int i=0;i<1;i++) canvas_place_minable(cv,r,BLK_DIAMOND_ORE,7,baseX+jr_next_int_bound(r,16),jr_next_int_bound(r,16),baseZ+jr_next_int_bound(r,16),0);
    for(int i=0;i<1;i++) canvas_place_minable(cv,r,BLK_LAPIS_ORE,6,baseX+jr_next_int_bound(r,16),jr_next_int_bound(r,16)+jr_next_int_bound(r,16),baseZ+jr_next_int_bound(r,16),0);

    for(int i=0;i<centerBiome.sandPerChunk2;i++) { int x=baseX+jr_next_int_bound(r,16)+8, z=baseZ+jr_next_int_bound(r,16)+8; canvas_place_sand_or_gravel_patch(cv,r,BLK_SAND,7,x,canvas_top_solid_or_liquid(cv,x,z),z); }
    for(int i=0;i<centerBiome.clayPerChunk;i++) { int x=baseX+jr_next_int_bound(r,16)+8, z=baseZ+jr_next_int_bound(r,16)+8; canvas_place_clay_patch(cv,r,4,x,canvas_top_solid_or_liquid(cv,x,z),z); }
    for(int i=0;i<centerBiome.sandPerChunk;i++) { int x=baseX+jr_next_int_bound(r,16)+8, z=baseZ+jr_next_int_bound(r,16)+8; canvas_place_sand_or_gravel_patch(cv,r,BLK_SAND,7,x,canvas_top_solid_or_liquid(cv,x,z),z); }

    int treeCount=centerBiome.treesPerChunk;
    if(jr_next_int_bound(r,10)==0) treeCount++;
    for(int i=0;i<treeCount;i++) {
        int x=baseX+jr_next_int_bound(r,16)+8, z=baseZ+jr_next_int_bound(r,16)+8;
        int y=canvas_top_solid(cv,x,z);
        canvas_place_biome_tree(cv,r,centerBiome,x,y,z);
    }

    for(int i=0;i<centerBiome.bigMushroomsPerChunk;i++) { int x=baseX+jr_next_int_bound(r,16)+8, z=baseZ+jr_next_int_bound(r,16)+8; canvas_place_big_mushroom(cv,r,x,canvas_top_solid(cv,x,z),z); }

    for(int i=0;i<centerBiome.flowersPerChunk;i++) {
        canvas_place_flowers(cv,r,BLK_YELLOW_FLOWER,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16)+8);
        if(jr_next_int_bound(r,4)==0) canvas_place_flowers(cv,r,BLK_RED_ROSE,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16)+8);
    }
    for(int i=0;i<centerBiome.grassPerChunk;i++) {
        int meta = ((centerBiome.id==BIOME_JUNGLE || centerBiome.id==BIOME_JUNGLE_HILLS) && jr_next_int_bound(r,4)==0) ? 2 : 1;
        canvas_place_tall_grass(cv,r,meta,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16)+8);
    }
    for(int i=0;i<centerBiome.deadBushPerChunk;i++) canvas_place_dead_bush(cv,r,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16)+8);
    for(int i=0;i<centerBiome.waterlilyPerChunk;i++) {
        int x=baseX+jr_next_int_bound(r,16)+8, z=baseZ+jr_next_int_bound(r,16)+8;
        int y=jr_next_int_bound(r,128);
        while(y > 0 && canvas_get(cv,x,y-1,z)==0) --y;
        canvas_place_waterlily(cv,r,x,y,z);
    }
    for(int i=0;i<centerBiome.mushroomsPerChunk;i++) {
        if(jr_next_int_bound(r,4)==0) { int x=baseX+jr_next_int_bound(r,16)+8, z=baseZ+jr_next_int_bound(r,16)+8; canvas_place_flowers(cv,r,BLK_BROWN_MUSHROOM,x,canvas_top_solid(cv,x,z),z); }
        if(jr_next_int_bound(r,8)==0) canvas_place_flowers(cv,r,BLK_RED_MUSHROOM,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16)+8);
    }
    if(jr_next_int_bound(r,4)==0) canvas_place_flowers(cv,r,BLK_BROWN_MUSHROOM,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16)+8);
    if(jr_next_int_bound(r,8)==0) canvas_place_flowers(cv,r,BLK_RED_MUSHROOM,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16)+8);

    for(int i=0;i<centerBiome.reedsPerChunk;i++) canvas_place_reeds(cv,r,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16)+8);
    for(int i=0;i<10;i++) canvas_place_reeds(cv,r,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16)+8);
    if(jr_next_int_bound(r,32)==0) canvas_place_pumpkin(cv,r,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16)+8);
    for(int i=0;i<centerBiome.cactiPerChunk;i++) canvas_place_cactus(cv,r,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,128),baseZ+jr_next_int_bound(r,16)+8);

    for(int i=0;i<50;i++) canvas_place_liquid_spring(cv,BLK_WATER,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,jr_next_int_bound(r,120)+8),baseZ+jr_next_int_bound(r,16)+8);
    for(int i=0;i<20;i++) canvas_place_liquid_spring(cv,BLK_LAVA,baseX+jr_next_int_bound(r,16)+8,jr_next_int_bound(r,jr_next_int_bound(r,jr_next_int_bound(r,112)+8)+8),baseZ+jr_next_int_bound(r,16)+8);

    if(centerBiome.id == BIOME_JUNGLE || centerBiome.id == BIOME_JUNGLE_HILLS) {
        /* BiomeGenJungle.decorate runs after the shared decorator and starts
           each WorldGenVines at y=64. */
        canvas_place_jungle_vines(cv,r,baseX,baseZ);
    }
    if(centerBiome.id == BIOME_DESERT || centerBiome.id == BIOME_DESERT_HILLS) {
        if(jr_next_int_bound(r,1000)==0) {
            int x=baseX+jr_next_int_bound(r,16)+8, z=baseZ+jr_next_int_bound(r,16)+8;
            canvas_place_desert_well(cv,x,canvas_top_solid(cv,x,z)+1,z);
        }
    }

    tp->snow = biome_manager_temperatures(&tp->biomeManager, tp->snow, &tp->snowCap, baseX+8, baseZ+8, 16,16);
    for(int x=baseX+8; x<baseX+24; x++) for(int z=baseZ+8; z<baseZ+24; z++) {
        int ix=x-(baseX+8), iz=z-(baseZ+8), y=canvas_snow_surface_y(cv,x,z);
        double temp=tp->snow[ix*16+iz] - (y-64)/64.0*0.3;
        if(temp < 0.5 && y>0 && y<128 && cv_is_water_id(canvas_get(cv,x,y-1,z))) canvas_set(cv,x,y-1,z,BLK_ICE);
        if(temp < 0.5 && y>0 && y<128 && canvas_get(cv,x,y,z)==0 && canvas_can_snow_stay_at(cv,x,y,z)) canvas_set(cv,x,y,z,BLK_SNOW_LAYER);
    }
}
static void pack_chunk_metadata_nibbles(const unsigned char *meta_bytes, unsigned char *data_nibbles) {
    memset(data_nibbles, 0, 16384);
    if(!meta_bytes) return;
    for(int idx=0; idx<32768; idx++) {
        set_nibble(data_nibbles, idx, meta_bytes[idx] & 15);
    }
}

static void extract_canvas_chunk(GenCanvas *cv, int cx, int cz, unsigned char *out, unsigned char *data) {
    int slot=canvas_chunk_slot(cv,cx,cz);
    memcpy(out, cv->blocks + slot*32768, 32768);
    if(data) pack_chunk_metadata_nibbles(cv->meta ? cv->meta + slot*32768 : NULL, data);
}


static void recalc_light_and_height(unsigned char *blocks, unsigned char *sky, unsigned char *heightmap) {
    memset(sky, 0, 16384);
    for(int x=0;x<16;x++) for(int z=0;z<16;z++) {
        int top=0;
        for(int y=127;y>=0;y--) { unsigned char id=get_block_local(blocks,x,y,z); if(id!=0 && id!=BLK_LEAVES && id!=BLK_SNOW_LAYER && id!=BLK_STILL_WATER && id!=BLK_WATER && id!=BLK_ICE) { top=y+1; break; } }
        heightmap[z<<4|x]=(unsigned char)top;
        int sun=1;
        for(int y=127;y>=0;y--) {
            int idx=chunk_index(x,y,z);
            unsigned char id=blocks[idx];
            if(sun) set_nibble(sky,idx,15); else set_nibble(sky,idx,0);
            if(id!=0 && id!=BLK_LEAVES && id!=BLK_SNOW_LAYER && id!=BLK_STILL_WATER && id!=BLK_WATER && id!=BLK_ICE) sun=0;
        }
    }
}

int worldgen_generate_chunk_arrays(long long seed, int cx, int cz, unsigned char *blocks, unsigned char *data, unsigned char *heightmap, unsigned char *biome_array) {
    if (!blocks || !data || !heightmap || !biome_array) return 0;
    memset(blocks, 0, 32768);
    memset(data, 0, 16384);
    memset(heightmap, 0, 256);
    memset(biome_array, 0, 256);

    TerrainProvider *tp = (TerrainProvider*)calloc(1, sizeof(*tp));
    if (!tp) return 0;
    terrain_provider_init(tp, (int64_t)seed);

    GenCanvas cv;
    cv.minCx = cx - 1;
    cv.minCz = cz - 1;
    cv.chunks = 3;
    cv.blocks = (unsigned char*)calloc((size_t)cv.chunks * (size_t)cv.chunks * 32768u, 1);
    cv.meta = (unsigned char*)calloc((size_t)cv.chunks * (size_t)cv.chunks * 32768u, 1);
    if (!cv.blocks || !cv.meta) {
        free(cv.blocks);
        free(cv.meta);
        terrain_provider_free(tp);
        free(tp);
        return 0;
    }

    for (int dz = 0; dz < cv.chunks; dz++) {
        for (int dx = 0; dx < cv.chunks; dx++) {
            generate_canvas_chunk(tp, &cv, cv.minCx + dx, cv.minCz + dz);
        }
    }

    worldgen_place_structure_blocks(tp, &cv, cx, cz);

    for (int pz = cz - 1; pz <= cz; pz++) {
        for (int px = cx - 1; px <= cx; px++) {
            qm_populate_canvas(tp, &cv, px, pz);
        }
    }

    extract_canvas_chunk(&cv, cx, cz, blocks, data);
    biome_manager_write_biome_array(&tp->biomeManager, cx, cz, biome_array);
    free(cv.blocks);
    free(cv.meta);

    unsigned char *sky = (unsigned char*)calloc(16384, 1);
    if (sky) {
        recalc_light_and_height(blocks, sky, heightmap);
        free(sky);
    } else {
        for (int x = 0; x < 16; x++) for (int z = 0; z < 16; z++) {
            int top = 0;
            for (int y = 127; y >= 0; y--) {
                unsigned char id = get_block_local(blocks, x, y, z);
                if (id != 0 && id != BLK_LEAVES && id != BLK_SNOW_LAYER && id != BLK_STILL_WATER && id != BLK_WATER && id != BLK_ICE) { top = y + 1; break; }
            }
            heightmap[z << 4 | x] = (unsigned char)top;
        }
    }
    terrain_provider_free(tp);
    free(tp);
    return 1;
}


/* -------------------------------------------------------------------------
   Debug / trace generation helpers.

   These are intentionally internal testing hooks: Nether and End chunks can be
   generated by tools and trace commands, but no portal or gameplay dimension
   travel is connected here.
   ------------------------------------------------------------------------- */
typedef struct NetherProvider {
    JavaRandom rand;
    OctaveNoise n1,n2,n3,slowsandGravel,exclusive,n6,n7;
    double *density,*slow,*gravel,*exclusiveNoise,*d1,*d2,*d3,*d4,*d5;
    int densityCap,slowCap,gravelCap,exclusiveCap,d1Cap,d2Cap,d3Cap,d4Cap,d5Cap;
} NetherProvider;

static void nether_provider_init(NetherProvider *np, int64_t seed) {
    memset(np,0,sizeof(*np));
    jr_set_seed(&np->rand, seed);
    octave_init(&np->n1,&np->rand,16);
    octave_init(&np->n2,&np->rand,16);
    octave_init(&np->n3,&np->rand,8);
    octave_init(&np->slowsandGravel,&np->rand,4);
    octave_init(&np->exclusive,&np->rand,4);
    octave_init(&np->n6,&np->rand,10);
    octave_init(&np->n7,&np->rand,16);
}
static void nether_provider_free(NetherProvider *np) {
    free(np->density); free(np->slow); free(np->gravel); free(np->exclusiveNoise);
    free(np->d1); free(np->d2); free(np->d3); free(np->d4); free(np->d5);
}

static double *nether_density(NetherProvider *np, int x, int y, int z, int xs, int ys, int zs) {
    int n=xs*ys*zs;
    if(!np->density || np->densityCap<n) { free(np->density); np->density=(double*)malloc(sizeof(double)*(size_t)n); np->densityCap=n; }
    if(!np->density) ExitProcess(2);
    double scaleXZ=684.412, scaleY=2053.236;
    np->d4 = octave_fill3(&np->n6,np->d4,&np->d4Cap,x,y,z,xs,1,zs,1.0,0.0,1.0);
    np->d5 = octave_fill3(&np->n7,np->d5,&np->d5Cap,x,y,z,xs,1,zs,100.0,0.0,100.0);
    np->d1 = octave_fill3(&np->n3,np->d1,&np->d1Cap,x,y,z,xs,ys,zs,scaleXZ/80.0,scaleY/60.0,scaleXZ/80.0);
    np->d2 = octave_fill3(&np->n1,np->d2,&np->d2Cap,x,y,z,xs,ys,zs,scaleXZ,scaleY,scaleXZ);
    np->d3 = octave_fill3(&np->n2,np->d3,&np->d3Cap,x,y,z,xs,ys,zs,scaleXZ,scaleY,scaleXZ);
    double yCurve[64];
    for(int yi=0; yi<ys; ++yi) {
        yCurve[yi] = cos((double)yi * M_PI * 6.0 / (double)ys) * 2.0;
        double edge=(double)yi;
        if(yi > ys/2) edge=(double)(ys-1-yi);
        if(edge < 4.0) { edge = 4.0 - edge; yCurve[yi] -= edge*edge*edge*10.0; }
    }
    int out=0, n2d=0;
    for(int xi=0; xi<xs; ++xi) for(int zi=0; zi<zs; ++zi) {
        double d4=(np->d4[n2d]+256.0)/512.0; if(d4>1.0) d4=1.0;
        double floorShape=0.0;
        double d5=np->d5[n2d]/8000.0; if(d5<0.0) d5=-d5;
        d5=d5*3.0-3.0;
        if(d5<0.0) { d5/=2.0; if(d5 < -1.0) d5=-1.0; d5/=1.4; d5/=2.0; d4=0.0; }
        else { if(d5>1.0) d5=1.0; d5/=6.0; }
        d4 += 0.5;
        floorShape = d5 * (double)ys / 16.0;
        ++n2d;
        for(int yi=0; yi<ys; ++yi) {
            double v=0.0;
            double curve=yCurve[yi];
            double a=np->d2[out]/512.0;
            double b=np->d3[out]/512.0;
            double blend=(np->d1[out]/10.0+1.0)/2.0;
            if(blend<0.0) v=a; else if(blend>1.0) v=b; else v=a+(b-a)*blend;
            v -= curve;
            if(yi > ys-4) { double t=(double)((float)(yi-(ys-4))/3.0f); v=v*(1.0-t)+-10.0*t; }
            if((double)yi < floorShape) { double t=(floorShape-(double)yi)/4.0; if(t<0.0)t=0.0; if(t>1.0)t=1.0; v=v*(1.0-t)+-10.0*t; }
            np->density[out++]=v;
        }
    }
    return np->density;
}

static void nether_generate_terrain(NetherProvider *np, int cx, int cz, unsigned char *blocks) {
    int cell=4, sea=32, xs=5, ys=17, zs=5;
    double *den=nether_density(np,cx*cell,0,cz*cell,xs,ys,zs);
    for(int ix=0; ix<cell; ++ix) for(int iz=0; iz<cell; ++iz) for(int iy=0; iy<16; ++iy) {
        double yStep=0.125;
        double d00=den[((ix+0)*zs + iz+0)*ys + iy+0];
        double d01=den[((ix+0)*zs + iz+1)*ys + iy+0];
        double d10=den[((ix+1)*zs + iz+0)*ys + iy+0];
        double d11=den[((ix+1)*zs + iz+1)*ys + iy+0];
        double sd00=(den[((ix+0)*zs + iz+0)*ys + iy+1]-d00)*yStep;
        double sd01=(den[((ix+0)*zs + iz+1)*ys + iy+1]-d01)*yStep;
        double sd10=(den[((ix+1)*zs + iz+0)*ys + iy+1]-d10)*yStep;
        double sd11=(den[((ix+1)*zs + iz+1)*ys + iy+1]-d11)*yStep;
        for(int sy=0; sy<8; ++sy) {
            double xStep=0.25;
            double xa=d00, xb=d01;
            double xd0=(d10-d00)*xStep, xd1=(d11-d01)*xStep;
            for(int sx=0; sx<4; ++sx) {
                double zStep=0.25;
                double v=xa, dzv=(xb-xa)*zStep;
                for(int sz=0; sz<4; ++sz) {
                    int id=0;
                    int y=iy*8+sy;
                    if(y < sea) id=BLK_STILL_LAVA;
                    if(v > 0.0) id=BLK_NETHERRACK;
                    set_block_local(blocks,ix*4+sx,y,iz*4+sz,(unsigned char)id);
                    v += dzv;
                }
                xa += xd0; xb += xd1;
            }
            d00 += sd00; d01 += sd01; d10 += sd10; d11 += sd11;
        }
    }
}

static void nether_replace_surface(NetherProvider *np, int cx, int cz, unsigned char *blocks) {
    double sc=1.0/32.0;
    np->slow = octave_fill3(&np->slowsandGravel,np->slow,&np->slowCap,cx*16,0,cz*16,16,1,16,sc,1.0,sc);
    np->gravel = octave_fill3(&np->slowsandGravel,np->gravel,&np->gravelCap,cx*16,109,cz*16,16,1,16,sc,1.0,sc);
    np->exclusiveNoise = octave_fill3(&np->exclusive,np->exclusiveNoise,&np->exclusiveCap,cx*16,0,cz*16,16,1,16,sc*2.0,sc*2.0,sc*2.0);
    for(int lx=0; lx<16; ++lx) for(int lz=0; lz<16; ++lz) {
        int idx2=lx*16+lz;
        int slow = np->slow[idx2] + jr_next_double(&np->rand)*0.2 > 0.0;
        int gravel = np->gravel[idx2] + jr_next_double(&np->rand)*0.2 > 0.0;
        int depth=(int)(np->exclusiveNoise[idx2]/3.0 + 3.0 + jr_next_double(&np->rand)*0.25);
        int run=-1, top=BLK_NETHERRACK, filler=BLK_NETHERRACK;
        for(int y=127; y>=0; --y) {
            int id=get_block_local(blocks,lx,y,lz);
            if(y >= 127 - jr_next_int_bound(&np->rand,5)) { set_block_local(blocks,lx,y,lz,BLK_BEDROCK); continue; }
            if(y <= 0 + jr_next_int_bound(&np->rand,5)) { set_block_local(blocks,lx,y,lz,BLK_BEDROCK); continue; }
            if(id == BLK_AIR) run=-1;
            else if(id == BLK_NETHERRACK) {
                if(run == -1) {
                    if(depth <= 0) { top=BLK_AIR; filler=BLK_NETHERRACK; }
                    else if(y >= 60 && y <= 65) {
                        top=BLK_NETHERRACK; filler=BLK_NETHERRACK;
                        if(gravel) { top=BLK_GRAVEL; filler=BLK_NETHERRACK; }
                        if(slow) { top=BLK_SOUL_SAND; filler=BLK_SOUL_SAND; }
                    }
                    if(y < 64 && top == BLK_AIR) top=BLK_STILL_LAVA;
                    run=depth;
                    set_block_local(blocks,lx,y,lz,(unsigned char)(y>=63 ? top : filler));
                } else if(run > 0) { --run; set_block_local(blocks,lx,y,lz,(unsigned char)filler); }
            }
        }
    }
}

static int chunk_get_world_local(const unsigned char *blocks, int cx, int cz, int wx, int y, int wz, int outside) {
    int lx=wx-(cx<<4), lz=wz-(cz<<4);
    if(lx<0||lx>=16||lz<0||lz>=16||y<0||y>=128) return outside;
    return get_block_local(blocks,lx,y,lz);
}
static void chunk_set_world_local(unsigned char *blocks, int cx, int cz, int wx, int y, int wz, int id) {
    int lx=wx-(cx<<4), lz=wz-(cz<<4);
    set_block_local(blocks,lx,y,lz,(unsigned char)id);
}
static void nether_place_fire(unsigned char *blocks, int cx, int cz, JavaRandom *r, int x, int y, int z) {
    for(int i=0;i<64;i++) {
        int wx=x+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        int wy=y+jr_next_int_bound(r,4)-jr_next_int_bound(r,4);
        int wz=z+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        if(chunk_get_world_local(blocks,cx,cz,wx,wy,wz,BLK_AIR)==BLK_AIR && chunk_get_world_local(blocks,cx,cz,wx,wy-1,wz,BLK_NETHERRACK)==BLK_NETHERRACK) chunk_set_world_local(blocks,cx,cz,wx,wy,wz,BLK_FIRE);
    }
}
static void nether_place_glowstone(unsigned char *blocks, int cx, int cz, JavaRandom *r, int x, int y, int z) {
    if(chunk_get_world_local(blocks,cx,cz,x,y,z,BLK_AIR)!=BLK_AIR) return;
    if(chunk_get_world_local(blocks,cx,cz,x,y+1,z,BLK_NETHERRACK)!=BLK_NETHERRACK) return;
    chunk_set_world_local(blocks,cx,cz,x,y,z,BLK_GLOWSTONE);
    for(int i=0;i<1500;i++) {
        int wx=x+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        int wy=y-jr_next_int_bound(r,12);
        int wz=z+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        if(chunk_get_world_local(blocks,cx,cz,wx,wy,wz,BLK_AIR)!=BLK_AIR) continue;
        int adj=0;
        if(chunk_get_world_local(blocks,cx,cz,wx-1,wy,wz,0)==BLK_GLOWSTONE) adj++;
        if(chunk_get_world_local(blocks,cx,cz,wx+1,wy,wz,0)==BLK_GLOWSTONE) adj++;
        if(chunk_get_world_local(blocks,cx,cz,wx,wy-1,wz,0)==BLK_GLOWSTONE) adj++;
        if(chunk_get_world_local(blocks,cx,cz,wx,wy+1,wz,0)==BLK_GLOWSTONE) adj++;
        if(chunk_get_world_local(blocks,cx,cz,wx,wy,wz-1,0)==BLK_GLOWSTONE) adj++;
        if(chunk_get_world_local(blocks,cx,cz,wx,wy,wz+1,0)==BLK_GLOWSTONE) adj++;
        if(adj == 1) chunk_set_world_local(blocks,cx,cz,wx,wy,wz,BLK_GLOWSTONE);
    }
}
static void nether_place_lava(unsigned char *blocks, int cx, int cz, JavaRandom *r, int x, int y, int z) {
    (void)r;
    if(chunk_get_world_local(blocks,cx,cz,x,y+1,z,BLK_NETHERRACK)!=BLK_NETHERRACK) return;
    int here=chunk_get_world_local(blocks,cx,cz,x,y,z,BLK_AIR);
    if(here!=BLK_AIR && here!=BLK_NETHERRACK) return;
    int rock=0, air=0;
    int dirs[5][3]={{-1,0,0},{1,0,0},{0,0,-1},{0,0,1},{0,-1,0}};
    for(int i=0;i<5;i++) {
        int id=chunk_get_world_local(blocks,cx,cz,x+dirs[i][0],y+dirs[i][1],z+dirs[i][2],BLK_NETHERRACK);
        if(id==BLK_NETHERRACK) rock++;
        if(id==BLK_AIR) air++;
    }
    if(rock==4 && air==1) chunk_set_world_local(blocks,cx,cz,x,y,z,BLK_LAVA);
}
static void nether_place_flower(unsigned char *blocks, int cx, int cz, JavaRandom *r, int id, int x, int y, int z) {
    for(int i=0;i<64;i++) {
        int wx=x+jr_next_int_bound(r,8)-jr_next_int_bound(r,8), wy=y+jr_next_int_bound(r,4)-jr_next_int_bound(r,4), wz=z+jr_next_int_bound(r,8)-jr_next_int_bound(r,8);
        if(chunk_get_world_local(blocks,cx,cz,wx,wy,wz,BLK_AIR)==BLK_AIR && chunk_get_world_local(blocks,cx,cz,wx,wy-1,wz,BLK_NETHERRACK)==BLK_NETHERRACK) chunk_set_world_local(blocks,cx,cz,wx,wy,wz,id);
    }
}

static void nether_place_bridge_debug(unsigned char *blocks, long long seed, int cx, int cz) {
    int gridX = cx >= 0 ? cx / 16 : (cx - 15) / 16;
    int gridZ = cz >= 0 ? cz / 16 : (cz - 15) / 16;
    JavaRandom fr = worldgen_set_random_seed(seed, gridX, gridZ, 30084232);
    int fcX = gridX * 16 + jr_next_int_bound(&fr, 8) + 4;
    int fcZ = gridZ * 16 + jr_next_int_bound(&fr, 8) + 4;
    if (abs(cx - fcX) > 1 || abs(cz - fcZ) > 1) return;
    int ox=(fcX<<4)+8, oz=(fcZ<<4)+8, y=64;
    for(int x=ox-28; x<=ox+28; ++x) for(int z=oz-3; z<=oz+3; ++z) {
        int lx=x-(cx<<4), lz=z-(cz<<4); if(lx<0||lx>=16||lz<0||lz>=16) continue;
        for(int yy=y; yy<=y+6; ++yy) set_block_local(blocks,lx,yy,lz,(yy==y||yy==y+5||z==oz-3||z==oz+3)?BLK_NETHER_BRICK:BLK_AIR);
        if((x-ox)%6==0) { set_block_local(blocks,lx,y+1,lz,BLK_NETHER_BRICK_FENCE); set_block_local(blocks,lx,y+2,lz,BLK_NETHER_BRICK_FENCE); }
    }
    for(int z=oz-28; z<=oz+28; ++z) for(int x=ox-3; x<=ox+3; ++x) {
        int lx=x-(cx<<4), lz=z-(cz<<4); if(lx<0||lx>=16||lz<0||lz>=16) continue;
        for(int yy=y; yy<=y+6; ++yy) set_block_local(blocks,lx,yy,lz,(yy==y||yy==y+5||x==ox-3||x==ox+3)?BLK_NETHER_BRICK:BLK_AIR);
        if((z-oz)%6==0) { set_block_local(blocks,lx,y+1,lz,BLK_NETHER_BRICK_FENCE); set_block_local(blocks,lx,y+2,lz,BLK_NETHER_BRICK_FENCE); }
    }
    chunk_set_world_local(blocks,cx,cz,ox+5,y+1,oz+5,BLK_CHEST);
    chunk_set_world_local(blocks,cx,cz,ox-5,y+1,oz-5,BLK_MOB_SPAWNER);
    chunk_set_world_local(blocks,cx,cz,ox-4,y+1,oz-5,BLK_NETHER_WART);
    chunk_set_world_local(blocks,cx,cz,ox-3,y+1,oz-5,BLK_NETHER_WART);
}

static void worldgen_generate_nether_chunk(long long seed, int cx, int cz, unsigned char *blocks, unsigned char *data, unsigned char *heightmap, unsigned char *biome_array) {
    memset(blocks, 0, 32768); memset(data, 0, 16384); memset(heightmap, 0, 256);
    if (biome_array) memset(biome_array, BIOME_HELL, 256);
    NetherProvider np; nether_provider_init(&np,(int64_t)seed);
    jr_set_seed(&np.rand, (int64_t)((uint64_t)(int64_t)cx * UINT64_C(341873128712) + (uint64_t)(int64_t)cz * UINT64_C(132897987541)));
    nether_generate_terrain(&np,cx,cz,blocks);
    nether_replace_surface(&np,cx,cz,blocks);
    nether_place_bridge_debug(blocks,seed,cx,cz);

    /* Chunk-local version of ChunkProviderHell.populate feature order. */
    int baseX=cx*16, baseZ=cz*16;
    for(int i=0;i<8;i++) nether_place_lava(blocks,cx,cz,&np.rand,baseX+jr_next_int_bound(&np.rand,16)+8,jr_next_int_bound(&np.rand,120)+4,baseZ+jr_next_int_bound(&np.rand,16)+8);
    int fireCount=jr_next_int_bound(&np.rand,jr_next_int_bound(&np.rand,10)+1)+1;
    for(int i=0;i<fireCount;i++) nether_place_fire(blocks,cx,cz,&np.rand,baseX+jr_next_int_bound(&np.rand,16)+8,jr_next_int_bound(&np.rand,120)+4,baseZ+jr_next_int_bound(&np.rand,16)+8);
    int glow1=jr_next_int_bound(&np.rand,jr_next_int_bound(&np.rand,10)+1);
    for(int i=0;i<glow1;i++) nether_place_glowstone(blocks,cx,cz,&np.rand,baseX+jr_next_int_bound(&np.rand,16)+8,jr_next_int_bound(&np.rand,120)+4,baseZ+jr_next_int_bound(&np.rand,16)+8);
    for(int i=0;i<10;i++) nether_place_glowstone(blocks,cx,cz,&np.rand,baseX+jr_next_int_bound(&np.rand,16)+8,jr_next_int_bound(&np.rand,128),baseZ+jr_next_int_bound(&np.rand,16)+8);
    if(jr_next_int_bound(&np.rand,1)==0) nether_place_flower(blocks,cx,cz,&np.rand,BLK_BROWN_MUSHROOM,baseX+jr_next_int_bound(&np.rand,16)+8,jr_next_int_bound(&np.rand,128),baseZ+jr_next_int_bound(&np.rand,16)+8);
    if(jr_next_int_bound(&np.rand,1)==0) nether_place_flower(blocks,cx,cz,&np.rand,BLK_RED_MUSHROOM,baseX+jr_next_int_bound(&np.rand,16)+8,jr_next_int_bound(&np.rand,128),baseZ+jr_next_int_bound(&np.rand,16)+8);

    unsigned char *sky=(unsigned char*)calloc(16384,1);
    if(sky) { recalc_light_and_height(blocks, sky, heightmap); free(sky); }
    nether_provider_free(&np);
}


typedef struct EndProvider {
    JavaRandom rand;
    OctaveNoise n1,n2,n3,n4,n5;
    double *density,*d1,*d2,*d3,*d4,*d5;
    int densityCap,d1Cap,d2Cap,d3Cap,d4Cap,d5Cap;
} EndProvider;
static void end_provider_init(EndProvider *ep, int64_t seed) {
    memset(ep,0,sizeof(*ep));
    jr_set_seed(&ep->rand, seed);
    octave_init(&ep->n1,&ep->rand,16);
    octave_init(&ep->n2,&ep->rand,16);
    octave_init(&ep->n3,&ep->rand,8);
    octave_init(&ep->n4,&ep->rand,10);
    octave_init(&ep->n5,&ep->rand,16);
}
static void end_provider_free(EndProvider *ep) { free(ep->density); free(ep->d1); free(ep->d2); free(ep->d3); free(ep->d4); free(ep->d5); }
static double *end_density(EndProvider *ep, int x, int y, int z, int xs, int ys, int zs) {
    int n=xs*ys*zs;
    if(!ep->density || ep->densityCap<n) { free(ep->density); ep->density=(double*)malloc(sizeof(double)*(size_t)n); ep->densityCap=n; }
    if(!ep->density) ExitProcess(2);
    double sx=684.412*2.0, sy=684.412;
    ep->d4 = octave_fill3(&ep->n4,ep->d4,&ep->d4Cap,x,0,z,xs,1,zs,1.121,1.0,1.121);
    ep->d5 = octave_fill3(&ep->n5,ep->d5,&ep->d5Cap,x,0,z,xs,1,zs,200.0,1.0,200.0);
    ep->d1 = octave_fill3(&ep->n3,ep->d1,&ep->d1Cap,x,y,z,xs,ys,zs,sx/80.0,sy/160.0,sx/80.0);
    ep->d2 = octave_fill3(&ep->n1,ep->d2,&ep->d2Cap,x,y,z,xs,ys,zs,sx,sy,sx);
    ep->d3 = octave_fill3(&ep->n2,ep->d3,&ep->d3Cap,x,y,z,xs,ys,zs,sx,sy,sx);
    int out=0, n2d=0;
    for(int xi=0; xi<xs; ++xi) for(int zi=0; zi<zs; ++zi) {
        double d4=(ep->d4[n2d]+256.0)/512.0; if(d4>1.0)d4=1.0;
        double d5=ep->d5[n2d]/8000.0;
        if(d5<0.0)d5=-d5*0.3;
        d5=d5*3.0-2.0;
        float fx=(float)(xi+x), fz=(float)(zi+z);
        float island=100.0f - sqrtf(fx*fx+fz*fz)*8.0f;
        if(island>80.0f)island=80.0f;
        if(island<-100.0f)island=-100.0f;
        if(d5>1.0)d5=1.0;
        d5/=8.0;
        d5=0.0;
        if(d4<0.0)d4=0.0;
        d4+=0.5;
        d5=d5*(double)ys/16.0;
        ++n2d;
        double mid=(double)ys/2.0;
        for(int yi=0; yi<ys; ++yi) {
            double v=0.0;
            double yy=((double)yi-mid)*8.0/d4;
            if(yy<0.0)yy=-yy;
            double a=ep->d2[out]/512.0;
            double b=ep->d3[out]/512.0;
            double blend=(ep->d1[out]/10.0+1.0)/2.0;
            if(blend<0.0)v=a; else if(blend>1.0)v=b; else v=a+(b-a)*blend;
            v -= 8.0;
            v += (double)island;
            int topClamp=2;
            if(yi > ys/2 - topClamp) { double t=(double)((float)(yi-(ys/2-topClamp))/64.0f); if(t<0.0)t=0.0; if(t>1.0)t=1.0; v=v*(1.0-t)+-3000.0*t; }
            int bottomClamp=8;
            if(yi < bottomClamp) { double t=(double)((float)(bottomClamp-yi)/((float)bottomClamp-1.0f)); v=v*(1.0-t)+-30.0*t; }
            ep->density[out++]=v;
        }
    }
    return ep->density;
}
static void end_generate_terrain(EndProvider *ep, int cx, int cz, unsigned char *blocks) {
    int cell=2, xs=3, ys=33, zs=3;
    double *den=end_density(ep,cx*cell,0,cz*cell,xs,ys,zs);
    for(int ix=0; ix<cell; ++ix) for(int iz=0; iz<cell; ++iz) for(int iy=0; iy<32; ++iy) {
        double yStep=0.25;
        double d00=den[((ix+0)*zs + iz+0)*ys + iy+0];
        double d01=den[((ix+0)*zs + iz+1)*ys + iy+0];
        double d10=den[((ix+1)*zs + iz+0)*ys + iy+0];
        double d11=den[((ix+1)*zs + iz+1)*ys + iy+0];
        double sd00=(den[((ix+0)*zs + iz+0)*ys + iy+1]-d00)*yStep;
        double sd01=(den[((ix+0)*zs + iz+1)*ys + iy+1]-d01)*yStep;
        double sd10=(den[((ix+1)*zs + iz+0)*ys + iy+1]-d10)*yStep;
        double sd11=(den[((ix+1)*zs + iz+1)*ys + iy+1]-d11)*yStep;
        for(int syi=0; syi<4; ++syi) {
            double xStep=0.125;
            double xa=d00, xb=d01;
            double xd0=(d10-d00)*xStep, xd1=(d11-d01)*xStep;
            for(int sx8=0; sx8<8; ++sx8) {
                double zStep=0.125;
                double v=xa, dzv=(xb-xa)*zStep;
                for(int sz8=0; sz8<8; ++sz8) {
                    int id = v > 0.0 ? BLK_END_STONE : BLK_AIR;
                    set_block_local(blocks,ix*8+sx8,iy*4+syi,iz*8+sz8,(unsigned char)id);
                    v += dzv;
                }
                xa += xd0; xb += xd1;
            }
            d00 += sd00; d01 += sd01; d10 += sd10; d11 += sd11;
        }
    }
}
static int end_top_solid(const unsigned char *blocks, int lx, int lz) {
    for(int y=127; y>=0; --y) if(get_block_local(blocks,lx,y,lz)!=BLK_AIR) return y+1;
    return 0;
}
static void end_place_spike(unsigned char *blocks, int cx, int cz, JavaRandom *r, int wx, int y, int wz) {
    int lx=wx-(cx<<4), lz=wz-(cz<<4);
    if(lx<0||lx>=16||lz<0||lz>=16||y<=0||y>=128) return;
    if(get_block_local(blocks,lx,y,lz)!=BLK_AIR || get_block_local(blocks,lx,y-1,lz)!=BLK_END_STONE) return;
    int h=jr_next_int_bound(r,32)+6;
    int rad=jr_next_int_bound(r,4)+1;
    for(int x=wx-rad; x<=wx+rad; ++x) for(int z=wz-rad; z<=wz+rad; ++z) {
        int dx=x-wx, dz=z-wz;
        int llx=x-(cx<<4), llz=z-(cz<<4);
        if(llx<0||llx>=16||llz<0||llz>=16) continue;
        if(dx*dx+dz*dz <= rad*rad+1 && get_block_local(blocks,llx,y-1,llz)!=BLK_END_STONE) return;
    }
    for(int yy=y; yy<y+h && yy<128; ++yy) for(int x=wx-rad; x<=wx+rad; ++x) for(int z=wz-rad; z<=wz+rad; ++z) {
        int dx=x-wx, dz=z-wz; if(dx*dx+dz*dz > rad*rad+1) continue;
        int llx=x-(cx<<4), llz=z-(cz<<4); if(llx>=0&&llx<16&&llz>=0&&llz<16) set_block_local(blocks,llx,yy,llz,BLK_OBSIDIAN);
    }
    chunk_set_world_local(blocks,cx,cz,wx,y+h,wz,BLK_BEDROCK);
}

static void worldgen_generate_end_chunk(long long seed, int cx, int cz, unsigned char *blocks, unsigned char *data, unsigned char *heightmap, unsigned char *biome_array) {
    memset(blocks, 0, 32768); memset(data, 0, 16384); memset(heightmap, 0, 256);
    if (biome_array) memset(biome_array, BIOME_SKY, 256);
    EndProvider ep; end_provider_init(&ep,(int64_t)seed);
    jr_set_seed(&ep.rand, (int64_t)((uint64_t)(int64_t)cx * UINT64_C(341873128712) + (uint64_t)(int64_t)cz * UINT64_C(132897987541)));
    end_generate_terrain(&ep,cx,cz,blocks);

    /* BiomeEndDecorator-style spike chance; dragon/crystal entities are not
       spawned in PexCraft, but the obsidian/bedrock block side effects are. */
    int baseX=cx*16, baseZ=cz*16;
    if(jr_next_int_bound(&ep.rand,5)==0) {
        int wx=baseX+jr_next_int_bound(&ep.rand,16)+8;
        int wz=baseZ+jr_next_int_bound(&ep.rand,16)+8;
        int lx=wx-(cx<<4), lz=wz-(cz<<4);
        int y=(lx>=0&&lx<16&&lz>=0&&lz<16) ? end_top_solid(blocks,lx,lz) : 0;
        if(y>0) end_place_spike(blocks,cx,cz,&ep.rand,wx,y,wz);
    }
    if (cx == 0 && cz == 0) {
        for (int lx=6; lx<=10; ++lx) for (int lz=6; lz<=10; ++lz) set_block_local(blocks,lx,64,lz,BLK_OBSIDIAN);
    }
    unsigned char *sky = (unsigned char*)calloc(16384,1);
    if (sky) { recalc_light_and_height(blocks, sky, heightmap); free(sky); }
    end_provider_free(&ep);
}


int worldgen_generate_dimension_chunk_arrays(long long seed, int dimension, int cx, int cz, unsigned char *blocks, unsigned char *data, unsigned char *heightmap, unsigned char *biome_array) {
    if (!blocks || !data || !heightmap || !biome_array) return 0;
    if (dimension == 0) return worldgen_generate_chunk_arrays(seed, cx, cz, blocks, data, heightmap, biome_array);
    if (dimension == -1) { worldgen_generate_nether_chunk(seed, cx, cz, blocks, data, heightmap, biome_array); return 1; }
    if (dimension == 1) { worldgen_generate_end_chunk(seed, cx, cz, blocks, data, heightmap, biome_array); return 1; }
    return 0;
}

typedef struct WorldgenTraceTarget {
    int found;
    int kind;
    int chunkX, chunkZ;
    int blockX, footY, blockZ;
    int surface; /* 1 = choose a safe surface after generation; 0 = underground target */
    char label[64];
} WorldgenTraceTarget;

static int worldgen_dist2_chunks(int ax, int az, int bx, int bz) {
    int dx=ax-bx, dz=az-bz;
    return dx*dx + dz*dz;
}

int worldgen_trace_target(long long seed, const char *kind, int fromBlockX, int fromBlockZ, int index, WorldgenTraceTarget *out) {
    if (!kind || !out) return 0;
    memset(out, 0, sizeof(*out));
    int fromCx = wg_floor_div16(fromBlockX), fromCz = wg_floor_div16(fromBlockZ);
    if (index < 0) index = 0;
    if (strcmp(kind, "stronghold") == 0 || strcmp(kind, "portalroom") == 0) {
        int sx[3], sz[3]; worldgen_get_stronghold_coords(seed, sx, sz);
        int pick = 0, best = 0x7fffffff;
        if (index >= 1 && index <= 3) pick = index - 1;
        else for (int i=0;i<3;i++) { int d=worldgen_dist2_chunks(fromCx,fromCz,sx[i],sz[i]); if (d < best) { best=d; pick=i; } }
        JavaRandom r = worldgen_structure_random(seed, sx[pick], sz[pick]);
        int baseX = (sx[pick] << 4) + 2, baseZ = (sz[pick] << 4) + 2;
        int y = 26 + jr_next_int_bound(&r, 16);
        out->found=1; out->kind=1; out->chunkX=sx[pick]; out->chunkZ=sz[pick]; out->blockX=baseX+2; out->blockZ=baseZ+29; out->footY=y+2; out->surface=0;
        snprintf(out->label,sizeof(out->label),"stronghold #%d",pick+1);
        return 1;
    }
    if (strcmp(kind, "village") == 0) {
        int bestCx=0,bestCz=0,best=0x7fffffff, found=0;
        for (int r=0; r<=128 && !found; ++r) {
            for (int dz=-r; dz<=r; ++dz) for (int dx=-r; dx<=r; ++dx) {
                if (abs(dx) != r && abs(dz) != r) continue;
                int cx=fromCx+dx, cz=fromCz+dz;
                if (worldgen_village_can_spawn(seed,cx,cz)) { int d=dx*dx+dz*dz; if (d < best) { best=d; bestCx=cx; bestCz=cz; found=1; } }
            }
        }
        if (!found) return 0;
        out->found=1; out->kind=2; out->chunkX=bestCx; out->chunkZ=bestCz; out->blockX=(bestCx<<4)+8; out->blockZ=(bestCz<<4)+8; out->footY=80; out->surface=1;
        snprintf(out->label,sizeof(out->label),"village");
        return 1;
    }
    if (strcmp(kind, "mineshaft") == 0 || strcmp(kind, "mine") == 0) {
        int bestCx=0,bestCz=0,best=0x7fffffff, found=0;
        for (int r=0; r<=192 && !found; ++r) {
            for (int dz=-r; dz<=r; ++dz) for (int dx=-r; dx<=r; ++dx) {
                if (abs(dx) != r && abs(dz) != r) continue;
                int cx=fromCx+dx, cz=fromCz+dz;
                if (worldgen_mineshaft_can_spawn(seed,cx,cz)) { int d=dx*dx+dz*dz; if (d < best) { best=d; bestCx=cx; bestCz=cz; found=1; } }
            }
        }
        if (!found) return 0;
        JavaRandom r = worldgen_structure_random(seed, bestCx, bestCz);
        int y = 18 + jr_next_int_bound(&r, 28);
        out->found=1; out->kind=3; out->chunkX=bestCx; out->chunkZ=bestCz; out->blockX=(bestCx<<4)+8; out->blockZ=(bestCz<<4)+8; out->footY=y+2; out->surface=0;
        snprintf(out->label,sizeof(out->label),"mineshaft");
        return 1;
    }
    return 0;
}


#ifndef PEXCRAFT_WORLDGEN_ARRAYS_ONLY
static void nbt_byte_value(BinBuf *b, const char *name, int v) { nbt_tag(b, 1, name); bb_u8(b, (unsigned int)(v & 0xff)); }
static void nbt_short_value(BinBuf *b, const char *name, int v) { nbt_tag(b, 2, name); bb_u16be(b, (unsigned int)(v & 0xffff)); }

typedef struct WorldgenLootItem { int slot, id, count, damage; } WorldgenLootItem;

static unsigned int worldgen_pos_hash(int wx, int y, int wz, unsigned int salt) {
    unsigned int h = (unsigned int)wx * 73428767u ^ (unsigned int)wz * 91227153u ^ (unsigned int)y * 42317861u ^ salt;
    h ^= h >> 16; h *= 2246822519u; h ^= h >> 13; h *= 3266489917u; h ^= h >> 16;
    return h;
}

static int worldgen_neighbor_id_local(const unsigned char *blocks, int x, int y, int z, int id, int radius) {
    for(int dy=-radius; dy<=radius; ++dy) for(int dz=-radius; dz<=radius; ++dz) for(int dx=-radius; dx<=radius; ++dx) {
        int xx=x+dx, yy=y+dy, zz=z+dz;
        if(xx<0||xx>=16||yy<0||yy>=128||zz<0||zz>=16) continue;
        if(blocks[chunk_index(xx,yy,zz)] == id) return 1;
    }
    return 0;
}

static const char *worldgen_spawner_entity(const unsigned char *blocks, int x, int y, int z) {
    if(worldgen_neighbor_id_local(blocks,x,y,z,BLK_END_PORTAL_FRAME,6) || worldgen_neighbor_id_local(blocks,x,y,z,BLK_STONE_BRICK,6)) return "Silverfish";
    if(worldgen_neighbor_id_local(blocks,x,y,z,BLK_NETHER_BRICK,8)) return "Blaze";
    if(worldgen_neighbor_id_local(blocks,x,y,z,BLK_WEB,8) || worldgen_neighbor_id_local(blocks,x,y,z,BLK_RAILS,8)) return "CaveSpider";
    return "Silverfish";
}

static int worldgen_chest_kind(const unsigned char *blocks, int x, int y, int z) {
    if(worldgen_neighbor_id_local(blocks,x,y,z,BLK_BOOKSHELF,8) || worldgen_neighbor_id_local(blocks,x,y,z,BLK_STONE_BRICK,8)) return 1; /* stronghold */
    if(worldgen_neighbor_id_local(blocks,x,y,z,BLK_RAILS,8) || worldgen_neighbor_id_local(blocks,x,y,z,BLK_WEB,8)) return 2; /* mineshaft */
    if(worldgen_neighbor_id_local(blocks,x,y,z,BLK_NETHER_BRICK,8)) return 3; /* fortress */
    return 0; /* village/dungeon fallback */
}

static int worldgen_make_loot(int kind, int wx, int y, int wz, WorldgenLootItem out[16]) {
    unsigned int h = worldgen_pos_hash(wx,y,wz,0x125125u);
    int n = 0;
#define ADD_LOOT(slot_, id_, count_, damage_) do { if(n < 16) { out[n].slot=(slot_); out[n].id=(id_); out[n].count=(count_); out[n].damage=(damage_); ++n; } } while(0)
    if(kind == 1) { /* stronghold library/corridor style */
        ADD_LOOT((h>>0)&26, 340, 1 + (int)((h>>4)&3), 0);      /* book */
        ADD_LOOT((h>>5)&26, 339, 2 + (int)((h>>9)&5), 0);      /* paper */
        if(h & 0x20u) ADD_LOOT((h>>10)&26, 345, 1, 0);         /* compass */
        if(h & 0x80u) ADD_LOOT((h>>12)&26, 358, 1, 0);         /* map */
        if(h & 0x200u) ADD_LOOT((h>>14)&26, 368, 1 + (int)((h>>16)&1), 0); /* ender pearl */
    } else if(kind == 2) { /* abandoned mineshaft */
        ADD_LOOT((h>>0)&26, 66, 4 + (int)((h>>4)&15), 0);      /* rails */
        ADD_LOOT((h>>5)&26, 263, 2 + (int)((h>>9)&7), 0);      /* coal */
        ADD_LOOT((h>>10)&26, 296, 1 + (int)((h>>14)&3), 0);    /* wheat */
        if(h & 0x400u) ADD_LOOT((h>>15)&26, 257, 1, 0);        /* iron pickaxe */
        if(h & 0x800u) ADD_LOOT((h>>16)&26, 265, 1 + (int)((h>>20)&3), 0); /* iron ingot */
        if(h & 0x1000u) ADD_LOOT((h>>21)&26, 362, 2 + (int)((h>>25)&3), 0); /* melon seeds */
        if(h & 0x2000u) ADD_LOOT((h>>22)&26, 361, 2 + (int)((h>>26)&3), 0); /* pumpkin seeds */
    } else if(kind == 3) { /* nether fortress */
        ADD_LOOT((h>>0)&26, 371, 1 + (int)((h>>4)&3), 0);      /* gold nugget */
        ADD_LOOT((h>>5)&26, 369, 1, 0);                        /* blaze rod */
        if(h & 0x100u) ADD_LOOT((h>>12)&26, 265, 1 + (int)((h>>16)&3), 0);
        if(h & 0x400u) ADD_LOOT((h>>17)&26, 329, 1, 0);        /* saddle */
    } else { /* village/blacksmith fallback */
        ADD_LOOT((h>>0)&26, 297, 1 + (int)((h>>4)&2), 0);      /* bread */
        ADD_LOOT((h>>5)&26, 260, 1 + (int)((h>>9)&2), 0);      /* apple */
        if(h & 0x40u) ADD_LOOT((h>>10)&26, 265, 1 + (int)((h>>14)&4), 0);
        if(h & 0x80u) ADD_LOOT((h>>15)&26, 266, 1 + (int)((h>>18)&2), 0);
        if(h & 0x200u) ADD_LOOT((h>>19)&26, 264, 1, 0);
        if(h & 0x400u) ADD_LOOT((h>>20)&26, 325, 1, 0);
        if(h & 0x800u) ADD_LOOT((h>>21)&26, 267, 1, 0);
    }
#undef ADD_LOOT
    return n;
}

static void nbt_item_stack(BinBuf *b, const WorldgenLootItem *it) {
    nbt_byte_value(b, "Slot", it->slot);
    nbt_short_value(b, "id", it->id);
    nbt_byte_value(b, "Count", it->count);
    nbt_short_value(b, "Damage", it->damage);
    nbt_end(b);
}

static void nbt_chest_items(BinBuf *b, const unsigned char *blocks, int cx, int cz, int x, int y, int z) {
    WorldgenLootItem items[16];
    int wx = (cx << 4) + x, wz = (cz << 4) + z;
    int count = worldgen_make_loot(worldgen_chest_kind(blocks,x,y,z), wx, y, wz, items);
    nbt_tag(b, 9, "Items");
    bb_u8(b, 10);
    bb_u32be(b, (unsigned int)count);
    for(int i=0;i<count;i++) nbt_item_stack(b, &items[i]);
}

static int worldgen_tile_entity_count(const unsigned char *blocks) {
    int count = 0;
    for (int x=0;x<16;x++) for (int z=0;z<16;z++) for (int y=0;y<128;y++) {
        int id = blocks[chunk_index(x,y,z)];
        if (id == BLK_CHEST || id == BLK_MOB_SPAWNER) count++;
    }
    return count;
}
static void worldgen_write_tile_entities(BinBuf *b, const unsigned char *blocks, int cx, int cz) {
    int count = worldgen_tile_entity_count(blocks);
    nbt_tag(b, 9, "TileEntities");
    bb_u8(b, 10);
    bb_u32be(b, (unsigned int)count);
    for (int x=0;x<16;x++) for (int z=0;z<16;z++) for (int y=0;y<128;y++) {
        int id = blocks[chunk_index(x,y,z)];
        if (id != BLK_CHEST && id != BLK_MOB_SPAWNER) continue;
        if (id == BLK_CHEST) {
            nbt_string(b, "id", "Chest");
            nbt_int(b, "x", (cx << 4) + x); nbt_int(b, "y", y); nbt_int(b, "z", (cz << 4) + z);
            nbt_chest_items(b, blocks, cx, cz, x, y, z);
            nbt_end(b);
        } else {
            nbt_string(b, "id", "MobSpawner");
            nbt_int(b, "x", (cx << 4) + x); nbt_int(b, "y", y); nbt_int(b, "z", (cz << 4) + z);
            nbt_string(b, "EntityId", worldgen_spawner_entity(blocks,x,y,z));
            nbt_short_value(b, "Delay", 20);
            nbt_end(b);
        }
    }
}


static int write_chunk_file(const char *world_dir, int cx, int cz, long long seed) {
    unsigned char *blocks = (unsigned char*)calloc(32768, 1);
    unsigned char *data = (unsigned char*)calloc(16384, 1);
    unsigned char *sky = (unsigned char*)calloc(16384, 1);
    unsigned char *blocklight = (unsigned char*)calloc(16384, 1);
    unsigned char *heightmap = (unsigned char*)calloc(256, 1);
    unsigned char *biome_array = (unsigned char*)calloc(256, 1);
    if (!blocks || !data || !sky || !blocklight || !heightmap || !biome_array) { free(blocks); free(data); free(sky); free(blocklight); free(heightmap); free(biome_array); return 0; }

    if (!worldgen_generate_chunk_arrays(seed, cx, cz, blocks, data, heightmap, biome_array)) {
        free(blocks); free(data); free(sky); free(blocklight); free(heightmap); free(biome_array);
        return 0;
    }
    recalc_light_and_height(blocks, sky, heightmap);

    BinBuf b = {0};
    bb_u8(&b, 10); nbt_name(&b, "");
    nbt_compound(&b, "Level");
    nbt_int(&b, "xPos", cx);
    nbt_int(&b, "zPos", cz);
    nbt_long(&b, "LastUpdate", 0);
    nbt_byte_array(&b, "Blocks", blocks, 32768);
    nbt_byte_array(&b, "Data", data, 16384);
    nbt_byte_array(&b, "SkyLight", sky, 16384);
    nbt_byte_array(&b, "BlockLight", blocklight, 16384);
    nbt_byte_array(&b, "HeightMap", heightmap, 256);
    nbt_byte_array(&b, "Biomes", biome_array, 256);
    nbt_byte(&b, "TerrainPopulated", 1);
    nbt_empty_compound_list(&b, "Entities");
    worldgen_write_tile_entities(&b, blocks, cx, cz);
    nbt_end(&b);
    nbt_end(&b);
    char bx[32], bz[32], fx[32], fz[32];
    base36_i32(cx & 63, bx, sizeof(bx));
    base36_i32(cz & 63, bz, sizeof(bz));
    base36_i32(cx, fx, sizeof(fx));
    base36_i32(cz, fz, sizeof(fz));
    char dir1[MAX_PATHBUF], dir2[MAX_PATHBUF], path[MAX_PATHBUF];
    snprintf(dir1, sizeof(dir1), "%s\\%s", world_dir, bx);
    snprintf(dir2, sizeof(dir2), "%s\\%s", dir1, bz);
    make_dir_recursive(dir2);
    snprintf(path, sizeof(path), "%s\\c.%s.%s.dat", dir2, fx, fz);
    int ok = write_gzip_store(path, b.data, b.len);
    bb_free(&b);
    free(blocks); free(data); free(sky); free(blocklight); free(heightmap); free(biome_array);
    return ok;
}
#endif

#ifndef PEXCRAFT_WORLDGEN_ARRAYS_ONLY
static unsigned long long world_dir_size_quick(const char *path) { return dir_size(path); }

#endif
