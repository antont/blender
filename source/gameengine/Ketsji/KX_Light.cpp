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

/** \file gameengine/Ketsji/KX_Light.cpp
 *  \ingroup ketsji
 */


#if defined(WIN32) && !defined(FREE_WINDOWS)
#pragma warning (disable : 4786)
#endif

#include "GL/glew.h"

#include "KX_Light.h"
#include "KX_Camera.h"
#include "RAS_IRasterizer.h"
#include "RAS_IRenderTools.h"

#include "KX_PyMath.h"

#include "DNA_object_types.h"
#include "DNA_scene_types.h"
#include "GPU_material.h"

#include "MEM_guardedalloc.h"

extern "C"
{
#include "BKE_object.h"
#include "BKE_scene.h"
#include "BKE_library.h"
#include "BKE_global.h"
#include "BKE_main.h"
#include "BLI_listbase.h"
}

static Scene* m_blenderlight_scene = NULL;
static int m_blenderlight_count = 0;

static std::vector<Object*> m_blenderlight_points = vector<Object*>();
static std::vector<Object*> m_blenderlight_spots = vector<Object*>();
static std::vector<Object*> m_blenderlight_suns = vector<Object*>();
static std::vector<Object*> m_blenderlight_hemis = vector<Object*>();
static std::vector<Object*> m_blenderlight_areas = vector<Object*>();
 
KX_LightObject::KX_LightObject(void* sgReplicationInfo,SG_Callbacks callbacks,
							   class RAS_IRenderTools* rendertools,
							   const RAS_LightObject&	lightobj,
							   bool glsl)
	: KX_GameObject(sgReplicationInfo,callbacks),
	  m_rendertools(rendertools)
{
	m_lightobj = lightobj;
	m_lightobj.m_scene = sgReplicationInfo;
	m_lightobj.m_light = this;
	m_rendertools->AddLight(&m_lightobj);
	m_glsl = glsl;
	m_blenderscene = ((KX_Scene*)sgReplicationInfo)->GetBlenderScene();
	m_dynamic = false;
};


KX_LightObject::~KX_LightObject()
{
	GPULamp *lamp;
	Object* obj;

	if((lamp = GetGPULamp())) {
		float obmat[4][4] = {{0}};
		GPU_lamp_update(lamp, 0, 0, obmat);
	}

	m_rendertools->RemoveLight(&m_lightobj);

	obj=GetBlenderObject();
	if (m_dynamic && obj)
		checkin_blenderlight(obj);
}


CValue*		KX_LightObject::GetReplica()
{

	KX_LightObject* replica = new KX_LightObject(*this);

	replica->ProcessReplica();
	
	replica->m_lightobj.m_light = replica;
	m_rendertools->AddLight(&replica->m_lightobj);

	replica->MakeDynamic();

	return replica;
}

void KX_LightObject::MakeDynamic()
{
	m_dynamic = true;
	SetBlenderObject(checkout_blenderlight());
}

