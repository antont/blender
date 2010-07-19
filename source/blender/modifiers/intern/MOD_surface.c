/*
* $Id$
*
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
* along with this program; if not, write to the Free Software  Foundation,
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
* The Original Code is Copyright (C) 2005 by the Blender Foundation.
* All rights reserved.
*
* Contributor(s): Daniel Dunbar
*                 Ton Roosendaal,
*                 Ben Batt,
*                 Brecht Van Lommel,
*                 Campbell Barton
*
* ***** END GPL LICENSE BLOCK *****
*
*/

#include "DNA_scene_types.h"
#include "DNA_object_types.h"
#include "DNA_meshdata_types.h"

#include "BLI_math.h"

#include "BKE_cdderivedmesh.h"

#include "MOD_modifiertypes.h"
#include "MOD_util.h"

#include "MEM_guardedalloc.h"


static void initData(ModifierData *md) 
{
	SurfaceModifierData *surmd = (SurfaceModifierData*) md;
	
	surmd->bvhtree = NULL;
}

static void freeData(ModifierData *md)
{
	SurfaceModifierData *surmd = (SurfaceModifierData*) md;
	
	if (surmd)
	{
		if(surmd->bvhtree) {
			free_bvhtree_from_mesh(surmd->bvhtree);
			MEM_freeN(surmd->bvhtree);
		}

		if(surmd->dm)
			surmd->dm->release(surmd->dm);

		if(surmd->x)
			MEM_freeN(surmd->x);
		
		if(surmd->v)
			MEM_freeN(surmd->v);

		surmd->bvhtree = NULL;
		surmd->dm = NULL;
	}
}

static int dependsOnTime(ModifierData *md)
{
	return 1;
}

static void deformVerts(
					  ModifierData *md, Object *ob, DerivedMesh *derivedData,
		float (*vertexCos)[3], int numVerts, int useRenderParams, int isFinalCalc)
{
	SurfaceModifierData *surmd = (SurfaceModifierData*) md;
	unsigned int numverts = 0, i = 0;
	
	if(surmd->dm)
		surmd->dm->release(surmd->dm);

	/* if possible use/create DerivedMesh */
	if(derivedData) surmd->dm = CDDM_copy(derivedData, 0);
	else surmd->dm = get_dm(md->scene, ob, NULL, NULL, NULL, 0);
	
	if(!ob->pd)
	{
		printf("SurfaceModifier deformVerts: Should not happen!\n");
		return;
	}
	
	if(surmd->dm)
	{
		int init = 0;
		float *vec;
		MVert *x, *v;

		CDDM_apply_vert_coords(surmd->dm, vertexCos);
		CDDM_calc_normals(surmd->dm);
		
		numverts = surmd->dm->getNumVerts ( surmd->dm );

		if(numverts != surmd->numverts || surmd->x == NULL || surmd->v == NULL || md->scene->r.cfra != surmd->cfra+1) {
			if(surmd->x) {
				MEM_freeN(surmd->x);
				surmd->x = NULL;
			}
			if(surmd->v) {
				MEM_freeN(surmd->v);
				surmd->v = NULL;
			}

			surmd->x = MEM_callocN(numverts * sizeof(MVert), "MVert");
			surmd->v = MEM_callocN(numverts * sizeof(MVert), "MVert");

			surmd->numverts = numverts;

			init = 1;
		}

		/* convert to global coordinates and calculate velocity */
		for(i = 0, x = surmd->x, v = surmd->v; i<numverts; i++, x++, v++) {
			vec = CDDM_get_vert(surmd->dm, i)->co;
			mul_m4_v3(ob->obmat, vec);

			if(init)
				v->co[0] = v->co[1] = v->co[2] = 0.0f;
			else
				sub_v3_v3v3(v->co, vec, x->co);
			
			copy_v3_v3(x->co, vec);
		}

		surmd->cfra = md->scene->r.cfra;

		if(surmd->bvhtree)
			free_bvhtree_from_mesh(surmd->bvhtree);
		else
			surmd->bvhtree = MEM_callocN(sizeof(BVHTreeFromMesh), "BVHTreeFromMesh");

		if(surmd->dm->getNumTessFaces(surmd->dm))
			bvhtree_from_mesh_faces(surmd->bvhtree, surmd->dm, 0.0, 2, 6);
		else
			bvhtree_from_mesh_edges(surmd->bvhtree, surmd->dm, 0.0, 2, 6);
	}
}


ModifierTypeInfo modifierType_Surface = {
	/* name */              "Surface",
	/* structName */        "SurfaceModifierData",
	/* structSize */        sizeof(SurfaceModifierData),
	/* type */              eModifierTypeType_OnlyDeform,
	/* flags */             eModifierTypeFlag_AcceptsMesh
							| eModifierTypeFlag_NoUserAdd,

	/* copyData */          0,
	/* deformVerts */       deformVerts,
	/* deformVertsEM */     0,
	/* deformMatricesEM */  0,
	/* applyModifier */     0,
	/* applyModifierEM */   0,
	/* initData */          initData,
	/* requiredDataMask */  0,
	/* freeData */          freeData,
	/* isDisabled */        0,
	/* updateDepgraph */    0,
	/* dependsOnTime */     dependsOnTime,
	/* foreachObjectLink */ 0,
	/* foreachIDLink */     0,
};
