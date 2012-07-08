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
 * Contributor(s): Alexander Gessler
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/assimp/bfbx.cpp
 *  \ingroup fbx
 */

#include <cassert>
#include "../assimp/SceneImporter.h"

extern "C"
{
#include "BKE_scene.h"
#include "BKE_context.h"

/* make dummy file */
#include "BLI_fileops.h"
#include "BLI_path_util.h"

	int bfbx_import(bContext *C, const char *filepath)
	{
		assert(C);
		assert(filepath);

		bassimp_import_settings defaults;
		defaults.enableAssimpLog = 0;
		defaults.reports = NULL;
		defaults.nolines = 0;
		defaults.triangulate = 0;

		bassimp::SceneImporter imp(filepath,*C,defaults);
		return imp.import() != 0 && imp.apply() != 0;
	}

	/*
    // export to fbx not currently implemented
	int bfbx_export(Scene *sce, const char *filepath, int selected, int apply_modifiers)
	{
		
		return 0;
	} */
}
