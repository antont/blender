/**
 * $Id$
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

#include "MemoryResource.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

MemoryResource::MemoryResource()
	: m_data(0), m_dataSize(0)
{
}


MemoryResource::~MemoryResource()
{
	disposeData();
}


bool MemoryResource::load(HINSTANCE hInstApp, LPCTSTR lpName, LPCTSTR lpType)
{
	bool success = false;

	HRSRC hResInfo = ::FindResource(hInstApp, lpName, lpType);
	if (hResInfo) {
		m_dataSize = ::SizeofResource(hInstApp, hResInfo);
		if (m_dataSize) {
			success = allocateData(m_dataSize);
			if (success) {
				HGLOBAL hData = ::LoadResource(hInstApp, hResInfo);
				if (hData) {
					LPVOID pData = ::LockResource(hData);
					::memcpy(m_data, pData, m_dataSize);

				}
				else {
					success = false;
				}
			}
		}
	}

	return success;
}


bool MemoryResource::allocateData(unsigned int numBytes)
{
	disposeData();
	m_data = new char [m_dataSize];
	return m_data ? true : false;
}


void MemoryResource::disposeData(void)
{
	if (m_data) {
		delete [] m_data;
		m_data = 0;
	}
}
