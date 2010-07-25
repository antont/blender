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
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <math.h>
#include <string.h>

#include "MEM_guardedalloc.h"

#include "DNA_brush_types.h"
#include "DNA_color_types.h"
#include "DNA_scene_types.h"
#include "DNA_windowmanager_types.h"

#include "WM_types.h"
#include "WM_api.h"

#include "RNA_access.h"

#include "BLI_math.h"
#include "BLI_blenlib.h"
#include "BLI_rand.h"

#include "BKE_brush.h"
#include "BKE_colortools.h"
#include "BKE_global.h"
#include "BKE_image.h"
#include "BKE_library.h"
#include "BKE_main.h"
#include "BKE_paint.h"
#include "BKE_texture.h"
#include "BKE_icons.h"

#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"

#include "RE_render_ext.h" /* externtex */
#include "RE_shader_ext.h"

/* Datablock add/copy/free/make_local */

Brush *add_brush(const char *name)
{
	Brush *brush;

	brush= alloc_libblock(&G.main->brush, ID_BR, name);

	/* BRUSH SCULPT TOOL SETTINGS */
	brush->sculpt_tool = SCULPT_TOOL_DRAW; /* sculpting defaults to the draw tool for new brushes */
	brush->size= 35; /* radius of the brush in pixels */
	brush->alpha= 0.5f; /* brush strength/intensity probably variable should be renamed? */
	brush->autosmooth_factor= 0.0f;
	brush->crease_pinch_factor= 0.5f;
	brush->sculpt_plane = SCULPT_DISP_DIR_VIEW;
	brush->plane_offset= 0.0f; /* how far above or below the plane that is found by averaging the faces */
	brush->plane_trim= 0.5f;
	brush->clone.alpha= 0.5f;
	brush->normal_weight= 0.0f;

	/* BRUSH PAINT TOOL SETTINGS */
	brush->rgb[0]= 1.0f; /* default rgb color of the brush when painting - white */
	brush->rgb[1]= 1.0f;
	brush->rgb[2]= 1.0f;

	/* BRUSH STROKE SETTINGS */
	brush->flag |= (BRUSH_SPACE|BRUSH_SPACE_ATTEN);
	brush->spacing= 10; /* how far each brush dot should be spaced as a percentage of brush diameter */

	brush->smooth_stroke_radius= 75;
	brush->smooth_stroke_factor= 0.9f;

	brush->rate= 0.1f; /* time delay between dots of paint or sculpting when doing airbrush mode */

	brush->jitter= 0.0f;

	/* BRUSH TEXTURE SETTINGS */
	default_mtex(&brush->mtex);

	brush->texture_sample_bias= 0; /* value to added to texture samples */

	/* brush appearance  */

	brush->add_col[0]= 1.00; /* add mode color is light red */
	brush->add_col[1]= 0.39;
	brush->add_col[2]= 0.39;

	brush->sub_col[0]= 0.39; /* subtract mode color is light blue */
	brush->sub_col[1]= 0.39;
	brush->sub_col[2]= 1.00;

	 /* the default alpha falloff curve */
	brush_curve_preset(brush, CURVE_PRESET_SMOOTH);

	/* enable fake user by default */
	brush->id.flag |= LIB_FAKEUSER;
	brush_toggled_fake_user(brush);

	return brush;
}

Brush *copy_brush(Brush *brush)
{
	Brush *brushn;
	
	brushn= copy_libblock(brush);

	if (brush->mtex.tex)
		id_us_plus((ID*)brush->mtex.tex);

	if (brush->icon_imbuf)
		brushn->icon_imbuf= IMB_dupImBuf(brush->icon_imbuf);

	brushn->curve= curvemapping_copy(brush->curve);

	/* enable fake user by default */
	if (!(brushn->id.flag & LIB_FAKEUSER)) {
		brushn->id.flag |= LIB_FAKEUSER;
		brush_toggled_fake_user(brushn);
	}

	return brushn;
}

/* not brush itself */
void free_brush(Brush *brush)
{
	if (brush->mtex.tex)
		brush->mtex.tex->id.us--;

	if (brush->icon_imbuf)
		IMB_freeImBuf(brush->icon_imbuf);

	BKE_previewimg_free(&(brush->preview));

	curvemapping_free(brush->curve);
}

void make_local_brush(Brush *brush)
{
	/* - only lib users: do nothing
		* - only local users: set flag
		* - mixed: make copy
		*/
	
	Brush *brushn;
	Scene *scene;
	int local= 0, lib= 0;

	if(brush->id.lib==0) return;

	if(brush->clone.image) {
		/* special case: ima always local immediately */
		brush->clone.image->id.lib= 0;
		brush->clone.image->id.flag= LIB_LOCAL;
		new_id(0, (ID *)brush->clone.image, 0);
	}

	for(scene= G.main->scene.first; scene; scene=scene->id.next)
		if(paint_brush(&scene->toolsettings->imapaint.paint)==brush) {
			if(scene->id.lib) lib= 1;
			else local= 1;
		}

	if(local && lib==0) {
		brush->id.lib= 0;
		brush->id.flag= LIB_LOCAL;
		new_id(0, (ID *)brush, 0);

		/* enable fake user by default */
		if (!(brush->id.flag & LIB_FAKEUSER)) {
			brush->id.flag |= LIB_FAKEUSER;
			brush_toggled_fake_user(brush);
		}
	}
	else if(local && lib) {
		brushn= copy_brush(brush);
		brushn->id.us= 1; /* only keep fake user */
		brushn->id.flag |= LIB_FAKEUSER;
		
		for(scene= G.main->scene.first; scene; scene=scene->id.next)
			if(paint_brush(&scene->toolsettings->imapaint.paint)==brush)
				if(scene->id.lib==0) {
					paint_brush_set(&scene->toolsettings->imapaint.paint, brushn);
					brushn->id.us++;
					brush->id.us--;
				}
	}
}

