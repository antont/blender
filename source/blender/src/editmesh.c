/**
 * $Id$
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
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */


#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MEM_guardedalloc.h"

#include "PIL_time.h"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_object_types.h"
#include "DNA_object_force.h"
#include "DNA_screen_types.h"
#include "DNA_key_types.h"
#include "DNA_scene_types.h"
#include "DNA_view3d_types.h"
#include "DNA_material_types.h"
#include "DNA_modifier_types.h"
#include "DNA_texture_types.h"
#include "DNA_userdef_types.h"

#include "BLI_blenlib.h"
#include "BLI_arithb.h"
#include "BLI_editVert.h"
#include "BLI_dynstr.h"
#include "BLI_rand.h"

#include "BKE_DerivedMesh.h"
#include "BKE_depsgraph.h"
#include "BKE_global.h"
#include "BKE_key.h"
#include "BKE_library.h"
#include "BKE_main.h"
#include "BKE_material.h"
#include "BKE_mesh.h"
#include "BKE_object.h"
#include "BKE_texture.h"
#include "BKE_utildefines.h"

#include "BIF_editkey.h"
#include "BIF_editmesh.h"
#include "BIF_editmode_undo.h"
#include "BIF_interface.h"
#include "BIF_meshtools.h"
#include "BIF_mywindow.h"
#include "BIF_space.h"
#include "BIF_screen.h"
#include "BIF_toolbox.h"

#include "BSE_view.h"
#include "BSE_edit.h"
#include "BSE_trans_types.h"

#include "BDR_drawobject.h"
#include "BDR_editobject.h"
#include "BDR_editface.h"
#include "BDR_vpaint.h"

#include "mydevice.h"
#include "blendef.h"

/* own include */
#include "editmesh.h"

/* 

editmesh.c:
- add/alloc/free data
- hashtables
- enter/exit editmode

*/


/* ***************** HASH ********************* */


#define EDHASHSIZE		(512*512)
#define EDHASH(a, b)	(a % EDHASHSIZE)


/* ************ ADD / REMOVE / FIND ****************** */

/* used to bypass normal calloc with fast one */
static void *(*callocvert)(size_t, size_t) = calloc;
static void *(*callocedge)(size_t, size_t) = calloc;
static void *(*callocface)(size_t, size_t) = calloc;

EditVert *addvertlist(float *vec)
{
	EditMesh *em = G.editMesh;
	EditVert *eve;
	static int hashnr= 0;

	eve= callocvert(sizeof(EditVert), 1);
	BLI_addtail(&em->verts, eve);
	
	if(vec) VECCOPY(eve->co, vec);

	eve->hash= hashnr++;
	if( hashnr>=EDHASHSIZE) hashnr= 0;

	/* new verts get keyindex of -1 since they did not
	 * have a pre-editmode vertex order
	 */
	eve->keyindex = -1;
	return eve;
}

void free_editvert (EditVert *eve)
{
	if(eve->dw) MEM_freeN(eve->dw);
	if(eve->fast==0) free(eve);
}


EditEdge *findedgelist(EditVert *v1, EditVert *v2)
{
	EditVert *v3;
	struct HashEdge *he;

	/* swap ? */
	if( v1 > v2) {
		v3= v2; 
		v2= v1; 
		v1= v3;
	}
	
	if(G.editMesh->hashedgetab==NULL)
		G.editMesh->hashedgetab= MEM_callocN(EDHASHSIZE*sizeof(struct HashEdge), "hashedgetab");

	he= G.editMesh->hashedgetab + EDHASH(v1->hash, v2->hash);
	
	while(he) {
		
		if(he->eed && he->eed->v1==v1 && he->eed->v2==v2) return he->eed;
		
		he= he->next;
	}
	return 0;
}

static void insert_hashedge(EditEdge *eed)
{
	/* assuming that eed is not in the list yet, and that a find has been done before */
	
	struct HashEdge *first, *he;

	first= G.editMesh->hashedgetab + EDHASH(eed->v1->hash, eed->v2->hash);

	if( first->eed==0 ) {
		first->eed= eed;
	}
	else {
		he= &eed->hash; 
		he->eed= eed;
		he->next= first->next;
		first->next= he;
	}
}

static void remove_hashedge(EditEdge *eed)
{
	/* assuming eed is in the list */
	
	struct HashEdge *first, *he, *prev=NULL;

	he=first= G.editMesh->hashedgetab + EDHASH(eed->v1->hash, eed->v2->hash);

	while(he) {
		if(he->eed == eed) {
			/* remove from list */
			if(he==first) {
				if(first->next) {
					he= first->next;
					first->eed= he->eed;
					first->next= he->next;
				}
				else he->eed= 0;
			}
			else {
				prev->next= he->next;
			}
			return;
		}
		prev= he;
		he= he->next;
	}
}

EditEdge *addedgelist(EditVert *v1, EditVert *v2, EditEdge *example)
{
	EditMesh *em = G.editMesh;
	EditVert *v3;
	EditEdge *eed;
	int swap= 0;
	
	if(v1==v2) return NULL;
	if(v1==NULL || v2==NULL) return NULL;

	/* swap ? */
	if(v1>v2) {
		v3= v2; 
		v2= v1; 
		v1= v3;
		swap= 1;
	}
	
	/* find in hashlist */
	eed= findedgelist(v1, v2);

	if(eed==NULL) {
	
		eed= (EditEdge *)callocedge(sizeof(EditEdge), 1);
		eed->v1= v1;
		eed->v2= v2;
		BLI_addtail(&em->edges, eed);
		eed->dir= swap;
		insert_hashedge(eed);
		
		/* copy edge data:
		   rule is to do this with addedgelist call, before addfacelist */
		if(example) {
			eed->crease= example->crease;
			eed->seam = example->seam;
			eed->h |= (example->h & EM_FGON);
		}
	}

	return eed;
}

void remedge(EditEdge *eed)
{
	EditMesh *em = G.editMesh;

	BLI_remlink(&em->edges, eed);
	remove_hashedge(eed);
}

void free_editedge(EditEdge *eed)
{
	if(eed->fast==0) free(eed);
}

void free_editface(EditFace *efa)
{
	if(efa->fast==0) free(efa);
}

void free_vertlist(ListBase *edve) 
{
	EditVert *eve, *next;

	if (!edve) return;

	eve= edve->first;
	while(eve) {
		next= eve->next;
		free_editvert(eve);
		eve= next;
	}
	edve->first= edve->last= NULL;
}

void free_edgelist(ListBase *lb)
{
	EditEdge *eed, *next;
	
	eed= lb->first;
	while(eed) {
		next= eed->next;
		free_editedge(eed);
		eed= next;
	}
	lb->first= lb->last= NULL;
}

void free_facelist(ListBase *lb)
{
	EditFace *efa, *next;
	
	efa= lb->first;
	while(efa) {
		next= efa->next;
		free_editface(efa);
		efa= next;
	}
	lb->first= lb->last= NULL;
}

