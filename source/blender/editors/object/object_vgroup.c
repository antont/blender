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

/** \file blender/editors/object/object_vgroup.c
 *  \ingroup edobj
 */


#include <string.h>
#include <stddef.h>
#include <math.h>
#include <assert.h>

#include "MEM_guardedalloc.h"

#include "DNA_cloth_types.h"
#include "DNA_curve_types.h"
#include "DNA_lattice_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_modifier_types.h"
#include "DNA_object_types.h"
#include "DNA_object_force.h"
#include "DNA_scene_types.h"
#include "DNA_particle_types.h"

#include "BLI_blenlib.h"
#include "BLI_editVert.h"
#include "BLI_utildefines.h"

#include "BKE_context.h"
#include "BKE_customdata.h"
#include "BKE_deform.h"
#include "BKE_depsgraph.h"
#include "BKE_global.h"
#include "BKE_mesh.h"
#include "BKE_report.h"
#include "BKE_DerivedMesh.h"//Jason

#include "RNA_access.h"
#include "RNA_define.h"

#include "WM_api.h"
#include "WM_types.h"

#include "ED_mesh.h"

#include "UI_resources.h"

#include "object_intern.h"

/************************ Exported Functions **********************/
static void vgroup_remap_update_users(Object *ob, int *map);
static void vgroup_delete_edit_mode(Object *ob, bDeformGroup *defgroup);
static void vgroup_delete_object_mode(Object *ob, bDeformGroup *dg);
static void vgroup_delete_all(Object *ob);

static Lattice *vgroup_edit_lattice(Object *ob)
{
	Lattice *lt= ob->data;
	BLI_assert(ob->type==OB_LATTICE);
	return (lt->editlatt)? lt->editlatt->latt: lt;
}

int ED_vgroup_object_is_edit_mode(Object *ob)
{
	if(ob->type == OB_MESH)
		return (((Mesh*)ob->data)->edit_mesh != NULL);
	else if(ob->type == OB_LATTICE)
		return (((Lattice*)ob->data)->editlatt != NULL);

	return 0;
}

bDeformGroup *ED_vgroup_add_name(Object *ob, const char *name)
{
	bDeformGroup *defgroup;
	
	if(!ob || !ELEM(ob->type, OB_MESH, OB_LATTICE))
		return NULL;
	
	defgroup = MEM_callocN(sizeof(bDeformGroup), "add deformGroup");

	BLI_strncpy(defgroup->name, name, sizeof(defgroup->name));

	BLI_addtail(&ob->defbase, defgroup);
	defgroup_unique_name(defgroup, ob);

	ob->actdef = BLI_countlist(&ob->defbase);

	return defgroup;
}

bDeformGroup *ED_vgroup_add(Object *ob) 
{
	return ED_vgroup_add_name(ob, "Group");
}

void ED_vgroup_delete(Object *ob, bDeformGroup *defgroup) 
{
	bDeformGroup *dg = (bDeformGroup *)ob->defbase.first;

	while (dg) {
		if (dg == defgroup)
			break;
		dg = dg->next;
	}

	if (dg == NULL)
		return;

	if(ED_vgroup_object_is_edit_mode(ob))
		vgroup_delete_edit_mode(ob, dg);
	else
		vgroup_delete_object_mode(ob, dg);
}

int ED_vgroup_data_create(ID *id)
{
	/* create deform verts */

	if(GS(id->name)==ID_ME) {
		Mesh *me= (Mesh *)id;
		me->dvert= CustomData_add_layer(&me->vdata, CD_MDEFORMVERT, CD_CALLOC, NULL, me->totvert);
		return TRUE;
	}
	else if(GS(id->name)==ID_LT) {
		Lattice *lt= (Lattice *)id;
		lt->dvert= MEM_callocN(sizeof(MDeformVert)*lt->pntsu*lt->pntsv*lt->pntsw, "lattice deformVert");
		return TRUE;
	}
	else {
		return FALSE;
	}
}

static int ED_vgroup_give_parray(ID *id, MDeformVert ***dvert_arr, int *dvert_tot)
{
	*dvert_tot = 0;
	*dvert_arr = NULL;

	if(id) {
		switch(GS(id->name)) {
			case ID_ME:
			{
				Mesh *me = (Mesh *)id;

				if(me->edit_mesh) {
					EditMesh *em = me->edit_mesh;
					EditVert *eve;
					int i;

					if (!CustomData_has_layer(&em->vdata, CD_MDEFORMVERT)) {
						return 0;
					}

					i= BLI_countlist(&em->verts);

					*dvert_arr= MEM_mallocN(sizeof(void*)*i, "vgroup parray from me");
					*dvert_tot = i;

					i = 0;
					for (eve=em->verts.first; eve; eve=eve->next, i++) {
						(*dvert_arr)[i] = CustomData_em_get(&em->vdata, eve->data, CD_MDEFORMVERT);
					}

					return 1;
				}
				else if(me->dvert) {
					int i;

					*dvert_tot= me->totvert;
					*dvert_arr= MEM_mallocN(sizeof(void*)*me->totvert, "vgroup parray from me");

					for (i=0; i<me->totvert; i++) {
						(*dvert_arr)[i] = me->dvert + i;
					}

					return 1;
				}
				else {
					return 0;
				}
			}
			case ID_LT:
			{
				int i=0;

				Lattice *lt= (Lattice *)id;
				lt= (lt->editlatt)? lt->editlatt->latt: lt;

				if(lt->dvert) {
					*dvert_tot= lt->pntsu*lt->pntsv*lt->pntsw;
					*dvert_arr= MEM_mallocN(sizeof(void*)*(*dvert_tot), "vgroup parray from me");

					for (i=0; i<*dvert_tot; i++) {
						(*dvert_arr)[i] = lt->dvert + i;
					}

					return 1;
				}
				else {
					return 0;
				}
			}
		}
	}

	return 0;
}

/* returns true if the id type supports weights */
int ED_vgroup_give_array(ID *id, MDeformVert **dvert_arr, int *dvert_tot)
{
	if(id) {
		switch(GS(id->name)) {
			case ID_ME:
			{
				Mesh *me = (Mesh *)id;
				*dvert_arr= me->dvert;
				*dvert_tot= me->totvert;
				return TRUE;
			}
			case ID_LT:
			{
				Lattice *lt= (Lattice *)id;
				lt= (lt->editlatt)? lt->editlatt->latt: lt;
				*dvert_arr= lt->dvert;
				*dvert_tot= lt->pntsu*lt->pntsv*lt->pntsw;
				return TRUE;
			}
		}
	}

	*dvert_arr= NULL;
	*dvert_tot= 0;
	return FALSE;
}

/* matching index only */
int ED_vgroup_copy_array(Object *ob, Object *ob_from)
{
	MDeformVert **dvert_array_from, **dvf;
	MDeformVert **dvert_array, **dv;
	int dvert_tot_from;
	int dvert_tot;
	int i;
	int totdef_from= BLI_countlist(&ob_from->defbase);
	int totdef= BLI_countlist(&ob->defbase);
	short new_vgroup= FALSE;

	ED_vgroup_give_parray(ob_from->data, &dvert_array_from, &dvert_tot_from);
	ED_vgroup_give_parray(ob->data, &dvert_array, &dvert_tot);

	if((dvert_array == NULL) && (dvert_array_from != NULL) && ED_vgroup_data_create(ob->data)) {
		ED_vgroup_give_parray(ob->data, &dvert_array, &dvert_tot);
		new_vgroup= TRUE;
	}

	if(ob==ob_from || dvert_tot==0 || (dvert_tot != dvert_tot_from) || dvert_array_from==NULL || dvert_array==NULL) {
		if (dvert_array) MEM_freeN(dvert_array);
		if (dvert_array_from) MEM_freeN(dvert_array_from);

		if(new_vgroup == TRUE) {
			/* free the newly added vgroup since it wasn't compatible */
			vgroup_delete_all(ob);
		}
		return 0;
	}

	/* do the copy */
	BLI_freelistN(&ob->defbase);
	BLI_duplicatelist(&ob->defbase, &ob_from->defbase);
	ob->actdef= ob_from->actdef;

	if(totdef_from < totdef) {
		/* correct vgroup indices because the number of vgroups is being reduced. */
		int *remap= MEM_mallocN(sizeof(int) * (totdef + 1), "ED_vgroup_copy_array");
		for(i=0; i<=totdef_from; i++) remap[i]= i;
		for(; i<=totdef; i++) remap[i]= 0; /* can't use these, so disable */

		vgroup_remap_update_users(ob, remap);
		MEM_freeN(remap);
	}

	dvf= dvert_array_from;
	dv= dvert_array;

	for(i=0; i<dvert_tot; i++, dvf++, dv++) {
		if((*dv)->dw)
			MEM_freeN((*dv)->dw);

		*(*dv)= *(*dvf);

		if((*dv)->dw)
			(*dv)->dw= MEM_dupallocN((*dv)->dw);
	}

	MEM_freeN(dvert_array);
	MEM_freeN(dvert_array_from);

	return 1;
}

/* for mesh in object mode
   lattice can be in editmode */
static void ED_vgroup_nr_vert_remove(Object *ob, int def_nr, int vertnum)
{
	/* This routine removes the vertex from the deform
	 * group with number def_nr.
	 *
	 * This routine is meant to be fast, so it is the
	 * responsibility of the calling routine to:
	 *   a) test whether ob is non-NULL
	 *   b) test whether ob is a mesh
	 *   c) calculate def_nr
	 */

	MDeformWeight *newdw;
	MDeformVert *dvert= NULL;
	int i, tot;

	/* get the deform vertices corresponding to the
	 * vertnum
	 */
	ED_vgroup_give_array(ob->data, &dvert, &tot);

	if(dvert==NULL)
		return;
	
	dvert+= vertnum;

	/* for all of the deform weights in the
	 * deform vert
	 */
	for(i=dvert->totweight - 1 ; i>=0 ; i--){

		/* if the def_nr is the same as the one
		 * for our weight group then remove it
		 * from this deform vert.
		 */
		if(dvert->dw[i].def_nr == def_nr) {
			dvert->totweight--;

			/* if there are still other deform weights
			 * attached to this vert then remove this
			 * deform weight, and reshuffle the others
			 */
			if(dvert->totweight) {
				newdw = MEM_mallocN(sizeof(MDeformWeight)*(dvert->totweight), 
									 "deformWeight");
				if(dvert->dw){
					memcpy(newdw, dvert->dw, sizeof(MDeformWeight)*i);
					memcpy(newdw+i, dvert->dw+i+1, 
							sizeof(MDeformWeight)*(dvert->totweight-i));
					MEM_freeN(dvert->dw);
				}
				dvert->dw=newdw;
			}
			/* if there are no other deform weights
			 * left then just remove the deform weight
			 */
			else {
				MEM_freeN(dvert->dw);
				dvert->dw = NULL;
				break;
			}
		}
	}

}