/* Library Operations */

int brush_set_nr(Brush **current_brush, int nr, const char *name)
{
	ID *idtest, *id;
	
	id= (ID*)(*current_brush);
	idtest= (ID*)BLI_findlink(&G.main->brush, nr-1);
	
	if(idtest==0) { /* new brush */
		if(id) idtest= (ID *)copy_brush((Brush *)id);
		else idtest= (ID *)add_brush(name);
		idtest->us--;
	}
	if(idtest!=id) {
		brush_delete(current_brush);
		*current_brush= (Brush *)idtest;
		id_us_plus(idtest);

		return 1;
	}

	return 0;
}

int brush_delete(Brush **current_brush)
{
	if (*current_brush) {
		(*current_brush)->id.us--;
		*current_brush= NULL;

		return 1;
	}

	return 0;
}

void brush_toggled_fake_user(Brush *brush)
{
	ID *id= (ID*)brush;
	if(id) {
		if(id->flag & LIB_FAKEUSER) {
			id_us_plus(id);
		} else {
			id->us--;
		}
	}
}

void brush_curve_preset(Brush *b, /*CurveMappingPreset*/int preset)
{
	CurveMap *cm = NULL;

	if(!b->curve)
		b->curve = curvemapping_add(1, 0, 0, 1, 1);

	cm = b->curve->cm;
	cm->flag &= ~CUMA_EXTEND_EXTRAPOLATE;

	b->curve->preset = preset;
	curvemap_reset(cm, &b->curve->clipr, b->curve->preset);
	curvemapping_changed(b->curve, 0);
}

static MTex *brush_active_texture(Brush *brush)
{
	if(brush)
		return &brush->mtex;
	return NULL;
}

int brush_texture_set_nr(Brush *brush, int nr)
{
	ID *idtest, *id=NULL;

	id= (ID *)brush->mtex.tex;

	idtest= (ID*)BLI_findlink(&G.main->tex, nr-1);
	if(idtest==0) { /* new tex */
		if(id) idtest= (ID *)copy_texture((Tex *)id);
		else idtest= (ID *)add_texture("Tex");
		idtest->us--;
	}
	if(idtest!=id) {
		brush_texture_delete(brush);

		brush->mtex.tex= (Tex*)idtest;
		id_us_plus(idtest);

		return 1;
	}

	return 0;
}

int brush_texture_delete(Brush *brush)
{
	if(brush->mtex.tex)
		brush->mtex.tex->id.us--;

	return 1;
}

int brush_clone_image_set_nr(Brush *brush, int nr)
{
	if(brush && nr > 0) {
		Image *ima= (Image*)BLI_findlink(&G.main->image, nr-1);

		if(ima) {
			brush_clone_image_delete(brush);
			brush->clone.image= ima;
			id_us_plus(&ima->id);
			brush->clone.offset[0]= brush->clone.offset[1]= 0.0f;

			return 1;
		}
	}

	return 0;
}

int brush_clone_image_delete(Brush *brush)
{
	if (brush && brush->clone.image) {
		brush->clone.image->id.us--;
		brush->clone.image= NULL;
		return 1;
	}

	return 0;
}

void brush_check_exists(Brush **brush, const char *name)
{
	if(*brush==NULL)
		brush_set_nr(brush, 1, name);
}

/* Brush Sampling */
void brush_sample_tex(Brush *brush, float *xy, float *rgba)
{
	MTex *mtex= &brush->mtex;

	if (mtex && mtex->tex) {
		float co[3], tin, tr, tg, tb, ta;
		int hasrgb;
		const int radius= brush_size(brush);

		co[0]= xy[0]/radius;
		co[1]= xy[1]/radius;
		co[2]= 0.0f;

		hasrgb= externtex(mtex, co, &tin, &tr, &tg, &tb, &ta);

		if (hasrgb) {
			rgba[0]= tr;
			rgba[1]= tg;
			rgba[2]= tb;
			rgba[3]= ta;
		}
		else {
			rgba[0]= tin;
			rgba[1]= tin;
			rgba[2]= tin;
			rgba[3]= 1.0f;
		}
	}
	else if (rgba)
		rgba[0]= rgba[1]= rgba[2]= rgba[3]= 1.0f;
}


