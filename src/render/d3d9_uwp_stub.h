#ifndef PEX_D3D9_UWP_STUB_H
#define PEX_D3D9_UWP_STUB_H
/*
   Direct3D 9 is not available to UWP/Xbox One apps.  PexCraft's immediate-mode
   compatibility renderer shares one source file between D3D9 and D3D11 and keeps
   some D3D9 fields around even when the runtime backend is D3D11.  This header
   provides compile-only D3D9 stand-ins for UWP builds.  They are never used at
   runtime: pex_renderer_d3d9_init() always fails because Direct3DCreate9()
   returns NULL, and Xbox forces RENDERER_D3D11.
*/
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifndef __cplusplus
#include <stddef.h>
#endif

#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE __stdcall
#endif

#ifndef D3D_OK
#define D3D_OK ((HRESULT)0L)
#endif
#ifndef D3DERR_DEVICELOST
#define D3DERR_DEVICELOST ((HRESULT)0x88760868L)
#endif
#ifndef D3DERR_DEVICENOTRESET
#define D3DERR_DEVICENOTRESET ((HRESULT)0x88760869L)
#endif

#define D3D_SDK_VERSION 32
#define D3DADAPTER_DEFAULT 0
#define D3DDEVTYPE_HAL 1
#define D3DRTYPE_SURFACE 1
#define D3DDEVCAPS_HWTRANSFORMANDLIGHT 0x00010000UL
#define D3DCREATE_HARDWARE_VERTEXPROCESSING 0x00000040UL
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING 0x00000020UL
#define D3DCREATE_FPU_PRESERVE 0x00000002UL
#define D3DUSAGE_WRITEONLY 0x00000008UL
#define D3DUSAGE_DYNAMIC 0x00000200UL
#define D3DUSAGE_DEPTHSTENCIL 0x00000002UL
#define D3DPOOL_DEFAULT 0
#define D3DPOOL_MANAGED 1
#define D3DPOOL_SYSTEMMEM 2
#define D3DSWAPEFFECT_DISCARD 1
#define D3DPRESENT_INTERVAL_IMMEDIATE 0x80000000UL
#define D3DPRESENT_INTERVAL_ONE 0x00000001UL
#define D3DCLEAR_TARGET 0x00000001UL
#define D3DCLEAR_ZBUFFER 0x00000002UL
#define D3DLOCK_DISCARD 0x00002000UL
#define D3DLOCK_READONLY 0x00000010UL

#define D3DFVF_XYZ 0x0002UL
#define D3DFVF_XYZRHW 0x0004UL
#define D3DFVF_DIFFUSE 0x0040UL
#define D3DFVF_TEX1 0x0100UL

#define D3DFMT_A8R8G8B8 21
#define D3DFMT_D16 80
#define D3DFMT_D24S8 75

#define D3DZB_FALSE 0
#define D3DZB_TRUE 1
#define D3DCULL_NONE 1
#define D3DSHADE_GOURAUD 2
#define D3DCMP_NEVER 1
#define D3DCMP_LESS 2
#define D3DCMP_EQUAL 3
#define D3DCMP_LESSEQUAL 4
#define D3DCMP_GREATER 5
#define D3DCMP_GREATEREQUAL 6
#define D3DCMP_ALWAYS 8

#define D3DBLEND_ZERO 1
#define D3DBLEND_ONE 2
#define D3DBLEND_SRCCOLOR 3
#define D3DBLEND_INVSRCCOLOR 4
#define D3DBLEND_SRCALPHA 5
#define D3DBLEND_INVSRCALPHA 6
#define D3DBLEND_DESTCOLOR 9
#define D3DBLEND_INVDESTCOLOR 10

#define D3DTADDRESS_WRAP 1
#define D3DTADDRESS_CLAMP 3
#define D3DTEXF_NONE 0
#define D3DTEXF_POINT 1
#define D3DTEXF_LINEAR 2

#define D3DTOP_DISABLE 1
#define D3DTOP_SELECTARG1 2
#define D3DTOP_MODULATE 4
#define D3DTA_TEXTURE 0x00000002UL
#define D3DTA_DIFFUSE 0x00000000UL
#define D3DFOG_LINEAR 3

