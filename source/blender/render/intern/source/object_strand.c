/**
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
 * The Original Code is: none of this file.
 *
 * Contributors: Brecht Van Lommel.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "MEM_guardedalloc.h"

#include "DNA_key_types.h"
#include "DNA_material_types.h"
#include "DNA_meshdata_types.h"

#include "BLI_blenlib.h"
#include "BLI_ghash.h"
#include "BLI_math.h"
#include "BLI_memarena.h"

#include "BKE_DerivedMesh.h"
#include "BKE_key.h"
#include "BKE_utildefines.h"

#include "camera.h"
#include "database.h"
#include "lamp.h"
#include "part.h"
#include "render_types.h"
#include "rendercore.h"
#include "result.h"
#include "shading.h"
#include "object.h"
#include "object_mesh.h"
#include "object_strand.h"
#include "zbuf.h"

/* to be removed */
void hoco_to_zco(ZSpan *zspan, float *zco, float *hoco);
void zspan_scanconvert_strand(ZSpan *zspan, void *handle, float *v1, float *v2, float *v3, void (*func)(void *, int, int, float, float, float) );
void zbufsinglewire(ZSpan *zspan, int obi, int zvlnr, float *ho1, float *ho2);



/******************************* Strands *************************************/

float *render_strand_get_orco(ObjectRen *obr, StrandRen *strand, int verify)
{
	float *orco;
	int nr= strand->index>>8;
	
	orco= obr->strandnodes[nr].orco;
	if(orco==NULL) {
		if(verify) 
			orco= obr->strandnodes[nr].orco= MEM_callocN(256*RE_ORCO_ELEMS*sizeof(float), "orco strand table");
		else
			return NULL;
	}
	return orco + (strand->index & 255)*RE_ORCO_ELEMS;
}

float *render_strand_get_surfnor(ObjectRen *obr, StrandRen *strand, int verify)
{
	float *surfnor;
	int nr= strand->index>>8;
	
	surfnor= obr->strandnodes[nr].surfnor;
	if(surfnor==NULL) {
		if(verify) 
			surfnor= obr->strandnodes[nr].surfnor= MEM_callocN(256*RE_SURFNOR_ELEMS*sizeof(float), "surfnor strand table");
		else
			return NULL;
	}
	return surfnor + (strand->index & 255)*RE_SURFNOR_ELEMS;
}

float *render_strand_get_uv(ObjectRen *obr, StrandRen *strand, int n, char **name, int verify)
{
	StrandTableNode *node;
	int nr= strand->index>>8, strandindex= (strand->index&255);
	int index= (n<<8) + strandindex;

	node= &obr->strandnodes[nr];

	if(verify) {
		if(n>=node->totuv) {
			float *uv= node->uv;
			int size= (n+1)*256;

			node->uv= MEM_callocN(size*sizeof(float)*RE_UV_ELEMS, "strand uv table");

			if(uv) {
				size= node->totuv*256;
				memcpy(node->uv, uv, size*sizeof(float)*RE_UV_ELEMS);
				MEM_freeN(uv);
			}

			node->totuv= n+1;
		}
	}
	else {
		if(n>=node->totuv)
			return NULL;

		if(name) *name= obr->mtface[n];
	}

	return node->uv + index*RE_UV_ELEMS;
}

MCol *render_strand_get_mcol(ObjectRen *obr, StrandRen *strand, int n, char **name, int verify)
{
	StrandTableNode *node;
	int nr= strand->index>>8, strandindex= (strand->index&255);
	int index= (n<<8) + strandindex;

	node= &obr->strandnodes[nr];

	if(verify) {
		if(n>=node->totmcol) {
			MCol *mcol= node->mcol;
			int size= (n+1)*256;

			node->mcol= MEM_callocN(size*sizeof(MCol)*RE_MCOL_ELEMS, "strand mcol table");

			if(mcol) {
				size= node->totmcol*256;
				memcpy(node->mcol, mcol, size*sizeof(MCol)*RE_MCOL_ELEMS);
				MEM_freeN(mcol);
			}

			node->totmcol= n+1;
		}
	}
	else {
		if(n>=node->totmcol)
			return NULL;

		if(name) *name= obr->mcol[n];
	}

	return node->mcol + index*RE_MCOL_ELEMS;
}

float *render_strand_get_simplify(struct ObjectRen *obr, struct StrandRen *strand, int verify)
{
	float *simplify;
	int nr= strand->index>>8;
	
	simplify= obr->strandnodes[nr].simplify;
	if(simplify==NULL) {
		if(verify) 
			simplify= obr->strandnodes[nr].simplify= MEM_callocN(256*RE_SIMPLIFY_ELEMS*sizeof(float), "simplify strand table");
		else
			return NULL;
	}
	return simplify + (strand->index & 255)*RE_SIMPLIFY_ELEMS;
}

