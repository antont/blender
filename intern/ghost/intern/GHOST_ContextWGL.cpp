/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2013 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Jason Wilkins
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file ghost/intern/GHOST_ContextWGL.cpp
 *  \ingroup GHOST
 * Definition of GHOST_ContextWGL class.
 */

#include "GHOST_ContextWGL.h"

#include <tchar.h>

#include <cstdio>
#include <cassert>
#include <vector>



WGLEWContext* wglewContext = NULL;



HGLRC GHOST_ContextWGL::s_sharedHGLRC = NULL;
int   GHOST_ContextWGL::s_sharedCount = 0;

bool GHOST_ContextWGL::s_singleContextMode = false;



/* Intel videocards don't work fine with multiple contexts and
 * have to share the same context for all windows.
 * But if we just share context for all windows it could work incorrect
 * with multiple videocards configuration. Suppose, that Intel videocards
 * can't be in multiple-devices configuration. */
static bool is_crappy_intel_card()
{
	return strstr((const char*)glGetString(GL_VENDOR), "Intel") != NULL;
}



bool win32_chk(bool result, const char* file = NULL, int line = 0, const char* text = NULL)
{
	if (!result) {
		LPTSTR formattedMsg = NULL;

		DWORD error = GetLastError();

		const char* msg;

		DWORD count = 0;

		switch (error) {
			case ERROR_INVALID_VERSION_ARB:
				msg = "The specified OpenGL version and feature set are either invalid or not supported.\n";
				break;

			case ERROR_INVALID_PROFILE_ARB:
				msg = "The specified OpenGL profile and feature set are either invalid or not supported.\n";
				break;

			case ERROR_INVALID_PIXEL_TYPE_ARB:
				msg = "The specified pixel type is invalid.\n";
				break;

			case ERROR_INCOMPATIBLE_DEVICE_CONTEXTS_ARB:
				msg = "The device contexts specified are not compatible.  This can occur if the device contexts are managed by different drivers or possibly on different graphics adapters.\n";
				break;

			case ERROR_INCOMPATIBLE_AFFINITY_MASKS_NV:
				msg = "The device context(s) and rendering context have non-matching affinity masks.\n";
				break;

			case ERROR_MISSING_AFFINITY_MASK_NV:
				msg = "The rendering context does not have an affinity mask set.\n";
				break;

			case ERROR_PROFILE_DOES_NOT_MATCH_DEVICE:
				msg = "The specified profile is intended for a device of a different type than the specified device.\n";
				break;

			default:
			{
				count =
					FormatMessage(
						(FORMAT_MESSAGE_ALLOCATE_BUFFER |
						 FORMAT_MESSAGE_FROM_SYSTEM     |
						 FORMAT_MESSAGE_IGNORE_INSERTS),
						NULL,
						error,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(LPTSTR)(&formattedMsg),
						0,
						NULL);

				msg = count > 0 ? formattedMsg : "<no system message>\n";
				break;
			}
		}

#ifndef NDEBUG
		_ftprintf(
			stderr,
			"%s(%d):[%s] -> Win32 Error# (%d): %s",
			file, 
			line,
			text,
			error,
			msg);
#else
		_ftprintf(
			stderr,
			"Win32 Error# (%d): %s",
			error,
			msg);
#endif

		SetLastError(NO_ERROR);

		if (count != 0)
			LocalFree(formattedMsg);
	}

	return result;
}

#ifndef NDEBUG
#define WIN32_CHK(x) win32_chk((x), __FILE__, __LINE__, #x)
#else
#define WIN32_CHK(x) win32_chk(x)
#endif



