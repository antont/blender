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
 * Contributor(s): Blender Foundation.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <stdlib.h>

#include "MEM_guardedalloc.h"

#include "DNA_material_types.h"
#include "DNA_meshdata_types.h"

#include "BLI_listbase.h"
#include "BLI_math.h"
#include "BLI_memarena.h"
#include "BLI_rand.h"
#include "BLI_threads.h"

#include "BKE_DerivedMesh.h"
#include "BKE_global.h"

#include "cache.h"
#include "diskocclusion.h"
#include "material.h"
#include "object_strand.h"
#include "part.h"
#include "raytrace.h"
#include "rendercore.h"
#include "render_types.h"
#include "shading.h"
#include "sss.h"

/******************************** Utilities **********************************/

static int mat_need_cache(Render *re, Material *ma)
{
	if(ma->mode & MA_SHLESS)
		return 0;
	else if(ma->amb == 0.0f && !(ma->mapto & MAP_AMB))
		return 0;
	else if(!re->db.sss_pass && mat_has_only_sss(ma))
		return 0;
	
	return 1;
}

/******************************* Pixel Cache *********************************/

#define CACHE_STEP 3

static PixelCacheSample *find_sample(PixelCache *cache, int x, int y)
{
	x -= cache->x;
	y -= cache->y;

	x /= cache->step;
	y /= cache->step;
	x *= cache->step;
	y *= cache->step;

	if(x < 0 || x >= cache->w || y < 0 || y >= cache->h)
		return NULL;
	else
		return &cache->sample[y*cache->w + x];
}

int pixel_cache_sample(PixelCache *cache, ShadeInput *shi)
{
	PixelCacheSample *samples[4], *sample;
	float *co= shi->geometry.co;
	float *n= shi->geometry.vn;
	int x= shi->geometry.xs;
	int y= shi->geometry.ys;
	float *ao= shi->shading.ao;
	float *env= shi->shading.env;
	float *indirect= shi->shading.indirect;
	float wn[4], wz[4], wb[4], tx, ty, w, totw, mino, maxo;
	float d[3], dist2;
	int i, x1, y1, x2, y2;

	/* first try to find a sample in the same pixel */
	if(cache->sample && cache->step) {
		sample= &cache->sample[(y-cache->y)*cache->w + (x-cache->x)];
		if(sample->filled) {
			sub_v3_v3v3(d, sample->co, co);
			dist2= dot_v3v3(d, d);
			if(dist2 < 0.5f*sample->dist2 && dot_v3v3(sample->n, n) > 0.98f) {
				copy_v3_v3(ao, sample->ao);
				copy_v3_v3(env, sample->env);
				copy_v3_v3(indirect, sample->indirect);
				return 1;
			}
		}
	}
	else {
		return 0;
	}

	/* try to interpolate between 4 neighbouring pixels */
	samples[0]= find_sample(cache, x, y);
	samples[1]= find_sample(cache, x+cache->step, y);
	samples[2]= find_sample(cache, x, y+cache->step);
	samples[3]= find_sample(cache, x+cache->step, y+cache->step);

	for(i=0; i<4; i++)
		if(!samples[i] || !samples[i]->filled) {
			return 0;
		}

	/* require intensities not being too different */
	mino= MIN4(samples[0]->intensity, samples[1]->intensity, samples[2]->intensity, samples[3]->intensity);
	maxo= MAX4(samples[0]->intensity, samples[1]->intensity, samples[2]->intensity, samples[3]->intensity);

	if(maxo - mino > 0.05f) {
		return 0;
	}

	/* compute weighted interpolation between samples */
	zero_v3(ao);
	zero_v3(env);
	zero_v3(indirect);
	totw= 0.0f;

	x1= samples[0]->x;
	y1= samples[0]->y;
	x2= samples[3]->x;
	y2= samples[3]->y;

	tx= (float)(x2 - x)/(float)(x2 - x1);
	ty= (float)(y2 - y)/(float)(y2 - y1);

	wb[3]= (1.0f-tx)*(1.0f-ty);
	wb[2]= (tx)*(1.0f-ty);
	wb[1]= (1.0f-tx)*(ty);
	wb[0]= tx*ty;

	for(i=0; i<4; i++) {
		sub_v3_v3v3(d, samples[i]->co, co);
		dist2= dot_v3v3(d, d);

		wz[i]= 1.0f; //(samples[i]->dist2/(1e-4f + dist2));
		wn[i]= pow(dot_v3v3(samples[i]->n, n), 32.0f);

		w= wb[i]*wn[i]*wz[i];

		totw += w;
		madd_v3_v3fl(ao, samples[i]->ao, w);
		madd_v3_v3fl(env, samples[i]->env, w);
		madd_v3_v3fl(indirect, samples[i]->indirect, w);
	}

	if(totw >= 0.9f) {
		totw= 1.0f/totw;
		mul_v3_fl(ao, totw);
		mul_v3_fl(env, totw);
		mul_v3_fl(indirect, totw);
		return 1;
	}

	return 0;
}

