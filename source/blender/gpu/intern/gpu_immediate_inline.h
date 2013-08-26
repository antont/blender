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
 * Contributor(s): Jason Wilkins.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/gpu/intern/gpu_immediate.h
 *  \ingroup gpu
 */

#ifndef __GPU_IMMEDIATE_INLINE_H__
#define __GPU_IMMEDIATE_INLINE_H__

#include "intern/gpu_immediate.h"

#include "intern/gpu_immediate_gl.h"
#include "intern/gpu_safety.h"

#include "BLI_math_base.h"

#include <limits.h>



#ifdef __cplusplus
extern "C" {
#endif



BLI_INLINE void gpuColor3f(GLfloat r, GLfloat g, GLfloat b)
{
	GPU_IMMEDIATE->color[0] = (GLubyte)(255.0f * r);
	GPU_IMMEDIATE->color[1] = (GLubyte)(255.0f * g);
	GPU_IMMEDIATE->color[2] = (GLubyte)(255.0f * b);
	GPU_IMMEDIATE->color[3] = 255;
}

BLI_INLINE void gpuColor3fv(const GLfloat *restrict v)
{
	GPU_IMMEDIATE->color[0] = (GLubyte)(255.0f * v[0]);
	GPU_IMMEDIATE->color[1] = (GLubyte)(255.0f * v[1]);
	GPU_IMMEDIATE->color[2] = (GLubyte)(255.0f * v[2]);
	GPU_IMMEDIATE->color[3] = 255;
}

BLI_INLINE void gpuColor3ub(GLubyte r, GLubyte g, GLubyte b)
{
	GPU_IMMEDIATE->color[0] = r;
	GPU_IMMEDIATE->color[1] = g;
	GPU_IMMEDIATE->color[2] = b;
	GPU_IMMEDIATE->color[3] = 255;
}

BLI_INLINE void gpuColor3ubv(const GLubyte *restrict v)
{
	GPU_IMMEDIATE->color[0] = v[0];
	GPU_IMMEDIATE->color[1] = v[1];
	GPU_IMMEDIATE->color[2] = v[2];
	GPU_IMMEDIATE->color[3] = 255;
}

BLI_INLINE void gpuColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	GPU_IMMEDIATE->color[0] = (GLubyte)(255.0f * r);
	GPU_IMMEDIATE->color[1] = (GLubyte)(255.0f * g);
	GPU_IMMEDIATE->color[2] = (GLubyte)(255.0f * b);
	GPU_IMMEDIATE->color[3] = (GLubyte)(255.0f * a);
}

BLI_INLINE void gpuColor4fv(const GLfloat *restrict v)
{
	GPU_IMMEDIATE->color[0] = (GLubyte)(255.0f * v[0]);
	GPU_IMMEDIATE->color[1] = (GLubyte)(255.0f * v[1]);
	GPU_IMMEDIATE->color[2] = (GLubyte)(255.0f * v[2]);
	GPU_IMMEDIATE->color[3] = (GLubyte)(255.0f * v[3]);
}

BLI_INLINE void gpuColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
	GPU_IMMEDIATE->color[0] = r;
	GPU_IMMEDIATE->color[1] = g;
	GPU_IMMEDIATE->color[2] = b;
	GPU_IMMEDIATE->color[3] = a;
}

BLI_INLINE void gpuColor4ubv(const GLubyte *restrict v)
{
	GPU_IMMEDIATE->color[0] = v[0];
	GPU_IMMEDIATE->color[1] = v[1];
	GPU_IMMEDIATE->color[2] = v[2];
	GPU_IMMEDIATE->color[3] = v[3];
}

BLI_INLINE void gpuColor4d(GLdouble r, GLdouble g, GLdouble b, GLdouble a)
{
	GPU_IMMEDIATE->color[0] = (GLubyte)(255.0 * r);
	GPU_IMMEDIATE->color[1] = (GLubyte)(255.0 * g);
	GPU_IMMEDIATE->color[2] = (GLubyte)(255.0 * b);
	GPU_IMMEDIATE->color[3] = (GLubyte)(255.0 * a);
}



/* This function converts a numerical value to the equivalent 24-bit
   color, while not being endian-sensitive. On little-endians, this
   is the same as doing a 'naive' indexing, on big-endian, it is not! */

BLI_INLINE void gpuColor3P(GLuint rgb)
{
	GPU_IMMEDIATE->color[0] = (rgb >>  0) & 0xFF;
	GPU_IMMEDIATE->color[1] = (rgb >>  8) & 0xFF;
	GPU_IMMEDIATE->color[2] = (rgb >> 16) & 0xFF;
	GPU_IMMEDIATE->color[3] = 255;
}

