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

#include "stdafx.h"
#include "Resource.h"
#include "Windowsx.h"
#include "Main.h"
#include "Config.h"
#include "Hooks.h"
#include "Window.h"
#include "DirectDraw.h"
#include "DirectDrawSurface.h"
#include "FpsCounter.h"
#include "PixelBuffer.h"

#define RECOUNT 64
#define WHITE 0xFFFFFFFF;

DisplayMode resolutionsList[64];

DWORD __fastcall AddDisplayMode(DEVMODE* devMode)
{
	DisplayMode* resList = resolutionsList;

	for (DWORD i = 0; i < RECOUNT; ++i, ++resList)
	{
		if (!resList->width)
		{
			resList->width = devMode->dmPelsWidth;
			resList->height = devMode->dmPelsHeight;
			resList->bpp = devMode->dmBitsPerPel;
			resList->frequency = devMode->dmDisplayFrequency;
			return i + 1;
		}

		if (resList->width == devMode->dmPelsWidth)
		{
			if (resList->height == devMode->dmPelsHeight)
			{
				BOOL succ = FALSE;
				if (resList->bpp == 8 && resList->bpp == devMode->dmBitsPerPel)
					succ = TRUE;
				else if (resList->bpp != 8 && devMode->dmBitsPerPel != 8)
				{
					succ = TRUE;

					if (resList->bpp <= devMode->dmBitsPerPel)
						resList->bpp = devMode->dmBitsPerPel;
				}

				if (succ)
				{
					if (resList->frequency < devMode->dmDisplayFrequency)
						resList->frequency = devMode->dmDisplayFrequency;

					return i + 1;
				}
			}
		}
	}

	return 0;
}

DWORD __stdcall RenderThread(LPVOID lpParameter)
{
	DirectDraw* ddraw = (DirectDraw*)lpParameter;
	ddraw->hDc = ::GetDC(ddraw->hDraw);
	if (ddraw->hDc)
	{
		if (!::GetPixelFormat(ddraw->hDc))
		{
			PIXELFORMATDESCRIPTOR pfd;
			INT glPixelFormat = GL::PreparePixelFormat(&pfd);
			if (!glPixelFormat)
			{
				glPixelFormat = ChoosePixelFormat(ddraw->hDc, &pfd);
				if (!glPixelFormat)
					Main::ShowError(IDS_ERROR_CHOOSE_PF, __FILE__, __LINE__);
				else if (pfd.dwFlags & PFD_NEED_PALETTE)
					Main::ShowError(IDS_ERROR_NEED_PALETTE, __FILE__, __LINE__);
			}

			GL::ResetPixelFormatDescription(&pfd);
			if (DescribePixelFormat(ddraw->hDc, glPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd) == NULL)
				Main::ShowError(IDS_ERROR_DESCRIBE_PF, __FILE__, __LINE__);

			if (!SetPixelFormat(ddraw->hDc, glPixelFormat, &pfd))
				Main::ShowError(IDS_ERROR_SET_PF, __FILE__, __LINE__);

			if ((pfd.iPixelType != PFD_TYPE_RGBA) || (pfd.cRedBits < 5) || (pfd.cGreenBits < 6) || (pfd.cBlueBits < 5))
				Main::ShowError(IDS_ERROR_BAD_PF, __FILE__, __LINE__);
		}

		HGLRC hRc = wglCreateContext(ddraw->hDc);
		if (hRc)
		{
			if (wglMakeCurrent(ddraw->hDc, hRc))
			{
				GL::CreateContextAttribs(ddraw->hDc, &hRc);
				if (glVersion >= GL_VER_2_0)
				{
					DWORD maxSize = ddraw->dwMode->width > ddraw->dwMode->height ? ddraw->dwMode->width : ddraw->dwMode->height;

					DWORD maxTexSize = 1;
					while (maxTexSize < maxSize)
						maxTexSize <<= 1;

					DWORD glMaxTexSize;
					GLGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&glMaxTexSize);
					if (maxTexSize > glMaxTexSize)
						glVersion = GL_VER_1_1;
				}

				GLPixelStorei(GL_UNPACK_ALIGNMENT, 8);

				timeBeginPeriod(1);
				{
					SetEvent(ddraw->hCheckEvent);
					if (glVersion >= GL_VER_2_0)
						ddraw->RenderNew();
					else
						ddraw->RenderOld();
				}
				timeEndPeriod(1);

				wglMakeCurrent(ddraw->hDc, NULL);
			}

			wglDeleteContext(hRc);
		}

		::ReleaseDC(ddraw->hDraw, ddraw->hDc);
		ddraw->hDc = NULL;
	}
	else
		SetEvent(ddraw->hCheckEvent);

	return NULL;
}

#pragma optimize("t", on)

DWORD __fastcall GetPow2(DWORD value)
{
	DWORD res = 1;
	while (res < value)
		res <<= 1;
	return res;
}

VOID __fastcall UseShaderProgram(ShaderProgram* program, DWORD texSize)
{
	if (!program->id)
	{
		program->id = GLCreateProgram();

		GLBindAttribLocation(program->id, 0, "vCoord");
		GLBindAttribLocation(program->id, 1, "vTex");

		GLuint vShader = GL::CompileShaderSource(program->vertexName, program->version, GL_VERTEX_SHADER);
		GLuint fShader = GL::CompileShaderSource(program->fragmentName, program->version, GL_FRAGMENT_SHADER);
		{

			GLAttachShader(program->id, vShader);
			GLAttachShader(program->id, fShader);
			{
				GLLinkProgram(program->id);
			}
			GLDetachShader(program->id, fShader);
			GLDetachShader(program->id, vShader);
		}
		GLDeleteShader(fShader);
		GLDeleteShader(vShader);

		GLUseProgram(program->id);

		GLUniform1i(GLGetUniformLocation(program->id, "tex01"), GL_TEXTURE0 - GL_TEXTURE0);

		GLint loc = GLGetUniformLocation(program->id, "pal01");
		if (loc >= 0)
			GLUniform1i(loc, GL_TEXTURE1 - GL_TEXTURE0);

		loc = GLGetUniformLocation(program->id, "texSize");
		if (loc >= 0)
			GLUniform2f(loc, (FLOAT)texSize, (FLOAT)texSize);
	}
	else
		GLUseProgram(program->id);
}