void brush_imbuf_new(Brush *brush, short flt, short texfall, int bufsize, ImBuf **outbuf)
{
	ImBuf *ibuf;
	float xy[2], dist, rgba[4], *dstf;
	int x, y, rowbytes, xoff, yoff, imbflag;
	const int radius= brush_size(brush);
	char *dst, crgb[3];
	const float alpha= brush_alpha(brush);

	imbflag= (flt)? IB_rectfloat: IB_rect;
	xoff = -bufsize/2.0f + 0.5f;
	yoff = -bufsize/2.0f + 0.5f;
	rowbytes= bufsize*4;

	if (*outbuf)
		ibuf= *outbuf;
	else
		ibuf= IMB_allocImBuf(bufsize, bufsize, 32, imbflag, 0);

	if (flt) {
		for (y=0; y < ibuf->y; y++) {
			dstf = ibuf->rect_float + y*rowbytes;

			for (x=0; x < ibuf->x; x++, dstf+=4) {
				xy[0] = x + xoff;
				xy[1] = y + yoff;

				if (texfall == 0) {
					dist = sqrt(xy[0]*xy[0] + xy[1]*xy[1]);

					VECCOPY(dstf, brush->rgb);
					dstf[3]= alpha*brush_curve_strength_clamp(brush, dist, radius);
				}
				else if (texfall == 1) {
					brush_sample_tex(brush, xy, dstf);
				}
				else {
					dist = sqrt(xy[0]*xy[0] + xy[1]*xy[1]);

					brush_sample_tex(brush, xy, rgba);

					dstf[0] = rgba[0]*brush->rgb[0];
					dstf[1] = rgba[1]*brush->rgb[1];
					dstf[2] = rgba[2]*brush->rgb[2];
					dstf[3] = rgba[3]*alpha*brush_curve_strength_clamp(brush, dist, radius);
				}
			}
		}
	}
	else {
		crgb[0]= FTOCHAR(brush->rgb[0]);
		crgb[1]= FTOCHAR(brush->rgb[1]);
		crgb[2]= FTOCHAR(brush->rgb[2]);

		for (y=0; y < ibuf->y; y++) {
			dst = (char*)ibuf->rect + y*rowbytes;

			for (x=0; x < ibuf->x; x++, dst+=4) {
				xy[0] = x + xoff;
				xy[1] = y + yoff;

				if (texfall == 0) {
					dist = sqrt(xy[0]*xy[0] + xy[1]*xy[1]);

					dst[0]= crgb[0];
					dst[1]= crgb[1];
					dst[2]= crgb[2];
					dst[3]= FTOCHAR(alpha*brush_curve_strength(brush, dist, radius));
				}
				else if (texfall == 1) {
					brush_sample_tex(brush, xy, rgba);
					dst[0]= FTOCHAR(rgba[0]);
					dst[1]= FTOCHAR(rgba[1]);
					dst[2]= FTOCHAR(rgba[2]);
					dst[3]= FTOCHAR(rgba[3]);
				}
				else {
					dist = sqrt(xy[0]*xy[0] + xy[1]*xy[1]);

					brush_sample_tex(brush, xy, rgba);
					dst[0] = FTOCHAR(rgba[0]*brush->rgb[0]);
					dst[1] = FTOCHAR(rgba[1]*brush->rgb[1]);
					dst[2] = FTOCHAR(rgba[2]*brush->rgb[2]);
					dst[3] = FTOCHAR(rgba[3]*alpha*brush_curve_strength_clamp(brush, dist, radius));
				}
			}
		}
	}

	*outbuf= ibuf;
}

/* Brush Painting */

typedef struct BrushPainterCache {
	short enabled;

	int size;			/* size override, if 0 uses 2*brush_size(brush) */
	short flt;			/* need float imbuf? */
	short texonly;		/* no alpha, color or fallof, only texture in imbuf */

	int lastsize;
	float lastalpha;
	float lastjitter;

	ImBuf *ibuf;
	ImBuf *texibuf;
	ImBuf *maskibuf;
} BrushPainterCache;

struct BrushPainter {
	Brush *brush;

	float lastmousepos[2];	/* mouse position of last paint call */

	float accumdistance;	/* accumulated distance of brush since last paint op */
	float lastpaintpos[2];	/* position of last paint op */
	float startpaintpos[2]; /* position of first paint */

	double accumtime;		/* accumulated time since last paint op (airbrush) */
	double lasttime;		/* time of last update */

	float lastpressure;

	short firsttouch;		/* first paint op */

	float startsize;
	float startalpha;
	float startjitter;
	float startspacing;

	BrushPainterCache cache;
};

BrushPainter *brush_painter_new(Brush *brush)
{
	BrushPainter *painter= MEM_callocN(sizeof(BrushPainter), "BrushPainter");

	painter->brush= brush;
	painter->firsttouch= 1;
	painter->cache.lastsize= -1; /* force ibuf create in refresh */

	painter->startsize = brush_size(brush);
	painter->startalpha = brush_alpha(brush);
	painter->startjitter = brush->jitter;
	painter->startspacing = brush->spacing;

	return painter;
}

void brush_painter_require_imbuf(BrushPainter *painter, short flt, short texonly, int size)
{
	if ((painter->cache.flt != flt) || (painter->cache.size != size) ||
		((painter->cache.texonly != texonly) && texonly)) {
		if (painter->cache.ibuf) IMB_freeImBuf(painter->cache.ibuf);
		if (painter->cache.maskibuf) IMB_freeImBuf(painter->cache.maskibuf);
		painter->cache.ibuf= painter->cache.maskibuf= NULL;
		painter->cache.lastsize= -1; /* force ibuf create in refresh */
	}

	if (painter->cache.flt != flt) {
		if (painter->cache.texibuf) IMB_freeImBuf(painter->cache.texibuf);
		painter->cache.texibuf= NULL;
		painter->cache.lastsize= -1; /* force ibuf create in refresh */
	}

	painter->cache.size= size;
	painter->cache.flt= flt;
	painter->cache.texonly= texonly;
	painter->cache.enabled= 1;
}

void brush_painter_free(BrushPainter *painter)
{
	Brush *brush = painter->brush;

	brush_set_size(brush, painter->startsize);
	brush_set_alpha(brush, painter->startalpha);
	brush->jitter = painter->startjitter;
	brush->spacing = painter->startspacing;

	if (painter->cache.ibuf) IMB_freeImBuf(painter->cache.ibuf);
	if (painter->cache.texibuf) IMB_freeImBuf(painter->cache.texibuf);
	if (painter->cache.maskibuf) IMB_freeImBuf(painter->cache.maskibuf);
	MEM_freeN(painter);
}