EditFace *addfacelist(EditVert *v1, EditVert *v2, EditVert *v3, EditVert *v4, EditFace *example, EditFace *exampleEdges)
{
	EditMesh *em = G.editMesh;
	EditFace *efa;
	EditEdge *e1, *e2=0, *e3=0, *e4=0;

	/* add face to list and do the edges */
	if(exampleEdges) {
		e1= addedgelist(v1, v2, exampleEdges->e1);
		e2= addedgelist(v2, v3, exampleEdges->e2);
		if(v4) e3= addedgelist(v3, v4, exampleEdges->e3); 
		else e3= addedgelist(v3, v1, exampleEdges->e3);
		if(v4) e4= addedgelist(v4, v1, exampleEdges->e4);
	}
	else {
		e1= addedgelist(v1, v2, NULL);
		e2= addedgelist(v2, v3, NULL);
		if(v4) e3= addedgelist(v3, v4, NULL); 
		else e3= addedgelist(v3, v1, NULL);
		if(v4) e4= addedgelist(v4, v1, NULL);
	}
	
	if(v1==v2 || v2==v3 || v1==v3) return NULL;
	if(e2==0) return NULL;

	efa= (EditFace *)callocface(sizeof(EditFace), 1);
	efa->v1= v1;
	efa->v2= v2;
	efa->v3= v3;
	efa->v4= v4;

	efa->e1= e1;
	efa->e2= e2;
	efa->e3= e3;
	efa->e4= e4;

	if(example) {
		efa->mat_nr= example->mat_nr;
		efa->tf= example->tf;
		efa->tf.flag &= ~TF_ACTIVE;
		efa->flag= example->flag;
	}
	else {
		if (G.obedit && G.obedit->actcol)
			efa->mat_nr= G.obedit->actcol-1;
		default_uv(efa->tf.uv, 1.0);

		/* Initialize colors */
		efa->tf.col[0]= efa->tf.col[1]= efa->tf.col[2]= efa->tf.col[3]= vpaint_get_current_col();
	}

	BLI_addtail(&em->faces, efa);

	if(efa->v4) {
		CalcNormFloat4(efa->v1->co, efa->v2->co, efa->v3->co, efa->v4->co, efa->n);
		CalcCent4f(efa->cent, efa->v1->co, efa->v2->co, efa->v3->co, efa->v4->co);
	}
	else {
		CalcNormFloat(efa->v1->co, efa->v2->co, efa->v3->co, efa->n);
		CalcCent3f(efa->cent, efa->v1->co, efa->v2->co, efa->v3->co);
	}

	return efa;
}

/* ************************ end add/new/find ************  */

/* ************************ Edit{Vert,Edge,Face} utilss ***************************** */

/* some nice utility functions */

EditVert *editedge_getOtherVert(EditEdge *eed, EditVert *eve)
{
	if (eve==eed->v1) {
		return eed->v2;
	} else if (eve==eed->v2) {
		return eed->v1;
	} else {
		return NULL;
	}
}

EditVert *editedge_getSharedVert(EditEdge *eed, EditEdge *eed2) 
{
	if (eed->v1==eed2->v1 || eed->v1==eed2->v2) {
		return eed->v1;
	} else if (eed->v2==eed2->v1 || eed->v2==eed2->v2) {
		return eed->v2;
	} else {
		return NULL;
	}
}

int editedge_containsVert(EditEdge *eed, EditVert *eve) 
{
	return (eed->v1==eve || eed->v2==eve);
}

int editface_containsVert(EditFace *efa, EditVert *eve) 
{
	return (efa->v1==eve || efa->v2==eve || efa->v3==eve || (efa->v4 && efa->v4==eve));
}

int editface_containsEdge(EditFace *efa, EditEdge *eed) 
{
	return (efa->e1==eed || efa->e2==eed || efa->e3==eed || (efa->e4 && efa->e4==eed));
}


/* ************************ stuct EditMesh manipulation ***************************** */

/* fake callocs for fastmalloc below */
static void *calloc_fastvert(size_t size, size_t nr)
{
	EditVert *eve= G.editMesh->curvert++;
	eve->fast= 1;
	return eve;
}
static void *calloc_fastedge(size_t size, size_t nr)
{
	EditEdge *eed= G.editMesh->curedge++;
	eed->fast= 1;
	return eed;
}
static void *calloc_fastface(size_t size, size_t nr)
{
	EditFace *efa= G.editMesh->curface++;
	efa->fast= 1;
	return efa;
}

/* allocate 1 chunk for all vertices, edges, faces. These get tagged to
   prevent it from being freed
*/
static void init_editmesh_fastmalloc(EditMesh *em, int totvert, int totedge, int totface)
{
	
	if(totvert) em->allverts= MEM_callocN(totvert*sizeof(EditVert), "allverts");
	else em->allverts= NULL;
	em->curvert= em->allverts;
	
	if(totedge==0) totedge= 4*totface;	// max possible

	if(totedge) em->alledges= MEM_callocN(totedge*sizeof(EditEdge), "alledges");
	else em->alledges= NULL;
	em->curedge= em->alledges;
	
	if(totface) em->allfaces= MEM_callocN(totface*sizeof(EditFace), "allfaces");
	else em->allfaces= NULL;
	em->curface= em->allfaces;

	callocvert= calloc_fastvert;
	callocedge= calloc_fastedge;
	callocface= calloc_fastface;
}

static void end_editmesh_fastmalloc(void)
{
	callocvert= calloc;
	callocedge= calloc;
	callocface= calloc;
}

void free_editMesh(EditMesh *em)
{
	if(em==NULL) return;
	
	if(em->verts.first) free_vertlist(&em->verts);
	if(em->edges.first) free_edgelist(&em->edges);
	if(em->faces.first) free_facelist(&em->faces);

	if(em->derivedFinal) {
		if (em->derivedFinal!=em->derivedCage) {
			em->derivedFinal->release(em->derivedFinal);
		}
		em->derivedFinal= NULL;
	}
	if(em->derivedCage) {
		em->derivedCage->release(em->derivedCage);
		em->derivedCage= NULL;
	}

	/* DEBUG: hashtabs are slowest part of enter/exit editmode. here a testprint */
#if 0
	if(em->hashedgetab) {
		HashEdge *he, *hen;
		int a, used=0, max=0, nr;
		he= em->hashedgetab;
		for(a=0; a<EDHASHSIZE; a++, he++) {
			if(he->eed) used++;
			hen= he->next;
			nr= 0;
			while(hen) {
				nr++;
				hen= hen->next;
			}
			if(max<nr) max= nr;
		}
		printf("hastab used %d max %d\n", used, max);
	}
#endif
	if(em->hashedgetab) MEM_freeN(em->hashedgetab);
	em->hashedgetab= NULL;
	
	if(em->allverts) MEM_freeN(em->allverts);
	if(em->alledges) MEM_freeN(em->alledges);
	if(em->allfaces) MEM_freeN(em->allfaces);
	
	em->allverts= em->curvert= NULL;
	em->alledges= em->curedge= NULL;
	em->allfaces= em->curface= NULL;
	
	mesh_octree_table(NULL, NULL, 'e');
	
	G.totvert= G.totface= 0;
}

