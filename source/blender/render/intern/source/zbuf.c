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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * Contributors: Hos, RPW
 *               2004-2006 Blender Foundation, full recode
 *
 * ***** END GPL LICENSE BLOCK *****
 */


/*---------------------------------------------------------------------------*/
/* Common includes                                                           */
/*---------------------------------------------------------------------------*/

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "BLI_blenlib.h"
#include "BLI_threads.h"
#include "BLI_jitter.h"

#include "MTC_matrixops.h"
#include "MEM_guardedalloc.h"

#include "DNA_lamp_types.h"
#include "DNA_mesh_types.h"
#include "DNA_node_types.h"
#include "DNA_meshdata_types.h"

#include "BKE_global.h"
#include "BKE_utildefines.h"

#include "radio_types.h"
#include "radio.h"  /* needs RG, some root data for radiosity */

#include "RE_render_ext.h"

/* local includes */
#include "gammaCorrectionTables.h"
#include "render_types.h"
#include "renderpipeline.h"
#include "renderdatabase.h"
#include "rendercore.h"
#include "pixelblending.h"

/* own includes */
#include "zbuf.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* defined in pipeline.c, is hardcopy of active dynamic allocated Render */
/* only to be used here in this file, it's for speed */
extern struct Render R;
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ****************** Spans ******************************* */

/* each zbuffer has coordinates transformed to local rect coordinates, so we can simply clip */
void zbuf_alloc_span(ZSpan *zspan, int rectx, int recty)
{
	memset(zspan, 0, sizeof(ZSpan));
	
	zspan->rectx= rectx;
	zspan->recty= recty;
	
	zspan->span1= MEM_mallocT(recty*sizeof(float), "zspan");
	zspan->span2= MEM_mallocT(recty*sizeof(float), "zspan");
}

static void zbuf_free_span(ZSpan *zspan)
{
	if(zspan) {
		if(zspan->span1) MEM_freeT(zspan->span1);
		if(zspan->span2) MEM_freeT(zspan->span2);
		zspan->span1= zspan->span2= NULL;
	}
}

/* reset range for clipping */
static void zbuf_init_span(ZSpan *zspan)
{
	zspan->miny1= zspan->miny2= zspan->recty+1;
	zspan->maxy1= zspan->maxy2= -1;
	zspan->minp1= zspan->maxp1= zspan->minp2= zspan->maxp2= NULL;
}

static void zbuf_add_to_span(ZSpan *zspan, float *v1, float *v2)
{
	float *minv, *maxv, *span;
	float xx1, dx0, xs0;
	int y, my0, my2;
	
	if(v1[1]<v2[1]) {
		minv= v1; maxv= v2;
	}
	else {
		minv= v2; maxv= v1;
	}
	
	my0= ceil(minv[1]);
	my2= floor(maxv[1]);
	
	if(my2<0 || my0>= zspan->recty) return;
	
	/* clip top */
	if(my2>=zspan->recty) my2= zspan->recty-1;
	/* clip bottom */
	if(my0<0) my0= 0;
	
	if(my0>my2) return;
	/* if(my0>my2) should still fill in, that way we get spans that skip nicely */
	
	xx1= maxv[1]-minv[1];
	if(xx1>FLT_EPSILON) {
		dx0= (minv[0]-maxv[0])/xx1;
		xs0= dx0*(minv[1]-my2) + minv[0];
	}
	else {
		dx0= 0.0f;
		xs0= MIN2(minv[0],maxv[0]);
	}
	
	/* empty span */
	if(zspan->maxp1 == NULL) {
		span= zspan->span1;
	}
	else {	/* does it complete left span? */
		if( maxv == zspan->minp1 || minv==zspan->maxp1) {
			span= zspan->span1;
		}
		else {
			span= zspan->span2;
		}
	}

	if(span==zspan->span1) {
//		printf("left span my0 %d my2 %d\n", my0, my2);
		if(zspan->minp1==NULL || zspan->minp1[1] > minv[1] ) {
			zspan->minp1= minv;
		}
		if(zspan->maxp1==NULL || zspan->maxp1[1] < maxv[1] ) {
			zspan->maxp1= maxv;
		}
		if(my0<zspan->miny1) zspan->miny1= my0;
		if(my2>zspan->maxy1) zspan->maxy1= my2;
	}
	else {
//		printf("right span my0 %d my2 %d\n", my0, my2);
		if(zspan->minp2==NULL || zspan->minp2[1] > minv[1] ) {
			zspan->minp2= minv;
		}
		if(zspan->maxp2==NULL || zspan->maxp2[1] < maxv[1] ) {
			zspan->maxp2= maxv;
		}
		if(my0<zspan->miny2) zspan->miny2= my0;
		if(my2>zspan->maxy2) zspan->maxy2= my2;
	}

	for(y=my2; y>=my0; y--, xs0+= dx0) {
		/* xs0 is the xcoord! */
		span[y]= xs0;
	}
}

/*-----------------------------------------------------------*/ 
/* Functions                                                 */
/*-----------------------------------------------------------*/ 


void fillrect(int *rect, int x, int y, int val)
{
	int len, *drect;

	len= x*y;
	drect= rect;
	while(len>0) {
		len--;
		*drect= val;
		drect++;
	}
}

/* based on Liang&Barsky, for clipping of pyramidical volume */
static short cliptestf(float p, float q, float *u1, float *u2)
{
	float r;
	
	if(p<0.0) {
		if(q<p) return 0;
		else if(q<0.0) {
			r= q/p;
			if(r>*u2) return 0;
			else if(r>*u1) *u1=r;
		}
	}
	else {
		if(p>0.0) {
			if(q<0.0) return 0;
			else if(q<p) {
				r= q/p;
				if(r<*u1) return 0;
				else if(r<*u2) *u2=r;
			}
		}
		else if(q<0.0) return 0;
	}
	return 1;
}

int testclip(float *v)
{
	float abs4;	/* WATCH IT: this function should do the same as cliptestf, otherwise troubles in zbufclip()*/
	short c=0;
	
	/* if we set clip flags, the clipping should be at least larger than epsilon. 
	   prevents issues with vertices lying exact on borders */
	abs4= fabs(v[3]) + FLT_EPSILON;
	
	if(v[2]< -abs4) c=16;			/* this used to be " if(v[2]<0) ", see clippz() */
	else if(v[2]> abs4) c+= 32;
	
	if( v[0]>abs4) c+=2;
	else if( v[0]< -abs4) c+=1;
	
	if( v[1]>abs4) c+=4;
	else if( v[1]< -abs4) c+=8;
	
	return c;
}



/* *************  ACCUMULATION ZBUF ************ */


static APixstr *addpsmainA(ListBase *lb)
{
	APixstrMain *psm;

	psm= MEM_mallocT(sizeof(APixstrMain), "addpsmainA");
	BLI_addtail(lb, psm);
	psm->ps= MEM_callocT(4096*sizeof(APixstr),"pixstr");

	return psm->ps;
}

static void freepsA(ListBase *lb)
{
	APixstrMain *psm, *psmnext;

	for(psm= lb->first; psm; psm= psmnext) {
		psmnext= psm->next;
		if(psm->ps)
			MEM_freeT(psm->ps);
		MEM_freeT(psm);
	}
}

static APixstr *addpsA(ZSpan *zspan)
{
	/* make new PS */
	if(zspan->apsmcounter==0) {
		zspan->curpstr= addpsmainA(zspan->apsmbase);
		zspan->apsmcounter= 4095;
	}
	else {
		zspan->curpstr++;
		zspan->apsmcounter--;
	}
	return zspan->curpstr;
}

static void zbufinvulAc4(ZSpan *zspan, int zvlnr, float *v1, float *v2, float *v3, float *v4)
{
	APixstr *ap, *apofs, *apn;
	double zxd, zyd, zy0, zverg;
	float x0,y0,z0;
	float x1,y1,z1,x2,y2,z2,xx1;
	float *span1, *span2;
	int *rz, x, y;
	int sn1, sn2, rectx, *rectzofs, my0, my2, mask;
	
	/* init */
	zbuf_init_span(zspan);
	
	/* set spans */
	zbuf_add_to_span(zspan, v1, v2);
	zbuf_add_to_span(zspan, v2, v3);
	if(v4) {
		zbuf_add_to_span(zspan, v3, v4);
		zbuf_add_to_span(zspan, v4, v1);
	}
	else
		zbuf_add_to_span(zspan, v3, v1);
	
	/* clipped */
	if(zspan->minp2==NULL || zspan->maxp2==NULL) return;

	if(zspan->miny1 < zspan->miny2) my0= zspan->miny2; else my0= zspan->miny1;
	if(zspan->maxy1 > zspan->maxy2) my2= zspan->maxy2; else my2= zspan->maxy1;
	
	if(my2<my0) return;
	
	/* ZBUF DX DY, in floats still */
	x1= v1[0]- v2[0];
	x2= v2[0]- v3[0];
	y1= v1[1]- v2[1];
	y2= v2[1]- v3[1];
	z1= v1[2]- v2[2];
	z2= v2[2]- v3[2];
	x0= y1*z2-z1*y2;
	y0= z1*x2-x1*z2;
	z0= x1*y2-y1*x2;
	
	if(z0==0.0) return;
	
	xx1= (x0*v1[0] + y0*v1[1])/z0 + v1[2];
	
	zxd= -(double)x0/(double)z0;
	zyd= -(double)y0/(double)z0;
	zy0= ((double)my2)*zyd + (double)xx1;
	
	/* start-offset in rect */
	rectx= zspan->rectx;
	rectzofs= (int *)(zspan->arectz+rectx*(my2));
	apofs= (zspan->apixbuf+ rectx*(my2));
	mask= zspan->mask;

	/* correct span */
	sn1= (my0 + my2)/2;
	if(zspan->span1[sn1] < zspan->span2[sn1]) {
		span1= zspan->span1+my2;
		span2= zspan->span2+my2;
	}
	else {
		span1= zspan->span2+my2;
		span2= zspan->span1+my2;
	}
	
	for(y=my2; y>=my0; y--, span1--, span2--) {
		
		sn1= floor(*span1);
		sn2= floor(*span2);
		sn1++; 
		
		if(sn2>=rectx) sn2= rectx-1;
		if(sn1<0) sn1= 0;
		
		if(sn2>=sn1) {
			zverg= (double)sn1*zxd + zy0;
			rz= rectzofs+sn1;
			ap= apofs+sn1;
			x= sn2-sn1;
			
			zverg-= zspan->polygon_offset;
			
			while(x>=0) {
				if( (int)zverg < *rz) {
//					int i= zvlnr & 3;
					
					apn= ap;
					while(apn) {
						if(apn->p[0]==0) {apn->p[0]= zvlnr; apn->z[0]= zverg; apn->mask[0]= mask; break; }
						if(apn->p[0]==zvlnr) {apn->mask[0]|= mask; break; }
						if(apn->p[1]==0) {apn->p[1]= zvlnr; apn->z[1]= zverg; apn->mask[1]= mask; break; }
						if(apn->p[1]==zvlnr) {apn->mask[1]|= mask; break; }
						if(apn->p[2]==0) {apn->p[2]= zvlnr; apn->z[2]= zverg; apn->mask[2]= mask; break; }
						if(apn->p[2]==zvlnr) {apn->mask[2]|= mask; break; }
						if(apn->p[3]==0) {apn->p[3]= zvlnr; apn->z[3]= zverg; apn->mask[3]= mask; break; }
						if(apn->p[3]==zvlnr) {apn->mask[3]|= mask; break; }
//						if(apn->p[i]==0) {apn->p[i]= zvlnr; apn->z[i]= zverg; apn->mask[i]= mask; break; }
//						if(apn->p[i]==zvlnr) {apn->mask[i]|= mask; break; }
						if(apn->next==NULL) apn->next= addpsA(zspan);
						apn= apn->next;
					}				
				}
				zverg+= zxd;
				rz++; 
				ap++; 
				x--;
			}
		}
		
		zy0-=zyd;
		rectzofs-= rectx;
		apofs-= rectx;
	}
}