/* for Mesh in Object mode */
/* allows editmode for Lattice */
static void ED_vgroup_nr_vert_add(Object *ob, int def_nr, int vertnum, float weight, int assignmode)
{
	/* add the vert to the deform group with the
	 * specified number
	 */
	MDeformVert *dv= NULL;
	MDeformWeight *newdw;
	int	i, tot;

	/* get the vert */
	ED_vgroup_give_array(ob->data, &dv, &tot);
	
	if(dv==NULL)
		return;
	
	/* check that vertnum is valid before trying to get the relevant dvert */
	if ((vertnum < 0) || (vertnum >= tot))
		return;
	else
		dv += vertnum;

	/* Lets first check to see if this vert is
	 * already in the weight group -- if so
	 * lets update it
	 */
	for(i=0; i<dv->totweight; i++){
		
		/* if this weight cooresponds to the
		 * deform group, then add it using
		 * the assign mode provided
		 */
		if(dv->dw[i].def_nr == def_nr){
			
			switch(assignmode) {
			case WEIGHT_REPLACE:
				dv->dw[i].weight=weight;
				break;
			case WEIGHT_ADD:
				dv->dw[i].weight+=weight;
				if(dv->dw[i].weight >= 1.0f)
					dv->dw[i].weight = 1.0f;
				break;
			case WEIGHT_SUBTRACT:
				dv->dw[i].weight-=weight;
				/* if the weight is zero or less then
				 * remove the vert from the deform group
				 */
				if(dv->dw[i].weight <= 0.0f)
					ED_vgroup_nr_vert_remove(ob, def_nr, vertnum);
				break;
			}
			return;
		}
	}

	/* if the vert wasn't in the deform group then
	 * we must take a different form of action ...
	 */

	switch(assignmode) {
	case WEIGHT_SUBTRACT:
		/* if we are subtracting then we don't
		 * need to do anything
		 */
		return;

	case WEIGHT_REPLACE:
	case WEIGHT_ADD:
		/* if we are doing an additive assignment, then
		 * we need to create the deform weight
		 */
		newdw = MEM_callocN(sizeof(MDeformWeight)*(dv->totweight+1), 
							 "deformWeight");
		if(dv->dw){
			memcpy(newdw, dv->dw, sizeof(MDeformWeight)*dv->totweight);
			MEM_freeN(dv->dw);
		}
		dv->dw=newdw;

		dv->dw[dv->totweight].weight=weight;
		dv->dw[dv->totweight].def_nr=def_nr;

		dv->totweight++;
		break;
	}
}

/* called while not in editmode */
void ED_vgroup_vert_add(Object *ob, bDeformGroup *dg, int vertnum, float weight, int assignmode)
{
	/* add the vert to the deform group with the
	 * specified assign mode
	 */
	int	def_nr;

	MDeformVert *dv= NULL;
	int tot;

	/* get the deform group number, exit if
	 * it can't be found
	 */
	def_nr = defgroup_find_index(ob, dg);
	if(def_nr < 0) return;

	/* if there's no deform verts then create some,
	 */
	if(ED_vgroup_give_array(ob->data, &dv, &tot) && dv==NULL)
		ED_vgroup_data_create(ob->data);

	/* call another function to do the work
	 */
	ED_vgroup_nr_vert_add(ob, def_nr, vertnum, weight, assignmode);
}

/* mesh object mode, lattice can be in editmode */
void ED_vgroup_vert_remove(Object *ob, bDeformGroup	*dg, int vertnum)
{
	/* This routine removes the vertex from the specified
	 * deform group.
	 */
	const int def_nr= defgroup_find_index(ob, dg);
	if(def_nr < 0)
		return;

	ED_vgroup_nr_vert_remove(ob, def_nr, vertnum);
}

static float get_vert_def_nr(Object *ob, int def_nr, int vertnum)
{
	MDeformVert *dvert= NULL;
	EditVert *eve;
	Mesh *me;
	int i;

	/* get the deform vertices corresponding to the vertnum */
	if(ob->type==OB_MESH) {
		me= ob->data;

		if(me->edit_mesh) {
			eve= BLI_findlink(&me->edit_mesh->verts, vertnum);
			if(!eve) {
				return 0.0f;
			}
			dvert= CustomData_em_get(&me->edit_mesh->vdata, eve->data, CD_MDEFORMVERT);
			vertnum= 0;
		}
		else {
			if(vertnum >= me->totvert) {
				return 0.0f;
			}
			dvert = me->dvert;
		}
	}
	else if(ob->type==OB_LATTICE) {
		Lattice *lt= vgroup_edit_lattice(ob);

		if(lt->dvert) {
			if(vertnum >= lt->pntsu*lt->pntsv*lt->pntsw) {
				return 0.0f;
			}
			dvert = lt->dvert;
		}
	}
	
	if(dvert==NULL)
		return -1;
	
	dvert += vertnum;
	
	for(i=dvert->totweight-1 ; i>=0 ; i--)
		if(dvert->dw[i].def_nr == def_nr)
			return dvert->dw[i].weight;

	return -1;
}

float ED_vgroup_vert_weight(Object *ob, bDeformGroup *dg, int vertnum)
{
	int def_nr;

	if(!ob) return -1;

	def_nr = defgroup_find_index(ob, dg);
	if(def_nr < 0) return -1;

	return get_vert_def_nr(ob, def_nr, vertnum);
}

void ED_vgroup_select_by_name(Object *ob, const char *name)
{	/* note: ob->actdef==0 signals on painting to create a new one, if a bone in posemode is selected */
	ob->actdef= defgroup_name_index(ob, name) + 1;
}

/********************** Operator Implementations *********************/

/* only in editmode */
static void vgroup_select_verts(Object *ob, int select)
{
	EditVert *eve;
	MDeformVert *dvert;
	int i;

	if(ob->type == OB_MESH) {
		Mesh *me= ob->data;
		EditMesh *em = BKE_mesh_get_editmesh(me);

		for(eve=em->verts.first; eve; eve=eve->next){
			dvert= CustomData_em_get(&em->vdata, eve->data, CD_MDEFORMVERT);

			if(dvert && dvert->totweight){
				for(i=0; i<dvert->totweight; i++){
					if(dvert->dw[i].def_nr == (ob->actdef-1)){
						if(!eve->h) {
							if(select) eve->f |= SELECT;
							else eve->f &= ~SELECT;
						}
						break;
					}
				}
			}
		}
		/* this has to be called, because this function operates on vertices only */
		if(select) EM_select_flush(em);	// vertices to edges/faces
		else EM_deselect_flush(em);

		BKE_mesh_end_editmesh(me, em);
	}
	else if(ob->type == OB_LATTICE) {
		Lattice *lt= vgroup_edit_lattice(ob);
		
		if(lt->dvert) {
			BPoint *bp;
			int a, tot;
			
			dvert= lt->dvert;

			tot= lt->pntsu*lt->pntsv*lt->pntsw;
			for(a=0, bp= lt->def; a<tot; a++, bp++, dvert++) {
				for(i=0; i<dvert->totweight; i++){
					if(dvert->dw[i].def_nr == (ob->actdef-1)) {
						if(select) bp->f1 |= SELECT;
						else bp->f1 &= ~SELECT;
						
						break;
					}
				}
			}
		}
	}
}

static void vgroup_duplicate(Object *ob)
{
	bDeformGroup *dg, *cdg;
	char name[sizeof(dg->name)];
	MDeformWeight *org, *cpy;
	MDeformVert *dvert, **dvert_array=NULL;
	int i, idg, icdg, dvert_tot=0;

	dg = BLI_findlink(&ob->defbase, (ob->actdef-1));
	if(!dg)
		return;
	
	if(!strstr(dg->name, "_copy")) {
		BLI_snprintf(name, sizeof(name), "%s_copy", dg->name);
	}
	else {
		BLI_snprintf(name, sizeof(name), "%s", dg->name);
	}

	cdg = defgroup_duplicate(dg);
	strcpy(cdg->name, name);
	defgroup_unique_name(cdg, ob);

	BLI_addtail(&ob->defbase, cdg);

	idg = (ob->actdef-1);
	ob->actdef = BLI_countlist(&ob->defbase);
	icdg = (ob->actdef-1);

	ED_vgroup_give_parray(ob->data, &dvert_array, &dvert_tot);
	
	if(!dvert_array)
		return;

	for(i = 0; i < dvert_tot; i++) {
		dvert = dvert_array[i];
		org = defvert_find_index(dvert, idg);
		if(org) {
			float weight = org->weight;
			/* defvert_verify_index re-allocs org so need to store the weight first */
			cpy = defvert_verify_index(dvert, icdg);
			cpy->weight = weight;
		}
	}

	MEM_freeN(dvert_array);
}

