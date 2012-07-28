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

/** \file touch/intern/TOUCH_Manager.cpp
 *  \ingroup TOUCH
 */

#include "TOUCH_Manager.h"

#define TOUCH_DOWN_ENCODING 'd'
#define TOUCH_MOVE_ENCODING 'm'
#define TOUCH_UP_ENCODING 'u'

TOUCH_Context::TOUCH_Context(){}

TOUCH_Manager::TOUCH_Manager()
{
	//TODO?
}

TOUCH_Manager::~TOUCH_Manager()
{
	//TODO?
}

void TOUCH_Manager::RegisterContext(std::vector<TOUCH_Context> * context_type, const char * context_id)
{
	char encoding = checkRegisteredContext(context_type, context_id);
	if(!encoding) {
		encoding = 'a'; //XXX PLACEHOLDER
		TOUCH_Context * new_context = new TOUCH_Context;
		new_context->external_id = STR_String(context_id);
		new_context->internal_encoding = encoding;
		context_type->push_back(*new_context);
	}
}

void TOUCH_Manager::AddTouchEvent(std::vector<TOUCH_event_info> event) // XXX Vectors will not work with c api
{
	for(int i = 0; i < event.size(); i++){
		/* if index 1 is touching down for the first time, clear the input string */
		if(event[i].state == TOUCH_DOWN) {
			if(event[i].index == 1) {
				input_string.Clear();
			}
			//touch_position_begin[i] = event[i].position; XXX
		}

		//touch_position_last[i] = event[i].position; XXX

		switch(event[i].state){
			case TOUCH_DOWN:
				input_string += TOUCH_DOWN_ENCODING;
				break;
			case TOUCH_MOVE:
				input_string += TOUCH_MOVE_ENCODING;
				break;
			case TOUCH_UP:
				input_string += TOUCH_UP_ENCODING;
				break;
			default:
				break;
		}

		input_string += event[i].index;
#if 0
		input_string += checkRegisteredContext(&registered_area, event[i].area);
		input_string += checkRegisteredRegion(event[i].region);
		input_string += checkRegisteredData(event[i].data);
#endif
	}

#ifdef INPUT_TOUCH_DEBUG
	printf(input_string, std::endl);
#endif

}

char TOUCH_Manager::checkRegisteredContext(std::vector<TOUCH_Context> * context_type, const char * context_id)
{
	for(int i = 0; i < context_type->size(); i++) {
		if(context_id == context_type->at(i).external_id) {
			return context_type->at(i).internal_encoding;
		}
	}
	return '\0'; // XXX avoid null?
}

void TOUCH_Manager::CreateManager() {
	manager = new TOUCH_Manager();
}

void TOUCH_Manager::DestroyManager() {
	delete manager;
	manager = 0;
}

TOUCH_Manager * TOUCH_Manager::GetManager() {
	return manager;
}