#define D3DCOLORWRITEENABLE_RED 1
#define D3DCOLORWRITEENABLE_GREEN 2
#define D3DCOLORWRITEENABLE_BLUE 4
#define D3DCOLORWRITEENABLE_ALPHA 8
#define D3DCOLOR_ARGB(a,r,g,b) ((DWORD)((((a)&255u)<<24)|(((r)&255u)<<16)|(((g)&255u)<<8)|((b)&255u)))
#define D3DCOLOR_XRGB(r,g,b) D3DCOLOR_ARGB(255u,(r),(g),(b))

#define D3DTS_WORLD 256
#define D3DTS_VIEW 2
#define D3DTS_PROJECTION 3

#define D3DPT_TRIANGLELIST 4
#define D3DPT_LINELIST 2
#define D3DPT_LINESTRIP 3

typedef int D3DFORMAT;
typedef int D3DPRIMITIVETYPE;
typedef int D3DRENDERSTATETYPE;
typedef int D3DTEXTURESTAGESTATETYPE;
typedef int D3DSAMPLERSTATETYPE;

enum {
    D3DRS_ZENABLE = 7,
    D3DRS_FILLMODE = 8,
    D3DRS_SHADEMODE = 9,
    D3DRS_ZWRITEENABLE = 14,
    D3DRS_ALPHATESTENABLE = 15,
    D3DRS_LASTPIXEL = 16,
    D3DRS_SRCBLEND = 19,
    D3DRS_DESTBLEND = 20,
    D3DRS_CULLMODE = 22,
    D3DRS_ZFUNC = 23,
    D3DRS_ALPHAREF = 24,
    D3DRS_ALPHAFUNC = 25,
    D3DRS_DITHERENABLE = 26,
    D3DRS_ALPHABLENDENABLE = 27,
    D3DRS_FOGENABLE = 28,
    D3DRS_SPECULARENABLE = 29,
    D3DRS_FOGCOLOR = 34,
    D3DRS_FOGTABLEMODE = 35,
    D3DRS_FOGSTART = 36,
    D3DRS_FOGEND = 37,
    D3DRS_LIGHTING = 137,
    D3DRS_CLIPPING = 136,
    D3DRS_COLORWRITEENABLE = 168
};
enum {
    D3DTSS_COLOROP = 1,
    D3DTSS_COLORARG1 = 2,
    D3DTSS_COLORARG2 = 3,
    D3DTSS_ALPHAOP = 4,
    D3DTSS_ALPHAARG1 = 5,
    D3DTSS_ALPHAARG2 = 6
};
enum {
    D3DSAMP_ADDRESSU = 1,
    D3DSAMP_ADDRESSV = 2,
    D3DSAMP_MINFILTER = 6,
    D3DSAMP_MAGFILTER = 5,
    D3DSAMP_MIPFILTER = 7
};

typedef struct IDirect3D9 IDirect3D9;
typedef struct IDirect3DDevice9 IDirect3DDevice9;
typedef struct IDirect3DBaseTexture9 IDirect3DBaseTexture9;
typedef struct IDirect3DTexture9 IDirect3DTexture9;
typedef struct IDirect3DVertexBuffer9 IDirect3DVertexBuffer9;
typedef struct IDirect3DSurface9 IDirect3DSurface9;
struct IDirect3D9 { int unused; };
struct IDirect3DDevice9 { int unused; };
struct IDirect3DBaseTexture9 { int unused; };
struct IDirect3DTexture9 { int unused; };
struct IDirect3DVertexBuffer9 { int unused; };
struct IDirect3DSurface9 { int unused; };