static void vgroup_normalize(Object *ob)
{
	bDeformGroup *dg;
	MDeformWeight *dw;
	MDeformVert *dvert, **dvert_array=NULL;
	int i, def_nr, dvert_tot=0;
	// Jason
	Mesh *me = ob->data;
	MVert *mv = me->mvert;
	int selectedVerts = me->editflag & ME_EDIT_VERT_SEL;

	ED_vgroup_give_parray(ob->data, &dvert_array, &dvert_tot);

	dg = BLI_findlink(&ob->defbase, (ob->actdef-1));

	if(dg) {
		float weight_max = 0.0f;

		def_nr= ob->actdef-1;

		for(i = 0; i < dvert_tot; i++) {
			// Jason
			if(selectedVerts && !((mv+i)->flag & SELECT)) {
				continue;
			}

			dvert = dvert_array[i];
			dw = defvert_find_index(dvert, def_nr);
			if(dw) {
				weight_max = MAX2(dw->weight, weight_max);
			}
		}

		if(weight_max > 0.0f) {
			for(i = 0; i < dvert_tot; i++) {
				// Jason
				if(selectedVerts && !((mv+i)->flag & SELECT)) {
					continue;
				}

				dvert = dvert_array[i];
				dw = defvert_find_index(dvert, def_nr);
				if(dw) {
					dw->weight /= weight_max;
					
					/* incase of division errors with very low weights */
					CLAMP(dw->weight, 0.0f, 1.0f);
				}
			}
		}
	}

	if (dvert_array) MEM_freeN(dvert_array);
}
// Jason
static int tryToAddVerts(int *verts, int length, int a, int b) {
	char containsA = FALSE;
	char containsB = FALSE;
	int added = 0;
	int i;
	for(i = 0; i < length && (!containsA || !containsB); i++) {
		if(verts[i] == a) {
			containsA = TRUE;
		} else if(verts[i] == b) {
			containsB = TRUE;
		} else if(verts[i] == -1) {
			if(!containsA) {
				verts[i] = a;
				containsA = TRUE;
				added++;
			} else if(!containsB){
				verts[i] = b;
				containsB = TRUE;
				added++;
			}
		}
	}
	return added;
}
//Jason
static int* getSurroundingVerts(Mesh *me, int vert, int *count) {
	int length = 0;
	int *tverts;
	int *verts = NULL;
	MFace *mf = me->mface;
	int totface = me->totface;
	int found = 0;
	int i;
	for(i = 0; i < totface; i++, mf++) {
		if(vert == mf->v1 || vert == mf->v2 || vert == mf->v3 || (mf->v4 &&vert == mf->v4)) {
			length+=2;
		}
	}
	if(!length) {
		return 0;
	}
	tverts = MEM_mallocN(sizeof(int)*length, "tempSurroundingVerts");
	mf = me->mface;
	for(i = 0; i < length; i++) {
		tverts[i] = -1;
	}
	for(i = 0; i < totface; i++, mf++) {
		int a=-1, b=-1;
		//printf("face %d verts: %d %d %d %d\n", i, mf->v1, mf->v2, mf->v3, mf->v4);
		if(mf->v1 == vert) {
			a = mf->v2;
			if(mf->v4) {
				b = mf->v4;
			} else {
				b = mf->v3;
			}
		} else if(mf->v2 == vert) {
			a = mf->v1;
			b = mf->v3;
		} else if(mf->v3 == vert) {
			a = mf->v2;
			if(mf->v4) {
				b = mf->v4;
			} else {
				b = mf->v1;
			}
		} else if (mf->v4 && mf->v4 == vert){
			a = mf->v1;
			b = mf->v3;
		} else {
			continue;
		}
		found += tryToAddVerts(tverts, length, a, b);
	}
	if(found) {
		verts = MEM_mallocN(sizeof(int)* found, "surroundingVerts");
		for(i = 0; i < found; i++) {
			verts[i] = tverts[i];
		}
		count[0] = found;
	}
	MEM_freeN(tverts);
	return verts;
}
static void getSingleCoordinate(MVert **points, int count, float *coord) {
	int i, k;
	for(k = 0; k < 3; k++) {
		coord[k] = 0;
	}
	for(i = 0; i < count; i++) {
		for(k = 0; k < 3; k++) {
			coord[k] += points[i]->co[k];
		}
	}
	for(k = 0; k < 3; k++) {
		coord[k] /= count;
	}
}
static void getNearestPointOnPlane(float *norm, float d, float *coord, float *point, float *dst) {
	float *temp = MEM_callocN(sizeof(float)*3, "temp");
	int i;
	float dotprod = 0;
	for(i = 0; i < 3; i++) {
		temp[i] = point[i]-coord[i];
	}
	for(i = 0; i < 3; i++) {
		dotprod += temp[i]*norm[i];
	}
	MEM_freeN(temp);
	for(i = 0; i < 3; i++) {
		dst[i] = point[i] - dotprod*norm[i];
	}
}
static float distance(float* a, float *b, int length) {
	int i;
	float sum = 0;
	for(i = 0; i < length; i++) {
		sum += (b[i]-a[i])*(b[i]-a[i]);
	}
	return sqrt(sum);
}
static void getVerticalAndHorizontalChange(float *norm, float d, float *coord, float *start, float distToStart, float *end, float **changes, float *dists, int index) {
	// A=Q-((Q-P).N)N
	// D = (a*x0 + b*y0 +c*z0 +d)
	float *projA, *projB;
	projA = MEM_callocN(sizeof(float)*3, "projectedA");
	projB = MEM_callocN(sizeof(float)*3, "projectedB");
	getNearestPointOnPlane(norm, d, coord, start, projA);
	getNearestPointOnPlane(norm, d, coord, end, projB);
	// (vertical and horizontal refer to the plane's y and xz respectively)
	// vertical distance
	dists[index] = norm[0]*end[0] + norm[1]*end[1] + norm[2]*end[2] + d;
	// vertical change
	changes[index][0] = dists[index] - distToStart;
	//printf("vc %f %f\n", distance(end, projB, 3)-distance(start, projA, 3), changes[index][0]);
	// horizontal change
	changes[index][1] = distance(projA, projB, 3);
	
	MEM_freeN(projA);
	MEM_freeN(projB);
}
// Jason
static int dm_deform_clear(DerivedMesh *dm, Object *ob) {
	dm->needsFree = 1;
	dm->release(dm);
	ob->derivedDeform=NULL;
	// dm = NULL;
	return NULL;
}
// Jason
static DerivedMesh* dm_deform_recalc(Scene *scene, Object *ob) {
	return mesh_get_derived_deform(scene, ob, CD_MASK_BAREMESH);
}
// Jason
static void moveCloserToDistanceFromPlane(Scene *scene, Object *ob, Mesh *me, int index, float *norm, float *coord, float d, float distToBe, float strength, float cp) {
	DerivedMesh *dm;
	MDeformWeight *dw;
	MVert m;
	MDeformVert *dvert = me->dvert+index;
	int totweight = dvert->totweight;
	float oldw = 0;
	float *oldPos = MEM_callocN(sizeof(float)*3, "oldPosition");
	float vc, hc, dist;
	int i, k;
	float **changes = MEM_mallocN(sizeof(float *)*totweight, "vertHorzChange");
	float *dists = MEM_mallocN(sizeof(float)*totweight, "distance");
	int *upDown = MEM_callocN(sizeof(int)*totweight, "upDownTracker");// track if up or down moved it closer for each bone
	int *dwIndices = MEM_callocN(sizeof(int)*totweight, "dwIndexTracker");
	float distToStart;
	int bestIndex = 0;
	char wasChange;
	char wasUp;
	int lastIndex = -1;
	float originalDistToBe = distToBe;
	for(i = 0; i < totweight; i++) {
		changes[i] = MEM_callocN(sizeof(float)*2, "vertHorzChange_"+i);
	}
	do {
		wasChange = FALSE;
		dm = dm_deform_recalc(scene, ob);
		dm->getVert(dm, index, &m);
		oldPos[0] = m.co[0];
		oldPos[1] = m.co[1];
		oldPos[2] = m.co[2];
		distToStart = norm[0]*oldPos[0] + norm[1]*oldPos[1] + norm[2]*oldPos[2] + d;
		//printf("dist %f \n",distToStart);
		//printf("mesh point %f %f %f dpoint %f %f %f \n", (me->mvert+index)->co[0], (me->mvert+index)->co[1], (me->mvert+index)->co[3], oldPos[0], oldPos[1], oldPos[2]);
		if(distToBe == originalDistToBe) {
			distToBe += distToStart - distToStart*strength;
		}
		for(i = 0; i < totweight; i++) {
			dwIndices[i] = i;
			dw = (dvert->dw+i);
			vc = hc = 0;
			if(!dw->weight) {
				changes[i][0] = 0;
				changes[i][1] = 0;
				dists[i] = distToStart;
				continue;
			}
			for(k = 0; k < 2; k++) {
				if(dm) {
					dm = dm_deform_clear(dm, ob);
				}
				oldw = dw->weight;
				if(k) {
					dw->weight *= 1+cp;
				} else {
					dw->weight /= 1+cp;
				}
				if(dw->weight == oldw) {
					changes[i][0] = 0;
					changes[i][1] = 0;
					dists[i] = distToStart;
					break;
				}
				if(dw->weight > 1) {
					dw->weight = 1;
				}
				dm = dm_deform_recalc(scene, ob);
				dm->getVert(dm, index, &m);
				getVerticalAndHorizontalChange(norm, d, coord, oldPos, distToStart, m.co, changes, dists, i);
				dw->weight = oldw;
				if(!k) {
					vc = changes[i][0];
					hc = changes[i][1];
					dist = dists[i];
				} else {
					if(fabs(dist - distToBe) < fabs(dists[i] - distToBe)) {
						upDown[i] = 0;
						changes[i][0] = vc;
						changes[i][1] = hc;
						dists[i] = dist;
					} else {
						upDown[i] = 1;
					}
					if(fabs(dists[i] - distToBe) > fabs(distToStart - distToBe)) {
						changes[i][0] = 0;
						changes[i][1] = 0;
						dists[i] = distToStart;
					}
				}
			}
			//printf("final vc: %f\n", changes[i][0]);
		}
		// sort the changes by the vertical change
		for(k = 0; k < totweight; k++) {
			float tf;
			int ti;
			bestIndex = k;
			for(i = k+1; i < totweight; i++) {
				dist = dists[i];

				if(fabs(dist) > fabs(dists[i])) {
					bestIndex = i;
				}
			}
			// switch with k
			if(bestIndex != k) {
				ti = upDown[k];
				upDown[k] = upDown[bestIndex];
				upDown[bestIndex] = ti;

				ti = dwIndices[k];
				dwIndices[k] = dwIndices[bestIndex];
				dwIndices[bestIndex] = ti;

				tf = changes[k][0];
				changes[k][0] = changes[bestIndex][0];
				changes[bestIndex][0] = tf;

				tf = changes[k][1];
				changes[k][1] = changes[bestIndex][1];
				changes[bestIndex][1] = tf;

				tf = dists[k];
				dists[k] = dists[bestIndex];
				dists[bestIndex] = tf;
			}
		}
		bestIndex = -1;
		// find the best change with an acceptable horizontal change
		for(i = 0; i < totweight; i++) {
			if(fabs(changes[i][0]) > fabs(changes[i][1]*2.0f)) {
				bestIndex = i;
				break;
			}
		}
		if(bestIndex != -1) {
			//printf("changing %d\n", bestIndex);
			wasChange = TRUE;
			if(lastIndex != -1) {
				if(wasUp != upDown[bestIndex]) {
					wasChange = FALSE;
				}
			}
			lastIndex = bestIndex;
			wasUp = upDown[bestIndex];
			dw = (dvert->dw+dwIndices[bestIndex]);
			oldw = dw->weight;
			if(upDown[bestIndex]) {
				dw->weight *= 1+cp;
			} else {
				dw->weight /= 1+cp;
			}
			if(dw->weight > 1) {
				dw->weight = 1;
			}
			if(oldw == dw->weight) {
				wasChange = FALSE;
			}
			if(dm) {
				dm = dm_deform_clear(dm, ob);
			}
		}
		//printf("best vc=%f hc=%f \n", changes[bestIndex][0], changes[bestIndex][1]);
	}while(wasChange && (distToStart-distToBe)/fabs(distToStart-distToBe) == (dists[bestIndex]-distToBe)/fabs(dists[bestIndex]-distToBe));
	MEM_freeN(upDown);
	for(i = 0; i < totweight; i++) {
		MEM_freeN(changes[i]);
	}
	MEM_freeN(changes);
	MEM_freeN(dists);
	MEM_freeN(dwIndices);
	MEM_freeN(oldPos);
}
// Jason
static void vgroup_fix(Scene *scene, Object *ob, float distToBe, float strength, float cp)
{
	int i;

	Mesh *me = ob->data;
	MVert *mv = me->mvert;
	int selectedVerts = me->editflag & ME_EDIT_VERT_SEL;
	int *verts = NULL;
	int *pcount = MEM_callocN(sizeof(int), "intPointer");
	for(i = 0; i < me->totvert && mv; i++, mv++) {
		// Jason
		if(selectedVerts && (mv->flag & SELECT)) {
			
			int count;
			if((verts = getSurroundingVerts(me, i, pcount))) {
				MVert m;
				MVert **p = MEM_callocN(sizeof(MVert*)*(count = pcount[0]), "deformedPoints");
				int k;

				DerivedMesh *dm = mesh_get_derived_deform(scene, ob, CD_MASK_BAREMESH);
				for(k = 0; k < count; k++) {
					dm->getVert(dm, verts[k], &m);
					p[k] = &m;
					//printf("deformed coords %f %f %f\n", p[k]->co[0], p[k]->co[1], p[k]->co[2]);
				}
				
				if(count >= 3) {
					float d, dist, mag;
					float *coord = MEM_callocN(sizeof(float)*3, "deformedCoord");
					float *norm = MEM_callocN(sizeof(float)*3, "planeNorm");
					getSingleCoordinate(p, count, coord);
					dm->getVert(dm, i, &m);
					norm[0] = m.co[0]-coord[0];
					norm[1] = m.co[1]-coord[1];
					norm[2] = m.co[2]-coord[2];
					mag = sqrt(norm[0]*norm[0] + norm[1]*norm[1] + norm[2]*norm[2]);
					for(k = 0; k < 3; k++) {
						norm[k]/=mag;
					}
					d = -norm[0]*coord[0] -norm[1]*coord[1] -norm[2]*coord[2];
					dist = (norm[0]*m.co[0] + norm[1]*m.co[1] + norm[2]*m.co[2] + d);
					//printf("status plane: (%f %f %f) %f dist: %f\n", norm[0], norm[1], norm[2], d, dist);
					moveCloserToDistanceFromPlane(scene, ob, me, i, norm, coord, d, distToBe, strength, cp);
					MEM_freeN(coord);
					MEM_freeN(norm);
				}

				MEM_freeN(verts);
				MEM_freeN(p);
			}
		}
	}
	MEM_freeN(pcount);
}

