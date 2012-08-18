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
 * Contributor(s): Nicholas Rishel
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file touch/TOUCH_ContextGameEngine.h
 *  \ingroup TOUCH
 */

#ifndef __TOUCH_CONTEXTGAMEENGINE_H__
#define __TOUCH_CONTEXTGAMEENGINE_H__

#include "TOUCH_TypesGameEngine.h"
#include "TOUCH_Context.h"

class TOUCH_ContextGameEngine : TOUCH_Context {
public:
	TOUCH_ContextGameEngine();
	~TOUCH_ContextGameEngine();
protected:
};

#endif // __TOUCH_CONTEXTGAMEENGINE_H__