VOID DirectDraw::RenderOld()
{
	DWORD glMaxTexSize;
	GLGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&glMaxTexSize);
	if (glMaxTexSize < 256)
		glMaxTexSize = 256;

	DWORD maxAllow = GetPow2(this->dwMode->width > this->dwMode->height ? this->dwMode->width : this->dwMode->height);
	DWORD maxTexSize = maxAllow < glMaxTexSize ? maxAllow : glMaxTexSize;
	DWORD texPitch = this->pitch / (this->dwMode->bpp >> 3);

	VOID* pixelBuffer = AlignedAlloc(maxTexSize * maxTexSize * sizeof(DWORD));
	{
		DWORD framePerWidth = this->dwMode->width / maxTexSize + (this->dwMode->width % maxTexSize ? 1 : 0);
		DWORD framePerHeight = this->dwMode->height / maxTexSize + (this->dwMode->height % maxTexSize ? 1 : 0);
		DWORD frameCount = framePerWidth * framePerHeight;
		Frame* frames = (Frame*)MemoryAlloc(frameCount * sizeof(Frame));
		{
			Frame* frame = frames;
			for (DWORD y = 0; y < this->dwMode->height; y += maxTexSize)
			{
				DWORD height = this->dwMode->height - y;
				if (height > maxTexSize)
					height = maxTexSize;

				for (DWORD x = 0; x < this->dwMode->width; x += maxTexSize, ++frame)
				{
					DWORD width = this->dwMode->width - x;
					if (width > maxTexSize)
						width = maxTexSize;

					frame->rect.x = x;
					frame->rect.y = y;
					frame->rect.width = width;
					frame->rect.height = height;

					frame->vSize.width = x + width;
					frame->vSize.height = y + height;

					frame->tSize.width = width == maxTexSize ? 1.0f : (FLOAT)width / maxTexSize;
					frame->tSize.height = height == maxTexSize ? 1.0f : (FLOAT)height / maxTexSize;

					GLGenTextures(1, &frame->id);
					GLBindTexture(GL_TEXTURE_2D, frame->id);

					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glCapsClampToEdge);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glCapsClampToEdge);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

					GLint filter = config.windowedMode && config.filtering ? GL_LINEAR : GL_NEAREST;
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);

					GLTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

					if (this->dwMode->bpp == 8 && GLColorTable)
						GLTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, maxTexSize, maxTexSize, GL_NONE, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, NULL);
					else
						GLTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, maxTexSize, maxTexSize, GL_NONE, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				}
			}

			GLClearColor(0.0, 0.0, 0.0, 1.0);
			this->viewport.refresh = TRUE;

			GLMatrixMode(GL_PROJECTION);
			GLLoadIdentity();
			GLOrtho(0.0, this->dwMode->width, this->dwMode->height, 0.0, 0.0, 1.0);
			GLMatrixMode(GL_MODELVIEW);
			GLLoadIdentity();

			GLEnable(GL_TEXTURE_2D);

			if (this->dwMode->bpp == 8 && glCapsSharedPalette)
				GLEnable(GL_SHARED_TEXTURE_PALETTE_EXT);

			FpsCounter* fpsCounter = new FpsCounter(FPS_ACCURACY);
			{
				BOOL isVSync;
				DOUBLE fpsSync, nextSyncTime;
				if (config.singleThread)
				{
					fpsSync = 1.0f / (this->dwMode->frequency ? this->dwMode->frequency : 60);
					nextSyncTime = 0.0f;
				}
				else
				{
					isVSync = FALSE;
					if (WGLSwapInterval && !config.singleThread)
						WGLSwapInterval(0);
				}

				do
				{
					if (this->dwMode->bpp == 8 || !mciVideo.deviceId)
					{
						BOOL isValid = TRUE;
						if (config.singleThread)
						{
							LONGLONG qp;
							QueryPerformanceFrequency((LARGE_INTEGER*)&qp);
							DOUBLE timerResolution = (DOUBLE)qp;

							QueryPerformanceCounter((LARGE_INTEGER*)&qp);
							DOUBLE currTime = (DOUBLE)qp / timerResolution;

							if (currTime >= nextSyncTime)
								nextSyncTime += fpsSync * (1.0f + (FLOAT)DWORD((currTime - nextSyncTime) / fpsSync));
							else
								isValid = FALSE;
						}
						else if (isVSync != config.vSync)
						{
							isVSync = config.vSync;
							if (WGLSwapInterval)
								WGLSwapInterval(isVSync);
						}

						if (isValid)
						{
							this->CheckView();

							if (this->isStateChanged)
							{
								this->isStateChanged = FALSE;
								GLint filter = config.windowedMode && config.filtering ? GL_LINEAR : GL_NEAREST;
								GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
								GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
							}

							if (this->dwMode->bpp == 8)
							{
								if (config.fpsCounter)
								{
									if (isFpsChanged)
									{
										isFpsChanged = FALSE;
										fpsCounter->Reset();
									}

									fpsCounter->Calculate();
								}

								if (glCapsSharedPalette && this->isPalChanged)
								{
									this->isPalChanged = FALSE;
									GLColorTable(GL_TEXTURE_2D, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_BYTE, this->palette);
								}

								DWORD count = frameCount;
								frame = frames;
								while (count--)
								{
									GLBindTexture(GL_TEXTURE_2D, frame->id);

									if (GLColorTable)
									{
										if (!glCapsSharedPalette)
											GLColorTable(GL_TEXTURE_2D, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_BYTE, this->palette);

										BYTE* pix = (BYTE*)pixelBuffer;
										for (INT y = frame->rect.y; y < frame->vSize.height; ++y)
										{
											BYTE* idx = this->indexBuffer + y * texPitch + frame->rect.x;
											MemoryCopy(pix, idx, frame->rect.width);
											pix += frame->rect.width;
										}

										if (config.fpsCounter && count == frameCount - 1)
										{
											DWORD fps = fpsCounter->value;
											DWORD digCount = 0;
											DWORD current = fps;
											do
											{
												++digCount;
												current = current / 10;
											} while (current);

											DWORD dcount = digCount;
											current = fps;
											do
											{
												WORD* lpDig = (WORD*)counters[current % 10];

												for (DWORD y = 0; y < FPS_HEIGHT; ++y)
												{
													BYTE* pix = (BYTE*)pixelBuffer + (FPS_Y + y) * frame->rect.width + FPS_X + FPS_WIDTH * (dcount - 1);

													WORD check = *lpDig++;
													DWORD width = FPS_WIDTH;
													do
													{
														if (check & 1)
															*pix = 0xFF;
														++pix;
														check >>= 1;
													} while (--width);
												}

												current = current / 10;
											} while (--dcount);
										}

										GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->rect.width, frame->rect.height, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, pixelBuffer);
									}
									else
									{
										DWORD* pix = (DWORD*)pixelBuffer;
										for (INT y = frame->rect.y; y < frame->vSize.height; ++y)
										{
											BYTE* idx = this->indexBuffer + y * texPitch + frame->rect.x;
											for (INT x = frame->rect.x; x < frame->vSize.width; ++x)
												*pix++ = this->palette[*idx++];
										}

										if (config.fpsCounter && count == frameCount - 1)
										{
											DWORD fps = fpsCounter->value;
											DWORD digCount = 0;
											DWORD current = fps;
											do
											{
												++digCount;
												current = current / 10;
											} while (current);

											DWORD dcount = digCount;
											current = fps;
											do
											{
												DWORD digit = current % 10;
												WORD* lpDig = (WORD*)counters[digit];

												for (DWORD y = 0; y < FPS_HEIGHT; ++y)
												{
													DWORD* pix = (DWORD*)pixelBuffer + (FPS_Y + y) * frame->rect.width + FPS_X + FPS_WIDTH * (dcount - 1);

													WORD check = *lpDig++;
													DWORD width = FPS_WIDTH;
													do
													{
														if (check & 1)
															*pix = 0xFFFFFFFF;
														++pix;
														check >>= 1;
													} while (--width);
												}

												current = current / 10;
											} while (--dcount);
										}

										GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->rect.width, frame->rect.height, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer);
									}

									GLBegin(GL_TRIANGLE_FAN);
									{
										GLTexCoord2f(0.0, 0.0);
										GLVertex2s(LOWORD(frame->rect.x), LOWORD(frame->rect.y));

										GLTexCoord2f(frame->tSize.width, 0.0);
										GLVertex2s(LOWORD(frame->vSize.width), LOWORD(frame->rect.y));

										GLTexCoord2f(frame->tSize.width, frame->tSize.height);
										GLVertex2s(LOWORD(frame->vSize.width), LOWORD(frame->vSize.height));

										GLTexCoord2f(0.0, frame->tSize.height);
										GLVertex2s(LOWORD(frame->rect.x), LOWORD(frame->vSize.height));
									}
									GLEnd();
									++frame;
								}
							}
							else
							{
								DWORD count = frameCount;
								frame = frames;
								while (count--)
								{
									GLBindTexture(GL_TEXTURE_2D, frame->id);

									DWORD* pix = (DWORD*)pixelBuffer;
									for (INT y = frame->rect.y; y < frame->vSize.height; ++y)
									{
										DWORD* idx = (DWORD*)this->indexBuffer + y * texPitch + frame->rect.x;
										for (INT x = frame->rect.x; x < frame->vSize.width; ++x)
											*pix++ = *idx++;
									}

									GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->rect.width, frame->rect.height, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer);

									GLBegin(GL_TRIANGLE_FAN);
									{
										GLTexCoord2f(0.0, 0.0);
										GLVertex2s(LOWORD(frame->rect.x), LOWORD(frame->rect.y));

										GLTexCoord2f(frame->tSize.width, 0.0);
										GLVertex2s(LOWORD(frame->vSize.width), LOWORD(frame->rect.y));

										GLTexCoord2f(frame->tSize.width, frame->tSize.height);
										GLVertex2s(LOWORD(frame->vSize.width), LOWORD(frame->vSize.height));

										GLTexCoord2f(0.0, frame->tSize.height);
										GLVertex2s(LOWORD(frame->rect.x), LOWORD(frame->vSize.height));
									}
									GLEnd();
									++frame;
								}
							}

							SwapBuffers(this->hDc);
							GLFinish();

							if (this->isTakeSnapshot)
							{
								this->isTakeSnapshot = FALSE;
								this->TakeScreenshot();
							}
						}

						Sleep(0);
					}
					else
					{
						Sleep(1);

						if (this->isTakeSnapshot)
							this->TakeScreenshot();
					}
				} while (!this->isFinish);
				GLFinish();
			}
			delete fpsCounter;

			frame = frames;
			DWORD count = frameCount;
			while (count--)
			{
				GLDeleteTextures(1, &frame->id);
				++frame;
			}
		}
		MemoryFree(frames);
	}
	AlignedFree(pixelBuffer);
}