static void vgroup_levels(Object *ob, float offset, float gain)
{
	bDeformGroup *dg;
	MDeformWeight *dw;
	MDeformVert *dvert, **dvert_array=NULL;
	int i, def_nr, dvert_tot=0;
	// Jason
	Mesh *me = ob->data;
	MVert *mv = me->mvert;
	int selectedVerts = me->editflag & ME_EDIT_VERT_SEL;

	ED_vgroup_give_parray(ob->data, &dvert_array, &dvert_tot);
	
	dg = BLI_findlink(&ob->defbase, (ob->actdef-1));
	
	if(dg) {
		def_nr= ob->actdef-1;
		
		for(i = 0; i < dvert_tot; i++) {
			// Jason
			if(selectedVerts && !((mv+i)->flag & SELECT)) {
				continue;
			}

			dvert = dvert_array[i];
			dw = defvert_find_index(dvert, def_nr);
			if(dw) {
				dw->weight = gain * (dw->weight + offset);
				
				CLAMP(dw->weight, 0.0f, 1.0f);
			}
		}
	}

	if (dvert_array) MEM_freeN(dvert_array);
}

/* TODO - select between groups */
static void vgroup_normalize_all(Object *ob, int lock_active)
{
	MDeformWeight *dw, *dw_act;
	MDeformVert *dvert, **dvert_array=NULL;
	int i, dvert_tot=0;
	float tot_weight;

	// Jason
	Mesh *me = ob->data;
	MVert *mv = me->mvert;
	int selectedVerts = me->editflag & ME_EDIT_VERT_SEL;

	ED_vgroup_give_parray(ob->data, &dvert_array, &dvert_tot);

	if(dvert_array) {
		if(lock_active) {
			int def_nr= ob->actdef-1;

			for(i = 0; i < dvert_tot; i++) {
				float lock_iweight= 1.0f;
				int j;
				// Jason
				if(selectedVerts && !((mv+i)->flag & SELECT)) {
					continue;
				}

				tot_weight= 0.0f;
				dw_act= NULL;
				dvert = dvert_array[i];

				j= dvert->totweight;
				while(j--) {
					dw= dvert->dw + j;

					if(dw->def_nr==def_nr) {
						dw_act= dw;
						lock_iweight = (1.0f - dw_act->weight);
					}
					else {
						tot_weight += dw->weight;
					}
				}

				if(tot_weight) {
					j= dvert->totweight;
					while(j--) {
						dw= dvert->dw + j;
						if(dw == dw_act) {
							if (dvert->totweight==1) {
								dw_act->weight= 1.0f; /* no other weights, set to 1.0 */
							}
						} else {
							if(dw->weight > 0.0f)
								dw->weight = (dw->weight / tot_weight) * lock_iweight;
						}

						/* incase of division errors with very low weights */
						CLAMP(dw->weight, 0.0f, 1.0f);
					}
				}
			}
		}
		else {
			for(i = 0; i < dvert_tot; i++) {
				int j;
				// Jason
				if(selectedVerts && !((mv+i)->flag & SELECT)) {
					continue;
				}

				tot_weight= 0.0f;
				dvert = dvert_array[i];

				j= dvert->totweight;
				while(j--) {
					dw= dvert->dw + j;
					tot_weight += dw->weight;
				}

				if(tot_weight) {
					j= dvert->totweight;
					while(j--) {
						dw= dvert->dw + j;
						dw->weight /= tot_weight;

						/* incase of division errors with very low weights */
						CLAMP(dw->weight, 0.0f, 1.0f);
					}
				}
			}
		}
	}

	if (dvert_array) MEM_freeN(dvert_array);
}
/* Jason was here */
static void vgroup_invert_locks(Object *ob)
{
	bDeformGroup *dg = ob->defbase.first;
	while(dg) {
		dg->flag = !dg->flag;
		dg = dg->next;
	}
}
/* Jason was here */
static void vgroup_lock_all(Object *ob)
{
	bDeformGroup *dg = ob->defbase.first;
	while(dg) {
		dg->flag = TRUE;
		dg = dg->next;
	}
}
/* Jason was here */
static void vgroup_unlock_all(Object *ob)
{
	bDeformGroup *dg = ob->defbase.first;
	while(dg) {
		dg->flag = FALSE;
		dg = dg->next;
	}
}

static void vgroup_invert(Object *ob, int auto_assign, int auto_remove)
{
	bDeformGroup *dg;
	MDeformWeight *dw;
	MDeformVert *dvert, **dvert_array=NULL;
	int i, def_nr, dvert_tot=0;
	// Jason
	Mesh *me = ob->data;
	MVert *mv = me->mvert;
	int selectedVerts = me->editflag & ME_EDIT_VERT_SEL;

	ED_vgroup_give_parray(ob->data, &dvert_array, &dvert_tot);

	dg = BLI_findlink(&ob->defbase, (ob->actdef-1));

	if(dg) {
		def_nr= ob->actdef-1;


		for(i = 0; i < dvert_tot; i++) {
			// Jason
			if(selectedVerts && !((mv+i)->flag & SELECT)) {
				continue;
			}
			dvert = dvert_array[i];

			if(auto_assign) {
				dw= defvert_verify_index(dvert, def_nr);
			} else {
				dw= defvert_find_index(dvert, def_nr);
			}

			if(dw) {
				dw->weight = 1.0f-dw->weight;

				if(auto_remove && dw->weight <= 0.0f) {
					/* could have a faster function for this */
					ED_vgroup_nr_vert_remove(ob, def_nr, i);
				}
			}
		}
	}

	if (dvert_array) MEM_freeN(dvert_array);
}

