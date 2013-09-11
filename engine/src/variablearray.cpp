/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "prefix.h"

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"
#include "mcio.h"

#include "stack.h"
#include "card.h"
#include "field.h"
#include "handler.h"
#include "hndlrlst.h"
#include "execpt.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "util.h"
#include "debug.h"
#include "globals.h"
#include "objectstream.h"
#include "osspec.h"
#include "redraw.h"

#include "core.h"

#include "stacksecurity.h"

void MCVariableArray::presethash(uint4 size)
{
	tablesize = size;
	table = new MCHashentry *[tablesize];
	memset((char *)table, 0, tablesize * sizeof(MCHashentry *));
	nfilled = keysize = 0;
	dimensions = EXTENT_ALLOCEVAL;
	extents = NULL;
}

void MCVariableArray::freehash(void)
{
	uint4 i;
	if (table != NULL)
		for (i = 0 ; i < tablesize ; i++)
			if (table[i] != NULL)
			{
				MCHashentry *e = table[i];
				while (e != NULL)
				{
					MCHashentry *next = e->next;
					delete e;
					e = next;
				}
			}
	delete table;
	if (extents != NULL)
		delete extents;
}

void MCVariableArray::resizehash(uint32_t p_new_tablesize)
{
	uint4 oldsize = tablesize;

	if (p_new_tablesize == 0)
		tablesize <<= 1;
	else
		tablesize = p_new_tablesize;

	MCHashentry **oldtable = table;
	table = new MCHashentry *[tablesize];
	memset((char *)table, 0, tablesize * sizeof(MCHashentry *));
	uint4 i;
	for (i = 0 ; i < oldsize ; i++)
	{
		if (oldtable[i] != NULL)
		{
			MCHashentry *e = oldtable[i];
			while (e != NULL)
			{
				MCHashentry *next = e->next;
				uint4 index = e->hash & tablesize - 1;
				e->next = table[index];
				table[index] = e;
				e = next;
			}
		}
	}
	delete oldtable;
}

////

void MCVariableArray::taketable(MCVariableArray *v)
{
	tablesize = v->tablesize;
	nfilled = v->nfilled;
	keysize = v->keysize;
	table = v->table;
	v->table = NULL;
	extents = v->extents;
	dimensions = v->dimensions;
	v->extents = NULL;
	v->dimensions = EXTENT_NONNUM;
}

bool MCVariableArray::copytable(const MCVariableArray &v)
{
	tablesize = v.tablesize;
	nfilled = v.nfilled;
	keysize = v.keysize;
	dimensions = v.dimensions;
	if (v.extents != NULL)
	{
		extents = new arrayextent[dimensions];
		if (extents == NULL)
			goto no_memory;
		memcpy(extents,v.extents,sizeof(arrayextent) * dimensions);
	}
	else
		extents = NULL;
	table = new MCHashentry *[tablesize];
	if (table == NULL)
		goto no_memory;
	memset((char *)table, 0, tablesize * sizeof(MCHashentry *));
	uint4 i;
	for (i = 0 ; i < tablesize ; i++)
	{
		if (v.table[i] != NULL)
		{
			MCHashentry *e = v.table[i];
			while (e != NULL)
			{
				MCHashentry *ne;
				ne = e -> Clone();
				if (ne == NULL)
					goto no_memory;
				ne->next = table[i];
				table[i] = ne;
				e = e->next;
			}
		}
	}
	return true;

no_memory:
	freehash();
	return false;
}

////

uint4 MCVariableArray::computehash(const MCString &s)
{
	uint4 value = 0;
	uint4 length = s.getlength();
	const char *sptr = s.getstring();
	while (length--)
		value += (value << 3) + MCS_tolower(*sptr++);
	return value;
}

void MCVariableArray::extentfromkey(char *skey)
{
	if (extents == NULL)
	{ //ok extents is null
		/*what it means if extents is null and dimensions is non 0
		  dimensions	extents	means
		  0			null	non-numeric array
		  1			null	need to recal in calcextents
		  >=2			null	first element being set evaluate and allocate
		  >=0			alloc	is numeric evaluate key and add to extents	*/
		if (dimensions != EXTENT_ALLOCEVAL) //either its non-numeric or needs to be recalculated
			return;
		dimensions = EXTENT_NONNUM;
		/*	or it's been set so that it's evaluated
		and extents will be allocated...*/
	}
	uint1 tdimensions = 0;
	MCString ts;
	uint4 num;
	char *sptr = skey;
	char *eptr;
	while ((eptr = strchr(sptr, ',')) != NULL || sptr != NULL)
	{
		uint2 length = eptr != NULL ? eptr - sptr : strlen(sptr);
		ts.set(sptr, length);

		// OK-2008-12-17: [[Bug 7529]] - Changed the extents from a uint2 to a uint4, that should hold enough keys for now
		if (MCU_stoui4(ts, num) && (tdimensions < dimensions
		                            || dimensions == EXTENT_NONNUM))
		{
			/*to get here key has to be numeric and new
			dimensions(tdimensions) are not greater than previous recorded
			dimensions. of course if dimensions is 0 then we get the
			dimensions and compare next element.*/
			if (dimensions == 0)
				MCU_realloc((char **)&extents, tdimensions, tdimensions + 1,
				            sizeof(arrayextent));
			// MW-2004-11-26: Switched logic as extents array might be initialised (VG)
			if (dimensions == EXTENT_NONNUM || num > extents[tdimensions].max)
				extents[tdimensions].max = num;
			if (dimensions == EXTENT_NONNUM || num < extents[tdimensions].min)
				extents[tdimensions].min = num;
			tdimensions++;
		}
		else
		{
			//delete extents and set it and dimensions to null (saying array is not numeric)
			delete extents;
			extents = NULL;
			dimensions = EXTENT_NONNUM;
			return;
		}
		if (eptr == NULL)
			break;
		sptr = eptr + 1;
		if (!*sptr)
			break;
	}
	if (dimensions == EXTENT_NONNUM)
		dimensions = tdimensions;
	if (dimensions > tdimensions)
	{
		//new dimensions(tdimensions) are less than previously recorded(dimensions)
		delete extents;
		extents = NULL;
		dimensions = EXTENT_NONNUM;
	}
}

MCHashentry *MCVariableArray::lookuphash(const MCString &s, Boolean cs, Boolean add)
{
	uint4 hash = computehash(s);
	uint4 index = hash & tablesize - 1;
	MCHashentry *e = table[index];
	while (e != NULL)
	{
		if (cs)
		{
			uint4 l = strlen(e->string);
			if (hash == e->hash && s.getlength() == l
			        && !strncmp(s.getstring(), e->string, l))
				return e;
		}
		else
			if (hash == e->hash && s == e->string)
				return e;
		e = e->next;
	}
	if (add)
	{
		if (nfilled++ > tablesize)
		{
			resizehash();
			index = hash & tablesize - 1;
		}
		e = MCHashentry::Create(s, hash);
		uint4 length = s.getlength();
		extentfromkey(e->string);
		keysize += length + 1;
		e->next = table[index];
		table[index] = e;
		return e;
	}

	return NULL;
}