int *render_strand_get_face(ObjectRen *obr, StrandRen *strand, int verify)
{
	int *face;
	int nr= strand->index>>8;
	
	face= obr->strandnodes[nr].face;
	if(face==NULL) {
		if(verify) 
			face= obr->strandnodes[nr].face= MEM_callocN(256*RE_FACE_ELEMS*sizeof(int), "face strand table");
		else
			return NULL;
	}
	return face + (strand->index & 255)*RE_FACE_ELEMS;
}

/* winspeed is exception, it is stored per instance */
float *render_strand_get_winspeed(ObjectInstanceRen *obi, StrandRen *strand, int verify)
{
	float *winspeed;
	int totvector;
	
	winspeed= obi->vectors;
	if(winspeed==NULL) {
		if(verify) {
			totvector= obi->obr->totvert + obi->obr->totstrand;
			winspeed= obi->vectors= MEM_callocN(totvector*RE_WINSPEED_ELEMS*sizeof(float), "winspeed strand table");
		}
		else
			return NULL;
	}
	return winspeed + (obi->obr->totvert + strand->index)*RE_WINSPEED_ELEMS;
}

StrandRen *render_object_strand_get(ObjectRen *obr, int nr)
{
	StrandRen *v;
	int a;

	a= render_object_chunk_get((void**)&obr->strandnodes, &obr->strandnodeslen, nr, sizeof(StrandTableNode));
	v= obr->strandnodes[a].strand;
	
	if(v == NULL) {
		int i;

		v= (StrandRen *)MEM_callocN(256*sizeof(StrandRen),"findOrAddStrand");
		obr->strandnodes[a].strand= v;

		for(i= (nr & 0xFFFFFF00), a=0; a<256; a++, i++)
			v[a].index= i;
	}

	return v + (nr & 255);
}

StrandBuffer *render_object_strand_buffer_add(ObjectRen *obr, int totvert)
{
	StrandBuffer *strandbuf;

	strandbuf= MEM_callocN(sizeof(StrandBuffer), "StrandBuffer");
	strandbuf->vert= MEM_callocN(sizeof(StrandVert)*totvert, "StrandVert");
	strandbuf->totvert= totvert;
	strandbuf->obr= obr;

	obr->strandbuf= strandbuf;

	return strandbuf;
}

/* *************** */

static float strand_eval_width(Material *ma, float strandco)
{
	float fac;

	strandco= 0.5f*(strandco + 1.0f);

	if(ma->strand_ease!=0.0f) {
		if(ma->strand_ease<0.0f)
			fac= pow(strandco, 1.0+ma->strand_ease);
		else
			fac= pow(strandco, 1.0/(1.0f-ma->strand_ease));
	}
	else fac= strandco;
	
	return ((1.0f-fac)*ma->strand_sta + (fac)*ma->strand_end);
}

