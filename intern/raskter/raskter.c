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
 * The Original Code is Copyright (C) 2012 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Peter Larabell.
 *
 * ***** END GPL LICENSE BLOCK *****
 */
/** \file raskter.c
 *  \ingroup RASKTER
 */

#include <stdio.h>
#include <malloc.h>
#include "raskter.h"

/* from BLI_utildefines.h */
#define MIN2(x, y)               ( (x) < (y) ? (x) : (y) )
#define MAX2(x, y)               ( (x) > (y) ? (x) : (y) )


struct e_status {
	int x;
	int ybeg;
	int xshift;
	int xdir;
	int drift;
	int drift_inc;
	int drift_dec;
	int num;
	struct e_status *e_next;
};

struct r_buffer_stats {
	float *buf;
	int sizex;
	int sizey;
};

static struct e_status *all_edges, *possible_edges;
static struct r_buffer_stats rb;
/*
 * Sort all the edges of the input polygon by Y, then by X, of the "first" vertex encountered.
 * This will ensure we can scan convert the entire poly in one pass.
 *
 * Really the poly should be clipped to the frame buffer's dimensions here for speed of drawing
 * just the poly. Since the DEM code could end up being coupled with this, we'll keep it separate
 * for now.
 */
static void preprocess_all_edges(struct poly_vert *verts, int num_verts, struct e_status *open_edge)
{
	int i;
	int xbeg;
	int ybeg;
	int xend;
	int yend;
	int dx;
	int dy;
	int temp_pos;
	int xdist;
	struct e_status *e_new;
	struct e_status *next_edge;
	struct e_status **next_edge_ref;
	struct poly_vert *v;
	/* set up pointers */
	v = verts;
	all_edges = NULL;
	/* loop all verts */
	for (i = 0; i < num_verts; i++) {
		/* determine beginnings and endings of edges, linking last vertex to first vertex */
		xbeg = v[i].x;
		ybeg = v[i].y;
		if (i) {
			/* we're not at the last vert, so end of the edge is the previous vertex */
			xend = v[i - 1].x;
			yend = v[i - 1].y;
		}
		else {
			/* we're at the first vertex, so the "end" of this edge is the last vertex */
			xend = v[num_verts - 1].x;
			yend = v[num_verts - 1].y;
		}
		/* make sure our edges are facing the correct direction */
		if (ybeg > yend) {
			/* flip the Xs */
			temp_pos = xbeg;
			xbeg = xend;
			xend = temp_pos;
			/* flip the Ys */
			temp_pos = ybeg;
			ybeg = yend;
			yend = temp_pos;
		}

		/* calculate y delta */
		dy = yend - ybeg;
		/* dont draw horizontal lines directly, they are scanned as part of the edges they connect, so skip em. :) */
		if (dy) {
			/* create the edge and determine it's slope (for incremental line drawing) */
			e_new = open_edge++;

			/* calculate x delta */
			dx = xend - xbeg;
			if (dx > 0) {
				e_new->xdir = 1;
				xdist = dx;
			}
			else {
				e_new->xdir = -1;
				xdist = -dx;
			}

			e_new->x = xbeg;
			e_new->ybeg = ybeg;
			e_new->num = dy;
			e_new->drift_dec = dy;

			/* calculate deltas for incremental drawing */
			if (dx >= 0) {
				e_new->drift = 0;
			}
			else {
				e_new->drift = -dy + 1;
			}
			if (dy >= xdist) {
				e_new->drift_inc = xdist;
				e_new->xshift = 0;
			}
			else {
				e_new->drift_inc = xdist % dy;
				e_new->xshift = (xdist / dy) * e_new->xdir;
			}
			next_edge_ref = &all_edges;
			/* link in all the edges, in sorted order */
			for (;; ) {
				next_edge = *next_edge_ref;
				if (!next_edge || (next_edge->ybeg > ybeg) || ((next_edge->ybeg == ybeg) && (next_edge->x >= xbeg))) {
					e_new->e_next = next_edge;
					*next_edge_ref = e_new;
					break;
				}
				next_edge_ref = &next_edge->e_next;
			}
		}
	}
}

