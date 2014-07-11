#ifndef HB_SK_H
#define HB_SK_H

#include "hb.h"
#include <SkTypeface.h>

HB_BEGIN_DECLS

struct hb_skia_face
{
    SkTypeface *typeface;
    uint16_t size;
};

hb_face_t *
hb_sk_face_create (hb_skia_face *typeface, hb_destroy_func_t destroy);

hb_font_t *
hb_sk_font_create (hb_skia_face *typeface, hb_destroy_func_t destroy);

HB_END_DECLS

#endif /* HB_SK_H */
