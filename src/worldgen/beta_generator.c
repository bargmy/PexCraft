/* Split from original monolithic main.c. Included by src/main.c unity build. */

#define BLK_AIR       0
#define BLK_STONE     1
#define BLK_GRASS     2
#define BLK_DIRT      3
#define BLK_BEDROCK   7
#define BLK_WATER     9
#define BLK_LAVA      10
#define BLK_STILL_LAVA 11
#define BLK_SAND      12
#define BLK_GRAVEL    13
#define BLK_GOLD_ORE  14
#define BLK_IRON_ORE  15
#define BLK_COAL_ORE  16
#define BLK_LOG       17
#define BLK_LEAVES    18
#define BLK_DIAMOND_ORE 56
#define BLK_REDSTONE_ORE 73
#define BLK_ICE       79
#define BLK_CLAY      82
#define BLK_SNOW_LAYER 78
#define BLK_YELLOW_FLOWER 37
#define BLK_RED_ROSE 38
#define BLK_BROWN_MUSHROOM 39
#define BLK_RED_MUSHROOM 40
#define BLK_REEDS     83
#define BLK_PUMPKIN   86

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

typedef enum BetaBiomeId { BIOME_RAINFOREST, BIOME_SWAMP, BIOME_SEASONAL_FOREST, BIOME_FOREST, BIOME_SAVANNA, BIOME_SHRUBLAND, BIOME_TAIGA, BIOME_DESERT, BIOME_PLAINS, BIOME_ICE_DESERT, BIOME_TUNDRA, BIOME_HELL } BetaBiomeId;
typedef struct BetaBiome { BetaBiomeId id; unsigned char top; unsigned char filler; } BetaBiome;
static BetaBiome beta_biomes[12];
static int beta_biomes_ready=0;
static void beta_init_biomes(void) {
    if(beta_biomes_ready) return;
    for(int i=0;i<12;i++){ beta_biomes[i].id=(BetaBiomeId)i; beta_biomes[i].top=BLK_GRASS; beta_biomes[i].filler=BLK_DIRT; }
    beta_biomes[BIOME_DESERT].top=beta_biomes[BIOME_DESERT].filler=BLK_SAND;
    beta_biomes[BIOME_ICE_DESERT].top=beta_biomes[BIOME_ICE_DESERT].filler=BLK_SAND;
    beta_biomes[BIOME_HELL].top=beta_biomes[BIOME_HELL].filler=BLK_DIRT;
    beta_biomes_ready=1;
}
static BetaBiomeId beta_biome_lookup(float temp, float humid) {
    humid *= temp;
    if (temp < 0.1f) return BIOME_TUNDRA;
    if (humid < 0.2f) return temp < 0.5f ? BIOME_TUNDRA : (temp < 0.95f ? BIOME_SAVANNA : BIOME_DESERT);
    if (humid > 0.5f && temp < 0.7f) return BIOME_SWAMP;
    if (temp < 0.5f) return BIOME_TAIGA;
    if (temp < 0.97f) return humid < 0.35f ? BIOME_SHRUBLAND : BIOME_FOREST;
    return humid < 0.45f ? BIOME_PLAINS : (humid < 0.9f ? BIOME_SEASONAL_FOREST : BIOME_RAINFOREST);
}