void strand_eval_point(StrandSegment *sseg, StrandPoint *spoint)
{
	Material *ma;
	StrandBuffer *strandbuf;
	float *simplify;
	float p[4][3], data[4], cross[3], crosslen, w, dx, dy, t;
	int type;

	strandbuf= sseg->buffer;
	ma= sseg->buffer->ma;
	t= spoint->t;
	type= (strandbuf->flag & R_STRAND_BSPLINE)? KEY_BSPLINE: KEY_CARDINAL;

	copy_v3_v3(p[0], sseg->v[0]->co);
	copy_v3_v3(p[1], sseg->v[1]->co);
	copy_v3_v3(p[2], sseg->v[2]->co);
	copy_v3_v3(p[3], sseg->v[3]->co);

	if(sseg->obi->flag & R_TRANSFORMED) {
		mul_m4_v3(sseg->obi->mat, p[0]);
		mul_m4_v3(sseg->obi->mat, p[1]);
		mul_m4_v3(sseg->obi->mat, p[2]);
		mul_m4_v3(sseg->obi->mat, p[3]);
	}

	if(t == 0.0f) {
		copy_v3_v3(spoint->co, p[1]);
		spoint->strandco= sseg->v[1]->strandco;

		spoint->dtstrandco= (sseg->v[2]->strandco - sseg->v[0]->strandco);
		if(sseg->v[0] != sseg->v[1])
			spoint->dtstrandco *= 0.5f;
	}
	else if(t == 1.0f) {
		copy_v3_v3(spoint->co, p[2]);
		spoint->strandco= sseg->v[2]->strandco;

		spoint->dtstrandco= (sseg->v[3]->strandco - sseg->v[1]->strandco);
		if(sseg->v[3] != sseg->v[2])
			spoint->dtstrandco *= 0.5f;
	}
	else {
		key_curve_position_weights(t, data, type);
		spoint->co[0]= data[0]*p[0][0] + data[1]*p[1][0] + data[2]*p[2][0] + data[3]*p[3][0];
		spoint->co[1]= data[0]*p[0][1] + data[1]*p[1][1] + data[2]*p[2][1] + data[3]*p[3][1];
		spoint->co[2]= data[0]*p[0][2] + data[1]*p[1][2] + data[2]*p[2][2] + data[3]*p[3][2];
		spoint->strandco= (1.0f-t)*sseg->v[1]->strandco + t*sseg->v[2]->strandco;
	}

	key_curve_tangent_weights(t, data, type);
	spoint->dtco[0]= data[0]*p[0][0] + data[1]*p[1][0] + data[2]*p[2][0] + data[3]*p[3][0];
	spoint->dtco[1]= data[0]*p[0][1] + data[1]*p[1][1] + data[2]*p[2][1] + data[3]*p[3][1];
	spoint->dtco[2]= data[0]*p[0][2] + data[1]*p[1][2] + data[2]*p[2][2] + data[3]*p[3][2];

	copy_v3_v3(spoint->tan, spoint->dtco);
	normalize_v3(spoint->tan);

	copy_v3_v3(spoint->nor, spoint->co);
	mul_v3_fl(spoint->nor, -1.0f);
	normalize_v3(spoint->nor);

	spoint->width= strand_eval_width(ma, spoint->strandco);
	
	/* simplification */
	simplify= render_strand_get_simplify(strandbuf->obr, sseg->strand, 0);
	spoint->alpha= (simplify)? simplify[1]: 1.0f;

	/* outer points */
	cross_v3_v3v3(cross, spoint->co, spoint->tan);

	w= spoint->co[2]*strandbuf->winmat[2][3] + strandbuf->winmat[3][3];
	dx= strandbuf->winx*cross[0]*strandbuf->winmat[0][0]/w;
	dy= strandbuf->winy*cross[1]*strandbuf->winmat[1][1]/w;
	w= sqrt(dx*dx + dy*dy);

	if(w > 0.0f) {
		if(strandbuf->flag & R_STRAND_B_UNITS) {
			crosslen= len_v3(cross);
			w= 2.0f*crosslen*strandbuf->minwidth/w;

			if(spoint->width < w) {
				spoint->alpha= spoint->width/w;
				spoint->width= w;
			}

			if(simplify)
				/* squared because we only change width, not length */
				spoint->width *= simplify[0]*simplify[0];

			mul_v3_fl(cross, spoint->width*0.5f/crosslen);
		}
		else
			mul_v3_fl(cross, spoint->width/w);
	}

	sub_v3_v3v3(spoint->co1, spoint->co, cross);
	add_v3_v3v3(spoint->co2, spoint->co, cross);

	copy_v3_v3(spoint->dsco, cross);
}

/* *************** */

void strand_apply_shaderesult_alpha(ShadeResult *shr, float alpha)
{
	if(alpha < 1.0f) {
		shr->combined[0] *= alpha;
		shr->combined[1] *= alpha;
		shr->combined[2] *= alpha;
		shr->combined[3] *= alpha;

		shr->col[0] *= alpha;
		shr->col[1] *= alpha;
		shr->col[2] *= alpha;
		shr->col[3] *= alpha;

		shr->alpha *= alpha;
	}
}

void strand_shade_point(Render *re, ShadeSample *ssamp, StrandSegment *sseg, StrandPoint *spoint)
{
	ShadeInput *shi= ssamp->shi;
	ShadeResult *shr= ssamp->shr;
	VlakRen vlr;

	memset(&vlr, 0, sizeof(vlr));
	vlr.flag= R_SMOOTH;
	if(sseg->buffer->ma->mode & MA_TANGENT_STR)
		vlr.flag |= R_TANGENT;

	shi->primitive.vlr= &vlr;
	shi->primitive.v1= NULL;
	shi->primitive.v2= NULL;
	shi->primitive.v3= NULL;
	shi->primitive.strand= sseg->strand;
	shi->primitive.obi= sseg->obi;
	shi->primitive.obr= sseg->obi->obr;

	/* cache for shadow */
	shi->shading.samplenr= re->sample.shadowsamplenr[shi->shading.thread]++;

	shade_input_set_strand(re, shi, sseg->strand, spoint);
	shade_input_set_strand_texco(re, shi, sseg->strand, sseg->v[1], spoint);
	
	/* init material vars */
	shade_input_init_material(re, shi);
	
	/* shade */
	shade_input_do_shade(re, shi, shr);

	/* apply simplification */
	strand_apply_shaderesult_alpha(shr, spoint->alpha);

	/* include lamphalos for strand, since halo layer was added already */
	if(re->params.flag & R_LAMPHALO)
		if(shi->shading.layflag & SCE_LAY_HALO)
			lamp_spothalo_render(re, shi, shr->combined, shr->combined[3]);
	
	shi->primitive.strand= NULL;
}

/* *************** */

struct StrandShadeCache {
	GHash *resulthash;
	GHash *refcounthash;
	MemArena *memarena;
};

