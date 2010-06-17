/*
 * $Id: sculpt.c 29425 2010-06-12 15:05:19Z jwilkins $
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
 * The Original Code is Copyright (C) 2006 by Nicholas Bishop
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 * Implements the Sculpt Mode tools
 *
 */

#include "MEM_guardedalloc.h"

#include "BLI_math.h"
#include "BLI_blenlib.h"
#include "BLI_dynstr.h"
#include "BLI_ghash.h"
#include "BLI_pbvh.h"
#include "BLI_threads.h"
#include "BLI_editVert.h"

#include "DNA_key_types.h"
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"

#include "BKE_brush.h"
#include "BKE_cdderivedmesh.h"
#include "BKE_context.h"
#include "BKE_customdata.h"
#include "BKE_DerivedMesh.h"
#include "BKE_depsgraph.h"
#include "BKE_global.h"
#include "BKE_image.h"
#include "BKE_key.h"
#include "BKE_library.h"
#include "BKE_main.h"
#include "BKE_mesh.h"
#include "BKE_modifier.h"
#include "BKE_multires.h"
#include "BKE_paint.h"
#include "BKE_report.h"
#include "BKE_texture.h"
#include "BKE_utildefines.h"
#include "BKE_colortools.h"

#include "BIF_gl.h"
#include "BIF_glutil.h"

#include "WM_api.h"
#include "WM_types.h"
#include "ED_screen.h"
#include "ED_sculpt.h"
#include "ED_view3d.h"
#include "ED_mesh.h"
#include "paint_intern.h"
#include "sculpt_intern.h"

#include "RNA_access.h"
#include "RNA_define.h"


#include "RE_render_ext.h"
#include "RE_shader_ext.h"

#include "gpu_buffers.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

//#ifdef _OPENMP
//#include <omp.h>
//#endif
/************************** Undo *************************/



/* Sculpt mode handles multires differently from regular meshes, but only if
   it's the last modifier on the stack and it is not on the first level */
struct MultiresModifierData *sculpt_multires_active(Scene *scene, Object *ob)
{
	ModifierData *md, *nmd;
	
	for(md= modifiers_getVirtualModifierList(ob); md; md= md->next) {
		if(md->type == eModifierType_Multires) {
			MultiresModifierData *mmd= (MultiresModifierData*)md;

			/* Check if any of the modifiers after multires are active
			 * if not it can use the multires struct */
			for(nmd= md->next; nmd; nmd= nmd->next)
				if(modifier_isEnabled(scene, nmd, eModifierMode_Realtime))
					break;

			if(!nmd && mmd->sculptlvl > 0)
				return mmd;
		}
	}

	return NULL;
}

static void update_cb(PBVHNode *node, void *unused)
{
	(void)unused;
	BLI_pbvh_node_mark_update(node);
}

/* Checks whether full update mode (slower) needs to be used to work with modifiers */
int sculpt_modifiers_active(Scene *scene, Object *ob)
{
	ModifierData *md;
	MultiresModifierData *mmd= sculpt_multires_active(scene, ob);

	/* check if there are any modifiers after what we are sculpting,
	   for a multires modifier with a deform modifier in front, we
	   do no need to recalculate the modifier stack. note that this
	   needs to be in sync with ccgDM_use_grid_pbvh! */
	if(mmd)
		md= mmd->modifier.next;
	else
		md= modifiers_getVirtualModifierList(ob);
	
	/* exception for shape keys because we can edit those */
	for(; md; md= md->next) {
		if(modifier_isEnabled(scene, md, eModifierMode_Realtime))
			if(md->type != eModifierType_ShapeKey)
				return 1;
	}

	return 0;
}

static void sculpt_undo_restore(bContext *C, ListBase *lb)
{
	Scene *scene = CTX_data_scene(C);
	Object *ob = CTX_data_active_object(C);
	DerivedMesh *dm = mesh_get_derived_final(scene, ob, 0);
	SculptSession *ss = ob->sculpt;
	SculptUndoNode *unode;
	MVert *mvert;
	MultiresModifierData *mmd;
	int *index;
	int i, j, update= 0;

	sculpt_update_mesh_elements(scene, ob, 0);

	for(unode=lb->first; unode; unode=unode->next) {
		if(!(strcmp(unode->idname, ob->id.name)==0))
			continue;

		if(unode->maxvert) {
			/* regular mesh restore */
			if(ss->totvert != unode->maxvert)
				continue;

			index= unode->index;
			mvert= ss->mvert;

			for(i=0; i<unode->totvert; i++) {
				swap_v3_v3(mvert[index[i]].co, unode->co[i]);
				mvert[index[i]].flag |= ME_VERT_PBVH_UPDATE;
			}
		}
		else if(unode->maxgrid && dm->getGridData) {
			/* multires restore */
			DMGridData **grids, *grid;
			float (*co)[3];
			int gridsize;

			if(dm->getNumGrids(dm) != unode->maxgrid)
				continue;
			if(dm->getGridSize(dm) != unode->gridsize)
				continue;

			grids= dm->getGridData(dm);
			gridsize= dm->getGridSize(dm);

			co = unode->co;
			for(j=0; j<unode->totgrid; j++) {
				grid= grids[unode->grids[j]];

				for(i=0; i<gridsize*gridsize; i++, co++)
					swap_v3_v3(grid[i].co, co[0]);
			}
		}

		update= 1;
	}

	if(update) {
		if(ss->kb) sculpt_mesh_to_key(ss->ob, ss->kb);
		if(ss->refkb) sculpt_key_to_mesh(ss->refkb, ob);

		/* we update all nodes still, should be more clever, but also
		   needs to work correct when exiting/entering sculpt mode and
		   the nodes get recreated, though in that case it could do all */
		BLI_pbvh_search_callback(ss->pbvh, NULL, NULL, update_cb, NULL);
		BLI_pbvh_update(ss->pbvh, PBVH_UpdateBB|PBVH_UpdateOriginalBB|PBVH_UpdateRedraw, NULL);

		if((mmd= sculpt_multires_active(scene, ob)))
			multires_mark_as_modified(ob);

		if(sculpt_modifiers_active(scene, ob))
			DAG_id_flush_update(&ob->id, OB_RECALC_DATA);
	}
}

