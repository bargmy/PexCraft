#ifndef PEXCRAFT_XBOX_UWP_COMPAT_H
#define PEXCRAFT_XBOX_UWP_COMPAT_H

/* Xbox One / UWP compatibility layer for the shared C engine files.
   The real UWP shell lives under platforms/xbox_uwp and provides the
   CoreWindow, D3D11 swapchain, app storage, and downloader glue. */

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mmsystem.h>
#include <objbase.h>
#include <initguid.h>
#include <wincodec.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

/* GL-shaped constants/types used by the old immediate-mode renderer facade.
   The Xbox target translates them to Direct3D 11 in backend_compat.c; no OpenGL
   library is loaded. */
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
typedef double GLdouble;
typedef unsigned char GLboolean;
typedef void GLvoid;
#ifndef GL_FALSE
#define GL_FALSE 0
#define GL_TRUE 1
#endif
#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif
#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT 0x00000100
#endif
#ifndef GL_TEXTURE_2D
#define GL_TEXTURE_2D 0x0DE1
#endif
#ifndef GL_BLEND
#define GL_BLEND 0x0BE2
#endif
#ifndef GL_ALPHA_TEST
#define GL_ALPHA_TEST 0x0BC0
#endif
#ifndef GL_DEPTH_TEST
#define GL_DEPTH_TEST 0x0B71
#endif
#ifndef GL_CULL_FACE
#define GL_CULL_FACE 0x0B44
#endif
#ifndef GL_FOG
#define GL_FOG 0x0B60
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION 0x1701
#endif
#ifndef GL_MODELVIEW
#define GL_MODELVIEW 0x1700
#endif
#ifndef GL_PROJECTION_MATRIX
#define GL_PROJECTION_MATRIX 0x0BA7
#endif
#ifndef GL_MODELVIEW_MATRIX
#define GL_MODELVIEW_MATRIX 0x0BA6
#endif
#ifndef GL_COMPILE
#define GL_COMPILE 0x1300
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif
#ifndef GL_LINES
#define GL_LINES 0x0001
#endif
#ifndef GL_LINE_LOOP
#define GL_LINE_LOOP 0x0002
#endif
#ifndef GL_LINE_STRIP
#define GL_LINE_STRIP 0x0003
#endif
#ifndef GL_TRIANGLE_FAN
#define GL_TRIANGLE_FAN 0x0006
#endif
#ifndef GL_TRIANGLE_STRIP
#define GL_TRIANGLE_STRIP 0x0005
#endif
#ifndef GL_UNSIGNED_BYTE
#define GL_UNSIGNED_BYTE 0x1401
#endif
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT 0x1405
#endif
#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif
#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif
#ifndef GL_REPEAT
#define GL_REPEAT 0x2901
#endif
#ifndef GL_NEAREST
#define GL_NEAREST 0x2600
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif
#ifndef GL_TEXTURE_MIN_FILTER
#define GL_TEXTURE_MIN_FILTER 0x2801
#endif
#ifndef GL_TEXTURE_MAG_FILTER
#define GL_TEXTURE_MAG_FILTER 0x2800
#endif
#ifndef GL_TEXTURE_WRAP_S
#define GL_TEXTURE_WRAP_S 0x2802
#endif
#ifndef GL_TEXTURE_WRAP_T
#define GL_TEXTURE_WRAP_T 0x2803
#endif
#ifndef GL_GREATER
#define GL_GREATER 0x0204
#endif
#ifndef GL_LEQUAL
#define GL_LEQUAL 0x0203
#endif
#ifndef GL_SRC_ALPHA
#define GL_SRC_ALPHA 0x0302
#endif
#ifndef GL_ONE_MINUS_SRC_ALPHA
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#endif
#ifndef GL_ONE
#define GL_ONE 1
#endif
#ifndef GL_ZERO
#define GL_ZERO 0
#endif
#ifndef GL_DST_COLOR
#define GL_DST_COLOR 0x0306
#endif
#ifndef GL_SRC_COLOR
#define GL_SRC_COLOR 0x0300
#endif
#ifndef GL_FOG_MODE
#define GL_FOG_MODE 0x0B65
#endif
#ifndef GL_FOG_START
#define GL_FOG_START 0x0B63
#endif
#ifndef GL_FOG_END
#define GL_FOG_END 0x0B64
#endif
#ifndef GL_FOG_COLOR
#define GL_FOG_COLOR 0x0B66
#endif
#ifndef GL_FOG_DENSITY
#define GL_FOG_DENSITY 0x0B62
#endif
#ifndef GL_EXP
#define GL_EXP 0x0800
#endif

