/* Pango
 * pango-ot.h:
 *
 * Copyright (C) 2000 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __PANGO_OT_H__
#define __PANGO_OT_H__

#include <pango/pangofc-font.h>
#include <pango/pango-glyph.h>
#include <pango/pango-font.h>

G_BEGIN_DECLS

#ifdef PANGO_ENABLE_ENGINE

typedef guint32  PangoOTTag;

typedef struct _PangoOTInfo    PangoOTInfo;
typedef struct _PangoOTBuffer  PangoOTBuffer;
typedef struct _PangoOTGlyph   PangoOTGlyph;
typedef struct _PangoOTRuleset PangoOTRuleset;

typedef enum 
{
  PANGO_OT_TABLE_GSUB,
  PANGO_OT_TABLE_GPOS
} PangoOTTableType;

/* Note that this must match OTLGlyphItem */
struct _PangoOTGlyph
{
  guint    glyph;
  guint    properties;
  guint    cluster;
  gushort  component;
  gushort  ligID;
  gushort  property_cache;    /* Internal */
};

PangoOTInfo *pango_ot_info_get (FT_Face face);

gboolean pango_ot_info_find_script   (PangoOTInfo      *info,
				      PangoOTTableType  table_type,
				      PangoOTTag        script_tag,
				      guint            *script_index);
gboolean pango_ot_info_find_language (PangoOTInfo      *info,
				      PangoOTTableType  table_type,
				      guint             script_index,
				      PangoOTTag        language_tag,
				      guint            *language_index,
				      guint            *required_feature_index);
gboolean pango_ot_info_find_feature  (PangoOTInfo      *info,
				      PangoOTTableType  table_type,
				      PangoOTTag        feature_tag,
				      guint             script_index,
				      guint             language_index,
				      guint            *feature_index);

PangoOTTag *pango_ot_info_list_scripts   (PangoOTInfo      *info,
					  PangoOTTableType  table_type);
PangoOTTag *pango_ot_info_list_languages (PangoOTInfo      *info,
					  PangoOTTableType  table_type,
					  guint             script_index,
					  PangoOTTag        language_tag);
PangoOTTag *pango_ot_info_list_features  (PangoOTInfo      *info,
					  PangoOTTableType  table_type,
					  PangoOTTag        tag,
					  guint             script_index,
					  guint             language_index);

PangoOTBuffer *pango_ot_buffer_new        (PangoFcFont       *font);
void           pango_ot_buffer_destroy    (PangoOTBuffer     *buffer);
void           pango_ot_buffer_clear      (PangoOTBuffer     *buffer);
void           pango_ot_buffer_set_rtl    (PangoOTBuffer     *buffer,
					   gboolean           rtl);
void           pango_ot_buffer_add_glyph  (PangoOTBuffer     *buffer,
					   guint              glyph,
					   guint              properties,
					   guint              cluster);
void           pango_ot_buffer_get_glyphs (PangoOTBuffer     *buffer,
					   PangoOTGlyph     **glyphs,
					   int               *n_glyphs);
void           pango_ot_buffer_output     (PangoOTBuffer     *buffer,
					   PangoGlyphString  *glyphs);

void           pango_ot_buffer_set_zero_width_marks (PangoOTBuffer     *buffer,
						     gboolean           zero_width_marks);

PangoOTRuleset *pango_ot_ruleset_new (PangoOTInfo       *info);

void            pango_ot_ruleset_add_feature (PangoOTRuleset   *ruleset,
					      PangoOTTableType  table_type,
					      guint             feature_index,
					      gulong            property_bit);
void            pango_ot_ruleset_substitute  (PangoOTRuleset   *ruleset,
					      PangoOTBuffer    *buffer);
void            pango_ot_ruleset_position    (PangoOTRuleset   *ruleset,
					      PangoOTBuffer    *buffer);

#endif /* PANGO_ENABLE_ENGINE */

G_END_DECLS

#endif /* __PANGO_OT_H__ */
