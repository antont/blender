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
 * Contributor(s): Brecht Van Lommel, Jason Wilkins.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/gpu/intern/gpu_basic_shader.c
 *  \ingroup gpu
 */

/* GLSL shaders to replace fixed function OpenGL materials and lighting. These
 * are deprecated in newer OpenGL versions and missing in OpenGL ES 2.0. Also,
 * two sided lighting is no longer natively supported on NVidia cards which
 * results in slow software fallback.
 *
 * Todo:
 * x Replace glLight and glMaterial functions entirely with GLSL uniforms, to make OpenGL ES 2.0 work.
 * x Replace glTexCoord and glColor with generic attributes.
 * x Optimize for case where fewer than 3 or 8 lights are used.
 * - Optimize for case where specular is not used.
 * - Optimize for case where no texture matrix is used.
 */

#include "GPU_basic_shader.h"

#include "intern/gpu_common.h"
#include "intern/gpu_safety.h"

#include "GPU_extensions.h"
#include "GPU_matrix.h"

#include "BLI_math.h"
#include "BLI_dynstr.h"

#include <string.h>



/* State */

static struct BASIC_SHADER {
	uint32_t options;

	GPUShader*   gpushader[GPU_BASIC_OPTION_COMBINATIONS];
	bool         failed   [GPU_BASIC_OPTION_COMBINATIONS];
	GPUcommon    common   [GPU_BASIC_OPTION_COMBINATIONS];

	bool need_normals;
} BASIC_SHADER;



/* Init / exit */

void GPU_basic_shaders_init(void)
{
	memset(&BASIC_SHADER, 0, sizeof(BASIC_SHADER));
}



void GPU_basic_shaders_exit(void)
{
	int i;

	for (i = 0; i < GPU_BASIC_OPTION_COMBINATIONS; i++)
		if (BASIC_SHADER.gpushader[i] != NULL)
			GPU_shader_free(BASIC_SHADER.gpushader[i]);
}



/* Shader feature enable/disable */

void GPU_basic_shader_enable(uint32_t options)
{
	GPU_ASSERT(!(options & GPU_BASIC_FAST_LIGHTING));

	BASIC_SHADER.options |= options;
}



void GPU_basic_shader_disable(uint32_t options)
{
	BASIC_SHADER.options &= ~options;
}



/* Shader lookup / create */

static uint32_t tweak_options(void)
{
	/* detect if we can do faster lighting for solid draw mode */
	if (  BASIC_SHADER.options & GPU_BASIC_LIGHTING      &&
		!(BASIC_SHADER.options & GPU_BASIC_LOCAL_VIEWER) &&
		gpu_fast_lighting())
	{
		BASIC_SHADER.options |= GPU_BASIC_FAST_LIGHTING;
	}

	return BASIC_SHADER.options;
}