VOID DirectDraw::RenderNew()
{
	DWORD maxTexSize = GetPow2(this->dwMode->width > this->dwMode->height ? this->dwMode->width : this->dwMode->height);
	FLOAT texWidth = this->dwMode->width == maxTexSize ? 1.0f : FLOAT((FLOAT)this->dwMode->width / maxTexSize);
	FLOAT texHeight = this->dwMode->height == maxTexSize ? 1.0f : FLOAT((FLOAT)this->dwMode->height / maxTexSize);

	DWORD texPitch = this->pitch / (this->dwMode->bpp >> 3);

	const CHAR* glslVersion = glVersion >= GL_VER_3_0 ? GLSL_VER_1_30 : GLSL_VER_1_10;

	struct {
		ShaderProgram simple;
		ShaderProgram nearest;
		ShaderProgram linear;
	} shaders = {
		{ 0, glslVersion, IDR_VERTEX_SIMPLE, IDR_FRAGMENT_SIMPLE },
		{ 0, glslVersion, IDR_VERTEX_NEAREST, IDR_FRAGMENT_NEAREST },
		{ 0, glslVersion, IDR_VERTEX_LINEAR, IDR_FRAGMENT_LINEAR },
	};

	{
		GLuint arrayName;
		if (glVersion >= GL_VER_3_0)
		{
			GLGenVertexArrays(1, &arrayName);
			GLBindVertexArray(arrayName);
		}

		GLuint bufferName;
		GLGenBuffers(1, &bufferName);
		{
			GLBindBuffer(GL_ARRAY_BUFFER, bufferName);
			{
				{
					FLOAT mvp[4][4] = {
						{ FLOAT(2.0f / this->dwMode->width), 0.0f, 0.0f, 0.0f },
						{ 0.0f, FLOAT(-2.0f / this->dwMode->height), 0.0f, 0.0f },
						{ 0.0f, 0.0f, 2.0f, 0.0f },
						{ -1.0f, 1.0f, -1.0f, 1.0f }
					};

					FLOAT buffer[4][8] = {
						{ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f },
						{ (FLOAT)this->dwMode->width, 0.0f, 0.0f, 1.0f, texWidth, 0.0f, 0.0f, 0.0f },
						{ (FLOAT)this->dwMode->width, (FLOAT)this->dwMode->height, 0.0f, 1.0f, texWidth, texHeight, 0.0f, 0.0f },
						{ 0.0f, (FLOAT)this->dwMode->height, 0.0f, 1.0f, 0.0f, texHeight, 0.0f, 0.0f }
					};

					for (DWORD i = 0; i < 4; ++i)
					{
						FLOAT* vector = &buffer[i][0];
						for (DWORD j = 0; j < 4; ++j)
						{
							FLOAT sum = 0.0f;
							for (DWORD v = 0; v < 4; ++v)
								sum += mvp[v][j] * vector[v];

							vector[j] = sum;
						}
					}

					GLBufferData(GL_ARRAY_BUFFER, sizeof(buffer), buffer, GL_STATIC_DRAW);
				}

				{
					GLEnableVertexAttribArray(0);
					GLVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 32, (GLvoid*)0);

					GLEnableVertexAttribArray(1);
					GLVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 32, (GLvoid*)16);
				}

				GLClearColor(0.0, 0.0, 0.0, 1.0);
				this->viewport.refresh = TRUE;
				this->isStateChanged = TRUE;
				this->isPalChanged = TRUE;

				FpsCounter* fpsCounter = new FpsCounter(FPS_ACCURACY);
				{
					BOOL isVSync;
					DOUBLE fpsSync, nextSyncTime;
					if (config.singleThread)
					{
						fpsSync = 1.0f / (this->dwMode->frequency ? this->dwMode->frequency : 60);
						nextSyncTime = 0.0f;
					}
					else
					{
						isVSync = FALSE;
						if (WGLSwapInterval && !config.singleThread)
							WGLSwapInterval(0);
					}

					if (this->dwMode->bpp == 8)
					{
						GLuint textures[2];
						GLuint paletteId = textures[0];
						GLuint indicesId = textures[1];
						GLGenTextures(2, textures);
						{
							GLActiveTexture(GL_TEXTURE1);
							GLBindTexture(GL_TEXTURE_1D, paletteId);

							GLTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, glCapsClampToEdge);
							GLTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_BASE_LEVEL, 0);
							GLTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_LEVEL, 0);
							GLTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
							GLTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
							GLTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, GL_NONE, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

							GLActiveTexture(GL_TEXTURE0);
							GLBindTexture(GL_TEXTURE_2D, indicesId);

							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glCapsClampToEdge);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glCapsClampToEdge);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
							GLTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, maxTexSize, maxTexSize, GL_NONE, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);

							PixelBuffer* pixelBuffer = new PixelBuffer(texPitch, this->dwMode->height);
							{
								GLPixelStorei(GL_UNPACK_ROW_LENGTH, this->pitch / (this->dwMode->bpp >> 3));
								{
									do
									{
										BOOL isValid = TRUE;
										if (config.singleThread)
										{
											LONGLONG qp;
											QueryPerformanceFrequency((LARGE_INTEGER*)&qp);
											DOUBLE timerResolution = (DOUBLE)qp;

											QueryPerformanceCounter((LARGE_INTEGER*)&qp);
											DOUBLE currTime = (DOUBLE)qp / timerResolution;

											if (currTime >= nextSyncTime)
												nextSyncTime += fpsSync * (1.0f + (FLOAT)DWORD((currTime - nextSyncTime) / fpsSync));
											else
												isValid = FALSE;
										}
										else if (isVSync != config.vSync)
										{
											isVSync = config.vSync;
											if (WGLSwapInterval)
												WGLSwapInterval(isVSync);
										}

										if (isValid)
										{
											if (config.fpsCounter)
											{
												if (isFpsChanged)
												{
													isFpsChanged = FALSE;
													fpsCounter->Reset();
												}

												fpsCounter->Calculate();
											}

											this->CheckView();

											if (this->isStateChanged)
											{
												this->isStateChanged = FALSE;
												UseShaderProgram(config.windowedMode && config.filtering ? &shaders.linear : &shaders.nearest, maxTexSize);
											}

											if (this->isPalChanged)
											{
												this->isPalChanged = FALSE;
												pixelBuffer->Reset();

												GLActiveTexture(GL_TEXTURE1);
												GLBindTexture(GL_TEXTURE_1D, paletteId);
												GLTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA, GL_UNSIGNED_BYTE, this->palette);

												GLActiveTexture(GL_TEXTURE0);
												GLBindTexture(GL_TEXTURE_2D, indicesId);
											}

											BOOL isDraw = FALSE;
											isDraw = pixelBuffer->Update(this->indexBuffer);

											if (config.fpsCounter)
											{
												isDraw = TRUE;

												DWORD fps = fpsCounter->value;
												DWORD digCount = 0;
												DWORD current = fps;
												do
												{
													++digCount;
													current = current / 10;
												} while (current);

												DWORD slice = texPitch - FPS_WIDTH;
												DWORD dcount = digCount;
												current = fps;
												do
												{
													WORD* lpDig = (WORD*)counters[current % 10];

													BYTE* idx = this->indexBuffer + FPS_Y * texPitch + FPS_X + FPS_WIDTH * (dcount - 1);
													BYTE* pix = (BYTE*)pixelBuffer->primaryBuffer + FPS_WIDTH * (dcount - 1);

													DWORD height = FPS_HEIGHT;
													do
													{
														WORD check = *lpDig++;
														DWORD width = FPS_WIDTH;
														do
														{
															*pix++ = (check & 1) ? 0xFF : *idx;
															++idx;
															check >>= 1;
														} while (--width);

														idx += slice;
														pix += slice;
													} while (--height);

													current = current / 10;
												} while (--dcount);

												dcount = 4;
												while (dcount != digCount)
												{
													BYTE* idx = this->indexBuffer + FPS_Y * texPitch + FPS_X + FPS_WIDTH * (dcount - 1);
													BYTE* pix = (BYTE*)pixelBuffer->primaryBuffer + FPS_WIDTH * (dcount - 1);

													DWORD height = FPS_HEIGHT;
													do
													{
														DWORD width = FPS_WIDTH;
														do
															*pix++ = *idx++;
														while (--width);

														idx += slice;
														pix += slice;
													} while (--height);

													--dcount;
												}

												GLTexSubImage2D(GL_TEXTURE_2D, 0, FPS_X, FPS_Y, FPS_WIDTH * 4, FPS_HEIGHT, GL_ALPHA, GL_UNSIGNED_BYTE, pixelBuffer->primaryBuffer);
											}

											if (isDraw)
											{
												GLDrawArrays(GL_TRIANGLE_FAN, 0, 4);
												SwapBuffers(this->hDc);
												GLFinish();
											}

											if (this->isTakeSnapshot)
											{
												this->isTakeSnapshot = FALSE;
												this->TakeScreenshot();
											}
										}

										Sleep(0);
									} while (!this->isFinish);
									GLFinish();
								}
								GLPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
							}
							delete pixelBuffer;
						}
						GLDeleteTextures(2, textures);
					}
					else
					{
						GLuint textureId;
						GLGenTextures(1, &textureId);
						{
							GLActiveTexture(GL_TEXTURE0);
							GLBindTexture(GL_TEXTURE_2D, textureId);

							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glCapsClampToEdge);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glCapsClampToEdge);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

							GLint filter = config.windowedMode && config.filtering ? GL_LINEAR : GL_NEAREST;
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
							GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);

							GLTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, maxTexSize, maxTexSize, GL_NONE, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

							UseShaderProgram(&shaders.simple, maxTexSize);

							do
							{
								if (mciVideo.deviceId)
								{
									Sleep(1);

									if (this->isTakeSnapshot)
										this->isTakeSnapshot = FALSE;
								}
								else
								{
									BOOL isValid = TRUE;
									if (config.singleThread)
									{
										LONGLONG qp;
										QueryPerformanceFrequency((LARGE_INTEGER*)&qp);
										DOUBLE timerResolution = (DOUBLE)qp;

										QueryPerformanceCounter((LARGE_INTEGER*)&qp);
										DOUBLE currTime = (DOUBLE)qp / timerResolution;

										if (currTime >= nextSyncTime)
											nextSyncTime += fpsSync * (1.0f + (FLOAT)DWORD((currTime - nextSyncTime) / fpsSync));
										else
											isValid = FALSE;
									}
									else if (isVSync != config.vSync)
									{
										isVSync = config.vSync;
										if (WGLSwapInterval)
											WGLSwapInterval(isVSync);
									}

									if (isValid)
									{
										this->CheckView();

										if (this->isStateChanged)
										{
											this->isStateChanged = FALSE;

											GLint filter = config.windowedMode && config.filtering ? GL_LINEAR : GL_NEAREST;
											GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
											GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
										}

										GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texPitch, this->dwMode->height, GL_RGBA, GL_UNSIGNED_BYTE, this->indexBuffer);

										GLDrawArrays(GL_TRIANGLE_FAN, 0, 4);
										SwapBuffers(this->hDc);
										GLFinish();

										if (this->isTakeSnapshot)
										{
											this->isTakeSnapshot = FALSE;
											this->TakeScreenshot();
										}
									}

									Sleep(0);
								}
							} while (!this->isFinish);
							GLFinish();
						}
						GLDeleteTextures(1, &textureId);
					}
				}
				delete fpsCounter;
			}
			GLBindBuffer(GL_ARRAY_BUFFER, NULL);
		}
		GLDeleteBuffers(1, &bufferName);

		if (glVersion >= GL_VER_3_0)
		{
			GLBindVertexArray(NULL);
			GLDeleteVertexArrays(1, &arrayName);
		}
	}
	GLUseProgram(NULL);

	ShaderProgram* program = (ShaderProgram*)&shaders;
	DWORD count = sizeof(shaders) / sizeof(ShaderProgram);
	do
	{
		if (program->id)
			GLDeleteProgram(program->id);

		++program;
	} while (--count);
}