static void brush_painter_do_partial(BrushPainter *painter, ImBuf *oldtexibuf, int x, int y, int w, int h, int xt, int yt, float *pos)
{
	Brush *brush= painter->brush;
	ImBuf *ibuf, *maskibuf, *texibuf;
	float *bf, *mf, *tf, *otf=NULL, xoff, yoff, xy[2], rgba[4];
	char *b, *m, *t, *ot= NULL;
	int dotexold, origx= x, origy= y;
	const int radius= brush_size(brush);

	xoff = -radius + 0.5f;
	yoff = -radius + 0.5f;
	xoff += (int)pos[0] - (int)painter->startpaintpos[0];
	yoff += (int)pos[1] - (int)painter->startpaintpos[1];

	ibuf = painter->cache.ibuf;
	texibuf = painter->cache.texibuf;
	maskibuf = painter->cache.maskibuf;

	dotexold = (oldtexibuf != NULL);

	if (painter->cache.flt) {
		for (; y < h; y++) {
			bf = ibuf->rect_float + (y*ibuf->x + origx)*4;
			tf = texibuf->rect_float + (y*texibuf->x + origx)*4;
			mf = maskibuf->rect_float + (y*maskibuf->x + origx)*4;

			if (dotexold)
				otf = oldtexibuf->rect_float + ((y - origy + yt)*oldtexibuf->x + xt)*4;

			for (x=origx; x < w; x++, bf+=4, mf+=4, tf+=4) {
				if (dotexold) {
					VECCOPY(tf, otf);
					tf[3] = otf[3];
					otf += 4;
				}
				else {
					xy[0] = x + xoff;
					xy[1] = y + yoff;

					brush_sample_tex(brush, xy, tf);
				}

				bf[0] = tf[0]*mf[0];
				bf[1] = tf[1]*mf[1];
				bf[2] = tf[2]*mf[2];
				bf[3] = tf[3]*mf[3];
			}
		}
	}
	else {
		for (; y < h; y++) {
			b = (char*)ibuf->rect + (y*ibuf->x + origx)*4;
			t = (char*)texibuf->rect + (y*texibuf->x + origx)*4;
			m = (char*)maskibuf->rect + (y*maskibuf->x + origx)*4;

			if (dotexold)
				ot = (char*)oldtexibuf->rect + ((y - origy + yt)*oldtexibuf->x + xt)*4;

			for (x=origx; x < w; x++, b+=4, m+=4, t+=4) {
				if (dotexold) {
					t[0] = ot[0];
					t[1] = ot[1];
					t[2] = ot[2];
					t[3] = ot[3];
					ot += 4;
				}
				else {
					xy[0] = x + xoff;
					xy[1] = y + yoff;

					brush_sample_tex(brush, xy, rgba);
					t[0]= FTOCHAR(rgba[0]);
					t[1]= FTOCHAR(rgba[1]);
					t[2]= FTOCHAR(rgba[2]);
					t[3]= FTOCHAR(rgba[3]);
				}

				b[0] = t[0]*m[0]/255;
				b[1] = t[1]*m[1]/255;
				b[2] = t[2]*m[2]/255;
				b[3] = t[3]*m[3]/255;
			}
		}
	}
}

static void brush_painter_fixed_tex_partial_update(BrushPainter *painter, float *pos)
{
	Brush *brush= painter->brush;
	BrushPainterCache *cache= &painter->cache;
	ImBuf *oldtexibuf, *ibuf;
	int imbflag, destx, desty, srcx, srcy, w, h, x1, y1, x2, y2;
	const int diameter= 2*brush_size(brush);

	imbflag= (cache->flt)? IB_rectfloat: IB_rect;
	if (!cache->ibuf)
		cache->ibuf= IMB_allocImBuf(diameter, diameter, 32, imbflag, 0);
	ibuf= cache->ibuf;

	oldtexibuf= cache->texibuf;
	cache->texibuf= IMB_allocImBuf(diameter, diameter, 32, imbflag, 0);

	if (oldtexibuf) {
		srcx= srcy= 0;
		destx= (int)painter->lastpaintpos[0] - (int)pos[0];
		desty= (int)painter->lastpaintpos[1] - (int)pos[1];
		w= oldtexibuf->x;
		h= oldtexibuf->y;

		IMB_rectclip(cache->texibuf, oldtexibuf, &destx, &desty, &srcx, &srcy, &w, &h);
	}
	else {
		srcx= srcy= 0;
		destx= desty= 0;
		w= h= 0;
	}
	
	x1= destx;
	y1= desty;
	x2= destx+w;
	y2= desty+h;

	/* blend existing texture in new position */
	if ((x1 < x2) && (y1 < y2))
		brush_painter_do_partial(painter, oldtexibuf, x1, y1, x2, y2, srcx, srcy, pos);

	if (oldtexibuf)
		IMB_freeImBuf(oldtexibuf);

	/* sample texture in new areas */
	if ((0 < x1) && (0 < ibuf->y))
		brush_painter_do_partial(painter, NULL, 0, 0, x1, ibuf->y, 0, 0, pos);
	if ((x2 < ibuf->x) && (0 < ibuf->y))
		brush_painter_do_partial(painter, NULL, x2, 0, ibuf->x, ibuf->y, 0, 0, pos);
	if ((x1 < x2) && (0 < y1))
		brush_painter_do_partial(painter, NULL, x1, 0, x2, y1, 0, 0, pos);
	if ((x1 < x2) && (y2 < ibuf->y))
		brush_painter_do_partial(painter, NULL, x1, y2, x2, ibuf->y, 0, 0, pos);
}

