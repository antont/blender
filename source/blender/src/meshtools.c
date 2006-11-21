/**
 * $Id: 
 *
 * ***** BEGIN GPL/BL DUAL LICENSE BLOCK *****
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
 * The Original Code is Copyright (C) 2004 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */

/*

meshtools.c: no editmode, tools operating on meshes

int join_mesh(void);

void fasterdraw(void);
void slowerdraw(void);

void sort_faces(void);

*/

#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MEM_guardedalloc.h"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_object_types.h"
#include "DNA_material_types.h"
#include "DNA_scene_types.h"
#include "DNA_screen_types.h"
#include "DNA_view3d_types.h"
#include "DNA_world_types.h"

#include "BLI_blenlib.h"
#include "BLI_arithb.h"

#include "BKE_depsgraph.h"
#include "BKE_customdata.h"
#include "BKE_global.h"
#include "BKE_library.h"
#include "BKE_main.h"
#include "BKE_mesh.h"
#include "BKE_material.h"
#include "BKE_object.h"
#include "BKE_utildefines.h"

#include "BIF_editmesh.h"
#include "BIF_graphics.h"
#include "BIF_interface.h"
#include "BIF_mywindow.h"
#include "BIF_screen.h"
#include "BIF_space.h"
#include "BIF_toolbox.h"
#include "BIF_editconstraint.h"

#include "BDR_editobject.h" 
#include "BDR_editface.h" 

#include "BLI_editVert.h"

#include "mydevice.h"
#include "blendef.h"

#include "BIF_meshtools.h" /* include ourself for prototypes */

#include "RE_pipeline.h"
#include "RE_shader_ext.h"


/* * ********************** no editmode!!! *********** */