/* on G.editMesh */
static void editMesh_set_hash(void)
{
	EditEdge *eed;

	G.editMesh->hashedgetab= NULL;
	
	for(eed=G.editMesh->edges.first; eed; eed= eed->next)  {
		if( findedgelist(eed->v1, eed->v2)==NULL )
			insert_hashedge(eed);
	}

}


/* ************************ IN & OUT EDITMODE ***************************** */


static void edge_normal_compare(EditEdge *eed, EditFace *efa1)
{
	EditFace *efa2;
	float cent1[3], cent2[3];
	float inp;
	
	efa2 = eed->tmp.f;
	if(efa1==efa2) return;
	
	inp= efa1->n[0]*efa2->n[0] + efa1->n[1]*efa2->n[1] + efa1->n[2]*efa2->n[2];
	if(inp<0.999 && inp >-0.999) eed->f2= 1;
		
	if(efa1->v4) CalcCent4f(cent1, efa1->v1->co, efa1->v2->co, efa1->v3->co, efa1->v4->co);
	else CalcCent3f(cent1, efa1->v1->co, efa1->v2->co, efa1->v3->co);
	if(efa2->v4) CalcCent4f(cent2, efa2->v1->co, efa2->v2->co, efa2->v3->co, efa2->v4->co);
	else CalcCent3f(cent2, efa2->v1->co, efa2->v2->co, efa2->v3->co);
	
	VecSubf(cent1, cent2, cent1);
	Normalise(cent1);
	inp= cent1[0]*efa1->n[0] + cent1[1]*efa1->n[1] + cent1[2]*efa1->n[2]; 

	if(inp < -0.001 ) eed->f1= 1;
}

#if 0
typedef struct {
	EditEdge *eed;
	float noLen,no[3];
	int adjCount;
} EdgeDrawFlagInfo;

static int edgeDrawFlagInfo_cmp(const void *av, const void *bv)
{
	const EdgeDrawFlagInfo *a = av;
	const EdgeDrawFlagInfo *b = bv;

	if (a->noLen<b->noLen) return -1;
	else if (a->noLen>b->noLen) return 1;
	else return 0;
}
#endif

static void edge_drawflags(void)
{
	EditMesh *em = G.editMesh;
	EditVert *eve;
	EditEdge *eed, *e1, *e2, *e3, *e4;
	EditFace *efa;
	
	/* - count number of times edges are used in faces: 0 en 1 time means draw edge
	 * - edges more than 1 time used: in *tmp.f is pointer to first face
	 * - check all faces, when normal differs to much: draw (flag becomes 1)
	 */

	/* later on: added flags for 'cylinder' and 'sphere' intersection tests in old
	   game engine (2.04)
	 */
	
	recalc_editnormals();
	
	/* init */
	eve= em->verts.first;
	while(eve) {
		eve->f1= 1;		/* during test it's set at zero */
		eve= eve->next;
	}
	eed= em->edges.first;
	while(eed) {
		eed->f2= eed->f1= 0;
		eed->tmp.f = 0;
		eed= eed->next;
	}

	efa= em->faces.first;
	while(efa) {
		e1= efa->e1;
		e2= efa->e2;
		e3= efa->e3;
		e4= efa->e4;
		if(e1->f2<4) e1->f2+= 1;
		if(e2->f2<4) e2->f2+= 1;
		if(e3->f2<4) e3->f2+= 1;
		if(e4 && e4->f2<4) e4->f2+= 1;
		
		if(e1->tmp.f == 0) e1->tmp.f = (void *) efa;
		if(e2->tmp.f == 0) e2->tmp.f = (void *) efa;
		if(e3->tmp.f ==0) e3->tmp.f = (void *) efa;
		if(e4 && (e4->tmp.f == 0)) e4->tmp.f = (void *) efa;
		
		efa= efa->next;
	}

	if(G.f & G_ALLEDGES) {
		efa= em->faces.first;
		while(efa) {
			if(efa->e1->f2>=2) efa->e1->f2= 1;
			if(efa->e2->f2>=2) efa->e2->f2= 1;
			if(efa->e3->f2>=2) efa->e3->f2= 1;
			if(efa->e4 && efa->e4->f2>=2) efa->e4->f2= 1;
			
			efa= efa->next;
		}		
	}	
	else {
		
		/* handle single-edges for 'test cylinder flag' (old engine) */
		
		eed= em->edges.first;
		while(eed) {
			if(eed->f2==1) eed->f1= 1;
			eed= eed->next;
		}

		/* all faces, all edges with flag==2: compare normal */
		efa= em->faces.first;
		while(efa) {
			if(efa->e1->f2==2) edge_normal_compare(efa->e1, efa);
			else efa->e1->f2= 1;
			if(efa->e2->f2==2) edge_normal_compare(efa->e2, efa);
			else efa->e2->f2= 1;
			if(efa->e3->f2==2) edge_normal_compare(efa->e3, efa);
			else efa->e3->f2= 1;
			if(efa->e4) {
				if(efa->e4->f2==2) edge_normal_compare(efa->e4, efa);
				else efa->e4->f2= 1;
			}
			efa= efa->next;
		}
		
		/* sphere collision flag */
		
		eed= em->edges.first;
		while(eed) {
			if(eed->f1!=1) {
				eed->v1->f1= eed->v2->f1= 0;
			}
			eed= eed->next;
		}
		
	}
}