static void brush_painter_refresh_cache(BrushPainter *painter, float *pos)
{
	Brush *brush= painter->brush;
	BrushPainterCache *cache= &painter->cache;
	MTex *mtex= &brush->mtex;
	int size;
	short flt;
	const int diameter= 2*brush_size(brush);
	const float alpha= brush_alpha(brush);

	if (diameter != cache->lastsize ||
		alpha != cache->lastalpha ||
		brush->jitter != cache->lastjitter)
	{
		if (cache->ibuf) {
			IMB_freeImBuf(cache->ibuf);
			cache->ibuf= NULL;
		}
		if (cache->maskibuf) {
			IMB_freeImBuf(cache->maskibuf);
			cache->maskibuf= NULL;
		}

		flt= cache->flt;
		size= (cache->size)? cache->size: diameter;

		if (!(mtex && mtex->tex) || (mtex->tex->type==0)) {
			brush_imbuf_new(brush, flt, 0, size, &cache->ibuf);
		}
		else if (brush->flag & BRUSH_FIXED_TEX) {
			brush_imbuf_new(brush, flt, 0, size, &cache->maskibuf);
			brush_painter_fixed_tex_partial_update(painter, pos);
		}
		else
			brush_imbuf_new(brush, flt, 2, size, &cache->ibuf);

		cache->lastsize= diameter;
		cache->lastalpha= alpha;
		cache->lastjitter= brush->jitter;
	}
	else if ((brush->flag & BRUSH_FIXED_TEX) && mtex && mtex->tex) {
		int dx = (int)painter->lastpaintpos[0] - (int)pos[0];
		int dy = (int)painter->lastpaintpos[1] - (int)pos[1];

		if ((dx != 0) || (dy != 0))
			brush_painter_fixed_tex_partial_update(painter, pos);
	}
}

void brush_painter_break_stroke(BrushPainter *painter)
{
	painter->firsttouch= 1;
}

static void brush_apply_pressure(BrushPainter *painter, Brush *brush, float pressure)
{
	if (brush_use_alpha_pressure(brush)) 
		brush_set_alpha(brush, MAX2(0.0, painter->startalpha*pressure));
	if (brush_use_size_pressure(brush))
		brush_set_size(brush, MAX2(1.0, painter->startsize*pressure));
	if (brush->flag & BRUSH_JITTER_PRESSURE)
		brush->jitter = MAX2(0.0, painter->startjitter*pressure);
	if (brush->flag & BRUSH_SPACING_PRESSURE)
		brush->spacing = MAX2(1.0, painter->startspacing*(1.5f-pressure));
}

void brush_jitter_pos(Brush *brush, float *pos, float *jitterpos)
{
	if(brush->jitter){
		float rand_pos[2];
		const int radius= brush_size(brush);
		const int diameter= 2*radius;

		// find random position within a circle of diameter 1
		do {
			rand_pos[0] = BLI_frand()-0.5f;
			rand_pos[1] = BLI_frand()-0.5f;
		} while (len_v2(rand_pos) > 0.5f);

		jitterpos[0] = pos[0] + 2*rand_pos[0]*diameter*brush->jitter;
		jitterpos[1] = pos[1] + 2*rand_pos[1]*diameter*brush->jitter;
	}
	else {
		VECCOPY2D(jitterpos, pos);
	}
}

int brush_painter_paint(BrushPainter *painter, BrushFunc func, float *pos, double time, float pressure, void *user)
{
	Brush *brush= painter->brush;
	int totpaintops= 0;

	if (pressure == 0.0f) {
		if(painter->lastpressure) // XXX - hack, operator misses
			pressure= painter->lastpressure;
		else
			pressure = 1.0f;	/* zero pressure == not using tablet */
	}
	if (painter->firsttouch) {
		/* paint exactly once on first touch */
		painter->startpaintpos[0]= pos[0];
		painter->startpaintpos[1]= pos[1];

		brush_apply_pressure(painter, brush, pressure);
		if (painter->cache.enabled)
			brush_painter_refresh_cache(painter, pos);
		totpaintops += func(user, painter->cache.ibuf, pos, pos);
		
		painter->lasttime= time;
		painter->firsttouch= 0;
		painter->lastpaintpos[0]= pos[0];
		painter->lastpaintpos[1]= pos[1];
	}
#if 0
	else if (painter->brush->flag & BRUSH_AIRBRUSH) {
		float spacing, step, paintpos[2], dmousepos[2], len;
		double starttime, curtime= time;

		/* compute brush spacing adapted to brush size */
		spacing= brush->rate; //radius*brush->spacing*0.01f;

		/* setup starting time, direction vector and accumulated time */
		starttime= painter->accumtime;
		sub_v2_v2v2(dmousepos, pos, painter->lastmousepos);
		len= normalize_v2(dmousepos);
		painter->accumtime += curtime - painter->lasttime;

		/* do paint op over unpainted time distance */
		while (painter->accumtime >= spacing) {
			step= (spacing - starttime)*len;
			paintpos[0]= painter->lastmousepos[0] + dmousepos[0]*step;
			paintpos[1]= painter->lastmousepos[1] + dmousepos[1]*step;

			if (painter->cache.enabled)
				brush_painter_refresh_cache(painter);
			totpaintops += func(user, painter->cache.ibuf,
				painter->lastpaintpos, paintpos);

			painter->lastpaintpos[0]= paintpos[0];
			painter->lastpaintpos[1]= paintpos[1];
			painter->accumtime -= spacing;
			starttime -= spacing;
		}
		
		painter->lasttime= curtime;
	}
#endif
	else {
		float startdistance, spacing, step, paintpos[2], dmousepos[2], finalpos[2];
		float t, len, press;
		const int radius= brush_size(brush);
		const int diameter= 2*radius;

		/* compute brush spacing adapted to brush radius, spacing may depend
		   on pressure, so update it */
		brush_apply_pressure(painter, brush, painter->lastpressure);
		spacing= MAX2(1.0f, radius)*brush->spacing*0.01f;

		/* setup starting distance, direction vector and accumulated distance */
		startdistance= painter->accumdistance;
		sub_v2_v2v2(dmousepos, pos, painter->lastmousepos);
		len= normalize_v2(dmousepos);
		painter->accumdistance += len;

		/* do paint op over unpainted distance */
		while ((len > 0.0f) && (painter->accumdistance >= spacing)) {
			step= spacing - startdistance;
			paintpos[0]= painter->lastmousepos[0] + dmousepos[0]*step;
			paintpos[1]= painter->lastmousepos[1] + dmousepos[1]*step;

			t = step/len;
			press= (1.0f-t)*painter->lastpressure + t*pressure;
			brush_apply_pressure(painter, brush, press);
			spacing= MAX2(1.0f, radius)*brush->spacing*0.01f;

			brush_jitter_pos(brush, paintpos, finalpos);

			if (painter->cache.enabled)
				brush_painter_refresh_cache(painter, finalpos);

			totpaintops +=
				func(user, painter->cache.ibuf, painter->lastpaintpos, finalpos);

			painter->lastpaintpos[0]= paintpos[0];
			painter->lastpaintpos[1]= paintpos[1];
			painter->accumdistance -= spacing;
			startdistance -= spacing;
		}

		/* do airbrush paint ops, based on the number of paint ops left over
		   from regular painting. this is a temporary solution until we have
		   accurate time stamps for mouse move events */
		if (brush->flag & BRUSH_AIRBRUSH) {
			double curtime= time;
			double painttime= brush->rate*totpaintops;

			painter->accumtime += curtime - painter->lasttime;
			if (painter->accumtime <= painttime)
				painter->accumtime= 0.0;
			else
				painter->accumtime -= painttime;

			while (painter->accumtime >= brush->rate) {
				brush_apply_pressure(painter, brush, pressure);

				brush_jitter_pos(brush, pos, finalpos);

				if (painter->cache.enabled)
					brush_painter_refresh_cache(painter, finalpos);

				totpaintops +=
					func(user, painter->cache.ibuf, painter->lastmousepos, finalpos);
				painter->accumtime -= brush->rate;
			}

			painter->lasttime= curtime;
		}
	}

	painter->lastmousepos[0]= pos[0];
	painter->lastmousepos[1]= pos[1];
	painter->lastpressure= pressure;

	brush_set_alpha(brush, painter->startalpha);
	brush_set_size(brush, painter->startsize);
	brush->jitter = painter->startjitter;
	brush->spacing = painter->startspacing;

	return totpaintops;
}