void MCVariableArray::removehash(const MCString& s, Boolean cs)
{
	uint4 hash = computehash(s);
	uint4 index = hash & tablesize - 1;
	MCHashentry *e = table[index];
	MCHashentry *last = NULL;
	Boolean match = False;
	while (e != NULL)
	{
		if (cs)
		{
			uint4 l = strlen(e->string);
			if (hash == e->hash && s.getlength() == l
			        && !strncmp(s.getstring(), e->string, l))
				match = True;
		}
		else
			if (hash == e->hash && s == e->string)
				match = True;
		if (match)
		{
			if (last == NULL)
				table[index] = e->next;
			else
				last->next = e->next;
			delete e;
			keysize -= s.getlength() + 1;
			nfilled--;
			delete extents;
			extents = NULL;
			if (nfilled == 0)
				dimensions = EXTENT_ALLOCEVAL;
			else
				dimensions = EXTENT_RECALC;
			return;
		}
		last = e;
		e = e->next;
	}
}

void MCVariableArray::removehash(MCHashentry *p_hash)
{
	uint4 index;
	index = p_hash -> hash & (tablesize - 1);
	
	MCHashentry *last;
	if (table[index] == p_hash)
		last = NULL;
	else
		for(last = table[index]; last -> next != p_hash; last = last -> next)
			;
	
	if (last == NULL)
		table[index] = p_hash -> next;
	else
		last -> next = p_hash -> next;
		
	keysize -= strlen(p_hash -> string) + 1;
	nfilled -= 1;
	delete extents;
	extents = NULL;
	if (nfilled == 0)
		dimensions = EXTENT_ALLOCEVAL;
	else
		dimensions = EXTENT_RECALC;
	
	delete p_hash;
}

void MCVariableArray::getextents(MCExecPoint &ep)
{
	ep.clear();
	calcextents();
	uint1 i;
	if (extents != NULL)
		for (i = 0 ; i < dimensions ; i++)
		{
			char buf[(U4L * 2) + 1];
			sprintf(buf, "%d,%d", extents[i].min, extents[i].max);
			ep.concatcstring(buf, EC_RETURN, i == 0);
		}
}

void MCVariableArray::getkeys(char **keylist, uint4 kcount)
{
	uint4 i;
	uint4 count = 0;
	for (i = 0 ; i < tablesize ; i++)
		if (table[i] != NULL)
		{
			MCHashentry *e = table[i];
			while (e != NULL)
			{
				keylist[count] = e->string;
				e = e->next;
				count++;
				if (count == kcount)
					break;
			}
		}
}

void MCVariableArray::getkeys(MCExecPoint &ep)
{
	ep.clear();
	char *startptr = ep.getbuffer(keysize);
	char *dptr = startptr;
	uint4 i;
	for (i = 0 ; i < tablesize ; i++)
		if (table[i] != NULL)
		{
			MCHashentry *e = table[i];
			while (e != NULL)
			{
				uint4 length = strlen(e->string);
				memcpy(dptr, e->string, length);
				dptr += length;
				*dptr++ = '\n';
				e = e->next;
			}
		}
	if (dptr != startptr)
		dptr--;
	ep.setlength(dptr - startptr);
}

Exec_stat MCVariableArray::transpose(MCVariableArray& v)
{
	v.calcextents();
	if (v.extents == NULL || v.dimensions != 2 || v.ismissingelement() == True)
		return ES_ERROR;
	presethash(v.nfilled);
	uint2 i, j;
	char tbuf[(U4L * 2) + 1];
	for (i = v.extents[COL_DIM].min; i <= v.extents[COL_DIM].max; i++)
		for (j = v.extents[ROW_DIM].min; j <= v.extents[ROW_DIM].max; j++)
		{
			sprintf(tbuf,"%d,%d", j, i);
			MCHashentry *e = v.lookuphash(tbuf, False, False);
			if (e == NULL)
				return ES_ERROR;
			sprintf(tbuf,"%d,%d", i, j);
			MCHashentry *ne = lookuphash(tbuf, False, True);
			ne -> value . assign(e -> value);
		}
	return ES_NORMAL;
}

Exec_stat MCVariableArray::dofunc(MCExecPoint& ep, Functions func, uint4 &nparams, real8 &n, real8 oldn, void *titems)
{
	uint4 i;
	for (i = 0 ; i < tablesize ; i++)
		if (table[i] != NULL)
		{
			MCHashentry *e = table[i];
			while (e != NULL)
			{
				real64_t value;
				if (!e -> value . get_as_real(ep, value))
					return ES_ERROR;
				MCU_dofunc(func, nparams, n, value, oldn, (MCSortnode *)titems);
				e = e->next;
			}
		}
	return ES_NORMAL;
}

Exec_stat MCVariableArray::factorarray(MCExecPoint &ep, Operators op)
{
	uint4 i;
	MCS_seterrno(0);
	if (ep.getformat() == VF_ARRAY)
	{
		MCVariableArray *v = ep.getarray() -> get_array();
		for (i = 0 ; i < v->tablesize ; i++)
			if (v->table[i] != NULL)
			{
				MCHashentry *e = v->table[i];
				MCHashentry *de;
				real64_t dst_value, src_value;
				while (e != NULL)
				{
					if ((de = lookuphash(e->string, False, False)) == NULL ||
					        !de -> value . get_as_real(ep, dst_value) || !e -> value . get_as_real(ep, src_value))
						return ES_ERROR;
					switch (op)
					{
					case O_PLUS:
						dst_value += src_value;
						break;
					case O_MINUS:
						dst_value -= src_value;
						break;
					case O_TIMES:
						dst_value *= src_value;
						break;
					case O_DIV:
						dst_value /= src_value;
						if (dst_value != MCinfinity && MCS_geterrno() == 0)
						{
							if (dst_value < 0.0)
								dst_value = ceil(dst_value);
							else
								dst_value = floor(dst_value);
						}
						break;
					case O_MOD:
						{
							real8 n = dst_value;
							dst_value = n/src_value;
							if (dst_value != MCinfinity && MCS_geterrno() == 0)
								dst_value = fmod(n, src_value);
						}
						break;
					case O_WRAP:
						{
							real8 n = dst_value;
							dst_value = n/src_value;
							if (dst_value != MCinfinity && MCS_geterrno() == 0)
								dst_value = MCU_fwrap(n, src_value);
						}
						break;
					default:
						dst_value /= src_value;
						break;
					}
					if (src_value == MCinfinity || MCS_geterrno() != 0)
					{
						MCS_seterrno(0);
						if (src_value == 0.0)
							MCeerror->add(EE_DIVIDE_ZERO, 0, 0);
						else
							MCeerror->add(EE_MATRIX_RANGE, 0, 0);
						return ES_ERROR;
					}
					de -> value . assign_real(dst_value);
					e = e->next;
				}
			}
	}
	else
	{
		real8 tnum = ep.getnvalue();
		for (i = 0 ; i < tablesize ; i++)
			if (table[i] != NULL)
			{
				MCHashentry *e = table[i];
				while (e != NULL)
				{
					real64_t value;
					if (!e -> value . get_as_real(ep, value))
						return ES_ERROR;
					switch (op)
					{
					case O_PLUS:
						value += tnum;
						break;
					case O_MINUS:
						value -= tnum;
						break;
					case O_TIMES:
						value *= tnum;
						break;
					case O_DIV:
						value /= tnum;
						if (value != MCinfinity && MCS_geterrno() == 0)
						{
							if (value < 0.0)
								value = ceil(value);
							else
								value = floor(value);
						}
						break;
					case O_MOD:
						{
							real8 n = value;
							value = n / tnum;
							if (value != MCinfinity && MCS_geterrno() == 0)
								value = fmod(n, tnum);
						}
						break;					
					case O_WRAP:
						{
							real8 n = value;
							value = n / tnum;
							if (value != MCinfinity && MCS_geterrno() == 0)
								value = MCU_fwrap(n, tnum);
						}
						break;
					default:
						value /= tnum;
						break;
					}
					if (value == MCinfinity || MCS_geterrno() != 0)
					{
						MCS_seterrno(0);
						if (tnum == 0.0)
							MCeerror->add(EE_DIVIDE_ZERO, 0, 0);
						else
							MCeerror->add(EE_MATRIX_RANGE, 0, 0);
						return ES_ERROR;
					}
					e -> value . assign_real(value);
					e = e->next;
				}
			}
	}
	return ES_NORMAL;
}