class GHOST_PixelFormatPFD : public GHOST_PixelFormat {
public:
	GHOST_PixelFormatPFD(HDC hDC, int i)
	{
		int check = ::DescribePixelFormat(hDC, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
		WIN32_CHK(check != 0);
	}

	virtual bool isUsable() const
	{
		const DWORD love = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER|PFD_TYPE_RGBA;
		const DWORD hate = PFD_GENERIC_FORMAT;

		return (love & pfd.dwFlags) == love  &&  !(hate & pfd.dwFlags);
	}

	virtual swap_t swapMethod() const
	{
		if (pfd.dwFlags & PFD_SWAP_COPY)
			return COPY;
		else if (pfd.dwFlags & PFD_SWAP_EXCHANGE)
			return EXCHANGE;
		else
			return UNKNOWN;
	}

	virtual int    colorBits()    const { return pfd.cColorBits;   }
	virtual int    alphaBits()    const { return pfd.cAlphaBits;   }
	virtual int    depthBits()    const { return pfd.cDepthBits;   }
	virtual int    stencilBits()  const { return pfd.cStencilBits; }
	virtual int    samples()      const { return 0;                }
	virtual bool   sRGB()         const { return false;            }

private:
	PIXELFORMATDESCRIPTOR pfd;
};



class GHOST_PixelFormatARB : public GHOST_PixelFormat {
public:
	GHOST_PixelFormatARB(HDC hDC, int iPixelFormat)
	{
		valid = ::wglGetPixelFormatAttribivARB(hDC, iPixelFormat, 0, nAttributes, iAttributes, value);

		WIN32_CHK(valid);

		if (WGLEW_ARB_multisample) {
			int attribute = WGL_SAMPLES_ARB;

			BOOL check = ::wglGetPixelFormatAttribivARB(hDC, iPixelFormat, 0, 1, &attribute, &_samples);

			if (!check)
				_samples = 0;
		}
		else {
			_samples = 0;
		}

		if (WGL_ARB_framebuffer_sRGB) {
			int attribute = WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB;

			BOOL check = ::wglGetPixelFormatAttribivARB(hDC, iPixelFormat, 0, 1, &attribute, &_sRGB);

			if (!check)
				_sRGB = false;
		}
		else {
			_sRGB = false;
		}
	}

	virtual bool   isUsable()     const
	{
		return
			valid                                 &&
			value[0] /* draw to window */         &&
			value[1] == WGL_FULL_ACCELERATION_ARB &&
			value[3] /* support opengl */         &&
			value[4] /* double buffer  */         &&
			value[6] == WGL_TYPE_RGBA_ARB;
	}

	virtual swap_t swapMethod() const
	{
		switch (value[2]) {
			case WGL_SWAP_EXCHANGE_ARB:
				return EXCHANGE;

			case WGL_SWAP_COPY_ARB:
				return COPY;

			case WGL_SWAP_UNDEFINED_ARB:
				return UNDEFINED;

			default:
				return UNKNOWN;
		}
	}

	virtual int    colorBits()    const { return value[ 7]; }
	virtual int    alphaBits()    const { return value[ 8]; }
	virtual int    depthBits()    const { return value[ 9]; }
	virtual int    stencilBits()  const { return value[10]; }
	virtual int    samples()      const { return _samples;  }
	virtual bool   sRGB()         const { return _sRGB;     }

private:
	static const int nAttributes = 11;
	static const int iAttributes[nAttributes];