/* turns Mesh into editmesh */
void make_editMesh()
{
	Mesh *me= G.obedit->data;
	MFace *mface;
	TFace *tface;
	MVert *mvert;
	KeyBlock *actkey;
	EditVert *eve, **evlist, *eve1, *eve2, *eve3, *eve4;
	EditFace *efa;
	EditEdge *eed;
	int tot, a;

	/* because of reload */
	free_editMesh(G.editMesh);
	
	G.totvert= tot= me->totvert;

	if(tot==0) {
		countall();
		return;
	}
	
	waitcursor(1);

	/* initialize fastmalloc for editmesh */
	init_editmesh_fastmalloc(G.editMesh, me->totvert, me->totedge, me->totface);

	actkey = ob_get_keyblock(G.obedit);
	if(actkey) {
		strcpy(G.editModeTitleExtra, "(Key) ");
		key_to_mesh(actkey, me);
		tot= actkey->totelem;
	}

	/* make editverts */
	mvert= me->mvert;

	evlist= (EditVert **)MEM_mallocN(tot*sizeof(void *),"evlist");
	for(a=0; a<tot; a++, mvert++) {
		eve= addvertlist(mvert->co);
		evlist[a]= eve;
		
		// face select sets selection in next loop
		if( (G.f & G_FACESELECT)==0 )
			eve->f |= (mvert->flag & 1);
		
		if (mvert->flag & ME_HIDE) eve->h= 1;		
		eve->no[0]= mvert->no[0]/32767.0;
		eve->no[1]= mvert->no[1]/32767.0;
		eve->no[2]= mvert->no[2]/32767.0;

		/* lets overwrite the keyindex of the editvert
		 * with the order it used to be in before
		 * editmode
		 */
		eve->keyindex = a;

		if (me->dvert){
			eve->totweight = me->dvert[a].totweight;
			if (me->dvert[a].dw){
				eve->dw = MEM_callocN (sizeof(MDeformWeight) * me->dvert[a].totweight, "deformWeight");
				memcpy (eve->dw, me->dvert[a].dw, sizeof(MDeformWeight) * me->dvert[a].totweight);
			}
		}

	}
	
	if(actkey && actkey->totelem!=me->totvert);
	else {
		unsigned int *mcol;
		MEdge *medge= me->medge;
		
		/* make edges */
		for(a=0; a<me->totedge; a++, medge++) {
			eed= addedgelist(evlist[medge->v1], evlist[medge->v2], NULL);
			eed->crease= ((float)medge->crease)/255.0;
			
			if(medge->flag & ME_SEAM) eed->seam= 1;
			if(medge->flag & SELECT) eed->f |= SELECT;
			if(medge->flag & ME_FGON) eed->h= EM_FGON;	// 2 different defines!
			if(medge->flag & ME_HIDE) eed->h |= 1;
			if(G.scene->selectmode==SCE_SELECT_EDGE) 
				EM_select_edge(eed, eed->f & SELECT);		// force edge selection to vertices, seems to be needed ...
		}
		
		/* make faces */
		mface= me->mface;
		tface= me->tface;
		mcol= (unsigned int *)me->mcol;
		
		for(a=0; a<me->totface; a++, mface++) {
			eve1= evlist[mface->v1];
			eve2= evlist[mface->v2];
			if(!mface->v3) error("Eeekadoodle! An MFACE-EDGE! Tell Zr!!!!");
			eve3= evlist[mface->v3];
			if(mface->v4) eve4= evlist[mface->v4]; else eve4= NULL;
			
			efa= addfacelist(eve1, eve2, eve3, eve4, NULL, NULL);

			if(efa) {
			
				if(mcol) memcpy(efa->tf.col, mcol, 4*sizeof(int));

				if(me->tface) {
					efa->tf= *tface;
				}
			
				efa->mat_nr= mface->mat_nr;
				efa->flag= mface->flag & ~ME_HIDE;
				
				if((G.f & G_FACESELECT)==0) {
					/* select face flag, if no edges we flush down */
					if(mface->flag & ME_FACE_SEL) {
						efa->f |= SELECT;
					}
					if(mface->flag & ME_HIDE) efa->h= 1;
				}
				else if (tface) {
					if( tface->flag & TF_HIDE) 
						efa->h= 1;
					else if( tface->flag & TF_SELECT) {
						EM_select_face(efa, 1);
					}
				}
			}

			if(me->tface) tface++;
			if(mcol) mcol+=4;
		}
	}
	
	MEM_freeN(evlist);

	end_editmesh_fastmalloc();	// resets global function pointers
	
	/* this creates coherent selections. also needed for older files */
	EM_selectmode_set();
	/* paranoia check to enforce hide rules */
	EM_hide_reset();
	/* sets helper flags which arent saved */
	EM_fgon_flags();
	
	countall();
	
	waitcursor(0);
}