static void vgroup_blend(Object *ob)
{
	bDeformGroup *dg;
	MDeformWeight *dw;
	MDeformVert *dvert_array=NULL, *dvert;
	int i, def_nr, dvert_tot=0;

	EditMesh *em= BKE_mesh_get_editmesh(((Mesh *)ob->data));
	// ED_vgroup_give_array(ob->data, &dvert_array, &dvert_tot);

	if(em==NULL)
		return;

	dg = BLI_findlink(&ob->defbase, (ob->actdef-1));

	if(dg) {
		int sel1, sel2;
		int i1, i2;

		EditEdge *eed;
		EditVert *eve;
		float *vg_weights;
		float *vg_users;

		def_nr= ob->actdef-1;

		i= 0;
		for(eve= em->verts.first; eve; eve= eve->next)
			eve->tmp.l= i++;

		dvert_tot= i;

		vg_weights= MEM_callocN(sizeof(float)*dvert_tot, "vgroup_blend_f");
		vg_users= MEM_callocN(sizeof(int)*dvert_tot, "vgroup_blend_i");

		for(eed= em->edges.first; eed; eed= eed->next) {
			sel1= eed->v1->f & SELECT;
			sel2= eed->v2->f & SELECT;

			if(sel1 != sel2) {
				/* i1 is always the selected one */
				if(sel1==TRUE && sel2==FALSE) {
					i1= eed->v1->tmp.l;
					i2= eed->v2->tmp.l;
					eve= eed->v2;
				}
				else {
					i2= eed->v1->tmp.l;
					i1= eed->v2->tmp.l;
					eve= eed->v1;
				}

				vg_users[i1]++;

				/* TODO, we may want object mode blending */
				if(em)	dvert= CustomData_em_get(&em->vdata, eve->data, CD_MDEFORMVERT);
				else	dvert= dvert_array+i2;

				dw= defvert_find_index(dvert, def_nr);

				if(dw) {
					vg_weights[i1] += dw->weight;
				}
			}
		}

		i= 0;
		for(eve= em->verts.first; eve; eve= eve->next) {
			if(eve->f & SELECT && vg_users[i] > 0) {
				/* TODO, we may want object mode blending */
				if(em)	dvert= CustomData_em_get(&em->vdata, eve->data, CD_MDEFORMVERT);
				else	dvert= dvert_array+i;

				dw= defvert_verify_index(dvert, def_nr);
				dw->weight= vg_weights[i] / (float)vg_users[i];
			}

			i++;
		}
		MEM_freeN(vg_weights);
		MEM_freeN(vg_users);
	}
}

static void vgroup_clean(Object *ob, float eul, int keep_single)
{
	bDeformGroup *dg;
	MDeformWeight *dw;
	MDeformVert *dvert, **dvert_array=NULL;
	int i, def_nr, dvert_tot=0;
	// Jason
	Mesh *me = ob->data;
	MVert *mv = me->mvert;
	int selectedVerts = me->editflag & ME_EDIT_VERT_SEL;

	ED_vgroup_give_parray(ob->data, &dvert_array, &dvert_tot);

	/* only the active group */
	dg = BLI_findlink(&ob->defbase, (ob->actdef-1));
	if(dg) {
		def_nr= ob->actdef-1;

		for(i = 0; i < dvert_tot; i++) {
			// Jason
			if(selectedVerts && !((mv+i)->flag & SELECT)) {
				continue;
			}
			dvert = dvert_array[i];

			dw= defvert_find_index(dvert, def_nr);

			if(dw) {
				if(dw->weight <= eul)
					if(keep_single==FALSE || dvert->totweight > 1)
						ED_vgroup_nr_vert_remove(ob, def_nr, i);
			}
		}
	}

	if (dvert_array) MEM_freeN(dvert_array);
}

static void vgroup_clean_all(Object *ob, float eul, int keep_single)
{

	MDeformWeight *dw;
	MDeformVert *dvert, **dvert_array=NULL;
	int i, dvert_tot=0;
	// Jason
	Mesh *me = ob->data;
	MVert *mv = me->mvert;
	int selectedVerts = me->editflag & ME_EDIT_VERT_SEL;

	ED_vgroup_give_parray(ob->data, &dvert_array, &dvert_tot);

	if(dvert_array) {
		for(i = 0; i < dvert_tot; i++) {
			int j;
			// Jason
			if(selectedVerts && !((mv+i)->flag & SELECT)) {
				continue;
			}

			dvert = dvert_array[i];
			j= dvert->totweight;

			while(j--) {

				if(keep_single && dvert->totweight == 1)
					break;

				dw= dvert->dw + j;

				if(dw->weight <= eul)
					ED_vgroup_nr_vert_remove(ob, dw->def_nr, i);

			}
		}
	}

	if (dvert_array) MEM_freeN(dvert_array);
}


static void dvert_mirror_op(MDeformVert *dvert, MDeformVert *dvert_mirr,
                            const char sel, const char sel_mirr,
                            const int *flip_map,
                            const short mirror_weights, const short flip_vgroups)
{
	BLI_assert(sel || sel_mirr);

	if(sel_mirr && sel) {
		/* swap */
		if(mirror_weights)
			SWAP(MDeformVert, *dvert, *dvert_mirr);
		if(flip_vgroups) {
			defvert_flip(dvert, flip_map);
			defvert_flip(dvert_mirr, flip_map);
		}
	}
	else {
		/* dvert should always be the target */
		if(sel_mirr) {
			SWAP(MDeformVert *, dvert, dvert_mirr);
		}

		if(mirror_weights)
			defvert_copy(dvert, dvert_mirr);
		if(flip_vgroups) {
			defvert_flip(dvert, flip_map);
		}
	}
}

void ED_vgroup_mirror(Object *ob, const short mirror_weights, const short flip_vgroups)
{
#define VGROUP_MIRR_OP dvert_mirror_op(dvert, dvert_mirr, sel, sel_mirr, flip_map, mirror_weights, flip_vgroups)

	EditVert *eve, *eve_mirr;
	MDeformVert *dvert, *dvert_mirr;
	short sel, sel_mirr;
	int	*flip_map;

	if(mirror_weights==0 && flip_vgroups==0)
		return;

	flip_map= defgroup_flip_map(ob, 0);

	/* only the active group */
	if(ob->type == OB_MESH) {
		Mesh *me= ob->data;
		EditMesh *em = BKE_mesh_get_editmesh(me);


		if(!CustomData_has_layer(&em->vdata, CD_MDEFORMVERT)) {
			MEM_freeN(flip_map);
			return;
		}

		EM_cache_x_mirror_vert(ob, em);

		/* Go through the list of editverts and assign them */
		for(eve=em->verts.first; eve; eve=eve->next){
			if((eve_mirr=eve->tmp.v)) {
				sel= eve->f & SELECT;
				sel_mirr= eve_mirr->f & SELECT;

				if((sel || sel_mirr) && (eve != eve_mirr)) {
					dvert= CustomData_em_get(&em->vdata, eve->data, CD_MDEFORMVERT);
					dvert_mirr= CustomData_em_get(&em->vdata, eve_mirr->data, CD_MDEFORMVERT);
					if(dvert && dvert_mirr) {
						VGROUP_MIRR_OP;
					}
				}

				eve->tmp.v= eve_mirr->tmp.v= NULL;
			}
		}

		BKE_mesh_end_editmesh(me, em);
	}
	else if (ob->type == OB_LATTICE) {
		Lattice *lt= ob->data;
		int i1, i2;
		int u, v, w;
		int pntsu_half;
		/* half but found up odd value */

		if(lt->editlatt) lt= lt->editlatt->latt;

		if(lt->pntsu == 1 || lt->dvert == NULL) {
			MEM_freeN(flip_map);
			return;
		}

		/* unlike editmesh we know that by only looping over the first hald of
		 * the 'u' indicies it will cover all points except the middle which is
		 * ok in this case */
		pntsu_half= lt->pntsu / 2;

		for(w=0; w<lt->pntsw; w++) {
			for(v=0; v<lt->pntsv; v++) {
				for(u=0; u<pntsu_half; u++) {
					int u_inv= (lt->pntsu - 1) - u;
					if(u != u_inv) {
						BPoint *bp, *bp_mirr;

						i1= LT_INDEX(lt, u, v, w);
						i2= LT_INDEX(lt, u_inv, v, w);

						bp= &lt->def[i1];
						bp_mirr= &lt->def[i2];

						sel= bp->f1 & SELECT;
						sel_mirr= bp_mirr->f1 & SELECT;

						if(sel || sel_mirr) {
							dvert= &lt->dvert[i1];
							dvert_mirr= &lt->dvert[i2];

							VGROUP_MIRR_OP;
						}
					}
				}
			}
		}
	}

	MEM_freeN(flip_map);

#undef VGROUP_MIRR_OP
}

static void vgroup_remap_update_users(Object *ob, int *map)
{
	ExplodeModifierData *emd;
	ModifierData *md;
	ParticleSystem *psys;
	ClothModifierData *clmd;
	ClothSimSettings *clsim;
	int a;

	/* these cases don't use names to refer to vertex groups, so when
	 * they get deleted the numbers get out of sync, this corrects that */

	if(ob->soft)
		ob->soft->vertgroup= map[ob->soft->vertgroup];

	for(md=ob->modifiers.first; md; md=md->next) {
		if(md->type == eModifierType_Explode) {
			emd= (ExplodeModifierData*)md;
			emd->vgroup= map[emd->vgroup];
		}
		else if(md->type == eModifierType_Cloth) {
			clmd= (ClothModifierData*)md;
			clsim= clmd->sim_parms;

			if(clsim) {
				clsim->vgroup_mass= map[clsim->vgroup_mass];
				clsim->vgroup_bend= map[clsim->vgroup_bend];
				clsim->vgroup_struct= map[clsim->vgroup_struct];
			}
		}
	}

	for(psys=ob->particlesystem.first; psys; psys=psys->next) {
		for(a=0; a<PSYS_TOT_VG; a++)
			psys->vgroup[a]= map[psys->vgroup[a]];
	}
}


static void vgroup_delete_update_users(Object *ob, int id)
{
	int i, tot= BLI_countlist(&ob->defbase) + 1;
	int *map= MEM_mallocN(sizeof(int) * tot, "vgroup del");

	map[id]= map[0]= 0;
	for(i=1; i<id; i++) map[i]=i;
	for(i=id+1; i<tot; i++) map[i]=i-1;

	vgroup_remap_update_users(ob, map);
	MEM_freeN(map);
}


static void vgroup_delete_object_mode(Object *ob, bDeformGroup *dg)
{
	MDeformVert *dvert_array=NULL;
	int i, e, dvert_tot=0;
	const int dg_index= BLI_findindex(&ob->defbase, dg);

	assert(dg_index > -1);
	
	ED_vgroup_give_array(ob->data, &dvert_array, &dvert_tot);

	if(dvert_array) {
		MDeformVert *dvert;
		for(i= 0, dvert= dvert_array; i < dvert_tot; i++, dvert++) {
			ED_vgroup_vert_remove(ob, dg, i); /* ok if the dg isnt in this dvert, will continue silently */
		}

		for(i= 0, dvert= dvert_array; i < dvert_tot; i++, dvert++) {
			for(e = 0; e < dvert->totweight; e++) {
				if(dvert->dw[e].def_nr > dg_index) {
					dvert->dw[e].def_nr--;
				}
			}
		}
	}

	vgroup_delete_update_users(ob, dg_index + 1);

	/* Remove the group */
	BLI_freelinkN(&ob->defbase, dg);

	/* Update the active deform index if necessary */
	if(ob->actdef > dg_index)
		ob->actdef--;
	if(ob->actdef < 1 && ob->defbase.first)
		ob->actdef= 1;

}

