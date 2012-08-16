/*
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
 * The Original Code is Copyright (C) 2009 by Nicholas Bishop
 * All rights reserved.
 *
 * Contributor(s): Jason Wilkins, Tom Musgrove.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

/** \file blender/editors/sculpt_paint/paint_cursor.c
 *  \ingroup edsculpt
 */

#include "MEM_guardedalloc.h"

#include "BLI_math.h"
#include "BLI_utildefines.h"

#include "DNA_brush_types.h"
#include "DNA_color_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"
#include "DNA_screen_types.h"
#include "DNA_userdef_types.h"
#include "DNA_space_types.h"

#include "BKE_brush.h"
#include "BKE_context.h"
#include "BKE_paint.h"

#include "WM_api.h"

#include "BIF_gl.h"
#include "BIF_glutil.h"

#include "ED_view3d.h"

#include "paint_intern.h"
/* still needed for sculpt_stroke_get_location, should be
 * removed eventually (TODO) */
#include "sculpt_intern.h"

#define CURSOR_RESOLUTION 40


/* TODOs:
 *
 * Some of the cursor drawing code is doing non-draw stuff
 * (e.g. updating the brush rake angle). This should be cleaned up
 * still.
 *
 * There is also some ugliness with sculpt-specific code.
 */

typedef struct SnapshotAlpha {
	int BKE_brush_size_get;
	int alpha_changed_timestamp;
} SnapshotAlpha;

typedef struct SnapshotTex {
	float size[3];
	float ofs[3];
	float rot;
	int BKE_brush_size_get;
	int winx;
	int winy;
	int brush_map_mode;
} SnapshotTex;

static int same_snap_image(SnapshotTex *snap, Brush *brush, ViewContext *vc)
{
	MTex *mtex = &brush->mtex;

	return (((mtex->tex) &&
	         equals_v3v3(mtex->ofs, snap->ofs) &&
	         equals_v3v3(mtex->size, snap->size) &&
	         mtex->rot == snap->rot) &&

	        /* make brush smaller shouldn't cause a resample */
	        ((mtex->brush_map_mode == MTEX_MAP_MODE_VIEW &&
	          (BKE_brush_size_get(vc->scene, brush) <= snap->BKE_brush_size_get)) ||
	         (BKE_brush_size_get(vc->scene, brush) == snap->BKE_brush_size_get)) &&

	        (mtex->brush_map_mode == snap->brush_map_mode) &&
	        (vc->ar->winx == snap->winx) &&
	        (vc->ar->winy == snap->winy));
}

static void make_snap_image(SnapshotTex *snap, Brush *brush, ViewContext *vc)
{
	if (brush->mtex.tex) {
		snap->brush_map_mode = brush->mtex.brush_map_mode;
		copy_v3_v3(snap->ofs, brush->mtex.ofs);
		copy_v3_v3(snap->size, brush->mtex.size);
		snap->rot = brush->mtex.rot;
	}
	else {
		snap->brush_map_mode = -1;
		snap->ofs[0] = snap->ofs[1] = snap->ofs[2] = -1;
		snap->size[0] = snap->size[1] = snap->size[2] = -1;
		snap->rot = -1;
	}

	snap->BKE_brush_size_get = BKE_brush_size_get(vc->scene, brush);
	snap->winx = vc->ar->winx;
	snap->winy = vc->ar->winy;
}