/* makes Mesh out of editmesh */
void load_editMesh(void)
{
	EditMesh *em = G.editMesh;
	Mesh *me= G.obedit->data;
	MVert *mvert, *oldverts;
	MEdge *medge;
	MFace *mface;
	MSticky *ms;
	EditVert *eve;
	EditFace *efa;
	EditEdge *eed;
	float *fp, *newkey, *oldkey, nor[3];
	int i, a, ototvert, totedge=0;
	MDeformVert *dvert;

	waitcursor(1);
	countall();

	/* this one also tests of edges are not in faces: */
	/* eed->f2==0: not in face, f2==1: draw it */
	/* eed->f1 : flag for dynaface (cylindertest, old engine) */
	/* eve->f1 : flag for dynaface (sphere test, old engine) */
	/* eve->f2 : being used in vertexnormals */
	edge_drawflags();
	
	eed= em->edges.first;
	while(eed) {
		totedge++;
		eed= eed->next;
	}
	
	/* new Vertex block */
	if(G.totvert==0) mvert= NULL;
	else mvert= MEM_callocN(G.totvert*sizeof(MVert), "loadeditMesh vert");

	/* new Edge block */
	if(totedge==0) medge= NULL;
	else medge= MEM_callocN(totedge*sizeof(MEdge), "loadeditMesh edge");
	
	/* new Face block */
	if(G.totface==0) mface= NULL;
	else mface= MEM_callocN(G.totface*sizeof(MFace), "loadeditMesh face");
	
	/* are we adding dverts? */
	if (G.totvert==0) dvert= NULL;
	else if(G.obedit->defbase.first==NULL) dvert= NULL;
	else dvert = MEM_callocN(G.totvert*sizeof(MDeformVert), "loadeditMesh3");

	if (me->dvert) free_dverts(me->dvert, me->totvert);
	me->dvert=dvert;

	/* lets save the old verts just in case we are actually working on
	 * a key ... we now do processing of the keys at the end */
	oldverts = me->mvert;
	ototvert= me->totvert;

	/* put new data in Mesh */
	me->mvert= mvert;
	me->totvert= G.totvert;

	if(me->medge) MEM_freeN(me->medge);
	me->medge= medge;
	me->totedge= totedge;
	
	if(me->mface) MEM_freeN(me->mface);
	me->mface= mface;
	me->totface= G.totface;
		
	/* the vertices, use ->tmp.l as counter */
	eve= em->verts.first;
	a= 0;

	while(eve) {
		VECCOPY(mvert->co, eve->co);
		mvert->mat_nr= 255;  /* what was this for, halos? */
		
		/* vertex normal */
		VECCOPY(nor, eve->no);
		VecMulf(nor, 32767.0);
		VECCOPY(mvert->no, nor);

		/* note: it used to remove me->dvert when it was not in use, cancelled that... annoying when you have a fresh vgroup */
		if (dvert){
			dvert->totweight=eve->totweight;
			if (eve->dw){
				dvert->dw = MEM_callocN (sizeof(MDeformWeight)*eve->totweight,
										 "deformWeight");
				memcpy (dvert->dw, eve->dw, 
						sizeof(MDeformWeight)*eve->totweight);
			}
		}

		eve->tmp.l = a++;  /* counter */
			
		mvert->flag= 0;
		if(eve->f1==1) mvert->flag |= ME_SPHERETEST;
		mvert->flag |= (eve->f & SELECT);
		if (eve->h) mvert->flag |= ME_HIDE;			
			
		eve= eve->next;
		mvert++;
		if(dvert) dvert++;
	}

	/* the edges */
	eed= em->edges.first;
	while(eed) {
		medge->v1= (unsigned int) eed->v1->tmp.l;
		medge->v2= (unsigned int) eed->v2->tmp.l;
		
		medge->flag= (eed->f & SELECT) | ME_EDGERENDER;
		if(eed->f2<2) medge->flag |= ME_EDGEDRAW;
		if(eed->f2==0) medge->flag |= ME_LOOSEEDGE;
		if(eed->seam) medge->flag |= ME_SEAM;
		if(eed->h & EM_FGON) medge->flag |= ME_FGON;	// different defines yes
		if(eed->h & 1) medge->flag |= ME_HIDE;
		
		medge->crease= (char)(255.0*eed->crease);

		medge++;
		eed= eed->next;
	}

	/* the faces */
	efa= em->faces.first;
	i = 0;
	while(efa) {
		mface= &((MFace *) me->mface)[i];
		
		mface->v1= (unsigned int) efa->v1->tmp.l;
		mface->v2= (unsigned int) efa->v2->tmp.l;
		mface->v3= (unsigned int) efa->v3->tmp.l;
		if (efa->v4) mface->v4 = (unsigned int) efa->v4->tmp.l;
			
		mface->mat_nr= efa->mat_nr;
		
		mface->flag= efa->flag;
		/* bit 0 of flag is already taken for smooth... */
		if(efa->f & 1) mface->flag |= ME_FACE_SEL;
		else mface->flag &= ~ME_FACE_SEL;
		if(efa->h) mface->flag |= ME_HIDE;
		
		/* mat_nr in vertex */
		if(me->totcol>1) {
			mvert= me->mvert+mface->v1;
			if(mvert->mat_nr == (char)255) mvert->mat_nr= mface->mat_nr;
			mvert= me->mvert+mface->v2;
			if(mvert->mat_nr == (char)255) mvert->mat_nr= mface->mat_nr;
			mvert= me->mvert+mface->v3;
			if(mvert->mat_nr == (char)255) mvert->mat_nr= mface->mat_nr;
			if(mface->v4) {
				mvert= me->mvert+mface->v4;
				if(mvert->mat_nr == (char)255) mvert->mat_nr= mface->mat_nr;
			}
		}
			
		/* watch: efa->e1->f2==0 means loose edge */ 
			
		if(efa->e1->f2==1) {
			efa->e1->f2= 2;
		}			
		if(efa->e2->f2==1) {
			efa->e2->f2= 2;
		}
		if(efa->e3->f2==1) {
			efa->e3->f2= 2;
		}
		if(efa->e4 && efa->e4->f2==1) {
			efa->e4->f2= 2;
		}


		/* no index '0' at location 3 or 4 */
		test_index_face(mface, NULL, &efa->tf, efa->v4?4:3);
			
		i++;
		efa= efa->next;
	}
	
	/* tface block */
	if( me->tface && me->totface ) {
		TFace *tfn, *tf;
			
		tf=tfn= MEM_callocN(sizeof(TFace)*me->totface, "tface");
		efa= em->faces.first;
		while(efa) {
				
			*tf= efa->tf;
				
			if(G.f & G_FACESELECT) {
				if( efa->h) tf->flag |= TF_HIDE;
				else tf->flag &= ~TF_HIDE;
				if( efa->f & SELECT)  tf->flag |= TF_SELECT;
				else tf->flag &= ~TF_SELECT;
			}
				
			tf++;
			efa= efa->next;
		}

		if(me->tface) MEM_freeN(me->tface);
		me->tface= tfn;
	}
	else if(me->tface) {
		MEM_freeN(me->tface);
		me->tface= NULL;
	}
		
	/* mcol: same as tface... */
	if(me->mcol && me->totface) {
		unsigned int *mcn, *mc;

		mc=mcn= MEM_mallocN(4*sizeof(int)*me->totface, "mcol");
		efa= em->faces.first;
		while(efa) {
			memcpy(mc, efa->tf.col, 4*sizeof(int));
				
			mc+=4;
			efa= efa->next;
		}
		if(me->mcol) MEM_freeN(me->mcol);
			me->mcol= (MCol *)mcn;
	}
	else if(me->mcol) {
		MEM_freeN(me->mcol);
		me->mcol= 0;
	}

	/* patch hook indices */
	{
		Object *ob;
		ModifierData *md;
		EditVert *eve, **vertMap = NULL;
		int i,j;

		for (ob=G.main->object.first; ob; ob=ob->id.next) {
			if (ob->data==me) {
				for (md=ob->modifiers.first; md; md=md->next) {
					if (md->type==eModifierType_Hook) {
						HookModifierData *hmd = (HookModifierData*) md;

						if (!vertMap) {
							vertMap = MEM_callocN(sizeof(*vertMap)*ototvert, "vertMap");

							for (eve=em->verts.first; eve; eve=eve->next) {
								if (eve->keyindex!=-1)
									vertMap[eve->keyindex] = eve;
							}
						}
						
						for (i=j=0; i<hmd->totindex; i++) {
							if(hmd->indexar[i] < ototvert) {
								eve = vertMap[hmd->indexar[i]];
								
								if (eve) {
									hmd->indexar[j++] = eve->tmp.l;
								}
							}
							else j++;
						}

						hmd->totindex = j;
					}
				}
			}
		}

		if (vertMap) MEM_freeN(vertMap);
	}

	/* are there keys? */
	if(me->key) {
		KeyBlock *currkey, *actkey = ob_get_keyblock(G.obedit);

		/* Lets reorder the key data so that things line up roughly
		 * with the way things were before editmode */
		currkey = me->key->block.first;
		while(currkey) {
			
			fp= newkey= MEM_callocN(me->key->elemsize*G.totvert,  "currkey->data");
			oldkey = currkey->data;

			eve= em->verts.first;

			i = 0;
			mvert = me->mvert;
			while(eve) {
				if (eve->keyindex >= 0 && eve->keyindex < currkey->totelem) { // valid old vertex
					if(currkey == actkey) {
						if (actkey == me->key->refkey) {
							VECCOPY(fp, mvert->co);
						}
						else {
							VECCOPY(fp, mvert->co);
							if(oldverts) {
								VECCOPY(mvert->co, oldverts[eve->keyindex].co);
							}
						}
					}
					else {
						if(oldkey) {
							VECCOPY(fp, oldkey + 3 * eve->keyindex);
						}
					}
				}
				else {
					VECCOPY(fp, mvert->co);
				}
				fp+= 3;
				++i;
				++mvert;
				eve= eve->next;
			}
			currkey->totelem= G.totvert;
			if(currkey->data) MEM_freeN(currkey->data);
			currkey->data = newkey;
			
			currkey= currkey->next;
		}
	}

	if(oldverts) MEM_freeN(oldverts);
	
	/* to be sure: clear ->tmp.l pointers */
	eve= em->verts.first;
	while(eve) {
		eve->tmp.l = 0;
		eve= eve->next;
	}
	
	/* remake softbody of all users */
	if(me->id.us>1) {
		Base *base;
		for(base= G.scene->base.first; base; base= base->next) {
			if(base->object->data==me) {
				base->object->softflag |= OB_SB_REDO;
				base->object->recalc |= OB_RECALC_DATA;
			}
		}
	}
	
	/* sticky */
	if(me->msticky) {
		if (ototvert<me->totvert) {
			ms= MEM_callocN(me->totvert*sizeof(MSticky), "msticky");
			memcpy(ms, me->msticky, ototvert*sizeof(MSticky));
			MEM_freeN(me->msticky);
			me->msticky= ms;
			error("Sticky was too small");
		}
	}

	mesh_calc_normals(me->mvert, me->totvert, me->mface, me->totface, NULL);

	waitcursor(0);
}