bool KX_LightObject::ApplyLight(KX_Scene *kxscene, int oblayer, int slot)
{
	KX_Scene* lightscene = (KX_Scene*)m_lightobj.m_scene;
	float vec[4];
	int scenelayer = ~0;

	if(kxscene && kxscene->GetBlenderScene())
		scenelayer = kxscene->GetBlenderScene()->lay;
	
	/* only use lights in the same layer as the object */
	if(!(m_lightobj.m_layer & oblayer))
		return false;
	/* only use lights in the same scene, and in a visible layer */
	if(kxscene != lightscene || !(m_lightobj.m_layer & scenelayer))
		return false;

	// lights don't get their openGL matrix updated, do it now
	if(GetSGNode()->IsDirty())
		GetOpenGLMatrix();

	MT_CmMatrix4x4& worldmatrix= *GetOpenGLMatrixPtr();

	vec[0] = worldmatrix(0,3);
	vec[1] = worldmatrix(1,3);
	vec[2] = worldmatrix(2,3);
	vec[3] = 1.0f;

	if(m_lightobj.m_type==RAS_LightObject::LIGHT_SUN) {
		
		vec[0] = worldmatrix(0,2);
		vec[1] = worldmatrix(1,2);
		vec[2] = worldmatrix(2,2);
		//vec[0]= base->object->obmat[2][0];
		//vec[1]= base->object->obmat[2][1];
		//vec[2]= base->object->obmat[2][2];
		vec[3]= 0.0;
		glLightfv((GLenum)(GL_LIGHT0+slot), GL_POSITION, vec); 
	}
	else {
		//vec[3]= 1.0;
		glLightfv((GLenum)(GL_LIGHT0+slot), GL_POSITION, vec); 
		glLightf((GLenum)(GL_LIGHT0+slot), GL_CONSTANT_ATTENUATION, 1.0);
		glLightf((GLenum)(GL_LIGHT0+slot), GL_LINEAR_ATTENUATION, m_lightobj.m_att1/m_lightobj.m_distance);
		// without this next line it looks backward compatible.
		//attennuation still is acceptable 
		glLightf((GLenum)(GL_LIGHT0+slot), GL_QUADRATIC_ATTENUATION, m_lightobj.m_att2/(m_lightobj.m_distance*m_lightobj.m_distance)); 
		
		if(m_lightobj.m_type==RAS_LightObject::LIGHT_SPOT) {
			vec[0] = -worldmatrix(0,2);
			vec[1] = -worldmatrix(1,2);
			vec[2] = -worldmatrix(2,2);
			//vec[0]= -base->object->obmat[2][0];
			//vec[1]= -base->object->obmat[2][1];
			//vec[2]= -base->object->obmat[2][2];
			glLightfv((GLenum)(GL_LIGHT0+slot), GL_SPOT_DIRECTION, vec);
			glLightf((GLenum)(GL_LIGHT0+slot), GL_SPOT_CUTOFF, m_lightobj.m_spotsize/2.0);
			glLightf((GLenum)(GL_LIGHT0+slot), GL_SPOT_EXPONENT, 128.0*m_lightobj.m_spotblend);
		}
		else
			glLightf((GLenum)(GL_LIGHT0+slot), GL_SPOT_CUTOFF, 180.0);
	}
	
	if (m_lightobj.m_nodiffuse) {
		vec[0] = vec[1] = vec[2] = vec[3] = 0.0;
	}
	else {
		vec[0]= m_lightobj.m_energy*m_lightobj.m_red;
		vec[1]= m_lightobj.m_energy*m_lightobj.m_green;
		vec[2]= m_lightobj.m_energy*m_lightobj.m_blue;
		vec[3]= 1.0;
	}

	glLightfv((GLenum)(GL_LIGHT0+slot), GL_DIFFUSE, vec);
	if(m_lightobj.m_nospecular)
	{
		vec[0] = vec[1] = vec[2] = vec[3] = 0.0;
	}
	else if (m_lightobj.m_nodiffuse) {
		vec[0]= m_lightobj.m_energy*m_lightobj.m_red;
		vec[1]= m_lightobj.m_energy*m_lightobj.m_green;
		vec[2]= m_lightobj.m_energy*m_lightobj.m_blue;
		vec[3]= 1.0;
	}

	glLightfv((GLenum)(GL_LIGHT0+slot), GL_SPECULAR, vec);
	glEnable((GLenum)(GL_LIGHT0+slot));

	return true;
}

GPULamp *KX_LightObject::GetGPULamp()
{
	if(m_glsl && GetBlenderObject())
	{
		Scene *scene = (m_dynamic) ? m_blenderlight_scene : m_blenderscene;
		return GPU_lamp_from_blender(scene, GetBlenderObject(), GetBlenderGroupObject());
	}
	else
		return NULL;
}