/*
 * This function clips drawing to the frame buffer. That clipping will likely be moved into the preprocessor
 * for speed, but waiting on final design choices for curve-data before eliminating data the DEM code will need
 * if it ends up being coupled with this function.
 */
int rast_scan_fill(struct poly_vert *verts, int num_verts)
{
	int x_curr;                 /* current pixel position in X */
	int y_curr;                 /* current scan line being drawn */
	int yp;                     /* y-pixel's position in frame buffer */
	int swixd = 0;              /* whether or not edges switched position in X */
	float *cpxl;                /* pixel pointers... */
	float *mpxl;
	float *spxl;
	struct e_status *e_curr;    /* edge pointers... */
	struct e_status *e_temp;
	struct e_status *edgbuf;
	struct e_status **edgec;


	/*
	 * If the number of verts specified to render as a polygon is less than 3,
	 * return immediately. Obviously we cant render a poly with sides < 3. The
	 * return for this we set to 1, simply so it can be distinguished from the
	 * next place we could return, /home/guest/blender-svn/soc-2011-tomato/intern/raskter/raskter.cwhich is a failure to allocate memory.
	 */
	if (num_verts < 3) {
		return(1);
	}

	/*
	 * Try to allocate an edge buffer in memory. needs to be the size of the edge tracking data
	 * multiplied by the number of edges, which is always equal to the number of verts in
	 * a 2D polygon. Here we return 0 to indicate a memory allocation failure, as opposed to a 1 for
	 * the preceeding error, which was a rasterization request on a 2D poly with less than
	 * 3 sides.
	 */
	if ((edgbuf = (struct e_status *)(malloc(sizeof(struct e_status) * num_verts))) == NULL) {
		return(0);
	}

	/*
	 * Do some preprocessing on all edges. This constructs a table structure in memory of all
	 * the edge properties and can "flip" some edges so sorting works correctly.
	 */
	preprocess_all_edges(verts, num_verts, edgbuf);

	/*
	 * Set the pointer for tracking the edges currently in processing to NULL to make sure
	 * we don't get some crazy value after initialization.
	 */
	possible_edges = NULL;

	/*
	 * Loop through all scan lines to be drawn. Since we sorted by Y values during
	 * preprocess_all_edges(), we can already exact values for the lowest and
	 * highest Y values we could possibly need by induction. The preprocessing sorted
	 * out edges by Y position, we can cycle the current edge being processed once
	 * it runs out of Y pixels. When we have no more edges, meaning the current edge
	 * is NULL after setting the "current" edge to be the previous current edge's
	 * "next" edge in the Y sorted edge connection chain, we can stop looping Y values,
	 * since we can't possibly have more scan lines if we ran out of edges. :)
	 *
	 * TODO: This clips Y to the frame buffer, which should be done in the preprocessor, but for now is done here.
	 *       Will get changed once DEM code gets in.
	 */
	for (y_curr = MAX2(all_edges->ybeg, 0); (all_edges || possible_edges) && (y_curr < rb.sizey); y_curr++) {

		/*
		 * Link any edges that start on the current scan line into the list of
		 * edges currently needed to draw at least this, if not several, scan lines.
		 */

		/*
		 * Set the current edge to the beginning of the list of edges to be rasterized
		 * into this scan line.
		 *
		 * We could have lots of edge here, so iterate over all the edges needed. The
		 * preprocess_all_edges() function sorted edges by X within each chunk of Y sorting
		 * so we safely cycle edges to thier own "next" edges in order.
		 *
		 * At each iteration, make sure we still have a non-NULL edge.
		 */
		for (edgec = &possible_edges; all_edges && (all_edges->ybeg == y_curr); ) {
			x_curr = all_edges->x;                       /* Set current X position. */
			for (;; ) {                                  /* Start looping edges. Will break when edges run out. */
				e_curr = *edgec;                         /* Set up a current edge pointer. */
				if (!e_curr || (e_curr->x >= x_curr)) {  /* If we have an no edge, or we need to skip some X-span, */
					e_temp = all_edges->e_next;          /* set a temp "next" edge to test. */
					*edgec = all_edges;                  /* Add this edge to the list to be scanned. */
					all_edges->e_next = e_curr;          /* Set up the next edge. */
					edgec = &all_edges->e_next;          /* Set our list to the next edge's location in memory. */
					all_edges = e_temp;                  /* Skip the NULL or bad X edge, set pointer to next edge. */
					break;                               /* Stop looping edges (since we ran out or hit empty X span. */
				}
				else {
					edgec = &e_curr->e_next;             /* Set the pointer to the edge list the "next" edge. */
				}
			}
		}

		/*
		 * Determine the current scan line's offset in the pixel buffer based on its Y position.
		 * Basically we just multiply the current scan line's Y value by the number of pixels in each line.
		 */
		yp = y_curr * rb.sizex;
		/*
		 * Set a "scan line pointer" in memory. The location of the buffer plus the row offset.
		 */
		spxl = rb.buf + (yp);
		/*
		 * Set up the current edge to the first (in X) edge. The edges which could possibly be in this
		 * list were determined in the preceeding edge loop above. They were already sorted in X by the
		 * initial processing function.
		 *
		 * At each iteration, test for a NULL edge. Since we'll keep cycling edge's to their own "next" edge
		 * we will eventually hit a NULL when the list runs out.
		 */
		for (e_curr = possible_edges; e_curr; e_curr = e_curr->e_next) {
			/*
			 * Calculate a span of pixels to fill on the current scan line.
			 *
			 * Set the current pixel pointer by adding the X offset to the scan line's start offset.
			 * Cycle the current edge the next edge.
			 * Set the max X value to draw to be one less than the next edge's first pixel. This way we are
			 * sure not to ever get into a situation where we have overdraw. (drawing the same pixel more than
			 * one time because it's on a vertex connecting two edges)
			 *
			 * Then blast through all the pixels in the span, advancing the pointer and setting the color to white.
			 *
			 * TODO: Here we clip to the scan line, this is not efficient, and should be done in the preprocessor,
			 *       but for now it is done here until the DEM code comes in.
			 */

			/* set up xmin and xmax bounds on this scan line */
			cpxl = spxl + MAX2(e_curr->x, 0);
			e_curr = e_curr->e_next;
			mpxl = spxl + MIN2(e_curr->x, rb.sizex) - 1;

			/* draw the pixels. */
			for (; cpxl <= mpxl; cpxl++){
				if(*cpxl < 0.5f){
					*cpxl = 1.0f;
				}else{
					*cpxl = 0.0f;
				}
			}
		}

		/*
		 * Loop through all edges of polygon that could be hit by this scan line,
		 * and figure out their x-intersections with the next scan line.
		 *
		 * Either A.) we wont have any more edges to test, or B.) we just add on the
		 * slope delta computed in preprocessing step. Since this draws non-antialiased
		 * polygons, we dont have fractional positions, so we only move in x-direction
		 * when needed to get all the way to the next pixel over...
		 */
		for (edgec = &possible_edges; (e_curr = *edgec); ) {
			if (!(--(e_curr->num))) {
				*edgec = e_curr->e_next;
			}
			else {
				e_curr->x += e_curr->xshift;
				if ((e_curr->drift += e_curr->drift_inc) > 0) {
					e_curr->x += e_curr->xdir;
					e_curr->drift -= e_curr->drift_dec;
				}
				edgec = &e_curr->e_next;
			}
		}
		/*
		 * It's possible that some edges may have crossed during the last step, so we'll be sure
		 * that we ALWAYS intersect scan lines in order by shuffling if needed to make all edges
		 * sorted by x-intersection coordinate. We'll always scan through at least once to see if
		 * edges crossed, and if so, we set the 'swixd' flag. If 'swixd' gets set on the initial
		 * pass, then we know we need to sort by x, so then cycle through edges again and perform
		 * the sort.-
		 */
		if (possible_edges) {
			for (edgec = &possible_edges; (e_curr = *edgec)->e_next; edgec = &(*edgec)->e_next) {
				/* if the current edge hits scan line at greater X than the next edge, we need to exchange the edges */
				if (e_curr->x > e_curr->e_next->x) {
					*edgec = e_curr->e_next;
					/* exchange the pointers */
					e_temp = e_curr->e_next->e_next;
					e_curr->e_next->e_next = e_curr;
					e_curr->e_next = e_temp;
					/* set flag that we had at least one switch */
					swixd = 1;
				}
			}
			/* if we did have a switch, look for more (there will more if there was one) */
			for (;; ) {
				/* reset exchange flag so it's only set if we encounter another one */
				swixd = 0;
				for (edgec = &possible_edges; (e_curr = *edgec)->e_next; edgec = &(*edgec)->e_next) {
					/* again, if current edge hits scan line at higher X than next edge, exchange the edges and set flag */
					if (e_curr->x > e_curr->e_next->x) {
						*edgec = e_curr->e_next;
						/* exchange the pointers */
						e_temp = e_curr->e_next->e_next;
						e_curr->e_next->e_next = e_curr;
						e_curr->e_next = e_temp;
						/* flip the exchanged flag */
						swixd = 1;
					}
				}
				/* if we had no exchanges, we're done reshuffling the pointers */
				if (!swixd) {
					break;
				}
			}
		}
	}

	free(edgbuf);
	return 1;
}

