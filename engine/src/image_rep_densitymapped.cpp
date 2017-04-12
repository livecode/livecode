/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "osspec.h"

#include "image.h"
#include "image_rep.h"

////////////////////////////////////////////////////////////////////////////////

MCDensityMappedImageRep::MCDensityMappedImageRep(MCStringRef p_filename)
{
	m_sources = nil;
	m_source_densities = nil;
	m_source_count = 0;
	
	m_locked = false;
	
	m_filename = MCValueRetain(p_filename);
}

MCDensityMappedImageRep::~MCDensityMappedImageRep()
{
	for (uindex_t i = 0; i < m_source_count; i++)
		m_sources[i]->Release();
	
	MCMemoryDeleteArray(m_sources);
	MCMemoryDeleteArray(m_source_densities);
	
	MCValueRelease(m_filename);
}

////////////////////////////////////////////////////////////////////////////////

// These methods attempt to find the best match to either the specified density, or to the last requested density.
// The functionality is then delegated to the matching rep if found.

uindex_t MCDensityMappedImageRep::GetFrameCount()
{
	uindex_t t_match;
	if (!GetBestMatch(1.0, t_match))
		return 0;
	
	return m_sources[t_match]->GetFrameCount();
}

bool MCDensityMappedImageRep::LockImageFrame(uindex_t p_index, MCGFloat p_density, MCGImageFrame& r_frame)
{
    uindex_t t_match;
	if (!GetBestMatch(p_density, t_match))
    	return false;
	
	uint32_t t_width, t_height;
	if (!GetGeometry(t_width, t_height))
		return false;
	
	if (m_sources[t_match]->LockImageFrame(p_index, p_density, r_frame))
    {
		// IM-2014-08-07: [[ Bug 13021 ]] Calculate image x/y scale from logical & actual size
		r_frame.x_scale = (MCGFloat)MCGImageGetWidth(r_frame.image) / t_width;
		r_frame.y_scale = (MCGFloat)MCGImageGetHeight(r_frame.image) / t_height;
        return true;
    }
    
    return false;
}

void MCDensityMappedImageRep::UnlockImageFrame(uindex_t p_index, MCGImageFrame& p_frame)
{
    MCGImageRelease(p_frame.image);
}

bool MCDensityMappedImageRep::LockBitmap(uindex_t p_index, MCGFloat p_density, MCImageBitmap *&r_bitmap)
{
	uindex_t t_match;
	if (!GetBestMatch(p_density, t_match))
		return false;
	
	uint32_t t_width, t_height;
	if (!GetGeometry(t_width, t_height))
		return false;
	
	m_locked = m_sources[t_match]->LockBitmap(p_index, p_density, r_bitmap);
	m_locked_source = t_match;
	
	return m_locked;
}

void MCDensityMappedImageRep::UnlockBitmap(uindex_t p_index, MCImageBitmap *p_bitmap)
{
	if (!m_locked)
		return;
	
	m_sources[m_locked_source]->UnlockBitmap(p_index, p_bitmap);
	
	m_locked = false;
}

bool MCDensityMappedImageRep::GetGeometry(uindex_t &r_width, uindex_t &r_height)
{
	uindex_t t_match;
	// IM-2014-08-01: [[ Bug 13021 ]] Make the 1.0 density source (or nearest match) the basis for the image geometry
	if (!GetBestMatch(1.0, t_match))
		return false;
	
	if (!m_sources[t_match]->GetGeometry(r_width, r_height))
		return false;

	r_width /= m_source_densities[t_match];
	r_height /= m_source_densities[t_match];
	
	return true;
}

bool MCDensityMappedImageRep::GetMetadata(MCImageMetadata &r_metadata)
{
	uindex_t t_match;
	// IM-2014-08-01: [[ Bug 13021 ]] Make the 1.0 density source (or nearest match) the basis for the image geometry
	if (!GetBestMatch(1.0, t_match))
		return false;
	
	if (!m_sources[t_match]->GetMetadata(r_metadata))
		return false;
    
	return true;
}

bool MCDensityMappedImageRep::GetFrameDuration(uindex_t p_index, uint32_t &r_duration)
{
	uindex_t t_match;
	if (!GetBestMatch(1.0, t_match))
		return false;
	
	return m_sources[t_match]->GetFrameDuration(p_index, r_duration);
}

