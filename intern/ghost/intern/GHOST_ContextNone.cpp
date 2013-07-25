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
 * The Original Code is Copyright (C) 2013 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Jason Wilkins
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file ghost/intern/GHOST_ContextNone.cpp
 *  \ingroup GHOST
 * Definition of GHOST_ContextNone class.
 */

#include "GHOST_ContextNone.h"



GHOST_TSuccess GHOST_ContextNone::swapBuffers()
{
	return GHOST_kSuccess;
}



GHOST_TSuccess GHOST_ContextNone::activateDrawingContext()
{
	return GHOST_kSuccess;
}



GHOST_TSuccess GHOST_ContextNone::installDrawingContext(bool, GHOST_TUns16)
{
	return GHOST_kSuccess;
}



GHOST_TSuccess GHOST_ContextNone::removeDrawingContext()
{
	return GHOST_kSuccess;
}