void remake_editMesh(void)
{
	make_editMesh();
	allqueue(REDRAWVIEW3D, 0);
	DAG_object_flush_update(G.scene, G.obedit, OB_RECALC_DATA);
	BIF_undo_push("Undo all changes");
}

/* *************** SEPARATE (partial exit editmode) *************/


void separatemenu(void)
{
	short event;

	if(G.editMesh->verts.first==NULL) return;
	   
	event = pupmenu("Separate %t|Selected%x1|All Loose Parts%x2");
	
	if (event==0) return;
	waitcursor(1);
	
	switch (event) {
	case 1: 
		separate_mesh();		    
		break;
	case 2:	    	    	    
		separate_mesh_loose();	    	    
		break;
	}
	waitcursor(0);
}


void separate_mesh(void)
{
	EditMesh *em = G.editMesh;
	EditMesh emcopy;
	EditVert *eve, *v1;
	EditEdge *eed, *e1;
	EditFace *efa, *vl1;
	Object *oldob;
	Mesh *me, *men;
	Base *base, *oldbase;
	ListBase edve, eded, edvl;
	
	TEST_EDITMESH	

	waitcursor(1);
	
	me= get_mesh(G.obedit);
	if(me->key) {
		error("Can't separate with vertex keys");
		return;
	}
	
	EM_selectmode_set();	// enforce full consistant selection flags 
	
	/* we are going to abuse the system as follows:
	 * 1. add a duplicate object: this will be the new one, we remember old pointer
	 * 2: then do a split if needed.
	 * 3. put apart: all NOT selected verts, edges, faces
	 * 4. call load_editMesh(): this will be the new object
	 * 5. freelist and get back old verts, edges, facs
	 */
	
	/* make only obedit selected */
	base= FIRSTBASE;
	while(base) {
		if(base->lay & G.vd->lay) {
			if(base->object==G.obedit) base->flag |= SELECT;
			else base->flag &= ~SELECT;
		}
		base= base->next;
	}
	
	/* no test for split, split doesn't split when a loose part is selected */
	/* SPLIT: first make duplicate */
	adduplicateflag(SELECT);
	/* SPLIT: old faces have 3x flag 128 set, delete these ones */
	delfaceflag(128);
	
	/* since we do tricky things with verts/edges/faces, this makes sure all is selected coherent */
	EM_selectmode_set();
	
	/* set apart: everything that is not selected */
	edve.first= edve.last= eded.first= eded.last= edvl.first= edvl.last= 0;
	eve= em->verts.first;
	while(eve) {
		v1= eve->next;
		if((eve->f & SELECT)==0) {
			BLI_remlink(&em->verts, eve);
			BLI_addtail(&edve, eve);
		}
		eve= v1;
	}
	eed= em->edges.first;
	while(eed) {
		e1= eed->next;
		if((eed->f & SELECT)==0) {
			BLI_remlink(&em->edges, eed);
			BLI_addtail(&eded, eed);
		}
		eed= e1;
	}
	efa= em->faces.first;
	while(efa) {
		vl1= efa->next;
		if((efa->f & SELECT)==0) {
			BLI_remlink(&em->faces, efa);
			BLI_addtail(&edvl, efa);
		}
		efa= vl1;
	}
	
	oldob= G.obedit;
	oldbase= BASACT;
	
	adduplicate(1, 0); /* notrans and a linked duplicate*/
	
	G.obedit= BASACT->object;	/* basact was set in adduplicate()  */

	men= copy_mesh(me);
	set_mesh(G.obedit, men);
	/* because new mesh is a copy: reduce user count */
	men->id.us--;
	
	load_editMesh();
	
	BASACT->flag &= ~SELECT;
	
	/* we cannot free the original buffer... */
	emcopy= *G.editMesh;
	emcopy.allverts= NULL;
	emcopy.alledges= NULL;
	emcopy.allfaces= NULL;
	emcopy.derivedFinal= emcopy.derivedCage= NULL;
	free_editMesh(&emcopy);
	
	em->verts= edve;
	em->edges= eded;
	em->faces= edvl;
	
	/* hashedges are freed now, make new! */
	editMesh_set_hash();
	
	DAG_object_flush_update(G.scene, G.obedit, OB_RECALC_DATA);	
	G.obedit= oldob;
	BASACT= oldbase;
	BASACT->flag |= SELECT;
	
	waitcursor(0);

	countall();
	allqueue(REDRAWVIEW3D, 0);
	DAG_object_flush_update(G.scene, G.obedit, OB_RECALC_DATA);	

}