static void zbuflineAc(ZSpan *zspan, int zvlnr, float *vec1, float *vec2)
{
	APixstr *ap, *apn;
	int *rectz;
	int start, end, x, y, oldx, oldy, ofs;
	int dz, vergz, mask, maxtest=0;
	float dx, dy;
	float v1[3], v2[3];
	
	dx= vec2[0]-vec1[0];
	dy= vec2[1]-vec1[1];
	
	mask= zspan->mask;
	
	if(fabs(dx) > fabs(dy)) {

		/* all lines from left to right */
		if(vec1[0]<vec2[0]) {
			VECCOPY(v1, vec1);
			VECCOPY(v2, vec2);
		}
		else {
			VECCOPY(v2, vec1);
			VECCOPY(v1, vec2);
			dx= -dx; dy= -dy;
		}

		start= floor(v1[0]);
		end= start+floor(dx);
		if(end>=zspan->rectx) end= zspan->rectx-1;
		
		oldy= floor(v1[1]);
		dy/= dx;
		
		vergz= v1[2];
		vergz-= zspan->polygon_offset;
		dz= (v2[2]-v1[2])/dx;
		if(vergz>0x50000000 && dz>0) maxtest= 1;		// prevent overflow
		
		rectz= (int *)(zspan->arectz+zspan->rectx*(oldy) +start);
		ap= (zspan->apixbuf+ zspan->rectx*(oldy) +start);

		if(dy<0) ofs= -zspan->rectx;
		else ofs= zspan->rectx;
		
		for(x= start; x<=end; x++, rectz++, ap++) {
			
			y= floor(v1[1]);
			if(y!=oldy) {
				oldy= y;
				rectz+= ofs;
				ap+= ofs;
			}
			
			if(x>=0 && y>=0 && y<zspan->recty) {
				if(vergz<*rectz) {
				
					apn= ap;
					while(apn) {	/* loop unrolled */
						if(apn->p[0]==0) {apn->p[0]= zvlnr; apn->z[0]= vergz; apn->mask[0]= mask; break; }
						if(apn->p[0]==zvlnr) {apn->mask[0]|= mask; break; }
						if(apn->p[1]==0) {apn->p[1]= zvlnr; apn->z[1]= vergz; apn->mask[1]= mask; break; }
						if(apn->p[1]==zvlnr) {apn->mask[1]|= mask; break; }
						if(apn->p[2]==0) {apn->p[2]= zvlnr; apn->z[2]= vergz; apn->mask[2]= mask; break; }
						if(apn->p[2]==zvlnr) {apn->mask[2]|= mask; break; }
						if(apn->p[3]==0) {apn->p[3]= zvlnr; apn->z[3]= vergz; apn->mask[3]= mask; break; }
						if(apn->p[3]==zvlnr) {apn->mask[3]|= mask; break; }
						if(apn->next==0) apn->next= addpsA(zspan);
						apn= apn->next;
					}				
					
				}
			}
			
			v1[1]+= dy;
			if(maxtest && (vergz > 0x7FFFFFF0 - dz)) vergz= 0x7FFFFFF0;
			else vergz+= dz;
		}
	}
	else {
	
		/* all lines from top to bottom */
		if(vec1[1]<vec2[1]) {
			VECCOPY(v1, vec1);
			VECCOPY(v2, vec2);
		}
		else {
			VECCOPY(v2, vec1);
			VECCOPY(v1, vec2);
			dx= -dx; dy= -dy;
		}

		start= floor(v1[1]);
		end= start+floor(dy);
		
		if(start>=zspan->recty || end<0) return;
		
		if(end>=zspan->recty) end= zspan->recty-1;
		
		oldx= floor(v1[0]);
		dx/= dy;
		
		vergz= v1[2];
		vergz-= zspan->polygon_offset;
		dz= (v2[2]-v1[2])/dy;
		if(vergz>0x50000000 && dz>0) maxtest= 1;		// prevent overflow

		rectz= (int *)( zspan->arectz+ (start)*zspan->rectx+ oldx );
		ap= (zspan->apixbuf+ zspan->rectx*(start) +oldx);
				
		if(dx<0) ofs= -1;
		else ofs= 1;

		for(y= start; y<=end; y++, rectz+=zspan->rectx, ap+=zspan->rectx) {
			
			x= floor(v1[0]);
			if(x!=oldx) {
				oldx= x;
				rectz+= ofs;
				ap+= ofs;
			}
			
			if(x>=0 && y>=0 && x<zspan->rectx) {
				if(vergz<*rectz) {
					
					apn= ap;
					while(apn) {	/* loop unrolled */
						if(apn->p[0]==0) {apn->p[0]= zvlnr; apn->z[0]= vergz; apn->mask[0]= mask; break; }
						if(apn->p[0]==zvlnr) {apn->mask[0]|= mask; break; }
						if(apn->p[1]==0) {apn->p[1]= zvlnr; apn->z[1]= vergz; apn->mask[1]= mask; break; }
						if(apn->p[1]==zvlnr) {apn->mask[1]|= mask; break; }
						if(apn->p[2]==0) {apn->p[2]= zvlnr; apn->z[2]= vergz; apn->mask[2]= mask; break; }
						if(apn->p[2]==zvlnr) {apn->mask[2]|= mask; break; }
						if(apn->p[3]==0) {apn->p[3]= zvlnr; apn->z[3]= vergz; apn->mask[3]= mask; break; }
						if(apn->p[3]==zvlnr) {apn->mask[3]|= mask; break; }
						if(apn->next==0) apn->next= addpsA(zspan);
						apn= apn->next;
					}	
					
				}
			}
			
			v1[0]+= dx;
			if(maxtest && (vergz > 0x7FFFFFF0 - dz)) vergz= 0x7FFFFFF0;
			else vergz+= dz;
		}
	}
}

/* *************  NORMAL ZBUFFER ************ */

static void zbufline(ZSpan *zspan, int zvlnr, float *vec1, float *vec2)
{
	int *rectz, *rectp;
	int start, end, x, y, oldx, oldy, ofs;
	int dz, vergz, maxtest= 0;
	float dx, dy;
	float v1[3], v2[3];
	
	dx= vec2[0]-vec1[0];
	dy= vec2[1]-vec1[1];
	
	if(fabs(dx) > fabs(dy)) {

		/* all lines from left to right */
		if(vec1[0]<vec2[0]) {
			VECCOPY(v1, vec1);
			VECCOPY(v2, vec2);
		}
		else {
			VECCOPY(v2, vec1);
			VECCOPY(v1, vec2);
			dx= -dx; dy= -dy;
		}

		start= floor(v1[0]);
		end= start+floor(dx);
		if(end>=zspan->rectx) end= zspan->rectx-1;
		
		oldy= floor(v1[1]);
		dy/= dx;
		
		vergz= floor(v1[2]);
		dz= floor((v2[2]-v1[2])/dx);
		if(vergz>0x50000000 && dz>0) maxtest= 1;		// prevent overflow
		
		rectz= zspan->rectz + oldy*zspan->rectx+ start;
		rectp= zspan->rectp + oldy*zspan->rectx+ start;
		
		if(dy<0) ofs= -zspan->rectx;
		else ofs= zspan->rectx;
		
		for(x= start; x<=end; x++, rectz++, rectp++) {
			
			y= floor(v1[1]);
			if(y!=oldy) {
				oldy= y;
				rectz+= ofs;
				rectp+= ofs;
			}
			
			if(x>=0 && y>=0 && y<zspan->recty) {
				if(vergz<*rectz) {
					*rectz= vergz;
					*rectp= zvlnr;
				}
			}
			
			v1[1]+= dy;
			
			if(maxtest && (vergz > 0x7FFFFFF0 - dz)) vergz= 0x7FFFFFF0;
			else vergz+= dz;
		}
	}
	else {
		/* all lines from top to bottom */
		if(vec1[1]<vec2[1]) {
			VECCOPY(v1, vec1);
			VECCOPY(v2, vec2);
		}
		else {
			VECCOPY(v2, vec1);
			VECCOPY(v1, vec2);
			dx= -dx; dy= -dy;
		}

		start= floor(v1[1]);
		end= start+floor(dy);
		
		if(end>=zspan->recty) end= zspan->recty-1;
		
		oldx= floor(v1[0]);
		dx/= dy;
		
		vergz= floor(v1[2]);
		dz= floor((v2[2]-v1[2])/dy);
		if(vergz>0x50000000 && dz>0) maxtest= 1;		// prevent overflow
		
		rectz= zspan->rectz + start*zspan->rectx+ oldx;
		rectp= zspan->rectp + start*zspan->rectx+ oldx;
		
		if(dx<0) ofs= -1;
		else ofs= 1;

		for(y= start; y<=end; y++, rectz+=zspan->rectx, rectp+=zspan->rectx) {
			
			x= floor(v1[0]);
			if(x!=oldx) {
				oldx= x;
				rectz+= ofs;
				rectp+= ofs;
			}
			
			if(x>=0 && y>=0 && x<zspan->rectx) {
				if(vergz<*rectz) {
					*rectz= vergz;
					*rectp= zvlnr;
				}
			}
			
			v1[0]+= dx;
			if(maxtest && (vergz > 0x7FFFFFF0 - dz)) vergz= 0x7FFFFFF0;
			else vergz+= dz;
		}
	}
}

static void zbufline_onlyZ(ZSpan *zspan, int zvlnr, float *vec1, float *vec2)
{
	int *rectz;
	int start, end, x, y, oldx, oldy, ofs;
	int dz, vergz, maxtest= 0;
	float dx, dy;
	float v1[3], v2[3];
	
	dx= vec2[0]-vec1[0];
	dy= vec2[1]-vec1[1];
	
	if(fabs(dx) > fabs(dy)) {
		
		/* all lines from left to right */
		if(vec1[0]<vec2[0]) {
			VECCOPY(v1, vec1);
			VECCOPY(v2, vec2);
		}
		else {
			VECCOPY(v2, vec1);
			VECCOPY(v1, vec2);
			dx= -dx; dy= -dy;
		}
		
		start= floor(v1[0]);
		end= start+floor(dx);
		if(end>=zspan->rectx) end= zspan->rectx-1;
		
		oldy= floor(v1[1]);
		dy/= dx;
		
		vergz= floor(v1[2]);
		dz= floor((v2[2]-v1[2])/dx);
		if(vergz>0x50000000 && dz>0) maxtest= 1;		// prevent overflow
		
		rectz= zspan->rectz + oldy*zspan->rectx+ start;
		
		if(dy<0) ofs= -zspan->rectx;
		else ofs= zspan->rectx;
		
		for(x= start; x<=end; x++, rectz++) {
			
			y= floor(v1[1]);
			if(y!=oldy) {
				oldy= y;
				rectz+= ofs;
			}
			
			if(x>=0 && y>=0 && y<zspan->recty) {
				if(vergz<*rectz) {
					*rectz= vergz;
				}
			}
			
			v1[1]+= dy;
			
			if(maxtest && (vergz > 0x7FFFFFF0 - dz)) vergz= 0x7FFFFFF0;
			else vergz+= dz;
		}
	}
	else {
		/* all lines from top to bottom */
		if(vec1[1]<vec2[1]) {
			VECCOPY(v1, vec1);
			VECCOPY(v2, vec2);
		}
		else {
			VECCOPY(v2, vec1);
			VECCOPY(v1, vec2);
			dx= -dx; dy= -dy;
		}
		
		start= floor(v1[1]);
		end= start+floor(dy);
		
		if(end>=zspan->recty) end= zspan->recty-1;
		
		oldx= floor(v1[0]);
		dx/= dy;
		
		vergz= floor(v1[2]);
		dz= floor((v2[2]-v1[2])/dy);
		if(vergz>0x50000000 && dz>0) maxtest= 1;		// prevent overflow
		
		rectz= zspan->rectz + start*zspan->rectx+ oldx;
		
		if(dx<0) ofs= -1;
		else ofs= 1;
		
		for(y= start; y<=end; y++, rectz+=zspan->rectx) {
			
			x= floor(v1[0]);
			if(x!=oldx) {
				oldx= x;
				rectz+= ofs;
			}
			
			if(x>=0 && y>=0 && x<zspan->rectx) {
				if(vergz<*rectz) {
					*rectz= vergz;
				}
			}
			
			v1[0]+= dx;
			if(maxtest && (vergz > 0x7FFFFFF0 - dz)) vergz= 0x7FFFFFF0;
			else vergz+= dz;
		}
	}
}


