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

/** \file ghost/intern/GHOST_Context.cpp
 *  \ingroup GHOST
 * Definition of GHOST_Context class.
 */

#include "GHOST_Context.h"

#include <cstdio>
#include <cstring>



GLEWContext* glewContext = NULL;



const char* get_glew_error_message_string(GLenum error)
{
	switch (error) {
		case GLEW_OK: /* also GLEW_NO_ERROR */
			return "OK";

		case GLEW_ERROR_NO_GL_VERSION:
			return "Missing GL version";

		case GLEW_ERROR_GL_VERSION_10_ONLY:
			return "Need at least OpenGL 1.1";

		case GLEW_ERROR_GLX_VERSION_11_ONLY:
			return "Need at least GLX 1.2";

		case GLEW_ERROR_NOT_GLES_VERSION:
			return "Need to be OpenGL ES version";

		case GLEW_ERROR_GLES_VERSION:
			return "Need to be desktop OpenGL version";

		case GLEW_ERROR_NO_EGL_VERSION:
			return "Missing EGL version";

		case GLEW_ERROR_EGL_VERSION_10_ONLY:
			return "Need at least EGL 1.1";

		default:
			return NULL;
	}
}



const char* get_glew_error_enum_string(GLenum error)
{
	switch (error) {
		case GLEW_OK: /* also GLEW_NO_ERROR */
			return "GLEW_OK";

		case GLEW_ERROR_NO_GL_VERSION:
			return "GLEW_ERROR_NO_GL_VERSION";

		case GLEW_ERROR_GL_VERSION_10_ONLY:
			return "GLEW_ERROR_GL_VERSION_10_ONLY";

		case GLEW_ERROR_GLX_VERSION_11_ONLY:
			return "GLEW_ERROR_GLX_VERSION_11_ONLY";

		case GLEW_ERROR_NOT_GLES_VERSION:
			return "GLEW_ERROR_NOT_GLES_VERSION";

		case GLEW_ERROR_GLES_VERSION:
			return "GLEW_ERROR_GLES_VERSION";

		case GLEW_ERROR_NO_EGL_VERSION:
			return "GLEW_ERROR_NO_EGL_VERSION";

		case GLEW_ERROR_EGL_VERSION_10_ONLY:
			return "GLEW_ERROR_EGL_VERSION_10_ONLY";

		default:
			return NULL;
	}
}



GLenum glew_chk(GLenum error, const char* file, int line, const char* text)
{
	if (error != GLEW_OK) {
		const char* code = get_glew_error_enum_string(error);
		const char* msg  = get_glew_error_message_string(error);

#ifndef NDEBUG
		fprintf(
			stderr,
			"%s(%d):[%s] -> GLEW Error (0x%04X): %s: %s\n",
			file,
			line,
			text,
			error,
			code ? code : "<no symbol>",
			msg  ? msg  : "<no message>");
#else
		fprintf(
			stderr,
			"GLEW Error (%04X): %s: %s\n",
			error,
			code ? code : "<no symbol>",
			msg  ? msg  : "<no message>");
#endif
	}

	return error;
}



void GHOST_Context::initGlew()
{
	glewContext = new GLEWContext;
	memset(glewContext, 0, sizeof(GLEWContext));

	delete m_glewContext;
	m_glewContext = glewContext;

	GLEW_CHK(glewInit());
}



int GHOST_PixelFormat::computeWeight() const
{
	return 0;
}



void GHOST_PixelFormat::print() const
{
}



int GHOST_PixelFormatChooser::choosePixelFormat(GHOST_PixelFormatChooser& factory)
{
	return 0;
}