/* join selected meshes into the active mesh, context sensitive
return 0 if no join is made (error) and 1 of the join is done */
int join_mesh(void)
{
	Base *base, *nextb;
	Object *ob;
	Material **matar, *ma;
	Mesh *me;
	MVert *mvert, *mvertmain;
	MEdge *medge = NULL, *medgemain;
	MFace *mface = NULL, *mfacemain;
	float imat[4][4], cmat[4][4];
	int a, b, totcol, totedge=0, totvert=0, totface=0, ok=0, vertofs, map[MAXMAT];
	int	i, j, index, haskey=0, edgeofs, faceofs;
	bDeformGroup *dg, *odg;
	MDeformVert *dvert;
	CustomData vdata, edata, fdata;

	if(G.obedit) return 0;
	
	ob= OBACT;
	if(!ob || ob->type!=OB_MESH) return 0;

#ifdef WITH_VERSE
	/* it isn't allowed to join shared object at verse server
	 * this function will be implemented as soon as possible */
	base= FIRSTBASE;
	while(base) {
		if TESTBASELIB(base) {
			if(base->object->type==OB_MESH) {
				if(base->object->vnode) {
					haskey= 1;
					break;
				}
			}
		}
		base= base->next;
	}
	if(haskey) {
		error("Can't join meshes shared at verse server");
		return 0;
	}
#endif

	/* count */
	base= FIRSTBASE;
	while(base) {
		if TESTBASELIB(base) {
			if(base->object->type==OB_MESH) {
				me= base->object->data;
				totvert+= me->totvert;
				totface+= me->totface;

				if(base->object == ob) ok= 1;

				if(me->key) {
					haskey= 1;
					break;
				}
			}
		}
		base= base->next;
	}
	
	if(haskey) {
		error("Can't join meshes with vertex keys");
		return 0;
	}
	/* that way the active object is always selected */ 
	if(ok==0) return 0;
	
	if(totvert==0 || totvert>MESH_MAX_VERTS) return 0;
	


	/* if needed add edges to other meshes */
	for(base= FIRSTBASE; base; base= base->next) {
		if TESTBASELIB(base) {
			if(base->object->type==OB_MESH) {
				me= base->object->data;
				totedge += me->totedge;
			}
		}
	}
	
	/* new material indices and material array */
	matar= MEM_callocN(sizeof(void *)*MAXMAT, "join_mesh");
	totcol= ob->totcol;
	
	/* obact materials in new main array, is nicer start! */
	for(a=1; a<=ob->totcol; a++) {
		matar[a-1]= give_current_material(ob, a);
		id_us_plus((ID *)matar[a-1]);
		/* increase id->us : will be lowered later */
	}
	
	base= FIRSTBASE;
	while(base) {
		if TESTBASELIB(base) {
			if(ob!=base->object && base->object->type==OB_MESH) {
				me= base->object->data;

				// Join this object's vertex groups to the base one's
				for (dg=base->object->defbase.first; dg; dg=dg->next){
					/* See if this group exists in the object */
					for (odg=ob->defbase.first; odg; odg=odg->next){
						if (!strcmp(odg->name, dg->name)){
							break;
						}
					}
					if (!odg){
						odg = MEM_callocN (sizeof(bDeformGroup), "join deformGroup");
						memcpy (odg, dg, sizeof(bDeformGroup));
						BLI_addtail(&ob->defbase, odg);
					}

				}
				if (ob->defbase.first && ob->actdef==0)
					ob->actdef=1;

				if(me->totvert) {
					for(a=1; a<=base->object->totcol; a++) {
						ma= give_current_material(base->object, a);
						if(ma) {
							for(b=0; b<totcol; b++) {
								if(ma == matar[b]) break;
							}
							if(b==totcol) {
								matar[b]= ma;
								ma->id.us++;
								totcol++;
							}
							if(totcol>=MAXMAT-1) break;
						}
					}
				}
			}
			if(totcol>=MAXMAT-1) break;
		}
		base= base->next;
	}

	me= ob->data;

	memset(&vdata, 0, sizeof(vdata));
	memset(&edata, 0, sizeof(edata));
	memset(&fdata, 0, sizeof(fdata));
	
	mvertmain= mvert= CustomData_add_layer(&vdata, CD_MVERT, 0, NULL, totvert);
	medgemain= medge= CustomData_add_layer(&edata, CD_MEDGE, 0, NULL, totedge);
	mfacemain= mface= CustomData_add_layer(&fdata, CD_MFACE, 0, NULL, totface);


	/* inverse transorm all selected meshes in this object */
	Mat4Invert(imat, ob->obmat);
	
	vertofs= 0;
	edgeofs= 0;
	faceofs= 0;
	base= FIRSTBASE;
	while(base) {
		nextb= base->next;
		if TESTBASELIB(base) {
			if(base->object->type==OB_MESH) {
				
				me= base->object->data;
				
				if(me->totvert) {
					CustomData_merge(&me->vdata, &vdata, CD_MASK_MESH, CD_DEFAULT, totvert);
					CustomData_copy_data(&me->vdata, &vdata, 0, vertofs, me->totvert);
					
					dvert= CustomData_get(&vdata, vertofs, CD_MDEFORMVERT);

					/* NEW VERSION */
					if (dvert){
						for (i=0; i<me->totvert; i++){
							for (j=0; j<dvert[i].totweight; j++){
								//	Find the old vertex group
								odg = BLI_findlink (&base->object->defbase, dvert[i].dw[j].def_nr);
								if(odg) {
									//	Search for a match in the new object
									for (dg=ob->defbase.first, index=0; dg; dg=dg->next, index++){
										if (!strcmp(dg->name, odg->name)){
											dvert[i].dw[j].def_nr = index;
											break;
										}
									}
								}
							}
						}
					}

					if(base->object != ob) {
						/* watch this: switch matmul order really goes wrong */
						Mat4MulMat4(cmat, base->object->obmat, imat);
						
						a= me->totvert;
						while(a--) {
							Mat4MulVecfl(cmat, mvert->co);
							mvert++;
						}
					}
					else mvert+= me->totvert;
				}
				if(me->totface) {
				
					/* make mapping for materials */
					memset(map, 0, 4*MAXMAT);
					for(a=1; a<=base->object->totcol; a++) {
						ma= give_current_material(base->object, a);
						if(ma) {
							for(b=0; b<totcol; b++) {
								if(ma == matar[b]) {
									map[a-1]= b;
									break;
								}
							}
						}
					}

					CustomData_merge(&me->fdata, &fdata, CD_MASK_MESH, CD_DEFAULT, totface);
					CustomData_copy_data(&me->fdata, &fdata, 0, faceofs, me->totface);

					for(a=0; a<me->totface; a++, mface++) {
						mface->v1+= vertofs;
						mface->v2+= vertofs;
						mface->v3+= vertofs;
						if(mface->v4) mface->v4+= vertofs;
						
						mface->mat_nr= map[(int)mface->mat_nr];
					}

					faceofs += me->totface;
				}
				
				if(me->totedge) {
					CustomData_merge(&me->edata, &edata, CD_MASK_MESH, CD_DEFAULT, totedge);
					CustomData_copy_data(&me->edata, &edata, 0, edgeofs, me->totedge);

					for(a=0; a<me->totedge; a++, medge++) {
						medge->v1+= vertofs;
						medge->v2+= vertofs;
					}

					edgeofs += me->totedge;
				}
				
				vertofs += me->totvert;
				
				if(base->object!=ob)
					free_and_unlink_base(base);
			}
		}
		base= nextb;
	}
	
	me= ob->data;
	
	CustomData_free(&me->vdata, me->totvert);
	CustomData_free(&me->edata, me->totedge);
	CustomData_free(&me->fdata, me->totface);

	me->totvert= totvert;
	me->totedge= totedge;
	me->totface= totface;
	
	me->vdata= vdata;
	me->edata= edata;
	me->fdata= fdata;

	mesh_update_customdata_pointers(me);
	
	/* old material array */
	for(a=1; a<=ob->totcol; a++) {
		ma= ob->mat[a-1];
		if(ma) ma->id.us--;
	}
	for(a=1; a<=me->totcol; a++) {
		ma= me->mat[a-1];
		if(ma) ma->id.us--;
	}
	if(ob->mat) MEM_freeN(ob->mat);
	if(me->mat) MEM_freeN(me->mat);
	ob->mat= me->mat= 0;
	
	if(totcol) {
		me->mat= matar;
		ob->mat= MEM_callocN(sizeof(void *)*totcol, "join obmatar");
	}
	else MEM_freeN(matar);
	
	ob->totcol= me->totcol= totcol;
	ob->colbits= 0;
	
	/* other mesh users */
	test_object_materials((ID *)me);
	
	DAG_scene_sort(G.scene);	// removed objects, need to rebuild dag before editmode call
	
	enter_editmode(EM_WAITCURSOR);
	exit_editmode(EM_FREEDATA|EM_WAITCURSOR);	// freedata, but no undo
	
	allqueue(REDRAWVIEW3D, 0);
	allqueue(REDRAWBUTSSHADING, 0);

	BIF_undo_push("Join Mesh");
	return 1;
}