typedef struct D3DDISPLAYMODE { UINT Width, Height, RefreshRate; D3DFORMAT Format; } D3DDISPLAYMODE;
typedef struct D3DCAPS9 { DWORD DevCaps; } D3DCAPS9;
typedef struct D3DLOCKED_RECT { INT Pitch; void *pBits; } D3DLOCKED_RECT;
typedef struct D3DSURFACE_DESC { D3DFORMAT Format; UINT Width; UINT Height; } D3DSURFACE_DESC;
typedef struct D3DVIEWPORT9 { DWORD X, Y, Width, Height; float MinZ, MaxZ; } D3DVIEWPORT9;
typedef struct D3DPRESENT_PARAMETERS {
    UINT BackBufferWidth;
    UINT BackBufferHeight;
    D3DFORMAT BackBufferFormat;
    UINT BackBufferCount;
    UINT MultiSampleType;
    DWORD MultiSampleQuality;
    UINT SwapEffect;
    HWND hDeviceWindow;
    BOOL Windowed;
    BOOL EnableAutoDepthStencil;
    D3DFORMAT AutoDepthStencilFormat;
    DWORD Flags;
    UINT FullScreen_RefreshRateInHz;
    UINT PresentationInterval;
} D3DPRESENT_PARAMETERS;
typedef struct D3DMATRIX {
    union {
        struct { float _11,_12,_13,_14,_21,_22,_23,_24,_31,_32,_33,_34,_41,_42,_43,_44; };
        float m[4][4];
    };
} D3DMATRIX;

static __inline IDirect3D9 *Direct3DCreate9(UINT sdk) { (void)sdk; return NULL; }
static __inline ULONG IDirect3D9_Release(IDirect3D9 *p) { (void)p; return 0; }
static __inline HRESULT IDirect3D9_GetAdapterDisplayMode(IDirect3D9 *p, UINT a, D3DDISPLAYMODE *m) { (void)p; (void)a; if (m) ZeroMemory(m, sizeof(*m)); return E_FAIL; }
static __inline HRESULT IDirect3D9_CheckDeviceFormat(IDirect3D9 *p, UINT a, UINT dt, D3DFORMAT af, DWORD u, UINT rt, D3DFORMAT cf) { (void)p;(void)a;(void)dt;(void)af;(void)u;(void)rt;(void)cf; return E_FAIL; }
static __inline HRESULT IDirect3D9_CheckDepthStencilMatch(IDirect3D9 *p, UINT a, UINT dt, D3DFORMAT af, D3DFORMAT rf, D3DFORMAT df) { (void)p;(void)a;(void)dt;(void)af;(void)rf;(void)df; return E_FAIL; }
static __inline HRESULT IDirect3D9_GetDeviceCaps(IDirect3D9 *p, UINT a, UINT dt, D3DCAPS9 *c) { (void)p;(void)a;(void)dt; if (c) ZeroMemory(c, sizeof(*c)); return E_FAIL; }
static __inline HRESULT IDirect3D9_CreateDevice(IDirect3D9 *p, UINT a, UINT dt, HWND w, DWORD f, D3DPRESENT_PARAMETERS *pp, IDirect3DDevice9 **dev) { (void)p;(void)a;(void)dt;(void)w;(void)f;(void)pp; if (dev) *dev = NULL; return E_FAIL; }

