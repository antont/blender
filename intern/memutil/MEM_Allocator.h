/**
 *
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
 * Contributor(s): Peter Schlaile <peter@schlaile.de> 2005
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */

#ifndef __MEM_Allocator_h_included__
#define __MEM_Allocator_h_included__ 1

#include "guardedalloc/MEM_guardedalloc.h"

template<typename _Tp>
struct MEM_Allocator
{
	typedef size_t    size_type;
	typedef ptrdiff_t difference_type;
	typedef _Tp*       pointer;
	typedef const _Tp* const_pointer;
	typedef _Tp&       reference;
	typedef const _Tp& const_reference;
	typedef _Tp        value_type;

	template<typename _Tp1>
        struct rebind { 
		typedef MEM_Allocator<_Tp1> other; 
	};

	MEM_Allocator() throw() {}
	MEM_Allocator(const MEM_Allocator& __a) throw() {}

	template<typename _Tp1>
        MEM_Allocator(const MEM_Allocator<_Tp1> __a) throw() { }

	~MEM_Allocator() throw() {}

	pointer address(reference __x) const { return &__x; }

	const_pointer address(const_reference __x) const { return &__x; }

	// NB: __n is permitted to be 0.  The C++ standard says nothing
	// about what the return value is when __n == 0.
	_Tp* allocate(size_type __n, const void* = 0) {
		_Tp* __ret = 0;
		if (__n)
			__ret = static_cast<_Tp*>(
				MEM_mallocN(__n * sizeof(_Tp),
					    "STL MEM_Allocator"));
		return __ret;
	}

	// __p is not permitted to be a null pointer.
	void deallocate(pointer __p, size_type){ 
		MEM_freeN(__p);
	}

	size_type max_size() const throw() { 
		return size_t(-1) / sizeof(_Tp); 
	}

	void construct(pointer __p, const _Tp& __val) { 
		new(__p) _Tp(__val); 
	}

	void destroy(pointer __p) { 
		__p->~_Tp(); 
	}
};

#endif