void KX_LightObject::Update()
{
	GPULamp *lamp;

	if((lamp = GetGPULamp()) != NULL && GetSGNode()) {
		float obmat[4][4];
		// lights don't get their openGL matrix updated, do it now
		if (GetSGNode()->IsDirty())
			GetOpenGLMatrix();
		double *dobmat = GetOpenGLMatrixPtr()->getPointer();

		for(int i=0; i<4; i++)
			for(int j=0; j<4; j++, dobmat++)
				obmat[i][j] = (float)*dobmat;

		GPU_lamp_update(lamp, m_lightobj.m_layer, 0, obmat);
		GPU_lamp_update_colors(lamp, m_lightobj.m_red, m_lightobj.m_green, 
			m_lightobj.m_blue, m_lightobj.m_energy);
		GPU_lamp_update_distance(lamp, m_lightobj.m_distance, m_lightobj.m_att1, m_lightobj.m_att2);
		GPU_lamp_update_spot(lamp, m_lightobj.m_spotsize, m_lightobj.m_spotblend);
	}
}

bool KX_LightObject::HasShadowBuffer()
{
	GPULamp *lamp;

	if((lamp = GetGPULamp()))
		return GPU_lamp_has_shadow_buffer(lamp);
	else
		return false;
}

int KX_LightObject::GetShadowLayer()
{
	GPULamp *lamp;

	if((lamp = GetGPULamp()))
		return GPU_lamp_shadow_layer(lamp);
	else
		return 0;
}

void KX_LightObject::BindShadowBuffer(RAS_IRasterizer *ras, KX_Camera *cam, MT_Transform& camtrans)
{
	GPULamp *lamp;
	float viewmat[4][4], winmat[4][4];
	int winsize;

	/* bind framebuffer */
	lamp = GetGPULamp();
	GPU_lamp_shadow_buffer_bind(lamp, viewmat, &winsize, winmat);

	/* setup camera transformation */
	MT_Matrix4x4 modelviewmat((float*)viewmat);
	MT_Matrix4x4 projectionmat((float*)winmat);

	MT_Transform trans = MT_Transform((float*)viewmat);
	camtrans.invert(trans);

	cam->SetModelviewMatrix(modelviewmat);
	cam->SetProjectionMatrix(projectionmat);
	
	cam->NodeSetLocalPosition(camtrans.getOrigin());
	cam->NodeSetLocalOrientation(camtrans.getBasis());
	cam->NodeUpdateGS(0);

	/* setup rasterizer transformations */
	ras->SetProjectionMatrix(projectionmat);
	ras->SetViewMatrix(modelviewmat, cam->NodeGetWorldOrientation(), cam->NodeGetWorldPosition(), cam->GetCameraData()->m_perspective);
}

void KX_LightObject::UnbindShadowBuffer(RAS_IRasterizer *ras)
{
	GPULamp *lamp = GetGPULamp();
	GPU_lamp_shadow_buffer_unbind(lamp);
}

void init_subpool(Scene *scene, std::vector<Object*> *subpool, int count, short type)
{
	Lamp *la;
	subpool->resize(count);
	for (int i = 0; i < count; ++i)
	{
		subpool->at(i) = add_object(m_blenderlight_scene, OB_LAMP);

		//Give the lights a unique name so the converter can avoid them
		strcpy(subpool->at(i)->id.name, "__pool__");

		la = (Lamp*)subpool->at(i)->data;
		la->type = type;
		la->energy = 0;
	}

}

void KX_LightObject::InitBlenderLightPool(Scene *scene, int point_count, int spot_count, int sun_count, int hemi_count, int area_count)
{
	Lamp* la;

	//If the light pool is already built, there is no need to rebuild it,
	//we don't want libload trying to reinitialize the light pool
	if (m_blenderlight_count != 0)
		return;

	m_blenderlight_scene = scene;
	m_blenderlight_count = point_count + spot_count + sun_count + hemi_count + area_count;

	//Don't bother finishing setup if there are no lights in the pools
	//Also if no Dynamic lights are allocated, sometimes libload will get passed the count
	//check and screw things up
	if (m_blenderlight_count == 0 && false)
		return;

	init_subpool(scene, &m_blenderlight_points, point_count, LA_LOCAL);
	init_subpool(scene, &m_blenderlight_spots, spot_count, LA_SPOT);
	init_subpool(scene, &m_blenderlight_suns, sun_count, LA_SUN);
	init_subpool(scene, &m_blenderlight_hemis, hemi_count, LA_HEMI);
	init_subpool(scene, &m_blenderlight_areas, area_count, LA_AREA);

	GPU_materials_free();
}

