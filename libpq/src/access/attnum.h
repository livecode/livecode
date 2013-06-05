/*-------------------------------------------------------------------------
 *
 * attnum.h
 *	  POSTGRES attribute number definitions.
 *
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/access/attnum.h,v 1.21 2005/05/25 21:40:42 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef ATTNUM_H
#define ATTNUM_H


/*
 * user defined attribute numbers start at 1.	-ay 2/95
 */
typedef int16 AttrNumber;

#define InvalidAttrNumber		0

/* ----------------
 *		support macros
 * ----------------
 */
/*
 * AttributeNumberIsValid
 *		True iff the attribute number is valid.
 */
#define AttributeNumberIsValid(attributeNumber) \
	((bool) ((attributeNumber) != InvalidAttrNumber))

/*
 * AttrNumberIsForUserDefinedAttr
 *		True iff the attribute number corresponds to an user defined attribute.
 */
#define AttrNumberIsForUserDefinedAttr(attributeNumber) \
	((bool) ((attributeNumber) > 0))

/*
 * AttrNumberGetAttrOffset
 *		Returns the attribute offset for an attribute number.
 *
 * Note:
 *		Assumes the attribute number is for an user defined attribute.
 */
#define AttrNumberGetAttrOffset(attNum) \
( \
	AssertMacro(AttrNumberIsForUserDefinedAttr(attNum)), \
	((attNum) - 1) \
)

/*
 * AttributeOffsetGetAttributeNumber
 *		Returns the attribute number for an attribute offset.
 */
#define AttrOffsetGetAttrNumber(attributeOffset) \
	 ((AttrNumber) (1 + (attributeOffset)))

#endif   /* ATTNUM_H */
