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
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2007 by Nicholas Bishop
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */ 

struct DerivedMesh;
struct Mesh;
struct MFace;
struct Multires;
struct Object;

typedef struct MultiresSubsurf {
	struct Mesh *me;
	int totlvl, lvl;
} MultiresSubsurf;

typedef struct IndexNode {
	struct IndexNode *next, *prev;
	int index;
} IndexNode;

void multires_free(struct Multires*);

void create_vert_face_map(ListBase **map, IndexNode **mem, const struct MFace *mface,
			  const int totvert, const int totface);

/* MultiresDM */
struct Mesh *MultiresDM_get_mesh(struct DerivedMesh *dm);
struct DerivedMesh *MultiresDM_new(struct MultiresSubsurf *, struct DerivedMesh*, int, int, int);
void *MultiresDM_get_vertnorm(struct DerivedMesh *);
void *MultiresDM_get_orco(struct DerivedMesh *);
struct MVert *MultiresDM_get_subco(struct DerivedMesh *);
struct ListBase *MultiresDM_get_vert_face_map(struct DerivedMesh *);
int MultiresDM_get_totlvl(struct DerivedMesh *);
int MultiresDM_get_lvl(struct DerivedMesh *);
void MultiresDM_set_update(struct DerivedMesh *, void (*)(struct DerivedMesh*));
int *MultiresDM_get_flags(struct DerivedMesh *);

#define MULTIRES_DM_UPDATE_BLOCK 1
#define MULTIRES_DM_UPDATE_ALWAYS 2

/* Modifier */
struct MDisps;
struct MFace;
struct MultiresModifierData;
typedef struct MultiresDisplacer {
	struct MDisps *grid;
	struct MFace *face;
	float mat[3][3];
	
	/* For matrix calc */
	float mat_target[3];
	float mat_center[3];
	float (*mat_norms)[3];

	int spacing;
	int sidetot;
	int sidendx;
	int type;
	int invert;
	float (*orco)[3];
	struct MVert *subco;
	float weight;

	int x, y, ax, ay;
} MultiresDisplacer;

void multires_load_old(struct DerivedMesh *, struct Multires *);
void multires_force_update(struct Object *ob);

struct DerivedMesh *multires_dm_create_from_derived(struct MultiresModifierData*, struct DerivedMesh*,
						    struct Mesh *, int, int);

int multiresModifier_switch_level(struct Object *ob, const int);
void multiresModifier_join(struct Object *ob);
void multiresModifier_subdivide(struct MultiresModifierData *mmd, struct Object *ob, int distance,
				int updateblock, int simple);
void multiresModifier_setLevel(void *mmd_v, void *ob_v);
int multiresModifier_reshape(struct MultiresModifierData *mmd, struct Object *dst, struct Object *src);

void multires_displacer_init(MultiresDisplacer *d, struct DerivedMesh *dm,
			     const int face_index, const int invert);
void multires_displacer_weight(MultiresDisplacer *d, const float w);
void multires_displacer_anchor(MultiresDisplacer *d, const int type, const int side_index);
void multires_displacer_anchor_edge(MultiresDisplacer *d, const int, const int, const int);
void multires_displacer_anchor_vert(MultiresDisplacer *d, const int);
void multires_displacer_jump(MultiresDisplacer *d);
void multires_displace(MultiresDisplacer *d, float out[3]);