/* only in editmode */
/* removes from active defgroup, if allverts==0 only selected vertices */
static void vgroup_active_remove_verts(Object *ob, const int allverts, bDeformGroup *dg)
{
	EditVert *eve;
	MDeformVert *dvert;
	MDeformWeight *newdw;
	bDeformGroup *eg;
	int	i;

	if(ob->type == OB_MESH) {
		Mesh *me= ob->data;
		EditMesh *em = BKE_mesh_get_editmesh(me);

		for(eve=em->verts.first; eve; eve=eve->next){
			dvert= CustomData_em_get(&em->vdata, eve->data, CD_MDEFORMVERT);
		
			if(dvert && dvert->dw && ((eve->f & SELECT) || allverts)){
				for(i=0; i<dvert->totweight; i++){
					/* Find group */
					eg = BLI_findlink(&ob->defbase, dvert->dw[i].def_nr);
					if(eg == dg){
						dvert->totweight--;
						if(dvert->totweight){
							newdw = MEM_mallocN(sizeof(MDeformWeight)*(dvert->totweight), "deformWeight");
							
							if(dvert->dw){
								memcpy(newdw, dvert->dw, sizeof(MDeformWeight)*i);
								memcpy(newdw+i, dvert->dw+i+1, sizeof(MDeformWeight)*(dvert->totweight-i));
								MEM_freeN(dvert->dw);
							}
							dvert->dw=newdw;
						}
						else{
							MEM_freeN(dvert->dw);
							dvert->dw=NULL;
							break;
						}
					}
				}
			}
		}
		BKE_mesh_end_editmesh(me, em);
	}
	else if(ob->type == OB_LATTICE) {
		Lattice *lt= vgroup_edit_lattice(ob);
		
		if(lt->dvert) {
			BPoint *bp;
			int a, tot= lt->pntsu*lt->pntsv*lt->pntsw;
				
			for(a=0, bp= lt->def; a<tot; a++, bp++) {
				if(allverts || (bp->f1 & SELECT))
					ED_vgroup_vert_remove(ob, dg, a);
			}
		}
	}
}

static void vgroup_delete_edit_mode(Object *ob, bDeformGroup *dg)
{
	int i;
	const int dg_index= BLI_findindex(&ob->defbase, dg);

	assert(dg_index > -1);

	/* Make sure that no verts are using this group */
	vgroup_active_remove_verts(ob, TRUE, dg);

	/* Make sure that any verts with higher indices are adjusted accordingly */
	if(ob->type==OB_MESH) {
		Mesh *me= ob->data;
		EditMesh *em = BKE_mesh_get_editmesh(me);
		EditVert *eve;
		MDeformVert *dvert;
		
		for(eve=em->verts.first; eve; eve=eve->next){
			dvert= CustomData_em_get(&em->vdata, eve->data, CD_MDEFORMVERT);

			if(dvert)
				for(i=0; i<dvert->totweight; i++)
					if(dvert->dw[i].def_nr > dg_index)
						dvert->dw[i].def_nr--;
		}
		BKE_mesh_end_editmesh(me, em);
	}
	else if(ob->type==OB_LATTICE) {
		Lattice *lt= vgroup_edit_lattice(ob);
		BPoint *bp;
		MDeformVert *dvert= lt->dvert;
		int a, tot;
		
		if(dvert) {
			tot= lt->pntsu*lt->pntsv*lt->pntsw;
			for(a=0, bp= lt->def; a<tot; a++, bp++, dvert++) {
				for(i=0; i<dvert->totweight; i++){
					if(dvert->dw[i].def_nr > dg_index)
						dvert->dw[i].def_nr--;
				}
			}
		}
	}

	vgroup_delete_update_users(ob, dg_index + 1);

	/* Remove the group */
	BLI_freelinkN (&ob->defbase, dg);

	/* Update the active deform index if necessary */
	if(ob->actdef > dg_index)
		ob->actdef--;
	if(ob->actdef < 1 && ob->defbase.first)
		ob->actdef= 1;

	/* remove all dverts */
	if(ob->defbase.first == NULL) {
		if(ob->type==OB_MESH) {
			Mesh *me= ob->data;
			CustomData_free_layer_active(&me->vdata, CD_MDEFORMVERT, me->totvert);
			me->dvert= NULL;
		}
		else if(ob->type==OB_LATTICE) {
			Lattice *lt= vgroup_edit_lattice(ob);
			if(lt->dvert) {
				MEM_freeN(lt->dvert);
				lt->dvert= NULL;
			}
		}
	}
}

static int vgroup_object_in_edit_mode(Object *ob)
{
	if(ob->type == OB_MESH)
		return (((Mesh*)ob->data)->edit_mesh != NULL);
	else if(ob->type == OB_LATTICE)
		return (((Lattice*)ob->data)->editlatt != NULL);
	
	return 0;
}

static void vgroup_delete(Object *ob)
{
	bDeformGroup *dg = BLI_findlink(&ob->defbase, ob->actdef-1);
	if(!dg)
		return;

	if(vgroup_object_in_edit_mode(ob))
		vgroup_delete_edit_mode(ob, dg);
	else
		vgroup_delete_object_mode(ob, dg);
}

static void vgroup_delete_all(Object *ob)
{
	/* Remove all DVerts */
	if(ob->type==OB_MESH) {
		Mesh *me= ob->data;
		CustomData_free_layer_active(&me->vdata, CD_MDEFORMVERT, me->totvert);
		me->dvert= NULL;
	}
	else if(ob->type==OB_LATTICE) {
		Lattice *lt= vgroup_edit_lattice(ob);
		if(lt->dvert) {
			MEM_freeN(lt->dvert);
			lt->dvert= NULL;
		}
	}
	
	/* Remove all DefGroups */
	BLI_freelistN(&ob->defbase);
	
	/* Fix counters/indices */
	ob->actdef= 0;
}

/* only in editmode */
static void vgroup_assign_verts(Object *ob, float weight)
{
	EditVert *eve;
	bDeformGroup *dg, *eg;
	MDeformWeight *newdw;
	MDeformVert *dvert;
	int	i, done;

	dg=BLI_findlink(&ob->defbase, ob->actdef-1);
	if(!dg)
		return;

	if(ob->type == OB_MESH) {
		Mesh *me= ob->data;
		EditMesh *em = BKE_mesh_get_editmesh(me);

		if(!CustomData_has_layer(&em->vdata, CD_MDEFORMVERT))
			EM_add_data_layer(em, &em->vdata, CD_MDEFORMVERT, NULL);

		/* Go through the list of editverts and assign them */
		for(eve=em->verts.first; eve; eve=eve->next){
			dvert= CustomData_em_get(&em->vdata, eve->data, CD_MDEFORMVERT);

			if(dvert && (eve->f & SELECT)){
				/* See if this vert already has a reference to this group */
				/*		If so: Change its weight */
				done=0;
				for(i=0; i<dvert->totweight; i++){
					eg = BLI_findlink(&ob->defbase, dvert->dw[i].def_nr);
					/* Find the actual group */
					if(eg==dg){
						dvert->dw[i].weight= weight;
						done=1;
						break;
					}
				 }
				/*		If not: Add the group and set its weight */
				if(!done){
					newdw = MEM_callocN(sizeof(MDeformWeight)*(dvert->totweight+1), "deformWeight");
					if(dvert->dw){
						memcpy(newdw, dvert->dw, sizeof(MDeformWeight)*dvert->totweight);
						MEM_freeN(dvert->dw);
					}
					dvert->dw=newdw;

					dvert->dw[dvert->totweight].weight= weight;
					dvert->dw[dvert->totweight].def_nr= ob->actdef-1;

					dvert->totweight++;

				}
			}
		}
		BKE_mesh_end_editmesh(me, em);
	}
	else if(ob->type == OB_LATTICE) {
		Lattice *lt= vgroup_edit_lattice(ob);
		BPoint *bp;
		int a, tot;
		
		if(lt->dvert==NULL)
			ED_vgroup_data_create(&lt->id);
		
		tot= lt->pntsu*lt->pntsv*lt->pntsw;
		for(a=0, bp= lt->def; a<tot; a++, bp++) {
			if(bp->f1 & SELECT)
				ED_vgroup_nr_vert_add(ob, ob->actdef-1, a, weight, WEIGHT_REPLACE);
		}
	}
}

/* only in editmode */
/* removes from all defgroup, if allverts==0 only selected vertices */
static void vgroup_remove_verts(Object *ob, int allverts)
{
	/* To prevent code redundancy, we just use vgroup_active_remove_verts, but that
	 * only operates on the active vgroup. So we iterate through all groups, by changing
	 * active group index
	 */
	bDeformGroup *dg;
	for(dg= ob->defbase.first; dg; dg= dg->next) {
		vgroup_active_remove_verts(ob, allverts, dg);
	}
}

/********************** vertex group operators *********************/

static int vertex_group_poll(bContext *C)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;
	ID *data= (ob)? ob->data: NULL;
	return (ob && !ob->id.lib && ELEM(ob->type, OB_MESH, OB_LATTICE) && data && !data->lib);
}

static int vertex_group_poll_edit(bContext *C)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;
	ID *data= (ob)? ob->data: NULL;

	if(!(ob && !ob->id.lib && data && !data->lib))
		return 0;

	return vgroup_object_in_edit_mode(ob);
}

static int vertex_group_add_exec(bContext *C, wmOperator *UNUSED(op))
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;

	ED_vgroup_add(ob);
	DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob->data);
	WM_event_add_notifier(C, NC_OBJECT|ND_DRAW, ob);
	
	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_add(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Add Vertex Group";
	ot->idname= "OBJECT_OT_vertex_group_add";
	
	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_add_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
}