static int load_brush_tex_alpha(Brush *br, ViewContext *vc)
{
	static GLuint overlay_texture_alpha = 0;
	static int init_alpha = 0;
	static SnapshotAlpha snap;
	static int old_alpha_size = -1;

	GLubyte *alpha_buffer = NULL;

	/*
	char do_tiled_texpaint = (br->mtex.brush_map_mode == MTEX_MAP_MODE_TILED_TEXPAINT);
	char do_tiled = (br->mtex.brush_map_mode == MTEX_MAP_MODE_TILED) || do_tiled_texpaint;
	*/
	int alpha_size;
	int j;
	int refresh;

	//if (do_tiled && !br->mask_mtex.tex) return 0;

	refresh =
			(snap.BKE_brush_size_get != BKE_brush_size_get(vc->scene, br)) ||
			!br->curve ||
			br->curve->changed_timestamp != snap.alpha_changed_timestamp ||
			!overlay_texture_alpha;

	if (refresh) {
		int s = BKE_brush_size_get(vc->scene, br);
		int r = 1;

		if (br->curve)
			snap.alpha_changed_timestamp = br->curve->changed_timestamp;

		snap.BKE_brush_size_get = BKE_brush_size_get(vc->scene, br);

		for (s >>= 1; s > 0; s >>= 1)
			r++;

		alpha_size = (1 << r);

		if (alpha_size < 256)
			alpha_size = 256;

		if (alpha_size < old_alpha_size)
			alpha_size = old_alpha_size;

		if (old_alpha_size != alpha_size) {
			if (overlay_texture_alpha) {
				glDeleteTextures(1, &overlay_texture_alpha);
				overlay_texture_alpha = 0;
			}

			init_alpha = 0;

			old_alpha_size = alpha_size;
		}

		alpha_buffer = MEM_mallocN(sizeof(GLubyte) * alpha_size * alpha_size, "load_tex_curve");

		/* dummy call to avoid generating curve tables in openmp, causes memory leaks since allocation
		 * is not thread safe */
		BKE_brush_curve_strength(br, 0.5, 1);

		#pragma omp parallel for schedule(static)
		for (j = 0; j < alpha_size; j++) {
			int i;
			float y;
			float len;

			for (i = 0; i < alpha_size; i++) {
				int index = j * alpha_size + i;
				float x;
				float avg;

				x = (float)i / alpha_size;
				y = (float)j / alpha_size;

				x -= 0.5f;
				y -= 0.5f;

				x *= 2;
				y *= 2;

				len = sqrtf(x * x + y * y);

				if (len <= 1) {
					x *= br->mtex.size[0];
					y *= br->mtex.size[1];

					x += br->mtex.ofs[0];
					y += br->mtex.ofs[1];

					avg = BKE_brush_curve_strength(br, len, 1);  /* Falloff curve */

					if(br->flag & BRUSH_USE_MASK) {
						avg *= br->mask_mtex.tex ? paint_get_tex_pixel(br, x, y, TRUE) : 1;
					}
					CLAMP(avg, 0.0, 1.0);

					alpha_buffer[index] = (GLubyte)(255 * avg);
				}
				else {
					alpha_buffer[index] = 0;
				}
			}
		}

		if (!overlay_texture_alpha)
			glGenTextures(1, &overlay_texture_alpha);
	}

	/* switch to second texture unit */
	glBindTexture(GL_TEXTURE_2D, overlay_texture_alpha);

	if (refresh) {
		if (!init_alpha) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, alpha_size, alpha_size, 0, GL_ALPHA, GL_UNSIGNED_BYTE, alpha_buffer);
			init_alpha = 1;
		}
		else {
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, alpha_size, alpha_size, GL_ALPHA, GL_UNSIGNED_BYTE, alpha_buffer);
		}

		if(alpha_buffer)
			MEM_freeN(alpha_buffer);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glEnable(GL_TEXTURE_2D);

	return 1;
}