/* Uses the brush curve control to find a strength value between 0 and 1 */
float brush_curve_strength_clamp(Brush *br, float p, const float len)
{
	if(p >= len)	return 0;
	else			p= p/len;

	p= curvemapping_evaluateF(br->curve, 0, p);
	if(p < 0.0)			p= 0.0f;
	else if(p > 1.0f)	p= 1.0f;
	return p;
}
/* same as above but can return negative values if the curve enables
 * used for sculpt only */
float brush_curve_strength(Brush *br, float p, const float len)
{
    if(p >= len)
        p= 1.0f;
    else
        p= p/len;

    return curvemapping_evaluateF(br->curve, 0, p);
}

/* TODO: should probably be unified with BrushPainter stuff? */
unsigned int *brush_gen_texture_cache(Brush *br, int half_side)
{
	unsigned int *texcache = NULL;
	MTex *mtex = &br->mtex;
	TexResult texres;
	int hasrgb, ix, iy;
	int side = half_side * 2;

	memset(&texres, 0, sizeof(TexResult));
	
	if(mtex->tex) {
		float x, y, step = 2.0 / side, co[3];

		texcache = MEM_callocN(sizeof(int) * side * side, "Brush texture cache");

		BKE_image_get_ibuf(mtex->tex->ima, NULL);
		
		/*do normalized cannonical view coords for texture*/
		for (y=-1.0, iy=0; iy<side; iy++, y += step) {
			for (x=-1.0, ix=0; ix<side; ix++, x += step) {
				co[0]= x;
				co[1]= y;
				co[2]= 0.0f;
				
				/* This is copied from displace modifier code */
				hasrgb = multitex_ext(mtex->tex, co, NULL, NULL, 0, &texres);
			
				/* if the texture gave an RGB value, we assume it didn't give a valid
				 * intensity, so calculate one (formula from do_material_tex).
				 * if the texture didn't give an RGB value, copy the intensity across
				 */
				if(hasrgb & TEX_RGB)
					texres.tin = (0.35 * texres.tr + 0.45 *
								  texres.tg + 0.2 * texres.tb);

				texres.tin = texres.tin * 255.0;
				((char*)texcache)[(iy*side+ix)*4] = (char)texres.tin;
				((char*)texcache)[(iy*side+ix)*4+1] = (char)texres.tin;
				((char*)texcache)[(iy*side+ix)*4+2] = (char)texres.tin;
				((char*)texcache)[(iy*side+ix)*4+3] = (char)texres.tin;
			}
		}
	}

	return texcache;
}

/**** Radial Control ****/
static struct ImBuf *brush_gen_radial_control_imbuf(Brush *br)
{
	ImBuf *im = MEM_callocN(sizeof(ImBuf), "radial control texture");
	unsigned int *texcache;
	int side = 128;
	int half = side / 2;
	int i, j;

	texcache = brush_gen_texture_cache(br, half);
	im->rect_float = MEM_callocN(sizeof(float) * side * side, "radial control rect");
	im->x = im->y = side;

	for(i=0; i<side; ++i) {
		for(j=0; j<side; ++j) {
			float magn= sqrt(pow(i - half, 2) + pow(j - half, 2));
			im->rect_float[i*side + j]= brush_curve_strength_clamp(br, magn, half);
		}
	}