BLI_INLINE void gpuColor4P(GLuint rgb, GLfloat a)
{
	GPU_IMMEDIATE->color[0] = (rgb >>  0) & 0xFF;
	GPU_IMMEDIATE->color[1] = (rgb >>  8) & 0xFF;
	GPU_IMMEDIATE->color[2] = (rgb >> 16) & 0xFF;
	GPU_IMMEDIATE->color[3] = (GLubyte)(255.0f * a);
}



BLI_INLINE void gpuGray3f(GLfloat luminance)
{
	GLubyte c = (GLubyte)(255.0 * luminance);

	GPU_IMMEDIATE->color[0] = c;
	GPU_IMMEDIATE->color[1] = c;
	GPU_IMMEDIATE->color[2] = c;
	GPU_IMMEDIATE->color[3] = 255;
}

BLI_INLINE void gpuGray4f(GLfloat luminance, GLfloat alpha)
{
	GLubyte c = (GLubyte)(255.0 * luminance);

	GPU_IMMEDIATE->color[0] = c;
	GPU_IMMEDIATE->color[1] = c;
	GPU_IMMEDIATE->color[2] = c;
	GPU_IMMEDIATE->color[3] = (GLubyte)(255.0f * alpha);
}



BLI_INLINE void gpuAlpha(GLfloat a)
{
	GPU_IMMEDIATE->color[3] = (GLubyte)(255.0f * a);
}

BLI_INLINE void gpuMultAlpha(GLfloat factor)
{
	GPU_IMMEDIATE->color[3] = (GLubyte)(factor * (GLfloat)(GPU_IMMEDIATE->color[3]));
}



BLI_INLINE void gpuGetColor4fv(GLfloat *restrict color)
{
	color[0] = (GLfloat)(GPU_IMMEDIATE->color[0]) / 255.0f;
	color[1] = (GLfloat)(GPU_IMMEDIATE->color[1]) / 255.0f;
	color[2] = (GLfloat)(GPU_IMMEDIATE->color[2]) / 255.0f;
	color[3] = (GLfloat)(GPU_IMMEDIATE->color[3]) / 255.0f;
}

BLI_INLINE void gpuGetColor4ubv(GLubyte *restrict color)
{
	color[0] = GPU_IMMEDIATE->color[0];
	color[1] = GPU_IMMEDIATE->color[1];
	color[2] = GPU_IMMEDIATE->color[2];
	color[3] = GPU_IMMEDIATE->color[3];
}



BLI_INLINE void gpuNormal3f(GLfloat x, GLfloat y, GLfloat z)
{
	GPU_IMMEDIATE->normal[0] = x;
	GPU_IMMEDIATE->normal[1] = y;
	GPU_IMMEDIATE->normal[2] = z;
}

BLI_INLINE void gpuNormal3fv(const GLfloat *restrict v)
{
	GPU_IMMEDIATE->normal[0] = v[0];
	GPU_IMMEDIATE->normal[1] = v[1];
	GPU_IMMEDIATE->normal[2] = v[2];
}

BLI_INLINE void gpuNormal3sv(const GLshort *restrict v)
{
	GPU_IMMEDIATE->normal[0] = v[0] / (float)SHRT_MAX;
	GPU_IMMEDIATE->normal[1] = v[1] / (float)SHRT_MAX;
	GPU_IMMEDIATE->normal[2] = v[2] / (float)SHRT_MAX;
}



BLI_INLINE void gpuTexCoord2f(GLfloat s, GLfloat t)
{
	GPU_IMMEDIATE->texCoord[0][0] = s;
	GPU_IMMEDIATE->texCoord[0][1] = t;
	GPU_IMMEDIATE->texCoord[0][2] = 0;
	GPU_IMMEDIATE->texCoord[0][3] = 1;
}

BLI_INLINE void gpuTexCoord2fv(const GLfloat *restrict v)
{
	GPU_IMMEDIATE->texCoord[0][0] = v[0];
	GPU_IMMEDIATE->texCoord[0][1] = v[1];
	GPU_IMMEDIATE->texCoord[0][2] = 0;
	GPU_IMMEDIATE->texCoord[0][3] = 1;
}

BLI_INLINE void gpuTexCoord2iv(const GLint *restrict v)
{
	GPU_IMMEDIATE->texCoord[0][0] = (GLfloat)(v[0]);
	GPU_IMMEDIATE->texCoord[0][1] = (GLfloat)(v[1]);
	GPU_IMMEDIATE->texCoord[0][2] = 0;
	GPU_IMMEDIATE->texCoord[0][3] = 1;
}

