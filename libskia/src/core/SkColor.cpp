/* libs/graphics/sgl/SkColor.cpp
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#include "SkColor.h"
#include "SkColorPriv.h"

SkPMColor SkPreMultiplyARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b) {
    if (a != 255) {
#if 0
        unsigned scale = SkAlpha255To256(a);
        r = SkAlphaMul(r, scale);
        g = SkAlphaMul(g, scale);
        b = SkAlphaMul(b, scale);
#else
        r = SkMulDiv255Round(r, a);
        g = SkMulDiv255Round(g, a);
        b = SkMulDiv255Round(b, a);
#endif
    }
    return SkPackARGB32(a, r, g, b);
}

SkPMColor SkPreMultiplyColor(SkColor c) {
    unsigned a = SkColorGetA(c);
    unsigned r = SkColorGetR(c);
    unsigned g = SkColorGetG(c);
    unsigned b = SkColorGetB(c);

    return SkPreMultiplyARGB(a, r, g, b);
}

//////////////////////////////////////////////////////////////////////////////////////////////

static inline SkScalar ByteToScalar(U8CPU x) {
    SkASSERT(x <= 255);
    return SkIntToScalar(x) / 255;
}

static inline SkScalar ByteDivToScalar(int numer, U8CPU denom) {
    // cast to keep the answer signed
    return SkIntToScalar(numer) / (int)denom;
}

void SkRGBToHSV(U8CPU r, U8CPU g, U8CPU b, SkScalar hsv[3]) {
    SkASSERT(hsv);

    unsigned min = SkMin32(r, SkMin32(g, b));
    unsigned max = SkMax32(r, SkMax32(g, b));
    unsigned delta = max - min;

    SkScalar v = ByteToScalar(max);
    SkASSERT(v >= 0 && v <= SK_Scalar1);

    if (0 == delta) { // we're a shade of gray
        hsv[0] = 0;
        hsv[1] = 0;
        hsv[2] = v;
        return;
    }

    SkScalar s = ByteDivToScalar(delta, max);
    SkASSERT(s >= 0 && s <= SK_Scalar1);

    SkScalar h;    
    if (r == max) {
        h = ByteDivToScalar(g - b, delta);
    } else if (g == max) {
        h = SkIntToScalar(2) + ByteDivToScalar(b - r, delta);
    } else { // b == max
        h = SkIntToScalar(4) + ByteDivToScalar(r - g, delta);
    }

    h *= 60;
    if (h < 0) {
        h += SkIntToScalar(360);
    }
    SkASSERT(h >= 0 && h < SkIntToScalar(360));

    hsv[0] = h;
    hsv[1] = s;
    hsv[2] = v;
}

static inline U8CPU UnitScalarToByte(SkScalar x) {
    if (x < 0) {
        return 0;
    }
    if (x >= SK_Scalar1) {
        return 255;
    }
    return SkScalarToFixed(x) >> 8;
}

SkColor SkHSVToColor(U8CPU a, const SkScalar hsv[3]) {
    SkASSERT(hsv);

    U8CPU s = UnitScalarToByte(hsv[1]);
    U8CPU v = UnitScalarToByte(hsv[2]);

    if (0 == s) { // shade of gray
        return SkColorSetARGB(a, v, v, v);
    }
    SkFixed hx = (hsv[0] < 0 || hsv[0] >= SkIntToScalar(360)) ? 0 : SkScalarToFixed(hsv[0]/60);
    SkFixed f = hx & 0xFFFF;
    
    unsigned v_scale = SkAlpha255To256(v);
    unsigned p = SkAlphaMul(255 - s, v_scale);
    unsigned q = SkAlphaMul(255 - (s * f >> 16), v_scale);
    unsigned t = SkAlphaMul(255 - (s * (SK_Fixed1 - f) >> 16), v_scale);
    
    unsigned r, g, b;

    SkASSERT((unsigned)(hx >> 16) < 6);
    switch (hx >> 16) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t;  g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return SkColorSetARGB(a, r, g, b);
}