VOID DirectDraw::CalcView()
{
	this->viewport.rectangle.x = this->viewport.rectangle.y = 0;
	this->viewport.point.x = this->viewport.point.y = 0.0f;

	this->viewport.rectangle.width = this->viewport.width;
	this->viewport.rectangle.height = this->viewport.height;

	this->viewport.clipFactor.x = this->viewport.viewFactor.x = (FLOAT)this->viewport.width / this->dwMode->width;
	this->viewport.clipFactor.y = this->viewport.viewFactor.y = (FLOAT)this->viewport.height / this->dwMode->height;

	if (config.aspectRatio && this->viewport.viewFactor.x != this->viewport.viewFactor.y)
	{
		if (this->viewport.viewFactor.x > this->viewport.viewFactor.y)
		{
			FLOAT fw = this->viewport.viewFactor.y * this->dwMode->width;
			this->viewport.rectangle.width = (DWORD)MathRound(fw);

			this->viewport.point.x = ((FLOAT)this->viewport.width - fw) / 2.0f;
			this->viewport.rectangle.x = (DWORD)MathRound(this->viewport.point.x);

			this->viewport.clipFactor.x = this->viewport.viewFactor.y;
		}
		else
		{
			FLOAT fh = this->viewport.viewFactor.x * this->dwMode->height;
			this->viewport.rectangle.height = (DWORD)MathRound(fh);

			this->viewport.point.y = ((FLOAT)this->viewport.height - fh) / 2.0f;
			this->viewport.rectangle.y = (DWORD)MathRound(this->viewport.point.y);

			this->viewport.clipFactor.y = this->viewport.viewFactor.x;
		}
	}
}