	/* Modulate curve with texture */
	if(texcache) {
		for(i=0; i<side; ++i) {
			for(j=0; j<side; ++j) {
				const int col= texcache[i*side+j];
				im->rect_float[i*side+j]*= (((char*)&col)[0]+((char*)&col)[1]+((char*)&col)[2])/3.0f/255.0f;
			}
		}

		MEM_freeN(texcache);
	}

	return im;
}

void brush_radial_control_invoke(wmOperator *op, Brush *br, float size_weight)
{
	int mode = RNA_int_get(op->ptr, "mode");
	float original_value= 0;

	if(mode == WM_RADIALCONTROL_SIZE)
		original_value = brush_size(br) * size_weight;
	else if(mode == WM_RADIALCONTROL_STRENGTH)
		original_value = brush_alpha(br);
	else if(mode == WM_RADIALCONTROL_ANGLE) {
		MTex *mtex = brush_active_texture(br);
		if(mtex)
			original_value = mtex->rot;
	}

	RNA_float_set(op->ptr, "initial_value", original_value);
	op->customdata = brush_gen_radial_control_imbuf(br);
}

int brush_radial_control_exec(wmOperator *op, Brush *br, float size_weight)
{
	int mode = RNA_int_get(op->ptr, "mode");
	float new_value = RNA_float_get(op->ptr, "new_value");
	const float conv = 0.017453293;

	if(mode == WM_RADIALCONTROL_SIZE)
		if (brush_use_locked_size(br)) {
			float initial_value = RNA_float_get(op->ptr, "initial_value");
			const float unprojected_radius = brush_unprojected_radius(br);
			brush_set_unprojected_radius(br, unprojected_radius * new_value/initial_value * size_weight);
		}
		else
			brush_set_size(br, new_value * size_weight);
	else if(mode == WM_RADIALCONTROL_STRENGTH)
		brush_set_alpha(br, new_value);
	else if(mode == WM_RADIALCONTROL_ANGLE) {
		MTex *mtex = brush_active_texture(br);
		if(mtex)
			mtex->rot = new_value * conv;
	}

	return OPERATOR_FINISHED;
}

/* Unified Size and Strength */

static void set_unified_settings(Brush *brush, short flag, int value)
{
	Scene *sce;
	for (sce= G.main->scene.first; sce; sce= sce->id.next) {
		if (sce->toolsettings && 
			ELEM4(brush,
			    paint_brush(&(sce->toolsettings->imapaint.paint)),
			    paint_brush(&(sce->toolsettings->vpaint->paint)),
			    paint_brush(&(sce->toolsettings->wpaint->paint)),
			    paint_brush(&(sce->toolsettings->sculpt->paint))))
		{
			if (value)
				sce->toolsettings->sculpt_paint_settings |= flag;
			else
				sce->toolsettings->sculpt_paint_settings &= ~flag;
		}
	}
}

static short unified_settings(Brush *brush)
{
	Scene *sce;
	for (sce= G.main->scene.first; sce; sce= sce->id.next) {
		if (sce->toolsettings && 
			ELEM4(brush,
			    paint_brush(&(sce->toolsettings->imapaint.paint)),
			    paint_brush(&(sce->toolsettings->vpaint->paint)),
			    paint_brush(&(sce->toolsettings->wpaint->paint)),
			    paint_brush(&(sce->toolsettings->sculpt->paint))))
		{
			return sce->toolsettings->sculpt_paint_settings;
		}
	}

	return 0;
}

static void set_unified_size(Brush *brush, int value)
{
	Scene *sce;
	for (sce= G.main->scene.first; sce; sce= sce->id.next) {
		if (sce->toolsettings && 
			ELEM4(brush,
			    paint_brush(&(sce->toolsettings->imapaint.paint)),
			    paint_brush(&(sce->toolsettings->vpaint->paint)),
			    paint_brush(&(sce->toolsettings->wpaint->paint)),
			    paint_brush(&(sce->toolsettings->sculpt->paint))))
		{
			sce->toolsettings->sculpt_paint_unified_size= value;
		}
	}
}

static int unified_size(Brush *brush)
{
	Scene *sce;
	for (sce= G.main->scene.first; sce; sce= sce->id.next) {
		if (sce->toolsettings && 
			ELEM4(brush,
			    paint_brush(&(sce->toolsettings->imapaint.paint)),
			    paint_brush(&(sce->toolsettings->vpaint->paint)),
			    paint_brush(&(sce->toolsettings->wpaint->paint)),
			    paint_brush(&(sce->toolsettings->sculpt->paint))))
		{
			return sce->toolsettings->sculpt_paint_unified_size;
		}
	}

	return 35; // XXX magic number
}

static void set_unified_alpha(Brush *brush, float value)
{
	Scene *sce;
	for (sce= G.main->scene.first; sce; sce= sce->id.next) {
		if (sce->toolsettings && 
			ELEM4(brush,
			    paint_brush(&(sce->toolsettings->imapaint.paint)),
			    paint_brush(&(sce->toolsettings->vpaint->paint)),
			    paint_brush(&(sce->toolsettings->wpaint->paint)),
			    paint_brush(&(sce->toolsettings->sculpt->paint))))
		{
			sce->toolsettings->sculpt_paint_unified_alpha= value;
		}
	}
}

static float unified_alpha(Brush *brush)
{
	Scene *sce;
	for (sce= G.main->scene.first; sce; sce= sce->id.next) {
		if (sce->toolsettings && 
			ELEM4(brush,
			    paint_brush(&(sce->toolsettings->imapaint.paint)),
			    paint_brush(&(sce->toolsettings->vpaint->paint)),
			    paint_brush(&(sce->toolsettings->wpaint->paint)),
			    paint_brush(&(sce->toolsettings->sculpt->paint))))
		{
			return sce->toolsettings->sculpt_paint_unified_alpha;
		}
	}

	return 0.5f; // XXX magic number
}

