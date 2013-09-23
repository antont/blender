#ifndef _GPU_UTILITY_H_
#define _GPU_UTILITY_H_

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
 * The Original Code is Copyright (C) 2012 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Jason Wilkins
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file source/blender/gpu/GPU_utility.h
 *  \ingroup gpu
 */

#include "GPU_glew.h"



#ifdef __cplusplus
extern "C" {
#endif




/* XXX jwilkins: temporary work around for MinGW32 build error */
#ifdef __MINGW32__
#undef GPU_ENABLE_STRING_MARKER
#define GPU_ENABLE_STRING_MARKER 0
#endif

#ifndef GPU_ENABLE_STRING_MARKER
#define GPU_ENABLE_STRING_MARKER 1
#endif

#if GL_GREMEDY_string_marker && GPU_ENABLE_STRING_MARKER
#define GPU_STRING_MARKER(msg)                   \
    if (GLEW_GREMEDY_string_marker) {            \
        glStringMarkerGREMEDY(sizeof(msg), msg); \
    }
#else
#define GPU_STRING_MARKER(msg) ((void)0)
#endif



const char* gpuErrorString(GLenum err);





#ifdef __cplusplus
}
#endif



#endif /* _GPU_UTILITY_H_ */