typedef struct BiomeManager {
    SimplexOctaves tempNoise, humidNoise, weirdNoise;
    double *temp, *humid, *weird;
    int tempCap, humidCap, weirdCap;
    BetaBiome biomes[256];
} BiomeManager;
static void biome_manager_init(BiomeManager *bm, int64_t worldSeed) {
    beta_init_biomes();
    memset(bm, 0, sizeof(*bm));
    simplex_octaves_init(&bm->tempNoise, worldSeed * INT64_C(9871), 4);
    simplex_octaves_init(&bm->humidNoise, worldSeed * INT64_C(39811), 4);
    simplex_octaves_init(&bm->weirdNoise, worldSeed * INT64_C(543321), 2);
}
static BetaBiome *biome_manager_get(BiomeManager *bm, int x, int z, int xs, int zs) {
    int n=xs*zs;
    bm->temp = simplex_octaves_fill(&bm->tempNoise, bm->temp, &bm->tempCap, x,z,xs,zs,0.025,0.025,0.25,0.5);
    bm->humid = simplex_octaves_fill(&bm->humidNoise, bm->humid, &bm->humidCap, x,z,xs,zs,0.05,0.05,0.3333333333333333,0.5);
    bm->weird = simplex_octaves_fill(&bm->weirdNoise, bm->weird, &bm->weirdCap, x,z,xs,zs,0.25,0.25,0.5882352941176471,0.5);
    for(int i=0;i<n;i++) {
        double w = bm->weird[i] * 1.1 + 0.5;
        double m = 0.01, inv = 1.0 - m;
        double t = (bm->temp[i] * 0.15 + 0.7) * inv + w * m;
        m = 0.002; inv = 1.0 - m;
        double h = (bm->humid[i] * 0.15 + 0.5) * inv + w * m;
        t = 1.0 - (1.0 - t) * (1.0 - t);
        if(t<0.0)t=0.0;
        if(h<0.0)h=0.0;
        if(t>1.0)t=1.0;
        if(h>1.0)h=1.0;
        bm->temp[i]=t; bm->humid[i]=h;
        bm->biomes[i] = beta_biomes[beta_biome_lookup((float)t,(float)h)];
    }
    return bm->biomes;
}
static double *biome_manager_temperatures(BiomeManager *bm, double *out, int *cap, int x, int z, int xs, int zs) {
    int n=xs*zs;
    out = simplex_octaves_fill(&bm->tempNoise, out, cap, x,z,xs,zs,0.025,0.025,0.25,0.5);
    bm->weird = simplex_octaves_fill(&bm->weirdNoise, bm->weird, &bm->weirdCap, x,z,xs,zs,0.25,0.25,0.5882352941176471,0.5);
    for(int i=0;i<n;i++) {
        double w=bm->weird[i]*1.1+0.5;
        double m=0.01, inv=1.0-m;
        double t=(out[i]*0.15+0.7)*inv+w*m;
        t=1.0-(1.0-t)*(1.0-t);
        if(t<0.0)t=0.0;
        if(t>1.0)t=1.0;
        out[i]=t;
    }
    return out;
}

typedef struct TerrainProvider {
    int64_t worldSeed;
    JavaRandom rand;
    OctaveNoise k,l,m,n,o,a,b,c;
    BiomeManager biomeManager;
    double *q,*r,*s,*t,*d,*e,*f,*g,*h,*snow;
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
    biome_manager_init(&tp->biomeManager, seed);
}
static void terrain_provider_free(TerrainProvider *tp) {
    free(tp->q); free(tp->r); free(tp->s); free(tp->t); free(tp->d); free(tp->e); free(tp->f); free(tp->g); free(tp->h); free(tp->snow);
    free(tp->biomeManager.temp); free(tp->biomeManager.humid); free(tp->biomeManager.weird);
}

static double *qm_density(TerrainProvider *tp, int x, int y, int z, int xs, int ys, int zs) {
    int n = xs*ys*zs;
    if(!tp->q || tp->qCap<n){ free(tp->q); tp->q=(double*)malloc(sizeof(double)*n); tp->qCap=n; }
    double scaleX = 684.412, scaleY = 684.412;
    double *temp = tp->biomeManager.temp;
    double *humid = tp->biomeManager.humid;
    tp->g = octave_fill2_bridge(&tp->a, tp->g, &tp->gCap, x,z,xs,zs,1.121,1.121,0.5);
    tp->h = octave_fill2_bridge(&tp->b, tp->h, &tp->hCap, x,z,xs,zs,200.0,200.0,0.5);
    tp->d = octave_fill3(&tp->m, tp->d, &tp->dCap, x,y,z,xs,ys,zs,scaleX/80.0,scaleY/160.0,scaleX/80.0);
    tp->e = octave_fill3(&tp->k, tp->e, &tp->eCap, x,y,z,xs,ys,zs,scaleX,scaleY,scaleX);
    tp->f = octave_fill3(&tp->l, tp->f, &tp->fCap, x,y,z,xs,ys,zs,scaleX,scaleY,scaleX);
    int outIdx=0, biomeIdx=0;
    int step = 16 / xs;
    for(int xi=0; xi<xs; xi++) {
        int bx = xi * step + step/2;
        for(int zi=0; zi<zs; zi++) {
            int bz = zi * step + step/2;
            double var21 = temp[bx * 16 + bz];
            double var23 = humid[bx * 16 + bz] * var21;
            double var25 = 1.0 - var23;
            var25 *= var25; var25 *= var25; var25 = 1.0 - var25;
            double var27 = (tp->g[biomeIdx] + 256.0) / 512.0;
            var27 *= var25;
            if(var27 > 1.0) var27 = 1.0;
            double var29 = tp->h[biomeIdx] / 8000.0;
            if(var29 < 0.0) var29 = -var29 * 0.3;
            var29 = var29 * 3.0 - 2.0;
            if(var29 < 0.0) {
                var29 /= 2.0; if(var29 < -1.0)var29=-1.0; var29/=1.4; var29/=2.0; var27=0.0;
            } else { if(var29>1.0)var29=1.0; var29/=8.0; }
            if(var27 < 0.0)var27=0.0;
            var27 += 0.5;
            var29 = var29 * ys / 16.0;
            double var31 = ys / 2.0 + var29 * 4.0;
            biomeIdx++;
            for(int yi=0; yi<ys; yi++) {
                double var34 = 0.0;
                double var36 = (yi - var31) * 12.0 / var27;
                if(var36 < 0.0) var36 *= 4.0;
                double var38 = tp->e[outIdx] / 512.0;
                double var40 = tp->f[outIdx] / 512.0;
                double var42 = (tp->d[outIdx] / 10.0 + 1.0) / 2.0;
                if(var42 < 0.0) var34 = var38; else if(var42 > 1.0) var34 = var40; else var34 = var38 + (var40-var38)*var42;
                var34 -= var36;
                if(yi > ys - 4) { double var44 = (yi - (ys - 4)) / 3.0; var34 = var34 * (1.0-var44) + -10.0 * var44; }
                tp->q[outIdx++] = var34;
            }
        }
    }
    return tp->q;
}