VOID DirectDraw::CheckView()
{
	if (this->viewport.refresh)
	{
		this->viewport.refresh = FALSE;
		this->CalcView();
		GLViewport(this->viewport.rectangle.x, this->viewport.rectangle.y, this->viewport.rectangle.width, this->viewport.rectangle.height);

		this->clearStage = 0;
	}

	if (++this->clearStage <= 2)
		GLClear(GL_COLOR_BUFFER_BIT);
}

VOID DirectDraw::CaptureMouse(UINT uMsg, LPMSLLHOOKSTRUCT mInfo)
{
	if (config.windowedMode)
	{
		switch (uMsg)
		{
		case WM_MOUSEMOVE:
		{
			this->HookMouse(uMsg, mInfo);
			break;
		}

		case WM_LBUTTONUP:
		{
			if (this->mbPressed & MK_LBUTTON)
				this->HookMouse(uMsg, mInfo);

			break;
		}

		case WM_RBUTTONUP:
		{
			if (this->mbPressed & MK_RBUTTON)
				this->HookMouse(uMsg, mInfo);

			break;
		}

		default:
			break;
		}
	}
}

VOID DirectDraw::HookMouse(UINT uMsg, LPMSLLHOOKSTRUCT mInfo)
{
	POINT point = mInfo->pt;
	ScreenToClient(this->hWnd, &point);
	RECT rect;
	GetClientRect(hWnd, &rect);

	if (point.x < rect.left || point.x > rect.right || point.y < rect.top || point.y > rect.bottom)
	{
		if (point.x < rect.left)
			point.x = rect.left;
		else if (point.x > rect.right)
			point.x = rect.right;

		if (point.y < rect.top)
			point.y = rect.top;
		else if (point.y > rect.bottom)
			point.y = rect.bottom;

		SendMessage(this->hWnd, uMsg, this->mbPressed, MAKELONG(point.x, point.y));
	}
}