static int load_brush_tex_image(Brush *br, ViewContext *vc)
{
	static GLuint overlay_texture = 0;
	static int init = 0;
	static int tex_changed_timestamp = -1;
	static SnapshotTex snap;
	static int old_size = -1;

	GLubyte *buffer = NULL;

	char do_tiled_texpaint = (br->mtex.brush_map_mode == MTEX_MAP_MODE_TILED_TEXPAINT);
	char do_tiled = (br->mtex.brush_map_mode == MTEX_MAP_MODE_TILED) || do_tiled_texpaint;

	int size;
	int j;
	int refresh;

	if (!br->mtex.tex) return 0;
	
	refresh =
		!overlay_texture ||
		(br->mtex.tex &&
		(!br->mtex.tex->preview ||
		br->mtex.tex->preview->changed_timestamp[0] != tex_changed_timestamp)) ||
		!same_snap_image(&snap, br, vc);

	if (refresh) {
		int s = BKE_brush_size_get(vc->scene, br);
		int r = 1;

		if (br->mtex.tex && br->mtex.tex->preview)
			tex_changed_timestamp = br->mtex.tex->preview->changed_timestamp[0];

		make_snap_image(&snap, br, vc);

		for (s >>= 1; s > 0; s >>= 1)
			r++;

		size = (1 << r);

		if (size < 256)
			size = 256;

		if (size < old_size)
			size = old_size;

		if (old_size != size) {
			if (overlay_texture) {
				glDeleteTextures(1, &overlay_texture);
				overlay_texture = 0;
			}

			init = 0;

			old_size = size;
		}

		buffer = MEM_mallocN(sizeof(GLubyte) * size * size, "load_tex");

		#pragma omp parallel for schedule(static)
		for (j = 0; j < size; j++) {
			int i;
			float y;
			float len;

			for (i = 0; i < size; i++) {

				// largely duplicated from tex_strength

				const float rotation = -br->mtex.rot;
				float radius = BKE_brush_size_get(vc->scene, br);
				int index = j * size + i;
				float x;
				float avg;

				x = (float)i / size;
				y = (float)j / size;

				if(!do_tiled_texpaint) {
					x -= 0.5f;
					y -= 0.5f;
				}

				if (do_tiled) {
					x *= vc->ar->winx / radius;
					y *= vc->ar->winy / radius;
				}
				else {
					x *= 2;
					y *= 2;
				}

				len = sqrtf(x * x + y * y);

				if (do_tiled || len <= 1) {
					/* it is probably worth optimizing for those cases where 
					 * the texture is not rotated by skipping the calls to
					 * atan2, sqrtf, sin, and cos. */
					if (rotation > 0.001f || rotation < -0.001f) {
						const float angle    = atan2f(y, x) + rotation;

						x = len * cosf(angle);
						y = len * sinf(angle);
					}

					x *= br->mtex.size[0];
					y *= br->mtex.size[1];

					x += br->mtex.ofs[0];
					y += br->mtex.ofs[1];

					avg = paint_get_tex_pixel(br, x, y, FALSE);

					avg += br->texture_sample_bias;

					buffer[index] = (GLubyte)(255 * avg);
				}
				else {
					buffer[index] = 0;
				}
			}
		}

		if (!overlay_texture)
			glGenTextures(1, &overlay_texture);
	}

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, overlay_texture);

	if (refresh) {
		if (!init) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, size, size, 0, GL_ALPHA, GL_UNSIGNED_BYTE, buffer);
			init = 1;
		}
		else {
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size, size, GL_ALPHA, GL_UNSIGNED_BYTE, buffer);
		}

		if (buffer)
			MEM_freeN(buffer);
	}

	glEnable(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (br->mtex.brush_map_mode == MTEX_MAP_MODE_VIEW) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	}

	glActiveTexture(GL_TEXTURE0);

	return 1;
}


static int project_brush_radius(ViewContext *vc,
                                float radius,
                                const float location[3])
{
	float view[3], nonortho[3], ortho[3], offset[3], p1[2], p2[2];

	ED_view3d_global_to_vector(vc->rv3d, location, view);

	/* create a vector that is not orthogonal to view */

	if (fabsf(view[0]) < 0.1f) {
		nonortho[0] = view[0] + 1.0f;
		nonortho[1] = view[1];
		nonortho[2] = view[2];
	}
	else if (fabsf(view[1]) < 0.1f) {
		nonortho[0] = view[0];
		nonortho[1] = view[1] + 1.0f;
		nonortho[2] = view[2];
	}
	else {
		nonortho[0] = view[0];
		nonortho[1] = view[1];
		nonortho[2] = view[2] + 1.0f;
	}

	/* get a vector in the plane of the view */
	cross_v3_v3v3(ortho, nonortho, view);
	normalize_v3(ortho);

	/* make a point on the surface of the brush tagent to the view */
	mul_v3_fl(ortho, radius);
	add_v3_v3v3(offset, location, ortho);

	/* project the center of the brush, and the tangent point to the view onto the screen */
	project_float(vc->ar, location, p1);
	project_float(vc->ar, offset, p2);

	/* the distance between these points is the size of the projected brush in pixels */
	return len_v2v2(p1, p2);
}

static int sculpt_get_brush_geometry(bContext *C, ViewContext *vc,
                                     int x, int y, int *pixel_radius,
                                     float location[3])
{
	Scene *scene = CTX_data_scene(C);
	Paint *paint = paint_get_active_from_context(C);
	Brush *brush = paint_brush(paint);
	float window[2];
	int hit;

	window[0] = x + vc->ar->winrct.xmin;
	window[1] = y + vc->ar->winrct.ymin;

	if (vc->obact->sculpt && vc->obact->sculpt->pbvh &&
	    sculpt_stroke_get_location(C, location, window))
	{
		*pixel_radius =
		    project_brush_radius(vc,
		                         BKE_brush_unprojected_radius_get(scene, brush),
		                         location);

		if (*pixel_radius == 0)
			*pixel_radius = BKE_brush_size_get(scene, brush);

		mul_m4_v3(vc->obact->obmat, location);

		hit = 1;
	}
	else {
		*pixel_radius = BKE_brush_size_get(scene, brush);
		hit = 0;
	}

	return hit;
}

