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
* Contributor(s): Alexandr Kuznetsov, Jason Wilkins.
*
* ***** END GPL LICENSE BLOCK *****
*/

/** \file blender/gpu/intern/gpu_immediate_glsl.c
*  \ingroup gpu
*/

#include "gpu_immediate_internal.h"
#include "GPU_matrix.h"
#include "MEM_guardedalloc.h"
#include "BLI_math_vector.h"

#include <string.h>

#include "gpu_extension_wrapper.h"
#include "gpu_glew.h"
#include "gpu_object_gles.h"
#include "GPU_object.h"

typedef struct bufferDataGLSL {
	size_t   size;
#if defined(WITH_GL_PROFILE_CORE) || defined(WITH_GL_PROFILE_COMPAT)
	GLuint   vao;
#endif
	GLuint   vbo;
	GLintptr unalignedPtr;
	GLubyte* mappedBuffer;
	GLubyte* unmappedBuffer;
} bufferDataGLSL;

typedef struct stateDataGLSL {
	GLfloat stateCurrentColor[4];
	GLfloat stateCurrentNormal[3];
} stateDataGLSL;

stateDataGLSL stateData = {{1, 1, 1, 1}, {0, 0, 1}};

#define ALIGN64(p) (((p) + 63) & ~63)

static void allocate(void)
{
	size_t newSize;

	GPU_CHECK_NO_ERROR();

	GPU_IMMEDIATE->stride = gpu_calc_stride();

	newSize = (size_t)(GPU_IMMEDIATE->stride * GPU_IMMEDIATE->maxVertexCount);

	if (GPU_IMMEDIATE->bufferData) {
		bufferDataGLSL* bufferData = (bufferDataGLSL*)GPU_IMMEDIATE->bufferData;

#if defined(WITH_GL_PROFILE_CORE) || defined(WITH_GL_COMPAT)
		if (bufferData->vao != 0)
			glBindVertexArray(bufferData->vao);
#endif

		if (bufferData->vbo != 0)
			gpu_glBindBuffer(GL_ARRAY_BUFFER, bufferData->vbo);

		if (newSize > bufferData->size) {
			if (bufferData->vbo != 0)
				gpu_glBufferData(GL_ARRAY_BUFFER, newSize, NULL, GL_STREAM_DRAW);

			if (bufferData->unalignedPtr != 0) {
				bufferData->unalignedPtr   = (GLintptr)MEM_reallocN((GLubyte*)(bufferData->unalignedPtr), newSize+63);
				bufferData->unmappedBuffer = (GLubyte*)ALIGN64(bufferData->unalignedPtr);
			}

			bufferData->size = newSize;
		}
	}
	else {
		bufferDataGLSL* bufferData = (bufferDataGLSL*)MEM_callocN(sizeof(bufferDataGLSL), "bufferDataGLSL");

#if defined(WITH_GL_PROFILE_CORE) || defined(WITH_GL_COMPAT)
		bufferData->vao = 0;
#endif

		bufferData->vbo = 0;

#if defined(WITH_GL_PROFILE_CORE) || defined(WITH_GL_COMPAT)
		if (GLEW_VERSION_3_0 || GLEW_ARB_vertex_array_object) {
			glGenVertexArrays(1, &(bufferData->vao));
			GPU_ASSERT(bufferData->vao != 0);
			glBindVertexArray(bufferData->vao);
		}
#endif

		if (GLEW_VERSION_1_5 || GLEW_ES_VERSION_2_0 || GLEW_ARB_vertex_buffer_object) {
			gpu_glGenBuffers(1, &(bufferData->vbo));
			GPU_ASSERT(bufferData->vbo != 0);
			gpu_glBindBuffer(GL_ARRAY_BUFFER, bufferData->vbo);
			gpu_glBufferData(GL_ARRAY_BUFFER, newSize, NULL, GL_STREAM_DRAW);
		}

		if (GLEW_VERSION_1_5 || GLEW_OES_mapbuffer || GLEW_ARB_vertex_buffer_object) {
			bufferData->unalignedPtr   = 0;
			bufferData->unmappedBuffer = NULL;
		}
		else {
			bufferData->unalignedPtr   = (GLintptr)MEM_mallocN(newSize+63, "bufferDataGLSL->unalignedPtr");
			bufferData->unmappedBuffer = (void*)ALIGN64(bufferData->unalignedPtr);
		}

		bufferData->size = newSize;

		GPU_IMMEDIATE->bufferData = bufferData;
	}

	GPU_CHECK_NO_ERROR();
}



