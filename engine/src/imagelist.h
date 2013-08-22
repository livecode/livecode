/********************************************************************/
/*    Copyright 2000 MetaCard Corporation.  All Rights Reserved.    */
/*    The software contained in this listing is proprietary to      */
/*    MetaCard Corporation, Boulder, Colorado and is covered by     */
/*    U.S. and other copyright protection.  Unauthorized copying    */
/*    adaptation, distribution, use or display is prohibited  and   */
/*    may be subject to civil and criminal penalties.  disclosure   */
/*    to others is prohibited.                                      */
/********************************************************************/
//
// List of patterns
//
#ifndef	IMAGELIST_H
#define	IMAGELIST_H

#include "dllst.h"
#include "graphics.h"

////////////////////////////////////////////////////////////////////////////////

// IM-2013-08-14: [[ ResIndependence ]] MCPattern struct which associates an image with a scale
struct MCPattern
{
	MCGImageRef image;
	MCGFloat scale;
	
	uint32_t references;
};

typedef MCPattern *MCPatternRef;

//////////

extern bool MCPatternCreate(MCGImageRef p_image, MCGFloat p_scale, MCPatternRef &r_pattern);
extern MCPatternRef MCPatternRetain(MCPatternRef p_pattern);
extern void MCPatternRelease(MCPatternRef p_pattern);

////////////////////////////////////////////////////////////////////////////////

class MCImageListNode : public MCDLlist
{
	MCImage *source;
	MCPatternRef image;
	uint32_t refcount;

public:
	MCImageListNode(MCImage *isource);
	~MCImageListNode();
	
	// MW-2009-02-02: [[ Improved image search ]]
	// Previously, the MCPixmapnodes were keyed on image id, however they are now
	// keyed on MCImage*'s since ids are not necessarily unique.
	bool allocimage(MCImage* source, MCPatternRef &r_image);
	bool freeimage(MCPatternRef image);
	bool unreferenced();
	
	MCImageListNode *next()
	{
		return (MCImageListNode *)MCDLlist::next();
	}
	MCImageListNode *prev()
	{
		return (MCImageListNode *)MCDLlist::prev();
	}
	void totop(MCImageListNode *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCImageListNode *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCImageListNode *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCImageListNode *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCImageListNode *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCImageListNode *remove(MCImageListNode *&list)
	{
		return (MCImageListNode *)MCDLlist::remove((MCDLlist *&)list);
	}
};

class MCImageList
{
	MCImageListNode *images;
public:
	MCImageList();
	~MCImageList();
	MCPatternRef allocpat(uint4 id, MCObject *optr);
	void freepat(MCPatternRef &pat);
};
#endif
