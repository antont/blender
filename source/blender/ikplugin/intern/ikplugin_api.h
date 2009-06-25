/**
 * $Id$
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
 * The Original Code is: all of this file.
 *
 * Original author: Benoit Bolsee
 * Contributor(s): 
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#ifndef IKPLUGIN_API_H
#define IKPLUGIN_API_H

#ifdef __cplusplus
extern "C" {
#endif

struct Object;
struct bPoseChannel;
struct bArmature;


struct IKPlugin {
	void (*initialize_tree_func)(struct Object *ob, float ctime);
	void (*execute_tree_func)(struct Object *ob, struct bPoseChannel *pchan, float ctime);
	void (*release_tree_func)(struct Object *ob, float ctime);
	void (*remove_armature_func)(struct bArmature *arm);
	void (*clear_cache)(struct bArmature *arm);
};

typedef struct IKPlugin IKPlugin;

#ifdef __cplusplus
}
#endif

#endif // IKPLUGIN_API_H