StrandShadeCache *strand_shade_cache_create()
{
	StrandShadeCache *cache;

	cache= MEM_callocN(sizeof(StrandShadeCache), "StrandShadeCache");
	cache->resulthash= BLI_ghash_new(BLI_ghashutil_ptrhash, BLI_ghashutil_ptrcmp, "StrandShadeCache gh");
	cache->refcounthash= BLI_ghash_new(BLI_ghashutil_ptrhash, BLI_ghashutil_ptrcmp, "StrandShadeCache2 gh");
	cache->memarena= BLI_memarena_new(BLI_MEMARENA_STD_BUFSIZE, "strand cache arena");
	
	return cache;
}

void strand_shade_cache_free(StrandShadeCache *cache)
{
	BLI_ghash_free(cache->refcounthash, NULL, NULL);
	BLI_ghash_free(cache->resulthash, NULL, (GHashValFreeFP)MEM_freeN);
	BLI_memarena_free(cache->memarena);
	MEM_freeN(cache);
}

static void strand_shade_get(Render *re, StrandShadeCache *cache, ShadeSample *ssamp, StrandSegment *sseg, StrandVert *svert)
{
	ShadeResult *hashshr;
	StrandPoint p;
	int *refcount;

	hashshr= BLI_ghash_lookup(cache->resulthash, svert);
	refcount= BLI_ghash_lookup(cache->refcounthash, svert);

	if(!hashshr) {
		/* not shaded yet, shade and insert into hash */
		p.t= (sseg->v[1] == svert)? 0.0f: 1.0f;
		strand_eval_point(sseg, &p);
		strand_shade_point(re, ssamp, sseg, &p);

		hashshr= MEM_callocN(sizeof(ShadeResult), "HashShadeResult");
		*hashshr= ssamp->shr[0];
		BLI_ghash_insert(cache->resulthash, svert, hashshr);
	}
	else
		/* already shaded, just copy previous result from hash */
		ssamp->shr[0]= *hashshr;
	
	/* lower reference count and remove if not needed anymore by any samples */
	(*refcount)--;
	if(*refcount == 0) {
		BLI_ghash_remove(cache->resulthash, svert, NULL, (GHashValFreeFP)MEM_freeN);
		BLI_ghash_remove(cache->refcounthash, svert, NULL, NULL);
	}
}

void strand_shade_segment(Render *re, StrandShadeCache *cache, StrandSegment *sseg, ShadeSample *ssamp, float t, float s, int addpassflag)
{
	ShadeResult shr1, shr2;

	/* get shading for two endpoints and interpolate */
	strand_shade_get(re, cache, ssamp, sseg, sseg->v[1]);
	shr1= ssamp->shr[0];
	strand_shade_get(re, cache, ssamp, sseg, sseg->v[2]);
	shr2= ssamp->shr[0];

	shade_result_interpolate(ssamp->shr, &shr1, &shr2, t, addpassflag);

	/* apply alpha along width */
	if(sseg->buffer->widthfade != 0.0f) {
		s = 1.0f - pow(fabs(s), sseg->buffer->widthfade);

		strand_apply_shaderesult_alpha(ssamp->shr, s);
	}
}

void strand_shade_unref(StrandShadeCache *cache, StrandVert *svert)
{
	int *refcount;

	/* lower reference count and remove if not needed anymore by any samples */
	refcount= BLI_ghash_lookup(cache->refcounthash, svert);

	(*refcount)--;
	if(*refcount == 0) {
		BLI_ghash_remove(cache->resulthash, svert, NULL, (GHashValFreeFP)MEM_freeN);
		BLI_ghash_remove(cache->refcounthash, svert, NULL, NULL);
	}
}

static void strand_shade_refcount(StrandShadeCache *cache, StrandVert *svert)
{
	int *refcount= BLI_ghash_lookup(cache->refcounthash, svert);

	if(!refcount) {
		refcount= BLI_memarena_alloc(cache->memarena, sizeof(int));
		*refcount= 1;
		BLI_ghash_insert(cache->refcounthash, svert, refcount);
	}
	else
		(*refcount)++;
}

/* *************** */

typedef struct StrandPart {
	Render *re;
	ZSpan *zspan;

	APixstrand *apixbuf;
	int *totapixbuf;
	int *rectz;
	int *rectmask;
	PixStr **rectdaps;
	int rectx, recty;
	int sample;
	int shadow;
	float (*jit)[2];

	StrandSegment *segment;
	float t[3], s[3];

	StrandShadeCache *cache;
} StrandPart;

typedef struct StrandSortSegment {
	struct StrandSortSegment *next;
	int obi, strand, segment;
	float z;
} StrandSortSegment;

static int compare_strand_segment(const void *poin1, const void *poin2)
{
	const StrandSortSegment *seg1= (const StrandSortSegment*)poin1;
	const StrandSortSegment *seg2= (const StrandSortSegment*)poin2;

	if(seg1->z < seg2->z)
		return -1;
	else if(seg1->z == seg2->z)
		return 0;
	else
		return 1;
}

