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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2011 Blender Foundation.
 * All rights reserved.
 *
 *
 * Contributor(s): Blender Foundation,
 *                 Sergey Sharybin
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/editors/space_clip/clip_utils.c
 *  \ingroup spclip
 */

#include "DNA_scene_types.h"
#include "DNA_object_types.h"   /* SELECT */

#include "MEM_guardedalloc.h"

#include "BLI_utildefines.h"
#include "BLI_math.h"
#include "BLI_listbase.h"

#include "BKE_context.h"
#include "BKE_movieclip.h"
#include "BKE_tracking.h"
#include "BKE_depsgraph.h"

#include "GPU_colors.h"
#include "GPU_primitives.h"

#include "BIF_glutil.h"

#include "WM_api.h"
#include "WM_types.h"

#include "ED_screen.h"
#include "ED_clip.h"

#include "UI_interface.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "UI_interface.h"
#include "UI_resources.h"
#include "UI_view2d.h"

#include "clip_intern.h"    // own include

void clip_graph_tracking_values_iterate_track(
        SpaceClip *sc, MovieTrackingTrack *track, void *userdata,
        void (*func)(void *userdata, MovieTrackingTrack *track, MovieTrackingMarker *marker, int coord,
                     int scene_framenr, float val),
        void (*segment_start)(void *userdata, MovieTrackingTrack *track, int coord),
        void (*segment_end)(void *userdata))
{
	MovieClip *clip = ED_space_clip_get_clip(sc);
	int width, height, coord;

	BKE_movieclip_get_size(clip, &sc->user, &width, &height);

	for (coord = 0; coord < 2; coord++) {
		int i, open = FALSE, prevfra = 0;
		float prevval = 0.0f;

		for (i = 0; i < track->markersnr; i++) {
			MovieTrackingMarker *marker = &track->markers[i];
			float val;

			if (marker->flag & MARKER_DISABLED) {
				if (open) {
					if (segment_end)
						segment_end(userdata);

					open = FALSE;
				}

				continue;
			}

			if (!open) {
				if (segment_start)
					segment_start(userdata, track, coord);

				open = TRUE;
				prevval = marker->pos[coord];
			}

			/* value is a pixels per frame speed */
			val = (marker->pos[coord] - prevval) * ((coord == 0) ? (width) : (height));
			val /= marker->framenr - prevfra;

			if (func) {
				int scene_framenr = BKE_movieclip_remap_clip_to_scene_frame(clip, marker->framenr);

				func(userdata, track, marker, coord, scene_framenr, val);
			}

			prevval = marker->pos[coord];
			prevfra = marker->framenr;
		}

		if (open) {
			if (segment_end)
				segment_end(userdata);
		}
	}
}

void clip_graph_tracking_values_iterate(
        SpaceClip *sc, int selected_only, int include_hidden, void *userdata,
        void (*func)(void *userdata, MovieTrackingTrack *track, MovieTrackingMarker *marker,
                     int coord, int scene_framenr, float val),
        void (*segment_start)(void *userdata, MovieTrackingTrack *track, int coord),
        void (*segment_end)(void *userdata))
{
	MovieClip *clip = ED_space_clip_get_clip(sc);
	MovieTracking *tracking = &clip->tracking;
	ListBase *tracksbase = BKE_tracking_get_active_tracks(tracking);
	MovieTrackingTrack *track;

	for (track = tracksbase->first; track; track = track->next) {
		if (!include_hidden && (track->flag & TRACK_HIDDEN) != 0)
			continue;

		if (selected_only && !TRACK_SELECTED(track))
			continue;

		clip_graph_tracking_values_iterate_track(sc, track, userdata, func, segment_start, segment_end);
	}
}

void clip_graph_tracking_iterate(SpaceClip *sc, int selected_only, int include_hidden, void *userdata,
                                 void (*func)(void *userdata, MovieTrackingMarker *marker))
{
	MovieClip *clip = ED_space_clip_get_clip(sc);
	MovieTracking *tracking = &clip->tracking;
	ListBase *tracksbase = BKE_tracking_get_active_tracks(tracking);
	MovieTrackingTrack *track;

	for (track = tracksbase->first; track; track = track->next) {
		int i;

		if (!include_hidden && (track->flag & TRACK_HIDDEN) != 0)
			continue;

		if (selected_only && !TRACK_SELECTED(track))
			continue;

		for (i = 0; i < track->markersnr; i++) {
			MovieTrackingMarker *marker = &track->markers[i];

			if (marker->flag & MARKER_DISABLED)
				continue;

			if (func)
				func(userdata, marker);
		}
	}
}

