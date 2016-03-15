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

struct MCPattern;
// IM-2014-05-13: [[ HiResPatterns ]] Make MCPattern struct opaque:
// access to image and transform now through accessor functions
typedef MCPattern *MCPatternRef;

class MCImage;
class MCImageRep;

//////////

// IM-2014-05-13: [[ HiResPatterns ]] Create pattern from bitmap image at given scale
// IM-2014-05-21: [[ HiResPatterns ]] Add filter param to pattern creation functions
extern bool MCPatternCreate(const MCGRaster &p_raster, MCGFloat p_scale, MCGImageFilter p_filter, MCPatternRef &r_pattern);

// IM-2014-05-13: [[ HiResPatterns ]] Create pattern from source rep and transform
// IM-2014-05-21: [[ HiResPatterns ]] Add filter param to pattern creation functions
extern bool MCPatternCreate(MCImageRep *p_source, MCGAffineTransform p_transform, MCGImageFilter p_filter, MCPatternRef &r_pattern);

extern MCPatternRef MCPatternRetain(MCPatternRef p_pattern);
extern void MCPatternRelease(MCPatternRef p_pattern);

extern bool MCPatternIsOpaque(MCPatternRef p_pattern);

extern MCImageRep *MCPatternGetSource(MCPatternRef p_pattern);

extern MCGAffineTransform MCPatternGetTransform(MCPatternRef p_pattern);

// IM-2014-05-13: [[ HiResPatterns ]] Return the logical width + height of the pattern (after transform is applied)
extern bool MCPatternGetGeometry(MCPatternRef p_pattern, uint32_t &r_width, uint32_t &r_height);

// IM-2014-05-21: [[ HiResPatterns ]] Get the image filter to use with this pattern
extern bool MCPatternGetFilter(MCPatternRef p_pattern, MCGImageFilter &r_filter);

// IM-2014-05-13: [[ HiResPatterns ]] Obtain an image & transform most suitable for the current transform of the target context
extern bool MCPatternLockForContextTransform(MCPatternRef p_pattern, const MCGAffineTransform &p_transform, MCGImageRef &r_image, MCGAffineTransform &r_pattern_transform);
// IM-2014-05-13: [[ HiResPatterns ]] Release the image returned by MCPatternLock...
extern void MCPatternUnlock(MCPatternRef p_pattern, MCGImageRef p_locked_image);

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
