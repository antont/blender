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
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file KX_Light.h
 *  \ingroup ketsji
 */

#ifndef __KX_LIGHT
#define __KX_LIGHT

#include "RAS_LightObject.h"
#include "KX_GameObject.h"
#include "DNA_lamp_types.h"

struct GPULamp;
struct Scene;
struct PoolLight;
class KX_Camera;
class RAS_IRasterizer;
class RAS_IRenderTools;
class MT_Transform;

class KX_LightObject : public KX_GameObject
{
	Py_Header
protected:
	RAS_LightObject		m_lightobj;
	PoolLight*			m_poollight;
	class RAS_IRenderTools*	m_rendertools;	//needed for registering and replication of lightobj
	bool				m_glsl;
	Scene*				m_blenderscene;

	bool				m_dynamic;
	void checkout_blenderlight();
	void checkin_blenderlight();
	void forfeit_blenderlight();

public:
	KX_LightObject(void* sgReplicationInfo,SG_Callbacks callbacks,class RAS_IRenderTools* rendertools,const struct RAS_LightObject&	lightobj, bool glsl);
	virtual ~KX_LightObject();
	virtual CValue*		GetReplica();
	RAS_LightObject*	GetLightData() { return &m_lightobj;}
	void				MakeDynamic();

	/* OpenGL Light */
	bool ApplyLight(KX_Scene *kxscene, int oblayer, int slot);

	/* GLSL Light */
	struct GPULamp *GetGPULamp();
	bool HasShadowBuffer();
	int GetShadowLayer();
	void BindShadowBuffer(class RAS_IRasterizer *ras, class KX_Camera *cam, class MT_Transform& camtrans);
	void UnbindShadowBuffer(class RAS_IRasterizer *ras);
	void Update();

	/* Light Pool */
	static void InitBlenderLightPool(Scene *scene, int point_count, int spot_count, int sun_count, int hemi_count, int area_count);
	static void FreeBlenderLightPool();
	
	void UpdateScene(class KX_Scene *kxscene) {m_lightobj.m_scene = (void*)kxscene;}

	virtual int GetGameObjectType() { return OBJ_LIGHT; }

#ifdef WITH_PYTHON
	/* attributes */
	static PyObject*	pyattr_get_color(void* self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_color(void* self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject* value);
	static PyObject*	pyattr_get_typeconst(void* self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_type(void* self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_type(void* self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject* value);
#endif
};

#endif //__KX_LIGHT