// MERG-2013-08-26: [[ RecursiveArrayOp ]] Support nested arrays in union and intersect
Exec_stat MCVariableArray::intersectarray(MCVariableArray& v, bool p_recursive)
{
	uint4 i;
	MCHashentry *last;
	for (i = 0 ; i < tablesize ; i++)
		if (table[i] != NULL)
		{
			last = NULL;
			MCHashentry *e = table[i];
			while (e != NULL)
			{
                MCHashentry *t_entry = v.lookuphash(e->string, False, False);
				if (t_entry == NULL)
				{
					if (last == NULL)
						table[i] = e->next;
					else
						last->next = e->next;
					keysize -= strlen(e->string) + 1;
					nfilled--;
					MCHashentry *tmp = e->next;
					delete e;
					e = tmp;
					delete extents;//clear extents
					extents = NULL;
					dimensions = nfilled == 0 ? EXTENT_ALLOCEVAL : EXTENT_RECALC;
				}
				else
				{
                    if (t_entry -> value . is_array() && e -> value . is_array() && p_recursive)
                        e -> value . intersectarray(t_entry -> value, p_recursive);
					last = e;
					e = e->next;
				}
			}
		}

	return ES_NORMAL;
}

// MERG-2013-08-26: [[ RecursiveArrayOp ]] Support nested arrays in union and intersect
Exec_stat MCVariableArray::unionarray(MCVariableArray& v, bool p_recursive)
{
	uint4 i;
	for (i = 0 ; i < v.tablesize ; i++)
		if (v.table[i] != NULL)
		{
			MCHashentry *e = v.table[i];
			while (e != NULL)
			{
                MCHashentry *t_entry = lookuphash(e->string, False, False);
                if (t_entry == NULL)
				{
					MCHashentry *ne = lookuphash(e->string, False, True);
					ne -> value . assign(e -> value);
				}
                else if (e -> value . is_array() && p_recursive)
                    t_entry -> value . unionarray(e ->value, p_recursive);
               
                e = e->next;
			}
		}
	return ES_NORMAL;
}

void MCVariableArray::calcextents(void)
{
	if (extents != NULL || dimensions != EXTENT_RECALC)
		return;
	dimensions = EXTENT_ALLOCEVAL;
	uint4 i;
	for (i = 0 ; i < tablesize ; i++)
		if (table[i] != NULL)
		{
			MCHashentry *e = table[i];
			while (e != NULL)
			{
				extentfromkey(e->string);
				e = e->next;
			}
			if (dimensions == EXTENT_NONNUM)
				break;
		}
}

uint2 MCVariableArray::getextent(uint1 tdimension) const
{
	if (extents == NULL || tdimension > dimensions)
		return 0;
	return extents[tdimension].max - extents[tdimension].min + 1;
}

Boolean MCVariableArray::ismissingelement(void) const
{
	if (extents == NULL)
		return True;
	uint2 telements = getextent(0);
	uint2 i;
	for (i = 1; i < dimensions; i++)
		telements *= getextent((uint1)i);
	return !(telements == nfilled);
}

Exec_stat MCVariableArray::matrixmultiply(MCExecPoint& ep, MCVariableArray &va, MCVariableArray &vb)
{
	va.calcextents();
	vb.calcextents();
	if (va.extents == NULL || vb.extents == NULL)
		return ES_ERROR; //one is not numeric
	if (!(va.dimensions == 2 && vb.dimensions == va.dimensions) ||
	        !(va.getextent(COL_DIM) == vb.getextent(ROW_DIM)) ||
	        !(!va.ismissingelement() && !vb.ismissingelement()))
		return ES_ERROR; //columns does not equal rows
	presethash(va.getextent(ROW_DIM) * vb.getextent(COL_DIM));
	uint2 i,j,k;
	char tbuf[(U4L * 2) + 1];
	MCHashentry *vaptr,*vbptr,*vcptr;
	for (i = va.extents[ROW_DIM].min; i <= va.extents[ROW_DIM].max; i++)
	{
		for (j = vb.extents[COL_DIM].min; j <= vb.extents[COL_DIM].max ; j++)
		{
			real64_t value;

			sprintf(tbuf,"%d,%d",i,j);
			vcptr = lookuphash(tbuf, False, True);
			value = 0.0;
			for (k = va.extents[COL_DIM].min; k <= va.extents[COL_DIM].max; k++)
			{
				sprintf(tbuf,"%d,%d",i,k);
				vaptr = va.lookuphash(tbuf, False, False);
				sprintf(tbuf,"%d,%d",k,j);
				vbptr = vb.lookuphash(tbuf, False, False);

				real64_t avalue, bvalue;
				if ((vaptr == NULL || vbptr == NULL) ||
				        (!vaptr -> value . get_as_real(ep, avalue) || !vbptr -> value . get_as_real(ep, bvalue)))
					return ES_ERROR;

				value += avalue * bvalue;
			}

			vcptr -> value . assign_real(value);
		}
	}
	return ES_NORMAL;
}

Boolean MCVariableArray::isnumeric(void)
{
	calcextents();
	if (table == NULL || extents == NULL || ismissingelement() == True)
		return False;
	return True;
}

MCHashentry *MCVariableArray::getnextelement(uint4 &l, MCHashentry *e, Boolean donumeric, MCExecPoint &ep)
{
	MCHashentry *ne = NULL;
	if (donumeric && dimensions == 1)
	{ //use numeric
		char tbuf[U4L];
		uint32_t i;
		i = extents[ROW_DIM].min + l;
		if (i > extents[ROW_DIM].max)
			return NULL;
		sprintf(tbuf, "%d", i);
		ne = lookuphash(tbuf, False, False);
		l++;
	}
	else
	{
		if (table == NULL)
			return NULL;
		if (e != NULL && e->next != NULL)
			ne = e->next;
		else
			while (l < tablesize)
			{
				ne = table[l];
				l++;
				if (ne != NULL)
					break;
			}
	}
	
	if (ne != NULL && ne -> value . is_number())
		ne -> value . ensure_string(ep);

	return ne;
}