static void setup(void)
{
	size_t i;
	size_t offset = 0;
	bufferDataGLSL* bufferData = (bufferDataGLSL*)(GPU_IMMEDIATE->bufferData);

	GLubyte* base = bufferData->vbo != 0 ? NULL : (GLubyte*)(bufferData->unmappedBuffer);

	GPU_CHECK_NO_ERROR();

	/* vertex */

	gpugameobj.gpuVertexPointer(
		GPU_IMMEDIATE->format.vertexSize,
		GL_FLOAT,
		GPU_IMMEDIATE->stride,
		base + offset);

	offset += (size_t)(GPU_IMMEDIATE->format.vertexSize) * sizeof(GLfloat);

	/* normal */

	GPU_CHECK_NO_ERROR();
	if (GPU_IMMEDIATE->format.normalSize != 0) {
		gpugameobj.gpuNormalPointer(
			GL_FLOAT,
			GPU_IMMEDIATE->stride,
			base + offset);

		offset += 3 * sizeof(GLfloat);
	}

	/* color */

	GPU_CHECK_NO_ERROR();
	if (GPU_IMMEDIATE->format.colorSize != 0) {
		gpugameobj.gpuColorPointer(
			4 * sizeof(GLubyte),
			GL_UNSIGNED_BYTE,
			GPU_IMMEDIATE->stride,
			base + offset);

		/* 4 bytes are always reserved for color, for efficient memory alignment */
		offset += 4 * sizeof(GLubyte);
	}

	/* texture coordinate */

	GPU_CHECK_NO_ERROR();
	if (GPU_IMMEDIATE->format.textureUnitCount == 1) {
		gpugameobj.gpuTexCoordPointer(
			GPU_IMMEDIATE->format.texCoordSize[0],
			GL_FLOAT,
			GPU_IMMEDIATE->stride,
			base + offset);

		offset +=
			(size_t)(GPU_IMMEDIATE->format.texCoordSize[0]) * sizeof(GLfloat);

	}
	else if (GPU_IMMEDIATE->format.textureUnitCount > 1) {
		for (i = 0; i < GPU_IMMEDIATE->format.textureUnitCount; i++) {
			/*glClientActiveTexture(GPU_IMMEDIATE->format.textureUnitMap[i]);

			glTexCoordPointer(
				GPU_IMMEDIATE->format.texCoordSize[i],
				GL_FLOAT,
				GPU_IMMEDIATE->stride,
				base + offset);*/

			offset +=
				(size_t)(GPU_IMMEDIATE->format.texCoordSize[i]) * sizeof(GLfloat);

		//	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		}

	//	glClientActiveTexture(GL_TEXTURE0);
	}

	/* float vertex attribute */

	GPU_CHECK_NO_ERROR();
	for (i = 0; i < GPU_IMMEDIATE->format.attribCount_f; i++) {
	/*	gpu_glVertexAttribPointer(
			GPU_IMMEDIATE->format.attribIndexMap_f[i],
			GPU_IMMEDIATE->format.attribSize_f[i],
			GL_FLOAT,
			GPU_IMMEDIATE->format.attribNormalized_f[i],
			GPU_IMMEDIATE->stride,
			base + offset);*/

		offset +=
			(size_t)(GPU_IMMEDIATE->format.attribSize_f[i]) * sizeof(GLfloat);

		/*gpu_glEnableVertexAttribArray(
			GPU_IMMEDIATE->format.attribIndexMap_f[i]);*/
	}

	/* byte vertex attribute */

	GPU_CHECK_NO_ERROR();
	for (i = 0; i < GPU_IMMEDIATE->format.attribCount_ub; i++) {
		if (GPU_IMMEDIATE->format.attribSize_ub[i] > 0) {
			/*glVertexAttribPointer(
				GPU_IMMEDIATE->format.attribIndexMap_ub[i],
				GPU_IMMEDIATE->format.attribSize_ub[i],
				GL_UNSIGNED_BYTE,
				GPU_IMMEDIATE->format.attribNormalized_ub[i],
				GPU_IMMEDIATE->stride,
				base + offset);*/

			offset += 4 * sizeof(GLubyte);

			/*glEnableVertexAttribArray(
				GPU_IMMEDIATE->format.attribIndexMap_ub[i]);*/
		}
	}

	GPU_CHECK_NO_ERROR();
}



typedef struct indexBufferDataGLSL {
	GLuint vbo;
	GLintptr unalignedPtr;
	GLubyte* unmappedBuffer;
	GLubyte* mappedBuffer;
	size_t size;
} indexBufferDataGLSL;

static void allocateIndex(void)
{
	if (GPU_IMMEDIATE->index) {
		GPUindex* index;
		size_t newSize;

		index = GPU_IMMEDIATE->index;

		switch(index->type) {
		case GL_UNSIGNED_BYTE:
			newSize = index->maxIndexCount * sizeof(GLubyte);
			break;
		case GL_UNSIGNED_SHORT:
			newSize = index->maxIndexCount * sizeof(GLushort);
			break;
		case GL_UNSIGNED_INT:
			newSize = index->maxIndexCount * sizeof(GLuint);
			break;
		default:
			GPU_ABORT();
			return;
		}

		if (index->bufferData) {
			indexBufferDataGLSL* bufferData = (indexBufferDataGLSL*)(index->bufferData);

			if (bufferData->vbo != 0)
				gpu_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferData->vbo);

			if (newSize > bufferData->size) {
				if (bufferData->vbo)
					gpu_glBufferData(GL_ELEMENT_ARRAY_BUFFER, newSize, NULL, GL_STREAM_DRAW);

				if (bufferData->unalignedPtr != 0) {
					bufferData->unalignedPtr   = (GLintptr)MEM_reallocN((GLubyte*)(bufferData->unalignedPtr), newSize+63);
					bufferData->unmappedBuffer = (GLubyte*)ALIGN64(bufferData->unalignedPtr);
				}

				bufferData->size = newSize;
			}
		}
		else {
			indexBufferDataGLSL* bufferData = (indexBufferDataGLSL*)MEM_callocN(sizeof(indexBufferDataGLSL), "indexBufferDataGLSL");

			bufferData->vbo = 0;

			if (GLEW_VERSION_1_5 || GLEW_ES_VERSION_2_0 || GLEW_ARB_vertex_buffer_object) {
				gpu_glGenBuffers(1, &(bufferData->vbo));
				GPU_ASSERT(bufferData->vbo != 0);
				gpu_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferData->vbo);
				gpu_glBufferData(GL_ELEMENT_ARRAY_BUFFER, newSize, NULL, GL_STREAM_DRAW);
			}

			if (GLEW_VERSION_1_5 || GLEW_OES_mapbuffer || GLEW_ARB_vertex_buffer_object) {
				bufferData->unalignedPtr   = 0;
				bufferData->unmappedBuffer = NULL;
			}
			else {
				bufferData->unalignedPtr   = (GLintptr)MEM_mallocN(newSize+63, "indexBufferData->unalignedPtr");
				bufferData->unmappedBuffer = (GLubyte*)ALIGN64(bufferData->unalignedPtr);
			}

			bufferData->size = newSize;

			index->bufferData = bufferData;
		}

		GPU_CHECK_NO_ERROR();
	}
}