static void sculpt_undo_free(ListBase *lb)
{
	SculptUndoNode *unode;

	for(unode=lb->first; unode; unode=unode->next) {
		if(unode->co)
			MEM_freeN(unode->co);
		if(unode->no)
			MEM_freeN(unode->no);
		if(unode->index)
			MEM_freeN(unode->index);
		if(unode->grids)
			MEM_freeN(unode->grids);
		if(unode->layer_disp)
			MEM_freeN(unode->layer_disp);
	}
}

SculptUndoNode *sculpt_undo_get_node(PBVHNode *node)
{
	ListBase *lb= undo_paint_push_get_list(UNDO_PAINT_MESH);
	SculptUndoNode *unode;

	if(!lb)
		return NULL;

	for(unode=lb->first; unode; unode=unode->next)
		if(unode->node == node)
			return unode;

	return NULL;
}

SculptUndoNode *sculpt_undo_push_node(SculptSession *ss, PBVHNode *node)
{
	ListBase *lb= undo_paint_push_get_list(UNDO_PAINT_MESH);
	Object *ob= ss->ob;
	SculptUndoNode *unode;
	int totvert, allvert, totgrid, maxgrid, gridsize, *grids;

	/* list is manipulated by multiple threads, so we lock */
	BLI_lock_thread(LOCK_CUSTOM1);

	if((unode= sculpt_undo_get_node(node))) {
		BLI_unlock_thread(LOCK_CUSTOM1);
		return unode;
	}

	unode= MEM_mallocN(sizeof(SculptUndoNode), "SculptUndoNode");
	strcpy(unode->idname, ob->id.name);
	unode->node= node;

	BLI_pbvh_node_num_verts(ss->pbvh, node, &totvert, &allvert);
	BLI_pbvh_node_get_grids(ss->pbvh, node, &grids, &totgrid,
		&maxgrid, &gridsize, NULL, NULL);

	unode->totvert= totvert;
	/* we will use this while sculpting, is mapalloc slow to access then? */
	unode->co= MEM_mapallocN(sizeof(float)*3*allvert, "SculptUndoNode.co");
	unode->no= MEM_mapallocN(sizeof(short)*3*allvert, "SculptUndoNode.no");
	undo_paint_push_count_alloc(UNDO_PAINT_MESH, (sizeof(float)*3 + sizeof(short)*3 + sizeof(int))*allvert);
	BLI_addtail(lb, unode);

	if(maxgrid) {
		/* multires */
		unode->maxgrid= maxgrid;
		unode->totgrid= totgrid;
		unode->gridsize= gridsize;
		unode->grids= MEM_mapallocN(sizeof(int)*totgrid, "SculptUndoNode.grids");

		unode->maxvert = 0;
		unode->index   = 0;
	}
	else {
		/* regular mesh */
		unode->maxvert= ss->totvert;
		unode->index= MEM_mapallocN(sizeof(int)*allvert, "SculptUndoNode.index");

		unode->maxgrid=  0;
		unode->totgrid=  0;
		unode->gridsize= 0;
		unode->grids=    0;
	}

	BLI_unlock_thread(LOCK_CUSTOM1);

	/* copy threaded, hopefully this is the performance critical part */
	{
		PBVHVertexIter vd;

		BLI_pbvh_vertex_iter_begin(ss->pbvh, node, vd, PBVH_ITER_ALL) {
			copy_v3_v3(unode->co[vd.i], vd.co);
			if(vd.no) VECCOPY(unode->no[vd.i], vd.no)
			else normal_float_to_short_v3(unode->no[vd.i], vd.fno);
			if(vd.vert_indices) unode->index[vd.i]= vd.vert_indices[vd.i];
		}
		BLI_pbvh_vertex_iter_end;
	}

	if(unode->grids) memcpy(unode->grids, grids, sizeof(int)*totgrid);

	unode->layer_disp= 0;

	return unode;
}

void sculpt_undo_push_begin(char *name)
{
	undo_paint_push_begin(UNDO_PAINT_MESH, name,
		sculpt_undo_restore, sculpt_undo_free);
}

void sculpt_undo_push_end(void)
{
	ListBase *lb= undo_paint_push_get_list(UNDO_PAINT_MESH);
	SculptUndoNode *unode;

	/* we don't need normals in the undo stack */
	for(unode=lb->first; unode; unode=unode->next) {
		if(unode->no) {
			MEM_freeN(unode->no);
			unode->no= NULL;
		}

		if(unode->layer_disp) {
			MEM_freeN(unode->layer_disp);
			unode->layer_disp= NULL;
		}
	}

	undo_paint_push_end(UNDO_PAINT_MESH);
}

void ED_sculpt_force_update(bContext *C)
{
	Object *ob= CTX_data_active_object(C);

	if(ob && (ob->mode & OB_MODE_SCULPT))
		multires_force_update(ob);
}