static int clipline(float *v1, float *v2)	/* return 0: do not draw */
{
	float dz,dw, u1=0.0, u2=1.0;
	float dx, dy, v13;
	
	dz= v2[2]-v1[2];
	dw= v2[3]-v1[3];
	
	/* this 1.01 is for clipping x and y just a tinsy larger. that way it is
		filled in with zbufwire correctly when rendering in parts. otherwise
		you see line endings at edges... */
	
	if(cliptestf(-dz-dw, v1[3]+v1[2], &u1,&u2)) {
		if(cliptestf(dz-dw, v1[3]-v1[2], &u1,&u2)) {
			
			dx= v2[0]-v1[0];
			dz= 1.01*(v2[3]-v1[3]);
			v13= 1.01*v1[3];
			
			if(cliptestf(-dx-dz, v1[0]+v13, &u1,&u2)) {
				if(cliptestf(dx-dz, v13-v1[0], &u1,&u2)) {
					
					dy= v2[1]-v1[1];
					
					if(cliptestf(-dy-dz, v1[1]+v13, &u1,&u2)) {
						if(cliptestf(dy-dz, v13-v1[1], &u1,&u2)) {
							
							if(u2<1.0) {
								v2[0]= v1[0]+u2*dx;
								v2[1]= v1[1]+u2*dy;
								v2[2]= v1[2]+u2*dz;
								v2[3]= v1[3]+u2*dw;
							}
							if(u1>0.0) {
								v1[0]= v1[0]+u1*dx;
								v1[1]= v1[1]+u1*dy;
								v1[2]= v1[2]+u1*dz;
								v1[3]= v1[3]+u1*dw;
							}
							return 1;
						}
					}
				}
			}
		}
	}
	
	return 0;
}

static void hoco_to_zco(ZSpan *zspan, float *zco, float *hoco)
{
	float div;
	
	div= 1.0f/hoco[3];
	zco[0]= zspan->zmulx*(1.0+hoco[0]*div) + zspan->zofsx;
	zco[1]= zspan->zmuly*(1.0+hoco[1]*div) + zspan->zofsy;
	zco[2]= 0x7FFFFFFF *(hoco[2]*div);
}

void zbufclipwire(ZSpan *zspan, int zvlnr, VlakRen *vlr)
{
	float vez[20], *f1, *f2, *f3, *f4= 0;
	int c1, c2, c3, c4, ec, and, or;

	/* edgecode: 1= draw */
	ec = vlr->ec;
	if(ec==0) return;

	c1= vlr->v1->clip;
	c2= vlr->v2->clip;
	c3= vlr->v3->clip;
	f1= vlr->v1->ho;
	f2= vlr->v2->ho;
	f3= vlr->v3->ho;
	
	if(vlr->v4) {
		f4= vlr->v4->ho;
		c4= vlr->v4->clip;
		
		and= (c1 & c2 & c3 & c4);
		or= (c1 | c2 | c3 | c4);
	}
	else {
		and= (c1 & c2 & c3);
		or= (c1 | c2 | c3);
	}
	
	if(or) {	/* not in the middle */
		if(and) {	/* out completely */
			return;
		}
		else {	/* clipping */

			if(ec & ME_V1V2) {
				QUATCOPY(vez, f1);
				QUATCOPY(vez+4, f2);
				if( clipline(vez, vez+4)) {
					hoco_to_zco(zspan, vez, vez);
					hoco_to_zco(zspan, vez+4, vez+4);
					zspan->zbuflinefunc(zspan, zvlnr, vez, vez+4);
				}
			}
			if(ec & ME_V2V3) {
				QUATCOPY(vez, f2);
				QUATCOPY(vez+4, f3);
				if( clipline(vez, vez+4)) {
					hoco_to_zco(zspan, vez, vez);
					hoco_to_zco(zspan, vez+4, vez+4);
					zspan->zbuflinefunc(zspan, zvlnr, vez, vez+4);
				}
			}
			if(vlr->v4) {
				if(ec & ME_V3V4) {
					QUATCOPY(vez, f3);
					QUATCOPY(vez+4, f4);
					if( clipline(vez, vez+4)) {
						hoco_to_zco(zspan, vez, vez);
						hoco_to_zco(zspan, vez+4, vez+4);
						zspan->zbuflinefunc(zspan, zvlnr, vez, vez+4);
					}
				}
				if(ec & ME_V4V1) {
					QUATCOPY(vez, f4);
					QUATCOPY(vez+4, f1);
					if( clipline(vez, vez+4)) {
						hoco_to_zco(zspan, vez, vez);
						hoco_to_zco(zspan, vez+4, vez+4);
						zspan->zbuflinefunc(zspan, zvlnr, vez, vez+4);
					}
				}
			}
			else {
				if(ec & ME_V3V1) {
					QUATCOPY(vez, f3);
					QUATCOPY(vez+4, f1);
					if( clipline(vez, vez+4)) {
						hoco_to_zco(zspan, vez, vez);
						hoco_to_zco(zspan, vez+4, vez+4);
						zspan->zbuflinefunc(zspan, zvlnr, vez, vez+4);
					}
				}
			}
			
			return;
		}
	}

	hoco_to_zco(zspan, vez, f1);
	hoco_to_zco(zspan, vez+4, f2);
	hoco_to_zco(zspan, vez+8, f3);
	if(vlr->v4) {
		hoco_to_zco(zspan, vez+12, f4);

		if(ec & ME_V3V4)  zspan->zbuflinefunc(zspan, zvlnr, vez+8, vez+12);
		if(ec & ME_V4V1)  zspan->zbuflinefunc(zspan, zvlnr, vez+12, vez);
	}
	else {
		if(ec & ME_V3V1)  zspan->zbuflinefunc(zspan, zvlnr, vez+8, vez);
	}

	if(ec & ME_V1V2)  zspan->zbuflinefunc(zspan, zvlnr, vez, vez+4);
	if(ec & ME_V2V3)  zspan->zbuflinefunc(zspan, zvlnr, vez+4, vez+8);

}

/**
 * Fill the z buffer, but invert z order, and add the face index to
 * the corresponing face buffer.
 *
 * This is one of the z buffer fill functions called in zbufclip() and
 * zbufwireclip(). 
 *
 * @param v1 [4 floats, world coordinates] first vertex
 * @param v2 [4 floats, world coordinates] second vertex
 * @param v3 [4 floats, world coordinates] third vertex
 */
static void zbufinvulGLinv4(ZSpan *zspan, int zvlnr, float *v1, float *v2, float *v3, float *v4) 
{
	double zxd, zyd, zy0, zverg;
	float x0,y0,z0;
	float x1,y1,z1,x2,y2,z2,xx1;
	float *span1, *span2;
	int *rectpofs, *rp;
	int *rz, x, y;
	int sn1, sn2, rectx, *rectzofs, my0, my2;
	
	/* init */
	zbuf_init_span(zspan);
	
	/* set spans */
	zbuf_add_to_span(zspan, v1, v2);
	zbuf_add_to_span(zspan, v2, v3);
	if(v4) {
		zbuf_add_to_span(zspan, v3, v4);
		zbuf_add_to_span(zspan, v4, v1);
	}
	else 
		zbuf_add_to_span(zspan, v3, v1);
	
	/* clipped */
	if(zspan->minp2==NULL || zspan->maxp2==NULL) return;
	
	if(zspan->miny1 < zspan->miny2) my0= zspan->miny2; else my0= zspan->miny1;
	if(zspan->maxy1 > zspan->maxy2) my2= zspan->maxy2; else my2= zspan->maxy1;
	
	//	printf("my %d %d\n", my0, my2);
	if(my2<my0) return;
	
	
	/* ZBUF DX DY, in floats still */
	x1= v1[0]- v2[0];
	x2= v2[0]- v3[0];
	y1= v1[1]- v2[1];
	y2= v2[1]- v3[1];
	z1= v1[2]- v2[2];
	z2= v2[2]- v3[2];
	x0= y1*z2-z1*y2;
	y0= z1*x2-x1*z2;
	z0= x1*y2-y1*x2;
	
	if(z0==0.0) return;
	
	xx1= (x0*v1[0] + y0*v1[1])/z0 + v1[2];
	
	zxd= -(double)x0/(double)z0;
	zyd= -(double)y0/(double)z0;
	zy0= ((double)my2)*zyd + (double)xx1;
	
	/* start-offset in rect */
	rectx= zspan->rectx;
	rectzofs= (zspan->rectz+rectx*my2);
	rectpofs= (zspan->rectp+rectx*my2);
	
	/* correct span */
	sn1= (my0 + my2)/2;
	if(zspan->span1[sn1] < zspan->span2[sn1]) {
		span1= zspan->span1+my2;
		span2= zspan->span2+my2;
	}
	else {
		span1= zspan->span2+my2;
		span2= zspan->span1+my2;
	}
	
	for(y=my2; y>=my0; y--, span1--, span2--) {
		
		sn1= floor(*span1);
		sn2= floor(*span2);
		sn1++; 
		
		if(sn2>=rectx) sn2= rectx-1;
		if(sn1<0) sn1= 0;
		
		if(sn2>=sn1) {
			zverg= (double)sn1*zxd + zy0;
			rz= rectzofs+sn1;
			rp= rectpofs+sn1;
			x= sn2-sn1;
			
			while(x>=0) {
				if( (int)zverg > *rz || *rz==0x7FFFFFFF) {
					*rz= (int)zverg;
					*rp= zvlnr;
				}
				zverg+= zxd;
				rz++; 
				rp++; 
				x--;
			}
		}
		
		zy0-=zyd;
		rectzofs-= rectx;
		rectpofs-= rectx;
	}
}

/* uses spanbuffers */

static void zbufinvulGL4(ZSpan *zspan, int zvlnr, float *v1, float *v2, float *v3, float *v4)
{
	double zxd, zyd, zy0, zverg;
	float x0,y0,z0;
	float x1,y1,z1,x2,y2,z2,xx1;
	float *span1, *span2;
	int *rectpofs, *rp;
	int *rz, x, y;
	int sn1, sn2, rectx, *rectzofs, my0, my2;
	
	/* init */
	zbuf_init_span(zspan);
	
	/* set spans */
	zbuf_add_to_span(zspan, v1, v2);
	zbuf_add_to_span(zspan, v2, v3);
	if(v4) {
		zbuf_add_to_span(zspan, v3, v4);
		zbuf_add_to_span(zspan, v4, v1);
	}
	else 
		zbuf_add_to_span(zspan, v3, v1);
		
	/* clipped */
	if(zspan->minp2==NULL || zspan->maxp2==NULL) return;
	
	if(zspan->miny1 < zspan->miny2) my0= zspan->miny2; else my0= zspan->miny1;
	if(zspan->maxy1 > zspan->maxy2) my2= zspan->maxy2; else my2= zspan->maxy1;
	
//	printf("my %d %d\n", my0, my2);
	if(my2<my0) return;
	
	
	/* ZBUF DX DY, in floats still */
	x1= v1[0]- v2[0];
	x2= v2[0]- v3[0];
	y1= v1[1]- v2[1];
	y2= v2[1]- v3[1];
	z1= v1[2]- v2[2];
	z2= v2[2]- v3[2];
	x0= y1*z2-z1*y2;
	y0= z1*x2-x1*z2;
	z0= x1*y2-y1*x2;
	
	if(z0==0.0) return;

	xx1= (x0*v1[0] + y0*v1[1])/z0 + v1[2];

	zxd= -(double)x0/(double)z0;
	zyd= -(double)y0/(double)z0;
	zy0= ((double)my2)*zyd + (double)xx1;

	/* start-offset in rect */
	rectx= zspan->rectx;
	rectzofs= (zspan->rectz+rectx*my2);
	rectpofs= (zspan->rectp+rectx*my2);

	/* correct span */
	sn1= (my0 + my2)/2;
	if(zspan->span1[sn1] < zspan->span2[sn1]) {
		span1= zspan->span1+my2;
		span2= zspan->span2+my2;
	}
	else {
		span1= zspan->span2+my2;
		span2= zspan->span1+my2;
	}
	
	for(y=my2; y>=my0; y--, span1--, span2--) {
		
		sn1= floor(*span1);
		sn2= floor(*span2);
		sn1++; 
		
		if(sn2>=rectx) sn2= rectx-1;
		if(sn1<0) sn1= 0;
		
		if(sn2>=sn1) {
			zverg= (double)sn1*zxd + zy0;
			rz= rectzofs+sn1;
			rp= rectpofs+sn1;
			x= sn2-sn1;
			
			while(x>=0) {
				if( (int)zverg < *rz) {
					*rz= (int)zverg;
					*rp= zvlnr;
				}
				zverg+= zxd;
				rz++; 
				rp++; 
				x--;
			}
		}
		
		zy0-=zyd;
		rectzofs-= rectx;
		rectpofs-= rectx;
	}
}

/**
 * Fill the z buffer. The face buffer is not operated on!
 *
 * This is one of the z buffer fill functions called in zbufclip() and
 * zbufwireclip(). 
 *
 * @param v1 [4 floats, world coordinates] first vertex
 * @param v2 [4 floats, world coordinates] second vertex
 * @param v3 [4 floats, world coordinates] third vertex
 */