MCHashentry *MCVariableArray::getnextkey(uint4& l, MCHashentry *e) const
{	
	if (e != NULL && e -> next != NULL)
		return e -> next;
	
	while(l < tablesize)
		if (table[l++] != NULL)
			return table[l - 1];

	return NULL;
}

MCHashentry *MCVariableArray::getnextkey(MCHashentry *e) const
{
	if (e != NULL && e -> next != NULL)
		return e -> next;

	uint32_t l;
	if (e != NULL)
		l = (( e -> hash) & (tablesize - 1)) + 1;
	else
		l = 0;
	while(l < tablesize)
		if (table[l++] != NULL)
			return table[l - 1];

	return NULL;
}

void MCVariableArray::combine(MCExecPoint& ep, char el, char k, char*& r_buffer, uint32_t& r_length)
{
	MCSortnode *items = new MCSortnode[nfilled];
	uint4 i;
	uint4 ncount = 0;
	uint4 ssize = 0;
	for (i = 0 ; i < tablesize ; i++)
		if (table[i] != NULL)
		{
			MCHashentry *e = table[i];
			while (e != NULL)
			{
				if (e -> value . ensure_string(ep))
				{
					ssize += e -> value . get_string() . getlength() + 2;
					items[ncount].data = e;
					items[ncount++].svalue = e->string;
				}
				e = e->next;
			}
		}

	MCU_sort(items, ncount, ST_ASCENDING, ST_TEXT);

	uint32_t size;
	size = ssize + keysize;

	char *sptr = new char[size];
	
	ssize = 0;
	for (i = 0 ; i < ncount ; i++)
	{
		MCHashentry *e = (MCHashentry *)items[i].data;

		uint4 esize;
		const char *estring;
		if (!e -> value . is_array())
		{
			esize = e -> value . get_string() . getlength();
			estring = e -> value . get_string() . getstring();
		}
		else
		{
			esize = 0;
			estring = NULL;
		}

		if (k)
		{
			uint4 ksize = strlen(e->string) + 1;
			memcpy(&sptr[ssize], e->string, ksize);
			ssize += ksize;
			sptr[ssize - 1] = k;
		}

		memcpy(&sptr[ssize], estring, esize);

		ssize += esize + 1;
		sptr[ssize - 1] = el;
	}
	delete items;
	
	r_buffer = sptr;

	// 2008-07-21: [[ Bug 6804 ]] 'combine' appends an extra separator to the end
	//   of a string. Our combined string always has a separato at the end after
	//   termination of the loop, so make sure we don't include it in the length.
	r_length = ssize - 1;
}

void MCVariableArray::split(const MCString& s, char e, char k)
{
	presethash(TABLE_SIZE);

	char numkey[U4L];
	const char *sptr, *etoken, *ktoken, *endptr;
	uint4 count = 0;

	sptr = s.getstring();
	endptr = sptr + s.getlength();
	while (sptr < endptr)
	{
		etoken = ktoken = sptr;
		uint4 size = endptr - sptr;
		if (!MCU_strchr(etoken, size, e))
			etoken = endptr;
		size = etoken - sptr;
		count++;
		MCHashentry *hptr;
		if (!k)
		{
			sprintf(numkey, "%d", count);
			hptr = lookuphash(numkey, True, True);
		}
		else
		{
			if (!MCU_strchr(ktoken, size, k))
				ktoken = etoken;
			MCString skey(sptr, ktoken - sptr);
			hptr = lookuphash(skey, True, True);
			sptr = ktoken + 1;
			if (sptr < etoken)
				size = etoken - sptr;
			else
				size = 0;
		}
		hptr -> value . assign_string(MCString(sptr, size));
		sptr = etoken + 1;
	}
}

struct MCColumnBuffer
{
	char *buffer;
	uint32_t capacity;
	uint32_t frontier;
};

void MCVariableArray::combine_column(MCExecPoint& ep, char p_row_delimiter, char p_column_delimiter, char*& r_buffer, uint32_t& r_length)
{
	// Calculate the size of the variable's array
	calcextents();

	// If this is a non-numeric array, or it has more than one dimension it isn't suitable for
	// splitting.
	if (dimensions == EXTENT_NONNUM || dimensions > 1)
		return;

	// We expect columns to have indices [1..32767]
	if (extents[0] . min < 1 || extents[0] . max > 32767)
		return;

	// Allocate a temporary array of strings to hold the contents of each column in order
	// A 'NULL' in any one of these entries means the column should be ignored.
	uint4 t_column_count;
	t_column_count = extents[0] . max;

	MCString *t_entries;
	t_entries = new MCString[t_column_count];
	if (t_entries == NULL)
		return;

	bool *t_entries_dead;
	t_entries_dead = new bool[t_column_count];
	if (t_entries_dead == NULL)
	{
		delete t_entries;
		return;
	}

	for(unsigned int i = 0; i < t_column_count; ++i)
	{
		t_entries[i] . set(NULL, 0);
		t_entries_dead[i] = false;
	}

	// The 'live column count' is the number of columns that still have data in them to
	// process. We iterate down rows until this reaches zero. Note that columns that are
	// not present (i.e. the key is not there) are not considered live and cause no
	// output to be generated.
	uint4 t_live_column_count;
	t_live_column_count = 0;

	// Collect the strings for each column. We achieve this by iterating through all the
	// keys of the array, and entering the corresponding string into the correct index
	// in 'entries'.
	//
	// Here 't_size' holds the total length of all the columns strings combined. We use
	// this to allocate a large enough string to hold the output.
	uint4 t_size;
	t_size = 0;
	for(uint4 t_index = 0; t_index < tablesize; ++t_index)
	{
		for(MCHashentry *t_entry = table[t_index]; t_entry != NULL; t_entry = t_entry -> next)
			if (!t_entry -> value . is_undefined())
			{
				uint2 t_column;
				if (!MCU_stoui2(t_entry -> string, t_column))
					assert(false);

				if (t_entry -> value . ensure_string(ep))
				{
					t_entries[t_column - 1] = t_entry -> value . get_string();
					t_size += t_entries[t_column - 1] . getlength() + 2;
				}
				else
				{
					t_entries[t_column - 1] . set("", 0);
					t_size += 2;
				}

				t_live_column_count += 1;
			}
	}

	char *t_output;
	t_output = (char *)malloc(t_size);
	
	char *t_frontier, *t_limit;
	t_frontier = t_output;
	t_limit = t_frontier + t_size;

	if (t_output == NULL)
	{
		delete t_entries;
		delete t_entries_dead;
		return;
	}

	// Iterate row-wise until all columns are exhausted.
	while(t_live_column_count > 0)
	{
		for(uint4 t_column = 0; t_column < t_column_count; ++t_column)
		{
			// We now parse the next cell in the column. Here the column's full string is given by:
			//   [t_column_start .. t_column_start + t_column_length]
			// And the current cell is given by:
			//	 [t_cell_start .. t_cell_end]

			const char *t_column_start;
			t_column_start =  t_entries[t_column] . getstring();

			uint4 t_column_length;
			t_column_length = t_entries[t_column] . getlength();

			// If an entry contains a NULL pointer, the column is to be ignored and generate no output.
			if (t_column_start == NULL)
				continue;

			const char *t_cell_start, *t_cell_end;
			t_cell_start = t_column_start;
			t_cell_end = t_cell_start;

			// If the column is not empty, then the end of the current cell is either the location
			// of the next row-delimiter char, or the end of the column string if none is found.
			if (t_column_length != 0)
			{
				if (t_cell_start[0] != p_row_delimiter)
				{
					uint4 t_length;
					t_length = t_column_length;
					if (!MCU_strchr(t_cell_end, t_length, p_row_delimiter, False))
						t_cell_end = t_cell_start + t_column_length;
				}
			}

			uint4 t_cell_length;
			t_cell_length = t_cell_end - t_cell_start;

			// If the length of this string is zero after taking into account the length of the current cell
			// and it hasn't been declared dead before, declare it dead and reduce the number of live columns.
			// By only checking for death here, we ensure that any final row delimiters are taken appropriately
			// into account.
			// i.e. if tColumns[1] = "", tColumns[2] = return, tColumns[3] = "" then the output
			//      will be tab & tab & return as opposed to empty
			if ((t_column_length - t_cell_length) == 0 && !t_entries_dead[t_column])
			{
				t_entries_dead[t_column] = true;
				t_live_column_count -= 1;
			}

			// Ensure the output buffer is long enough to contain the current cell.
			if (t_limit - t_frontier < (int4)(t_cell_length + 1))
			{
				uint4 t_new_size;
				t_new_size = (t_size + t_cell_length + 1 + VAR_PAD) & VAR_MASK;

				char *t_new_output;
				t_new_output = (char *)realloc(t_output, t_new_size);

				t_frontier += t_new_output - t_output;
				t_limit = t_new_output + t_new_size;
				t_output = t_new_output;
				t_size = t_new_size;
			}

			// Append the cell to the output buffer
			memcpy(t_frontier, t_cell_start, t_cell_length);
			t_frontier += t_cell_length;

			// Update the column's string pointer. If the column has text left then
			// the character after the current cell must be a row-delimiter so consume
			// it.
			t_column_start += t_cell_length;
			t_column_length -= t_cell_length;
			if (t_column_length > 0)
			{
				t_column_start += 1;
				t_column_length -= 1;
			}

			t_entries[t_column] . set(t_column_start, t_column_length);

			// If we are not the last column then add a column separator, otherwise
			// if there are still living columns, add a row delimiter.
			if (t_column + 1 < t_column_count)
				*t_frontier++ = p_column_delimiter;
			else if (t_live_column_count > 0)
				*t_frontier++ = p_row_delimiter;
		}
	}

	delete[] t_entries;
	delete[] t_entries_dead;

	r_buffer = (char *)realloc(t_output, t_frontier - t_output);
	r_length = t_frontier - t_output;
}

