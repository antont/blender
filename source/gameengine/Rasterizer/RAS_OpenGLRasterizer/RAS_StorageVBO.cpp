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

#include "GL/glew.h"

VBO::VBO(RAS_DisplayArray *data, unsigned int indices)
{
	GLuint vbo;
	RAS_MeshSlot::iterator it;

	this->data = data;
	this->size = data->m_vertex.size();
	this->indices = indices;

	//	Determine drawmode
	if (data->m_type == data->QUAD)
		this->mode = GL_QUADS;
	else if (data->m_type == data->TRIANGLE)
		this->mode = GL_TRIANGLES;
	else
		this->mode = GL_LINE;

	// Generate Buffers
	glGenBuffersARB(1, &this->vertex);
	glGenBuffersARB(1, &this->normal);
	glGenBuffersARB(2, this->UV);
	glGenBuffersARB(1, &this->tangent);
	glGenBuffersARB(1, &this->ibo);
	glGenBuffersARB(1, &this->dummy);

	// Fill the buffers with initial data
	UpdatePositions();
	UpdateNormals();
	UpdateUVs();
	UpdateTangents();
	UpdateIndices();

	// Set up a dummy buffer
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->dummy);
	GLbyte* dummy = new GLbyte [this->size];
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(GLbyte), dummy, GL_STATIC_DRAW_ARB);
	delete dummy;
}

VBO::~VBO()
{
	glDeleteBuffersARB(1, &this->ibo);
	glDeleteBuffersARB(1, &this->vertex);
	glDeleteBuffersARB(1, &this->normal);
	glDeleteBuffersARB(2, this->UV);
	glDeleteBuffersARB(1, &this->tangent);
}

void VBO::UpdatePositions()
{
	int space = this->size*3*sizeof(GLfloat);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vertex);

	// Lets the video card know we are done with the old VBO
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, 0, NULL, GL_DYNAMIC_DRAW_ARB);

	// Gather position data
	GLfloat* positions = new GLfloat[this->size*3];
	for (unsigned int i=0, j=0; i<data->m_vertex.size(); ++i, j+=3)
	{
		memcpy(&positions[j], data->m_vertex[i].getXYZ(), sizeof(float)*3);
	}

	// Upload Data to VBO
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, space, positions, GL_DYNAMIC_DRAW_ARB);

	// Clean up
	delete positions;
}

void VBO::UpdateNormals()
{
	int space = this->size*3*sizeof(GLfloat);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->normal);

	// Lets the video card know we are done with the old VBO
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, 0, NULL, GL_DYNAMIC_DRAW_ARB);

	// Gather normal data
	GLfloat* normals = new GLfloat[this->size*3];
	for (unsigned int i=0, j=0; i<data->m_vertex.size(); ++i, j+=3)
	{
		memcpy(&normals[j], data->m_vertex[i].getNormal(), sizeof(float)*3);
	}

	// Upload Data to VBO
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, space, normals, GL_DYNAMIC_DRAW_ARB);

	// Clean up
	delete normals;
}

void VBO::UpdateUVs()
{
	GLfloat* uvs;

	int space = this->size*2*sizeof(GLfloat);

	for (int uv=0; uv<2; ++uv)
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->UV[uv]);

		// Lets the video card know we are done with the old VBO
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, 0, NULL, GL_DYNAMIC_DRAW_ARB);

		// Gather uv data
		uvs = new GLfloat[this->size*2];
		for (unsigned int i=0, j=0; i<data->m_vertex.size(); ++i, j+=2)
		{
			if (uv == 0)
				memcpy(&uvs[j], data->m_vertex[i].getUV1(), sizeof(float)*2);
			else if (uv == 1)
				memcpy(&uvs[j], data->m_vertex[i].getUV2(), sizeof(float)*2);
		}

		// Upload Data to VBO
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, space, uvs, GL_DYNAMIC_DRAW_ARB);
	}

	// Clean up
	delete uvs;
}

