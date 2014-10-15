#ifndef HB_SK_H
#define HB_SK_H

#include "hb.h"
#include <SkTypeface.h>

HB_BEGIN_DECLS

struct hb_skia_face_t
{
    SkTypeface *typeface;
    uint16_t size;
};

struct MCHarfbuzzSkiaFace
{
    hb_face_t *face;
    hb_skia_face_t *skia_face;
};

hb_face_t *
hb_sk_face_create (hb_skia_face_t *typeface, hb_destroy_func_t destroy);

hb_font_t *
hb_sk_font_create (MCHarfbuzzSkiaFace *typeface, hb_destroy_func_t destroy);

void hb_sk_set_face(MCHarfbuzzSkiaFace *p_face, hb_skia_face_t *p_hb_sk_face);

HB_END_DECLS

#endif /* HB_SK_H */