BLI_INLINE void gpuTexCoord3f(const GLfloat s, const GLfloat t, const GLfloat u)
{
	GPU_IMMEDIATE->texCoord[0][0] = s;
	GPU_IMMEDIATE->texCoord[0][1] = t;
	GPU_IMMEDIATE->texCoord[0][2] = u;
	GPU_IMMEDIATE->texCoord[0][3] = 1;
}

BLI_INLINE void gpuTexCoord3fv (const GLfloat *restrict v)
{
	GPU_IMMEDIATE->texCoord[0][0] = v[0];
	GPU_IMMEDIATE->texCoord[0][1] = v[1];
	GPU_IMMEDIATE->texCoord[0][2] = v[2];
	GPU_IMMEDIATE->texCoord[0][3] = 1;
}



BLI_INLINE void gpuMultiTexCoord2f(GLint index, GLfloat s, GLfloat t)
{
	GPU_IMMEDIATE->texCoord[index][0] = s;
	GPU_IMMEDIATE->texCoord[index][1] = t;
	GPU_IMMEDIATE->texCoord[index][2] = 0;
	GPU_IMMEDIATE->texCoord[index][3] = 1;
}

BLI_INLINE void gpuMultiTexCoord2fv(GLint index, const GLfloat *restrict v)
{
	GPU_IMMEDIATE->texCoord[index][0] = v[0];
	GPU_IMMEDIATE->texCoord[index][1] = v[1];
	GPU_IMMEDIATE->texCoord[index][2] = 0;
	GPU_IMMEDIATE->texCoord[index][3] = 1;
}

BLI_INLINE void gpuMultiTexCoord3fv(GLint index, const GLfloat *restrict v)
{
	GPU_IMMEDIATE->texCoord[index][0] = v[0];
	GPU_IMMEDIATE->texCoord[index][1] = v[1];
	GPU_IMMEDIATE->texCoord[index][2] = v[2];
	GPU_IMMEDIATE->texCoord[index][3] = 1;
}

BLI_INLINE void gpuMultiTexCoord4fv(GLint index, const GLfloat *restrict v)
{
	GPU_IMMEDIATE->texCoord[index][0] = v[0];
	GPU_IMMEDIATE->texCoord[index][1] = v[1];
	GPU_IMMEDIATE->texCoord[index][2] = v[2];
	GPU_IMMEDIATE->texCoord[index][3] = v[3];
}




BLI_INLINE void gpuVertexAttrib2fv(GLsizei index, const GLfloat *restrict v)
{
	GPU_IMMEDIATE->attrib_f[index][0] = v[0];
	GPU_IMMEDIATE->attrib_f[index][1] = v[1];
	GPU_IMMEDIATE->attrib_f[index][2] = 0;
	GPU_IMMEDIATE->attrib_f[index][3] = 1;
}

BLI_INLINE void gpuVertexAttrib3fv(GLsizei index, const GLfloat *restrict v)
{
	GPU_IMMEDIATE->attrib_f[index][0] = v[0];
	GPU_IMMEDIATE->attrib_f[index][1] = v[1];
	GPU_IMMEDIATE->attrib_f[index][2] = v[2];
	GPU_IMMEDIATE->attrib_f[index][3] = 1;
}

BLI_INLINE void gpuVertexAttrib4fv(GLsizei index, const GLfloat *restrict v)
{
	GPU_IMMEDIATE->attrib_f[index][0] = v[0];
	GPU_IMMEDIATE->attrib_f[index][1] = v[1];
	GPU_IMMEDIATE->attrib_f[index][2] = v[2];
	GPU_IMMEDIATE->attrib_f[index][3] = v[3];
}

BLI_INLINE void gpuVertexAttrib4ubv(GLsizei index, const GLubyte *restrict v)
{
	GPU_IMMEDIATE->attrib_ub[index][0] = v[0];
	GPU_IMMEDIATE->attrib_ub[index][1] = v[1];
	GPU_IMMEDIATE->attrib_ub[index][2] = v[2];
	GPU_IMMEDIATE->attrib_ub[index][3] = v[3];
}



BLI_INLINE void gpuVertex2f(GLfloat x, GLfloat y)
{
	GPU_IMMEDIATE->vertex[0] = x;
	GPU_IMMEDIATE->vertex[1] = y;
	GPU_IMMEDIATE->vertex[2] = 0;
	GPU_IMMEDIATE->vertex[3] = 1;

	GPU_IMMEDIATE->copyVertex();
}