PixelCache *pixel_cache_create(Render *re, RenderPart *pa, ShadeSample *ssamp)
{
	PixStr ps, **rd= NULL;
	PixelCache *cache;
	PixelCacheSample *sample;
	ShadeInput *shi;
	int *ro=NULL, *rp=NULL, *rz=NULL;
	int x, y, step = CACHE_STEP, offs;

	cache= MEM_callocN(sizeof(PixelCache), "PixelCache");
	cache->w= pa->rectx;
	cache->h= pa->recty;
	cache->x= pa->disprect.xmin;
	cache->y= pa->disprect.ymin;
	cache->step= step;
	cache->sample= MEM_callocN(sizeof(PixelCacheSample)*cache->w*cache->h, "PixelCacheSample");
	sample= cache->sample;

	if(re->params.osa) {
		rd= pa->rectdaps;
	}
	else {
		/* fake pixel struct for non-osa */
		ps.next= NULL;
		ps.mask= 0xFFFF;

		ro= pa->recto;
		rp= pa->rectp;
		rz= pa->rectz;
	}

	/* compute a sample at every step pixels */
	offs= 0;

	for(y=pa->disprect.ymin; y<pa->disprect.ymax; y++) {
		for(x=pa->disprect.xmin; x<pa->disprect.xmax; x++, sample++, rd++, ro++, rp++, rz++, offs++) {
			PixelRow *row= pa->pixelrow;
			int totrow;

			if(!(((x - pa->disprect.xmin + step) % step) == 0 || x == pa->disprect.xmax-1))
				continue;
			if(!(((y - pa->disprect.ymin + step) % step) == 0 || y == pa->disprect.ymax-1))
				continue;

			totrow= pixel_row_fill(row, re, pa, offs);
			shade_samples_from_pixel(re, ssamp, &row[0], x, y);

			shi= ssamp->shi;
			if(shi->primitive.vlr && mat_need_cache(re, shi->material.mat)) {
				disk_occlusion_sample_direct(re, shi);

				copy_v3_v3(sample->co, shi->geometry.co);
				copy_v3_v3(sample->n, shi->geometry.vno);
				copy_v3_v3(sample->ao, shi->shading.ao);
				copy_v3_v3(sample->env, shi->shading.env);
				copy_v3_v3(sample->indirect, shi->shading.indirect);
				sample->intensity= MAX3(sample->ao[0], sample->ao[1], sample->ao[2]);
				sample->intensity= MAX2(sample->intensity, MAX3(sample->env[0], sample->env[1], sample->env[2]));
				sample->intensity= MAX2(sample->intensity, MAX3(sample->indirect[0], sample->indirect[1], sample->indirect[2]));
				sample->dist2= dot_v3v3(shi->geometry.dxco, shi->geometry.dxco) + dot_v3v3(shi->geometry.dyco, shi->geometry.dyco);
				sample->x= shi->geometry.xs;
				sample->y= shi->geometry.ys;
				sample->filled= 1;
			}

			if(re->cb.test_break(re->cb.tbh))
				break;
		}
	}

	return cache;
}

void pixel_cache_free(PixelCache *cache)
{
	if(cache->sample)
		MEM_freeN(cache->sample);

	MEM_freeN(cache);
}

void pixel_cache_insert_sample(PixelCache *cache, ShadeInput *shi)
{
	PixelCacheSample *sample;

	if(!(cache->sample && cache->step))
		return;

	sample= &cache->sample[(shi->geometry.ys-cache->y)*cache->w + (shi->geometry.xs-cache->x)];
	copy_v3_v3(sample->co, shi->geometry.co);
	copy_v3_v3(sample->n, shi->geometry.vno);
	copy_v3_v3(sample->ao, shi->shading.ao);
	copy_v3_v3(sample->env, shi->shading.env);
	copy_v3_v3(sample->indirect, shi->shading.indirect);
	sample->intensity= MAX3(sample->ao[0], sample->ao[1], sample->ao[2]);
	sample->intensity= MAX2(sample->intensity, MAX3(sample->env[0], sample->env[1], sample->env[2]));
	sample->intensity= MAX2(sample->intensity, MAX3(sample->indirect[0], sample->indirect[1], sample->indirect[2]));
	sample->dist2= dot_v3v3(shi->geometry.dxco, shi->geometry.dxco) + dot_v3v3(shi->geometry.dyco, shi->geometry.dyco);
	sample->filled= 1;
}

/******************************* Surface Cache ********************************/