void gpu_lock_buffer_glsl(void)
{
	allocate();
	allocateIndex();
	setup();
}



void gpu_begin_buffer_glsl(void)
{
	bufferDataGLSL* bufferData = (bufferDataGLSL*)(GPU_IMMEDIATE->bufferData);

	bufferData->mappedBuffer =
		(GLubyte*)GPU_buffer_start_update(GL_ARRAY_BUFFER, bufferData->unmappedBuffer);

	GPU_IMMEDIATE->mappedBuffer = bufferData->mappedBuffer;
}



static const size_t VQEOS_SIZE =  3 * 65536 / 2;
static const size_t VQEOC_SIZE =  3 *   256 / 2;

static GLushort* vqeos;
static GLubyte*  vqeoc;

static GLuint vq[2];

void gpu_quad_elements_init(void)
{
	/* init once*/
	static char init = 0;

	int i, j;

	if (init) return;

	init = 1;

	vqeos = (GLushort*)MEM_mallocN(VQEOS_SIZE * sizeof(GLushort), "vqeos");
	vqeoc = (GLubyte* )MEM_mallocN(VQEOC_SIZE * sizeof(GLubyte ), "vqeoc");

	for (i = 0, j = 0; i < 65535; i++, j++) {
		vqeos[j] = i;

		if (i%4 == 3) {
			vqeos[++j] = i-3;
			vqeos[++j] = i-1;
		}
	}

	for (i = 0, j = 0; i < 255; i++, j++) {
		vqeoc[j] = i;

		if(i%4 == 3) {
			vqeoc[++j] = i-3;
			vqeoc[++j] = i-1;
		}
	}

	if (GLEW_VERSION_1_5 || GLEW_ES_VERSION_2_0 || GLEW_ARB_vertex_buffer_object) {
		gpu_glGenBuffers(2, vq);

		gpu_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vq[0]);
		gpu_glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * VQEOS_SIZE, vqeos, GL_STATIC_DRAW);

		gpu_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vq[1]);
		gpu_glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte ) * VQEOC_SIZE, vqeoc, GL_STATIC_DRAW);

		MEM_freeN(vqeos);
		MEM_freeN(vqeoc);

		vqeos = NULL;
		vqeoc = NULL;
	}
}