static void zbufinvulGL_onlyZ(ZSpan *zspan, int zvlnr, float *v1, float *v2, float *v3, float *v4) 
{
	double zxd, zyd, zy0, zverg;
	float x0,y0,z0;
	float x1,y1,z1,x2,y2,z2,xx1;
	float *span1, *span2;
	int *rz, x, y;
	int sn1, sn2, rectx, *rectzofs, my0, my2;
	
	/* init */
	zbuf_init_span(zspan);
	
	/* set spans */
	zbuf_add_to_span(zspan, v1, v2);
	zbuf_add_to_span(zspan, v2, v3);
	if(v4) {
		zbuf_add_to_span(zspan, v3, v4);
		zbuf_add_to_span(zspan, v4, v1);
	}
	else 
		zbuf_add_to_span(zspan, v3, v1);
	
	/* clipped */
	if(zspan->minp2==NULL || zspan->maxp2==NULL) return;
	
	if(zspan->miny1 < zspan->miny2) my0= zspan->miny2; else my0= zspan->miny1;
	if(zspan->maxy1 > zspan->maxy2) my2= zspan->maxy2; else my2= zspan->maxy1;
	
	//	printf("my %d %d\n", my0, my2);
	if(my2<my0) return;
	
	
	/* ZBUF DX DY, in floats still */
	x1= v1[0]- v2[0];
	x2= v2[0]- v3[0];
	y1= v1[1]- v2[1];
	y2= v2[1]- v3[1];
	z1= v1[2]- v2[2];
	z2= v2[2]- v3[2];
	x0= y1*z2-z1*y2;
	y0= z1*x2-x1*z2;
	z0= x1*y2-y1*x2;
	
	if(z0==0.0) return;
	
	xx1= (x0*v1[0] + y0*v1[1])/z0 + v1[2];
	
	zxd= -(double)x0/(double)z0;
	zyd= -(double)y0/(double)z0;
	zy0= ((double)my2)*zyd + (double)xx1;
	
	/* start-offset in rect */
	rectx= zspan->rectx;
	rectzofs= (zspan->rectz+rectx*my2);
	
	/* correct span */
	sn1= (my0 + my2)/2;
	if(zspan->span1[sn1] < zspan->span2[sn1]) {
		span1= zspan->span1+my2;
		span2= zspan->span2+my2;
	}
	else {
		span1= zspan->span2+my2;
		span2= zspan->span1+my2;
	}
	
	for(y=my2; y>=my0; y--, span1--, span2--) {
		
		sn1= floor(*span1);
		sn2= floor(*span2);
		sn1++; 
		
		if(sn2>=rectx) sn2= rectx-1;
		if(sn1<0) sn1= 0;
		
		if(sn2>=sn1) {
			zverg= (double)sn1*zxd + zy0;
			rz= rectzofs+sn1;
			x= sn2-sn1;
			
			while(x>=0) {
				if( (int)zverg < *rz) {
					*rz= (int)zverg;
				}
				zverg+= zxd;
				rz++; 
				x--;
			}
		}
		
		zy0-=zyd;
		rectzofs-= rectx;
	}
}


/**
 * (clip pyramid)
 * Sets labda: flag, and parametrize the clipping of vertices in
 * viewspace coordinates. labda = -1 means no clipping, labda in [0,
	 * 1] means a clipping.
 * Note: uses globals.
 * @param v1 start coordinate s
 * @param v2 target coordinate t
 * @param b1 
 * @param b2 
 * @param b3
 * @param a index for coordinate (x, y, or z)
 */

static void clippyra(float *labda, float *v1, float *v2, int *b2, int *b3, int a)
{
	float da,dw,u1=0.0,u2=1.0;
	float v13;
	
	labda[0]= -1.0;
	labda[1]= -1.0;

	da= v2[a]-v1[a];
	/* prob; we clip slightly larger, osa renders add 2 pixels on edges, should become variable? */
	/* or better; increase r.winx/y size, but thats quite a complex one. do it later */
	if(a==2) {
		dw= (v2[3]-v1[3]);
		v13= v1[3];
	}
	else {
		dw= R.clipcrop*(v2[3]-v1[3]);
		v13= R.clipcrop*v1[3];
	}	
	/* according the original article by Liang&Barsky, for clipping of
	 * homogenous coordinates with viewplane, the value of "0" is used instead of "-w" .
	 * This differs from the other clipping cases (like left or top) and I considered
	 * it to be not so 'homogenic'. But later it has proven to be an error,
	 * who would have thought that of L&B!
	 */

	if(cliptestf(-da-dw, v13+v1[a], &u1,&u2)) {
		if(cliptestf(da-dw, v13-v1[a], &u1,&u2)) {
			*b3=1;
			if(u2<1.0) {
				labda[1]= u2;
				*b2=1;
			}
			else labda[1]=1.0;  /* u2 */
			if(u1>0.0) {
				labda[0]= u1;
				*b2=1;
			} else labda[0]=0.0;
		}
	}
}

/**
 * (make vertex pyramide clip)
 * Checks labda and uses this to make decision about clipping the line
 * segment from v1 to v2. labda is the factor by which the vector is
 * cut. ( calculate s + l * ( t - s )). The result is appended to the
 * vertex list of this face.
 * 
 * 
 * @param v1 start coordinate s
 * @param v2 target coordinate t
 * @param b1 
 * @param b2 
 * @param clve vertex vector.
 */

static void makevertpyra(float *vez, float *labda, float **trias, float *v1, float *v2, int *b1, int *clve)
{
	float l1, l2, *adr;

	l1= labda[0];
	l2= labda[1];

	if(l1!= -1.0) {
		if(l1!= 0.0) {
			adr= vez+4*(*clve);
			trias[*b1]=adr;
			(*clve)++;
			adr[0]= v1[0]+l1*(v2[0]-v1[0]);
			adr[1]= v1[1]+l1*(v2[1]-v1[1]);
			adr[2]= v1[2]+l1*(v2[2]-v1[2]);
			adr[3]= v1[3]+l1*(v2[3]-v1[3]);
		} 
		else trias[*b1]= v1;
		
		(*b1)++;
	}
	if(l2!= -1.0) {
		if(l2!= 1.0) {
			adr= vez+4*(*clve);
			trias[*b1]=adr;
			(*clve)++;
			adr[0]= v1[0]+l2*(v2[0]-v1[0]);
			adr[1]= v1[1]+l2*(v2[1]-v1[1]);
			adr[2]= v1[2]+l2*(v2[2]-v1[2]);
			adr[3]= v1[3]+l2*(v2[3]-v1[3]);
			(*b1)++;
		}
	}
}

/* ------------------------------------------------------------------------- */

void projectverto(float *v1, float winmat[][4], float *adr)
{
	/* calcs homogenic coord of vertex v1 */
	float x,y,z;

	x= v1[0]; 
	y= v1[1]; 
	z= v1[2];
	adr[0]= x*winmat[0][0]				+	z*winmat[2][0] + winmat[3][0];
	adr[1]= 		      y*winmat[1][1]	+	z*winmat[2][1] + winmat[3][1];
	adr[2]=										z*winmat[2][2] + winmat[3][2];
	adr[3]=										z*winmat[2][3] + winmat[3][3];

	//printf("hoco %f %f %f %f\n", adr[0], adr[1], adr[2], adr[3]);
}

/* ------------------------------------------------------------------------- */

void projectvert(float *v1, float winmat[][4], float *adr)
{
	/* calcs homogenic coord of vertex v1 */
	float x,y,z;

	x= v1[0]; 
	y= v1[1]; 
	z= v1[2];
	adr[0]= x*winmat[0][0]+ y*winmat[1][0]+ z*winmat[2][0]+ winmat[3][0];
	adr[1]= x*winmat[0][1]+ y*winmat[1][1]+ z*winmat[2][1]+ winmat[3][1];
	adr[2]= x*winmat[0][2]+ y*winmat[1][2]+ z*winmat[2][2]+ winmat[3][2];
	adr[3]= x*winmat[0][3]+ y*winmat[1][3]+ z*winmat[2][3]+ winmat[3][3];
}

/* do zbuffering and clip, f1 f2 f3 are hocos, c1 c2 c3 are clipping flags */

void zbufclip(ZSpan *zspan, int zvlnr, float *f1, float *f2, float *f3, int c1, int c2, int c3)
{
	float *vlzp[32][3], labda[3][2];
	float vez[400], *trias[40];

	if(c1 | c2 | c3) {	/* not in middle */
		if(c1 & c2 & c3) {	/* completely out */
			return;
		} else {	/* clipping */
			int arg, v, b, clipflag[3], b1, b2, b3, c4, clve=3, clvlo, clvl=1;

			vez[0]= f1[0]; vez[1]= f1[1]; vez[2]= f1[2]; vez[3]= f1[3];
			vez[4]= f2[0]; vez[5]= f2[1]; vez[6]= f2[2]; vez[7]= f2[3];
			vez[8]= f3[0]; vez[9]= f3[1]; vez[10]= f3[2];vez[11]= f3[3];

			vlzp[0][0]= vez;
			vlzp[0][1]= vez+4;
			vlzp[0][2]= vez+8;

			clipflag[0]= ( (c1 & 48) | (c2 & 48) | (c3 & 48) );
			if(clipflag[0]==0) {	/* othwerwise it needs to be calculated again, after the first (z) clip */
				clipflag[1]= ( (c1 & 3) | (c2 & 3) | (c3 & 3) );
				clipflag[2]= ( (c1 & 12) | (c2 & 12) | (c3 & 12) );
			}
			
			for(b=0;b<3;b++) {
				
				if(clipflag[b]) {
				
					clvlo= clvl;
					
					for(v=0; v<clvlo; v++) {
					
						if(vlzp[v][0]!=0) {	/* face is still there */
							b2= b3 =0;	/* clip flags */

							if(b==0) arg= 2;
							else if (b==1) arg= 0;
							else arg= 1;
							
							clippyra(labda[0], vlzp[v][0],vlzp[v][1], &b2,&b3, arg);
							clippyra(labda[1], vlzp[v][1],vlzp[v][2], &b2,&b3, arg);
							clippyra(labda[2], vlzp[v][2],vlzp[v][0], &b2,&b3, arg);

							if(b2==0 && b3==1) {
								/* completely 'in' */;
								vlzp[clvl][0]= vlzp[v][0];
								vlzp[clvl][1]= vlzp[v][1];
								vlzp[clvl][2]= vlzp[v][2];
								clvl++;
							} else if(b3==0) {
								vlzp[v][0]=0;
								/* completely 'out' */;
							} else {
								b1=0;
								makevertpyra(vez, labda[0], trias, vlzp[v][0],vlzp[v][1], &b1,&clve);
								makevertpyra(vez, labda[1], trias, vlzp[v][1],vlzp[v][2], &b1,&clve);
								makevertpyra(vez, labda[2], trias, vlzp[v][2],vlzp[v][0], &b1,&clve);

								/* after front clip done: now set clip flags */
								if(b==0) {
									clipflag[1]= clipflag[2]= 0;
									f1= vez;
									for(b3=0; b3<clve; b3++) {
										c4= testclip(f1);
										clipflag[1] |= (c4 & 3);
										clipflag[2] |= (c4 & 12);
										f1+= 4;
									}
								}
								
								vlzp[v][0]=0;
								if(b1>2) {
									for(b3=3; b3<=b1; b3++) {
										vlzp[clvl][0]= trias[0];
										vlzp[clvl][1]= trias[b3-2];
										vlzp[clvl][2]= trias[b3-1];
										clvl++;
									}
								}
							}
						}
					}
				}
			}

            /* warning, this should never happen! */
			if(clve>38 || clvl>31) printf("clip overflow: clve clvl %d %d\n",clve,clvl);

            /* perspective division */
			f1=vez;
			for(c1=0;c1<clve;c1++) {
				hoco_to_zco(zspan, f1, f1);
				f1+=4;
			}
			for(b=1;b<clvl;b++) {
				if(vlzp[b][0]) {
					zspan->zbuffunc(zspan, zvlnr, vlzp[b][0],vlzp[b][1],vlzp[b][2], NULL);
				}
			}
			return;
		}
	}

	/* perspective division: HCS to ZCS */
	hoco_to_zco(zspan, vez, f1);
	hoco_to_zco(zspan, vez+4, f2);
	hoco_to_zco(zspan, vez+8, f3);
	zspan->zbuffunc(zspan, zvlnr, vez,vez+4,vez+8, NULL);
}