// This function returns a pointer to the first instance of 'c' after 'frontier',
// or 'limit' if it reaches that first.
static const char *strchr_limit(const char *frontier, const char *limit, char c)
{
	while(frontier < limit)
	{
		if (*frontier == c)
			return frontier;
			
		frontier += 1;
	}
			
	return limit;
}

void MCVariableArray::split_column(const MCString& s, char p_row_delimiter, char p_column_delimiter)
{
	// Initialize the array part of ourselves
	presethash(TABLE_SIZE);

	// Frontier is our position in the string
	const char *t_frontier;
	t_frontier = s . getstring();

	// Limit is the last character in the string
	const char *t_limit;
	t_limit = t_frontier + s . getlength();

	// Arity is the number of columns we have so far
	uint4 t_arity;
	t_arity = 0;

	// Columns is a pointer to the hash entries for each column
	MCColumnBuffer *t_columns;
	t_columns = NULL;

	// Current row index
	uint4 t_row_index;
	t_row_index = 0;

	while(t_frontier < t_limit)
	{
		// Increment our row index
		t_row_index += 1;

		// MW-2009-01-22: [[ Bug 7408 ]] Use the strchr_limit function to correctly
		//   search only until the end of the string.
		//
		// Compute the end of the current row
		const char *t_row_limit;
		t_row_limit = strchr_limit(t_frontier, t_limit, p_row_delimiter);

		// Initialize a column index
		uint4 t_column_index;
		t_column_index = 0;
		
		// Now loop through the cells in the row
		while(t_frontier <= t_row_limit)
		{
			// Increment the column index
			t_column_index += 1;

			// MW-2009-01-22: [[ Bug 7408 ]] Use the strchr_limit function to correctly
			//   search only until the end of the row.
			//
			const char *t_cell_limit;
			t_cell_limit = strchr_limit(t_frontier, t_row_limit, p_column_delimiter);

			// If this row terminates in a column char then ignore it if it would
			// means creating a new column
			if (t_frontier == t_row_limit && t_arity < t_column_index)
			{
				t_frontier += 1;
				break;
			}

			// If this row has more columns than the others - make up the difference
			if (t_arity < t_column_index)
			{
				t_arity = t_column_index;
				t_columns = (MCColumnBuffer *)realloc(t_columns, sizeof(MCColumnBuffer) * t_arity);

				MCColumnBuffer *t_column;
				t_column = &t_columns[t_arity - 1];
				if (t_row_index > 1)
				{
					t_column -> capacity = ((t_row_index - 1) + VAR_PAD) & VAR_MASK;
					t_column -> buffer = (char *)malloc(t_column -> capacity);
					memset(t_column -> buffer, p_row_delimiter, t_row_index - 1);
					t_column -> frontier = t_row_index - 1;
				}
				else
				{
					t_column -> capacity = 0;
					t_column -> buffer = NULL;
					t_column -> frontier = 0;
				}

			}
			
			// Grab a reference to the appropriate column
			MCColumnBuffer *t_entry;
			t_entry = &t_columns[t_column_index - 1];

			uint4 t_cell_size;
			t_cell_size = t_cell_limit - t_frontier;

			uint4 t_old_size;
			t_old_size = t_entry -> frontier;

			uint4 t_size;
			t_size = t_old_size + t_cell_size;
			if (t_row_limit != t_limit)
				t_size += 1;

			if (t_size > t_entry -> capacity)
			{
				t_entry -> capacity = (t_size + VAR_PAD) & VAR_MASK;
				t_entry -> buffer = (char *)realloc(t_entry -> buffer, t_entry -> capacity);
			}

			memcpy(t_entry -> buffer + t_old_size, t_frontier, t_cell_size);
			if (t_row_limit != t_limit)
				t_entry -> buffer[t_size - 1] = p_row_delimiter;

			t_entry -> frontier = t_size;

			// Our frontier is now the character following the cell delimiter
			// If we are the last cell in a row, then this will be ready for
			// the next loop round the 'row' loop.
			t_frontier = t_cell_limit + 1;
		}

		// Pad the rest of the columns with a delimiter
		if (t_row_limit != t_limit)
		{
			while(t_column_index < t_arity)
			{
				t_column_index += 1;

				MCColumnBuffer *t_entry;
				t_entry = &t_columns[t_column_index - 1];

				uint4 t_old_size;
				t_old_size = t_entry -> frontier;

				uint4 t_size;
				t_size = t_old_size + 1;

				if (t_size > t_entry -> capacity)
				{
					t_entry -> capacity = (t_size + VAR_PAD) & VAR_MASK;
					t_entry -> buffer = (char *)realloc(t_entry -> buffer, t_entry -> capacity);
				}

				t_entry -> buffer[t_size - 1] = p_row_delimiter;
				t_entry -> frontier = t_size;
			}
		}
	}

	for(uint32_t i = 0; i < t_arity; i++)
	{
		char t_index[U4L];
		sprintf(t_index, "%u", i + 1);

		MCHashentry *e;
		e = lookuphash(t_index, True, True);
		if (t_columns[i] . frontier > 0)
			e -> value . assign_buffer(t_columns[i] . buffer, t_columns[i] . frontier);
		else
			e -> value . assign_empty();
	}

	free(t_columns);
}