void gpu_end_buffer_glsl(void)
{
	bufferDataGLSL* bufferData = (bufferDataGLSL*)(GPU_IMMEDIATE->bufferData);

	if (bufferData->mappedBuffer != NULL) {
		GPU_buffer_finish_update(GL_ARRAY_BUFFER, GPU_IMMEDIATE->offset, bufferData->mappedBuffer);

		bufferData   ->mappedBuffer = NULL;
		GPU_IMMEDIATE->mappedBuffer = NULL;
	}

	if (!(GPU_IMMEDIATE->mode == GL_NOOP || GPU_IMMEDIATE->count == 0)) {
		GPU_CHECK_NO_ERROR();

		gpuMatrixCommit();

		if(GPU_IMMEDIATE->format.colorSize == 0 && curglslesi && (curglslesi->colorloc != -1)) {
			glVertexAttrib4fv(curglslesi->colorloc, stateData.stateCurrentColor);
		}

		GPU_CHECK_NO_ERROR();
		if (GPU_IMMEDIATE->mode != GL_QUADS) {
			glDrawArrays(GPU_IMMEDIATE->mode, 0, GPU_IMMEDIATE->count);
		GPU_CHECK_NO_ERROR();
		}
		else {
			if (GPU_IMMEDIATE->count <= 255){
				if (GLEW_VERSION_1_5 || GLEW_ES_VERSION_2_0 || GLEW_ARB_vertex_buffer_object)
					gpu_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vq[1]);
		GPU_CHECK_NO_ERROR();

				glDrawElements(GL_TRIANGLES, 3 * GPU_IMMEDIATE->count / 2, GL_UNSIGNED_BYTE, vqeoc);
		GPU_CHECK_NO_ERROR();
			}
			else if(GPU_IMMEDIATE->count <= 65535) {
				if (GLEW_VERSION_1_5 || GLEW_ES_VERSION_2_0 || GLEW_ARB_vertex_buffer_object)
					gpu_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vq[0]);

		GPU_CHECK_NO_ERROR();
				glDrawElements(GL_TRIANGLES, 3 * GPU_IMMEDIATE->count / 2, GL_UNSIGNED_SHORT, vqeos);
		GPU_CHECK_NO_ERROR();
			}
			else {
				printf("To big GL_QUAD object to draw. Vertices: %i", GPU_IMMEDIATE->count);
			}

			if (GLEW_VERSION_1_5 || GLEW_ES_VERSION_2_0 || GLEW_ARB_vertex_buffer_object)
				gpu_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ((indexBufferDataGLSL*)(GPU_IMMEDIATE->index->bufferData))->vbo);
		}

		GPU_CHECK_NO_ERROR();
	}
}



void gpu_unlock_buffer_glsl(void)
{
	GPU_CHECK_NO_ERROR();

	gpugameobj.gpuCleanupAfterDraw();

	GPU_CHECK_NO_ERROR();
}



void gpu_shutdown_buffer_glsl(GPUimmediate *restrict immediate)
{
	if (immediate->bufferData) {
		bufferDataGLSL* bufferData = (bufferDataGLSL*)(immediate->bufferData);

		if (bufferData->unalignedPtr != 0) {
			MEM_freeN((GLubyte*)(bufferData->unalignedPtr));
		}

#if defined(WITH_GL_PROFILE_CORE) || defined(WITH_GL_PROFILE_COMPAT)
		if (bufferData->vao != 0)
			glDeleteVertexArrays(1, &(bufferData->vao));
#endif

		if (bufferData->vbo != 0)
			gpu_glDeleteBuffers(1, &(bufferData->vbo));

		MEM_freeN(immediate->bufferData);
		immediate->bufferData = NULL;

		gpu_index_shutdown_buffer_glsl(immediate->index);
	}
}