VOID DirectDraw::ScaleMouse(UINT uMsg, LPARAM* lParam)
{
	if (config.windowedMode)
	{
		INT xPos = GET_X_LPARAM(*lParam);
		INT yPos = GET_Y_LPARAM(*lParam);

		if (xPos < this->viewport.rectangle.x)
			xPos = 0;
		else if (xPos >= this->viewport.rectangle.x + this->viewport.rectangle.width)
			xPos = this->dwMode->width - 1;
		else
			xPos = (INT)((FLOAT)(xPos - this->viewport.rectangle.x) / this->viewport.clipFactor.x);

		if (yPos < this->viewport.rectangle.y)
			yPos = 0;
		else if (yPos >= this->viewport.rectangle.y + this->viewport.rectangle.height)
			yPos = this->dwMode->height - 1;
		else
			yPos = (INT)((FLOAT)(yPos - this->viewport.rectangle.y) / this->viewport.clipFactor.y);

		*lParam = MAKELONG(xPos, yPos);
	}
}

#pragma optimize("", on)

VOID DirectDraw::TakeScreenshot()
{
	if (OpenClipboard(NULL))
	{
		EmptyClipboard();

		DWORD dataCount = this->viewport.rectangle.width * this->viewport.rectangle.height;
		DWORD dataSize = dataCount * 3;
		HGLOBAL hMemory = GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + dataSize);
		{
			VOID* data = GlobalLock(hMemory);
			{
				MemoryZero(data, sizeof(BITMAPINFOHEADER));

				BITMAPINFOHEADER* bmi = (BITMAPINFOHEADER*)data;
				bmi->biSize = sizeof(BITMAPINFOHEADER);
				bmi->biWidth = this->viewport.rectangle.width;
				bmi->biHeight = this->viewport.rectangle.height;
				bmi->biPlanes = 1;
				bmi->biBitCount = 24;
				bmi->biCompression = BI_RGB;
				bmi->biXPelsPerMeter = 1;
				bmi->biYPelsPerMeter = 1;

				BYTE* dst = (BYTE*)data + sizeof(BITMAPINFOHEADER);
				GLReadPixels(this->viewport.rectangle.x, this->viewport.rectangle.y, this->viewport.rectangle.width, this->viewport.rectangle.height, glCapsBGR ? GL_BGR_EXT : GL_RGB, GL_UNSIGNED_BYTE, dst);
				if (!glCapsBGR)
				{
					do
					{
						BYTE val = *dst;
						*dst = *(dst + 2);
						*(dst + 2) = val;
						dst += 3;
					} while (--dataCount);
				}
			}
			GlobalUnlock(hMemory);

			SetClipboardData(CF_DIB, hMemory);
			MessageBeep(1);
		}
		GlobalFree(hMemory);

		CloseClipboard();
	}
}