void separate_mesh_loose(void)
{
	EditMesh *em = G.editMesh;
	EditMesh emcopy;
	EditVert *eve, *v1;
	EditEdge *eed, *e1;
	EditFace *efa, *vl1;
	Object *oldob;
	Mesh *me, *men;
	Base *base, *oldbase;
	ListBase edve, eded, edvl;
	int vertsep=0;	
	short done=0, check=1;
		
	TEST_EDITMESH
	waitcursor(1);	
	
	/* we are going to abuse the system as follows:
	 * 1. add a duplicate object: this will be the new one, we remember old pointer
	 * 2: then do a split if needed.
	 * 3. put apart: all NOT selected verts, edges, faces
	 * 4. call load_editMesh(): this will be the new object
	 * 5. freelist and get back old verts, edges, facs
	 */
			
	
			
	while(!done){		
		vertsep=check=1;
		
		countall();
		
		me= get_mesh(G.obedit);
		if(me->key) {
			error("Can't separate a mesh with vertex keys");
			return;
		}		
		
		/* make only obedit selected */
		base= FIRSTBASE;
		while(base) {
			if(base->lay & G.vd->lay) {
				if(base->object==G.obedit) base->flag |= SELECT;
				else base->flag &= ~SELECT;
			}
			base= base->next;
		}		
		
		/*--------- Select connected-----------*/		
		
		EM_clear_flag_all(SELECT);

		/* Select a random vert to start with */
		eve= em->verts.first;
		eve->f |= SELECT;
		
		while(check==1) {
			check= 0;			
			eed= em->edges.first;			
			while(eed) {				
				if(eed->h==0) {
					if(eed->v1->f & SELECT) {
						if( (eed->v2->f & SELECT)==0 ) {
							eed->v2->f |= SELECT;
							vertsep++;
							check= 1;
						}
					}
					else if(eed->v2->f & SELECT) {
						if( (eed->v1->f & SELECT)==0 ) {
							eed->v1->f |= SELECT;
							vertsep++;
							check= SELECT;
						}
					}
				}
				eed= eed->next;				
			}
		}		
		/*----------End of select connected--------*/
		
		
		/* If the amount of vertices that is about to be split == the total amount 
		   of verts in the mesh, it means that there is only 1 unconnected object, so we don't have to separate
		*/
		if(G.totvert==vertsep) done=1;				
		else{			
			/* No splitting: select connected goes fine */
			
			EM_select_flush();	// from verts->edges->faces

			/* set apart: everything that is not selected */
			edve.first= edve.last= eded.first= eded.last= edvl.first= edvl.last= 0;
			eve= em->verts.first;
			while(eve) {
				v1= eve->next;
				if((eve->f & SELECT)==0) {
					BLI_remlink(&em->verts, eve);
					BLI_addtail(&edve, eve);
				}
				eve= v1;
			}
			eed= em->edges.first;
			while(eed) {
				e1= eed->next;
				if( (eed->f & SELECT)==0 ) {
					BLI_remlink(&em->edges, eed);
					BLI_addtail(&eded, eed);
				}
				eed= e1;
			}
			efa= em->faces.first;
			while(efa) {
				vl1= efa->next;
				if( (efa->f & SELECT)==0 ) {
					BLI_remlink(&em->faces, efa);
					BLI_addtail(&edvl, efa);
				}
				efa= vl1;
			}
			
			oldob= G.obedit;
			oldbase= BASACT;
			
			adduplicate(1, 0); /* notrans and 0 for linked duplicate */
			
			G.obedit= BASACT->object;	/* basact was set in adduplicate()  */
		
			men= copy_mesh(me);
			set_mesh(G.obedit, men);
			/* because new mesh is a copy: reduce user count */
			men->id.us--;
			
			load_editMesh();
			
			BASACT->flag &= ~SELECT;
			
			/* we cannot free the original buffer... */
			emcopy= *G.editMesh;
			emcopy.allverts= NULL;
			emcopy.alledges= NULL;
			emcopy.allfaces= NULL;
			emcopy.derivedFinal= emcopy.derivedCage= NULL;
			free_editMesh(&emcopy);
			
			em->verts= edve;
			em->edges= eded;
			em->faces= edvl;
			
			/* hashedges are freed now, make new! */
			editMesh_set_hash();
			
			G.obedit= oldob;
			BASACT= oldbase;
			BASACT->flag |= SELECT;	
					
		}		
	}
	
	/* unselect the vertices that we (ab)used for the separation*/
	EM_clear_flag_all(SELECT);
		
	waitcursor(0);
	countall();
	allqueue(REDRAWVIEW3D, 0);
	DAG_object_flush_update(G.scene, G.obedit, OB_RECALC_DATA);	
}

/* ******************************************** */

/* *************** UNDO ***************************** */
/* new mesh undo, based on pushing editmesh data itself */
/* reuses same code as for global and curve undo... unify that (ton) */

/* only one 'hack', to save memory it doesn't store the first push, but does a remake editmesh */

/* a compressed version of editmesh data */
typedef struct EditVertC
{
	float no[3];
	float co[3];
	unsigned char f, h;
	short totweight;
	struct MDeformWeight *dw;
	int keyindex; 
} EditVertC;

typedef struct EditEdgeC
{
	int v1, v2;
	unsigned char f, h, seam, pad;
	short crease, fgoni;
} EditEdgeC;

typedef struct EditFaceC
{
	int v1, v2, v3, v4;
	unsigned char mat_nr, flag, f, h, fgonf;
	short pad1;
} EditFaceC;

typedef struct UndoMesh {
	EditVertC *verts;
	EditEdgeC *edges;
	EditFaceC *faces;
	TFace *tfaces;
	int totvert, totedge, totface;
	int lastvert, firstvert; /*index for last and first selected vert. -1 if no first/last vert exists (nasty)*/
	short selectmode;
} UndoMesh;


/* for callbacks */

static void free_undoMesh(void *umv)
{
	UndoMesh *um= umv;
	EditVertC *evec;
	int a;
	
	for(a=0, evec= um->verts; a<um->totvert; a++, evec++) {
		if(evec->dw) MEM_freeN(evec->dw);
	}
	
	if(um->verts) MEM_freeN(um->verts);
	if(um->edges) MEM_freeN(um->edges);
	if(um->faces) MEM_freeN(um->faces);
	if(um->tfaces) MEM_freeN(um->tfaces);
	MEM_freeN(um);
}

static void *editMesh_to_undoMesh(void)
{
	EditMesh *em= G.editMesh;
	UndoMesh *um;
	Mesh *me= G.obedit->data;
	EditVert *eve;
	EditEdge *eed;
	EditFace *efa;
	EditVertC *evec=NULL;
	EditEdgeC *eedc=NULL;
	EditFaceC *efac=NULL;
	TFace *tface= NULL;
	int i, a=0;
	
	um= MEM_callocN(sizeof(UndoMesh), "undomesh");
	
	um->selectmode = G.scene->selectmode;
	um->lastvert = -1;
	um->firstvert = -1;
	
	if(em->lastvert){
		for (i=0,eve=G.editMesh->verts.first; eve; i++,eve=eve->next){
			if(eve == em->lastvert){
				um->lastvert = i;
				break;
			}
		}
	}
	if(em->firstvert){
		for (i=0,eve=G.editMesh->verts.first; eve; i++,eve=eve->next){
			if(eve == em->firstvert){
				um->firstvert = i;
				break;
			}
		}
	}

	for(eve=em->verts.first; eve; eve= eve->next) um->totvert++;
	for(eed=em->edges.first; eed; eed= eed->next) um->totedge++;
	for(efa=em->faces.first; efa; efa= efa->next) um->totface++;

	/* malloc blocks */
	
	if(um->totvert) evec= um->verts= MEM_callocN(um->totvert*sizeof(EditVertC), "allvertsC");
	if(um->totedge) eedc= um->edges= MEM_callocN(um->totedge*sizeof(EditEdgeC), "alledgesC");
	if(um->totface) efac= um->faces= MEM_callocN(um->totface*sizeof(EditFaceC), "allfacesC");

	if(me->tface || me->mcol) tface= um->tfaces= MEM_mallocN(um->totface*sizeof(TFace), "all tfacesC");

		//printf("copy editmesh %d\n", um->totvert*sizeof(EditVert) + um->totedge*sizeof(EditEdge) + um->totface*sizeof(EditFace));
		//printf("copy undomesh %d\n", um->totvert*sizeof(EditVertC) + um->totedge*sizeof(EditEdgeC) + um->totface*sizeof(EditFaceC));
	
	/* now copy vertices */
	for(eve=em->verts.first; eve; eve= eve->next, evec++, a++) {
		VECCOPY(evec->co, eve->co);
		VECCOPY(evec->no, eve->no);

		evec->f= eve->f;
		evec->h= eve->h;
		evec->keyindex= eve->keyindex;
		evec->totweight= eve->totweight;
		evec->dw= MEM_dupallocN(eve->dw);
		
		eve->tmp.l = a;
	}
	
	/* copy edges */
	for(eed=em->edges.first; eed; eed= eed->next, eedc++)  {
		eedc->v1= (int)eed->v1->tmp.l;
		eedc->v2= (int)eed->v2->tmp.l;
		eedc->f= eed->f;
		eedc->h= eed->h;
		eedc->seam= eed->seam;
		eedc->crease= (short)(eed->crease*255.0);
		eedc->fgoni= eed->fgoni;
	}
	
	/* copy faces */
	for(efa=em->faces.first; efa; efa= efa->next, efac++) {
		efac->v1= (int)efa->v1->tmp.l;
		efac->v2= (int)efa->v2->tmp.l;
		efac->v3= (int)efa->v3->tmp.l;
		if(efa->v4) efac->v4= (int)efa->v4->tmp.l;
		else efac->v4= -1;
		
		efac->mat_nr= efa->mat_nr;
		efac->flag= efa->flag;
		efac->f= efa->f;
		efac->h= efa->h;
		efac->fgonf= efa->fgonf;
		
		if(tface) {
			*tface= efa->tf;
			tface++;
		}
	}
	
	return um;
}