/* Extra GL-shaped constants/types used by backend_compat.c.  They are only
   compile-time compatibility tokens for the D3D11 backend; Xbox UWP never
   loads OpenGL. */
typedef unsigned int GLbitfield;
typedef float GLclampf;
#ifndef GL_ONE_MINUS_DST_COLOR
#define GL_ONE_MINUS_DST_COLOR 0x0307
#endif
#ifndef GL_ONE_MINUS_SRC_COLOR
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#endif
#ifndef GL_LESS
#define GL_LESS 0x0201
#endif
#ifndef GL_ALWAYS
#define GL_ALWAYS 0x0207
#endif
#ifndef GL_GEQUAL
#define GL_GEQUAL 0x0206
#endif
#ifndef GL_EQUAL
#define GL_EQUAL 0x0202
#endif
#ifndef GL_VIEWPORT
#define GL_VIEWPORT 0x0BA2
#endif
#ifndef GL_LINES
#define GL_LINES 0x0001
#endif
#ifndef GL_LINE_LOOP
#define GL_LINE_LOOP 0x0002
#endif
#ifndef GL_LINE_STRIP
#define GL_LINE_STRIP 0x0003
#endif
#ifndef GL_TRIANGLE_STRIP
#define GL_TRIANGLE_STRIP 0x0005
#endif
#ifndef GL_TRIANGLE_FAN
#define GL_TRIANGLE_FAN 0x0006
#endif
#ifndef GL_TEXTURE_ENV
#define GL_TEXTURE_ENV 0x2300
#endif
#ifndef GL_TEXTURE_ENV_MODE
#define GL_TEXTURE_ENV_MODE 0x2200
#endif
#ifndef GL_MODULATE
#define GL_MODULATE 0x2100
#endif
#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif
#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT 0x00000100
#endif
#ifndef GL_STENCIL_BUFFER_BIT
#define GL_STENCIL_BUFFER_BIT 0x00000400
#endif
#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif
#ifndef GL_UNSIGNED_BYTE
#define GL_UNSIGNED_BYTE 0x1401
#endif
#ifndef GL_TEXTURE_MIN_FILTER
#define GL_TEXTURE_MIN_FILTER 0x2801
#endif
#ifndef GL_TEXTURE_MAG_FILTER
#define GL_TEXTURE_MAG_FILTER 0x2800
#endif
#ifndef GL_TEXTURE_WRAP_S
#define GL_TEXTURE_WRAP_S 0x2802
#endif
#ifndef GL_TEXTURE_WRAP_T
#define GL_TEXTURE_WRAP_T 0x2803
#endif

/* UWP has no desktop OpenGL import library.  backend_compat.c keeps fallback
   calls in shared code, but the Xbox target forces D3D11.  These stubs satisfy
   the C compiler/linker for unreachable fallback branches. */