VOID DirectDraw::RenderStart()
{
	if (!this->isFinish || !this->hWnd)
		return;

	this->isFinish = FALSE;

	RECT rect;
	GetClientRect(this->hWnd, &rect);

	if (config.singleWindow)
		this->hDraw = this->hWnd;
	else
	{
		if (!config.windowedMode)
		{
			this->hDraw = CreateWindowEx(
				WS_EX_CONTROLPARENT | WS_EX_TOPMOST,
				WC_DRAW,
				NULL,
				mciVideo.deviceId ? WS_POPUP : (WS_VISIBLE | WS_POPUP),
				0, 0,
				rect.right, rect.bottom,
				this->hWnd,
				NULL,
				hDllModule,
				NULL);
		}
		else
		{
			this->hDraw = CreateWindowEx(
				WS_EX_CONTROLPARENT,
				WC_DRAW,
				NULL,
				mciVideo.deviceId ? (WS_CHILD | WS_MAXIMIZE) : (WS_VISIBLE | WS_CHILD | WS_MAXIMIZE),
				0, 0,
				rect.right, rect.bottom,
				this->hWnd,
				NULL,
				hDllModule,
				NULL);
		}

		Window::SetCapturePanel(this->hDraw);

		SetClassLongPtr(this->hDraw, GCLP_HBRBACKGROUND, NULL);
		RedrawWindow(this->hDraw, NULL, NULL, RDW_INVALIDATE);
	}

	SetClassLongPtr(this->hWnd, GCLP_HBRBACKGROUND, NULL);
	RedrawWindow(this->hWnd, NULL, NULL, RDW_INVALIDATE);

	this->viewport.width = rect.right;
	this->viewport.height = rect.bottom;
	this->viewport.refresh = TRUE;

	DWORD threadId;
	SECURITY_ATTRIBUTES sAttribs;
	MemoryZero(&sAttribs, sizeof(SECURITY_ATTRIBUTES));
	sAttribs.nLength = sizeof(SECURITY_ATTRIBUTES);
	this->hDrawThread = CreateThread(&sAttribs, NULL, RenderThread, this, NORMAL_PRIORITY_CLASS, &threadId);

	WaitForSingleObject(this->hCheckEvent, INFINITE);
	Window::CheckMenu();
}

VOID DirectDraw::RenderStop()
{
	if (this->isFinish)
		return;

	this->isFinish = TRUE;
	WaitForSingleObject(this->hDrawThread, INFINITE);
	CloseHandle(this->hDrawThread);
	this->hDrawThread = NULL;

	if (this->hDraw != this->hWnd)
	{
		DestroyWindow(this->hDraw);
		GL::ResetPixelFormat();
	}

	this->hDraw = NULL;

	glVersion = NULL;
	Window::CheckMenu();
}

DirectDraw::DirectDraw(IDrawUnknown** list)
	: IDraw(list)
{
	this->surfaceEntries = NULL;
	this->paletteEntries = NULL;

	this->attachedSurface = NULL;

	this->dwMode = NULL;
	this->pitch = 0;

	this->indexBuffer = NULL;
	this->palette = NULL;

	this->hWnd = NULL;
	this->hDraw = NULL;
	this->hDc = NULL;

	this->isFinish = TRUE;
	this->isTakeSnapshot = FALSE;

	MemoryZero(&this->windowPlacement, sizeof(WINDOWPLACEMENT));

	this->hCheckEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

DirectDraw::~DirectDraw()
{
	this->RenderStop();

	if (this->indexBuffer)
		AlignedFree(this->indexBuffer);

	if (this->palette)
		AlignedFree(this->palette);

	CloseHandle(this->hCheckEvent);
}

HRESULT DirectDraw::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
	*ppvObj = new IDrawUnknown(NULL);
	return DD_OK;
}

ULONG DirectDraw::Release()
{
	if (--this->refCount)
		return this->refCount;

	delete this;
}

HRESULT DirectDraw::EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback)
{
	MemoryZero(resolutionsList, sizeof(resolutionsList));

	DEVMODE devMode;
	MemoryZero(&devMode, sizeof(DEVMODE));
	devMode.dmSize = sizeof(DEVMODE);

	DWORD count = 0;
	for (DWORD i = 0; EnumDisplaySettings(NULL, i, &devMode); ++i)
	{
		if ((devMode.dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL)) == (DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL) && devMode.dmPelsWidth >= 640 && devMode.dmPelsHeight >= 480)
		{
			DWORD idx;

			if (devMode.dmBitsPerPel != 8 && (devMode.dmPelsWidth == 1024 && devMode.dmPelsHeight == 768 || devMode.dmPelsWidth == 800 && devMode.dmPelsHeight == 600 || devMode.dmPelsWidth == 640 && devMode.dmPelsHeight == 480))
			{
				idx = AddDisplayMode(&devMode);
				if (idx)
				{
					if (count < idx)
						count = idx;
				}
				else
					break;
			}

			if (devMode.dmPelsWidth >= 800 && devMode.dmPelsHeight >= 600)
			{
				devMode.dmBitsPerPel = 8;
				idx = AddDisplayMode(&devMode);
				if (idx)
				{
					if (count < idx)
						count = idx;
				}
				else
					break;
			}
		}

		MemoryZero(&devMode, sizeof(DEVMODE));
		devMode.dmSize = sizeof(DEVMODE);
	}

	DisplayMode min, max;
	min.frequency = min.bpp = min.height = min.width = 0x00000000;
	max.frequency = max.bpp = max.height = max.width = 0xFFFFFFFF;

	DisplayMode* mode = &max;
	DWORD modesCount = count;
	while (modesCount--)
	{
		DisplayMode* stored = &min;
		DisplayMode* check = resolutionsList;
		DWORD checksCount = count;
		while (checksCount--)
		{
			if ((check->width < mode->width || check->width == mode->width && (check->height < mode->height || check->height == mode->height && (check->bpp < mode->bpp || check->bpp == mode->bpp && check->frequency < mode->frequency)))
				&& (check->width > stored->width || check->width == stored->width && (check->height > stored->height || check->height == stored->height && (check->bpp > stored->bpp || check->bpp == stored->bpp && check->frequency > stored->frequency))))
				stored = check;

			++check;
		}

		mode = stored;

		if (mode->bpp == 8)
		{
			DDSURFACEDESC ddSurfaceDesc;
			ddSurfaceDesc.dwWidth = mode->width;
			ddSurfaceDesc.dwHeight = mode->height;
			ddSurfaceDesc.ddpfPixelFormat.dwRGBBitCount = mode->bpp;
			ddSurfaceDesc.dwRefreshRate = mode->frequency;

			if (!lpEnumModesCallback(&ddSurfaceDesc, NULL))
				break;
		}
	}

	return DD_OK;
}