static int vertex_group_remove_exec(bContext *C, wmOperator *op)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;

	if(RNA_boolean_get(op->ptr, "all"))
		vgroup_delete_all(ob);
	else
		vgroup_delete(ob);

	DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob->data);
	WM_event_add_notifier(C, NC_OBJECT|ND_DRAW, ob);
	
	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_remove(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Remove Vertex Group";
	ot->idname= "OBJECT_OT_vertex_group_remove";
	
	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_remove_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;

	/* properties */
	RNA_def_boolean(ot->srna, "all", 0, "All", "Remove from all vertex groups.");
}

static int vertex_group_assign_exec(bContext *C, wmOperator *op)
{
	ToolSettings *ts= CTX_data_tool_settings(C);
	Object *ob= CTX_data_edit_object(C);

	if(RNA_boolean_get(op->ptr, "new"))
		ED_vgroup_add(ob);

	vgroup_assign_verts(ob, ts->vgroup_weight);
	DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob->data);
	
	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_assign(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Assign Vertex Group";
	ot->idname= "OBJECT_OT_vertex_group_assign";
	
	/* api callbacks */
	ot->poll= vertex_group_poll_edit;
	ot->exec= vertex_group_assign_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;

	/* properties */
	RNA_def_boolean(ot->srna, "new", 0, "New", "Assign vertex to new vertex group.");
}

static int vertex_group_remove_from_exec(bContext *C, wmOperator *op)
{
	Object *ob= CTX_data_edit_object(C);

	if(RNA_boolean_get(op->ptr, "all"))
		vgroup_remove_verts(ob, 0);
	else {
		bDeformGroup *dg= BLI_findlink(&ob->defbase, ob->actdef - 1);

		if(dg == NULL) {
			return OPERATOR_CANCELLED;
		}

		vgroup_active_remove_verts(ob, FALSE, dg);
	}

	DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob->data);

	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_remove_from(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Remove from Vertex Group";
	ot->idname= "OBJECT_OT_vertex_group_remove_from";

	/* api callbacks */
	ot->poll= vertex_group_poll_edit;
	ot->exec= vertex_group_remove_from_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;

	/* properties */
	RNA_def_boolean(ot->srna, "all", 0, "All", "Remove from all vertex groups.");
}

static int vertex_group_select_exec(bContext *C, wmOperator *UNUSED(op))
{
	Object *ob= CTX_data_edit_object(C);

	if(!ob || ob->id.lib)
		return OPERATOR_CANCELLED;

	vgroup_select_verts(ob, 1);
	WM_event_add_notifier(C, NC_GEOM|ND_SELECT, ob->data);

	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_select(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Select Vertex Group";
	ot->idname= "OBJECT_OT_vertex_group_select";

	/* api callbacks */
	ot->poll= vertex_group_poll_edit;
	ot->exec= vertex_group_select_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
}

static int vertex_group_deselect_exec(bContext *C, wmOperator *UNUSED(op))
{
	Object *ob= CTX_data_edit_object(C);

	vgroup_select_verts(ob, 0);
	WM_event_add_notifier(C, NC_GEOM|ND_SELECT, ob->data);

	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_deselect(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Deselect Vertex Group";
	ot->idname= "OBJECT_OT_vertex_group_deselect";

	/* api callbacks */
	ot->poll= vertex_group_poll_edit;
	ot->exec= vertex_group_deselect_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
}

static int vertex_group_copy_exec(bContext *C, wmOperator *UNUSED(op))
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;

	vgroup_duplicate(ob);
	DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_OBJECT|ND_DRAW, ob);
	WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob->data);

	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_copy(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Copy Vertex Group";
	ot->idname= "OBJECT_OT_vertex_group_copy";

	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_copy_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
}

static int vertex_group_levels_exec(bContext *C, wmOperator *op)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;
	
	float offset= RNA_float_get(op->ptr,"offset");
	float gain= RNA_float_get(op->ptr,"gain");
	
	vgroup_levels(ob, offset, gain);
	
	DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_OBJECT|ND_DRAW, ob);
	WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob->data);
	
	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_levels(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Vertex Group Levels";
	ot->idname= "OBJECT_OT_vertex_group_levels";
	
	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_levels_exec;
	
	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
	
	RNA_def_float(ot->srna, "offset", 0.f, -1.0, 1.0, "Offset", "Value to add to weights.", -1.0f, 1.f);
	RNA_def_float(ot->srna, "gain", 1.f, 0.f, FLT_MAX, "Gain", "Value to multiply weights by.", 0.0f, 10.f);
}

static int vertex_group_normalize_exec(bContext *C, wmOperator *UNUSED(op))
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;

	vgroup_normalize(ob);

	DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_OBJECT|ND_DRAW, ob);
	WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob->data);

	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_normalize(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Normalize Vertex Group";
	ot->idname= "OBJECT_OT_vertex_group_normalize";

	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_normalize_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
}

static int vertex_group_normalize_all_exec(bContext *C, wmOperator *op)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;
	int lock_active= RNA_boolean_get(op->ptr,"lock_active");

	vgroup_normalize_all(ob, lock_active);

	DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_OBJECT|ND_DRAW, ob);
	WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob->data);

	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_normalize_all(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Normalize All Vertex Groups";
	ot->idname= "OBJECT_OT_vertex_group_normalize_all";

	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_normalize_all_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;

	RNA_def_boolean(ot->srna, "lock_active", TRUE, "Lock Active", "Keep the values of the active group while normalizing others.");
}
/* Jason */
static int vertex_group_fix_exec(bContext *C, wmOperator *op)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;
	Scene *scene= CTX_data_scene(C);
	
	float distToBe= RNA_float_get(op->ptr,"dist");
	float strength= RNA_float_get(op->ptr,"strength");
	float cp= RNA_float_get(op->ptr,"cp");
	
	vgroup_fix(scene, ob, distToBe, strength, cp);
	
	DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_OBJECT|ND_DRAW, ob);
	WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob->data);
	
	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_fix(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Fix Vertex Group Deform";
	ot->idname= "OBJECT_OT_vertex_group_fix";
	
	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_fix_exec;
	
	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
	RNA_def_float(ot->srna, "dist", 0.0f, -FLT_MAX, FLT_MAX, "Distance", "The distance to move to.", -10.0f, 10.0f);	
	RNA_def_float(ot->srna, "strength", 1.f, -2.0f, FLT_MAX, "Strength", "The distance moved can be changed by this multiplier.", -2.0f, 2.0f);
	RNA_def_float(ot->srna, "cp", 1.0f, 0.05f, FLT_MAX, "Change Sensitivity", "Changes the amount weights are altered with each iteration: lower values are slower.", 0.05f, 1.f);
}
/* Jason was here */
static int vertex_group_invert_locks_exec(bContext *C, wmOperator *op)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;

	vgroup_invert_locks(ob);

	return OPERATOR_FINISHED;
}
/* Jason was here */
void OBJECT_OT_vertex_group_invert_locks(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Invert All Vertex Group Locks";
	ot->idname= "OBJECT_OT_vertex_group_invert_locks";

	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_invert_locks_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
}
/* Jason was here */
static int vertex_group_lock_all_exec(bContext *C, wmOperator *op)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;

	vgroup_lock_all(ob);

	return OPERATOR_FINISHED;
}
/* Jason was here */
void OBJECT_OT_vertex_group_lock_all(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Turn on all Vertex Group Locks";
	ot->idname= "OBJECT_OT_vertex_group_lock_all";

	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_lock_all_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
}
/* Jason was here */
static int vertex_group_unlock_all_exec(bContext *C, wmOperator *op)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;

	vgroup_unlock_all(ob);

	return OPERATOR_FINISHED;
}
/* Jason was here */
void OBJECT_OT_vertex_group_unlock_all(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Turn off all Vertex Group Locks";
	ot->idname= "OBJECT_OT_vertex_group_unlock_all";

	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_unlock_all_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
}
static int vertex_group_invert_exec(bContext *C, wmOperator *op)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;
	int auto_assign= RNA_boolean_get(op->ptr,"auto_assign");
	int auto_remove= RNA_boolean_get(op->ptr,"auto_remove");

	vgroup_invert(ob, auto_assign, auto_remove);
	DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_OBJECT|ND_DRAW, ob);
	WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob->data);

	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_invert(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Invert Vertex Group";
	ot->idname= "OBJECT_OT_vertex_group_invert";

	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_invert_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;

	RNA_def_boolean(ot->srna, "auto_assign", TRUE, "Add Weights", "Add verts from groups that have zero weight before inverting.");
	RNA_def_boolean(ot->srna, "auto_remove", TRUE, "Remove Weights", "Remove verts from groups that have zero weight after inverting.");
}


static int vertex_group_blend_exec(bContext *C, wmOperator *UNUSED(op))
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;

	vgroup_blend(ob);

	DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_OBJECT|ND_DRAW, ob);
	WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob->data);

	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_blend(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Blend Vertex Group";
	ot->idname= "OBJECT_OT_vertex_group_blend";
	ot->description= "";

	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_blend_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
}


static int vertex_group_clean_exec(bContext *C, wmOperator *op)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;

	float limit= RNA_float_get(op->ptr,"limit");
	int all_groups= RNA_boolean_get(op->ptr,"all_groups");
	int keep_single= RNA_boolean_get(op->ptr,"keep_single");

	if(all_groups)	vgroup_clean_all(ob, limit, keep_single);
	else			vgroup_clean(ob, limit, keep_single);

	DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_OBJECT|ND_DRAW, ob);
	WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob->data);

	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_clean(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Clean Vertex Group";
	ot->idname= "OBJECT_OT_vertex_group_clean";
	ot->description= "Remove Vertex Group assignments which aren't required";

	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_clean_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;

	RNA_def_float(ot->srna, "limit", 0.01f, 0.0f, 1.0, "Limit", "Remove weights under this limit.", 0.001f, 0.99f);
	RNA_def_boolean(ot->srna, "all_groups", FALSE, "All Groups", "Clean all vertex groups.");
	RNA_def_boolean(ot->srna, "keep_single", FALSE, "Keep Single", "Keep verts assigned to at least one group when cleaning.");
}


static int vertex_group_mirror_exec(bContext *C, wmOperator *op)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;

	ED_vgroup_mirror(ob, RNA_boolean_get(op->ptr,"mirror_weights"), RNA_boolean_get(op->ptr,"flip_group_names"));

	DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_OBJECT|ND_DRAW, ob);
	WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob->data);

	return OPERATOR_FINISHED;
}

