/**
 * BKE_shrinkwrap.h
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
 * The Original Code is Copyright (C) Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */
#ifndef BKE_SIMPLE_DEFORM_H
#define BKE_SIMPLE_DEFORM_H

struct Object;
struct DerivedMesh;
struct SimpleDeformModifierData;

/* struct DerivedMesh *simpledeformModifier_do(struct SimpleDeformModifierData *smd, struct Object *ob, struct DerivedMesh *dm, int useRenderParams, int isFinalCalc); */
void SimpleDeformModifier_do(SimpleDeformModifierData *smd, float (*vertexCos)[3], int numVerts);

#endif

