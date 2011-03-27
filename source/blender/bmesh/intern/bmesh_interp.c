/**
 * BME_interp.c    August 2008
 *
 *	BM interpolation functions.
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
 * The Original Code is Copyright (C) 2007 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Geoffrey Bantle.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include "MEM_guardedalloc.h"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"

#include "BKE_customdata.h" 
#include "BKE_utildefines.h"
#include "BKE_multires.h"

#include "BLI_array.h"
#include "BLI_math.h"
#include "BLI_cellalloc.h"

#include "bmesh.h"
#include "bmesh_private.h"

/*
 * BME_INTERP.C
 *
 * Functions for interpolating data across the surface of a mesh.
 *
*/

/**
 *			bmesh_data_interp_from_verts
 *
 *  Interpolates per-vertex data from two sources to a target.
 * 
 *  Returns -
 *	Nothing
 */
void BM_Data_Interp_From_Verts(BMesh *bm, BMVert *v1, BMVert *v2, BMVert *v, float fac)
{
	void *src[2];
	float w[2];
	if (v1->head.data && v2->head.data) {
		src[0]= v1->head.data;
		src[1]= v2->head.data;
		w[0] = 1.0f-fac;
		w[1] = fac;
		CustomData_bmesh_interp(&bm->vdata, src, w, NULL, 2, v->head.data);
	}
}

/*
    BM Data Vert Average

    Sets all the customdata (e.g. vert, loop) associated with a vert
    to the average of the face regions surrounding it.
*/


void BM_Data_Vert_Average(BMesh *UNUSED(bm), BMFace *UNUSED(f))
{
	// BMIter iter;
}

/**
 *			bmesh_data_facevert_edgeinterp
 *
 *  Walks around the faces of an edge and interpolates the per-face-edge
 *  data between two sources to a target.
 * 
 *  Returns -
 *	Nothing
*/
 
void BM_Data_Facevert_Edgeinterp(BMesh *bm, BMVert *v1, BMVert *UNUSED(v2), BMVert *v, BMEdge *e1, float fac){
	void *src[2];
	float w[2];
	BMLoop *l=NULL, *v1loop = NULL, *vloop = NULL, *v2loop = NULL;
	
	w[1] = 1.0f - fac;
	w[0] = fac;

	if(!e1->l) return;
	l = e1->l;
	do{
		if(l->v == v1){ 
			v1loop = l;
			vloop = (BMLoop*)(v1loop->next);
			v2loop = (BMLoop*)(vloop->next);
		}else if(l->v == v){
			v1loop = (BMLoop*)(l->next);
			vloop = l;
			v2loop = (BMLoop*)(l->prev);
			
		}

		src[0] = v1loop->head.data;
		src[1] = v2loop->head.data;					

		CustomData_bmesh_interp(&bm->ldata, src,w, NULL, 2, vloop->head.data); 				
		l = l->radial_next;
	}while(l!=e1->l);
}

void BM_loops_to_corners(BMesh *bm, Mesh *me, int findex,
                         BMFace *f, int numTex, int numCol) 
{
	BMLoop *l;
	BMIter iter;
	MTFace *texface;
	MTexPoly *texpoly;
	MCol *mcol;
	MLoopCol *mloopcol;
	MLoopUV *mloopuv;
	int i, j;

	for(i=0; i < numTex; i++){
		texface = CustomData_get_n(&me->fdata, CD_MTFACE, findex, i);
		texpoly = CustomData_bmesh_get_n(&bm->pdata, f->head.data, CD_MTEXPOLY, i);
		
		texface->tpage = texpoly->tpage;
		texface->flag = texpoly->flag;
		texface->transp = texpoly->transp;
		texface->mode = texpoly->mode;
		texface->tile = texpoly->tile;
		texface->unwrap = texpoly->unwrap;

		j = 0;
		BM_ITER(l, &iter, bm, BM_LOOPS_OF_FACE, f) {
			mloopuv = CustomData_bmesh_get_n(&bm->ldata, l->head.data, CD_MLOOPUV, i);
			texface->uv[j][0] = mloopuv->uv[0];
			texface->uv[j][1] = mloopuv->uv[1];

			j++;
		}

	}

	for(i=0; i < numCol; i++){
		mcol = CustomData_get_n(&me->fdata, CD_MCOL, findex, i);

		j = 0;
		BM_ITER(l, &iter, bm, BM_LOOPS_OF_FACE, f) {
			mloopcol = CustomData_bmesh_get_n(&bm->ldata, l->head.data, CD_MLOOPCOL, i);
			mcol[j].r = mloopcol->r;
			mcol[j].g = mloopcol->g;
			mcol[j].b = mloopcol->b;
			mcol[j].a = mloopcol->a;

			j++;
		}
	}
}