SurfaceCache *surface_cache_create(Render *re, ObjectRen *obr, DerivedMesh *dm, float mat[][4], int timeoffset)
{
	SurfaceCache *cache;
	MFace *mface;
	MVert *mvert;
	float (*co)[3];
	int a, totvert, totface;

	totvert= dm->getNumVerts(dm);
	totface= dm->getNumFaces(dm);

	for(cache=re->db.surfacecache.first; cache; cache=cache->next)
		if(cache->obr.ob == obr->ob && cache->obr.par == obr->par
			&& cache->obr.index == obr->index && cache->totvert==totvert && cache->totface==totface)
			break;

	if(!cache) {
		cache= MEM_callocN(sizeof(SurfaceCache), "SurfaceCache");
		cache->obr= *obr;
		cache->totvert= totvert;
		cache->totface= totface;
		cache->face= MEM_callocN(sizeof(int)*4*cache->totface, "StrandSurfFaces");
		cache->ao= MEM_callocN(sizeof(float)*3*cache->totvert, "StrandSurfAO");
		cache->env= MEM_callocN(sizeof(float)*3*cache->totvert, "StrandSurfEnv");
		cache->indirect= MEM_callocN(sizeof(float)*3*cache->totvert, "StrandSurfIndirect");
		BLI_addtail(&re->db.surfacecache, cache);
	}

	if(timeoffset == -1 && !cache->prevco)
		cache->prevco= co= MEM_callocN(sizeof(float)*3*cache->totvert, "StrandSurfCo");
	else if(timeoffset == 0 && !cache->co)
		cache->co= co= MEM_callocN(sizeof(float)*3*cache->totvert, "StrandSurfCo");
	else if(timeoffset == 1 && !cache->nextco)
		cache->nextco= co= MEM_callocN(sizeof(float)*3*cache->totvert, "StrandSurfCo");
	else
		return cache;

	mvert= dm->getVertArray(dm);
	for(a=0; a<cache->totvert; a++, mvert++) {
		copy_v3_v3(co[a], mvert->co);
		mul_m4_v3(mat, co[a]);
	}

	mface= dm->getFaceArray(dm);
	for(a=0; a<cache->totface; a++, mface++) {
		cache->face[a][0]= mface->v1;
		cache->face[a][1]= mface->v2;
		cache->face[a][2]= mface->v3;
		cache->face[a][3]= mface->v4;
	}

	return cache;
}

void surface_cache_free(RenderDB *rdb)
{
	SurfaceCache *cache;

	for(cache=rdb->surfacecache.first; cache; cache=cache->next) {
		if(cache->co) MEM_freeN(cache->co);
		if(cache->prevco) MEM_freeN(cache->prevco);
		if(cache->nextco) MEM_freeN(cache->nextco);
		if(cache->ao) MEM_freeN(cache->ao);
		if(cache->env) MEM_freeN(cache->env);
		if(cache->indirect) MEM_freeN(cache->indirect);
		if(cache->face) MEM_freeN(cache->face);
	}

	BLI_freelistN(&rdb->surfacecache);
}

void surface_cache_sample(SurfaceCache *cache, ShadeInput *shi)
{
	StrandRen *strand= shi->primitive.strand;
	int *face, *index = render_strand_get_face(shi->primitive.obr, strand, 0);
	float w[4], *co1, *co2, *co3, *co4;

	if(cache && cache->face && cache->co && cache->ao && index) {
		face= cache->face[*index];

		co1= cache->co[face[0]];
		co2= cache->co[face[1]];
		co3= cache->co[face[2]];
		co4= (face[3])? cache->co[face[3]]: NULL;

		interp_weights_face_v3(w, co1, co2, co3, co4, strand->vert->co);

		zero_v3(shi->shading.ao);
		zero_v3(shi->shading.env);
		zero_v3(shi->shading.indirect);

		madd_v3_v3fl(shi->shading.ao, cache->ao[face[0]], w[0]);
		madd_v3_v3fl(shi->shading.env, cache->env[face[0]], w[0]);
		madd_v3_v3fl(shi->shading.indirect, cache->indirect[face[0]], w[0]);
		madd_v3_v3fl(shi->shading.ao, cache->ao[face[1]], w[1]);
		madd_v3_v3fl(shi->shading.env, cache->env[face[1]], w[1]);
		madd_v3_v3fl(shi->shading.indirect, cache->indirect[face[1]], w[1]);
		madd_v3_v3fl(shi->shading.ao, cache->ao[face[2]], w[2]);
		madd_v3_v3fl(shi->shading.env, cache->env[face[2]], w[2]);
		madd_v3_v3fl(shi->shading.indirect, cache->indirect[face[2]], w[2]);
		if(face[3]) {
			madd_v3_v3fl(shi->shading.ao, cache->ao[face[3]], w[3]);
			madd_v3_v3fl(shi->shading.env, cache->env[face[3]], w[3]);
			madd_v3_v3fl(shi->shading.indirect, cache->indirect[face[3]], w[3]);
		}
	}
	else {
		shi->shading.ao[0]= 1.0f;
		shi->shading.ao[1]= 1.0f;
		shi->shading.ao[2]= 1.0f;
		zero_v3(shi->shading.env);
		zero_v3(shi->shading.indirect);
	}
}

/******************************* Irradiance Cache ********************************/

/* Initial implementation based on Pixie (Copyright © 1999 - 2010, Okan Arikan) */

/* Relevant papers:
   [1] A Ray Tracing Solution for Diffuse Interreflection.
       Ward, Rubinstein, Clear. 1988.
   [2] Irradiance Gradients.
       Ward, Heckbert. 1992.
   [3] Approximate Global Illumination System for Computer Generated Films.
       Tabellion, Lamorlette. 2004.
   [4] Making Radiance and Irradiance Caching Practical: Adaptive Caching
       and Neighbor Clamping. Křivánek, Bouatouch, Pattanaik, Žára. 2006. */