static void do_strand_point_project(float winmat[][4], ZSpan *zspan, float *co, float *hoco, float *zco)
{
	camera_matrix_co_to_hoco(winmat, hoco, co);
	hoco_to_zco(zspan, zco, hoco);
}

static void strand_project_point(float winmat[][4], float winx, float winy, StrandPoint *spoint)
{
	float div;

	camera_matrix_co_to_hoco(winmat, spoint->hoco, spoint->co);

	div= 1.0f/spoint->hoco[3];
	spoint->x= spoint->hoco[0]*div*winx*0.5f;
	spoint->y= spoint->hoco[1]*div*winy*0.5f;
}

static APixstrand *addpsmainAstrand(ListBase *lb)
{
	APixstrMain *psm;

	psm= MEM_mallocN(sizeof(APixstrMain), "addpsmainA");
	BLI_addtail(lb, psm);
	psm->ps= MEM_callocN(4096*sizeof(APixstrand),"pixstr strand");

	return psm->ps;
}

static APixstrand *addpsAstrand(ZSpan *zspan)
{
	/* make new PS */
	if(zspan->apstrandmcounter==0) {
		zspan->curpstrand= addpsmainAstrand(zspan->apsmbase);
		zspan->apstrandmcounter= 4095;
	}
	else {
		zspan->curpstrand++;
		zspan->apstrandmcounter--;
	}
	return zspan->curpstrand;
}

static void do_strand_fillac(void *handle, int x, int y, float u, float v, float z)
{
	StrandPart *spart= (StrandPart*)handle;
	StrandShadeCache *cache= spart->cache;
	StrandSegment *sseg= spart->segment;
	APixstrand *apn, *apnew;
	float t, s;
	int offset, mask, obi, strnr, seg, zverg, bufferz, maskz=0;

	offset = y*spart->rectx + x;
	obi= sseg->obi - spart->re->db.objectinstance;
	strnr= sseg->strand->index + 1;
	seg= sseg->v[1] - sseg->strand->vert;
	mask= (1<<spart->sample);

	/* check against solid z-buffer */
	zverg= (int)z;

	if(spart->rectdaps) {
		/* find the z of the sample */
		PixStr *ps;
		PixStr **rd= spart->rectdaps + offset;
		
		bufferz= 0x7FFFFFFF;
		if(spart->rectmask) maskz= 0x7FFFFFFF;
		
		if(*rd) {	
			for(ps= (PixStr *)(*rd); ps; ps= ps->next) {
				if(mask & ps->mask) {
					bufferz= ps->z;
					if(spart->rectmask)
						maskz= ps->maskz;
					break;
				}
			}
		}
	}
	else {
		bufferz= (spart->rectz)? spart->rectz[offset]: 0x7FFFFFFF;
		if(spart->rectmask)
			maskz= spart->rectmask[offset];
	}

#define CHECK_ADD(n) \
	if(apn->p[n]==strnr && apn->obi[n]==obi && apn->seg[n]==seg) \
	{ if(!(apn->mask[n] & mask)) { apn->mask[n] |= mask; apn->v[n] += t; apn->u[n] += s; } break; }
#define CHECK_ASSIGN(n) \
	if(apn->p[n]==0) \
	{apn->obi[n]= obi; apn->p[n]= strnr; apn->z[n]= zverg; apn->mask[n]= mask; apn->v[n]= t; apn->u[n]= s; apn->seg[n]= seg; break; }

	/* add to pixel list */
	if(zverg < bufferz && (spart->totapixbuf[offset] < MAX_PIXEL_ROW)) {
		if(!spart->rectmask || zverg > maskz) {
			t = u*spart->t[0] + v*spart->t[1] + (1.0f-u-v)*spart->t[2];
			s = fabs(u*spart->s[0] + v*spart->s[1] + (1.0f-u-v)*spart->s[2]);

			apn= spart->apixbuf + offset;
			while(apn) {
				CHECK_ADD(0);
				CHECK_ADD(1);
				CHECK_ADD(2);
				CHECK_ADD(3);
				CHECK_ASSIGN(0);
				CHECK_ASSIGN(1);
				CHECK_ASSIGN(2);
				CHECK_ASSIGN(3);

				apnew= addpsAstrand(spart->zspan);
				SWAP(APixstrand, *apnew, *apn);
				apn->next= apnew;
				CHECK_ASSIGN(0);
			}

			if(cache) {
				strand_shade_refcount(cache, sseg->v[1]);
				strand_shade_refcount(cache, sseg->v[2]);
			}
			spart->totapixbuf[offset]++;
		}
	}
}

static int strand_test_clip(float winmat[][4], ZSpan *zspan, float *bounds, float *co, float *zcomp)
{
	float hoco[4];
	int clipflag= 0;

	camera_matrix_co_to_hoco(winmat, hoco, co);

	/* we compare z without perspective division for segment sorting */
	*zcomp= hoco[2];

	if(hoco[0] > bounds[1]*hoco[3]) clipflag |= 1;
	else if(hoco[0]< bounds[0]*hoco[3]) clipflag |= 2;
	else if(hoco[1] > bounds[3]*hoco[3]) clipflag |= 4;
	else if(hoco[1]< bounds[2]*hoco[3]) clipflag |= 8;

	clipflag |= camera_hoco_test_clip(hoco);

	return clipflag;
}