BLI_INLINE void gpuVertex2fv(const GLfloat *restrict v)
{
	GPU_IMMEDIATE->vertex[0] = v[0];
	GPU_IMMEDIATE->vertex[1] = v[1];
	GPU_IMMEDIATE->vertex[2] = 0;
	GPU_IMMEDIATE->vertex[3] = 1;

	GPU_IMMEDIATE->copyVertex();
}

BLI_INLINE void gpuVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	GPU_IMMEDIATE->vertex[0] = x;
	GPU_IMMEDIATE->vertex[1] = y;
	GPU_IMMEDIATE->vertex[2] = z;
	GPU_IMMEDIATE->vertex[3] = 1;

	GPU_IMMEDIATE->copyVertex();
 }

BLI_INLINE void gpuVertex3fv(const GLfloat *restrict v)
{
	GPU_IMMEDIATE->vertex[0] = v[0];
	GPU_IMMEDIATE->vertex[1] = v[1];
	GPU_IMMEDIATE->vertex[2] = v[2];
	GPU_IMMEDIATE->vertex[3] = 1;

	GPU_IMMEDIATE->copyVertex();
}

BLI_INLINE void gpuVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
	GPU_IMMEDIATE->vertex[0] = (GLfloat)(x);
	GPU_IMMEDIATE->vertex[1] = (GLfloat)(y);
	GPU_IMMEDIATE->vertex[2] = (GLfloat)(z);
	GPU_IMMEDIATE->vertex[3] = 1;

	GPU_IMMEDIATE->copyVertex();
}

BLI_INLINE void gpuVertex3dv(const GLdouble *restrict v)
{
	GPU_IMMEDIATE->vertex[0] = (GLfloat)(v[0]);
	GPU_IMMEDIATE->vertex[1] = (GLfloat)(v[1]);
	GPU_IMMEDIATE->vertex[2] = (GLfloat)(v[2]);
	GPU_IMMEDIATE->vertex[3] = 1;

	GPU_IMMEDIATE->copyVertex();
}

BLI_INLINE void gpuVertex2i(GLint x, GLint y)
{
	GPU_IMMEDIATE->vertex[0] = (GLfloat)(x);
	GPU_IMMEDIATE->vertex[1] = (GLfloat)(y);
	GPU_IMMEDIATE->vertex[2] = 0;
	GPU_IMMEDIATE->vertex[3] = 1;

	GPU_IMMEDIATE->copyVertex();
}

BLI_INLINE void gpuVertex2iv(const GLint *restrict v)
{
	GPU_IMMEDIATE->vertex[0] = (GLfloat)(v[0]);
	GPU_IMMEDIATE->vertex[1] = (GLfloat)(v[1]);
	GPU_IMMEDIATE->vertex[2] = 0;
	GPU_IMMEDIATE->vertex[3] = 1;

	GPU_IMMEDIATE->copyVertex();
}

BLI_INLINE void gpuVertex2sv(const GLshort *restrict v)
{
	GPU_IMMEDIATE->vertex[0] = (GLfloat)(v[0]);
	GPU_IMMEDIATE->vertex[1] = (GLfloat)(v[1]);
	GPU_IMMEDIATE->vertex[2] = 0;
	GPU_IMMEDIATE->vertex[3] = 1;

	GPU_IMMEDIATE->copyVertex();
}



BLI_INLINE void gpuDraw(GLenum mode)
{
	GPU_CHECK_CAN_REPEAT();

	GPU_IMMEDIATE->mode = mode;
	gpu_end_buffer_gl();
}

BLI_INLINE void gpuRepeat(void)
{
	GPU_CHECK_CAN_REPEAT();

	gpu_end_buffer_gl();
}



BLI_INLINE void gpuDrawElements(GLenum mode)
{
	GPU_IMMEDIATE->mode = mode;
	gpu_draw_elements_gl();
}




void gpu_draw_elements_gl(void);



BLI_INLINE void gpuRepeatElements(void)
{
	gpu_draw_elements_gl();
}



void gpu_draw_range_elements_gl(void);



BLI_INLINE void gpuDrawRangeElements(GLenum mode)
{
	GPU_IMMEDIATE->mode = mode;
	gpu_draw_range_elements_gl();
}

BLI_INLINE void gpuRepeatRangeElements(void)
{
	gpu_draw_range_elements_gl();
}



#ifdef __cplusplus
}
#endif

#endif /* __GPU_IMMEDIATE_INLINE_H_ */