static void set_unified_unprojected_radius(Brush *brush, float value)
{
	Scene *sce;
	for (sce= G.main->scene.first; sce; sce= sce->id.next) {
		if (sce->toolsettings && 
			ELEM4(brush,
			    paint_brush(&(sce->toolsettings->imapaint.paint)),
			    paint_brush(&(sce->toolsettings->vpaint->paint)),
			    paint_brush(&(sce->toolsettings->wpaint->paint)),
			    paint_brush(&(sce->toolsettings->sculpt->paint))))
		{
			sce->toolsettings->sculpt_paint_unified_unprojected_radius= value;
		}
	}
}

static float unified_unprojected_radius(Brush *brush)
{
	Scene *sce;
	for (sce= G.main->scene.first; sce; sce= sce->id.next) {
		if (sce->toolsettings && 
			ELEM4(brush,
			    paint_brush(&(sce->toolsettings->imapaint.paint)),
			    paint_brush(&(sce->toolsettings->vpaint->paint)),
			    paint_brush(&(sce->toolsettings->wpaint->paint)),
			    paint_brush(&(sce->toolsettings->sculpt->paint))))
		{
			return sce->toolsettings->sculpt_paint_unified_unprojected_radius;
		}
	}

	return 0.125f; // XXX magic number
}
void brush_set_size(Brush *brush, int size)
{
	if (unified_settings(brush) & SCULPT_PAINT_USE_UNIFIED_SIZE)
		set_unified_size(brush, size);
	else
		brush->size= size;

	WM_main_add_notifier(NC_BRUSH|NA_EDITED, brush);
}

int brush_size(Brush *brush)
{
	return (unified_settings(brush) & SCULPT_PAINT_USE_UNIFIED_SIZE) ? unified_size(brush) : brush->size;
}

void brush_set_use_locked_size(Brush *brush, int value)
{
	if (unified_settings(brush) & SCULPT_PAINT_USE_UNIFIED_SIZE) {
		set_unified_settings(brush, SCULPT_PAINT_UNIFIED_LOCK_BRUSH_SIZE, value);
	}
	else {
		if (value)
			brush->flag |= BRUSH_LOCK_SIZE;
		else
			brush->flag &= ~BRUSH_LOCK_SIZE;
	}

	WM_main_add_notifier(NC_BRUSH|NA_EDITED, brush);
}

int brush_use_locked_size(Brush *brush)
{
	return (unified_settings(brush) & SCULPT_PAINT_USE_UNIFIED_SIZE) ? (unified_settings(brush) & SCULPT_PAINT_UNIFIED_LOCK_BRUSH_SIZE) : (brush->flag & BRUSH_LOCK_SIZE);
}

void brush_set_use_size_pressure(Brush *brush, int value)
{
	if (unified_settings(brush) & SCULPT_PAINT_USE_UNIFIED_SIZE) {
		set_unified_settings(brush, SCULPT_PAINT_UNIFIED_SIZE_PRESSURE, value);
	}
	else {
		if (value)
			brush->flag |= BRUSH_SIZE_PRESSURE;
		else
			brush->flag &= ~BRUSH_SIZE_PRESSURE;
	}

	WM_main_add_notifier(NC_BRUSH|NA_EDITED, brush);
}

int brush_use_size_pressure(Brush *brush)
{
	return (unified_settings(brush) & SCULPT_PAINT_USE_UNIFIED_SIZE) ? (unified_settings(brush) & SCULPT_PAINT_UNIFIED_SIZE_PRESSURE) : (brush->flag & BRUSH_SIZE_PRESSURE);
}

void brush_set_use_alpha_pressure(Brush *brush, int value)
{
	if (unified_settings(brush) & SCULPT_PAINT_USE_UNIFIED_ALPHA) {
		set_unified_settings(brush, SCULPT_PAINT_UNIFIED_ALPHA_PRESSURE, value);
	}
	else {
		if (value)
			brush->flag |= BRUSH_ALPHA_PRESSURE;
		else
			brush->flag &= ~BRUSH_ALPHA_PRESSURE;
	}

	WM_main_add_notifier(NC_BRUSH|NA_EDITED, brush);
}

int brush_use_alpha_pressure(Brush *brush)
{
	return (unified_settings(brush) & SCULPT_PAINT_USE_UNIFIED_ALPHA) ? (unified_settings(brush) & SCULPT_PAINT_UNIFIED_ALPHA_PRESSURE) : (brush->flag & BRUSH_ALPHA_PRESSURE);
}

void brush_set_unprojected_radius(Brush *brush, float unprojected_radius)
{
	if (unified_settings(brush) & SCULPT_PAINT_USE_UNIFIED_SIZE)
		set_unified_unprojected_radius(brush, unprojected_radius);
	else
		brush->unprojected_radius= unprojected_radius;

	WM_main_add_notifier(NC_BRUSH|NA_EDITED, brush);
}

float brush_unprojected_radius(Brush *brush)
{
	return (unified_settings(brush) & SCULPT_PAINT_USE_UNIFIED_SIZE) ? unified_unprojected_radius(brush) : brush->unprojected_radius;
}

void brush_set_alpha(Brush *brush, float alpha)
{
	if (unified_settings(brush) & SCULPT_PAINT_USE_UNIFIED_ALPHA) 
		set_unified_alpha(brush, alpha);
	else
		brush->alpha= alpha;

	WM_main_add_notifier(NC_BRUSH|NA_EDITED, brush);
}

float brush_alpha(Brush *brush)
{
	return (unified_settings(brush) & SCULPT_PAINT_USE_UNIFIED_ALPHA) ? unified_alpha(brush) : brush->alpha;
}