static __inline ULONG IDirect3DDevice9_Release(IDirect3DDevice9 *p) { (void)p; return 0; }
static __inline HRESULT IDirect3DDevice9_SetRenderState(IDirect3DDevice9 *p, D3DRENDERSTATETYPE s, DWORD v) { (void)p;(void)s;(void)v; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_SetTextureStageState(IDirect3DDevice9 *p, DWORD st, D3DTEXTURESTAGESTATETYPE s, DWORD v) { (void)p;(void)st;(void)s;(void)v; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_SetSamplerState(IDirect3DDevice9 *p, DWORD smp, D3DSAMPLERSTATETYPE s, DWORD v) { (void)p;(void)smp;(void)s;(void)v; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_SetTexture(IDirect3DDevice9 *p, DWORD st, IDirect3DBaseTexture9 *t) { (void)p;(void)st;(void)t; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_SetFVF(IDirect3DDevice9 *p, DWORD fvf) { (void)p;(void)fvf; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_SetStreamSource(IDirect3DDevice9 *p, UINT n, IDirect3DVertexBuffer9 *vb, UINT off, UINT stride) { (void)p;(void)n;(void)vb;(void)off;(void)stride; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_SetTransform(IDirect3DDevice9 *p, int st, const D3DMATRIX *m) { (void)p;(void)st;(void)m; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_CreateVertexBuffer(IDirect3DDevice9 *p, UINT len, DWORD usage, DWORD fvf, UINT pool, IDirect3DVertexBuffer9 **vb, void *shared) { (void)p;(void)len;(void)usage;(void)fvf;(void)pool;(void)shared; if (vb) *vb = NULL; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_DrawPrimitive(IDirect3DDevice9 *p, D3DPRIMITIVETYPE prim, UINT start, UINT count) { (void)p;(void)prim;(void)start;(void)count; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_Clear(IDirect3DDevice9 *p, DWORD c, const void *r, DWORD flags, DWORD color, float z, DWORD stencil) { (void)p;(void)c;(void)r;(void)flags;(void)color;(void)z;(void)stencil; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_SetViewport(IDirect3DDevice9 *p, const D3DVIEWPORT9 *vp) { (void)p;(void)vp; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_CreateTexture(IDirect3DDevice9 *p, UINT w, UINT h, UINT levels, DWORD usage, D3DFORMAT fmt, UINT pool, IDirect3DTexture9 **tex, void *shared) { (void)p;(void)w;(void)h;(void)levels;(void)usage;(void)fmt;(void)pool;(void)shared; if (tex) *tex = NULL; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_GetRenderTarget(IDirect3DDevice9 *p, DWORD idx, IDirect3DSurface9 **s) { (void)p;(void)idx; if (s) *s = NULL; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_CreateOffscreenPlainSurface(IDirect3DDevice9 *p, UINT w, UINT h, D3DFORMAT fmt, UINT pool, IDirect3DSurface9 **s, void *shared) { (void)p;(void)w;(void)h;(void)fmt;(void)pool;(void)shared; if (s) *s = NULL; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_GetRenderTargetData(IDirect3DDevice9 *p, IDirect3DSurface9 *rt, IDirect3DSurface9 *dst) { (void)p;(void)rt;(void)dst; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_BeginScene(IDirect3DDevice9 *p) { (void)p; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_EndScene(IDirect3DDevice9 *p) { (void)p; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_Present(IDirect3DDevice9 *p, const RECT *a, const RECT *b, HWND c, const void *d) { (void)p;(void)a;(void)b;(void)c;(void)d; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_Reset(IDirect3DDevice9 *p, D3DPRESENT_PARAMETERS *pp) { (void)p;(void)pp; return E_FAIL; }
static __inline HRESULT IDirect3DDevice9_TestCooperativeLevel(IDirect3DDevice9 *p) { (void)p; return E_FAIL; }

static __inline ULONG IDirect3DVertexBuffer9_Release(IDirect3DVertexBuffer9 *p) { (void)p; return 0; }
static __inline HRESULT IDirect3DVertexBuffer9_Lock(IDirect3DVertexBuffer9 *p, UINT off, UINT size, void **data, DWORD flags) { (void)p;(void)off;(void)size;(void)flags; if (data) *data = NULL; return E_FAIL; }
static __inline HRESULT IDirect3DVertexBuffer9_Unlock(IDirect3DVertexBuffer9 *p) { (void)p; return E_FAIL; }
static __inline ULONG IDirect3DTexture9_Release(IDirect3DTexture9 *p) { (void)p; return 0; }
static __inline HRESULT IDirect3DTexture9_LockRect(IDirect3DTexture9 *p, UINT level, D3DLOCKED_RECT *lr, const RECT *r, DWORD flags) { (void)p;(void)level;(void)r;(void)flags; if (lr) ZeroMemory(lr, sizeof(*lr)); return E_FAIL; }
static __inline HRESULT IDirect3DTexture9_UnlockRect(IDirect3DTexture9 *p, UINT level) { (void)p;(void)level; return E_FAIL; }
static __inline ULONG IDirect3DSurface9_Release(IDirect3DSurface9 *p) { (void)p; return 0; }
static __inline HRESULT IDirect3DSurface9_GetDesc(IDirect3DSurface9 *p, D3DSURFACE_DESC *d) { (void)p; if (d) ZeroMemory(d, sizeof(*d)); return E_FAIL; }
static __inline HRESULT IDirect3DSurface9_LockRect(IDirect3DSurface9 *p, D3DLOCKED_RECT *lr, const RECT *r, DWORD flags) { (void)p;(void)r;(void)flags; if (lr) ZeroMemory(lr, sizeof(*lr)); return E_FAIL; }
static __inline HRESULT IDirect3DSurface9_UnlockRect(IDirect3DSurface9 *p) { (void)p; return E_FAIL; }

#endif /* PEX_D3D9_UWP_STUB_H */