std::vector<Object*>* get_subpool(short type)
{
	if (type == RAS_LightObject::LIGHT_NORMAL)
		return &m_blenderlight_points;
	if (type == RAS_LightObject::LIGHT_SPOT)
		return &m_blenderlight_spots;
	if (type == RAS_LightObject::LIGHT_SUN)
		return &m_blenderlight_suns;
	if (type == RAS_LightObject::LIGHT_HEMI)
		return &m_blenderlight_hemis;
	if (type == RAS_LightObject::LIGHT_AREA)
		return &m_blenderlight_areas;
}

Object* KX_LightObject::checkout_blenderlight()
{
	std::vector<Object*> *subpool = get_subpool(m_lightobj.m_type);

	if (subpool->size() == 0)
		return NULL; //Sorry, out of lights :(

	Object* obj = subpool->back();
	subpool->pop_back();
	return obj;
}

void KX_LightObject::checkin_blenderlight(Object* lamp)
{
	std::vector<Object*> *subpool = get_subpool(m_lightobj.m_type);
	subpool->push_back(lamp);
}

void KX_LightObject::FreeBlenderLightPool()
{
	Base *base;
	std::vector<Object*> *subpool;
	unsigned int i = 0;
	int total = m_blenderlight_points.size() + m_blenderlight_spots.size() + m_blenderlight_suns.size() + m_blenderlight_hemis.size() + m_blenderlight_areas.size();
	if (total != m_blenderlight_count)
	{
		printf("Light pool still has lights checked out. Or extra lights have been checked in\n");
		return;
	}

	for (int i = 0; i < m_blenderlight_count; ++i)
	{
		if (m_blenderlight_points.size() > 0)
			subpool = &m_blenderlight_points;
		else if (m_blenderlight_spots.size() > 0)
			subpool = &m_blenderlight_spots;
		else if (m_blenderlight_suns.size() > 0)
			subpool = &m_blenderlight_suns;
		else if (m_blenderlight_hemis.size() > 0)
			subpool = &m_blenderlight_hemis;
		else if (m_blenderlight_areas.size() > 0)
			subpool = &m_blenderlight_areas;

		base = object_in_scene(subpool->back(), m_blenderlight_scene);
		BLI_remlink(&m_blenderlight_scene->base, base);
		free_libblock_us(&G.main->object, base->object);
		if(m_blenderlight_scene->basact==base) m_blenderlight_scene->basact=NULL;
		MEM_freeN(base);
		subpool->pop_back();
	}

	m_blenderlight_count = 0;

	GPU_materials_free();
}

#ifdef WITH_PYTHON
/* ------------------------------------------------------------------------- */
/* Python Integration Hooks					                                 */
/* ------------------------------------------------------------------------- */

PyTypeObject KX_LightObject::Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"KX_LightObject",
	sizeof(PyObjectPlus_Proxy),
	0,
	py_base_dealloc,
	0,
	0,
	0,
	0,
	py_base_repr,
	0,
	&KX_GameObject::Sequence,
	&KX_GameObject::Mapping,
	0,0,0,
	NULL,
	NULL,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	0,0,0,0,0,0,0,
	Methods,
	0,
	0,
	&KX_GameObject::Type,
	0,0,0,0,0,0,
	py_base_new
};

PyMethodDef KX_LightObject::Methods[] = {
	{NULL,NULL} //Sentinel
};