void gpu_index_shutdown_buffer_glsl(GPUindex *restrict index)
{
	if (index && index->bufferData) {
		indexBufferDataGLSL* bufferData = (indexBufferDataGLSL*)(index->bufferData);

		if (bufferData->vbo != 0)
			gpu_glDeleteBuffers(1, &(bufferData->vbo));

		if (bufferData->unalignedPtr != 0)
			MEM_freeN((GLubyte*)(bufferData->unalignedPtr));

		MEM_freeN(index->bufferData);
	}
}



void gpu_current_color_glsl(void)
{
	stateData.stateCurrentColor[0] = (GLfloat)(GPU_IMMEDIATE->color[0]) / 255.0f;
	stateData.stateCurrentColor[1] = (GLfloat)(GPU_IMMEDIATE->color[1]) / 255.0f;
	stateData.stateCurrentColor[2] = (GLfloat)(GPU_IMMEDIATE->color[2]) / 255.0f;
	stateData.stateCurrentColor[3] = (GLfloat)(GPU_IMMEDIATE->color[3]) / 255.0f;

	//if(curglslesi && (curglslesi->colorloc != -1)) {
	//	glVertexAttrib4fv(curglslesi->colorloc, stateData.stateCurrentColor);
	//}
}



void gpu_get_current_color_glsl(GLfloat *color)
{
	copy_v4_v4(color, stateData.stateCurrentColor);
}



void gpu_current_normal_glsl(void)
{
	copy_v3_v3(stateData.stateCurrentNormal, GPU_IMMEDIATE->normal);
}



void gpu_index_begin_buffer_glsl(void)
{
	GPUindex *restrict   index      = GPU_IMMEDIATE->index;
	indexBufferDataGLSL* bufferData = (indexBufferDataGLSL*)(index->bufferData);

	bufferData->mappedBuffer =
		(GLubyte*)GPU_buffer_start_update(GL_ELEMENT_ARRAY_BUFFER, bufferData->unmappedBuffer);

	index->mappedBuffer = bufferData->mappedBuffer;
}



void gpu_index_end_buffer_glsl(void)
{
	GPUindex *restrict   index      = GPU_IMMEDIATE->index;
	indexBufferDataGLSL* bufferData = (indexBufferDataGLSL*)(index->bufferData);

	GPU_buffer_finish_update(GL_ELEMENT_ARRAY_BUFFER, index->offset, bufferData->mappedBuffer);

	bufferData->mappedBuffer = NULL;
	index     ->mappedBuffer = NULL;
}



void gpu_draw_elements_glsl(void)
{
	GPUindex* index = GPU_IMMEDIATE->index;
	indexBufferDataGLSL* bufferData = (indexBufferDataGLSL*)(index->bufferData);

	GPU_CHECK_NO_ERROR();

	gpuMatrixCommit();

	if(GPU_IMMEDIATE->format.colorSize == 0 && curglslesi && (curglslesi->colorloc != -1))
		glVertexAttrib4fv(curglslesi->colorloc, stateData.stateCurrentColor);

	GPU_CHECK_NO_ERROR();

	glDrawElements(
		GPU_IMMEDIATE->mode,
		index->count,
		index->type,
		bufferData->vbo != 0 ? NULL : bufferData->unmappedBuffer);

	GPU_CHECK_NO_ERROR();
}

void gpu_draw_range_elements_glsl(void)
{
	GPUindex* index = GPU_IMMEDIATE->index;
	indexBufferDataGLSL* bufferData = (indexBufferDataGLSL*)(index->bufferData);

	GPU_CHECK_NO_ERROR();

	if(GPU_IMMEDIATE->format.colorSize == 0 && curglslesi && (curglslesi->colorloc != -1))
		glVertexAttrib4fv(curglslesi->colorloc, stateData.stateCurrentColor);

	gpuMatrixCommit();

#if defined(WITH_GL_PROFILE_CORE) || defined(WITH_GL_PROFILE_COMPAT)
	glDrawRangeElements(
		GPU_IMMEDIATE->mode,
		index->indexMin,
		index->indexMax,
		index->count,
		index->type,
		bufferData->vbo != 0 ? NULL : bufferData->unmappedBuffer);
#else
	glDrawElements(
		GPU_IMMEDIATE->mode,
		index->count,
		index->type,
		bufferData->vbo != 0 ? NULL : bufferData->unmappedBuffer);
#endif

	GPU_CHECK_NO_ERROR();
}