	bool valid;
	int value[nAttributes];
	int _samples;
	int _sRGB;
};

const int GHOST_PixelFormatARB::iAttributes[nAttributes] = {
	WGL_DRAW_TO_WINDOW_ARB,
	WGL_ACCELERATION_ARB,
	WGL_SWAP_METHOD_ARB,
	WGL_SUPPORT_OPENGL_ARB,
	WGL_DOUBLE_BUFFER_ARB,
	WGL_STEREO_ARB,
	WGL_PIXEL_TYPE_ARB,
	WGL_COLOR_BITS_ARB,
	WGL_ALPHA_BITS_ARB,
	WGL_DEPTH_BITS_ARB,
	WGL_STENCIL_BITS_ARB,
};




GHOST_ContextWGL::GHOST_ContextWGL(
	HWND hWnd,
	HDC  hDC,
	int  contextProfileMask,
	int  contextMajorVersion,
	int  contextMinorVersion,
	int  contextFlags
)
	: m_hWnd(hWnd)
	, m_hDC(hDC)
	, m_contextProfileMask(contextProfileMask)
	, m_contextMajorVersion(contextMajorVersion)
	, m_contextMinorVersion(contextMinorVersion)
	, m_contextFlags(contextFlags)
	, m_hGLRC(NULL)
	, m_needSetPixelFormat(true)
	, m_wglewContext(NULL)
#ifndef NDEBUG
	, m_dummyVendor(NULL)
	, m_dummyRenderer(NULL)
	, m_dummyVersion(NULL)
#endif
{
	assert(m_hWnd);
	assert(m_hDC);

	assert((m_contextProfileMask & ~(WGL_CONTEXT_CORE_PROFILE_BIT_ARB|WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB|WGL_CONTEXT_ES_PROFILE_BIT_EXT|WGL_CONTEXT_ES2_PROFILE_BIT_EXT)) == 0);
}



GHOST_ContextWGL::~GHOST_ContextWGL()
{
	if (m_hGLRC != NULL) {
		if (m_hGLRC == ::wglGetCurrentContext())
			WIN32_CHK(::wglMakeCurrent(NULL, NULL));

		if (m_hGLRC != s_sharedHGLRC || s_sharedCount == 1) {
			assert(s_sharedCount > 0);

			s_sharedCount--;

			if (s_sharedCount == 0)
				s_sharedHGLRC = NULL;

			WIN32_CHK(::wglDeleteContext(m_hGLRC));
		}
	}

	delete m_wglewContext;
}



GHOST_TSuccess GHOST_ContextWGL::swapBuffers()
{
	return WIN32_CHK(::SwapBuffers(m_hDC)) ? GHOST_kSuccess : GHOST_kFailure;
}



GHOST_TSuccess GHOST_ContextWGL::activateDrawingContext()
{
	activateGLEW();

	return WIN32_CHK(::wglMakeCurrent(m_hDC, m_hGLRC)) ? GHOST_kSuccess : GHOST_kFailure;
}



/* Ron Fosner's code for weighting pixel formats and forcing software.
 * See http://www.opengl.org/resources/faq/technical/weight.cpp */

static int weight_pixel_format(PIXELFORMATDESCRIPTOR& pfd)
{
	int weight = 0;

	/* assume desktop color depth is 32 bits per pixel */

	/* cull unusable pixel formats */
	/* if no formats can be found, can we determine why it was rejected? */
	if (!(pfd.dwFlags & PFD_SUPPORT_OPENGL)  ||
	    !(pfd.dwFlags & PFD_DRAW_TO_WINDOW)  ||
	    !(pfd.dwFlags & PFD_DOUBLEBUFFER)    || /* Blender _needs_ this */
	    !(pfd.iPixelType == PFD_TYPE_RGBA)   ||
	     (pfd.cDepthBits < 16)               ||
	     (pfd.dwFlags & PFD_GENERIC_FORMAT))    /* no software renderers */
	{
		return 0;
	}

	weight = 1;  /* it's usable */

	/* the bigger the depth buffer the better */
	/* give no weight to a 16-bit depth buffer, because those are crap */
	weight += pfd.cDepthBits - 16;

	weight += pfd.cColorBits -  8;

#ifdef GHOST_OPENGL_ALPHA
	if (pfd.cAlphaBits > 0)
		weight++;
#endif

#ifdef GHOST_OPENGL_STENCIL
	if (pfd.cStencilBits >= 8)
		weight++;
#endif

	/* want swap copy capability -- it matters a lot */
	if (pfd.dwFlags & PFD_SWAP_COPY)
		weight += 16;

	return weight;
}



/*
 * A modification of Ron Fosner's replacement for ChoosePixelFormat
 * returns 0 on error, else returns the pixel format number to be used
 */
static int choose_pixel_format_legacy(HDC hDC, PIXELFORMATDESCRIPTOR& preferredPFD)
{
	int iPixelFormat = 0;
	int weight = 0;

	int iStereoPixelFormat = 0;
	int stereoWeight = 0;

	/* choose a pixel format using the useless Windows function in case we come up empty handed */
	int iLastResortPixelFormat = ::ChoosePixelFormat(hDC, &preferredPFD);

	WIN32_CHK(iLastResortPixelFormat != 0);

	int lastPFD = ::DescribePixelFormat(hDC, 1, sizeof(PIXELFORMATDESCRIPTOR), NULL);

	WIN32_CHK(lastPFD != 0);

	for (int i = 1; i <= lastPFD; i++) {
		PIXELFORMATDESCRIPTOR pfd;
		int check = ::DescribePixelFormat(hDC, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

		WIN32_CHK(check == lastPFD);

		int w = weight_pixel_format(pfd);

		if (w > weight) {
			weight = w;
			iPixelFormat = i;
		}

		if (w > stereoWeight && (preferredPFD.dwFlags & pfd.dwFlags & PFD_STEREO)) {
			stereoWeight = w;
			iStereoPixelFormat = i;
		}
	}

	/* choose any available stereo format over a non-stereo format */
	if (iStereoPixelFormat != 0)
		iPixelFormat = iStereoPixelFormat;

	if (iPixelFormat == 0) {
		fprintf(stderr, "Warning! Using result of ChoosePixelFormat.\n");
		iPixelFormat = iLastResortPixelFormat;
	}

	return iPixelFormat;
}



/*
 * Clone a window for the purpose of creating a temporary context to initialize WGL extensions.
 * There is no generic way to clone the lpParam parameter, so the caller is responsible for cloning it themselves.
 */

static HWND clone_window(HWND hWnd, LPVOID lpParam)
{
	int count;

	SetLastError(NO_ERROR);

	DWORD dwExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
	WIN32_CHK(GetLastError() == NO_ERROR);

	WCHAR lpClassName[100] = L"";
	count = GetClassNameW(hWnd, lpClassName, sizeof(lpClassName));
	WIN32_CHK(count != 0);

	WCHAR lpWindowName[100] = L"";
	count = GetWindowTextW(hWnd, lpWindowName, sizeof(lpWindowName));
	WIN32_CHK(count != 0);

	DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
	WIN32_CHK(GetLastError() == NO_ERROR);

	RECT rect;
	GetWindowRect(hWnd, &rect);
	WIN32_CHK(GetLastError() == NO_ERROR);

	HWND hWndParent = (HWND)GetWindowLong(hWnd, GWL_HWNDPARENT);
	WIN32_CHK(GetLastError() == NO_ERROR);

	HMENU hMenu = GetMenu(hWnd);
	WIN32_CHK(GetLastError() == NO_ERROR);

	HINSTANCE hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
	WIN32_CHK(GetLastError() == NO_ERROR);

	HWND hwndCloned =
		CreateWindowExW(
			dwExStyle,
			lpClassName,
			lpWindowName,
			dwStyle,
			rect.left,
			rect.top,
			rect.right - rect.left,
			rect.bottom - rect.top,
			hWndParent,
			hMenu,
			hInstance,
			lpParam);

	WIN32_CHK(hwndCloned != NULL);

	return hwndCloned;
}



void GHOST_ContextWGL::initContextWGLEW(PIXELFORMATDESCRIPTOR& preferredPFD)
{
	wglewContext = new WGLEWContext;
	memset(wglewContext, 0, sizeof(WGLEWContext));

	delete m_wglewContext;
	m_wglewContext = wglewContext;

	HWND  dummyHWND  = NULL;
	HDC   dummyHDC   = NULL;
	HGLRC dummyHGLRC = NULL;

	SetLastError(NO_ERROR);

	HDC prevHDC = ::wglGetCurrentDC();
	WIN32_CHK(GetLastError() == NO_ERROR);

	HGLRC prevHGLRC = ::wglGetCurrentContext();
	WIN32_CHK(GetLastError() == NO_ERROR);

	dummyHWND = clone_window(m_hWnd, NULL);

	if (dummyHWND == NULL)
		goto finalize;

	dummyHDC = GetDC(dummyHWND);

	if (!WIN32_CHK(dummyHDC != NULL))
		goto finalize;

	int iPixelFormat = choose_pixel_format_legacy(dummyHDC, preferredPFD);

	if (iPixelFormat == 0)
		goto finalize;

	PIXELFORMATDESCRIPTOR chosenPFD;
	if (!WIN32_CHK(::DescribePixelFormat(dummyHDC, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &chosenPFD)))
		goto finalize;

	if (!WIN32_CHK(::SetPixelFormat(dummyHDC, iPixelFormat, &chosenPFD)))
		goto finalize;

	dummyHGLRC = ::wglCreateContext(dummyHDC);

	if (!WIN32_CHK(dummyHGLRC != NULL))
		goto finalize;

	if (!WIN32_CHK(::wglMakeCurrent(dummyHDC, dummyHGLRC)))
		goto finalize;

	if (wglewInit() != GLEW_OK)
		fprintf(stderr, "Warning! WGLEW failed to initialize properly.\n");

	// the following are not technially WGLEW, but they also require a context to work

#ifndef NDEBUG
	delete m_dummyRenderer;
	delete m_dummyVendor;
	delete m_dummyVersion;

	m_dummyRenderer = _strdup(reinterpret_cast<const char *>(glGetString(GL_RENDERER)));
	m_dummyVendor   = _strdup(reinterpret_cast<const char *>(glGetString(GL_VENDOR)));
	m_dummyVersion  = _strdup(reinterpret_cast<const char *>(glGetString(GL_VERSION)));
#endif

	s_singleContextMode = is_crappy_intel_card();

finalize:
	WIN32_CHK(::wglMakeCurrent(prevHDC, prevHGLRC));

	if (dummyHGLRC != NULL)
		WIN32_CHK(::wglDeleteContext(dummyHGLRC));

	if (dummyHWND != NULL) {
		if (dummyHDC != NULL)
			WIN32_CHK(::ReleaseDC(dummyHWND, dummyHDC));

		WIN32_CHK(::DestroyWindow(dummyHWND));
	}
}



static void makeAttribList(
	std::vector<int>& out,
	bool stereoVisual,
	int  numOfAASamples,
	int  swapMethod,
	bool needAlpha,
	bool needStencil,
	bool sRGB)
{
	out.clear();
	out.reserve(30);

	out.push_back(WGL_SUPPORT_OPENGL_ARB);
	out.push_back(GL_TRUE);

	out.push_back(WGL_DRAW_TO_WINDOW_ARB);
	out.push_back(GL_TRUE);

	out.push_back(WGL_DOUBLE_BUFFER_ARB);
	out.push_back(GL_TRUE);

	out.push_back(WGL_ACCELERATION_ARB);
	out.push_back(WGL_FULL_ACCELERATION_ARB);

	out.push_back(WGL_SWAP_METHOD_ARB);
	out.push_back(swapMethod);

	if (stereoVisual) {
		out.push_back(WGL_STEREO_ARB);
		out.push_back(GL_TRUE);
	}

	out.push_back(WGL_PIXEL_TYPE_ARB);
	out.push_back(WGL_TYPE_RGBA_ARB);

	out.push_back(WGL_COLOR_BITS_ARB);
	out.push_back(24);

	out.push_back(WGL_DEPTH_BITS_ARB);
	out.push_back(24);

	if (needAlpha) {
		out.push_back(WGL_ALPHA_BITS_ARB);
		out.push_back(8);
	}

	if (needStencil) {
		out.push_back(WGL_STENCIL_BITS_ARB);
		out.push_back(8);
	}

	if (numOfAASamples > 0) {
		out.push_back(WGL_SAMPLES_ARB);
		out.push_back(numOfAASamples);

		out.push_back(WGL_SAMPLE_BUFFERS_ARB);
		out.push_back(1);
	}

	if (sRGB) {
		out.push_back(WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB);
		out.push_back(GL_TRUE);
	}

	out.push_back(0);
}



int GHOST_ContextWGL::_choose_pixel_format_arb_2(
	bool stereoVisual,
	int  numOfAASamples,
	bool needAlpha,
	bool needStencil,
	bool sRGB,
	int  swapMethod)
{
	std::vector<int> iAttributes;

	int iPixelFormat = 0;

	int samples;

	// guard against some insanely high number of samples
	if (numOfAASamples > 64) {
		fprintf(stderr, "Warning! Clamping number of samples to 64.\n");
		samples = 64;
	}
	else {
		samples = numOfAASamples;
	}

	// request a format with as many samples as possible, but not more than requested
	while (samples >= 0) {
		makeAttribList(
			iAttributes,
			stereoVisual,
			samples,
			swapMethod,
			needAlpha,
			needStencil,
			sRGB);

		UINT nNumFormats;
		WIN32_CHK(wglChoosePixelFormatARB(m_hDC, &(iAttributes[0]), NULL, 1, &iPixelFormat, &nNumFormats));

		if (nNumFormats > 0) // total number of formats that match (regardless of size of iPixelFormat array) (see WGL_ARB_pixel_format extension spec)
			break;

		iPixelFormat = 0; // if not reset, then the state of iPixelFormat is undefined after call to wglChoosePixelFormatARB (see WGL_ARB_pixel_format extension spec)

		samples--;
	}

	// check how many samples were actually gotten
	if (iPixelFormat != 0) {
		int iQuery[] = { WGL_SAMPLES_ARB };
		int actualSamples;
		wglGetPixelFormatAttribivARB(m_hDC, iPixelFormat, 0, 1, iQuery, &actualSamples);

		if (actualSamples != numOfAASamples) {
			fprintf(
				stderr,
				"Warning! Unable to find a multisample pixel format that supports %d samples. Substituting one that uses %d samples.\n",
				numOfAASamples,
				actualSamples);
		}
	}

	return iPixelFormat;
}



int GHOST_ContextWGL::_choose_pixel_format_arb_1(
	bool stereoVisual,
	int  numOfAASamples,
	bool needAlpha,
	bool needStencil,
	bool sRGB,
	int& swapMethodOut)
{
	int iPixelFormat;

	swapMethodOut   = WGL_SWAP_COPY_ARB;
	iPixelFormat = _choose_pixel_format_arb_2(stereoVisual, numOfAASamples, needAlpha, needStencil, sRGB, swapMethodOut);

	if (iPixelFormat == 0) {
		swapMethodOut = WGL_SWAP_UNDEFINED_ARB;
		iPixelFormat = _choose_pixel_format_arb_2(stereoVisual, numOfAASamples, needAlpha, needStencil, sRGB, swapMethodOut);
	}

	if (iPixelFormat == 0) {
		swapMethodOut   = WGL_SWAP_EXCHANGE_ARB;
		iPixelFormat = _choose_pixel_format_arb_2(stereoVisual, numOfAASamples, needAlpha, needStencil, sRGB, swapMethodOut);
	}

	return iPixelFormat;
}



int GHOST_ContextWGL::choose_pixel_format_arb(
	bool stereoVisual,
	int  numOfAASamples,
	bool needAlpha,
	bool needStencil,
	bool sRGB)
{
	int iPixelFormat;
	int swapMethodOut;

	iPixelFormat =
		_choose_pixel_format_arb_1(
			stereoVisual,
			numOfAASamples,
			needAlpha,
			needStencil,
			sRGB,
			swapMethodOut);

	if (iPixelFormat == 0 && stereoVisual) {
		fprintf(stderr, "Warning! Unable to find a stereo pixel format.\n");

		iPixelFormat =
			_choose_pixel_format_arb_1(
				false,
				numOfAASamples,
				needAlpha,
				needStencil,
				sRGB,
				swapMethodOut);
	}

	if (swapMethodOut != WGL_SWAP_COPY_ARB) {
		fprintf(
			stderr,
			"Warning! Unable to find a pixel format that supports WGL_SWAP_COPY_ARB. Substituting one that uses %s.\n",
			swapMethodOut == WGL_SWAP_UNDEFINED_ARB ? "WGL_SWAP_UNDEFINED_ARB" : "WGL_SWAP_EXCHANGE_ARB");
	}

	return iPixelFormat;
}



int GHOST_ContextWGL::choose_pixel_format(
	bool stereoVisual,
	int  numOfAASamples,
	bool needAlpha,
	bool needStencil,
	bool sRGB)
{
	PIXELFORMATDESCRIPTOR preferredPFD = {
		sizeof(PIXELFORMATDESCRIPTOR),   /* size */
		1,                               /* version */
		PFD_SUPPORT_OPENGL |
		PFD_DRAW_TO_WINDOW |
		PFD_SWAP_COPY      |             /* support swap copy */
		PFD_DOUBLEBUFFER   |             /* support double-buffering */
		(stereoVisual ? PFD_STEREO : 0), /* support stereo */
		PFD_TYPE_RGBA,                   /* color type */
		24,                              /* preferred color depth */
		0, 0, 0, 0, 0, 0,                /* color bits (ignored) */
		needAlpha ? 8 : 0,               /* alpha buffer */
		0,                               /* alpha shift (ignored) */
		0,                               /* no accumulation buffer */
		0, 0, 0, 0,                      /* accum bits (ignored) */
		24,                              /* depth buffer */
		needStencil ? 8 : 0,             /* stencil buffer */
		0,                               /* no auxiliary buffers */
		PFD_MAIN_PLANE,                  /* main layer */
		0,                               /* reserved */
		0, 0, 0                          /* layer, visible, and damage masks (ignored) */
	};

	initContextWGLEW(preferredPFD);

	if (numOfAASamples > 0 && !WGLEW_ARB_multisample) {
		fprintf(stderr, "Warning! Unable to request a multisample framebuffer.\n");
		numOfAASamples = 0;
	}

	if (sRGB && !(WGLEW_ARB_framebuffer_sRGB || WGLEW_EXT_framebuffer_sRGB)) {
		fprintf(stderr, "Warning! Unable to request an sRGB framebuffer.\n");
		sRGB = false;
	}

	int iPixelFormat = 0;

	if (WGLEW_ARB_pixel_format)
		iPixelFormat = choose_pixel_format_arb(stereoVisual, numOfAASamples, needAlpha, needStencil, sRGB);

	if (iPixelFormat == 0)
		iPixelFormat = choose_pixel_format_legacy(m_hDC, preferredPFD);

	return iPixelFormat;
}



#ifndef NDEBUG
static void reportContextString(const char* name, const char* dummy, const char* context)
{
	fprintf(stderr, "%s: %s\n", name, context);

	if (strcmp(dummy, context) != 0)
		fprintf(stderr, "Warning! Dummy %s: %s\n", name, dummy);
}
#endif



GHOST_TSuccess GHOST_ContextWGL::initializeDrawingContext(bool stereoVisual, GHOST_TUns16 numOfAASamples)
{
	SetLastError(NO_ERROR);

	HGLRC prevHGLRC = ::wglGetCurrentContext();
	WIN32_CHK(GetLastError() == NO_ERROR);

	HDC   prevHDC   = ::wglGetCurrentDC();
	WIN32_CHK(GetLastError() == NO_ERROR);

	const bool needAlpha   = false;
	const bool needStencil = false;
	const bool sRGB        = false;

	if (m_needSetPixelFormat) {
		int iPixelFormat = choose_pixel_format(stereoVisual, numOfAASamples, needAlpha, needStencil, sRGB);

		if (iPixelFormat == 0)
			goto error;

		PIXELFORMATDESCRIPTOR chosenPFD;

		int lastPFD = ::DescribePixelFormat(m_hDC, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &chosenPFD);

		if (!WIN32_CHK(lastPFD != 0))
			goto error;

		if (needAlpha && chosenPFD.cAlphaBits == 0)
			fprintf(stderr, "Warning! Unable to find a pixel format with an alpha channel.\n");

		if (needStencil && chosenPFD.cStencilBits == 0)
			fprintf(stderr, "Warning! Unable to find a pixel format with a stencil buffer.\n");

		if (!WIN32_CHK(::SetPixelFormat(m_hDC, iPixelFormat, &chosenPFD)))
			goto error;

		m_needSetPixelFormat = false;
	}

	activateWGLEW();

	if (WGLEW_ARB_create_context) {
		int profileBitCore   = m_contextProfileMask & WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
		int profileBitCompat = m_contextProfileMask & WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
		int profileBitES     = m_contextProfileMask & WGL_CONTEXT_ES_PROFILE_BIT_EXT;

		if (!WGLEW_ARB_create_context_profile && profileBitCore)
			fprintf(stderr, "Warning! OpenGL core profile not available.\n");

		if (!WGLEW_ARB_create_context_profile && profileBitCompat)
			fprintf(stderr, "Warning! OpenGL compatibility profile not available.\n");

		if (!WGLEW_EXT_create_context_es_profile && profileBitES && m_contextMajorVersion == 1)
			fprintf(stderr, "Warning! OpenGL ES profile not available.\n");

		if (!WGLEW_EXT_create_context_es2_profile && profileBitES && m_contextMajorVersion == 2)
			fprintf(stderr, "Warning! OpenGL ES2 profile not available.\n");

		int profileMask = 0;

		if (WGLEW_ARB_create_context_profile && profileBitCore)
			profileMask |= profileBitCore;

		if (WGLEW_ARB_create_context_profile && profileBitCompat)
			profileMask |= profileBitCompat;

		if (WGLEW_EXT_create_context_es_profile && profileBitES)
			profileMask |= profileBitES;

		std::vector<int> iAttributes;

		if (profileMask) {
			iAttributes.push_back(WGL_CONTEXT_PROFILE_MASK_ARB);
			iAttributes.push_back(profileMask);
		}

		if (m_contextMajorVersion != 0) {
			iAttributes.push_back(WGL_CONTEXT_MAJOR_VERSION_ARB);
			iAttributes.push_back(m_contextMajorVersion);
		}

		if (m_contextMinorVersion != 0) {
			iAttributes.push_back(WGL_CONTEXT_MINOR_VERSION_ARB);
			iAttributes.push_back(m_contextMinorVersion);
		}

		if (m_contextFlags != 0) {
			iAttributes.push_back(WGL_CONTEXT_FLAGS_ARB);
			iAttributes.push_back(m_contextFlags);
		}

		iAttributes.push_back(0);

		if (!s_singleContextMode || s_sharedHGLRC == NULL)
			m_hGLRC = ::wglCreateContextAttribsARB(m_hDC, NULL, &(iAttributes[0]));
		else
			m_hGLRC = s_sharedHGLRC;
	}
	else {
		if (m_contextProfileMask  != 0)
			fprintf(stderr, "Warning! Legacy WGL is unable to select between OpenGL profiles.");

		if (m_contextMajorVersion != 0 || m_contextMinorVersion != 0)
			fprintf(stderr, "Warning! Legacy WGL is unable to select between OpenGL versions.");

		if (m_contextFlags != 0)
			fprintf(stderr, "Warning! Legacy WGL is unable to set context flags.");

		if (!s_singleContextMode || s_sharedHGLRC == NULL)
			m_hGLRC = ::wglCreateContext(m_hDC);
		else
			m_hGLRC = s_sharedHGLRC;
	}

	if (!WIN32_CHK(m_hGLRC != NULL))
		goto error;

	if (s_sharedHGLRC == NULL)
		s_sharedHGLRC = m_hGLRC;

	s_sharedCount++;

	if (!s_singleContextMode && s_sharedHGLRC != m_hGLRC && !WIN32_CHK(::wglShareLists(s_sharedHGLRC, m_hGLRC)))
		goto error;

	if (!WIN32_CHK(::wglMakeCurrent(m_hDC, m_hGLRC)))
		goto error;

	initContextGLEW();

#ifndef NDEBUG
	reportContextString("Vendor",   m_dummyVendor,   reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	reportContextString("Renderer", m_dummyRenderer, reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	reportContextString("Version",  m_dummyVersion,  reinterpret_cast<const char*>(glGetString(GL_VERSION)));
#endif

	return GHOST_kSuccess;

error:
	::wglMakeCurrent(prevHDC, prevHGLRC);

	return GHOST_kFailure;
}



GHOST_TSuccess GHOST_ContextWGL::releaseNativeHandles()
{
	GHOST_TSuccess success = m_hGLRC != s_sharedHGLRC || s_sharedCount == 1 ? GHOST_kSuccess : GHOST_kFailure;

	m_hWnd = NULL;
	m_hDC  = NULL;

	return success;
}