void clip_delete_track(bContext *C, MovieClip *clip, ListBase *tracksbase, MovieTrackingTrack *track)
{
	MovieTracking *tracking = &clip->tracking;
	MovieTrackingStabilization *stab = &tracking->stabilization;
	MovieTrackingTrack *act_track = BKE_tracking_track_get_active(tracking);

	int has_bundle = FALSE, update_stab = FALSE;

	if (track == act_track)
		tracking->act_track = NULL;

	if (track == stab->rot_track) {
		stab->rot_track = NULL;

		update_stab = TRUE;
	}

	/* handle reconstruction display in 3d viewport */
	if (track->flag & TRACK_HAS_BUNDLE)
		has_bundle = TRUE;

	BKE_tracking_track_free(track);
	BLI_freelinkN(tracksbase, track);

	WM_event_add_notifier(C, NC_MOVIECLIP | NA_EDITED, clip);

	if (update_stab) {
		tracking->stabilization.ok = FALSE;
		WM_event_add_notifier(C, NC_MOVIECLIP | ND_DISPLAY, clip);
	}

	DAG_id_tag_update(&clip->id, 0);

	if (has_bundle)
		WM_event_add_notifier(C, NC_SPACE | ND_SPACE_VIEW3D, NULL);
}

void clip_delete_marker(bContext *C, MovieClip *clip, ListBase *tracksbase,
                        MovieTrackingTrack *track, MovieTrackingMarker *marker)
{
	if (track->markersnr == 1) {
		clip_delete_track(C, clip, tracksbase, track);
	}
	else {
		BKE_tracking_marker_delete(track, marker->framenr);

		WM_event_add_notifier(C, NC_MOVIECLIP | NA_EDITED, clip);
	}
}

void clip_view_center_to_point(SpaceClip *sc, float x, float y)
{
	int width, height;
	float aspx, aspy;

	ED_space_clip_get_size(sc, &width, &height);
	ED_space_clip_get_aspect(sc, &aspx, &aspy);

	sc->xof = (x - 0.5f) * width * aspx;
	sc->yof = (y - 0.5f) * height * aspy;
}

void clip_draw_cfra(SpaceClip *sc, ARegion *ar, Scene *scene)
{
	View2D *v2d = &ar->v2d;
	float xscale, yscale;
	float c;

	/* Draw a light green line to indicate current frame */
	c = (float)(sc->user.framenr * scene->r.framelen);

	UI_ThemeColor(TH_CFRAME);
	gpuLineWidth(2.0);

	gpuSingleLinef(c, v2d->cur.ymin, c, v2d->cur.ymax);

	gpuLineWidth(1.0);

	UI_view2d_view_orthoSpecial(ar, v2d, 1);

	/* because the frame number text is subject to the same scaling as the contents of the view */
	UI_view2d_getscale(v2d, &xscale, &yscale);
	gpuScale(1.0f / xscale, 1.0f, 1.0f);

	clip_draw_curfra_label(sc->user.framenr, (float)sc->user.framenr * xscale, 18);

	/* restore view transform */
	gpuScale(xscale, 1.0, 1.0);
}

void clip_draw_sfra_efra(View2D *v2d, Scene *scene)
{
	UI_view2d_view_ortho(v2d);

	/* currently clip editor supposes that editing clip length is equal to scene frame range */

	glEnable(GL_BLEND);
	
	gpuCurrentColor4x(CPACK_BLACK, 0.400f);

	gpuSingleFilledRectf(v2d->cur.xmin, v2d->cur.ymin, (float)SFRA, v2d->cur.ymax);
	gpuSingleFilledRectf((float)EFRA, v2d->cur.ymin, v2d->cur.xmax, v2d->cur.ymax);

	glDisable(GL_BLEND);

	UI_ThemeColorShade(TH_BACK, -60);

	/* thin lines where the actual frames are */
	gpuImmediateFormat_V2(); // DOODLE: pair of mono lines
	gpuBegin(GL_LINES);
	gpuAppendLinef((float)SFRA, v2d->cur.ymin, (float)SFRA, v2d->cur.ymax);
	gpuAppendLinef((float)EFRA, v2d->cur.ymin, (float)EFRA, v2d->cur.ymax);
	gpuEnd();
	gpuImmediateUnformat();
}