void fasterdraw(void)
{
	Base *base;
	Mesh *me;
	int toggle, a;

	if(G.obedit || G.vd==NULL) return;

	/* reset flags */
	me= G.main->mesh.first;
	while(me) {
		me->flag &= ~ME_ISDONE;
		me= me->id.next;
	}

	base= FIRSTBASE;
	while(base) {
		if( TESTBASELIB(base) && (base->object->type==OB_MESH)) {
			me= base->object->data;
			if(me->id.lib==0 && (me->flag & ME_ISDONE)==0) {
				me->flag |= ME_ISDONE;
				toggle= 0;
				for(a=0; a<me->totedge; a++) {
					MEdge *med = &me->medge[a];

					if( (med->flag & ME_EDGEDRAW) && !( (toggle++) & 3) ) {
						med->flag ^= ME_EDGEDRAW;
					}
				}
			}
		}
		base= base->next;
	}

	/* important?: reset flags again */
	me= G.main->mesh.first;
	while(me) {
		me->flag &= ~ME_ISDONE;
		me= me->id.next;
	}

	allqueue(REDRAWVIEW3D, 0);
}

void slowerdraw(void)		/* reset fasterdraw */
{
	Base *base;
	Mesh *me;
	int a;

	if(G.obedit || G.vd==NULL) return;

	base= FIRSTBASE;
	while(base) {
		if( TESTBASELIB(base) && (base->object->type==OB_MESH)) {
			me= base->object->data;
			if(me->id.lib==0) {
				for(a=0; a<me->totedge; a++) {
					me->medge[a].flag |= ME_EDGEDRAW;
				}
			}
		}
		base= base->next;
	}

	allqueue(REDRAWVIEW3D, 0);
}