void MCVariableArray::combine_as_set(MCExecPoint& ep, char el, char*& r_buffer, uint32_t& r_length)
{
	MCSortnode *items = new MCSortnode[nfilled];
	uint4 i;
	uint4 ncount = 0;
	uint4 ssize = 0;
	for (i = 0 ; i < tablesize ; i++)
		if (table[i] != NULL)
		{
			MCHashentry *e = table[i];
			while (e != NULL)
			{
				if (e -> value . is_string() && e -> value . get_string() == MCtruemcstring)
				{
					ssize += strlen(e -> string) + 1;
					items[ncount].data = e;
					items[ncount++].svalue = e->string;
				}
				e = e->next;
			}
		}

	uint32_t size;
	size = ssize;

	char *sptr = new char[size];
	
	ssize = 0;
	for (i = 0 ; i < ncount ; i++)
	{
		MCHashentry *e = (MCHashentry *)items[i].data;

		uint4 esize;
		const char *estring;
		estring = e -> string;
		esize = strlen(e -> string);

		memcpy(&sptr[ssize], estring, esize);

		ssize += esize + 1;
		sptr[ssize - 1] = el;
	}
	delete items;
	
	r_buffer = sptr;

	// 2008-07-21: [[ Bug 6804 ]] 'combine' appends an extra separator to the end
	//   of a string. Our combined string always has a separato at the end after
	//   termination of the loop, so make sure we don't include it in the length.
	r_length = ssize - 1;
}

void MCVariableArray::split_as_set(const MCString& s, char e)
{
	presethash(TABLE_SIZE);

	char numkey[U4L];
	const char *sptr, *etoken, *endptr;
	uint4 count = 0;

	sptr = s.getstring();
	endptr = sptr + s.getlength();
	while (sptr < endptr)
	{
		etoken = sptr;
		uint4 size = endptr - sptr;
		if (!MCU_strchr(etoken, size, e))
			etoken = endptr;
		size = etoken - sptr;
		count++;
		MCHashentry *hptr;
		
		MCString skey(sptr, etoken - sptr);
		hptr = lookuphash(skey, True, True);
		hptr -> value . assign_constant_string(MCtruemcstring);
		sptr = etoken + 1;
	}
}

// MERG-2013-05-07: [[ RevisedPropsProp ]] Array of object props that must be set first
//   to ensure other properties don't set them differently.
static struct { Properties prop; const char *tag; } s_preprocess_props[] =
{
    // MERG-2013-08-30: [[ RevisedPropsProp ]] Ensure lockLocation of groups is set before rectangle
    { P_LOCK_LOCATION, "lockLocation" },
    { P_LOCK_LOCATION, "lockLoc" },
    { P_RECTANGLE, "rectangle" },// gradients will be wrong if this isn't set first
    { P_RECTANGLE, "rect" },     // synonym
    { P_WIDTH, "width" },        // incase left,right are in the array
    { P_HEIGHT, "height" },      // incase top,bottom are in the array
    { P_STYLE, "style" },        // changes numerous properties including text alignment
    { P_TEXT_SIZE, "textSize" }, // changes textHeight
	// MERG-2013-06-24: [[ RevisedPropsProp ]] Ensure filename takes precedence over text.
    { P_FILE_NAME, "fileName" }, // setting image filenames to empty after setting the text will clear them
    // MERG-2013-07-20: [[ Bug 11060 ]] hilitedLines being lost.
    { P_LIST_BEHAVIOR, "listBehavior" }, // setting hilitedLines before listBehavior will lose the hilited lines
    { P_HTML_TEXT, "htmlText" }, // setting hilitedLines before htmlText will lose the hilited lines
    // MERG-2013-08-30: [[ RevisedPropsProp ]] Ensure button text has precedence over label and menuHistory
    { P_TEXT, "text" }, 
    { P_MENU_HISTORY, "menuHistory" },
    { P_FORE_PATTERN, "forePattern" },
    { P_FORE_PATTERN, "foregroundPattern" },
    { P_FORE_PATTERN, "textPattern" },
    { P_FORE_PATTERN, "thumbPattern" },
    { P_BACK_PATTERN, "backPattern" },
    { P_BACK_PATTERN, "backgroundPattern" },
    { P_BACK_PATTERN, "fillPat" },
    { P_HILITE_PATTERN, "hilitePattern" },
    { P_HILITE_PATTERN, "markerPattern" },
    { P_HILITE_PATTERN, "thirdPattern" },
    { P_BORDER_PATTERN, "borderPattern" },
    { P_TOP_PATTERN, "topPattern" },
    { P_BOTTOM_PATTERN, "bottomPattern" },
    { P_SHADOW_PATTERN, "shadowPattern" },
    { P_FOCUS_PATTERN, "focusPattern" },
    { P_PATTERNS, "patterns" },
 };

Exec_stat MCVariableArray::setprops(uint4 parid, MCObject *optr)
{
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	MCExecPoint ep(optr, NULL, NULL);
	MCRedrawLockScreen();
	MCerrorlock++;
    MCHashentry *e;
    uindex_t j;

    // MERG-2013-05-07: [[ RevisedPropsProp ]] pre-process to ensure properties
	//   that impact others are set first.
    uindex_t t_preprocess_size = sizeof(s_preprocess_props) / sizeof(s_preprocess_props[0]);
    for (j=0; j<t_preprocess_size; j++)
    {
		// MERG-2013-06-24: [[ RevisedPropsProp ]] Make sure we do a case-insensitive search
		//   for the property name.
        e = lookuphash(s_preprocess_props[j].tag,false,false);
        if (e)
        {
            e -> value . fetch(ep);
            optr->setprop(parid, (Properties)s_preprocess_props[j].prop, ep, False);
        }
    }
	
    uint4 i;
    for (i = 0 ; i < tablesize ; i++)
		if (table[i] != NULL)
		{
			e = table[i];
			while (e != NULL)
			{
                MCScriptPoint sp(e->string);
                Symbol_type type;
                const LT *te;
                if (sp.next(type) && sp.lookup(SP_FACTOR, te) == PS_NORMAL
                    && te->type == TT_PROPERTY && te->which != P_ID)
                {
                    // MERG-2013-05-07: [[ RevisedPropsProp ]] check if the key was
					//   in the pre-processed.
					// MW-2013-06-24: [[ RevisedPropsProp ]] set a boolean if the prop has already been
					//   set.
					bool t_been_preprocessed;
					t_been_preprocessed = false;
                    for (j=0; j<t_preprocess_size; j++)
                        if (te->which == s_preprocess_props[j].prop)
						{
                            t_been_preprocessed = true;
							break;
						}
						
					// MW-2013-06-24: [[ RevisedPropsProp ]] Only attempt to set the prop if it hasn't
					//   already been processed.
					if (!t_been_preprocessed)
					{
						e -> value . fetch(ep);
						if ((Properties)te->which > P_FIRST_ARRAY_PROP)
							optr->setarrayprop(parid, (Properties)te->which, ep, kMCEmptyName, False);
						else
							optr->setprop(parid, (Properties)te->which, ep, False);
					}
                }
            	e = e->next;
			}
		}
	
	MCerrorlock--;
	MCRedrawUnlockScreen();

	return ES_NORMAL;
}