void OBJECT_OT_vertex_group_mirror(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Mirror Vertex Group";
	ot->idname= "OBJECT_OT_vertex_group_mirror";
	ot->description= "Mirror all vertex groups, flip weights and/or names, editing only selected vertices, flipping when both sides are selected otherwise copy from unselected";

	/* api callbacks */
	ot->poll= vertex_group_poll_edit;
	ot->exec= vertex_group_mirror_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;

	/* properties */
	RNA_def_boolean(ot->srna, "mirror_weights", TRUE, "Mirror Weights", "Mirror weights.");
	RNA_def_boolean(ot->srna, "flip_group_names", TRUE, "Flip Groups", "Flip vertex group names.");

}

static int vertex_group_copy_to_linked_exec(bContext *C, wmOperator *UNUSED(op))
{
	Scene *scene= CTX_data_scene(C);
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;
	Base *base;
	int retval= OPERATOR_CANCELLED;

	for(base=scene->base.first; base; base= base->next) {
		if(base->object->type==ob->type) {
			if(base->object!=ob && base->object->data==ob->data) {
				BLI_freelistN(&base->object->defbase);
				BLI_duplicatelist(&base->object->defbase, &ob->defbase);
				base->object->actdef= ob->actdef;

				DAG_id_tag_update(&base->object->id, OB_RECALC_DATA);
				WM_event_add_notifier(C, NC_OBJECT|ND_DRAW, base->object);
				WM_event_add_notifier(C, NC_GEOM|ND_DATA, base->object->data);

				retval = OPERATOR_FINISHED;
			}
		}
	}

	return retval;
}

void OBJECT_OT_vertex_group_copy_to_linked(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Copy Vertex Groups to Linked";
	ot->idname= "OBJECT_OT_vertex_group_copy_to_linked";
	ot->description= "Copy Vertex Groups to all users of the same Geometry data";

	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_copy_to_linked_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
}

static int vertex_group_copy_to_selected_exec(bContext *C, wmOperator *op)
{
	Object *obact= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;
	int change= 0;
	int fail= 0;

	CTX_DATA_BEGIN(C, Object*, ob, selected_editable_objects)
	{
		if(obact != ob) {
			if(ED_vgroup_copy_array(ob, obact)) change++;
			else                                fail++;
		}
	}
	CTX_DATA_END;

	if((change == 0 && fail == 0) || fail) {
		BKE_reportf(op->reports, RPT_ERROR, "Copy to VGroups to Selected warning done %d, failed %d, object data must have matching indicies", change, fail);
	}

	return OPERATOR_FINISHED;
}


void OBJECT_OT_vertex_group_copy_to_selected(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Copy Vertex Group to Selected";
	ot->idname= "OBJECT_OT_vertex_group_copy_to_selected";
	ot->description= "Copy Vertex Groups to other selected objects with matching indices";

	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_copy_to_selected_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
}

static EnumPropertyItem vgroup_items[]= {
	{0, NULL, 0, NULL, NULL}};

static int set_active_group_exec(bContext *C, wmOperator *op)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;
	int nr= RNA_enum_get(op->ptr, "group");

	ob->actdef= nr+1;

	DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob);

	return OPERATOR_FINISHED;
}

static EnumPropertyItem *vgroup_itemf(bContext *C, PointerRNA *UNUSED(ptr), PropertyRNA *UNUSED(prop), int *free)
{	
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;
	EnumPropertyItem tmp = {0, "", 0, "", ""};
	EnumPropertyItem *item= NULL;
	bDeformGroup *def;
	int a, totitem= 0;
	
	if(!ob)
		return vgroup_items;
	
	for(a=0, def=ob->defbase.first; def; def=def->next, a++) {
		tmp.value= a;
		tmp.icon= ICON_GROUP_VERTEX;
		tmp.identifier= def->name;
		tmp.name= def->name;
		RNA_enum_item_add(&item, &totitem, &tmp);
	}

	RNA_enum_item_end(&item, &totitem);
	*free= 1;

	return item;
}

void OBJECT_OT_vertex_group_set_active(wmOperatorType *ot)
{
	PropertyRNA *prop;

	/* identifiers */
	ot->name= "Set Active Vertex Group";
	ot->idname= "OBJECT_OT_vertex_group_set_active";
	ot->description= "Set the active vertex group";

	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= set_active_group_exec;
	ot->invoke= WM_menu_invoke;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;

	/* properties */
	prop= RNA_def_enum(ot->srna, "group", vgroup_items, 0, "Group", "Vertex group to set as active.");
	RNA_def_enum_funcs(prop, vgroup_itemf);
	ot->prop= prop;
}

/*creates the name_array parameter for vgroup_do_remap, call this before fiddling
  with the order of vgroups then call vgroup_do_remap after*/
static char *vgroup_init_remap(Object *ob)
{
	bDeformGroup *def;
	int def_tot = BLI_countlist(&ob->defbase);
	char *name_array= MEM_mallocN(MAX_VGROUP_NAME * sizeof(char) * def_tot, "sort vgroups");
	char *name;

	name= name_array;
	for(def = ob->defbase.first; def; def=def->next) {
		BLI_strncpy(name, def->name, MAX_VGROUP_NAME);
		name += MAX_VGROUP_NAME;
	}

	return name_array;
}

static int vgroup_do_remap(Object *ob, char *name_array, wmOperator *op)
{
	MDeformVert *dvert= NULL;
	bDeformGroup *def;
	int def_tot = BLI_countlist(&ob->defbase);
	int *sort_map_update= MEM_mallocN(MAX_VGROUP_NAME * sizeof(int) * def_tot + 1, "sort vgroups"); /* needs a dummy index at the start*/
	int *sort_map= sort_map_update + 1;
	char *name;
	int i;

	name= name_array;
	for(def= ob->defbase.first, i=0; def; def=def->next, i++){
		sort_map[i]= BLI_findstringindex(&ob->defbase, name, offsetof(bDeformGroup, name));
		name += MAX_VGROUP_NAME;
	}

	if(ob->mode == OB_MODE_EDIT) {
		if(ob->type==OB_MESH) {
			EditMesh *em = BKE_mesh_get_editmesh(ob->data);
			EditVert *eve;

			for(eve=em->verts.first; eve; eve=eve->next){
				dvert= CustomData_em_get(&em->vdata, eve->data, CD_MDEFORMVERT);
				if(dvert && dvert->totweight){
					defvert_remap(dvert, sort_map);
				}
			}
		}
		else {
			BKE_report(op->reports, RPT_ERROR, "Editmode lattice isnt supported yet.");
			MEM_freeN(sort_map_update);
			return OPERATOR_CANCELLED;
		}
	}
	else {
		int dvert_tot=0;

		ED_vgroup_give_array(ob->data, &dvert, &dvert_tot);

		/*create as necassary*/
		while(dvert && dvert_tot--) {
			if(dvert->totweight)
				defvert_remap(dvert, sort_map);
			dvert++;
		}
	}

	/* update users */
	for(i=0; i<def_tot; i++)
		sort_map[i]++;

	sort_map_update[0]= 0;
	vgroup_remap_update_users(ob, sort_map_update);

	ob->actdef= sort_map_update[ob->actdef];
	
	MEM_freeN(sort_map_update);

	return OPERATOR_FINISHED;
}

static int vgroup_sort(void *def_a_ptr, void *def_b_ptr)
{
	bDeformGroup *def_a= (bDeformGroup *)def_a_ptr;
	bDeformGroup *def_b= (bDeformGroup *)def_b_ptr;

	return BLI_natstrcmp(def_a->name, def_b->name);
}

static int vertex_group_sort_exec(bContext *C, wmOperator *op)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;
	char *name_array;
	int ret;

	/*init remapping*/
	name_array = vgroup_init_remap(ob);

	/*sort vgroup names*/
	BLI_sortlist(&ob->defbase, vgroup_sort);

	/*remap vgroup data to map to correct names*/
	ret = vgroup_do_remap(ob, name_array, op);

	if (ret != OPERATOR_CANCELLED) {
		DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
		WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob);
	}

	if (name_array) MEM_freeN(name_array);

	return ret;
}

void OBJECT_OT_vertex_group_sort(wmOperatorType *ot)
{
	ot->name= "Sort Vertex Groups";
	ot->idname= "OBJECT_OT_vertex_group_sort";
	ot->description= "Sorts vertex groups alphabetically";

	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vertex_group_sort_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
}

static int vgroup_move_exec(bContext *C, wmOperator *op)
{
	Object *ob= CTX_data_pointer_get_type(C, "object", &RNA_Object).data;
	bDeformGroup *def;
	char *name_array;
	int dir= RNA_enum_get(op->ptr, "direction"), ret;

	def = BLI_findlink(&ob->defbase, ob->actdef - 1);
	if (!def) {
		return OPERATOR_CANCELLED;
	}

	name_array = vgroup_init_remap(ob);

	if (dir == 1) { /*up*/
		void *prev = def->prev;

		BLI_remlink(&ob->defbase, def);
		BLI_insertlinkbefore(&ob->defbase, prev, def);
	} else { /*down*/
		void *next = def->next;

		BLI_remlink(&ob->defbase, def);
		BLI_insertlinkafter(&ob->defbase, next, def);
	}

	ret = vgroup_do_remap(ob, name_array, op);

	if (name_array) MEM_freeN(name_array);

	if (ret != OPERATOR_CANCELLED) {
		DAG_id_tag_update(&ob->id, OB_RECALC_DATA);
		WM_event_add_notifier(C, NC_GEOM|ND_DATA, ob);
	}

	return ret;
}

void OBJECT_OT_vertex_group_move(wmOperatorType *ot)
{
	static EnumPropertyItem vgroup_slot_move[] = {
		{1, "UP", 0, "Up", ""},
		{-1, "DOWN", 0, "Down", ""},
		{0, NULL, 0, NULL, NULL}
	};

	/* identifiers */
	ot->name= "Move Vertex Group";
	ot->idname= "OBJECT_OT_vertex_group_move";

	/* api callbacks */
	ot->poll= vertex_group_poll;
	ot->exec= vgroup_move_exec;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;

	RNA_def_enum(ot->srna, "direction", vgroup_slot_move, 0, "Direction", "Direction to move, UP or DOWN");
}
