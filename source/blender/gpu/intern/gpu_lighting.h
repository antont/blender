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

/** \file gpu_lighting.h
 *  \ingroup gpu
 */

#ifndef GPU_LIGHTING_H
#define GPU_LIGHTING_H



#include "BLI_utildefines.h"

#include "intern/gpu_glew.h"
#include "intern/gpu_known.h"


#ifdef __cplusplus
extern "C" {
#endif

#if 0

typedef struct GPUlighting {
	void (*material_fv)(GLenum face, GLenum pname, const GLfloat *params);
	void (*material_i)(GLenum face, GLenum pname, GLint param);
	void (*get_material_fv)(GLenum face, GLenum pname, GLfloat *params);
	void (*color_material)(GLenum face, GLenum mode);
	void (*enable_color_material)(void);
	void (*disable_color_material)(void);
	void (*light_f)(GLint light, GLenum pname, GLfloat param);
	void (*light_fv)(GLint light, GLenum pname, const GLfloat* params);
	void (*enable_light)(GLint light);
	void (*disable_light)(GLint light);
	GLboolean (*is_light_enabled)(GLint light);
	void (*light_model_i)(GLenum pname, GLint param);
	void (*light_model_fv)(GLenum pname, const GLfloat* params);
	void (*enable_lighting)(void);
	void (*disable_lighting)(void);
	GLboolean (*is_lighting_enabled)(void);
} GPUlighting;




extern GPUlighting *restrict GPU_LIGHTING;



void gpuInitializeLighting(void);
void gpuShutdownLighting(void);


#if defined(GLEW_ES_ONLY)

// XXX jwilkins: just here because it is passed to the light replacement library,
// however it this value doesn't change then it can be assumed by the GLSL implementation and
// won't have to be set, and if so then it can be removed
#ifndef GL_LIGHT_MODEL_LOCAL_VIEWER
#define GL_LIGHT_MODEL_LOCAL_VIEWER 0x0B51
#endif

#endif

#endif

/* Simple Lighting */



typedef struct GPUsimplelight {
	float position[4];
	float diffuse [4];
	float specular[4];

	float constant_attenuation;
	float linear_attenuation;
	float quadratic_attenuation;

	float spot_direction[3];
	float spot_cutoff;
	float spot_exponent;
} GPUsimplelight;

typedef struct GPUsimplematerial {
	float diffuse [4];
	float specular[4];
	int   shininess;
} GPUsimplematerial;

void GPU_commit_light   (uint32_t lights_enabled, const GPUsimplelight *light);
void GPU_commit_material(const GPUsimplematerial* material);



#ifdef __cplusplus
}
#endif

#endif /* GPU_LIGHTING_H */