/* ********************** SORT FACES ******************* */

static void permutate(void *list, int num, int size, int *index)
{
	void *buf;
	int len;
	int i;

	len = num * size;

	buf = MEM_mallocN(len, "permutate");
	memcpy(buf, list, len);
	
	for (i = 0; i < num; i++) {
		memcpy((char *)list + (i * size), (char *)buf + (index[i] * size), size);
	}
	MEM_freeN(buf);
}

static MVert *mvertbase;
static MFace *mfacebase;

static int verg_mface(const void *v1, const void *v2)
{
	MFace *x1, *x2;

	MVert *ve1, *ve2;
	int i1, i2;

	i1 = ((int *) v1)[0];
	i2 = ((int *) v2)[0];
	
	x1 = mfacebase + i1;
	x2 = mfacebase + i2;

	ve1= mvertbase+x1->v1;
	ve2= mvertbase+x2->v1;
	
	if( ve1->co[2] > ve2->co[2] ) return 1;
	else if( ve1->co[2] < ve2->co[2]) return -1;
	return 0;
}


void sort_faces(void)
{
	Object *ob= OBACT;
	Mesh *me;
	CustomDataLayer *layer;
	int i, *index;
	
	if(ob==0) return;
	if(G.obedit) return;
	if(ob->type!=OB_MESH) return;
	
	if(okee("Sort faces in Z axis")==0) return;
	me= ob->data;
	if(me->totface==0) return;

/*	create index list */
	index = (int *) MEM_mallocN(sizeof(int) * me->totface, "sort faces");
	for (i = 0; i < me->totface; i++) {
		index[i] = i;
	}
	mvertbase= me->mvert;
	mfacebase = me->mface;

/* sort index list instead of faces itself 
   and apply this permutation to all face layers */
	qsort(index, me->totface, sizeof(int), verg_mface);

	for(i = 0; i < me->fdata.totlayer; i++) {
		layer = &me->fdata.layers[i];
		permutate(layer->data, me->totface, CustomData_sizeof(layer->type), index);
	}

	MEM_freeN(index);

	allqueue(REDRAWVIEW3D, 0);
	DAG_object_flush_update(G.scene, ob, OB_RECALC_DATA);
}

/* ********************* MESH VERTEX OCTREE LOOKUP ************* */

/* important note; this is unfinished, needs better API for editmode, and custom threshold */

#define MOC_RES			8
#define MOC_NODE_RES	8
#define MOC_THRESH		0.0002f

typedef struct MocNode {
	struct MocNode *next;
	long index[MOC_NODE_RES];
} MocNode;

static int mesh_octree_get_base_offs(float *co, float *offs, float *div)
{
	int vx, vy, vz;
	
	vx= floor( (co[0]-offs[0])/div[0] );
	vy= floor( (co[1]-offs[1])/div[1] );
	vz= floor( (co[2]-offs[2])/div[2] );
	
	CLAMP(vx, 0, MOC_RES-1);
	CLAMP(vy, 0, MOC_RES-1);
	CLAMP(vz, 0, MOC_RES-1);

	return (vx*MOC_RES*MOC_RES) + vy*MOC_RES + vz;
}

static void mesh_octree_add_node(MocNode **bt, long index)
{
	if(*bt==NULL) {
		*bt= MEM_callocN(sizeof(MocNode), "MocNode");
		(*bt)->index[0]= index;
	}
	else {
		int a;
		for(a=0; a<MOC_NODE_RES; a++) {
			if((*bt)->index[a]==index)
				return;
			else if((*bt)->index[a]==0) {
				(*bt)->index[a]= index;
				return;
			}
		}
		mesh_octree_add_node(&(*bt)->next, index);
	}
}