static __inline void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { (void)r;(void)g;(void)b;(void)a; }
static __inline void glDisable(GLenum x) { (void)x; }
static __inline void glEnable(GLenum x) { (void)x; }
static __inline void glBlendFunc(GLenum a, GLenum b) { (void)a;(void)b; }
static __inline void glDepthFunc(GLenum x) { (void)x; }
static __inline void glDepthMask(GLboolean x) { (void)x; }
static __inline void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) { (void)r;(void)g;(void)b;(void)a; }
static __inline void glAlphaFunc(GLenum f, GLclampf ref) { (void)f;(void)ref; }
static __inline void glLineWidth(GLfloat w) { (void)w; }
static __inline void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { (void)r;(void)g;(void)b;(void)a; }
static __inline void glViewport(GLint x, GLint y, GLsizei w, GLsizei h) { (void)x;(void)y;(void)w;(void)h; }
static __inline void glClear(GLbitfield mask) { (void)mask; }
static __inline void glMatrixMode(GLenum m) { (void)m; }
static __inline void glLoadIdentity(void) { }
static __inline void glPushMatrix(void) { }
static __inline void glPopMatrix(void) { }
static __inline void glTranslatef(GLfloat x, GLfloat y, GLfloat z) { (void)x;(void)y;(void)z; }
static __inline void glRotatef(GLfloat a, GLfloat x, GLfloat y, GLfloat z) { (void)a;(void)x;(void)y;(void)z; }
static __inline void glScalef(GLfloat x, GLfloat y, GLfloat z) { (void)x;(void)y;(void)z; }
static __inline void glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) { (void)l;(void)r;(void)b;(void)t;(void)n;(void)f; }
static __inline void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zn, GLdouble zf) { (void)fovy;(void)aspect;(void)zn;(void)zf; }
static __inline void glGetFloatv(GLenum pname, GLfloat *p) { (void)pname; if (p) memset(p, 0, sizeof(GLfloat) * 16); }
static __inline void glGetDoublev(GLenum pname, GLdouble *p) { (void)pname; if (p) memset(p, 0, sizeof(GLdouble) * 16); }
static __inline void glGetIntegerv(GLenum pname, GLint *p) { (void)pname; if (p) *p = 0; }
static __inline void glFogi(GLenum p, GLint v) { (void)p;(void)v; }
static __inline void glFogf(GLenum p, GLfloat v) { (void)p;(void)v; }
static __inline void glFogfv(GLenum p, const GLfloat *v) { (void)p;(void)v; }
static __inline void glTexCoord2f(GLfloat u, GLfloat v) { (void)u;(void)v; }
static __inline void glBegin(GLenum m) { (void)m; }
static __inline void glVertex3f(GLfloat x, GLfloat y, GLfloat z) { (void)x;(void)y;(void)z; }
static __inline void glVertex2f(GLfloat x, GLfloat y) { (void)x;(void)y; }
static __inline void glEnd(void) { }
static __inline GLuint glGenLists(GLsizei n) { (void)n; return 0; }
static __inline void glNewList(GLuint id, GLenum mode) { (void)id;(void)mode; }
static __inline void glEndList(void) { }
static __inline void glCallList(GLuint id) { (void)id; }
static __inline void glGenTextures(GLsizei n, GLuint *tex) { if (tex) for (GLsizei i=0;i<n;i++) tex[i]=0; }
static __inline void glBindTexture(GLenum target, GLuint tex) { (void)target;(void)tex; }
static __inline void glTexParameteri(GLenum t, GLenum p, GLint v) { (void)t;(void)p;(void)v; }
static __inline void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) { (void)target;(void)level;(void)internalformat;(void)width;(void)height;(void)border;(void)format;(void)type;(void)pixels; }
static __inline void glDeleteTextures(GLsizei n, const GLuint *tex) { (void)n;(void)tex; }
static __inline void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) { (void)target;(void)level;(void)xoffset;(void)yoffset;(void)x;(void)y;(void)width;(void)height; }

/* Small Win32-style shims that shared code already uses on Android/SDL2. */
#ifndef MAPVK_VK_TO_VSC
#define MAPVK_VK_TO_VSC 0
#endif
static __inline UINT MapVirtualKeyA(UINT code, UINT maptype) { (void)maptype; return code; }
static __inline int GetKeyNameTextA(LONG lparam, char *buf, int cap) { (void)lparam; if (buf && cap > 0) buf[0] = 0; return 0; }
static __inline char *lstrcpynA(char *dst, const char *src, int cap) { if (!dst || cap <= 0) return dst; snprintf(dst, (size_t)cap, "%s", src ? src : ""); return dst; }
static __inline BOOL CopyFileA(const char *src, const char *dst, BOOL fail_if_exists) { (void)fail_if_exists; FILE *in=fopen(src,"rb"); if(!in) return FALSE; FILE *out=fopen(dst,"wb"); if(!out){fclose(in);return FALSE;} char buf[8192]; size_t n; while((n=fread(buf,1,sizeof(buf),in))>0) fwrite(buf,1,n,out); fclose(in); fclose(out); return TRUE; }
static __inline BOOL MoveFileA(const char *src, const char *dst) { return rename(src, dst) == 0; }
static __inline HRESULT CoInitialize(void *reserved) { (void)reserved; return S_OK; }
static __inline BOOL GetClientRect(HWND hwnd, RECT *rc) { (void)hwnd; if (rc) { rc->left=0; rc->top=0; rc->right=1280; rc->bottom=720; } return TRUE; }
static __inline BOOL SwapBuffers(HDC dc) { (void)dc; return TRUE; }

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#endif /* PEXCRAFT_XBOX_UWP_COMPAT_H */