static void basic_shader_bind(void)
{
	/* glsl code */
	extern const char datatoc_gpu_shader_basic_vert_glsl[];
	extern const char datatoc_gpu_shader_basic_frag_glsl[];

	const uint32_t options = tweak_options();

GPU_CHECK_NO_ERROR();

	/* create shader if it doesn't exist yet */
	if (BASIC_SHADER.gpushader[options] != NULL) {
		GPU_shader_bind(BASIC_SHADER.gpushader[options]);
GPU_CHECK_NO_ERROR();
		gpu_set_common(BASIC_SHADER.common + options);
	}
	else if (!BASIC_SHADER.failed[options]) {
		DynStr* vert = BLI_dynstr_new();
		DynStr* frag = BLI_dynstr_new();
		DynStr* defs = BLI_dynstr_new();

		gpu_include_common_vert(vert);
		BLI_dynstr_append(vert, datatoc_gpu_shader_basic_vert_glsl);

		gpu_include_common_frag(frag);
		BLI_dynstr_append(frag, datatoc_gpu_shader_basic_frag_glsl);

		gpu_include_common_defs(defs);

		if (options & GPU_BASIC_TWO_SIDE)
			BLI_dynstr_append(defs, "#define USE_TWO_SIDE\n");

		if (options & GPU_BASIC_TEXTURE_2D)
			BLI_dynstr_append(defs, "#define USE_TEXTURE_2D\n");

		if (options & GPU_BASIC_LOCAL_VIEWER)
			BLI_dynstr_append(defs, "#define USE_LOCAL_VIEWER\n");

		if (options & GPU_BASIC_SMOOTH)
			BLI_dynstr_append(defs, "#define USE_SMOOTH\n");

		if (options & GPU_BASIC_LIGHTING) {
			BLI_dynstr_append(defs, "#define USE_LIGHTING\n");

			if (options & GPU_BASIC_FAST_LIGHTING)
				BLI_dynstr_append(defs, "#define USE_FAST_LIGHTING\n");
			else
				BLI_dynstr_append(defs, "#define USE_SCENE_LIGHTING\n");
		}

		BASIC_SHADER.gpushader[options] =
			GPU_shader_create(
				BLI_dynstr_get_cstring(vert),
				BLI_dynstr_get_cstring(frag),
				NULL,
				BLI_dynstr_get_cstring(defs));
GPU_CHECK_NO_ERROR();

		if (BASIC_SHADER.gpushader[options] != NULL) {
			int i;

			gpu_init_common(BASIC_SHADER.common + options, BASIC_SHADER.gpushader[options]);
			gpu_set_common (BASIC_SHADER.common + options);

			GPU_shader_bind(BASIC_SHADER.gpushader[options]);
GPU_CHECK_NO_ERROR();

			/* the mapping between samplers and texture units is static, so it can committed here once */
			for (i = 0; i < GPU_MAX_COMMON_SAMPLERS; i++)
				glUniform1i(BASIC_SHADER.common[options].sampler[i], i);
GPU_CHECK_NO_ERROR();
		}
		else {
			BASIC_SHADER.failed[options] = true;
			gpu_set_common(NULL);
		}
	}
	else {
		gpu_set_common(NULL);
	}
}



/* Bind / Unbind */



void GPU_basic_shader_bind(void)
{
	if (GPU_glsl_support())
		basic_shader_bind();

#if defined(WITH_GL_PROFILE_COMPAT)
	/* Only change state if it is different than the Blender default */

	if (BASIC_SHADER.options & GPU_BASIC_LIGHTING)
		glEnable(GL_LIGHTING);

	if (BASIC_SHADER.options & GPU_BASIC_TEXTURE_2D)
		glEnable(GL_TEXTURE_2D);

	if (BASIC_SHADER.options & GPU_BASIC_TWO_SIDE)
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	if (BASIC_SHADER.options & GPU_BASIC_LOCAL_VIEWER)
		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

	if (BASIC_SHADER.options & GPU_BASIC_SMOOTH)
		glShadeModel(GL_SMOOTH);
#endif

	if (BASIC_SHADER.options & GPU_BASIC_LIGHTING) {
GPU_CHECK_NO_ERROR();
		gpu_commit_light();
GPU_CHECK_NO_ERROR();
		gpu_commit_material();
GPU_CHECK_NO_ERROR();

		BASIC_SHADER.need_normals = true; // Temporary hack. Should be solved outside of this file.
	}
	else {
		BASIC_SHADER.need_normals = false; // Temporary hack. Should be solved outside of this file.
	}
}



void GPU_basic_shader_unbind(void)
{
	if (GPU_glsl_support())
		GPU_shader_unbind();

#if defined(WITH_GL_PROFILE_COMPAT)
	/* If GL state was switched from the Blender default then reset it. */

	if (BASIC_SHADER.options & GPU_BASIC_LIGHTING)
		glDisable(GL_LIGHTING);

	if (BASIC_SHADER.options & GPU_BASIC_TEXTURE_2D)
		glDisable(GL_TEXTURE_2D);

	if (BASIC_SHADER.options & GPU_BASIC_TWO_SIDED)
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

	if (BASIC_SHADER.options & GPU_BASIC_LOCAL_VIEWER)
		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);

	if (BASIC_SHADER.options & GPU_BASIC_SMOOTH)
		glShadeModel(GL_FLAT);
#endif

	BASIC_SHADER.need_normals = false; // Temporary hack. Should be solved outside of this file.
}



bool GPU_basic_shader_needs_normals(void)
{
	return BASIC_SHADER.need_normals;
}