// When this returns (if p_merge == false) nfilled with either be zero, or the array will
// have been initialized. Thus it needs to be cleaned up by the caller if IO_ERROR is
// returned.
IO_stat MCVariableArray::loadkeys(IO_handle stream, bool p_merge)
{
	IO_stat stat;
	uint4 t_new_nfilled;
	t_new_nfilled = 0;
	if ((stat = IO_read_uint4(&t_new_nfilled, stream)) != IO_NORMAL || t_new_nfilled == 0)
	{
		if (!p_merge)
			nfilled = t_new_nfilled;
		return stat;
	}
	bool decrypt = (t_new_nfilled & SAVE_ENCRYPTED) != 0;
	bool large = (t_new_nfilled & SAVE_LARGE) != 0;
	t_new_nfilled &= ~(SAVE_ENCRYPTED | SAVE_LARGE);

	bool t_encrypted = MCStackSecurityIsIOEncryptionEnabled();
	MCStackSecuritySetIOEncryptionEnabled(decrypt);
	if (!p_merge)
	{
		nfilled = t_new_nfilled;
		extents = NULL;
		dimensions = EXTENT_NONNUM;
		keysize = 0;
		tablesize = TABLE_SIZE;
		while (tablesize < nfilled)
			tablesize <<= 1;
		table = new MCHashentry *[tablesize];
		memset((char *)table, 0, tablesize * sizeof(MCHashentry *));
	}
	else if (t_new_nfilled > tablesize)
	{
		uint32_t t_new_tablesize;
		t_new_tablesize = tablesize;
		while(t_new_tablesize < t_new_nfilled)
			t_new_tablesize <<= 1;
		resizehash(t_new_tablesize);
	}

	uint32_t t_size;
	t_size = large ? 4 : 2;
	
	uint4 nprops = t_new_nfilled;
	while (stat == IO_NORMAL && nprops--)
	{
		MCHashentry *e = nil;
		char *t_string = nil;
		uint32_t t_length = 0;
		
		// IM-2013-04-04 loadkeys() would previously translate the key string,
		// so we pass p_translate = true.
		stat = IO_read_string(t_string, t_length, stream, 1, true, true);
		
		if (stat == IO_NORMAL)
		{
			keysize += t_length + 1;
			MCString t_key(t_string, t_length);
			/* UNCHECKED */ e = MCHashentry::Create(t_key, computehash(t_key));
			MCCStringFree(t_string);
			t_string = nil;

			uint4 index = e->hash & tablesize - 1;
			e->next = table[index];
			table[index] = e;

			stat = IO_read_string(t_string, t_length, stream, t_size, false, false);
		}
		
		if (stat == IO_NORMAL)
		{
			if (t_length != 0)
				e->value.assign_buffer(t_string, t_length);
			else
				e->value.assign_empty();
		}
	}
	
	MCStackSecuritySetIOEncryptionEnabled(t_encrypted);
	return stat;
}

IO_stat MCVariableArray::savekeys(IO_handle stream)
{
	if (table == NULL)
		return IO_write_uint4(0, stream);

	uint4 t_writable_nfilled;
	t_writable_nfilled = 0;

	uint4 i;
	Boolean large = False;
	for (i = 0 ; i < tablesize ; i++)
	{
		if (table[i] != NULL)
		{
			MCHashentry *e = table[i];
			while (e != NULL)
			{
				if (e -> value . is_string() && e -> value . get_string() . getlength() > MAXUINT2)
					large = True;
		
				if (!e -> value . is_array())
					t_writable_nfilled += 1;

				e = e->next;
			}
		}
	}



	if (MCStackSecurityIsIOEncrypted())
		t_writable_nfilled |= SAVE_ENCRYPTED;
	if (large)
		t_writable_nfilled |= SAVE_LARGE;
	IO_stat stat;
	if ((stat = IO_write_uint4(t_writable_nfilled, stream)) != IO_NORMAL)
		return stat;
	
	for (i = 0 ; i < tablesize ; i++)
		if (table[i] != NULL)
		{
			MCHashentry *e = table[i];
			while (e != NULL)
			{
				// Skip any array valued keys.
				if (e -> value . is_array())
				{
					e = e -> next;
					continue;
				}

				// IM-2013-04-04: [[ BZ 10811 ]] pre 6.0 versions of loadkeys() expect
				// a null-terminated string of non-zero length (including null),
				// but IO_write_string() writes a single zero byte for an empty string
				// so we need a special case here.
				if (e->string == nil || e->string[0] == '\0')
				{
					// write length + null
					if ((stat = IO_write_uint1(1, stream)) != IO_NORMAL)
						return stat;
					// write null
					if ((stat = IO_write_uint1(0, stream)) != IO_NORMAL)
						return stat;
				}
				else
				{
					if ((stat = IO_write_string(e->string, stream, 1)) != IO_NORMAL)
						return stat;
				}

				const char *t_value_str;
				uint32_t t_value_length;
				MCExecPoint ep;
				if (e -> value . ensure_string(ep))
				{
					t_value_str = e -> value . get_string() . getstring();
					t_value_length = e -> value . get_string() . getlength();
				}
				else
				{
					t_value_str = NULL;
					t_value_length = 0;
				}

				uint32_t t_size;
				if (large)
					t_size = 4;
				else
					t_size = 2;
				MCString t_string(t_value_str, t_value_length);
				if ((stat = IO_write_string(t_string, stream, t_size, false)) != IO_NORMAL)
					return stat;

				e = e->next;
			}
		}
	return IO_NORMAL;
}

bool MCVariableArray::isnested(void)
{
	MCHashentry *t_entry;
	t_entry = NULL;

	uint32_t t_index;
	t_index = 0;

	for(;;)
	{
		t_entry = getnextkey(t_index, t_entry);
		if (t_entry == NULL)
			break;

		if (t_entry -> value . is_array())
			return true;
	}

	return false;
}

uint4 MCVariableArray::measure(bool p_only_nested)
{
	uint4 t_size;
	t_size = 0;

	MCHashentry *t_entry;
	t_entry = NULL;
	
	uint32_t t_index;
	t_index = 0;

	for(;;)
	{
		t_entry = getnextkey(t_index, t_entry);
		if (t_entry == NULL)
			break;

		if (p_only_nested && !t_entry -> value . is_array())
			continue;

		t_size += t_entry -> Measure();
	}

	// We should return zero if the array has no nested elements
	if (!p_only_nested || t_size != 0)
		t_size += 5;

	return t_size;
}