/**
 *			BM_data_interp_from_face
 *
 *  projects target onto source, and pulls interpolated customdata from
 *  source.
 * 
 *  Returns -
 *	Nothing
*/
void BM_face_interp_from_face(BMesh *bm, BMFace *target, BMFace *source)
{
	BMLoop *l1, *l2;
	void **blocks=NULL;
	float (*cos)[3]=NULL, *w=NULL;
	BLI_array_staticdeclare(cos, 64);
	BLI_array_staticdeclare(w, 64);
	BLI_array_staticdeclare(blocks, 64);
	
	BM_Copy_Attributes(bm, bm, source, target);
	
	l2 = bm_firstfaceloop(source);
	do {
		BLI_array_growone(cos);
		copy_v3_v3(cos[BLI_array_count(cos)-1], l2->v->co);
		BLI_array_growone(w);
		BLI_array_append(blocks, l2->head.data);
		l2 = l2->next;
	} while (l2 != bm_firstfaceloop(source));

	l1 = bm_firstfaceloop(target);
	do {
		interp_weights_poly_v3(w, cos, source->len, l1->v->co);
		CustomData_bmesh_interp(&bm->ldata, blocks, w, NULL, BLI_array_count(blocks), l1->head.data);
		l1 = l1->next;
	} while (l1 != bm_firstfaceloop(target));

	BLI_array_free(cos);
	BLI_array_free(w);
	BLI_array_free(blocks);
}

/***** multires interpolation*****

mdisps is a grid of displacements, ordered thus:

v1/center -- v4/next -> x
|                 |
|				  |
v2/prev ---- v3/cur
|
V

y
*/
static int compute_mdisp_quad(BMLoop *l, float v1[3], float v2[3], float v3[3], float v4[3], float e1[3], float e2[3])
{
	float cent[3] = {0.0f, 0.0f, 0.0f}, n[3], p[3];
	BMLoop *l2;
	
	/*computer center*/
	l2 = bm_firstfaceloop(l->f);
	do {
		add_v3_v3(cent, l2->v->co);
		l2 = l2->next;
	} while (l2 != bm_firstfaceloop(l->f));
	
	mul_v3_fl(cent, 1.0/(float)l->f->len);
	
	add_v3_v3v3(p, l->prev->v->co, l->v->co);
	mul_v3_fl(p, 0.5);
	add_v3_v3v3(n, l->next->v->co, l->v->co);
	mul_v3_fl(n, 0.5);
	
	copy_v3_v3(v1, cent);
	copy_v3_v3(v2, p);
	copy_v3_v3(v3, l->v->co);
	copy_v3_v3(v4, n);
	
	sub_v3_v3v3(e1, v2, v1);
	sub_v3_v3v3(e2, v3, v4);
	
	return 1;
}


