/**
 * $Id: SCA_ActuatorEventManager.h 15425 2008-07-04 08:14:50Z ben2610 $
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
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */
#ifndef __KX_ACTUATOREVENTMANAGER
#define __KX_ACTUATOREVENTMANAGER

#include "SCA_EventManager.h"

#include <vector>

using namespace std;

class SCA_ActuatorEventManager : public SCA_EventManager
{
	class SCA_LogicManager*	m_logicmgr;

public:
	SCA_ActuatorEventManager(class SCA_LogicManager* logicmgr);
	virtual ~SCA_ActuatorEventManager();
	virtual void NextFrame();
	virtual void UpdateFrame();
	//SCA_LogicManager* GetLogicManager() { return m_logicmgr;}
};

#endif //__KX_ACTUATOREVENTMANAGER

