/*
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
 * The Original Code is: all of this file.
 *
 * Contributor(s): Matt Ebb
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#ifndef POINTDENSITY_H
#define POINTDENSITY_H 

struct Render;
struct TexResult;
struct RenderParams;
struct ListBase;

/* Pointdensity Texture */

void tex_pointdensity_init(struct Render *re, struct Tex *tex);
void tex_pointdensity_free(struct Render *re, struct Tex *tex);

int tex_pointdensity_sample(struct RenderParams *rpm, struct Tex *tex,
	float *texvec, struct TexResult *texres);

/* Make point density kd-trees for all point density textures in the scene */

void pointdensity_make(struct Render *re, struct ListBase *lb);

#endif /* POINTDENSITY_H */