static void do_scanconvert_strand(Render *re, StrandPart *spart, ZSpan *zspan, float t, float dt, float *co1, float *co2, float *co3, float *co4, int sample)
{
	float jco1[3], jco2[3], jco3[3], jco4[3], jx, jy;

	copy_v3_v3(jco1, co1);
	copy_v3_v3(jco2, co2);
	copy_v3_v3(jco3, co3);
	copy_v3_v3(jco4, co4);

	if(spart->jit) {
		jx= -spart->jit[sample][0];
		jy= -spart->jit[sample][1];

		jco1[0] += jx; jco1[1] += jy;
		jco2[0] += jx; jco2[1] += jy;
		jco3[0] += jx; jco3[1] += jy;
		jco4[0] += jx; jco4[1] += jy;

		/* XXX mblur? */
	}

	spart->sample= sample;

	spart->t[0]= t-dt;
	spart->s[0]= -1.0f;
	spart->t[1]= t-dt;
	spart->s[1]= 1.0f;
	spart->t[2]= t;
	spart->s[2]= 1.0f;
	zspan_scanconvert_strand(zspan, spart, jco1, jco2, jco3, do_strand_fillac);
	spart->t[0]= t-dt;
	spart->s[0]= -1.0f;
	spart->t[1]= t;
	spart->s[1]= 1.0f;
	spart->t[2]= t;
	spart->s[2]= -1.0f;
	zspan_scanconvert_strand(zspan, spart, jco1, jco3, jco4, do_strand_fillac);
}

static void strand_render(Render *re, StrandSegment *sseg, float winmat[][4], StrandPart *spart, ZSpan *zspan, int totzspan, StrandPoint *p1, StrandPoint *p2)
{
	if(spart) {
		float t= p2->t;
		float dt= p2->t - p1->t;
		int a;

		if(re->params.osa) {
			for(a=0; a<re->params.osa; a++)
				do_scanconvert_strand(re, spart, zspan, t, dt, p1->zco2, p1->zco1, p2->zco1, p2->zco2, a);
		}
		else
			do_scanconvert_strand(re, spart, zspan, t, dt, p1->zco2, p1->zco1, p2->zco1, p2->zco2, 0);
	}
	else {
		float hoco1[4], hoco2[4];
		int a, obi, index;
  
		obi= sseg->obi - re->db.objectinstance;
		index= sseg->strand->index;

  		camera_matrix_co_to_hoco(winmat, hoco1, p1->co);
  		camera_matrix_co_to_hoco(winmat, hoco2, p2->co);
  
		for(a=0; a<totzspan; a++) {
#if 0
			/* render both strand and single pixel wire to counter aliasing */
			zbufclip4(re, &zspan[a], obi, index, p1->hoco2, p1->hoco1, p2->hoco1, p2->hoco2, p1->clip2, p1->clip1, p2->clip1, p2->clip2);
#endif
			/* only render a line for now, which makes the shadow map more
			   similiar across frames, and so reduces flicker */
			zbufsinglewire(&zspan[a], obi, index, hoco1, hoco2);
		}
	}
}
  
static int strand_segment_recursive(Render *re, float winmat[][4], StrandPart *spart, ZSpan *zspan, int totzspan, StrandSegment *sseg, StrandPoint *p1, StrandPoint *p2, int depth)
{
	StrandPoint p;
	StrandBuffer *buffer= sseg->buffer;
	float dot, d1[2], d2[2], len1, len2;

	if(depth == buffer->maxdepth)
		return 0;

	p.t= (p1->t + p2->t)*0.5f;
	strand_eval_point(sseg, &p);
	strand_project_point(buffer->winmat, buffer->winx, buffer->winy, &p);

	d1[0]= (p.x - p1->x);
	d1[1]= (p.y - p1->y);
	len1= d1[0]*d1[0] + d1[1]*d1[1];

	d2[0]= (p2->x - p.x);
	d2[1]= (p2->y - p.y);
	len2= d2[0]*d2[0] + d2[1]*d2[1];

	if(len1 == 0.0f || len2 == 0.0f)
		return 0;
	
	dot= d1[0]*d2[0] + d1[1]*d2[1];
	if(dot*dot > sseg->sqadaptcos*len1*len2)
		return 0;

	if(spart) {
		do_strand_point_project(winmat, zspan, p.co1, p.hoco1, p.zco1);
		do_strand_point_project(winmat, zspan, p.co2, p.hoco2, p.zco2);
	}
	else {
#if 0
		camera_matrix_co_to_hoco(winmat, p.hoco1, p.co1);
		camera_matrix_co_to_hoco(winmat, p.hoco2, p.co2);
		p.clip1= camera_hoco_test_clip(p.hoco1);
		p.clip2= camera_hoco_test_clip(p.hoco2);
#endif
	}

	if(!strand_segment_recursive(re, winmat, spart, zspan, totzspan, sseg, p1, &p, depth+1))
		strand_render(re, sseg, winmat, spart, zspan, totzspan, p1, &p);
	if(!strand_segment_recursive(re, winmat, spart, zspan, totzspan, sseg, &p, p2, depth+1))
		strand_render(re, sseg, winmat, spart, zspan, totzspan, &p, p2);
	
	return 1;
}

