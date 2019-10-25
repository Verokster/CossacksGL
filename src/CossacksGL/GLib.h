/*
	MIT License

	Copyright (c) 2019 Oleksiy Ryabchun

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#pragma once

#include "windows.h"
#include "GL\gl.h"

#ifndef ptrdiff_t
#ifdef _WIN64
typedef __int64 ptrdiff_t;
#else
typedef _W64 int ptrdiff_t;
#endif
#endif

typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
typedef char GLchar;

#define GL_VER_1_1 0x01010000
#define GL_VER_1_2 0x01020000
#define GL_VER_2_0 0x02000000
#define GL_VER_3_0 0x03000000

#define GLSL_VER_1_10 "#version 110\n"
#define GLSL_VER_1_30 "#version 130\n"

#define WGL_DRAW_TO_WINDOW_ARB         0x2001
#define WGL_SUPPORT_OPENGL_ARB         0x2010
#define WGL_DOUBLE_BUFFER_ARB          0x2011
#define WGL_PIXEL_TYPE_ARB             0x2013
#define WGL_TYPE_RGBA_ARB              0x202B
#define WGL_COLOR_BITS_ARB             0x2014
#define WGL_DEPTH_BITS_ARB             0x2022
#define WGL_ACCELERATION_ARB           0x2003
#define WGL_FULL_ACCELERATION_ARB      0x2027
#define WGL_DRAW_TO_BITMAP_ARB         0x2002
#define WGL_SUPPORT_GDI_ARB            0x200F
#define WGL_SWAP_METHOD_ARB            0x2007
#define WGL_SWAP_EXCHANGE_ARB          0x2028
#define WGL_SWAP_COPY_ARB              0x2029
#define WGL_STENCIL_BITS_ARB           0x2023
#define WGL_ACCUM_BITS_ARB             0x201D
#define WGL_NUMBER_OVERLAYS_ARB        0x2008
#define WGL_NUMBER_UNDERLAYS_ARB       0x2009

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x0002
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506

#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE_BASE_LEVEL 0x813C
#define GL_TEXTURE_MAX_LEVEL 0x813D

#define GL_CLAMP_TO_EDGE 0x812F

#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STREAM_DRAW 0x88E0
#define GL_STATIC_DRAW 0x88E4

#define GL_R8 0x8229
#define GL_RED 0x1903

#define GL_SHARED_TEXTURE_PALETTE_EXT 0x81FB

#define ERROR_INVALID_VERSION_ARB 0x2095
#define ERROR_INVALID_PROFILE_ARB 0x2096

typedef HGLRC(__stdcall *WGLCREATECONTEXTATTRIBSARB)(HDC hDC, HGLRC hshareContext, const DWORD *attribList);
typedef BOOL(__stdcall *WGLCHOOSEPIXELFORMATARB) (HDC hDC, const INT* piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, INT *piFormats, UINT *nNumFormats);
typedef BOOL(__stdcall *WGLCHOOSEPIXELFORMAT) (HDC hDC, const INT* piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, INT *piFormats, UINT *nNumFormats);
typedef const CHAR* (__stdcall* WGLGETEXTENSIONSSTRING)();
typedef BOOL(__stdcall *WGLSWAPINTERVAL)(INT interval);

typedef const GLubyte* (__stdcall *GLGETSTRING)(GLenum name);
typedef VOID(__stdcall *GLVERTEX2S)(GLshort x, GLshort y);
typedef VOID(__stdcall *GLTEXCOORD2F)(GLfloat s, GLfloat t);
typedef VOID(__stdcall *GLBEGIN)(GLenum mode);
typedef VOID(__stdcall *GLEND)();
typedef VOID(__stdcall *GLVIEWPORT)(GLint x, GLint y, GLsizei width, GLsizei height);
typedef VOID(__stdcall *GLMATRIXMODE)(GLenum mode);
typedef VOID(__stdcall *GLLOADIDENTITY)();
typedef VOID(__stdcall *GLORTHO)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
typedef VOID(__stdcall *GLFINISH)();
typedef VOID(__stdcall *GLENABLE)(GLenum cap);
typedef VOID(__stdcall *GLBINDTEXTURE)(GLenum target, GLuint texture);
typedef VOID(__stdcall *GLDELETETEXTURES)(GLsizei n, const GLuint *textures);
typedef VOID(__stdcall *GLTEXPARAMETERI)(GLenum target, GLenum pname, GLint param);
typedef VOID(__stdcall *GLTEXENVI)(GLenum target, GLenum pname, GLint param);
typedef VOID(__stdcall *GLTEXIMAGE1D)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data);
typedef VOID(__stdcall *GLTEXSUBIMAGE1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* data);
typedef VOID(__stdcall *GLTEXIMAGE2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
typedef VOID(__stdcall *GLTEXSUBIMAGE2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
typedef GLenum(__stdcall *GLGENTEXTURES)(GLsizei n, GLuint* textures);
typedef VOID(__stdcall *GLGETINTEGERV)(GLenum pname, GLint* data);
typedef VOID(__stdcall *GLCLEAR)(GLbitfield mask);
typedef VOID(__stdcall *GLCLEARCOLOR)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef VOID(__stdcall *GLCOLORTABLE)(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *data);
typedef VOID(__stdcall *GLREADPIXELS)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* data);
typedef VOID(__stdcall *GLPIXELSTOREI)(GLenum pname, GLint param);

#ifdef _DEBUG
typedef GLenum(__stdcall *GLGETERROR)();
#endif

typedef VOID(__stdcall *GLACTIVETEXTURE)(GLenum texture);
typedef VOID(__stdcall *GLGENBUFFERS)(GLsizei n, GLuint* buffers);
typedef VOID(__stdcall *GLDELETEBUFFERS)(GLsizei n, const GLuint* buffers);
typedef VOID(__stdcall *GLBINDBUFFER)(GLenum target, GLuint buffer);
typedef VOID*(__stdcall *GLMAPBUFFERRANGE)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
typedef GLboolean(__stdcall *GLUNMAPBUFFER)(GLenum target);
typedef VOID(__stdcall *GLFLUSHMAPPEDBUFFERRANGE)(GLenum target, GLintptr offset, GLsizeiptr length);
typedef VOID(__stdcall *GLBUFFERDATA)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
typedef VOID(__stdcall *GLDRAWARRAYS)(GLenum mode, GLint first, GLsizei count);
typedef VOID(__stdcall *GLDRAWELEMENTS)(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
typedef VOID(__stdcall *GLGENVERTEXARRAYS)(GLsizei n, GLuint* arrays);
typedef VOID(__stdcall *GLBINDVERTEXARRAY)(GLuint array);
typedef VOID(__stdcall *GLDELETEVERTEXARRAYS)(GLsizei n, const GLuint* arrays);

typedef VOID(__stdcall *GLENABLEVERTEXATTRIBARRAY)(GLuint index);
typedef VOID(__stdcall *GLVERTEXATTRIBPOINTER)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);

typedef GLuint(__stdcall *GLCREATESHADER)(GLenum shaderType);
typedef VOID(__stdcall *GLDELETESHADER)(GLuint shader);
typedef GLuint(__stdcall *GLCREATEPROGRAM)();
typedef VOID(__stdcall *GLDELETEPROGRAM)(GLuint program);
typedef VOID(__stdcall *GLSHADERSOURCE)(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
typedef VOID(__stdcall *GLCOMPILESHADER)(GLuint shader);
typedef VOID(__stdcall *GLATTACHSHADER)(GLuint program, GLuint shader);
typedef VOID(__stdcall *GLDETACHSHADER)(GLuint program, GLuint shader);
typedef VOID(__stdcall *GLLINKPROGRAM)(GLuint program);
typedef VOID(__stdcall *GLUSEPROGRAM)(GLuint program);
typedef VOID(__stdcall *GLGETSHADERIV)(GLuint shader, GLenum pname, GLint* params);
typedef VOID(__stdcall *GLGETSHADERINFOLOG)(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);

typedef VOID(__stdcall* GLBINDATTRIBLOCATION)(GLuint program, GLuint index, const GLchar* name);
typedef GLuint(__stdcall *GLGETUNIFORMLOCATION)(GLuint program, const GLchar* name);

typedef VOID(__stdcall *GLUNIFORM1I)(GLint location, GLint v0);
typedef VOID(__stdcall *GLUNIFORM2F)(GLint location, GLfloat v0, GLfloat v1);

extern WGLCREATECONTEXTATTRIBSARB WGLCreateContextAttribs;
extern WGLCHOOSEPIXELFORMAT WGLChoosePixelFormat;
extern WGLGETEXTENSIONSSTRING WGLGetExtensionsString;
extern WGLSWAPINTERVAL WGLSwapInterval;

extern GLGETSTRING GLGetString;
extern GLVERTEX2S GLVertex2s;
extern GLTEXCOORD2F GLTexCoord2f;
extern GLBEGIN GLBegin;
extern GLEND GLEnd;
extern GLVIEWPORT GLViewport;
extern GLMATRIXMODE GLMatrixMode;
extern GLLOADIDENTITY GLLoadIdentity;
extern GLORTHO GLOrtho;
extern GLFINISH GLFinish;
extern GLENABLE GLEnable;
extern GLBINDTEXTURE GLBindTexture;
extern GLDELETETEXTURES GLDeleteTextures;
extern GLTEXPARAMETERI GLTexParameteri;
extern GLTEXENVI GLTexEnvi;
extern GLTEXIMAGE1D GLTexImage1D;
extern GLTEXSUBIMAGE1D GLTexSubImage1D;
extern GLTEXIMAGE2D GLTexImage2D;
extern GLTEXSUBIMAGE2D GLTexSubImage2D;
extern GLGENTEXTURES GLGenTextures;
extern GLGETINTEGERV GLGetIntegerv;
extern GLCLEAR GLClear;
extern GLCLEARCOLOR GLClearColor;
extern GLCOLORTABLE GLColorTable;
extern GLREADPIXELS GLReadPixels;
extern GLPIXELSTOREI GLPixelStorei;

#ifdef _DEBUG
extern GLGETERROR GLGetError;
#endif

extern GLACTIVETEXTURE GLActiveTexture;
extern GLGENBUFFERS GLGenBuffers;
extern GLDELETEBUFFERS GLDeleteBuffers;
extern GLBINDBUFFER GLBindBuffer;
extern GLBUFFERDATA GLBufferData;
extern GLDRAWARRAYS GLDrawArrays;
extern GLGENVERTEXARRAYS GLGenVertexArrays;
extern GLBINDVERTEXARRAY GLBindVertexArray;
extern GLDELETEVERTEXARRAYS GLDeleteVertexArrays;

extern GLENABLEVERTEXATTRIBARRAY GLEnableVertexAttribArray;
extern GLVERTEXATTRIBPOINTER GLVertexAttribPointer;

extern GLCREATESHADER GLCreateShader;
extern GLDELETESHADER GLDeleteShader;
extern GLCREATEPROGRAM GLCreateProgram;
extern GLDELETEPROGRAM GLDeleteProgram;
extern GLSHADERSOURCE GLShaderSource;
extern GLCOMPILESHADER GLCompileShader;
extern GLATTACHSHADER GLAttachShader;
extern GLDETACHSHADER GLDetachShader;
extern GLLINKPROGRAM GLLinkProgram;
extern GLUSEPROGRAM GLUseProgram;
extern GLGETSHADERIV GLGetShaderiv;
extern GLGETSHADERINFOLOG GLGetShaderInfoLog;

extern GLBINDATTRIBLOCATION GLBindAttribLocation;
extern GLGETUNIFORMLOCATION GLGetUniformLocation;

extern GLUNIFORM1I GLUniform1i;
extern GLUNIFORM2F GLUniform2f;

extern DWORD glVersion;
extern DWORD glCapsClampToEdge;
extern BOOL glCapsSharedPalette;
extern BOOL glCapsBGR;

namespace GL
{
	VOID __fastcall CreateContextAttribs(HDC hDc, HGLRC* hRc);
	VOID __fastcall ResetPixelFormatDescription(PIXELFORMATDESCRIPTOR* pfd);
	VOID __fastcall PreparePixelFormatDescription(PIXELFORMATDESCRIPTOR* pfd);
	INT __fastcall PreparePixelFormat(PIXELFORMATDESCRIPTOR* pfd);
	VOID __fastcall ResetPixelFormat();
	GLuint __fastcall CompileShaderSource(DWORD name, const CHAR* version, GLenum type);
}