/* Draw an overlay that shows what effect the brush's texture will
 * have on brush strength */
/* TODO: sculpt only for now */
static void paint_draw_alpha_overlay(UnifiedPaintSettings *ups, Brush *brush,
                                     ViewContext *vc, int x, int y, float zoomx, float zoomy, int in_uv_editor)
{
	rctf quad;

	/* check for overlay mode */
	if (!(brush->flag & BRUSH_TEXTURE_OVERLAY) ||
	    !(ELEM3(brush->mtex.brush_map_mode, MTEX_MAP_MODE_VIEW, MTEX_MAP_MODE_TILED, MTEX_MAP_MODE_TILED_TEXPAINT)) ||
	    (brush->flag & BRUSH_FIXED_TEX && in_uv_editor))
	{
		return;
	}

	/* save lots of GL state
	 * TODO: check on whether all of these are needed? */
	glPushAttrib(GL_COLOR_BUFFER_BIT |
	             GL_CURRENT_BIT |
	             GL_DEPTH_BUFFER_BIT |
	             GL_ENABLE_BIT |
	             GL_LINE_BIT |
	             GL_POLYGON_BIT |
	             GL_STENCIL_BUFFER_BIT |
	             GL_TRANSFORM_BIT |
	             GL_VIEWPORT_BIT |
	             GL_TEXTURE_BIT);

	if (load_brush_tex_alpha(brush, vc)) {
		int radius;

		/* setup opengl texture state to combine the alpha and texture images */
		if (load_brush_tex_image(brush, vc)) {
			/* modulate in first unit, negate in second */
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PRIMARY_COLOR);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_TEXTURE1);
			glActiveTexture(GL_TEXTURE1);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PREVIOUS);
			glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR);
			glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PRIMARY_COLOR);
			glActiveTexture(GL_TEXTURE0);
		} else {
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PRIMARY_COLOR);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
			glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR);
			glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
		}

		glEnable(GL_BLEND);

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_ALWAYS);

		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glLoadIdentity();

		if (brush->mtex.brush_map_mode == MTEX_MAP_MODE_VIEW) {
			float texmatrix[16];
			/* brush rotation */
			glActiveTexture(GL_TEXTURE1);
			glPushMatrix();
			glLoadIdentity();
			glTranslatef(0.5, 0.5, 0);
			glRotatef((double)RAD2DEGF((brush->flag & BRUSH_RAKE) ?
			                           ups->last_angle : ups->special_rotation),
			          0.0, 0.0, 1.0);

			/* scale based on tablet pressure */
			if (ups->draw_pressure && BKE_brush_use_size_pressure(vc->scene, brush)) {
				glScalef(1.0f / ups->pressure_value, 1.0f / ups->pressure_value, 1);
			}

			glTranslatef(-0.5f, -0.5f, 0);
			glGetFloatv(GL_TEXTURE_MATRIX, texmatrix);
			glActiveTexture(GL_TEXTURE0);
			glLoadMatrixf(texmatrix);

			if (ups->draw_anchored) {
				const float *aim = ups->anchored_initial_mouse;
				const rcti *win = &vc->ar->winrct;
				quad.xmin = aim[0] - ups->anchored_size - win->xmin;
				quad.ymin = aim[1] - ups->anchored_size - win->ymin;
				quad.xmax = aim[0] + ups->anchored_size - win->xmin;
				quad.ymax = aim[1] + ups->anchored_size - win->ymin;
			}
			else {
				radius = BKE_brush_size_get(vc->scene, brush);

				radius *= MAX2(zoomx, zoomy);

				quad.xmin = x - radius;
				quad.ymin = y - radius;
				quad.xmax = x + radius;
				quad.ymax = y + radius;
			}
		}
		else {
			radius = BKE_brush_size_get(vc->scene, brush);

			radius *= MAX2(zoomx, zoomy);

			quad.xmin = x - radius;
			quad.ymin = y - radius;
			quad.xmax = x + radius;
			quad.ymax = y + radius;
		}

		/* set quad color */
		glColor4f(U.sculpt_paint_overlay_col[0],
		          U.sculpt_paint_overlay_col[1],
		          U.sculpt_paint_overlay_col[2],
		          brush->texture_overlay_alpha / 100.0f);

		/* draw textured quad */
		if(brush->mtex.brush_map_mode == MTEX_MAP_MODE_VIEW) {
			int i;
			float xcenter;
			float ycenter;
			radius = (quad.xmax - quad.xmin)/ 2.0;
			xcenter = quad.xmin + radius;
			ycenter = quad.ymin + radius;

			glBegin(GL_TRIANGLE_FAN);
			/* center vertex specification */
			glTexCoord2f(0.5, 0.5);
			glMultiTexCoord2f(GL_TEXTURE1, 0.5, 0.5);
			glVertex2f(xcenter, ycenter);
			for(i = 0; i < CURSOR_RESOLUTION; i++) {
				float t = (float) i / (CURSOR_RESOLUTION - 1);
				float cur = t * 2.0 * M_PI;
				float tmpcos = cosf(cur);
				float tmpsin = sinf(cur);

				glTexCoord2f(0.5 + 0.5 * tmpcos, 0.5 + 0.5 * tmpsin);
				glMultiTexCoord2f(GL_TEXTURE1, 0.5 + 0.5 * tmpcos, 0.5 + 0.5 * tmpsin);
				glVertex2f(xcenter + tmpcos * radius, ycenter + tmpsin * radius);
			}
			glEnd();
		} else {
			int i;
			short sizex = vc->ar->winrct.xmax - vc->ar->winrct.xmin;
			short sizey = vc->ar->winrct.ymax - vc->ar->winrct.ymin;

			glBegin(GL_TRIANGLE_FAN);
			/* center vertex specification */
			glTexCoord2f(0.5, 0.5);
			glMultiTexCoord2f(GL_TEXTURE1, ((float)x)/sizex, ((float)y)/sizey);
			glVertex2f(x, y);
			for(i = 0; i < CURSOR_RESOLUTION; i++) {
				float t = (float) i / (CURSOR_RESOLUTION - 1);
				float cur = t * 2.0 * M_PI;
				float tmpcos = cosf(cur);
				float tmpsin = sinf(cur);
				float xpos = x + tmpcos * radius;
				float ypos = y + tmpsin * radius;

				glTexCoord2f(0.5 + 0.5 * tmpcos, 0.5 + 0.5 * tmpsin);
				glMultiTexCoord2f(GL_TEXTURE1, xpos/sizex, ypos/sizey);
				glVertex2f(xpos, ypos);
			}
			glEnd();
		}

		/* should be caught by enable bits but do explicitly just in case..*/
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glPopMatrix();
		glActiveTexture(GL_TEXTURE0);
		glPopMatrix();
	}

	glPopAttrib();
}