uint32_t MCDensityMappedImageRep::GetDataCompression()
{
	uindex_t t_match;
	if (!GetBestMatch(1.0, t_match))
		return F_RLE;
	
	return m_sources[t_match]->GetDataCompression();
}

////////////////////////////////////////////////////////////////////////////////

bool MCDensityMappedImageRep::AddImageSourceWithDensity(MCReferencedImageRep *p_source, MCGFloat p_density)
{
	if (p_source == nil)
		return false;
	
	if (!MCMemoryResizeArray(m_source_count + 1, m_sources, m_source_count))
		return false;
	
	if (!MCMemoryResizeArray(m_source_count, m_source_densities, m_source_count))
		return false;
	
	p_source->Retain();
	m_sources[m_source_count - 1] = p_source;
	m_source_densities[m_source_count - 1] = p_density;
	
	return true;
}

bool MCDensityMappedImageRep::GetBestMatch(MCGFloat p_density, uindex_t &r_match)
{
	// the best match is defined as the smallest value equal to or greater than the requested value, or the largest value if none are greater
	
	if (m_source_count == 0)
		return false;
	
	MCGFloat t_density;
	t_density = m_source_densities[0];
	
	uindex_t t_match;
	t_match = 0;
	
	for (uindex_t i = 1; i < m_source_count; i++)
	{
		// if current density is lower than requested density then take any higher-density image
		// else if current density is higher than requested density then take any lower-density image not lower than the requested density
		if ((t_density < p_density && m_source_densities[i] > t_density) ||
			(t_density > p_density && m_source_densities[i] < t_density && m_source_densities[i] >= p_density))
		{
			t_density = m_source_densities[i];
			t_match = i;
		}
	}	

	r_match = t_match;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// IM-2013-07-30: [[ ResIndependence ]] Density-mapped Image Support
// IM-2013-10-30: [[ FullscreenMode ]] Moved from ifile.cpp

typedef struct _MCImageScaleLabels_t
{
	MCGFloat scale;
	const char **labels;
} MCImageScaleLabel;

static const char *s_ultra_low_labels[] = {
	"@ultra-low",
	nil
};
static const char *s_extra_low_labels[] = {
	"@extra-low",
	nil
};
static const char *s_low_labels[] = {
	"@low",
	nil
};
static const char *s_medium_labels[] = {
	"@medium",
	// unlabeled default scale
	"",
	nil
};
static const char *s_high_labels[] = {
	"@high",
	nil
};
static const char *s_extra_high_labels[] = {
	"@extra-high",
	// MM-2012-10-03: [[ ResIndependence ]] Added missing @2x
	"@2x",
	nil
};
static const char *s_ultra_high_labels[] = {
	"@ultra-high",
	nil
};

static MCImageScaleLabel s_image_scale_labels[] = {
	{0.25, s_ultra_low_labels},
	{0.5, s_extra_low_labels},
	{0.75, s_low_labels},
	{1.0, s_medium_labels},
	{1.5, s_high_labels},
	{2.0, s_extra_high_labels},
	{4.0, s_ultra_high_labels},
	
	{0.0, nil}
};

//////////

bool MCImageGetScaleForLabel(MCStringRef p_label, MCGFloat &r_scale)
{
	MCImageScaleLabel *t_scale_label;
	t_scale_label = s_image_scale_labels;
	
	while (t_scale_label->labels != nil)
	{
		const char **t_label = t_scale_label->labels;
		while (*t_label != nil)
		{
			if (MCStringIsEqualToCString(p_label, *t_label, kMCStringOptionCompareExact))
			{
				r_scale = t_scale_label->scale;
				return true;
			}
			t_label++;
		}
		t_scale_label++;
	}
	
	return false;
}

bool MCImageGetLabelsForScale(MCGFloat p_scale, const char **&r_labels)
{
	MCImageScaleLabel *t_scale_label;
	t_scale_label = s_image_scale_labels;
	
	while (t_scale_label->labels != nil)
	{
		if (p_scale == t_scale_label->scale)
		{
			r_labels = t_scale_label->labels;
			return true;
		}
		t_scale_label++;
	}
	
	return false;
}

//////////

bool MCImageSplitScaledFilename(MCStringRef p_filename, MCStringRef &r_base, MCStringRef &r_extension, bool &r_has_scale, MCGFloat &r_scale)
{
	if (MCStringIsEmpty(p_filename))
		return false;
	
	bool t_success;
	t_success = true;
	
	MCGFloat t_scale;
	bool t_has_scale = false;
	
	uint32_t t_length;
	t_length = MCStringGetLength(p_filename);
	
	uint32_t t_index, t_name_start, t_label_start, t_label_search_start, t_ext_start;
	
    if (MCStringLastIndexOfChar(p_filename, '/', UINDEX_MAX, kMCStringOptionCompareExact, t_index))
		t_name_start = t_index + 1;
	else
		t_name_start = 0;
	
    if (MCStringFirstIndexOfChar(p_filename, '.', t_name_start, kMCStringOptionCompareExact, t_index))
		t_ext_start = t_index;
	else
		t_ext_start = t_length;
	
	// find first '@' char before the extension part
	t_label_start = t_label_search_start = t_name_start;
    while (MCStringFirstIndexOfChar(p_filename, '@', t_label_search_start, kMCStringOptionCompareExact, t_index))
	{
		if (t_index > t_ext_start)
			break;
		
		t_label_start = t_index;
		t_label_search_start = t_label_start + 1;
	}
	
	// check label begins with '@'
    if (MCStringGetCharAtIndex(p_filename, t_label_start) != '@')
	{
		// no scale label
		t_label_start = t_ext_start;
	}
	else
	{
		MCAutoStringRef t_label;
        /* UNCHECKED */ MCStringCopySubstring(p_filename, MCRangeMakeMinMax(t_label_start, t_ext_start), &t_label);
        
        t_has_scale = MCImageGetScaleForLabel(*t_label, t_scale);
		
		if (!t_has_scale)
		{
			// @... is not a recognised scale
			t_label_start = t_ext_start;
		}
	}
	
	MCAutoStringRef t_base, t_extension;
    t_success = MCStringCopySubstring(p_filename, MCRangeMake(0, t_label_start), &t_base)
    && MCStringCopySubstring(p_filename, MCRangeMakeMinMax(t_ext_start, t_length), &t_extension);
	
	if (t_success)
	{
		r_base = MCValueRetain(*t_base);
		r_extension = MCValueRetain(*t_extension);
		r_has_scale = t_has_scale;
		r_scale = t_has_scale ? t_scale : 1.0;
	}

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageRepGetReferencedWithScale(MCStringRef p_base, MCStringRef p_extension, MCGFloat p_scale, MCImageRep *&r_rep)
{
	bool t_success;
	t_success = true;
	
	MCImageRep *t_rep;
	t_rep = nil;
	
	const char **t_labels;
	t_labels = nil;
	
	if (t_success)
		t_success = MCImageGetLabelsForScale(p_scale, t_labels);
	
    MCAutoStringRef t_default_path;
	// construct default path as base path with first tag for the given scale
	if (t_success)
		t_success = MCStringFormat(&t_default_path, "%@%s%@", p_base, t_labels[0], p_extension);
	
	if (t_success)
	{
		MCCachedImageRep *t_cached_rep;
		t_cached_rep = nil;
		
		if (MCCachedImageRep::FindWithKey(*t_default_path, t_cached_rep))
			t_rep = t_cached_rep->Retain();
		// not in cache, so see if default path exists.
		else if (MCS_exists(*t_default_path, True))
			t_success = MCImageRepCreateReferencedWithSearchKey(*t_default_path, *t_default_path, t_rep);
		// else loop through remaining labels and check for matching files
		else
		{
			for (uint32_t i = 1; t_success && t_rep == nil && t_labels[i] != nil; i++)
			{
				MCAutoStringRef t_scaled_path;
				t_success = MCStringFormat(&t_scaled_path, "%@%s%@", p_base, t_labels[i], p_extension);
				
				if (t_success && MCS_exists(*t_scaled_path, True))
					t_success = MCImageRepCreateReferencedWithSearchKey(*t_scaled_path, *t_default_path, t_rep);
			}
		}
	}
	
	if (t_success)
		r_rep = t_rep;
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

struct MCImageScaledRep
{
	MCImageRep *rep;
	MCGFloat scale;
};

//////////

void MCImageFreeScaledRepList(MCImageScaledRep *p_list, uint32_t p_count)
{
	if (p_list == nil)
		return;
	
	for (uint32_t i = 0; i < p_count; i++)
		p_list[i].rep->Release();
	
	MCMemoryDeleteArray(p_list);
}

// IM-2013-07-30: [[ ResIndependence ]] support for retrieving the density-mapped file list
// IM-2013-10-30: [[ FullscreenMode ]] Modified to return a list of density-mapped image reps
bool MCImageGetScaledFiles(MCStringRef p_base, MCStringRef p_extension, MCImageScaledRep *&r_list, uint32_t &r_count)
{
	bool t_success;
	t_success = true;
	
	MCImageScaledRep *t_list;
	t_list = nil;
	
	uint32_t t_count;
	t_count = 0;
	
	if (t_success)
	{
		MCImageScaleLabel *t_scale_labels;
		t_scale_labels = s_image_scale_labels;
		
		while (t_success && t_scale_labels->labels != nil)
		{
			MCImageRep *t_rep;
			t_rep = nil;
			
			t_success = MCImageRepGetReferencedWithScale(p_base, p_extension, t_scale_labels->scale, t_rep);

			if (t_success && t_rep != nil)
			{
				t_success = MCMemoryResizeArray(t_count + 1, t_list, t_count);
				
				if (t_success)
				{
					t_list[t_count - 1].rep = t_rep->Retain();
					t_list[t_count - 1].scale = t_scale_labels->scale;
				}
				
				t_rep->Release();
			}
			
			t_scale_labels++;
		}
	}
	
	// Fail if we don't find any matching files.
	if (t_success)
		t_success = t_count > 0;
	
	if (t_success)
	{
		r_list = t_list;
		r_count = t_count;
	}
	else
		MCImageFreeScaledRepList(t_list, t_count);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageRepCreateDensityMapped(MCStringRef p_filename, const MCImageScaledRep *p_list, uindex_t p_count, MCImageRep *&r_rep)
{
	bool t_success;
	t_success = true;
	
	MCDensityMappedImageRep *t_rep;
	t_rep = nil;
	
	if (t_success)
		t_success = nil != (t_rep = new (nothrow) MCDensityMappedImageRep(p_filename));
	
	for (uindex_t i = 0; t_success && i < p_count; i++)
		t_success = t_rep->AddImageSourceWithDensity(static_cast<MCReferencedImageRep*>(p_list[i].rep), p_list[i].scale);
	
	if (t_success)
	{
		MCCachedImageRep::AddRep(t_rep);
		r_rep = t_rep->Retain();
	}
	else
		delete t_rep;
	
	return t_success;
}

bool MCImageRepGetDensityMapped(MCStringRef p_filename, MCImageRep *&r_rep)
{
	// try to resolve files in the following way:
	// if it has a density tag then
	//    try to get referenced rep for the exact filename
	//    if that fails, try to get referenced file with matching base name & scale
	// else
	//    try to get density mapped set for the base filename
	
	bool t_success = true;
	
	MCImageRep *t_rep = nil;

    MCAutoStringRef t_base, t_extension;
	
	MCGFloat t_density;
	
	bool t_has_tag;
	
    t_success = MCImageSplitScaledFilename(p_filename, &t_base, &t_extension, t_has_tag, t_density);
	
	if (!t_success)
		return false;
	
	// check filename for density tag
	if (t_has_tag)
        t_success = MCImageRepGetReferencedWithScale(*t_base, *t_extension, t_density, t_rep);
	else
	{
		MCCachedImageRep *t_cached_rep;
		t_cached_rep = nil;
		
		if (MCCachedImageRep::FindWithKey(p_filename, t_cached_rep))
		{
			t_rep = t_cached_rep->Retain();
		}
		else
		{
			MCImageScaledRep *t_list;
			t_list = nil;
			
			uint32_t t_count;
			t_count = 0;
			
            t_success = MCImageGetScaledFiles(*t_base, *t_extension, t_list, t_count);
			if (t_success && t_count > 0)
				t_success = MCImageRepCreateDensityMapped(p_filename, t_list, t_count, t_rep);
			
			MCImageFreeScaledRepList(t_list, t_count);
		}
	}
	    
	if (t_success)
	{
		r_rep = t_rep;
	}
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