#define EPSILON					1e-6f
#define MAX_PIXEL_DIST			10.0f
#define MAX_ERROR_K				1.0f
#define WEIGHT_NORMAL_DENOM		65.823047821929777f		/* 1/(1 - cos(10°)) */
#define SINGULAR_VALUE_EPSILON	1e-4f
#define MAX_CACHE_DIMENSION		((3+3+1)*4)

/* Data Structures */

typedef struct IrrCacheSample {
	/* cache sample in a node */
	struct IrrCacheSample *next;

	float P[3];						/* position */
	float N[3];						/* normal */
	float dP;						/* radius */
	int read;						/* read from file */
} IrrCacheSample;

typedef struct IrrCacheNode {
	/* node in the cache tree */
	float center[3];					/* center of node */
	float side;							/* max side length */
	IrrCacheSample *samples;			/* samples in the node */
	struct IrrCacheNode *children[8];	/* child nodes */
} IrrCacheNode;

typedef struct Lsq4DFit {
	float (*A)[4];
	float (*B)[MAX_CACHE_DIMENSION];
	float (*w);
	int tot, alloc;
} Lsq4DFit;

struct IrrCache {
	/* irradiance cache */
	IrrCacheNode root;			/* root node of the tree */
	int maxdepth;				/* maximum tree dist */

	IrrCacheNode **stack;		/* stack for traversal */
	int stacksize;				/* stack size */

	MemArena *arena;			/* memory arena for nodes and samples */
	int thread;					/* thread owning the cache */

	int totsample;
	int totnode;
	int totlookup;
	int totpost;

	/* options */
	int neighbour_clamp;
	int lsq_reconstruction;
	int locked;
	int dimension;
	int use_light_dir;
	
	/* least squares reconstruction */
	Lsq4DFit lsq;

	/* test: a stable global coordinate system may help */
};

/* Least Squares fit to hyperplane for better interpolation of samples, this
   helps especially at tile borders or visibility boundaries, as it allows us
   to extrapolate rather than just interpolate. */

void lsq_4D_alloc(Lsq4DFit *lsq)
{
	lsq->alloc = 25;
	lsq->A = MEM_callocN(sizeof(*lsq->A)*lsq->alloc, "lsq->A");
	lsq->B = MEM_callocN(sizeof(*lsq->B)*lsq->alloc, "lsq->B");
	lsq->w = MEM_callocN(sizeof(*lsq->w)*lsq->alloc, "lsq->w");
}

void lsq_4D_free(Lsq4DFit *lsq)
{
	MEM_freeN(lsq->A);
	MEM_freeN(lsq->B);
	MEM_freeN(lsq->w);
}

void lsq_4D_init(Lsq4DFit *lsq)
{
	lsq->tot = 0;
}

void lsq_4D_add(Lsq4DFit *lsq, float a[4], float *b, int dimension, float weight)
{
	int i;

	if(lsq->tot >= lsq->alloc) {
		lsq->alloc *= 2;
		lsq->A = MEM_reallocN(lsq->A, sizeof(*lsq->A)*lsq->alloc);
		lsq->B = MEM_reallocN(lsq->B, sizeof(*lsq->B)*lsq->alloc);
		lsq->w = MEM_reallocN(lsq->w, sizeof(*lsq->w)*lsq->alloc);
	}

	copy_v4_v4(lsq->A[lsq->tot], a);
	for(i=0; i<dimension; i++)
		lsq->B[lsq->tot][i] = b[i];
	lsq->w[lsq->tot] = weight;
	lsq->tot++;
}

void lsq_4D_solve(Lsq4DFit *lsq, float *solution, int dimension)
{
	float AtA[4][4], AtAinv[4][4], AtB[MAX_CACHE_DIMENSION][4];
	float x[4], meanB[MAX_CACHE_DIMENSION], totw;
	float (*A)[4]= lsq->A, (*B)[MAX_CACHE_DIMENSION] = lsq->B, *w = lsq->w;
	int i, j, k;

	memset(AtA, 0, sizeof(AtA));
	memset(AtB, 0, sizeof(AtB));
	memset(meanB, 0, sizeof(meanB));
	totw = 0.0f;

	/* computed weighted mean B, to do least squares on (B-meanB),
	   this works better on underdetermined systems where lsq will
	   minimize ||x||, which is not what we want */
	for(k=0; k<lsq->tot; k++) {
		for(i=0; i<dimension; i++)
			meanB[i] += B[k][i]*w[k];
		totw += w[k];
	}

	for(i=0; i<dimension; i++)
		meanB[i] /= totw;

	/* build AtA and AtB from A and B */
	for(k=0; k<lsq->tot; k++) {
		for(i=0; i<4; i++)
			for(j=0; j<4; j++)
				AtA[i][j] += w[k]*A[k][i]*A[k][j];
		
		for(i=0; i<dimension; i++)
			for(j=0; j<4; j++)
				AtB[i][j] += w[k]*A[k][j]*(B[k][i] - meanB[i]);

	}

	/* use SVD for stable inverting of AtA */
	pseudoinverse_m4_m4(AtAinv, AtA, SINGULAR_VALUE_EPSILON);

	for(i=0; i<dimension; i++) {
		mul_v4_m4v4(x, AtAinv, AtB[i]);
		solution[i]= meanB[i] + x[3];
	}
}

/* Create and Free */