int isect_ray_tri_threshold_v3_uvw(float p1[3], float d[3], float _v0[3], float _v1[3], float _v2[3], float *lambda, float uv[3], float threshold)
{
	float p[3], s[3], e1[3], e2[3], q[3];
	float a, f, u, v;
	float du = 0, dv = 0;
	float v0[3], v1[3], v2[3], c[3];
	
	/*expand triange a bit*/
	cent_tri_v3(c, _v0, _v1, _v2);
	sub_v3_v3v3(v0, _v0, c);
	sub_v3_v3v3(v1, _v1, c);
	sub_v3_v3v3(v2, _v2, c);
	mul_v3_fl(v0, 1.0+threshold);
	mul_v3_fl(v1, 1.0+threshold);
	mul_v3_fl(v2, 1.0+threshold);
	add_v3_v3(v0, c);
	add_v3_v3(v1, c);
	add_v3_v3(v2, c);
	
	sub_v3_v3v3(e1, v1, v0);
	sub_v3_v3v3(e2, v2, v0);
	
	cross_v3_v3v3(p, d, e2);
	a = dot_v3v3(e1, p);
	if ((a > -0.000001) && (a < 0.000001)) return 0;
	f = 1.0f/a;
	
	sub_v3_v3v3(s, p1, v0);
	
	cross_v3_v3v3(q, s, e1);
	*lambda = f * dot_v3v3(e2, q);
	if ((*lambda < 0.0+FLT_EPSILON)) return 0;
	
	u = f * dot_v3v3(s, p);
	v = f * dot_v3v3(d, q);
	
	if (u < 0) du = u;
	if (u > 1) du = u - 1;
	if (v < 0) dv = v;
	if (v > 1) dv = v - 1;
	if (u > 0 && v > 0 && u + v > 1)
	{
		float t = u + v - 1;
		du = u - t/2;
		dv = v - t/2;
	}

	mul_v3_fl(e1, du);
	mul_v3_fl(e2, dv);
	
	if (dot_v3v3(e1, e1) + dot_v3v3(e2, e2) > threshold * threshold)
	{
		return 0;
	}

	if(uv) {
		uv[0]= u;
		uv[1]= v;
		uv[2]= fabs(1.0-u-v);
	}
	
	return 1;
}


/*tl is loop to project onto, sl is loop whose internal displacement, co, is being
  projected.  x and y are location in loop's mdisps grid of co.*/
static int mdisp_in_mdispquad(BMLoop *l, BMLoop *tl, float p[3], float *x, float *y, int res)
{
	float v1[3], v2[3], c[3], co[3], v3[3], v4[3], e1[3], e2[3];
	float w[4], dir[3], uv[4] = {0.0f, 0.0f, 0.0f, 0.0f}, hit[3];
	float lm, eps =  FLT_EPSILON*80;
	int ret=0;
	
	compute_mdisp_quad(tl, v1, v2, v3, v4, e1, e2);
	copy_v3_v3(dir, l->f->no);
	copy_v3_v3(co, dir);
	mul_v3_fl(co, -0.000001);
	add_v3_v3(co, p);
	
	/*four tests, two per triangle, once again normal, once along -normal*/
	ret = isect_ray_tri_threshold_v3_uvw(co, dir, v1, v2, v3, &lm, uv, eps);
	ret = ret || isect_ray_tri_threshold_v3_uvw(co, dir, v1, v3, v4, &lm, uv, eps);
		
	if (!ret) {
		/*now try other direction*/
		negate_v3(dir);
		ret = isect_ray_tri_threshold_v3_uvw(co, dir, v1, v2, v3, &lm, uv, eps);
		ret = ret || isect_ray_tri_threshold_v3_uvw(co, dir, v1, v3, v4, &lm, uv, eps);
	}
		
	if (!ret || isnan(lm) || (uv[0]+uv[1]+uv[2]+uv[3]) < 1.0-FLT_EPSILON*10)
		return 0;
	
	mul_v3_fl(dir, lm);
	add_v3_v3v3(hit, co, dir);

	/*expand quad a bit*/
	cent_quad_v3(c, v1, v2, v3, v4);
	
	sub_v3_v3(v1, c); sub_v3_v3(v2, c);
	sub_v3_v3(v3, c); sub_v3_v3(v4, c);
	mul_v3_fl(v1, 1.0+eps); mul_v3_fl(v2, 1.0+eps);
	mul_v3_fl(v3, 1.0+eps);	mul_v3_fl(v4, 1.0+eps);
	add_v3_v3(v1, c); add_v3_v3(v2, c);
	add_v3_v3(v3, c); add_v3_v3(v4, c);
	
	interp_weights_face_v3(uv, v1, v2, v3, v4, hit);
	
	*x = ((1.0+FLT_EPSILON)*uv[2] + (1.0+FLT_EPSILON)*uv[3])*(float)(res-1);
	*y = ((1.0+FLT_EPSILON)*uv[1] + (1.0+FLT_EPSILON)*uv[2])*(float)(res-1);
	
	return 1;
}