static void undoMesh_to_editMesh(void *umv)
{
	UndoMesh *um= (UndoMesh*)umv;
	EditMesh *em= G.editMesh;
	EditVert *eve, **evar=NULL;
	EditEdge *eed;
	EditFace *efa;
	EditVertC *evec;
	EditEdgeC *eedc;
	EditFaceC *efac;
	TFace *tface;
	int a=0;
	
	G.scene->selectmode = um->selectmode;
	
	free_editMesh(G.editMesh);
	
	/* malloc blocks */
	memset(em, 0, sizeof(EditMesh));

	
		
	init_editmesh_fastmalloc(em, um->totvert, um->totedge, um->totface);

	/* now copy vertices */
	if(um->totvert) evar= MEM_mallocN(um->totvert*sizeof(EditVert *), "vertex ar");
	for(a=0, evec= um->verts; a<um->totvert; a++, evec++) {
		eve= addvertlist(evec->co);
		evar[a]= eve;

		VECCOPY(eve->no, evec->no);
		eve->f= evec->f;
		eve->h= evec->h;
		eve->totweight= evec->totweight;
		eve->keyindex= evec->keyindex;
		eve->dw= MEM_dupallocN(evec->dw);
	}

	/* copy edges */
	for(a=0, eedc= um->edges; a<um->totedge; a++, eedc++) {
		eed= addedgelist(evar[eedc->v1], evar[eedc->v2], NULL);

		eed->f= eedc->f;
		eed->h= eedc->h;
		eed->seam= eedc->seam;
		eed->fgoni= eedc->fgoni;
		eed->crease= ((float)eedc->crease)/255.0;
	}
	
	/* copy faces */
	tface= um->tfaces;
	for(a=0, efac= um->faces; a<um->totface; a++, efac++) {
		if(efac->v4 != -1)
			efa= addfacelist(evar[efac->v1], evar[efac->v2], evar[efac->v3], evar[efac->v4], NULL, NULL);
		else 
			efa= addfacelist(evar[efac->v1], evar[efac->v2], evar[efac->v3], NULL, NULL ,NULL);

		efa->mat_nr= efac->mat_nr;
		efa->flag= efac->flag;
		efa->f= efac->f;
		efa->h= efac->h;
		efa->fgonf= efac->fgonf;
		
		if(tface) {
			efa->tf= *tface;
			tface++;
		}
	}
	
	end_editmesh_fastmalloc();
	if(evar) MEM_freeN(evar);
	
	/*restore last and first selected vertex pointers*/
	
	G.totvert = um->totvert; 
	if(um->lastvert != -1 || um-> firstvert != -1){ 

		EM_init_index_arrays(1,0,0);
		if(um->lastvert != -1) em->lastvert = EM_get_vert_for_index(um->lastvert);
		else em->lastvert = NULL;
		
		if(um->firstvert != -1) em->firstvert = EM_get_vert_for_index(um->firstvert);
		else em->firstvert = NULL;
		
		EM_free_index_arrays();
	}

}


/* and this is all the undo system needs to know */
void undo_push_mesh(char *name)
{
	undo_editmode_push(name, free_undoMesh, undoMesh_to_editMesh, editMesh_to_undoMesh);
}



/* *************** END UNDO *************/

static EditVert **g_em_vert_array = NULL;
static EditEdge **g_em_edge_array = NULL;
static EditFace **g_em_face_array = NULL;

void EM_init_index_arrays(int forVert, int forEdge, int forFace)
{
	EditVert *eve;
	EditEdge *eed;
	EditFace *efa;
	int i;

	if (forVert) {
		g_em_vert_array = MEM_mallocN(sizeof(*g_em_vert_array)*G.totvert, "em_v_arr");

		for (i=0,eve=G.editMesh->verts.first; eve; i++,eve=eve->next)
			g_em_vert_array[i] = eve;
	}

	if (forEdge) {
		g_em_edge_array = MEM_mallocN(sizeof(*g_em_edge_array)*G.totedge, "em_e_arr");

		for (i=0,eed=G.editMesh->edges.first; eed; i++,eed=eed->next)
			g_em_edge_array[i] = eed;
	}

	if (forFace) {
		g_em_face_array = MEM_mallocN(sizeof(*g_em_face_array)*G.totface, "em_f_arr");

		for (i=0,efa=G.editMesh->faces.first; efa; i++,efa=efa->next)
			g_em_face_array[i] = efa;
	}
}

void EM_free_index_arrays(void)
{
	if (g_em_vert_array) MEM_freeN(g_em_vert_array);
	if (g_em_edge_array) MEM_freeN(g_em_edge_array);
	if (g_em_face_array) MEM_freeN(g_em_face_array);
	g_em_vert_array = NULL;
	g_em_edge_array = NULL;
	g_em_face_array = NULL;
}

EditVert *EM_get_vert_for_index(int index)
{
	return g_em_vert_array?g_em_vert_array[index]:NULL;
}

EditEdge *EM_get_edge_for_index(int index)
{
	return g_em_edge_array?g_em_edge_array[index]:NULL;
}

EditFace *EM_get_face_for_index(int index)
{
	return g_em_face_array?g_em_face_array[index]:NULL;
}
