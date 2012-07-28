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

/** \file touch/TOUCH_API.h
 *  \ingroup TOUCH
 */

#ifndef __TOUCH_API_H__
#define __TOUCH_API_H__

#include "TOUCH_Types.h"

TOUCH_DECLARE_HANDLE(TOUCH_Handle);

#ifdef __cplusplus
extern "C" {
#endif

extern TOUCH_Handle TOUCH_InitManager();
extern void TOUCH_DestoryManager(TOUCH_Handle * handle);
#if 0
extern void TOUCH_RegisterContext(TOUCH_Handle* handle, const char * context);
extern void TOUCH_RegisterRegion(TOUCH_Handle* handle, const char * context);
extern void TOUCH_RegisterData(TOUCH_Handle* handle, const char * context);
#endif
extern void TOUCH_AddTouchEvent(TOUCH_Handle* handle, void * event);

#ifdef __cplusplus
}
#endif

#endif /* __TOUCH_API_H__ */