static void qm_generate_base(TerrainProvider *tp, int cx, int cz, unsigned char *blocks, BetaBiome *biomes, double *temps) {
    (void)biomes;
    int cell = 4, sea = 64;
    int xCells = cell + 1, yCells = 17, zCells = cell + 1;
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
                    double temp = temps[(x*4+xx)*16 + z*4+zz];
                    int id = 0;
                    if(ycell*8 + yy < sea) id = (temp < 0.5 && ycell*8 + yy >= sea-1) ? BLK_ICE : BLK_WATER;
                    if(vz > 0.0) id = BLK_STONE;
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

static void qm_replace_surface(TerrainProvider *tp, int cx, int cz, unsigned char *blocks, BetaBiome *biomes) {
    int sea=64;
    tp->r = octave_fill3(&tp->n, tp->r, &tp->rCap, cx*16, cz*16, 0.0, 16,16,1,0.03125,0.03125,1.0);
    tp->s = octave_fill3(&tp->n, tp->s, &tp->sCap, cz*16,109.0134,cx*16,16,1,16,0.03125,1.0,0.03125);
    tp->t = octave_fill3(&tp->o, tp->t, &tp->tCap, cx*16, cz*16, 0.0, 16,16,1,0.0625,0.0625,0.0625);
    for(int x=0;x<16;x++) for(int z=0;z<16;z++) {
        BetaBiome *bio = &biomes[x + z*16];
        int sand = tp->r[x+z*16] + jr_next_double(&tp->rand) * 0.2 > 0.0;
        int gravel = tp->s[x+z*16] + jr_next_double(&tp->rand) * 0.2 > 3.0;
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
                    else if(y >= sea-4 && y <= sea+1) {
                        top = bio->top; filler = bio->filler;
                        if(gravel) top = 0;
                        if(gravel) filler = BLK_GRAVEL;
                        if(sand) top = BLK_SAND;
                        if(sand) filler = BLK_SAND;
                    }
                    if(y < sea && top == 0) top = BLK_WATER;
                    run = depth;
                    blocks[idx] = (y >= sea-1) ? top : filler;
                } else if(run > 0) { run--; blocks[idx] = filler; }
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
                        if(blocks[idx] == BLK_WATER || blocks[idx] == BLK_LAVA) foundLiquid=1;
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
    int64_t s1 = jr_next_long(&rand) / 2LL * 2LL + 1LL;
    int64_t s2 = jr_next_long(&rand) / 2LL * 2LL + 1LL;
    for(int sx=cx-8; sx<=cx+8; sx++) for(int sz=cz-8; sz<=cz+8; sz++) {
        int64_t seed = (int64_t)((uint64_t)(int64_t)sx * (uint64_t)s1 + (uint64_t)(int64_t)sz * (uint64_t)s2) ^ tp->worldSeed;
        jr_set_seed(&rand, seed);
        caves_for_source(&rand, sx, sz, cx, cz, blocks);
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
    for(int i=0;i<count;i++) {
        double px=x1+(x2-x1)*i/count, py=y1+(y2-y1)*i/count, pz=z1+(z2-z1)*i/count;
        double size=(sin(i*M_PI/count)+1.0)*jr_next_float(r)*count/16.0 + 1.0;
        int minX=beta_floor_double(px-size/2.0), minY=beta_floor_double(py-size/2.0), minZ=beta_floor_double(pz-size/2.0);
        int maxX=beta_floor_double(px+size/2.0), maxY=beta_floor_double(py+size/2.0), maxZ=beta_floor_double(pz+size/2.0);
        for(int xx=minX;xx<=maxX;xx++) for(int yy=minY;yy<=maxY;yy++) for(int zz=minZ;zz<=maxZ;zz++) {
            if(xx>=0&&xx<16&&yy>=0&&yy<128&&zz>=0&&zz<16) {
                double dx=(xx+0.5-px)/(size/2.0), dy=(yy+0.5-py)/(size/2.0), dz=(zz+0.5-pz)/(size/2.0);
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
static void qm_populate_local(TerrainProvider *tp, int cx, int cz, unsigned char *blocks, BetaBiome centerBiome) {
    int baseX=cx*16, baseZ=cz*16;
    JavaRandom *r=&tp->rand;
    jr_set_seed(r, tp->worldSeed);
    int64_t s1=jr_next_long(r)/2LL*2LL+1LL, s2=jr_next_long(r)/2LL*2LL+1LL;
    jr_set_seed(r, ((int64_t)((uint64_t)(int64_t)cx*(uint64_t)s1 + (uint64_t)(int64_t)cz*(uint64_t)s2)) ^ tp->worldSeed);

    for(int i=0;i<10;i++) place_ore_vein(blocks,r,BLK_CLAY,32,jr_next_int_bound(r,16),jr_next_int_bound(r,128),jr_next_int_bound(r,16));
    for(int i=0;i<20;i++) place_ore_vein(blocks,r,BLK_DIRT,32,jr_next_int_bound(r,16),jr_next_int_bound(r,128),jr_next_int_bound(r,16));
    for(int i=0;i<10;i++) place_ore_vein(blocks,r,BLK_GRAVEL,32,jr_next_int_bound(r,16),jr_next_int_bound(r,128),jr_next_int_bound(r,16));
    for(int i=0;i<20;i++) place_ore_vein(blocks,r,BLK_COAL_ORE,16,jr_next_int_bound(r,16),jr_next_int_bound(r,128),jr_next_int_bound(r,16));
    for(int i=0;i<20;i++) place_ore_vein(blocks,r,BLK_IRON_ORE,8,jr_next_int_bound(r,16),jr_next_int_bound(r,64),jr_next_int_bound(r,16));
    for(int i=0;i<2;i++) place_ore_vein(blocks,r,BLK_GOLD_ORE,8,jr_next_int_bound(r,16),jr_next_int_bound(r,32),jr_next_int_bound(r,16));
    for(int i=0;i<8;i++) place_ore_vein(blocks,r,BLK_REDSTONE_ORE,7,jr_next_int_bound(r,16),jr_next_int_bound(r,16),jr_next_int_bound(r,16));
    for(int i=0;i<1;i++) place_ore_vein(blocks,r,BLK_DIAMOND_ORE,7,jr_next_int_bound(r,16),jr_next_int_bound(r,16),jr_next_int_bound(r,16));

    double treeNoise = octave_noise2(&tp->c, baseX * 0.5, baseZ * 0.5);
    int trees = (int)((treeNoise / 8.0 + jr_next_double(r) * 4.0 + 4.0) / 3.0);
    int treeCount=0;
    if(jr_next_int_bound(r,10)==0) treeCount++;
    if(centerBiome.id==BIOME_RAINFOREST || centerBiome.id==BIOME_FOREST || centerBiome.id==BIOME_SEASONAL_FOREST || centerBiome.id==BIOME_TAIGA) treeCount += trees + (centerBiome.id==BIOME_SEASONAL_FOREST ? 2 : 5);
    if(centerBiome.id==BIOME_DESERT || centerBiome.id==BIOME_TUNDRA || centerBiome.id==BIOME_PLAINS) treeCount -= 20;
    if(treeCount<0) treeCount=0;
    for(int i=0;i<treeCount;i++) { int x=jr_next_int_bound(r,16), z=jr_next_int_bound(r,16); int y=highest_solid(blocks,x,z)+1; place_beta_tree(blocks,r,x,y,z); }

    for(int i=0;i<2;i++) place_flower(blocks,r,BLK_YELLOW_FLOWER,jr_next_int_bound(r,16),jr_next_int_bound(r,128),jr_next_int_bound(r,16));
    if(jr_next_int_bound(r,2)==0) place_flower(blocks,r,BLK_RED_ROSE,jr_next_int_bound(r,16),jr_next_int_bound(r,128),jr_next_int_bound(r,16));
    if(jr_next_int_bound(r,4)==0) place_flower(blocks,r,BLK_BROWN_MUSHROOM,jr_next_int_bound(r,16),jr_next_int_bound(r,128),jr_next_int_bound(r,16));
    if(jr_next_int_bound(r,8)==0) place_flower(blocks,r,BLK_RED_MUSHROOM,jr_next_int_bound(r,16),jr_next_int_bound(r,128),jr_next_int_bound(r,16));

    tp->snow = biome_manager_temperatures(&tp->biomeManager, tp->snow, &tp->snowCap, baseX+8, baseZ+8, 16,16);
    for(int x=0;x<16;x++) for(int z=0;z<16;z++) {
        int y=highest_solid(blocks,x,z)+1;
        double temp=tp->snow[x*16+z] - (y - 64) / 64.0 * 0.3;
        if(temp < 0.5 && y>0 && y<128) {
            unsigned char below=get_block_local(blocks,x,y-1,z);
            if(below!=0 && below!=BLK_WATER && below!=BLK_ICE && get_block_local(blocks,x,y,z)==0) set_block_local(blocks,x,y,z,BLK_SNOW_LAYER);
        }
    }
}

static void recalc_light_and_height(unsigned char *blocks, unsigned char *sky, unsigned char *heightmap) {
    memset(sky, 0, 16384);
    for(int x=0;x<16;x++) for(int z=0;z<16;z++) {
        int top=0;
        for(int y=127;y>=0;y--) { unsigned char id=get_block_local(blocks,x,y,z); if(id!=0 && id!=BLK_LEAVES && id!=BLK_SNOW_LAYER && id!=BLK_WATER && id!=BLK_ICE) { top=y+1; break; } }
        heightmap[z<<4|x]=(unsigned char)top;
        int sun=1;
        for(int y=127;y>=0;y--) {
            int idx=chunk_index(x,y,z);
            unsigned char id=blocks[idx];
            if(sun) set_nibble(sky,idx,15); else set_nibble(sky,idx,0);
            if(id!=0 && id!=BLK_LEAVES && id!=BLK_SNOW_LAYER && id!=BLK_WATER && id!=BLK_ICE) sun=0;
        }
    }
}

static int write_chunk_file(const char *world_dir, int cx, int cz, long long seed) {
    unsigned char *blocks = (unsigned char*)calloc(32768, 1);
    unsigned char *data = (unsigned char*)calloc(16384, 1);
    unsigned char *sky = (unsigned char*)calloc(16384, 1);
    unsigned char *blocklight = (unsigned char*)calloc(16384, 1);
    unsigned char *heightmap = (unsigned char*)calloc(256, 1);
    if (!blocks || !data || !sky || !blocklight || !heightmap) { free(blocks); free(data); free(sky); free(blocklight); free(heightmap); return 0; }

    TerrainProvider tp;
    terrain_provider_init(&tp, (int64_t)seed);
    jr_set_seed(&tp.rand, (int64_t)((uint64_t)(int64_t)cx * UINT64_C(341873128712) + (uint64_t)(int64_t)cz * UINT64_C(132897987541)));
    BetaBiome *biomes = biome_manager_get(&tp.biomeManager, cx*16, cz*16, 16, 16);
    double *temps = tp.biomeManager.temp;
    qm_generate_base(&tp, cx, cz, blocks, biomes, temps);
    qm_replace_surface(&tp, cx, cz, blocks, biomes);
    qm_generate_caves(&tp, cx, cz, blocks);
    BetaBiome centerBiome = biome_manager_get(&tp.biomeManager, cx*16+16, cz*16+16, 1, 1)[0];
    qm_populate_local(&tp, cx, cz, blocks, centerBiome);
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
    nbt_byte(&b, "TerrainPopulated", 1);
    nbt_empty_compound_list(&b, "Entities");
    nbt_empty_compound_list(&b, "TileEntities");
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
    terrain_provider_free(&tp);
    free(blocks); free(data); free(sky); free(blocklight); free(heightmap);
    return ok;
}

static unsigned long long world_dir_size_quick(const char *path) { return dir_size(path); }