void render_strand_segment(Render *re, float winmat[][4], StrandPart *spart, ZSpan *zspan, int totzspan, StrandSegment *sseg)
{
	StrandBuffer *buffer= sseg->buffer;
	StrandPoint *p1= &sseg->point1;
	StrandPoint *p2= &sseg->point2;

	p1->t= 0.0f;
	p2->t= 1.0f;

	strand_eval_point(sseg, p1);
	strand_project_point(buffer->winmat, buffer->winx, buffer->winy, p1);
	strand_eval_point(sseg, p2);
	strand_project_point(buffer->winmat, buffer->winx, buffer->winy, p2);

	if(spart) {
		do_strand_point_project(winmat, zspan, p1->co1, p1->hoco1, p1->zco1);
		do_strand_point_project(winmat, zspan, p1->co2, p1->hoco2, p1->zco2);
		do_strand_point_project(winmat, zspan, p2->co1, p2->hoco1, p2->zco1);
		do_strand_point_project(winmat, zspan, p2->co2, p2->hoco2, p2->zco2);
	}
	else {
#if 0
		camera_matrix_co_to_hoco(winmat, p1->hoco1, p1->co1);
		camera_matrix_co_to_hoco(winmat, p1->hoco2, p1->co2);
		camera_matrix_co_to_hoco(winmat, p2->hoco1, p2->co1);
		camera_matrix_co_to_hoco(winmat, p2->hoco2, p2->co2);
		p1->clip1= camera_hoco_test_clip(p1->hoco1);
		p1->clip2= camera_hoco_test_clip(p1->hoco2);
		p2->clip1= camera_hoco_test_clip(p2->hoco1);
		p2->clip2= camera_hoco_test_clip(p2->hoco2);
#endif
	}

	if(!strand_segment_recursive(re, winmat, spart, zspan, totzspan, sseg, p1, p2, 0))
		strand_render(re, sseg, winmat, spart, zspan, totzspan, p1, p2);
}