static IrrCache *irr_cache_new(Render *re, int thread)
{
	IrrCache *cache;
	float bb[2][3], viewbb[2][3];

	cache= MEM_callocN(sizeof(IrrCache), "IrrCache");
	cache->thread= thread;
	cache->maxdepth= 1;

	cache->arena= BLI_memarena_new(BLI_MEMARENA_STD_BUFSIZE, "irr cache arena");
	BLI_memarena_use_calloc(cache->arena);

	/* initialize root node with bounds */
	render_instances_bound(&re->db, viewbb);
	INIT_MINMAX(bb[0], bb[1]);
	box_minmax_bounds_m4(bb[0], bb[1], viewbb, re->cam.viewinv);
	mid_v3_v3v3(cache->root.center, bb[0], bb[1]);
	cache->root.side= MAX3(bb[1][0]-bb[0][0], bb[1][1]-bb[0][1], bb[1][2]-bb[0][2]);

	/* allocate stack */
	cache->stacksize= 10;
	cache->stack= MEM_mallocN(sizeof(IrrCacheNode*)*cache->stacksize*8, "IrrCache stack");

	/* options */
	cache->lsq_reconstruction= 1;
	cache->neighbour_clamp= 1;
	
	if(cache->lsq_reconstruction)
		lsq_4D_alloc(&cache->lsq);

	if(re->db.wrld.mode & WO_AMB_OCC)
		cache->dimension++;
	if(re->db.wrld.mode & WO_ENV_LIGHT)
		cache->dimension += 3;
	if(re->db.wrld.mode & WO_INDIRECT_LIGHT)
		cache->dimension += 3;

	if(re->db.wrld.ao_shading_method == WO_LIGHT_SHADE_ONCE)
		cache->dimension *= 4;

	return cache;
}

static void irr_cache_delete(IrrCache *cache)
{
	if(G.f & G_DEBUG)
		printf("irr cache stats: %d samples, %d lookups, post %d, %.4f.\n", cache->totsample, cache->totlookup, cache->totpost, (float)cache->totsample/(float)cache->totlookup);

	if(cache->lsq_reconstruction)
		lsq_4D_free(&cache->lsq);

	BLI_memarena_free(cache->arena);
	MEM_freeN(cache->stack);
	MEM_freeN(cache);
}

void irr_cache_check_stack(IrrCache *cache)
{
	/* increase stack size as more nodes are added */
	if(cache->maxdepth > cache->stacksize) {
		cache->stacksize= cache->maxdepth + 5;
		if(cache->stack)
			MEM_freeN(cache->stack);
		cache->stack= MEM_mallocN(sizeof(IrrCacheNode*)*cache->stacksize*8, "IrrCache stack");
	}
}

static int irr_cache_node_point_inside(IrrCacheNode *node, float scale, float add, float P[3])
{
	float side= node->side*scale + add;

	return len_squared_v3v3(node->center, P) <= side*side;
}

static float *irr_sample_C(IrrCacheSample *sample)
{
	return (float*)((char*)sample + sizeof(IrrCacheSample));
}

static void irr_sample_set(Render *re, IrrCacheSample *sample, float *ao, float env[3], float indirect[3], float ldir_ao[3], float ldir_env[3][3], float ldir_indirect[3][3])
{
	float *C= irr_sample_C(sample);
	int method= re->db.wrld.ao_shading_method;

	/* pack colors and light directions into a single array */

	if(ao) {
		*C= *ao;
		C++;

		if(method == WO_LIGHT_SHADE_ONCE) {
			copy_v3_v3(C, ldir_ao);
			C += 3;
		}
	}

	if(env) {
		copy_v3_v3(C, env);
		C += 3;

		if(method == WO_LIGHT_SHADE_ONCE) {
			memcpy(C, ldir_env, sizeof(float)*3*3);
			C += 3*3;
		}
	}

	if(indirect) {
		copy_v3_v3(C, indirect);
		C += 3;

		if(method == WO_LIGHT_SHADE_ONCE) {
			memcpy(C, ldir_indirect, sizeof(float)*3*3);
			C += 3*3;
		}
	}
}

static void irr_sample_eval(Render *re, ShadeInput *shi, float *C, float *ao, float env[3], float indirect[3])
{
	float *dir_ao= NULL, (*dir_env)[3]= NULL, (*dir_indirect)[3]= NULL;
	int method= re->db.wrld.ao_shading_method;

	if(ao) {
		*ao= *C;
		C++;

		if(method == WO_LIGHT_SHADE_ONCE) {
			dir_ao= C;
			C += 3;
		}
	}

	if(env) {
		copy_v3_v3(env, C);
		C += 3;

		if(method == WO_LIGHT_SHADE_ONCE) {
			dir_env = (float(*)[3])C;
			C += 3*3;
		}
	}

	if(indirect) {
		copy_v3_v3(indirect, C);
		C += 3;

		if(method == WO_LIGHT_SHADE_ONCE) {
			dir_indirect = (float(*)[3])C;
			C += 3*3;
		}
	}

	ray_cache_post_apply(re, shi, 
		ao, env, indirect, dir_ao, dir_env, dir_indirect);
}

/* Neighbour Clamping [Křivánek 2006] */

