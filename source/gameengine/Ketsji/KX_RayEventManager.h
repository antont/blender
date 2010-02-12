/**
 * Manager for ray events
 *
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#ifndef __KX_RAYEVENTMGR
#define __KX_RAYEVENTMGR
#include "SCA_EventManager.h"
#include <vector>
using namespace std;

class KX_RayEventManager : public SCA_EventManager
{
public:
	KX_RayEventManager(class SCA_LogicManager* logicmgr)
		: SCA_EventManager(logicmgr, RAY_EVENTMGR)
	{}
	virtual void NextFrame();


#ifdef WITH_CXX_GUARDEDALLOC
public:
	void *operator new( unsigned int num_bytes) { return MEM_mallocN(num_bytes, "GE:KX_RayEventManager"); }
	void operator delete( void *mem ) { MEM_freeN(mem); }
#endif
};

#endif //__KX_RAYEVENTMGR