int PLX_raskterize(float *verts, int num, float *buf, int buf_x, int buf_y) {
	int i;                                       /* i: Loop counter. */
	struct poly_vert *ply;                       /* ply: Pointer to a list of integer buffer-space vertex coordinates. */

	/*
	 * Allocate enough memory for our poly_vert list. It'll be the size of the poly_vert
	 * data structure multiplied by the number of verts.
	 *
	 * In the event of a failure to allocate the memory, return 0, so this error can
	 * be distinguished as a memory allocation error.
	 */
	if ((ply = (struct poly_vert *)(malloc(sizeof(struct poly_vert) * num))) == NULL) {
		return(0);
	}

	/*
	 * Loop over all verts passed in to be rasterized. Each vertex's X and Y coordinates are
	 * then converted from normalized screen space (0.0 <= POS <= 1.0) to integer coordinates
	 * in the buffer-space coordinates passed in inside buf_x and buf_y.
	 *
	 * It's worth noting that this function ONLY outputs fully white pixels in a mask. Every pixel
	 * drawn will be 1.0f in value, there is no anti-aliasing.
	 */
	for (i = 0; i < num; i++) {                          /* Loop over all verts. */
		ply[i].x = (verts[i << 1] * buf_x) + 0.5f;       /* Range expand normalized X to integer buffer-space X. */
		ply[i].y = (verts[(i << 1) + 1] * buf_y) + 0.5f; /* Range expand normalized Y to integer buffer-space Y. */
	}

	rb.buf = buf;                                /* Set the output buffer pointer. */
	rb.sizex = buf_x;                            /* Set the output buffer size in X. (width) */
	rb.sizey = buf_y;                            /* Set the output buffer size in Y. (height) */

	i = rast_scan_fill(ply, num);                /* Call our rasterizer, passing in the integer coords for each vert. */
	free(ply);                                   /* Free the memory allocated for the integer coordinate table. */
	return(i);                                   /* Return the value returned by the rasterizer. */
}