void undo_tangent(MDisps *md, int res, int x, int y, int redo)
{
#if 0
	float co[3], tx[3], ty[3], mat[3][3];
	
	/* construct tangent space matrix */
	grid_tangent(res, 0, x, y, 0, md->disps, tx);
	normalize_v3(tx);

	grid_tangent(res, 0, x, y, 1, md->disps, ty);
	normalize_v3(ty);
	
	column_vectors_to_mat3(mat, tx, ty, no);
	if (redo)
		invert_m3(mat);
	
	mul_v3_m3v3(co, mat, md->disps[y*res+x]);
	copy_v3_v3(md->disps[y*res+x], co);
#endif
}

static void bmesh_loop_interp_mdisps(BMesh *bm, BMLoop *target, BMFace *source)
{
	MDisps *mdisps;
	BMLoop *l2;
	float x, y, d, v1[3], v2[3], v3[3], v4[3] = {0.0f, 0.0f, 0.0f}, e1[3], e2[3], e3[3], e4[3];
	int ix, iy, res;
	
	if (!CustomData_has_layer(&bm->ldata, CD_MDISPS))
		return;
	
	mdisps = CustomData_bmesh_get(&bm->ldata, target->head.data, CD_MDISPS);
	compute_mdisp_quad(target, v1, v2, v3, v4, e1, e2);
	
	/*if no disps data allocate a new grid, the size of the first grid in source.*/
	if (!mdisps->totdisp) {
		MDisps *md2 = CustomData_bmesh_get(&bm->ldata, bm_firstfaceloop(source)->head.data, CD_MDISPS);
		
		mdisps->totdisp = md2->totdisp;
		if (mdisps->totdisp)
			mdisps->disps = BLI_cellalloc_calloc(sizeof(float)*3*mdisps->totdisp, "mdisp->disps in bmesh_loop_intern_mdisps");
		else 
			return;
	}
	
	res = (int)(sqrt(mdisps->totdisp)+0.5f);
	d = 1.0f/(float)(res-1);
	for (x=0.0f, ix=0; ix<res; x += d, ix++) {
		for (y=0.0f, iy=0; iy<res; y+= d, iy++) {
			float co1[3], co2[3], co[3];
			
			copy_v3_v3(co1, e1);
			mul_v3_fl(co1, y);
			add_v3_v3(co1, v1);
			
			copy_v3_v3(co2, e2);
			mul_v3_fl(co2, y);
			add_v3_v3(co2, v4);
			
			sub_v3_v3v3(co, co2, co1);
			mul_v3_fl(co, x);
			add_v3_v3(co, co1);
			
			l2 = bm_firstfaceloop(source);
			do {
				float x2, y2;
				int ix2, iy2;
				MDisps *md1, *md2;

				md1 = CustomData_bmesh_get(&bm->ldata, target->head.data, CD_MDISPS);
				md2 = CustomData_bmesh_get(&bm->ldata, l2->head.data, CD_MDISPS);
				
				if (mdisp_in_mdispquad(target, l2, co, &x2, &y2, res)) {
					int i1, j1;
					float tx[3], mat[3][3], ty[3];
					
					ix2 = (int)x2;
					iy2 = (int)y2;
					
					for (i1=ix2-1; i1<ix2+1; i1++) {
						for (j1=iy2-1; j1<iy2+1; j1++) {
							if (i1 < 0 || i1 >= res) continue;
							if (j1 < 0 || j1 >= res) continue;
							
							undo_tangent(md2, res, i1, j1, 0);
						}
					}

					old_mdisps_bilinear(md1->disps[iy*res+ix], md2->disps, res, x2, y2);
					
					for (i1=ix2-1; i1<ix2+1; i1++) {
						for (j1=iy2-1; j1<iy2+1; j1++) {
							if (i1 < 0 || i1 >= res) continue;
							if (j1 < 0 || j1 >= res) continue;
							
							undo_tangent(md2, res, i1, j1, 1);
						}
					}

				}
				l2 = l2->next;
			} while (l2 != bm_firstfaceloop(source));
		}
	}
}