static void mesh_octree_free_node(MocNode **bt)
{
	if( (*bt)->next ) {
		mesh_octree_free_node(&(*bt)->next);
	}
	MEM_freeN(*bt);
}


/* temporal define, just to make nicer code below */
#define MOC_ADDNODE(vx, vy, vz)	mesh_octree_add_node(basetable + ((vx)*MOC_RES*MOC_RES) + (vy)*MOC_RES + (vz), index)

static void mesh_octree_add_nodes(MocNode **basetable, float *co, float *offs, float *div, long index)
{
	float fx, fy, fz;
	int vx, vy, vz;
	
	fx= (co[0]-offs[0])/div[0];
	fy= (co[1]-offs[1])/div[1];
	fz= (co[2]-offs[2])/div[2];
	CLAMP(fx, 0.0f, MOC_RES-MOC_THRESH);
	CLAMP(fy, 0.0f, MOC_RES-MOC_THRESH);
	CLAMP(fz, 0.0f, MOC_RES-MOC_THRESH);
	
	vx= floor(fx);
	vy= floor(fy);
	vz= floor(fz);
	
	MOC_ADDNODE(vx, vy, vz);
	
	if( vx>0 )
		if( fx-((float)vx)-MOC_THRESH < 0.0f)
			MOC_ADDNODE(vx-1, vy, vz);
	if( vx<MOC_RES-2 )
		if( fx-((float)vx)+MOC_THRESH > 1.0f)
			MOC_ADDNODE(vx+1, vy, vz);

	if( vy>0 )
		if( fy-((float)vy)-MOC_THRESH < 0.0f) 
			MOC_ADDNODE(vx, vy-1, vz);
	if( vy<MOC_RES-2 )
		if( fy-((float)vy)+MOC_THRESH > 1.0f) 
			MOC_ADDNODE(vx, vy+1, vz);

	if( vz>0 )
		if( fz-((float)vz)-MOC_THRESH < 0.0f) 
			MOC_ADDNODE(vx, vy, vz-1);
	if( vz<MOC_RES-2 )
		if( fz-((float)vz)+MOC_THRESH > 1.0f) 
			MOC_ADDNODE(vx, vy, vz+1);
	
}

static long mesh_octree_find_index(MocNode **bt, MVert *mvert, float *co)
{
	float *vec;
	int a;
	
	if(*bt==NULL)
		return -1;
	
	for(a=0; a<MOC_NODE_RES; a++) {
		if((*bt)->index[a]) {
			/* does mesh verts and editmode, code looks potential dangerous, octree should really be filled OK! */
			if(mvert) {
				vec= (mvert+(*bt)->index[a]-1)->co;
				if(FloatCompare(vec, co, MOC_THRESH))
					return (*bt)->index[a]-1;
			}
			else {
				EditVert *eve= (EditVert *)((*bt)->index[a]);
				if(FloatCompare(eve->co, co, MOC_THRESH))
					return (*bt)->index[a];
			}
		}
		else return -1;
	}
	if( (*bt)->next)
		return mesh_octree_find_index(&(*bt)->next, mvert, co);
	
	return -1;
}


