/*  BKE_deform.h   June 2001
 *  
 *  support for deformation groups and hooks
 * 
 *	Reevan McKay et al
 *
 * $Id$
 *
 * ***** BEGIN GPL/BL DUAL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version. The Blender
 * Foundation also sells licenses for use in proprietary software under
 * the Blender License.  See http://www.blender.org/BL/ for information
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
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */

#ifndef BKE_DEFORM_H
#define BKE_DEFORM_H

struct Object;
struct ListBase;
struct bDeformGroup;

void copy_defgroups (struct ListBase *lb1, struct ListBase *lb2);
struct bDeformGroup* copy_defgroup (struct bDeformGroup *ingroup);
void color_temperature (float input, unsigned char *r, unsigned char *g, unsigned char *b);

void hook_object_deform(struct Object *ob, int index, float *vec);

int curve_modifier(struct Object *ob, char mode);
int mesh_modifier(struct Object *ob, char mode);
int lattice_modifier(struct Object *ob, char mode);


#endif

