/**
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
#ifndef __IDENTIFIER_EXPR
#define __IDENTIFIER_EXPR

#include "Expression.h"

class CIdentifierExpr : public CExpression
{
	CValue*		m_idContext;
	STR_String	m_identifier;
public:
	CIdentifierExpr(const STR_String& identifier,CValue* id_context);
	virtual ~CIdentifierExpr();

	virtual CValue*			Calculate();
	virtual bool			MergeExpression(CExpression* otherexpr);
	virtual unsigned char	GetExpressionID();
	virtual bool			NeedsRecalculated();
	virtual CExpression*	CheckLink(std::vector<CBrokenLinkInfo*>& brokenlinks);
	virtual void			ClearModified();
	virtual void			BroadcastOperators(VALUE_OPERATOR op);


#ifdef WITH_CXX_GUARDEDALLOC
public:
	void *operator new( unsigned int num_bytes) { return MEM_mallocN(num_bytes, "GE:CIdentifierExpr"); }
	void operator delete( void *mem ) { MEM_freeN(mem); }
#endif
};

#endif //__IDENTIFIER_EXPR