static void irr_cache_clamp_sample(IrrCache *cache, IrrCacheSample *nsample)
{
	IrrCacheSample *sample;
	IrrCacheNode *node, **stack, **stacknode;
	float P[3];
	int i;

	copy_v3_v3(P, nsample->P);

	stack= cache->stack;
	stacknode= stack;

	*stacknode++= &cache->root;
	while(stacknode > stack) {
		node= *(--stacknode);

		/* sum the values in this level */
		for(sample=node->samples; sample; sample=sample->next) {
			float l;

			/* avoid issues with coincident points */
			l= len_squared_v3v3(sample->P, P);
			l= (l > EPSILON)? sqrtf(l): EPSILON;

			nsample->dP= minf(nsample->dP, sample->dP + l);
			sample->dP= minf(sample->dP, nsample->dP + l);
		}

		/* check the children */
		for(i=0; i<8; i++) {
			IrrCacheNode *tnode= node->children[i];

			if(tnode && irr_cache_node_point_inside(tnode, 4.0f, 0.0f, P))
				*stacknode++= tnode;
		}
	}
}

/* Insert Sample */

static void irr_cache_insert(IrrCache *cache, IrrCacheSample *sample)
{
	IrrCacheNode *node;
	float dist, P[3];
	int i, j, depth;

	copy_v3_v3(P, sample->P);

	/* error multiplier */
	dist = sample->dP/MAX_ERROR_K;
	
	/* insert the new sample into the cache */
	node= &cache->root;
	depth= 0;
	while(node->side > (2*dist)) {
		depth++;

		j= 0;
		for(i=0; i<3; i++)
			if(P[i] > node->center[i])
				j |= 1 << i;

		if(node->children[j] == NULL) {
			IrrCacheNode *nnode= BLI_memarena_alloc(cache->arena, sizeof(IrrCacheNode));

			for(i=0; i<3; i++) {
				float fac= (P[i] > node->center[i])? 0.25f: -0.25f;
				nnode->center[i]= node->center[i] + fac*node->side;
			}

			nnode->side= node->side*0.5f;
			node->children[j]= nnode;

			cache->totnode++;
		}

		node= node->children[j];
	}

	sample->next= node->samples;
	node->samples= sample;

	cache->maxdepth= MAX2(depth, cache->maxdepth);
	irr_cache_check_stack(cache);

	cache->totsample++;
}

/* Add a Sample */

static void irr_cache_add(Render *re, ShadeInput *shi, IrrCache *cache, float *ao, float env[3], float indirect[3], float P[3], float dPdu[3], float dPdv[3], float N[3], float bumpN[3], int preprocess)
{
	IrrCacheSample *sample;
	float Rmean, ldir_ao[3], ldir_env[3][3], ldir_indirect[3][3];
	int method= re->db.wrld.ao_shading_method;
	
	/* do raytracing */

	if(method == WO_LIGHT_SHADE_FULL && preprocess && !shi->primitive.strand) {
		/* for full shading, we need material & textures */
		shade_input_init_material(re, shi);
		shade_input_set_shade_texco(re, shi);
		mat_shading_begin(re, shi, &shi->material, 1);
	}

	ray_ao_env_indirect(re, shi, ao, env, indirect, ldir_ao, ldir_env, ldir_indirect, &Rmean, 1);

	if(method == WO_LIGHT_SHADE_FULL)
		mat_shading_end(re, &shi->material);

	/* save sample to cache? */
	if(!1) ///*(MAX_ERROR != 0) &&*/ (*ao > EPSILON)) { // XXX?
		return;

	sample= BLI_memarena_alloc(cache->arena, sizeof(IrrCacheSample) + sizeof(float)*cache->dimension);

	// XXX too large? try setting pixel dist to 0!
	/* compute the radius of validity [Tabellion 2004] */
	if(re->db.wrld.aomode & WO_LIGHT_CACHE_FILE)
		Rmean= Rmean*0.5f; /* XXX pixel dist is not reusable .. */
	else
		Rmean= minf(Rmean*0.5f, MAX_PIXEL_DIST*(len_v3(dPdu) + len_v3(dPdv))*0.5f);
	
	/* record the data */
	copy_v3_v3(sample->P, P);
	copy_v3_v3(sample->N, N);
	sample->dP= Rmean;

	/* copy color values */
	if(method != WO_LIGHT_SHADE_ONCE) {
		irr_sample_set(re, sample, ao, env, indirect, NULL, NULL, NULL);
	}
	else {
#if 0
		if(ao) mul_m3_v3(re->cam.viewninv, ldir_ao);
		if(env) {
			mul_m3_v3(re->cam.viewninv, ldir_env[0]);
			mul_m3_v3(re->cam.viewninv, ldir_env[1]);
			mul_m3_v3(re->cam.viewninv, ldir_env[2]);
		}
		if(indirect) {
			mul_m3_v3(re->cam.viewninv, ldir_indirect[0]);
			mul_m3_v3(re->cam.viewninv, ldir_indirect[1]);
			mul_m3_v3(re->cam.viewninv, ldir_indirect[2]);
		}
#endif

		irr_sample_set(re, sample, ao, env, indirect, ldir_ao, ldir_env, ldir_indirect);
	}

	irr_sample_eval(re, shi, irr_sample_C(sample), ao, env, indirect);

	/* neighbour clamping trick */
	if(cache->neighbour_clamp) {
		irr_cache_clamp_sample(cache, sample);
		Rmean= sample->dP; /* copy dP back so we get the right place in the octree */
	}

	/* insert into tree */
	irr_cache_insert(cache, sample);
}