static void zbufclip4(ZSpan *zspan, int zvlnr, float *f1, float *f2, float *f3, float *f4, int c1, int c2, int c3, int c4)
{
	float vez[16];
	
	if(c1 | c2 | c3 | c4) {	/* not in middle */
		if(c1 & c2 & c3 & c4) {	/* completely out */
			return;
		} else {	/* clipping */
			zbufclip(zspan, zvlnr, f1, f2, f3, c1, c2, c3);
			zbufclip(zspan, zvlnr, f1, f3, f4, c1, c3, c4);
		}
		return;
	}

	/* perspective division: HCS to ZCS */
	hoco_to_zco(zspan, vez, f1);
	hoco_to_zco(zspan, vez+4, f2);
	hoco_to_zco(zspan, vez+8, f3);
	hoco_to_zco(zspan, vez+12, f4);

	zspan->zbuffunc(zspan, zvlnr, vez, vez+4, vez+8, vez+12);
}


/* ***************** ZBUFFER MAIN ROUTINES **************** */

void set_part_zbuf_clipflag(RenderPart *pa)
{
	VertRen *ver=NULL;
	float minx, miny, maxx, maxy, wco;
	unsigned short clipclear;
	int v;

	minx= (2*pa->disprect.xmin - R.winx-1)/(float)R.winx;
	maxx= (2*pa->disprect.xmax - R.winx+1)/(float)R.winx;
	miny= (2*pa->disprect.ymin - R.winy-1)/(float)R.winy;
	maxy= (2*pa->disprect.ymax - R.winy+1)/(float)R.winy;
	
	/* supports up to 4 threads this way */
	clipclear= ~(15 << 4*(pa->thread & 3));
	
	/* extra security to prevent access to same data */
	BLI_lock_thread(LOCK_CUSTOM1);
	
	for(v=0; v<R.totvert; v++) {
		if((v & 255)==0)
			ver= RE_findOrAddVert(&R, v);
		else ver++;
		
		wco= ver->ho[3];
		ver->flag &= clipclear;
		
		switch(pa->thread & 3) {
			case 0:
				if( ver->ho[0] > maxx*wco) ver->flag |= 1;
				else if( ver->ho[0]< minx*wco) ver->flag |= 2;
				if( ver->ho[1] > maxy*wco) ver->flag |= 4;
				else if( ver->ho[1]< miny*wco) ver->flag |= 8;
				break;
			case 1:
				if( ver->ho[0] > maxx*wco) ver->flag |= 16;
				else if( ver->ho[0]< minx*wco) ver->flag |= 32;
				if( ver->ho[1] > maxy*wco) ver->flag |= 64;
				else if( ver->ho[1]< miny*wco) ver->flag |= 128;
				break;
			case 2:
				if( ver->ho[0] > maxx*wco) ver->flag |= 256;
				else if( ver->ho[0]< minx*wco) ver->flag |= 512;
				if( ver->ho[1] > maxy*wco) ver->flag |= 1024;
				else if( ver->ho[1]< miny*wco) ver->flag |= 2048;
				break;
			case 3:
				if( ver->ho[0] > maxx*wco) ver->flag |= 4096;
				else if( ver->ho[0]< minx*wco) ver->flag |= 8192;
				if( ver->ho[1] > maxy*wco) ver->flag |= 16384;
				else if( ver->ho[1]< miny*wco) ver->flag |= 32768;
				break;
		}
	}
	
	BLI_unlock_thread(LOCK_CUSTOM1);
}

void zbuffer_solid(RenderPart *pa, unsigned int lay, short layflag)
{
	ZSpan zspan;
	VlakRen *vlr= NULL;
	Material *ma=0;
	int v, zvlnr;
	unsigned short clipmask;
	short nofill=0, env=0, wire=0;

	zbuf_alloc_span(&zspan, pa->rectx, pa->recty);
	
	/* needed for transform from hoco to zbuffer co */
	zspan.zmulx=  ((float)R.winx)/2.0;
	zspan.zmuly=  ((float)R.winy)/2.0;
	
	if(R.osa) {
		zspan.zofsx= -pa->disprect.xmin - R.jit[pa->sample][0];
		zspan.zofsy= -pa->disprect.ymin - R.jit[pa->sample][1];
	}
	else if(R.i.curblur) {
		zspan.zofsx= -pa->disprect.xmin - R.jit[R.i.curblur-1][0];
		zspan.zofsy= -pa->disprect.ymin - R.jit[R.i.curblur-1][1];
	}
	else {
		zspan.zofsx= -pa->disprect.xmin;
		zspan.zofsy= -pa->disprect.ymin;
	}
	/* to centre the sample position */
	zspan.zofsx -= 0.5f;
	zspan.zofsy -= 0.5f;
	
	/* the buffers */
	zspan.rectz= pa->rectz;
	zspan.rectp= pa->rectp;
	fillrect(pa->rectp, pa->rectx, pa->recty, 0);
	fillrect(pa->rectz, pa->rectx, pa->recty, 0x7FFFFFFF);
	
	/* filling methods */
	zspan.zbuffunc= zbufinvulGL4;
	zspan.zbuflinefunc= zbufline;

	/* part clipflag, threaded */
	clipmask= (15 << 4*(pa->thread & 3));

	for(v=0; v<R.totvlak; v++) {

		if((v & 255)==0) vlr= R.blovl[v>>8];
		else vlr++;
		
		if((vlr->flag & R_VISIBLE) && (vlr->lay & lay)) {
			if(vlr->mat!=ma) {
				ma= vlr->mat;
				nofill= ma->mode & (MA_ZTRA|MA_ONLYCAST);
				env= (ma->mode & MA_ENV);
				wire= (ma->mode & MA_WIRE);
				
				if(ma->mode & MA_ZINV) zspan.zbuffunc= zbufinvulGLinv4;
				else zspan.zbuffunc= zbufinvulGL4;
			}
			
			if(nofill==0) {
				unsigned short partclip;
				
				/* partclipping doesn't need viewplane clipping */
				if(vlr->v4) partclip= vlr->v1->flag & vlr->v2->flag & vlr->v3->flag & vlr->v4->flag;
				else partclip= vlr->v1->flag & vlr->v2->flag & vlr->v3->flag;
				
				if((partclip & clipmask)==0) {
					
					if(env) zvlnr= -1;
					else zvlnr= v+1;
					
					if(wire) zbufclipwire(&zspan, zvlnr, vlr);
					else {
						/* strands allow to be filled in as quad */
						if(vlr->v4 && (vlr->flag & R_STRAND)) {
							zbufclip4(&zspan, zvlnr, vlr->v1->ho, vlr->v2->ho, vlr->v3->ho, vlr->v4->ho, vlr->v1->clip, vlr->v2->clip, vlr->v3->clip, vlr->v4->clip);
						}
						else {
							zbufclip(&zspan, zvlnr, vlr->v1->ho, vlr->v2->ho, vlr->v3->ho, vlr->v1->clip, vlr->v2->clip, vlr->v3->clip);
							if(vlr->v4) {
								if(zvlnr>0) zvlnr+= RE_QUAD_OFFS;
								zbufclip(&zspan, zvlnr, vlr->v1->ho, vlr->v3->ho, vlr->v4->ho, vlr->v1->clip, vlr->v3->clip, vlr->v4->clip);
							}
						}
					}
				}
			}
		}
	}
	
	zbuf_free_span(&zspan);
}

typedef struct {
	float *vert;
	float hoco[4];
	int clip;
} VertBucket;

/* warning, not threaded! */
static int hashlist_projectvert(float *v1, float winmat[][4], float *hoco)
{
	static VertBucket bucket[256], *buck;
	
	/* init static bucket */
	if(v1==NULL) {
		memset(bucket, 0, 256*sizeof(VertBucket));
		return 0;
	}
	
	buck= &bucket[ (((long)v1)/16) & 255 ];
	if(buck->vert==v1) {
		QUATCOPY(hoco, buck->hoco);
		return buck->clip;
	}
	
	projectvert(v1, winmat, hoco);
	buck->clip = testclip(hoco);
	buck->vert= v1;
	QUATCOPY(buck->hoco, hoco);
	return buck->clip;
}

/* used for booth radio 'tool' as during render */
void RE_zbufferall_radio(struct RadView *vw, RNode **rg_elem, int rg_totelem, Render *re)
{
	ZSpan zspan;
	float hoco[4][4], winmat[4][4];
	int a, zvlnr;
	int c1, c2, c3, c4= 0;

	if(rg_totelem==0) return;

	hashlist_projectvert(NULL, winmat, NULL);
	
	/* needed for projectvert */
	MTC_Mat4MulMat4(winmat, vw->viewmat, vw->winmat);

	zbuf_alloc_span(&zspan, vw->rectx, vw->recty);
	zspan.zmulx=  ((float)vw->rectx)/2.0;
	zspan.zmuly=  ((float)vw->recty)/2.0;
	zspan.zofsx= -0.5f;
	zspan.zofsy= -0.5f;
	
	/* the buffers */
	zspan.rectz= vw->rectz;
	zspan.rectp= vw->rect;
	fillrect(zspan.rectz, vw->rectx, vw->recty, 0x7FFFFFFF);
	fillrect(zspan.rectp, vw->rectx, vw->recty, 0xFFFFFF);
	
	/* filling methods */
	zspan.zbuffunc= zbufinvulGL4;
	
	if(rg_elem) {	/* radio tool */
		RNode **re, *rn;

		re= rg_elem;
		re+= (rg_totelem-1);
		for(a= rg_totelem-1; a>=0; a--, re--) {
			rn= *re;
			if( (rn->f & RAD_SHOOT)==0 ) {    /* no shootelement */
				
				if( rn->f & RAD_TWOSIDED) zvlnr= a;
				else if( rn->f & RAD_BACKFACE) zvlnr= 0xFFFFFF;	
				else zvlnr= a;
				
				c1= hashlist_projectvert(rn->v1, winmat, hoco[0]);
				c2= hashlist_projectvert(rn->v2, winmat, hoco[1]);
				c3= hashlist_projectvert(rn->v3, winmat, hoco[2]);
				
				if(rn->v4) {
					c4= hashlist_projectvert(rn->v4, winmat, hoco[3]);
				}
	
				if(rn->v4)
					zbufclip4(&zspan, zvlnr, hoco[0], hoco[1], hoco[2], hoco[3], c1, c2, c3, c4);
				else
					zbufclip(&zspan, zvlnr, hoco[0], hoco[1], hoco[2], c1, c2, c3);
			}
		}
	}
	else {	/* radio render */
		VlakRen *vlr=NULL;
		RadFace *rf;
		int totface=0;
		
		for(a=0; a<re->totvlak; a++) {
			if((a & 255)==0) vlr= re->blovl[a>>8]; else vlr++;
		
			if(vlr->radface) {
				rf= vlr->radface;
				if( (rf->flag & RAD_SHOOT)==0 ) {    /* no shootelement */
					
					if( rf->flag & RAD_TWOSIDED) zvlnr= totface;
					else if( rf->flag & RAD_BACKFACE) zvlnr= 0xFFFFFF;	/* receives no energy, but is zbuffered */
					else zvlnr= totface;
					
					c1= hashlist_projectvert(vlr->v1->co, winmat, hoco[0]);
					c2= hashlist_projectvert(vlr->v2->co, winmat, hoco[1]);
					c3= hashlist_projectvert(vlr->v3->co, winmat, hoco[2]);
					
					if(vlr->v4) {
						c4= hashlist_projectvert(vlr->v4->co, winmat, hoco[3]);
					}
		
					if(vlr->v4)
						zbufclip4(&zspan, zvlnr, hoco[0], hoco[1], hoco[2], hoco[3], c1, c2, c3, c4);
					else
						zbufclip(&zspan, zvlnr, hoco[0], hoco[1], hoco[2], c1, c2, c3);
				}
				totface++;
			}
		}
	}

	zbuf_free_span(&zspan);
}