HRESULT DirectDraw::SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP)
{
	this->dwMode = NULL;

	DisplayMode* resList = resolutionsList;
	for (DWORD i = 0; i < RECOUNT; ++i, ++resList)
	{
		if (!resList->width)
			return DDERR_INVALIDMODE;

		if (resList->width == dwWidth && resList->height == dwHeight && resList->bpp == dwBPP)
		{
			this->dwMode = resList;
			break;
		}
	}

	this->pitch = dwWidth * dwBPP >> 3;
	if (this->pitch % 16)
		this->pitch = (this->pitch + 16) & 0xFFFFFFF0;

	if (this->indexBuffer)
		AlignedFree(this->indexBuffer);

	if (this->indexBuffer)
		AlignedFree(this->indexBuffer);

	this->indexBuffer = (BYTE*)AlignedAlloc(dwWidth * pitch * (dwBPP >> 3)); // pitch * (dwBPP >> 3) - for EU video fix
	MemoryZero(this->indexBuffer, dwWidth * pitch);

	if (!this->palette)
		this->palette = (DWORD*)AlignedAlloc(256 * sizeof(DWORD));

	MemoryZero(this->palette, 255 * sizeof(DWORD));
	this->palette[255] = WHITE;

	if (config.windowedMode)
	{
		if (!(GetWindowLong(this->hWnd, GWL_STYLE) & WS_BORDER))
			this->SetWindowedMode();
		else
			Window::SetCaptureMouse(TRUE);
	}
	else
		this->SetFullscreenMode();

	this->RenderStart();

	return DD_OK;
}

VOID DirectDraw::SetFullscreenMode()
{
	DEVMODE devMode;
	MemoryZero(&devMode, sizeof(DEVMODE));
	devMode.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devMode);

	devMode.dmPelsWidth = this->dwMode->width;
	devMode.dmPelsHeight = this->dwMode->height;
	devMode.dmFields |= (DM_PELSWIDTH | DM_PELSHEIGHT);

	if (this->dwMode->frequency)
	{
		devMode.dmDisplayFrequency = this->dwMode->frequency;
		devMode.dmFields |= DM_DISPLAYFREQUENCY;
	}

	if (ChangeDisplaySettingsEx(NULL, &devMode, NULL, CDS_FULLSCREEN | CDS_TEST | CDS_RESET, NULL) == DISP_CHANGE_SUCCESSFUL)
	{
		if (config.windowedMode)
			GetWindowPlacement(hWnd, &this->windowPlacement);

		if (GetMenu(hWnd))
			SetMenu(hWnd, NULL);

		SetWindowLong(this->hWnd, GWL_STYLE, FS_STYLE);
		SetWindowPos(this->hWnd, NULL, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL);
		SetForegroundWindow(this->hWnd);

		Window::SetCaptureMouse(FALSE);

		if (ChangeDisplaySettingsEx(NULL, &devMode, NULL, CDS_FULLSCREEN | CDS_RESET, NULL) == DISP_CHANGE_SUCCESSFUL)
		{
			RECT rc;
			GetWindowRect(this->hWnd, &rc);
			if (rc.right - rc.left != devMode.dmPelsWidth || rc.bottom - rc.top != devMode.dmPelsHeight)
			{
				SetWindowPos(this->hWnd, NULL, 0, 0, devMode.dmPelsWidth, devMode.dmPelsHeight, NULL);
				SetForegroundWindow(this->hWnd);
			}
		}
	}
}

VOID DirectDraw::SetWindowedMode()
{
	ChangeDisplaySettings(NULL, NULL);

	Window::SetCaptureMouse(TRUE);

	if (!this->windowPlacement.length)
	{
		this->windowPlacement.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(hWnd, &this->windowPlacement);

		INT monWidth = GetSystemMetrics(SM_CXSCREEN);
		INT monHeight = GetSystemMetrics(SM_CYSCREEN);

		INT newWidth = (INT)MathRound(0.75f * monWidth);
		INT newHeight = (INT)MathRound(0.75f * monHeight);

		FLOAT k = (FLOAT)this->dwMode->width / this->dwMode->height;

		INT check = (INT)MathRound((FLOAT)newHeight * k);
		if (newWidth > check)
			newWidth = check;
		else
			newHeight = (INT)MathRound((FLOAT)newWidth / k);

		RECT* rect = &this->windowPlacement.rcNormalPosition;
		rect->left = (monWidth - newWidth) >> 1;
		rect->top = (monHeight - newHeight) >> 1;
		rect->right = rect->left + newWidth;
		rect->bottom = rect->top + newHeight;
		AdjustWindowRect(rect, WIN_STYLE, TRUE);

		this->windowPlacement.ptMinPosition.x = this->windowPlacement.ptMinPosition.y = -1;
		this->windowPlacement.ptMaxPosition.x = this->windowPlacement.ptMaxPosition.y = -1;

		this->windowPlacement.flags = NULL;
		this->windowPlacement.showCmd = SW_SHOWNORMAL;
	}

	if (!GetMenu(hWnd))
		SetMenu(hWnd, config.menu);

	SetWindowLong(this->hWnd, GWL_STYLE, WIN_STYLE);
	SetWindowPlacement(this->hWnd, &this->windowPlacement);
}

HRESULT DirectDraw::SetCooperativeLevel(HWND hWnd, DWORD dwFlags)
{
	if (hWnd && hWnd != this->hWnd)
	{
		this->hWnd = hWnd;
		this->hDc = NULL;
		this->mbPressed = NULL;
	}

	return DD_OK;
}

HRESULT DirectDraw::CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, IDrawPalette** lplpDDPalette, IUnknown* pUnkOuter)
{
	*lplpDDPalette = new DirectDrawPalette((IDrawUnknown**)&this->paletteEntries, this);
	MemoryCopy(this->palette, lpDDColorArray, 255 * sizeof(PALETTEENTRY));
	this->isPalChanged = TRUE;

	return DD_OK;
}

HRESULT DirectDraw::CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc, IDrawSurface** lplpDDSurface, IUnknown* pUnkOuter)
{
	if (this->dwMode)
	{
		lpDDSurfaceDesc->dwWidth = this->dwMode->width;
		lpDDSurfaceDesc->dwHeight = this->dwMode->height;
		lpDDSurfaceDesc->lPitch = this->pitch;
	}

	this->attachedSurface = new DirectDrawSurface((IDrawUnknown**)&this->surfaceEntries, this);
	*lplpDDSurface = this->attachedSurface;

	return DD_OK;
}