/* Lookup */

int irr_cache_lookup(Render *re, ShadeInput *shi, IrrCache *cache, float *ao, float env[3], float indirect[3], float cP[3], float cdPdu[3], float cdPdv[3], float cN[3], float cbumpN[3], int preprocess)
{
	IrrCacheSample *sample;
	IrrCacheNode *node, **stack, **stacknode;
	float accum[MAX_CACHE_DIMENSION], P[3], dPdu[3], dPdv[3], N[3], bumpN[3], totw;
	float discard_weight, maxdist, distfac;
	int i, added= 0, totfound= 0, use_lsq, dimension;
	Lsq4DFit *lsq= &cache->lsq;

	/* a small value for discard-smoothing of irradiance */
	discard_weight= (preprocess)? 0.1f: 0.0f;

	/* transform the lookup point to the correct coordinate system */
	copy_v3_v3(P, cP);
	copy_v3_v3(dPdu, cdPdu);
	copy_v3_v3(dPdv, cdPdv);

	/* normals depend on how we do bump mapping */
	if(re->db.wrld.ao_shading_method == WO_LIGHT_SHADE_NONE) {
		copy_v3_v3(N, cN);
		cbumpN= NULL;
	}
	else if(re->db.wrld.ao_shading_method == WO_LIGHT_SHADE_ONCE) {
		copy_v3_v3(N, cN);
		copy_v3_v3(bumpN, cbumpN);
	}
	else {
		copy_v3_v3(N, cbumpN);
		cbumpN= NULL;
	}

	negate_v3(N);
	if(cbumpN) negate_v3(bumpN); /* blender normal is negated */

	mul_m4_v3(re->cam.viewinv, P);
	mul_m3_v3(re->cam.viewninv, dPdu);
	mul_m3_v3(re->cam.viewninv, dPdv);
	mul_m3_v3(re->cam.viewninv, N);
	if(cbumpN)
		mul_m3_v3(re->cam.viewninv, bumpN);

	//print_m3("nm", re->cam.viewnmat);
	
	totw= 0.0f;

	/* setup tree traversal */
	stack= cache->stack;
	stacknode= stack;
	*stacknode++= &cache->root;

	use_lsq= cache->lsq_reconstruction;
	dimension= cache->dimension;

	if(use_lsq)
		lsq_4D_init(lsq);
	else
		memset(accum, 0, sizeof(accum));

	/* the motivation for this factor is that in preprocess we only require
	   one sample for lookup not to fail, whereas for least squares
	   reconstruction we need more samples for a proper reconstruction. it's
	   quite arbitrary though and it would be good to have an actual
	   guarantee that we have enough samples for reconstruction */
	distfac= (preprocess)? 1.0f: 2.0f;

	maxdist= sqrtf(len_v3(dPdu)*len_v3(dPdv))*0.5f;
	maxdist *= MAX_PIXEL_DIST*distfac;

	while(stacknode > stack) {
		node= *(--stacknode);

		/* sum the values in this level */
		for(sample=node->samples; sample; sample=sample->next) {
			float D[3], a, e1, e2, w, dist;

			/* D vector from sample to query point */
			sub_v3_v3v3(D, sample->P, P);

			/* ignore sample in the front */
			a= dot_v3v3(D, sample->N);
			if((a*a/(dot_v3v3(D, D) + EPSILON)) > 0.1f)
				continue;

			dist= len_v3(D);

			if(dist > sample->dP*distfac)
				continue;

#if 0
			{
				/* error metric following [Tabellion 2004] */
				//float area= sqrtf(len_v3(dPdu)*len_v3(dPdv));
				float area= (len_v3(dPdu) + len_v3(dPdv))*0.5f;
				e1= sqrtf(dot_v3v3(D, D))/maxf(minf(sample->dP*0.5f, 10.0f*area), 1.5f*area);
			}
			e1= len_v3(D)/sample->dP;
#endif

			/* positional error */
			e1= dist/maxdist;

			/* directional error */
			e2= maxf(1.0f - dot_v3v3(N, sample->N), 0.0f);
			e2= sqrtf(e2*WEIGHT_NORMAL_DENOM);

			/* compute the weight */
			w= 1.0f - MAX_ERROR_K*maxf(e1, e2);

			if(w > BLI_thread_frand(cache->thread)*discard_weight) {
				if(!preprocess) {
					float *C= irr_sample_C(sample);

					if(use_lsq) {
						float a[4]= {D[0], D[1], D[2], 1.0f};
						lsq_4D_add(lsq, a, C, dimension, w);
					}
					else {
						for(i=0; i<dimension; i++)
							accum[i] += w*C[i];
					}
				}

				totw += w;
				totfound++;
			}
		}

		/* check the children */
		for(i=0; i<8; i++) {
			IrrCacheNode *tnode= node->children[i];

			if(tnode && irr_cache_node_point_inside(tnode, 1.0f, maxdist, P))
				*stacknode++= tnode;
		}
	}

	/* do we have anything ? */
	if(totw > EPSILON && totfound >= 1) {
		if(!preprocess) {
			if(use_lsq) {
				lsq_4D_solve(lsq, accum, dimension);
			}
			else {
				float invw= 1.0/totw;

				for(i=0; i<dimension; i++)
					accum[i] *= invw;
			}

			irr_sample_eval(re, shi, accum, ao, env, indirect);
		}
	}
	else if(!cache->locked) {
		/* create a new sample */
		irr_cache_add(re, shi, cache, ao, env, indirect, P, dPdu, dPdv, N, bumpN, preprocess);
		added= 1;

		if(!preprocess)
			cache->totpost++;
	}

	if(!preprocess)
		cache->totlookup++;

	/* make sure we don't have NaNs */
	// assert(dot_v3v3(C, C) >= 0);

	return added;
}

