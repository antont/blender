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

/** \file source/blender/gpu/intern/gpu_aspect.c
 *  \ingroup gpu
 */

/* my interface */
#include "intern/gpu_aspect_intern.h"

/* my library */
#include "GPU_safety.h"

/* external */
#include "MEM_guardedalloc.h"



static GPUaspectimpl** GPU_ASPECT_FUNCS = NULL;

static size_t aspect_max  = 0;
static size_t aspect_free = 0;
static size_t aspect_fill = 0;

static GPUaspectimpl dummy = { NULL };

static uint32_t    current_aspect = -1;
static const void* current_object = NULL;



void gpu_aspect_init(void)
{
	const size_t count = 100;

	GPU_ASPECT_FUNCS = (GPUaspectimpl**)MEM_callocN(count * sizeof(GPUaspectimpl*), "GPU aspect function array");

	aspect_max  = count;
	aspect_free = count;
	aspect_fill = 0;
}



void gpu_aspect_exit(void)
{
	MEM_freeN(GPU_ASPECT_FUNCS);
	GPU_ASPECT_FUNCS = NULL;

	aspect_max   = 0;
	aspect_fill  = 0;
	aspect_free  = 0;
	
	current_aspect = -1;
}



void GPU_commit_aspect(void)
{
	GPUaspectimpl* aspectImpl = GPU_ASPECT_FUNCS[current_aspect];

	GPU_ASSERT(current_aspect != -1);

	if (aspectImpl != NULL && aspectImpl->commit != NULL )
		aspectImpl->commit(aspectImpl->param);
}



void GPU_gen_aspects(size_t count, uint32_t* aspects)
{
	uint32_t src, dst;

	if (count == 0) {
		return;
	}

	if (count > aspect_free) {
		aspect_max   = aspect_max + count - aspect_free;
		GPU_ASPECT_FUNCS = (GPUaspectimpl**)MEM_reallocN(GPU_ASPECT_FUNCS, aspect_max * sizeof(GPUaspectimpl*));
		aspect_free  = count;
	}

	src = aspect_fill;
	dst = 0;

	while (dst < count) {
		if (!GPU_ASPECT_FUNCS[src]) {
			GPU_ASPECT_FUNCS[src] = &dummy;
			aspects[dst] = src;
			dst++;
			aspect_fill = dst;
			aspect_free--;
		}

		src++;
	}
}



void GPU_delete_aspects(size_t count, const uint32_t* aspects)
{
	uint32_t i;

	for (i = 0; i < count; i++) {
		if (aspects[i] < aspect_fill) {
			aspect_fill = aspects[i];
		}

		GPU_ASPECT_FUNCS[aspects[i]] = NULL;
	}
}



void GPU_aspect_impl(uint32_t aspect, GPUaspectimpl* aspectImpl)
{
	GPU_ASPECT_FUNCS[aspect] = aspectImpl;
}



bool GPU_aspect_begin(uint32_t aspect, const void* object)
{
	GPUaspectimpl* aspectImpl;

	GPU_ASSERT(current_aspect == -1);

	current_aspect = aspect;
	current_object = object;

	aspectImpl = GPU_ASPECT_FUNCS[aspect];
	return (aspectImpl != NULL && aspectImpl->begin != NULL) ? aspectImpl->begin(aspectImpl->param, object) : true;
}



bool GPU_aspect_end(void)
{
	GPUaspectimpl* aspectImpl = GPU_ASPECT_FUNCS[current_aspect];
	const void*     object      = current_object;

	GPU_ASSERT(current_aspect != -1);

	current_aspect = -1;
	current_object = NULL;

	return (aspectImpl  != NULL && aspectImpl->end != NULL) ? aspectImpl->end(aspectImpl->param, object) : true;
}



void GPU_aspect_enable(uint32_t aspect, uint32_t options)
{
	GPUaspectimpl* aspectImpl = GPU_ASPECT_FUNCS[aspect];

	if (aspectImpl != NULL && aspectImpl->enable != NULL)
		aspectImpl->enable(aspectImpl->param, options);
}



void GPU_aspect_disable(uint32_t aspect, uint32_t options)
{
	GPUaspectimpl* aspectImpl = GPU_ASPECT_FUNCS[aspect];

	if (aspectImpl != NULL && aspectImpl->disable != NULL )
		aspectImpl->disable(aspectImpl->param, options);
}
