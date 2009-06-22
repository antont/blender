/**
 * $Id: gpu_buffers.h 20687 2009-06-07 11:26:46Z imbusy $
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version. The Blender
 * Foundation also sells licenses for use in proprietary software under
 * the Blender License.  See http://www.blender.org/BL/ for information
 * about this.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2005 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Brecht Van Lommel.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#ifndef __GPU_BUFFERS_H__
#define __GPU_BUFFERS_H__

#define MAX_FREE_GPU_BUFFERS 8

struct DerivedMesh;

typedef struct GPUBuffer
{
	int size;	/* in bytes */
	void *pointer;	/* used with vertex arrays */
	unsigned int id;	/* used with vertex buffer objects */
} GPUBuffer;

typedef struct GPUBufferPool
{
	int size;	/* number of allocated buffers stored */
	int start;	/* for a queue like structure */
				/* when running out of space for storing buffers,
				the last one used will be thrown away */

	GPUBuffer* buffers[MAX_FREE_GPU_BUFFERS];
} GPUBufferPool;

typedef struct GPUBufferMaterial
{
	int start;	/* at which vertex in the buffer the material starts */
	int end;	/* at which vertex it ends */
	char mat_nr;
} GPUBufferMaterial;

typedef struct GPUDrawObject
{
	GPUBuffer *vertices;
	GPUBuffer *normals;
	GPUBuffer *uv;
	GPUBuffer *colors;

	GPUBufferMaterial *materials;

	int nmaterials;
	int nelements;
} GPUDrawObject;

GPUBufferPool *GPU_buffer_pool_new();
void GPU_buffer_pool_free( GPUBufferPool *pool );

GPUBuffer *GPU_buffer_alloc( int size, GPUBufferPool *pool );
void GPU_buffer_free( GPUBuffer *buffer, GPUBufferPool *pool );

GPUDrawObject *GPU_drawobject_new( struct DerivedMesh *dm );
void GPU_drawobject_free( GPUDrawObject *object );

/* called before drawing */
void GPU_vertex_setup( struct DerivedMesh *dm );
void GPU_normal_setup( struct DerivedMesh *dm );
void GPU_uv_setup( struct DerivedMesh *dm );
void GPU_color_setup( struct DerivedMesh *dm );

/* upload three unsigned chars, representing RGB colors, for each vertex */
void GPU_color3_upload( struct DerivedMesh *dm, char *data );
/* upload four unsigned chars, representing RGBA colors, for each vertex */
void GPU_color4_upload( struct DerivedMesh *dm, char *data );

/* called after drawing */
void GPU_buffer_unbind();

#endif