/* mode is 's' start, or 'e' end, or 'u' use */
/* if end, ob can be NULL */
long mesh_octree_table(Object *ob, float *co, char mode)
{
	MocNode **bt;
	static MocNode **basetable= NULL;
	static float offs[3], div[3];
	
	if(mode=='u') {		/* use table */
		if(basetable==NULL)
			mesh_octree_table(ob, NULL, 's');
	   
		if(basetable) {
			Mesh *me= ob->data;
			bt= basetable + mesh_octree_get_base_offs(co, offs, div);
			if(ob==G.obedit)
				return mesh_octree_find_index(bt, NULL, co);
			else
				return mesh_octree_find_index(bt, me->mvert, co);
		}
		return -1;
	}
	else if(mode=='s') {	/* start table */
		Mesh *me= ob->data;
		BoundBox *bb = mesh_get_bb(me);
		
		/* for quick unit coordinate calculus */
		VECCOPY(offs, bb->vec[0]);
		offs[0]-= MOC_THRESH;		/* we offset it 1 threshold unit extra */
		offs[1]-= MOC_THRESH;
		offs[2]-= MOC_THRESH;
			
		VecSubf(div, bb->vec[6], bb->vec[0]);
		div[0]+= 2*MOC_THRESH;		/* and divide with 2 threshold unit more extra (try 8x8 unit grid on paint) */
		div[1]+= 2*MOC_THRESH;
		div[2]+= 2*MOC_THRESH;
		
		VecMulf(div, 1.0f/MOC_RES);
		if(div[0]==0.0f) div[0]= 1.0f;
		if(div[1]==0.0f) div[1]= 1.0f;
		if(div[2]==0.0f) div[2]= 1.0f;
			
		if(basetable) /* happens when entering this call without ending it */
			mesh_octree_table(ob, co, 'e');
		
		basetable= MEM_callocN(MOC_RES*MOC_RES*MOC_RES*sizeof(void *), "sym table");
		
		if(ob==G.obedit) {
			EditVert *eve;
			
			for(eve= G.editMesh->verts.first; eve; eve= eve->next) {
				mesh_octree_add_nodes(basetable, eve->co, offs, div, (long)(eve));
			}
		}
		else {		
			MVert *mvert;
			long a;
			
			for(a=1, mvert= me->mvert; a<=me->totvert; a++, mvert++) {
				mesh_octree_add_nodes(basetable, mvert->co, offs, div, a);
			}
		}
	}
	else if(mode=='e') { /* end table */
		if(basetable) {
			int a;
			
			for(a=0, bt=basetable; a<MOC_RES*MOC_RES*MOC_RES; a++, bt++) {
				if(*bt) mesh_octree_free_node(bt);
			}
			MEM_freeN(basetable);
			basetable= NULL;
		}
	}
	return 0;
}

int mesh_get_x_mirror_vert(Object *ob, int index)
{
	Mesh *me= ob->data;
	MVert *mvert= me->mvert+index;
	float vec[3];
	
	vec[0]= -mvert->co[0];
	vec[1]= mvert->co[1];
	vec[2]= mvert->co[2];
	
	return mesh_octree_table(ob, vec, 'u');
}

EditVert *editmesh_get_x_mirror_vert(Object *ob, float *co)
{
	float vec[3];
	long poinval;
	
	vec[0]= -co[0];
	vec[1]= co[1];
	vec[2]= co[2];
	
	poinval= mesh_octree_table(ob, vec, 'u');
	if(poinval != -1)
		return (EditVert *)(poinval);
	return NULL;
}


/* ****************** render BAKING ********************** */

/* all selected meshes with UV maps are rendered for current scene visibility */
void objects_bake_render(void)
{
	short event;
	
	event= pupmenu("Bake Selected Meshes %t|Full Render %x1|Ambient Occlusion %x2|Normals %x3|Texture Only %x4");
	if(event>0) {
		Render *re= RE_NewRender("_Bake View_");
		
		if(event==1) event= RE_BAKE_ALL;
		else if(event==2) event= RE_BAKE_AO;
		else if(event==3) event= RE_BAKE_NORMALS;
		else event= RE_BAKE_TEXTURE;
		
		if(event==RE_BAKE_AO) {
			if((G.scene->r.mode & R_RAYTRACE)==0 || G.scene->world==NULL
			   || (G.scene->world->mode & WO_AMB_OCC)==0) {
				error("No ray-trace or AO set up");
				return;
			}
		}
		
		waitcursor(1);
		RE_timecursor_cb(re, set_timecursor);
		RE_test_break_cb(re, blender_test_break);
		G.afbreek= 0;	/* blender_test_break uses this global */
		
		RE_Database_Baking(re, G.scene, event);
		
		RE_bake_shade_all_selected(re, event);
		
		RE_Database_Free(re);
		waitcursor(0);
		
		allqueue(REDRAWIMAGE, 0);
		allqueue(REDRAWVIEW3D, 0);
	}
}