/* Special actions taken when paint cursor goes over mesh */
/* TODO: sculpt only for now */
static void paint_cursor_on_hit(UnifiedPaintSettings *ups, Brush *brush, ViewContext *vc,
                                const float location[3])
{
	float unprojected_radius, projected_radius;

	/* update the brush's cached 3D radius */
	if (!BKE_brush_use_locked_size(vc->scene, brush)) {
		/* get 2D brush radius */
		if (ups->draw_anchored)
			projected_radius = ups->anchored_size;
		else {
			if (brush->flag & BRUSH_ANCHORED)
				projected_radius = 8;
			else
				projected_radius = BKE_brush_size_get(vc->scene, brush);
		}
	
		/* convert brush radius from 2D to 3D */
		unprojected_radius = paint_calc_object_space_radius(vc, location,
		                                                    projected_radius);

		/* scale 3D brush radius by pressure */
		if (ups->draw_pressure && BKE_brush_use_size_pressure(vc->scene, brush))
			unprojected_radius *= ups->pressure_value;

		/* set cached value in either Brush or UnifiedPaintSettings */
		BKE_brush_unprojected_radius_set(vc->scene, brush, unprojected_radius);
	}
}

void paint_draw_cursor(bContext *C, int x, int y, void *UNUSED(unused))
{
	Scene *scene = CTX_data_scene(C);
	UnifiedPaintSettings *ups = &scene->toolsettings->unified_paint_settings;
	Paint *paint = paint_get_active_from_context(C);
	Brush *brush = paint_brush(paint);
	ViewContext vc;
	float final_radius;
	float translation[2];
	float outline_alpha, *outline_col;
	float location[3];
	int pixel_radius, hit = 0, in_uv_editor;
	float zoomx = 1.0, zoomy = 1.0;

	/* set various defaults */
	translation[0] = x;
	translation[1] = y;
	outline_alpha = 0.5;
	outline_col = brush->add_col;
	final_radius = BKE_brush_size_get(scene, brush);

	/* check that brush drawing is enabled */
	if (!(paint->flags & PAINT_SHOW_BRUSH))
		return;

	in_uv_editor = get_imapaint_zoom(C, &zoomx, &zoomy);

	if(CTX_data_mode_enum(C) == CTX_MODE_PAINT_TEXTURE) {
		brush->mtex.brush_map_mode = MTEX_MAP_MODE_TILED_TEXPAINT;

		if((brush->flag & BRUSH_RAKE) || (brush->flag & BRUSH_RANDOM_ROTATION))
			brush->mtex.brush_map_mode = MTEX_MAP_MODE_VIEW;
	}

	if(in_uv_editor) {
		brush->mtex.brush_map_mode = MTEX_MAP_MODE_VIEW;

		if(brush->flag & BRUSH_FIXED_TEX)
			brush->mtex.brush_map_mode = MTEX_MAP_MODE_TILED;
	}


	/* can't use stroke vc here because this will be called during
	 * mouse over too, not just during a stroke */
	view3d_set_viewcontext(C, &vc);


	/* this is probably here so that rake takes into
		 * account the brush movements before the stroke
		 * starts, but this doesn't really belong in draw code
		 *  TODO) */
	paint_calculate_rake_rotation(ups, translation);

	//print_v2("mouse"__FILE__, translation);

	if(!in_uv_editor) {
		/* test if brush is over the mesh. sculpt only for now */
		hit = sculpt_get_brush_geometry(C, &vc, x, y, &pixel_radius, location);

	/* uv sculpting brush won't get scaled maybe I should make this respect the setting
	 * of lock size */
	} else if(!(scene->toolsettings->use_uv_sculpt)){
#define PX_SIZE_FADE_MAX 12.0f
#define PX_SIZE_FADE_MIN 4.0f

		float temp_radius =  MAX2(final_radius * zoomx, final_radius * zoomy);

		if (temp_radius < PX_SIZE_FADE_MIN) {
			return;
		}
		else if (temp_radius < PX_SIZE_FADE_MAX) {
			outline_alpha *= (temp_radius - PX_SIZE_FADE_MIN) / (PX_SIZE_FADE_MAX - PX_SIZE_FADE_MIN);
		}
#undef PX_SIZE_FADE_MAX
#undef PX_SIZE_FADE_MIN
	}
	/* draw overlay */
	paint_draw_alpha_overlay(ups, brush, &vc, x, y, zoomx, zoomy, in_uv_editor);

	if (BKE_brush_use_locked_size(scene, brush) && !in_uv_editor) {
		BKE_brush_size_set(scene, brush, pixel_radius);
		final_radius = pixel_radius;
	}

	/* check if brush is subtracting, use different color then */
	/* TODO: no way currently to know state of pen flip or
		 * invert key modifier without starting a stroke */
	/* TODO no.2 add interface to query the tool if it supports inversion */
	if ((!(brush->flag & BRUSH_INVERTED) ^
		 !(brush->flag & BRUSH_DIR_IN)) &&
			ELEM5(brush->sculpt_tool, SCULPT_TOOL_DRAW,
				  SCULPT_TOOL_INFLATE, SCULPT_TOOL_CLAY,
				  SCULPT_TOOL_PINCH, SCULPT_TOOL_CREASE))
	{
		outline_col = brush->sub_col;
	}

	/* only do if brush is over the mesh */
	if (hit)
		paint_cursor_on_hit(ups, brush, &vc, location);

	if (ups->draw_anchored) {
		final_radius = ups->anchored_size;
		translation[0] = ups->anchored_initial_mouse[0] - vc.ar->winrct.xmin;
		translation[1] = ups->anchored_initial_mouse[1] - vc.ar->winrct.ymin;
	}

	/* make lines pretty */
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);

	/* set brush color */
	glColor4f(outline_col[0], outline_col[1], outline_col[2], outline_alpha);

	/* draw brush outline */
	glPushMatrix();
	glTranslatef(translation[0], translation[1], 0);
	if(in_uv_editor)
		glScalef(zoomx, zoomy, 1.0f);
	glutil_draw_lined_arc(0.0, M_PI * 2.0, final_radius, CURSOR_RESOLUTION);
	glPopMatrix();

	/* restore GL state */
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
}

/* Public API */

void paint_cursor_start(bContext *C, int (*poll)(bContext *C))
{
	Paint *p = paint_get_active_from_context(C);

	if (p && !p->paint_cursor)
		p->paint_cursor = WM_paint_cursor_activate(CTX_wm_manager(C), poll, paint_draw_cursor, NULL);
}