/* render call to fill in strands */
int zbuffer_strands_abuf(Render *re, RenderPart *pa, APixstrand *apixbuf, ListBase *apsmbase, unsigned int lay, int negzmask, float winmat[][4], int winx, int winy, int sample, float (*jit)[2], float clipcrop, int shadow, StrandShadeCache *cache)
{
	ObjectRen *obr;
	ObjectInstanceRen *obi;
	ZSpan zspan;
	StrandRen *strand=0;
	StrandVert *svert;
	StrandBound *sbound;
	StrandPart spart;
	StrandSegment sseg;
	StrandSortSegment *sortsegments = NULL, *sortseg, *firstseg;
	MemArena *memarena;
	float z[4], bounds[4], obwinmat[4][4];
	int a, b, c, i, totsegment, clip[4];

	if(re->cb.test_break(re->cb.tbh))
		return 0;
	if(re->db.totstrand == 0)
		return 0;

	/* setup StrandPart */
	memset(&spart, 0, sizeof(spart));

	spart.re= re;
	spart.rectx= pa->rectx;
	spart.recty= pa->recty;
	spart.apixbuf= apixbuf;
	spart.zspan= &zspan;
	spart.rectdaps= pa->rectdaps;
	spart.rectz= pa->rectz;
	spart.rectmask= pa->rectmask;
	spart.cache= cache;
	spart.shadow= shadow;
	spart.jit= jit;

	zbuf_alloc_span(&zspan, pa->rectx, pa->recty, clipcrop);

	/* needed for transform from hoco to zbuffer co */
	zspan.zmulx= ((float)winx)/2.0;
	zspan.zmuly= ((float)winy)/2.0;
	
	zspan.zofsx= -pa->disprect.xmin;
	zspan.zofsy= -pa->disprect.ymin;

	/* to center the sample position */
	if(!shadow) {
		zspan.zofsx -= 0.5f;
		zspan.zofsy -= 0.5f;
	}

	zspan.apsmbase= apsmbase;

	/* clipping setup */
	bounds[0]= (2*pa->disprect.xmin - winx-1)/(float)winx;
	bounds[1]= (2*pa->disprect.xmax - winx+1)/(float)winx;
	bounds[2]= (2*pa->disprect.ymin - winy-1)/(float)winy;
	bounds[3]= (2*pa->disprect.ymax - winy+1)/(float)winy;

	memarena= BLI_memarena_new(BLI_MEMARENA_STD_BUFSIZE, "zbuf strands arena");
	firstseg= NULL;
	sortseg= sortsegments;
	totsegment= 0;

	/* for all object instances */
	for(obi=re->db.instancetable.first, i=0; obi; obi=obi->next, i++) {
		obr= obi->obr;

		if(!obr->strandbuf || !(obr->strandbuf->lay & lay))
			continue;

		/* compute matrix and try clipping whole object */
		if(obi->flag & R_TRANSFORMED)
			mul_m4_m4m4(obwinmat, obi->mat, winmat);
		else
			copy_m4_m4(obwinmat, winmat);

		if(box_clip_bounds_m4(obi->obr->boundbox, bounds, winmat))
			continue;

		/* for each bounding box containing a number of strands */
		sbound= obr->strandbuf->bound;
		for(c=0; c<obr->strandbuf->totbound; c++, sbound++) {
			if(box_clip_bounds_m4(sbound->boundbox, bounds, winmat))
				continue;

			/* for each strand in this bounding box */
			for(a=sbound->start; a<sbound->end; a++) {
				strand= render_object_strand_get(obr, a);
				svert= strand->vert;

				/* keep clipping and z depth for 4 control points */
				clip[1]= strand_test_clip(obwinmat, &zspan, bounds, svert->co, &z[1]);
				clip[2]= strand_test_clip(obwinmat, &zspan, bounds, (svert+1)->co, &z[2]);
				clip[0]= clip[1]; z[0]= z[1];

				for(b=0; b<strand->totvert-1; b++, svert++) {
					/* compute 4th point clipping and z depth */
					if(b < strand->totvert-2) {
						clip[3]= strand_test_clip(obwinmat, &zspan, bounds, (svert+2)->co, &z[3]);
					}
					else {
						clip[3]= clip[2]; z[3]= z[2];
					}

					/* check clipping and add to sortsegments buffer */
					if(!(clip[0] & clip[1] & clip[2] & clip[3])) {
						sortseg= BLI_memarena_alloc(memarena, sizeof(StrandSortSegment));
						sortseg->obi= i;
						sortseg->strand= strand->index;
						sortseg->segment= b;

						sortseg->z= 0.5f*(z[1] + z[2]);

						sortseg->next= firstseg;
						firstseg= sortseg;
						totsegment++;
					}

					/* shift clipping and z depth */
					clip[0]= clip[1]; z[0]= z[1];
					clip[1]= clip[2]; z[1]= z[2];
					clip[2]= clip[3]; z[2]= z[3];
				}
			}
		}
	}

	if(!re->cb.test_break(re->cb.tbh)) {
		/* convert list to array and sort */
		sortsegments= MEM_mallocN(sizeof(StrandSortSegment)*totsegment, "StrandSortSegment");
		for(a=0, sortseg=firstseg; a<totsegment; a++, sortseg=sortseg->next)
			sortsegments[a]= *sortseg;
		qsort(sortsegments, totsegment, sizeof(StrandSortSegment), compare_strand_segment);
	}

	BLI_memarena_free(memarena);

	spart.totapixbuf= MEM_callocN(sizeof(int)*pa->rectx*pa->recty, "totapixbuf");

	if(!re->cb.test_break(re->cb.tbh)) {
		/* render segments in sorted order */
		sortseg= sortsegments;
		for(a=0; a<totsegment; a++, sortseg++) {
			if(re->cb.test_break(re->cb.tbh))
				break;

			obi= &re->db.objectinstance[sortseg->obi];
			obr= obi->obr;

			if(obi->flag & R_TRANSFORMED)
				mul_m4_m4m4(obwinmat, obi->mat, winmat);
			else
				copy_m4_m4(obwinmat, winmat);

			sseg.obi= obi;
			sseg.strand= render_object_strand_get(obr, sortseg->strand);
			sseg.buffer= sseg.strand->buffer;
			sseg.sqadaptcos= sseg.buffer->adaptcos;
			sseg.sqadaptcos *= sseg.sqadaptcos;

			svert= sseg.strand->vert + sortseg->segment;
			sseg.v[0]= (sortseg->segment > 0)? (svert-1): svert;
			sseg.v[1]= svert;
			sseg.v[2]= svert+1;
			sseg.v[3]= (sortseg->segment < sseg.strand->totvert-2)? svert+2: svert+1;
			sseg.shaded= 0;

			spart.segment= &sseg;

			render_strand_segment(re, obwinmat, &spart, &zspan, 1, &sseg);
		}
	}

	if(sortsegments)
		MEM_freeN(sortsegments);
	MEM_freeN(spart.totapixbuf);
	
	zbuf_free_span(&zspan);

	return totsegment;
}

void strand_minmax(StrandRen *strand, float *min, float *max)
{
	StrandVert *svert;
	int a;

	for(a=0, svert=strand->vert; a<strand->totvert; a++, svert++)
		DO_MINMAX(svert->co, min, max)
}