void BM_multires_smooth_bounds(BMesh *bm, BMFace *f)
{
	BMLoop *l;
	BMIter liter;
	
	return;//XXX
	
	if (!CustomData_has_layer(&bm->ldata, CD_MDISPS))
		return;
	
	BM_ITER(l, &liter, bm, BM_LOOPS_OF_FACE, f) {
		MDisps *mdp = CustomData_bmesh_get(&bm->ldata, l->prev->head.data, CD_MDISPS);
		MDisps *mdl = CustomData_bmesh_get(&bm->ldata, l->head.data, CD_MDISPS);
		MDisps *mdn = CustomData_bmesh_get(&bm->ldata, l->next->head.data, CD_MDISPS);
		float co[3];
		int sides;
		int x, y;
		
		/*****
		mdisps is a grid of displacements, ordered thus:
		
		v1/center -- v4/next -> x
		|                 |
		|				  |
		v2/prev ---- v3/cur
		|
		V
		
		y
		*****/
		  
		sides = sqrt(mdp->totdisp);
		for (y=0; y<sides; y++) {
			add_v3_v3v3(co, mdp->disps[y*sides + sides-1], mdl->disps[y*sides]);
			mul_v3_fl(co, 0.5);

			copy_v3_v3(mdp->disps[y*sides + sides-1], co);
			copy_v3_v3(mdl->disps[y*sides], co);
		}
	}
}

void BM_loop_interp_multires(BMesh *bm, BMLoop *target, BMFace *source)
{
	bmesh_loop_interp_mdisps(bm, target, source);
}

void BM_loop_interp_from_face(BMesh *bm, BMLoop *target, BMFace *source)
{
	BMLoop *l;
	void **blocks=NULL;
	float (*cos)[3]=NULL, *w=NULL, cent[3] = {0.0f, 0.0f, 0.0f};
	BLI_array_staticdeclare(cos, 64);
	BLI_array_staticdeclare(w, 64);
	BLI_array_staticdeclare(blocks, 64);
	int i;
	
	BM_Copy_Attributes(bm, bm, source, target->f);
	
	l = bm_firstfaceloop(source);
	do {
		BLI_array_growone(cos);
		copy_v3_v3(cos[BLI_array_count(cos)-1], l->v->co);
		add_v3_v3(cent, cos[BLI_array_count(cos)-1]);
		
		BLI_array_append(w, 0.0f);
		BLI_array_append(blocks, l->head.data);
		l = l->next;
	} while (l != bm_firstfaceloop(source));

	/*scale source face coordinates a bit, so points sitting directonly on an
      edge will work.*/
	mul_v3_fl(cent, 1.0f/(float)source->len);
	for (i=0; i<source->len; i++) {
		float vec[3];
		sub_v3_v3v3(vec, cent, cos[i]);
		mul_v3_fl(vec, 0.01);
		add_v3_v3(cos[i], vec);
	}
	
	/*interpolate*/
	interp_weights_poly_v3(w, cos, source->len, target->v->co);
	CustomData_bmesh_interp(&bm->ldata, blocks, w, NULL, source->len, target->head.data);
	
	BLI_array_free(cos);
	BLI_array_free(w);
	BLI_array_free(blocks);
	
	if (CustomData_has_layer(&bm->ldata, CD_MDISPS)) {
		bmesh_loop_interp_mdisps(bm, target, source);
	}
}

