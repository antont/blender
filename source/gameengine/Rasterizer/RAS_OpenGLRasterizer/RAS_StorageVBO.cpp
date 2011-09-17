/*
 * $Id$
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
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include "RAS_StorageVBO.h"
#include "RAS_MeshObject.h"

#include "GL/glew.h"

VBO::VBO(RAS_DisplayArray *data, unsigned int indices)
{
	this->data = data;
	this->size = data->m_vertex.size();
	this->indices = indices;
	this->stride = 28*sizeof(GLfloat);

	//	Determine drawmode
	if (data->m_type == data->QUAD)
		this->mode = GL_QUADS;
	else if (data->m_type == data->TRIANGLE)
		this->mode = GL_TRIANGLES;
	else
		this->mode = GL_LINE;

	// Generate Buffers
	glGenBuffersARB(1, &this->ibo);
	glGenBuffersARB(1, &this->vbo_id);

	// Allocate some space to gather data into before uploading to GPU
	this->vbo = new GLfloat[this->stride*this->size];

	// Fill the buffers with initial data
	UpdateIndices();
	UpdateData();

	// Establish offsets
	this->vertex_offset = 0;
	this->normal_offset = (void*)(3*sizeof(GLfloat));
	this->tangent_offset = (void*)(6*sizeof(GLfloat));
	this->color_offset = (void*)(10*sizeof(GLfloat));
	this->uv_offset = (void*)(11*sizeof(GLfloat));
	this->dummy_offset = (void*)(27*sizeof(GLfloat));
}

VBO::~VBO()
{
	glDeleteBuffersARB(1, &this->ibo);
	glDeleteBuffersARB(1, &this->vbo_id);

	delete this->vbo;
}

void VBO::UpdateData()
{
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vbo_id);

	// Lets the video card know we are done with the old VBO
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, 0, NULL, GL_DYNAMIC_DRAW_ARB);

	// Gather data
	for (unsigned int i=0, j=0; i<data->m_vertex.size(); i++, j+=this->stride/sizeof(GLfloat))
	{
		memcpy(&this->vbo[j], data->m_vertex[i].getXYZ(), sizeof(float)*3);
		memcpy(&this->vbo[j+3], data->m_vertex[i].getNormal(), sizeof(float)*3);
		memcpy(&this->vbo[j+6], data->m_vertex[i].getTangent(), sizeof(float)*4);
		memcpy(&this->vbo[j+10], data->m_vertex[i].getRGBA(), sizeof(char)*4);

		for (unsigned int k=0; k<RAS_TexVert::MAX_UNIT; k+=2)
			memcpy(&this->vbo[j+11+k], data->m_vertex[i].getUV(k), sizeof(float)*2);
	}

	// Upload Data to GPU
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, this->size*this->stride, this->vbo, GL_DYNAMIC_DRAW_ARB);
}

void VBO::UpdateIndices()
{
	int space = data->m_index.size() * sizeof(GLushort);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, this->ibo);

	// Lets the video card know we are done with the old VBO
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0, NULL, GL_DYNAMIC_DRAW_ARB);

	// Upload Data to VBO
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, space, &data->m_index[0], GL_DYNAMIC_DRAW_ARB);
}

void VBO::Draw(int texco_num, RAS_IRasterizer::TexCoGen* texco, int attrib_num, RAS_IRasterizer::TexCoGen* attrib, bool multi)
{
	// Bind buffers
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, this->ibo);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vbo_id);

	// Vertexes
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, this->stride, this->vertex_offset);

	// Normals
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, this->stride, this->normal_offset);

	// Colors
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_UNSIGNED_BYTE, this->stride, this->color_offset);

	if (multi)
	{
		for (int unit=0; unit<texco_num; ++unit)
		{
			glClientActiveTexture(GL_TEXTURE0_ARB + unit);

			switch(texco[unit])
			{
				case RAS_IRasterizer::RAS_TEXCO_ORCO:
				case RAS_IRasterizer::RAS_TEXCO_GLOB:
					glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vbo_id);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(3, GL_FLOAT, this->stride, this->vertex_offset);
					break;
				case RAS_IRasterizer::RAS_TEXCO_UV:
					glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vbo_id);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(2, GL_FLOAT, this->stride, (void*)((intptr_t)this->uv_offset+(sizeof(GLfloat)*2*unit)));
					break;
				case RAS_IRasterizer::RAS_TEXCO_NORM:
					glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vbo_id);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(3, GL_FLOAT, this->stride, this->normal_offset);
					break;
				case RAS_IRasterizer::RAS_TEXTANGENT:
					glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vbo_id);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(4, GL_FLOAT, this->stride, this->tangent_offset);
					break;
				default:
					glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vbo_id);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(1, GL_SHORT, this->stride, this->dummy_offset);
					break;
			}
		}
	}
	else //TexFace
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vbo_id);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, this->stride, this->uv_offset);
	}

	if (GLEW_ARB_vertex_program)
	{
		int uv = 0;
		for (int unit=0; unit<attrib_num; ++unit)
		{
			switch(attrib[unit])
			{
				case RAS_IRasterizer::RAS_TEXCO_ORCO:
				case RAS_IRasterizer::RAS_TEXCO_GLOB:
					glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vbo_id);
					glVertexAttribPointerARB(unit, 3, GL_FLOAT, GL_FALSE, this->stride, this->vertex_offset);
					glEnableVertexAttribArrayARB(unit);
					break;
				case RAS_IRasterizer::RAS_TEXCO_UV:
					glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vbo_id);
					glVertexAttribPointerARB(unit, 2, GL_FLOAT, GL_FALSE, this->stride, (void*)((intptr_t)this->uv_offset+(sizeof(GLfloat)*2*uv++)));
					glEnableVertexAttribArrayARB(unit);
					break;
				case RAS_IRasterizer::RAS_TEXCO_NORM:
					glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vbo_id);
					glVertexAttribPointerARB(unit, 2, GL_FLOAT, GL_FALSE, stride, this->normal_offset);
					glEnableVertexAttribArrayARB(unit);
					break;
				case RAS_IRasterizer::RAS_TEXTANGENT:
					glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vbo_id);
					glVertexAttribPointerARB(unit, 4, GL_FLOAT, GL_FALSE, this->stride, this->tangent_offset);
					glEnableVertexAttribArrayARB(unit);
					break;
				default:
					break;
			}
		}
	}
	
	glDrawElements(this->mode, this->indices, GL_UNSIGNED_SHORT, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (GLEW_ARB_vertex_program)
	{
		for (int i=0; i<attrib_num; ++i)
			glDisableVertexAttribArrayARB(i);
	}

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
 }

RAS_StorageVBO::RAS_StorageVBO(int *texco_num, RAS_IRasterizer::TexCoGen *texco, int *attrib_num, RAS_IRasterizer::TexCoGen *attrib):
	m_texco_num(texco_num),
	m_texco(texco),
	m_attrib_num(attrib_num),
	m_attrib(attrib)
{
}

RAS_StorageVBO::~RAS_StorageVBO()
{
}

bool RAS_StorageVBO::Init()
{
	return true;
}

void RAS_StorageVBO::Exit()
{
	m_vbo_lookup.clear();
}

void RAS_StorageVBO::IndexPrimitives(RAS_MeshSlot& ms)
{
	IndexPrimitivesInternal(ms, false);
}

void RAS_StorageVBO::IndexPrimitivesMulti(RAS_MeshSlot& ms)
{
	IndexPrimitivesInternal(ms, true);
}

void RAS_StorageVBO::IndexPrimitivesInternal(RAS_MeshSlot& ms, bool multi)
{
	RAS_MeshSlot::iterator it;
	VBO *vbo;
	
	for (ms.begin(it); !ms.end(it); ms.next(it))
	{
		vbo = m_vbo_lookup[it.array];

		if(vbo == 0)
			m_vbo_lookup[it.array] = vbo = new VBO(it.array, it.totindex);

		// Update the vbo
		if (ms.m_mesh->MeshModified())
		{
			vbo->UpdateData();
		}

		vbo->Draw(*m_texco_num, m_texco, *m_attrib_num, m_attrib, multi);
	}
}