IO_stat MCVariableArray::save(MCObjectOutputStream& p_stream, bool p_only_nested)
{
	MCHashentry *t_entry;
	t_entry = NULL;
	
	uint32_t t_index;
	t_index = 0;

	IO_stat t_stat;
	t_stat = p_stream . WriteU32(nfilled);

	while(t_stat == IO_NORMAL)
	{
		t_entry = getnextkey(t_index, t_entry);
		if (t_entry == NULL)
			break;

		if (p_only_nested && !t_entry -> value . is_array())
			continue;

		t_stat = t_entry -> Save(p_stream);
	}
	
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU8(0);

	return t_stat;
}

IO_stat MCVariableArray::load(MCObjectInputStream& p_stream, bool p_merge)
{
	IO_stat t_stat;

	uint32_t t_nfilled;
	t_nfilled = 0;
	t_stat = p_stream . ReadU32(t_nfilled);

	// Make sure we exit if there are no keys to read.
	if (t_nfilled == 0 && !p_merge)
	{
		nfilled = 0;
		return t_stat;
	}

	if (t_stat == IO_NORMAL)
	{
		uint32_t t_table_size;
		t_table_size = TABLE_SIZE;
		while(t_table_size < t_nfilled)
			t_table_size <<= 1;

		if (p_merge)
		{
			if (tablesize < t_table_size)
				resizehash(t_table_size);
		}
		else
			presethash(t_table_size);
	}

	while(t_stat == IO_NORMAL)
	{
		MCHashentry *t_entry;
		t_stat = MCHashentry::Load(p_stream, t_entry);
		if (t_entry == NULL)
			break;

		t_entry -> hash = computehash(t_entry -> string);

		uint32_t t_index;
		t_index = t_entry -> hash & (tablesize - 1);

		uint4 t_length;
		t_length = strlen(t_entry -> string);
		extentfromkey(t_entry -> string);
		keysize += t_length + 1;
		t_entry -> next = table[t_index];
		table[t_index] = t_entry;

		nfilled += 1;
	}

	return t_stat;
}

void MCVariableArray::listelements(MCHashentry **p_entries)
{
	uint4 t_count;
	t_count = 0;
	for(uint32_t i = 0 ; i < tablesize ; i++)
	if (table[i] != NULL)
	{
		MCHashentry *e = table[i];
		while (e != NULL)
		{
			p_entries[t_count] = e;
			e = e->next;
			t_count++;
			if (t_count == nfilled)
				break;
		}
	}
}

uint4 MCHashentry::Measure(void)
{
	uint4 t_size;
	t_size = 1 + 4 + strlen(string) + 1;

	switch(value . get_format())
	{
		case VF_UNDEFINED:
		break;

		case VF_STRING:
			t_size += 4 + value . get_string() . getlength();
		break;

		case VF_BOTH:
			// MW-2008-10-29: [[ Bug ]] If we are VF_BOTH and of string length 0 and numeric value 0.0
			//   we should be written out as UNDEFINED.
			if (value . get_string() . getlength() == 0 && value . get_real() == 0.0)
				;
			else
				t_size += 8;
		break;

		case VF_NUMBER:
			t_size += 8;
		break;

		case VF_ARRAY:
			t_size += value . get_array() -> measure(false);
		break;
	}

	return t_size;
}

IO_stat MCHashentry::Save(MCObjectOutputStream& p_stream)
{
	// MW-2008-08-26: [[ Bug 7005 ]] If the format is VF_BOTH we write it out
	//   as a number.
	// MW-2008-10-28: [[ Bug ]] If the format is VF_BOTH but svalue is empty
	//   then treat as VF_UNDEFINED.
	unsigned int t_type;
	if (value . get_format() == VF_BOTH)
	{
		if (value . get_string() . getlength() == 0 && value . get_real() == 0.0)
			t_type = (unsigned int)VF_UNDEFINED;
		else
			t_type = (unsigned int)VF_NUMBER;
	}
	else
		t_type = (unsigned int)value . get_format();
	
	IO_stat t_stat;
	t_stat = p_stream . WriteU8(t_type + 1);
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU32(Measure() - 1);
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteCString(string);
	if (t_stat == IO_NORMAL)
		switch((Value_format)t_type)
		{
		case VF_UNDEFINED:
		break;

		case VF_STRING:
			t_stat = p_stream . WriteU32(value . get_string() . getlength());
			if (t_stat == IO_NORMAL)
				t_stat = p_stream . Write(value . get_string() . getstring(), value . get_string() . getlength());
		break;
		
		case VF_NUMBER:
			t_stat = p_stream . WriteFloat64(value . get_real());
		break;

		case VF_ARRAY:
			t_stat = value . get_array() -> save(p_stream, false);
		break;
		}

	return t_stat;
}

IO_stat MCHashentry::Load(MCObjectInputStream& p_stream, MCHashentry*& r_entry)
{
	IO_stat t_stat;

	uint8_t t_type;
	t_stat = p_stream . ReadU8(t_type);
	if (t_stat == IO_NORMAL && t_type == 0)
	{
		r_entry = NULL;
		return IO_NORMAL;
	}

	uint32_t t_length;
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . ReadU32(t_length);

	char *t_key;
	t_key = NULL;
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . ReadCString(t_key);

	MCHashentry *t_entry;
	t_entry = NULL;
	if (t_stat == IO_NORMAL)
	{
		uint32_t t_key_length;
		t_key_length = t_key == NULL ? 0 : strlen(t_key);

		t_entry = MCHashentry::Create(t_key_length);
		memcpy(t_entry -> string, t_key, t_key_length);
		t_entry -> string[t_key_length] = '\0';

		switch((Value_format)(t_type - 1))
		{
		case VF_UNDEFINED:
		break;

		case VF_STRING:
		{
			uint32_t t_string_length;
			t_stat = p_stream . ReadU32(t_string_length);

			char *t_string;
			t_string = NULL;
			if (t_stat == IO_NORMAL)
			{
				t_string = new char[t_string_length];
				t_stat = p_stream . Read(t_string, t_string_length);
			}

			if (t_stat == IO_NORMAL)
				t_entry -> value . assign_buffer(t_string, t_string_length);
			else
				delete t_string;
		}
		break;

		// MW-2008-08-26: [[ Bug 7005 ]] Due to a bug in saving code in versions <= 3.0.0-rc-2,
		//   a 'BOTH' type will be output even though it should be NUMBER, we take account of
		//   this here.
		case VF_BOTH:
		case VF_NUMBER:
		{
			double t_number;
			t_stat = p_stream . ReadFloat64(t_number);
			if (t_stat == IO_NORMAL)
				t_entry -> value . assign_real(t_number);
		}
		break;

		case VF_ARRAY:
			t_stat = t_entry -> value . loadarray(p_stream, false);
		break;

		default:
			// MW-2008-08-26: [[ Bug 7005 ]] Make sure we skip the correct amount
			//   if the type is unknown to us.
			t_stat = p_stream . Read(NULL, t_length - t_key_length - 1 - 4);
		break;
		}
	}

	free(t_key);

	if (t_stat == IO_NORMAL)
		r_entry = t_entry;
	else
	{
		// MW-2008-08-26: [[ Bug 7005 ]] Make sure we return NULL in r_entry
		//   if an IO error occurs.
		r_entry = NULL;
		delete t_entry;
	}

	return t_stat;
}