PyAttributeDef KX_LightObject::Attributes[] = {
	KX_PYATTRIBUTE_INT_RW("layer", 1, 20, true, KX_LightObject, m_lightobj.m_layer),
	KX_PYATTRIBUTE_FLOAT_RW("energy", 0, 10, KX_LightObject, m_lightobj.m_energy),
	KX_PYATTRIBUTE_FLOAT_RW("distance", 0.01, 5000, KX_LightObject, m_lightobj.m_distance),
	KX_PYATTRIBUTE_RW_FUNCTION("color", KX_LightObject, pyattr_get_color, pyattr_set_color),
	KX_PYATTRIBUTE_FLOAT_RW("lin_attenuation", 0, 1, KX_LightObject, m_lightobj.m_att1),
	KX_PYATTRIBUTE_FLOAT_RW("quad_attenuation", 0, 1, KX_LightObject, m_lightobj.m_att2),
	KX_PYATTRIBUTE_FLOAT_RW("spotsize", 1, 180, KX_LightObject, m_lightobj.m_spotsize),
	KX_PYATTRIBUTE_FLOAT_RW("spotblend", 0, 1, KX_LightObject, m_lightobj.m_spotblend),
	KX_PYATTRIBUTE_RO_FUNCTION("SPOT", KX_LightObject, pyattr_get_typeconst),
	KX_PYATTRIBUTE_RO_FUNCTION("SUN", KX_LightObject, pyattr_get_typeconst),
	KX_PYATTRIBUTE_RO_FUNCTION("NORMAL", KX_LightObject, pyattr_get_typeconst),
	KX_PYATTRIBUTE_RW_FUNCTION("type", KX_LightObject, pyattr_get_type, pyattr_set_type),
	{ NULL }	//Sentinel
};

PyObject* KX_LightObject::pyattr_get_color(void *self_v, const KX_PYATTRIBUTE_DEF *attrdef)
{
	KX_LightObject* self = static_cast<KX_LightObject*>(self_v);
	return Py_BuildValue("[fff]", self->m_lightobj.m_red, self->m_lightobj.m_green, self->m_lightobj.m_blue);
}

int KX_LightObject::pyattr_set_color(void *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value)
{
	KX_LightObject* self = static_cast<KX_LightObject*>(self_v);

	MT_Vector3 color;
	if (PyVecTo(value, color))
	{
		self->m_lightobj.m_red = color[0];
		self->m_lightobj.m_green = color[1];
		self->m_lightobj.m_blue = color[2];
		return PY_SET_ATTR_SUCCESS;
	}
	return PY_SET_ATTR_FAIL;
}

PyObject* KX_LightObject::pyattr_get_typeconst(void *self_v, const KX_PYATTRIBUTE_DEF *attrdef)
{
	PyObject* retvalue;

	const char* type = attrdef->m_name;

	if(!strcmp(type, "SPOT")) {
		retvalue = PyLong_FromSsize_t(RAS_LightObject::LIGHT_SPOT);
	} else if (!strcmp(type, "SUN")) {
		retvalue = PyLong_FromSsize_t(RAS_LightObject::LIGHT_SUN);
	} else if (!strcmp(type, "NORMAL")) {
		retvalue = PyLong_FromSsize_t(RAS_LightObject::LIGHT_NORMAL);
	}
    else {
        /* should never happen */
        PyErr_SetString(PyExc_TypeError, "light.type: internal error, invalid light type");
        retvalue = NULL;
    }

	return retvalue;
}

PyObject* KX_LightObject::pyattr_get_type(void* self_v, const KX_PYATTRIBUTE_DEF *attrdef)
{
	KX_LightObject* self = static_cast<KX_LightObject*>(self_v);
	return PyLong_FromSsize_t(self->m_lightobj.m_type);
}

int KX_LightObject::pyattr_set_type(void* self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject* value)
{
	KX_LightObject* self = static_cast<KX_LightObject*>(self_v);
	int val = PyLong_AsSsize_t(value);
	if((val==-1 && PyErr_Occurred()) || val<0 || val>2) {
		PyErr_SetString(PyExc_ValueError, "light.type= val: KX_LightObject, expected an int between 0 and 2");
		return PY_SET_ATTR_FAIL;
	}
	
	switch(val) {
		case 0:
			self->m_lightobj.m_type = self->m_lightobj.LIGHT_SPOT;
			break;
		case 1:
			self->m_lightobj.m_type = self->m_lightobj.LIGHT_SUN;
			break;
		case 2:
			self->m_lightobj.m_type = self->m_lightobj.LIGHT_NORMAL;
			break;
	}

	return PY_SET_ATTR_SUCCESS;
}
#endif // WITH_PYTHON