static void update_data_blocks(BMesh *bm, CustomData *olddata, CustomData *data)
{
	BMIter iter;
	// BLI_mempool *oldpool = olddata->pool;
	void *block;

	CustomData_bmesh_init_pool(data, data==&bm->ldata ? 2048 : 512);

	if (data == &bm->vdata) {
		BMVert *eve;
		
		BM_ITER(eve, &iter, bm, BM_VERTS_OF_MESH, NULL) {
			block = NULL;
			CustomData_bmesh_set_default(data, &block);
			CustomData_bmesh_copy_data(olddata, data, eve->head.data, &block);
			CustomData_bmesh_free_block(olddata, &eve->head.data);
			eve->head.data= block;
		}
	}
	else if (data == &bm->edata) {
		BMEdge *eed;

		BM_ITER(eed, &iter, bm, BM_EDGES_OF_MESH, NULL) {
			block = NULL;
			CustomData_bmesh_set_default(data, &block);
			CustomData_bmesh_copy_data(olddata, data, eed->head.data, &block);
			CustomData_bmesh_free_block(olddata, &eed->head.data);
			eed->head.data= block;
		}
	}
	else if (data == &bm->pdata || data == &bm->ldata) {
		BMIter liter;
		BMFace *efa;
		BMLoop *l;

		BM_ITER(efa, &iter, bm, BM_FACES_OF_MESH, NULL) {
			if (data == &bm->pdata) {
				block = NULL;
				CustomData_bmesh_set_default(data, &block);
				CustomData_bmesh_copy_data(olddata, data, efa->head.data, &block);
				CustomData_bmesh_free_block(olddata, &efa->head.data);
				efa->head.data= block;
			}

			if (data == &bm->ldata) {
				BM_ITER(l, &liter, bm, BM_LOOPS_OF_FACE, efa) {
					block = NULL;
					CustomData_bmesh_set_default(data, &block);
					CustomData_bmesh_copy_data(olddata, data, l->head.data, &block);
					CustomData_bmesh_free_block(olddata, &l->head.data);
					l->head.data= block;
				}
			}
		}
	}
}


void BM_add_data_layer(BMesh *bm, CustomData *data, int type)
{
	CustomData olddata;

	olddata= *data;
	olddata.layers= (olddata.layers)? MEM_dupallocN(olddata.layers): NULL;
	CustomData_add_layer(data, type, CD_DEFAULT, NULL, 0);

	update_data_blocks(bm, &olddata, data);
	if (olddata.layers) MEM_freeN(olddata.layers);
}

void BM_add_data_layer_named(BMesh *bm, CustomData *data, int type, char *name)
{
	CustomData olddata;

	olddata= *data;
	olddata.layers= (olddata.layers)? MEM_dupallocN(olddata.layers): NULL;
	CustomData_add_layer_named(data, type, CD_DEFAULT, NULL, 0, name);

	update_data_blocks(bm, &olddata, data);
	if (olddata.layers) MEM_freeN(olddata.layers);
}

void BM_free_data_layer(BMesh *bm, CustomData *data, int type)
{
	CustomData olddata;

	olddata= *data;
	olddata.layers= (olddata.layers)? MEM_dupallocN(olddata.layers): NULL;
	CustomData_free_layer_active(data, type, 0);

	update_data_blocks(bm, &olddata, data);
	if (olddata.layers) MEM_freeN(olddata.layers);
}

float BM_GetCDf(CustomData *cd, void *element, int type)
{
	if (CustomData_has_layer(cd, type)) {
		float *f = CustomData_bmesh_get(cd, ((BMHeader*)element)->data, type);
		return *f;
	}

	return 0.0;
}

void BM_SetCDf(CustomData *cd, void *element, int type, float val)
{
	if (CustomData_has_layer(cd, type)) {
		float *f = CustomData_bmesh_get(cd, ((BMHeader*)element)->data, type);
		*f = val;
	}

	return;
}