void zbuffer_shadow(Render *re, LampRen *lar, int *rectz, int size, float jitx, float jity)
{
	ZSpan zspan;
	VlakRen *vlr= NULL;
	Material *ma=0;
	int a, ok=1, lay= -1;

	if(lar->mode & LA_LAYER) lay= lar->lay;

	zbuf_alloc_span(&zspan, size, size);
	zspan.zmulx=  ((float)size)/2.0;
	zspan.zmuly=  ((float)size)/2.0;
	zspan.zofsx= jitx;
	zspan.zofsy= jity;
	
	/* the buffers */
	zspan.rectz= rectz;
	fillrect(rectz, size, size, 0x7FFFFFFE);
	
	/* filling methods */
	zspan.zbuflinefunc= zbufline_onlyZ;
	zspan.zbuffunc= zbufinvulGL_onlyZ;
				
	for(a=0; a<re->totvlak; a++) {

		if((a & 255)==0) vlr= re->blovl[a>>8];
		else vlr++;

		if(vlr->mat!= ma) {
			ma= vlr->mat;
			ok= 1;
			if((ma->mode & MA_SHADBUF)==0) ok= 0;
		}
		
		if(ok && (vlr->flag & R_VISIBLE) && (vlr->lay & lay)) {
			if(ma->mode & MA_WIRE) zbufclipwire(&zspan, a+1, vlr);
			else if(vlr->flag & R_STRAND) zbufclipwire(&zspan, a+1, vlr);
			else {
				if(vlr->v4) 
					zbufclip4(&zspan, 0, vlr->v1->ho, vlr->v2->ho, vlr->v3->ho, vlr->v4->ho, vlr->v1->clip, vlr->v2->clip, vlr->v3->clip, vlr->v4->clip);
				else
					zbufclip(&zspan, 0, vlr->v1->ho, vlr->v2->ho, vlr->v3->ho, vlr->v1->clip, vlr->v2->clip, vlr->v3->clip);
			}
		}
	}
	zbuf_free_span(&zspan);
}

/* ******************** VECBLUR ACCUM BUF ************************* */

typedef struct DrawBufPixel {
	float *colpoin;
	float alpha;
} DrawBufPixel;


static void zbuf_fill_in_rgba(ZSpan *zspan, DrawBufPixel *col, float *v1, float *v2, float *v3, float *v4)
{
	DrawBufPixel *rectpofs, *rp;
	double zxd, zyd, zy0, zverg;
	float x0,y0,z0;
	float x1,y1,z1,x2,y2,z2,xx1;
	float *span1, *span2;
	float *rectzofs, *rz;
	int x, y;
	int sn1, sn2, rectx, my0, my2;
	
	/* init */
	zbuf_init_span(zspan);
	
	/* set spans */
	zbuf_add_to_span(zspan, v1, v2);
	zbuf_add_to_span(zspan, v2, v3);
	zbuf_add_to_span(zspan, v3, v4);
	zbuf_add_to_span(zspan, v4, v1);
	
	/* clipped */
	if(zspan->minp2==NULL || zspan->maxp2==NULL) return;
	
	if(zspan->miny1 < zspan->miny2) my0= zspan->miny2; else my0= zspan->miny1;
	if(zspan->maxy1 > zspan->maxy2) my2= zspan->maxy2; else my2= zspan->maxy1;
	
	//	printf("my %d %d\n", my0, my2);
	if(my2<my0) return;
	
	/* ZBUF DX DY, in floats still */
	x1= v1[0]- v2[0];
	x2= v2[0]- v3[0];
	y1= v1[1]- v2[1];
	y2= v2[1]- v3[1];
	z1= v1[2]- v2[2];
	z2= v2[2]- v3[2];
	x0= y1*z2-z1*y2;
	y0= z1*x2-x1*z2;
	z0= x1*y2-y1*x2;
	
	if(z0==0.0) return;
	
	xx1= (x0*v1[0] + y0*v1[1])/z0 + v1[2];
	
	zxd= -(double)x0/(double)z0;
	zyd= -(double)y0/(double)z0;
	zy0= ((double)my2)*zyd + (double)xx1;
	
	/* start-offset in rect */
	rectx= zspan->rectx;
	rectzofs= (float *)(zspan->rectz + rectx*my2);
	rectpofs= ((DrawBufPixel *)zspan->rectp) + rectx*my2;
	
	/* correct span */
	sn1= (my0 + my2)/2;
	if(zspan->span1[sn1] < zspan->span2[sn1]) {
		span1= zspan->span1+my2;
		span2= zspan->span2+my2;
	}
	else {
		span1= zspan->span2+my2;
		span2= zspan->span1+my2;
	}
	
	for(y=my2; y>=my0; y--, span1--, span2--) {
		
		sn1= floor(*span1);
		sn2= floor(*span2);
		sn1++; 
		
		if(sn2>=rectx) sn2= rectx-1;
		if(sn1<0) sn1= 0;
		
		if(sn2>=sn1) {
			zverg= (double)sn1*zxd + zy0;
			rz= rectzofs+sn1;
			rp= rectpofs+sn1;
			x= sn2-sn1;
			
			while(x>=0) {
				if( zverg < *rz) {
					*rz= zverg;
					*rp= *col;
				}
				zverg+= zxd;
				rz++; 
				rp++; 
				x--;
			}
		}
		
		zy0-=zyd;
		rectzofs-= rectx;
		rectpofs-= rectx;
	}
}

static void antialias_tagbuf(int xsize, int ysize, char *rectmove)
{
	char *row1, *row2, *row3;
	char prev, next;
	int a, x, y, step;
	
	/* 1: tag pixels to be candidate for AA */
	for(y=2; y<ysize; y++) {
		/* setup rows */
		row1= rectmove + (y-2)*xsize;
		row2= row1 + xsize;
		row3= row2 + xsize;
		for(x=2; x<xsize; x++, row1++, row2++, row3++) {
			if(row2[1]) {
				if(row2[0]==0 || row2[2]==0 || row1[1]==0 || row3[1]==0)
					row2[1]= 128;
			}
		}
	}
	/* 2: evaluate horizontal scanlines and calculate alphas */
	row1= rectmove;
	for(y=0; y<ysize; y++) {
		row1++;
		for(x=1; x<xsize; x++, row1++) {
			if(row1[0]==128 && row1[1]==128) {
				/* find previous color and next color and amount of steps to blend */
				prev= row1[-1];
				step= 1;
				while(x+step<xsize && row1[step]==128)
					step++;
				
				if(x+step!=xsize) {
					/* now we can blend values */
					next= row1[step];
					if(prev!=next) {
						for(a=0; a<step; a++) {
							int fac, mfac;
							
							fac= ((a+1)<<8)/(step+1);
							mfac= 255-fac;
							
							row1[a]= (prev*mfac + next*fac)>>8; 
						}
					}
				}
			}
		}
	}
	/* 2: evaluate vertical scanlines and calculate alphas */
	for(x=0; x<xsize; x++) {
		row1= rectmove + x+xsize;
		
		for(y=1; y<ysize; y++, row1+=xsize) {
			if(row1[0]==128 && row1[xsize]==128) {
				/* find previous color and next color and amount of steps to blend */
				prev= row1[-xsize];
				step= 1;
				while(y+step<ysize && row1[step*xsize]==128)
					step++;
				
				if(y+step!=ysize) {
					/* now we can blend values */
					next= row1[step*xsize];
					if(prev!=next) {
						for(a=0; a<step; a++) {
							int fac, mfac;
							
							fac= ((a+1)<<8)/(step+1);
							mfac= 255-fac;
							
							row1[a*xsize]= (prev*mfac + next*fac)>>8; 
						}
					}
				}
			}
		}
	}
	
	
	/* last: pixels with 0 we fill in zbuffer, with 1 we skip for mask */
	for(y=2; y<ysize; y++) {
		/* setup rows */
		row1= rectmove + (y-2)*xsize;
		row2= row1 + xsize;
		row3= row2 + xsize;
		for(x=2; x<xsize; x++, row1++, row2++, row3++) {
			if(row2[1]==0) {
				if(row2[0]>1 || row2[2]>1 || row1[1]>1 || row3[1]>1)
					row2[1]= 1;
			}
		}
	}
}