void irr_cache_create(Render *re, RenderPart *pa)
{
	IrrCache *cache;
	
	if(!((re->db.wrld.aomode & WO_LIGHT_CACHE) && (re->db.wrld.mode & (WO_AMB_OCC|WO_ENV_LIGHT|WO_INDIRECT_LIGHT))))
		return;
	if((re->params.r.mode & R_RAYTRACE) == 0)
		return;

	BLI_lock_thread(LOCK_RCACHE);
	cache= irr_cache_new(re, pa->thread);
	re->db.irrcache[pa->thread]= cache;
	BLI_unlock_thread(LOCK_RCACHE);
}

void irr_cache_fill(Render *re, RenderPart *pa, RenderLayer *rl, ShadeSample *ssamp, int docrop)
{
	RenderResult *rr= pa->result;
	IrrCache *cache= re->db.irrcache[pa->thread];
	int crop, x, y, seed, step;

	if(!cache || cache->locked)
		return;

	seed= pa->rectx*pa->disprect.ymin;

	crop= 0;
	if(docrop)
		crop= 1;

	//step= MAX2(pa->disprect.ymax - pa->disprect.ymin + 2*crop, pa->disprect.xmax - pa->disprect.xmin + 2*crop);
	step= 1;

	while(step > 0) {
		rr->renrect.ymin= 0;
		rr->renrect.ymax= -2*crop;
		rr->renlay= rl;

		for(y=pa->disprect.ymin+crop; y<pa->disprect.ymax-crop; y+=step, rr->renrect.ymax++) {
			for(x=pa->disprect.xmin+crop; x<pa->disprect.xmax-crop; x+=step) {
				int lx = (x - pa->disprect.xmin);
				int ly = (y - pa->disprect.ymin);
				int od = lx + ly*(pa->disprect.xmax - pa->disprect.xmin);
				PixelRow *row= pa->pixelrow;
				int a, b, totrow;

				BLI_thread_srandom(pa->thread, seed++);

				/* create shade pixel row, sorted front to back */
				totrow= pixel_row_fill(row, re, pa, od);

				for(a=0; a<totrow; a++) {
					shade_samples_from_pixel(re, ssamp, &row[a], lx+pa->disprect.xmin, ly+pa->disprect.ymin);

					for(b=0; b<ssamp->tot; b++) {
						ShadeInput *shi= &ssamp->shi[b];
						ShadeGeometry *geom= &shi->geometry;
						float *ao= (re->db.wrld.mode & WO_AMB_OCC)? shi->shading.ao: NULL;
						float *env= (re->db.wrld.mode & WO_ENV_LIGHT)? shi->shading.env: NULL;
						float *indirect= (re->db.wrld.mode & WO_INDIRECT_LIGHT)? shi->shading.indirect: NULL;
						int added;

						if(!mat_need_cache(re, shi->material.mat))
							continue;

						if(shi->primitive.strand) {
							added= 0;
#if 0
							float co[3], n[3];

							/* TODO: dxco/dyco? */
							shade_strand_surface_co(shi, co, n);
							added= irr_cache_lookup(re, shi, cache,
								ao, env, indirect,
								co, geom->dxco, geom->dyco, n, n, 1);
#endif
						}
						else {
							added= irr_cache_lookup(re, shi, cache,
								ao, env, indirect,
								geom->co, geom->dxco, geom->dyco, geom->vno, geom->vn, 1);
						}
						
						if(added) {
							if(indirect)
								add_v3_v3(rl->rectf + od*4, indirect);
							if(env)
								add_v3_v3(rl->rectf + od*4, env);
							if(ao)
								rl->rectf[od*4] += *ao;
						}
					}
				}

				if(re->cb.test_break(re->cb.tbh)) break;
			}

			if(re->cb.test_break(re->cb.tbh)) break;
		}

		step /= 2;
	}

	memset(rl->rectf, 0, sizeof(float)*4*rl->rectx*rl->recty);
}

void irr_cache_free(Render *re, RenderPart *pa)
{
	IrrCache *cache= re->db.irrcache[pa->thread];

	if(cache) {
		irr_cache_delete(cache);
		re->db.irrcache[pa->thread]= NULL;
	}
}