void VBO::UpdateTangents()
{
	int space = this->size*4*sizeof(GLfloat);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->tangent);

	// Lets the video card know we are done with the old VBO
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, 0, NULL, GL_DYNAMIC_DRAW_ARB);

	// Gather tangent data
	GLfloat* tangents = new GLfloat[this->size*4];
	for (unsigned int i=0, j=0; i<data->m_vertex.size(); ++i, j+=4)
	{
		memcpy(&tangents[j], data->m_vertex[i].getTangent(), sizeof(float)*4);
	}

	// Upload Data to VBO
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, space, tangents, GL_DYNAMIC_DRAW_ARB);

	// Clean up
	delete tangents;
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
	// Indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, this->ibo);

	// Vertexes
	glBindBuffer(GL_ARRAY_BUFFER_ARB, this->vertex);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, 0);

	// Normals
	glBindBuffer(GL_ARRAY_BUFFER_ARB, this->normal);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, 0, 0);

	if (multi)
	{
		bool enable = false;
		for (int i=0; i<texco_num; ++i)
		{
			glClientActiveTexture(GL_TEXTURE0_ARB + i);
			switch(texco[i])
			{
			case RAS_IRasterizer::RAS_TEXCO_ORCO:
			case RAS_IRasterizer::RAS_TEXCO_GLOB:
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vertex);
				glTexCoordPointer(3, GL_FLOAT, 0, 0);
				enable=true;
				break;
			case RAS_IRasterizer::RAS_TEXCO_UV1:
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->UV[0]);
				glTexCoordPointer(2, GL_FLOAT, 0, 0);
				enable=true;
				break;
			case RAS_IRasterizer::RAS_TEXCO_NORM:
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->normal);
				glTexCoordPointer(3, GL_FLOAT, 0, 0);
				enable=true;
				break;
			case RAS_IRasterizer::RAS_TEXTANGENT:
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->tangent);
				glTexCoordPointer(4, GL_FLOAT, 0, 0);
				enable=true;
				break;
			case RAS_IRasterizer::RAS_TEXCO_UV2:
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->UV[1]);
				glTexCoordPointer(2, GL_FLOAT, 0, 0);
				enable=true;
				break;
			default:
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->dummy);
				glTexCoordPointer(1, GL_BYTE, 0, 0);
				break;
			}
		}
		if (enable)
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if (GLEW_ARB_vertex_program)
	{
		for (int i=0; i<attrib_num; ++i)
		{
			switch(attrib[i])
			{
			case RAS_IRasterizer::RAS_TEXCO_ORCO:
			case RAS_IRasterizer::RAS_TEXCO_GLOB:
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->vertex);
				glVertexAttribPointerARB(i, 3, GL_FLOAT, GL_FALSE, 0, 0);
				glEnableVertexAttribArrayARB(i);
				break;
			case RAS_IRasterizer::RAS_TEXCO_UV1:
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->UV[0]);
				glVertexAttribPointerARB(i, 2, GL_FLOAT, GL_FALSE, 0, 0);
				glEnableVertexAttribArrayARB(i);
				break;
			case RAS_IRasterizer::RAS_TEXCO_NORM:
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->normal);
				glVertexAttribPointerARB(i, 2, GL_FLOAT, GL_FALSE, 0, 0);
				glEnableVertexAttribArrayARB(i);
				break;
			case RAS_IRasterizer::RAS_TEXTANGENT:
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->tangent);
				glVertexAttribPointerARB(i, 4, GL_FLOAT, GL_FALSE, 0, 0);
				glEnableVertexAttribArrayARB(i);
				break;
			case RAS_IRasterizer::RAS_TEXCO_UV2:
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, this->UV[1]);
				glVertexAttribPointerARB(i, 2, GL_FLOAT, GL_FALSE, 0, 0);
				glEnableVertexAttribArrayARB(i);
				break;
			default:
				break;
			}
		}
	}
	
	glDrawElements(this->mode, this->indices, GL_UNSIGNED_SHORT, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (GLEW_ARB_vertex_program)
	{
		for (int i=0; i<attrib_num; ++i)
			glDisableVertexAttribArrayARB(i);
	}

	glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
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
		//vbo->UpdateIndices();
		//vbo->UpdatePositions();
		//vbo->UpdateNormals();
		//vbo->UpdateUVs();

		vbo->Draw(*m_texco_num, m_texco, *m_attrib_num, m_attrib, multi);
	}
}