void RE_zbuf_accumulate_vecblur(NodeBlurData *nbd, int xsize, int ysize, float *newrect, float *imgrect, float *vecbufrect, float *zbufrect)
{
	ZSpan zspan;
	DrawBufPixel *rectdraw, *dr;
	float jit[16][2];
	float v1[3], v2[3], v3[3], v4[3], fx, fy;
	float *rectvz, *dvz, *dimg, *dvec1, *dvec2, *dz1, *dz2, *rectz, *minvecbufrect= NULL;
	float maxspeedsq= (float)nbd->maxspeed*nbd->maxspeed;
	int y, x, step, maxspeed=nbd->maxspeed, samples= nbd->samples;
	int tsktsk= 0;
	char *rectmove, *dm;
	
	zbuf_alloc_span(&zspan, xsize, ysize);
	zspan.zmulx=  ((float)xsize)/2.0;
	zspan.zmuly=  ((float)ysize)/2.0;
	zspan.zofsx= 0.0f;
	zspan.zofsy= 0.0f;
	
	/* the buffers */
	rectz= MEM_mapallocT(sizeof(float)*xsize*ysize, "zbuf accum");
	zspan.rectz= (int *)rectz;
	
	rectmove= MEM_mapallocT(xsize*ysize, "rectmove");
	rectdraw= MEM_mapallocT(sizeof(DrawBufPixel)*xsize*ysize, "rect draw");
	zspan.rectp= (int *)rectdraw;
	
	/* debug... check if PASS_VECTOR_MAX still is in buffers */
	dvec1= vecbufrect;
	for(x= 4*xsize*ysize; x>0; x--, dvec1++) {
		if(dvec1[0]==PASS_VECTOR_MAX) {
			dvec1[0]= 0.0f;
			tsktsk= 1;
		}
	}
	if(tsktsk) printf("tsk tsk! PASS_VECTOR_MAX left in buffer...\n");
	
	/* min speed? then copy speedbuffer to recalculate speed vectors */
	if(nbd->minspeed) {
		float minspeed= (float)nbd->minspeed;
		float minspeedsq= minspeed*minspeed;
		
		minvecbufrect= MEM_mapallocT(4*sizeof(float)*xsize*ysize, "minspeed buf");
		
		dvec1= vecbufrect;
		dvec2= minvecbufrect;
		for(x= 2*xsize*ysize; x>0; x--, dvec1+=2, dvec2+=2) {
			if(dvec1[0]==0.0f && dvec1[1]==0.0f) {
				dvec2[0]= dvec1[0];
				dvec2[1]= dvec1[1];
			}
			else {
				float speedsq= dvec1[0]*dvec1[0] + dvec1[1]*dvec1[1];
				if(speedsq <= minspeedsq) {
					dvec2[0]= 0.0f;
					dvec2[1]= 0.0f;
				}
				else {
					speedsq= 1.0f - minspeed/sqrt(speedsq);
					dvec2[0]= speedsq*dvec1[0];
					dvec2[1]= speedsq*dvec1[1];
				}
			}
		}
		SWAP(float *, minvecbufrect, vecbufrect);
	}
	
	/* make vertex buffer with averaged speed and zvalues */
	rectvz= MEM_mapallocT(5*sizeof(float)*(xsize+1)*(ysize+1), "vertices");
	dvz= rectvz;
	for(y=0; y<=ysize; y++) {
		
		if(y==0) {
			dvec1= vecbufrect + 4*y*xsize;
			dz1= zbufrect + y*xsize;
		}
		else {
			dvec1= vecbufrect + 4*(y-1)*xsize;
			dz1= zbufrect + (y-1)*xsize;
		}
		
		if(y==ysize) {
			dvec2= vecbufrect + 4*(y-1)*xsize;
			dz2= zbufrect + (y-1)*xsize;
		}
		else {
			dvec2= vecbufrect + 4*y*xsize;
			dz2= zbufrect + y*xsize;
		}
		
		for(x=0; x<=xsize; x++, dz1++, dz2++) {
			
			/* two vectors, so a step loop */
			for(step=0; step<2; step++, dvec1+=2, dvec2+=2, dvz+=2) {
				/* average on minimal speed */
				int div= 0;
				
				if(x!=0) {
					if(dvec1[-4]!=0.0f || dvec1[-3]!=0.0f) {
						dvz[0]= dvec1[-4];
						dvz[1]= dvec1[-3];
						div++;
					}
					if(dvec2[-4]!=0.0f || dvec2[-3]!=0.0f) {
						if(div==0) {
							dvz[0]= dvec2[-4];
							dvz[1]= dvec2[-3];
							div++;
						}
						else if( (ABS(dvec2[-4]) + ABS(dvec2[-3]))< (ABS(dvz[0]) + ABS(dvz[1])) ) {
							dvz[0]= dvec2[-4];
							dvz[1]= dvec2[-3];
						}
					}
				}

				if(x!=xsize) {
					if(dvec1[0]!=0.0f || dvec1[1]!=0.0f) {
						if(div==0) {
							dvz[0]= dvec1[0];
							dvz[1]= dvec1[1];
							div++;
						}
						else if( (ABS(dvec1[0]) + ABS(dvec1[1]))< (ABS(dvz[0]) + ABS(dvz[1])) ) {
							dvz[0]= dvec1[0];
							dvz[1]= dvec1[1];
						}
					}
					if(dvec2[0]!=0.0f || dvec2[1]!=0.0f) {
						if(div==0) {
							dvz[0]= dvec2[0];
							dvz[1]= dvec2[1];
						}
						else if( (ABS(dvec2[0]) + ABS(dvec2[1]))< (ABS(dvz[0]) + ABS(dvz[1])) ) {
							dvz[0]= dvec2[0];
							dvz[1]= dvec2[1];
						}
					}
				}
				if(maxspeed) {
					float speedsq= dvz[0]*dvz[0] + dvz[1]*dvz[1];
					if(speedsq > maxspeedsq) {
						speedsq= (float)maxspeed/sqrt(speedsq);
						dvz[0]*= speedsq;
						dvz[1]*= speedsq;
					}
				}
			}
			/* the z coordinate */
			if(x!=0) {
				if(x!=xsize)
					dvz[0]= 0.25f*(dz1[-1] + dz2[-1] + dz1[0] + dz2[0]);
				else dvz[0]= 0.5f*(dz1[0] + dz2[0]);
			}
			else dvz[0]= 0.5f*(dz1[-1] + dz2[-1]);
			
			dvz++;
		}
	}
	
	/* set border speeds to keep border speeds on border */
	dz1= rectvz;
	dz2= rectvz+5*(ysize)*(xsize+1);
	for(x=0; x<=xsize; x++, dz1+=5, dz2+=5) {
		dz1[1]= 0.0f;
		dz2[1]= 0.0f;
		dz1[3]= 0.0f;
		dz2[3]= 0.0f;
	}
	dz1= rectvz;
	dz2= rectvz+5*(xsize);
	for(y=0; y<=ysize; y++, dz1+=5*(xsize+1), dz2+=5*(xsize+1)) {
		dz1[0]= 0.0f;
		dz2[0]= 0.0f;
		dz1[2]= 0.0f;
		dz2[2]= 0.0f;
	}
	
	/* tag moving pixels, only these faces we draw */
	dm= rectmove;
	dvec1= vecbufrect;
	for(x=xsize*ysize; x>0; x--, dm++, dvec1+=4) {
		if(dvec1[0]!=0.0f || dvec1[1]!=0.0f || dvec1[2]!=0.0f || dvec1[3]!=0.0f)
			*dm= 255;
	}
	
	antialias_tagbuf(xsize, ysize, rectmove);
	
	BLI_initjit(jit[0], 16);
	
	
	/* accumulate */
	samples/= 2;
	for(step= 1; step<=samples; step++) {
		float speedfac= 0.5f*nbd->fac*(float)step/(float)(samples+1);
		float blendfac= 1.0f/(ABS(step)+1);
		int side, z= 4;
		
		for(side=0; side<2; side++) {
			
			/* clear zbuf, if we draw future we fill in not moving pixels */
			if(0)
				for(x= xsize*ysize-1; x>=0; x--) rectz[x]= 10e16;
			else 
				for(x= xsize*ysize-1; x>=0; x--) {
					if(rectmove[x]==0)
						rectz[x]= zbufrect[x];
					else
						rectz[x]= 10e16;
				}
			
			/* clear drawing buffer */
			for(x= xsize*ysize-1; x>=0; x--) rectdraw[x].colpoin= NULL;
			
			dimg= imgrect;
			dm= rectmove;
			dz1= rectvz;
			dz2= rectvz + 5*(xsize + 1);
			
			if(side) {
				dz1+= 2;
				dz2+= 2;
				z= 2;
				speedfac= -speedfac;
			}

			for(fy= -0.5f+jit[step & 15][0], y=0; y<ysize; y++, fy+=1.0f) {
				for(fx= -0.5f+jit[step & 15][1], x=0; x<xsize; x++, fx+=1.0f, dimg+=4, dz1+=5, dz2+=5, dm++) {
					if(*dm>1) {
						DrawBufPixel col;
						
						/* make vertices */
						v1[0]= speedfac*dz1[0]+fx;			v1[1]= speedfac*dz1[1]+fy;			v1[2]= dz1[z];
						v2[0]= speedfac*dz1[5]+fx+1.0f;		v2[1]= speedfac*dz1[6]+fy;			v2[2]= dz1[z+5];
						v3[0]= speedfac*dz2[5]+fx+1.0f;		v3[1]= speedfac*dz2[6]+fy+1.0f;		v3[2]= dz2[z+5];
						v4[0]= speedfac*dz2[0]+fx;			v4[1]= speedfac*dz2[1]+fy+1.0f;		v4[2]= dz2[z];
						
						if(*dm==255) col.alpha= 1.0f;
						else if(*dm<2) col.alpha= 0.0f;
						else col.alpha= ((float)*dm)/255.0f;
						col.colpoin= dimg;
						
						zbuf_fill_in_rgba(&zspan, &col, v1, v2, v3, v4);
					}
				}
				dz1+=5;
				dz2+=5;
			}
			
			/* accum */
			for(dr= rectdraw, dz2=newrect, x= xsize*ysize-1; x>=0; x--, dr++, dz2+=4) {
				if(dr->colpoin) {
					float bfac= dr->alpha*blendfac;
					float mf= 1.0f - bfac;
					
					dz2[0]= mf*dz2[0] + bfac*dr->colpoin[0];
					dz2[1]= mf*dz2[1] + bfac*dr->colpoin[1];
					dz2[2]= mf*dz2[2] + bfac*dr->colpoin[2];
					dz2[3]= mf*dz2[3] + bfac*dr->colpoin[3];
				}
			}
		}
	}
	
	MEM_freeT(rectz);
	MEM_freeT(rectmove);
	MEM_freeT(rectdraw);
	MEM_freeT(rectvz);
	if(minvecbufrect) MEM_freeT(vecbufrect);  /* rects were swapped! */
	zbuf_free_span(&zspan);
}

/* ******************** ABUF ************************* */

/**
 * Copy results from the solid face z buffering to the transparent
 * buffer.
 */
static void copyto_abufz(RenderPart *pa, int *arectz, int sample)
{
	PixStr *ps;
	int x, y, *rza;
	long *rd;
	
	if(R.osa==0) {
		memcpy(arectz, pa->rectz, 4*pa->rectx*pa->recty);
		return;
	}
	
	rza= arectz;
	rd= pa->rectdaps;

	sample= (1<<sample);
	
	for(y=0; y<pa->recty; y++) {
		for(x=0; x<pa->rectx; x++) {
			
			*rza= 0x7FFFFFFF;
			if(*rd) {	
				/* when there's a sky pixstruct, fill in sky-Z, otherwise solid Z */
				for(ps= (PixStr *)(*rd); ps; ps= ps->next) {
					if(sample & ps->mask) {
						*rza= ps->z;
						break;
					}
				}
			}
			
			rd++; rza++;
		}
	}
}


/* ------------------------------------------------------------------------ */

/**
 * Do accumulation z buffering.
 */

static void zbuffer_abuf(RenderPart *pa, APixstr *APixbuf, ListBase *apsmbase, unsigned int lay)
{
	ZSpan zspan;
	Material *ma=NULL;
	VlakRen *vlr=NULL;
	float vec[3], hoco[4], mul, zval, fval;
	int v, zvlnr, zsample, dofill= 0;
	unsigned short clipmask;
	
	zbuf_alloc_span(&zspan, pa->rectx, pa->recty);
	
	/* needed for transform from hoco to zbuffer co */
	zspan.zmulx=  ((float)R.winx)/2.0;
	zspan.zmuly=  ((float)R.winy)/2.0;
	
	/* the buffers */
	zspan.arectz= MEM_mallocT(sizeof(int)*pa->rectx*pa->recty, "Arectz");
	zspan.apixbuf= APixbuf;
	zspan.apsmbase= apsmbase;
	
	/* filling methods */
	zspan.zbuffunc= zbufinvulAc4;
	zspan.zbuflinefunc= zbuflineAc;
	
	/* part clipflag, 4 threads */
	clipmask= (15 << 4*(pa->thread & 3));
	
	for(zsample=0; zsample<R.osa || R.osa==0; zsample++) {
		
		copyto_abufz(pa, zspan.arectz, zsample);	/* init zbuffer */
		zspan.mask= 1<<zsample;
		
		if(R.osa) {
			zspan.zofsx= -pa->disprect.xmin - R.jit[zsample][0];
			zspan.zofsy= -pa->disprect.ymin - R.jit[zsample][1];
		}
		else if(R.i.curblur) {
			zspan.zofsx= -pa->disprect.xmin - R.jit[R.i.curblur-1][0];
			zspan.zofsy= -pa->disprect.ymin - R.jit[R.i.curblur-1][1];
		}
		else {
			zspan.zofsx= -pa->disprect.xmin;
			zspan.zofsy= -pa->disprect.ymin;
		}
		/* to centre the sample position */
		zspan.zofsx -= 0.5f;
		zspan.zofsy -= 0.5f;
		
		for(v=0; v<R.totvlak; v++) {
			if((v & 255)==0)
				vlr= R.blovl[v>>8];
			else vlr++;
			
			if(vlr->mat!=ma) {
				ma= vlr->mat;
				dofill= (ma->mode & MA_ZTRA) && !(ma->mode & MA_ONLYCAST);
			}
			
			if(dofill) {
				if((vlr->flag & R_VISIBLE) && (vlr->lay & lay)) {
					unsigned short partclip;
					
					/* partclipping doesn't need viewplane clipping */
					if(vlr->v4) partclip= vlr->v1->flag & vlr->v2->flag & vlr->v3->flag & vlr->v4->flag;
					else partclip= vlr->v1->flag & vlr->v2->flag & vlr->v3->flag;
					
					if((partclip & clipmask)==0) {
						/* a little advantage for transp rendering (a z offset) */
						if( ma->zoffs != 0.0) {
							mul= 0x7FFFFFFF;
							zval= mul*(1.0+vlr->v1->ho[2]/vlr->v1->ho[3]);

							VECCOPY(vec, vlr->v1->co);
							/* z is negative, otherwise its being clipped */ 
							vec[2]-= ma->zoffs;
							projectverto(vec, R.winmat, hoco);
							fval= mul*(1.0+hoco[2]/hoco[3]);

							zspan.polygon_offset= (int) fabs(zval - fval );
						}
						else zspan.polygon_offset= 0;
						
						zvlnr= v+1;
			
						if(ma->mode & (MA_WIRE)) zbufclipwire(&zspan, zvlnr, vlr);
						else {
							if(vlr->v4 && (vlr->flag & R_STRAND)) {
								zbufclip4(&zspan, zvlnr, vlr->v1->ho, vlr->v2->ho, vlr->v3->ho, vlr->v4->ho, vlr->v1->clip, vlr->v2->clip, vlr->v3->clip, vlr->v4->clip);
							}
							else {
								zbufclip(&zspan, zvlnr, vlr->v1->ho, vlr->v2->ho, vlr->v3->ho, vlr->v1->clip, vlr->v2->clip, vlr->v3->clip);
								if(vlr->v4) {
									zvlnr+= RE_QUAD_OFFS;
									zbufclip(&zspan, zvlnr, vlr->v1->ho, vlr->v3->ho, vlr->v4->ho, vlr->v1->clip, vlr->v3->clip, vlr->v4->clip);
								}
							}
						}
					}
				}
				if( (v & 255)==255) 
					if(R.test_break()) 
						break; 
			}
		}
		
		if(R.osa==0) break;
		if(R.test_break()) break;
	}
	
	MEM_freeT(zspan.arectz);
	zbuf_free_span(&zspan);

}

/* different rules for speed in transparent pass...  */
/* speed pointer NULL = sky, we clear */
/* else if either alpha is full or no solid was filled in: copy speed */
/* else fill in minimum speed */
static void add_transp_speed(RenderLayer *rl, int offset, float *speed, float alpha, long *rdrect)
{
	RenderPass *rpass;
	
	for(rpass= rl->passes.first; rpass; rpass= rpass->next) {
		if(rpass->passtype==SCE_PASS_VECTOR) {
			float *fp= rpass->rect + 4*offset;
			
			if(speed==NULL) {
				/* clear */
				if(fp[0]==PASS_VECTOR_MAX) fp[0]= 0.0f;
				if(fp[1]==PASS_VECTOR_MAX) fp[1]= 0.0f;
				if(fp[2]==PASS_VECTOR_MAX) fp[2]= 0.0f;
				if(fp[3]==PASS_VECTOR_MAX) fp[3]= 0.0f;
			}
			else if(rdrect==NULL || rdrect[offset]==0 || alpha>0.95f) {
				QUATCOPY(fp, speed);
			}
			else {
				/* add minimum speed in pixel */
				if( (ABS(speed[0]) + ABS(speed[1]))< (ABS(fp[0]) + ABS(fp[1])) ) {
					fp[0]= speed[0];
					fp[1]= speed[1];
				}
				if( (ABS(speed[2]) + ABS(speed[3]))< (ABS(fp[2]) + ABS(fp[3])) ) {
					fp[2]= speed[2];
					fp[3]= speed[3];
				}
				
			}
		}
	}
}

static void add_transp_passes(RenderLayer *rl, int offset, ShadeResult *shr, float weight)
{
	RenderPass *rpass;
	
	for(rpass= rl->passes.first; rpass; rpass= rpass->next) {
		float *fp, *col= NULL;
		int a, pixsize= 3;
		
		switch(rpass->passtype) {
			case SCE_PASS_RGBA:
				if(shr) col= shr->col;
				pixsize= 4;
				break;
			case SCE_PASS_DIFFUSE:
				if(shr) col= shr->diff;
				break;
			case SCE_PASS_SPEC:
				if(shr) col= shr->spec;
				break;
			case SCE_PASS_SHADOW:
				if(shr) col= shr->shad;
				break;
			case SCE_PASS_AO:
				if(shr) col= shr->ao;
				break;
			case SCE_PASS_RAY:
				if(shr) col= shr->ray;
				break;
			case SCE_PASS_NORMAL:
				if(shr) col= shr->nor;
				break;
		}
		if(col) {
			fp= rpass->rect + pixsize*offset;
			for(a=0; a<pixsize; a++)
				fp[a]+= weight*col[a];
		}
	}
}


static int vergzvlak(const void *a1, const void *a2)
{
	const int *x1=a1, *x2=a2;

	if( x1[0] < x2[0] ) return 1;
	else if( x1[0] > x2[0]) return -1;
	return 0;
}

/**
* Shade this face at this location in SCS.
 */
static void shadetrapixel(ShadePixelInfo *shpi, float x, float y, int z, int facenr, int mask)
{
	float rco[3];
	
	if(facenr<0)
		return;
	else if( (facenr & RE_QUAD_MASK) > R.totvlak) {
		printf("error in shadetrapixel nr: %d\n", (facenr & RE_QUAD_MASK));
		return;
	}
	/* correction back for zbuffer filling in */
	x+= 0.5f;
	y+= 0.5f;
	
	if(R.osa) {
		VlakRen *vlr= RE_findOrAddVlak(&R, (facenr-1) & RE_QUAD_MASK);
		float accumcol[4]={0,0,0,0}, tot=0.0;
		int a;
		
		if(vlr->flag & R_FULL_OSA) {
			for(a=0; a<R.osa; a++) {
				if(mask & (1<<a)) {
					shadepixel(shpi, x+R.jit[a][0], y+R.jit[a][1], z, facenr, 1<<a, rco);
					accumcol[0]+= shpi->shr.combined[0];
					accumcol[1]+= shpi->shr.combined[1];
					accumcol[2]+= shpi->shr.combined[2];
					accumcol[3]+= shpi->shr.combined[3];
					tot+= 1.0;
				}
			}
			tot= 1.0/tot;
			shpi->shr.combined[0]= accumcol[0]*tot;
			shpi->shr.combined[1]= accumcol[1]*tot;
			shpi->shr.combined[2]= accumcol[2]*tot;
			shpi->shr.combined[3]= accumcol[3]*tot;
		}
		else {
			int b= R.samples->centmask[mask];
			x= x+R.samples->centLut[b & 15];
			y= y+R.samples->centLut[b>>4];
			shadepixel(shpi, x, y, z, facenr, mask, rco);
		}
	}
	else {
		shadepixel(shpi, x, y, z, facenr, mask, rco);
	}
}

static int addtosamp_shr(ShadeResult *samp_shr, ShadeResult *shr, int mask, int addpassflag)
{
	int a, retval = R.osa;
	
	for(a=0; a < R.osa; a++) {
		if(mask & (1<<a)) {
			addAlphaUnderFloat(samp_shr->combined, shr->combined);
			
			if(addpassflag & SCE_PASS_VECTOR) {
				QUATCOPY(samp_shr->winspeed, shr->winspeed);
			}
			else if(addpassflag & SCE_PASS_NORMAL) {
				VECCOPY(samp_shr->nor, shr->nor);
			}
		}
		
		if(samp_shr->combined[3]>0.999) retval--;
		samp_shr++;
	}
	return retval;
}

#define MAX_ZROW	2000
/* main render call to fill in pass the full transparent layer */

void zbuffer_transp_shade(RenderPart *pa, RenderLayer *rl, float *pass)
{
	RenderResult *rr= pa->result;
	ShadePixelInfo shpi;
	APixstr *APixbuf;      /* Zbuffer: linked list of face samples */
	APixstr *ap, *aprect, *apn;
	ListBase apsmbase={NULL, NULL};
	ShadeResult samp_shr[16];
	float fac, sampalpha, *passrect= pass;
	long *rdrect;
	int x, y, crop=0, a, zrow[MAX_ZROW][3], totface;
	int sval, addpassflag, offs= 0, od, addzbuf;

	/* looks nicer for calling code */
	if(R.test_break())
		return;
	
	APixbuf= MEM_callocT(pa->rectx*pa->recty*sizeof(APixstr), "APixbuf");
	
	if(R.osa>16) {
		printf("abufsetrow: osa too large\n");
		G.afbreek= 1;
		return;
	}
	
	/* fill shadepixel info struct */
	shpi.thread= pa->thread;
	shpi.lay= rl->lay;
	shpi.layflag= rl->layflag;
	shpi.passflag= 0;
	
	if(rl->passflag & ~(SCE_PASS_Z|SCE_PASS_NORMAL|SCE_PASS_VECTOR|SCE_PASS_COMBINED))
		shpi.passflag= rl->passflag;
	addpassflag= rl->passflag & ~(SCE_PASS_Z|SCE_PASS_COMBINED);
	addzbuf= rl->passflag & SCE_PASS_Z;
	
	if(R.osa)
		sampalpha= 1.0f/(float)R.osa;
	else
		sampalpha= 1.0f;
	
	/* fill the Apixbuf */
	zbuffer_abuf(pa, APixbuf, &apsmbase, rl->lay);
	aprect= APixbuf;
	rdrect= pa->rectdaps;
	
	/* filtered render, for now we assume only 1 filter size */
	if(pa->crop) {
		crop= 1;
		offs= pa->rectx + 1;
		passrect+= 4*offs;
		aprect+= offs;
	}
	
	/* init scanline updates */
	rr->renrect.ymin= 0;
	rr->renrect.ymax= -pa->crop;
	rr->renlay= rl;
				
	/* render the tile */
	for(y=pa->disprect.ymin+crop; y<pa->disprect.ymax-crop; y++, rr->renrect.ymax++) {
		pass= passrect;
		ap= aprect;
		od= offs;
		
		if(R.test_break())
			break;
		
		for(x=pa->disprect.xmin+crop; x<pa->disprect.xmax-crop; x++, ap++, pass+=4, od++) {
			
			if(ap->p[0]==NULL) {
				if(addpassflag & SCE_PASS_VECTOR) 
					add_transp_speed(rl, od, NULL, 0.0f, rdrect);
			}
			else {
				/* sort in z */
				totface= 0;
				apn= ap;
				while(apn) {
					for(a=0; a<4; a++) {
						if(apn->p[a]) {
							zrow[totface][0]= apn->z[a];
							zrow[totface][1]= apn->p[a];
							zrow[totface][2]= apn->mask[a];
							totface++;
							if(totface>=MAX_ZROW) totface= MAX_ZROW-1;
						}
						else break;
					}
					apn= apn->next;
				}
				
				if(totface==1) {
					
					shadetrapixel(&shpi, (float)x, (float)y, zrow[0][0], zrow[0][1], zrow[0][2]);
					
					if(R.osa) {
						add_filt_fmask(zrow[0][2], shpi.shr.combined, pass, rr->rectx);
					}
					else {
						QUATCOPY(pass, shpi.shr.combined);
					}
					
					if(addpassflag) 
						add_transp_passes(rl, od, &shpi.shr, 1.0f);
					
					if(addpassflag & SCE_PASS_VECTOR)
						add_transp_speed(rl, od, shpi.shr.winspeed, pass[3], rdrect);
					
					if(addzbuf)
						if(pa->rectz[od]>zrow[0][0])
							pa->rectz[od]= zrow[0][0];
				}
				else {
					
					if(totface==2) {
						if(zrow[0][0] < zrow[1][0]) {
							a= zrow[0][0]; zrow[0][0]= zrow[1][0]; zrow[1][0]= a;
							a= zrow[0][1]; zrow[0][1]= zrow[1][1]; zrow[1][1]= a;
							a= zrow[0][2]; zrow[0][2]= zrow[1][2]; zrow[1][2]= a;
						}
						
					}
					else {	/* totface>2 */
						qsort(zrow, totface, sizeof(int)*3, vergzvlak);
					}
					
					if(addzbuf)
						if(pa->rectz[od]>zrow[totface-1][0])
							pa->rectz[od]= zrow[totface-1][0];
					
					if(R.osa==0) {
						while(totface>0) {
							totface--;
							
							shadetrapixel(&shpi, (float)x, (float)y, zrow[totface][0], zrow[totface][1], zrow[totface][2]);
							addAlphaUnderFloat(pass, shpi.shr.combined);
							if(addpassflag) 
								add_transp_passes(rl, od, &shpi.shr, 1.0f);
							
							if(pass[3]>=0.999) break;
						}
						if(addpassflag & SCE_PASS_VECTOR)
							add_transp_speed(rl, od, shpi.shr.winspeed, pass[3], rdrect);
					}
					else {
						/* for each mask-sample we alpha-under colors. then in end it's added using filter */
						memset(samp_shr, 0, sizeof(ShadeResult)*R.osa);

						while(totface>0) {
							totface--;
							
							shadetrapixel(&shpi, (float)x, (float)y, zrow[totface][0], zrow[totface][1], zrow[totface][2]);
							sval= addtosamp_shr(samp_shr, &shpi.shr, zrow[totface][2], addpassflag);
							
							if(sval==0) break;
						}
						
						for(a=0; a<R.osa; a++) {
							add_filt_fmask(1<<a, samp_shr[a].combined, pass, rr->rectx);
						
							if(addpassflag) 
								add_transp_passes(rl, od, samp_shr+a, sampalpha);
						}
						
						if(addpassflag & SCE_PASS_VECTOR) {
							fac= 0.0f;
							for(a=0; a<R.osa; a++) 
								fac+= samp_shr[a].combined[3];
							add_transp_speed(rl, od, samp_shr[0].winspeed, fac*sampalpha, rdrect);
						}
					}
				}
			}
		}
		
		aprect+= pa->rectx;
		passrect+= 4*pa->rectx;
		offs+= pa->rectx;
	}

	/* disable scanline updating */
	rr->renlay= NULL;

	MEM_freeT(APixbuf);
	freepsA(&apsmbase);	

}

/* *************** */

/* uses part zbuffer values to convert into distances from camera in renderlayer */
void convert_zbuf_to_distbuf(RenderPart *pa, RenderLayer *rl)
{
	RenderPass *rpass;
	float *rectzf, zco;
	int a, *rectz, ortho= R.r.mode & R_ORTHO;
	
	if(pa->rectz==NULL) return;
	for(rpass= rl->passes.first; rpass; rpass= rpass->next)
		if(rpass->passtype==SCE_PASS_Z)
			break;
	
	if(rpass==NULL) {
		printf("called convert zbuf wrong...\n");
		return;
	}
	
	rectzf= rpass->rect;
	rectz= pa->rectz;
	
	for(a=pa->rectx*pa->recty; a>0; a--, rectz++, rectzf++) {
		if(*rectz==0x7FFFFFFF)
			*rectzf= 10e10;
		else {
			/* inverse of zbuf calc: zbuf = MAXZ*hoco_z/hoco_w */
			/* or: (R.winmat[3][2] - zco*R.winmat[3][3])/(R.winmat[2][2] - R.winmat[2][3]*zco); */
			/* if ortho [2][3] is zero, else [3][3] is zero */
			
			zco= ((float)*rectz)/2147483647.0f;
			if(ortho)
				*rectzf= (R.winmat[3][2] - zco*R.winmat[3][3])/(R.winmat[2][2]);
			else
				*rectzf= (R.winmat[3][2])/(R.winmat[2][2] - R.winmat[2][3]*zco);
		}
	}
}


/* end of zbuf.c */




