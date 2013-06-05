#include "SkLineClipper.h"

// return X coordinate of intersection with horizontal line at Y
static SkScalar sect_with_horizontal(const SkPoint src[2], SkScalar Y) {
    SkScalar dy = src[1].fY - src[0].fY;
    if (SkScalarNearlyZero(dy)) {
        return SkScalarAve(src[0].fX, src[1].fX);
    } else {
        return src[0].fX + SkScalarMulDiv(Y - src[0].fY, src[1].fX - src[0].fX,
                                          dy);
    }
}

// return Y coordinate of intersection with vertical line at X
static SkScalar sect_with_vertical(const SkPoint src[2], SkScalar X) {
    SkScalar dx = src[1].fX - src[0].fX;
    if (SkScalarNearlyZero(dx)) {
        return SkScalarAve(src[0].fY, src[1].fY);
    } else {
        return src[0].fY + SkScalarMulDiv(X - src[0].fX, src[1].fY - src[0].fY,
                                          dx);
    }
}

///////////////////////////////////////////////////////////////////////////////

static inline bool nestedLT(SkScalar a, SkScalar b, SkScalar dim) {
    return a <= b && (a < b || dim > 0);
}

// returns true if outer contains inner, even if inner is empty.
// note: outer.contains(inner) always returns false if inner is empty.
static inline bool containsNoEmptyCheck(const SkRect& outer,
                                        const SkRect& inner) {
    return  outer.fLeft <= inner.fLeft && outer.fTop <= inner.fTop &&
            outer.fRight >= inner.fRight && outer.fBottom >= inner.fBottom;
}

bool SkLineClipper::IntersectLine(const SkPoint src[2], const SkRect& clip,
                                  SkPoint dst[2]) {
    SkRect bounds;
    
    bounds.set(src, 2);
    if (containsNoEmptyCheck(clip, bounds)) {
        if (src != dst) {
            memcpy(dst, src, 2 * sizeof(SkPoint));
        }
        return true;
    }
    // check for no overlap, and only permit coincident edges if the line
    // and the edge are colinear
    if (nestedLT(bounds.fRight, clip.fLeft, bounds.width()) ||
        nestedLT(clip.fRight, bounds.fLeft, bounds.width()) ||
        nestedLT(bounds.fBottom, clip.fTop, bounds.height()) ||
        nestedLT(clip.fBottom, bounds.fTop, bounds.height())) {
        return false;
    }

    int index0, index1;
    
    if (src[0].fY < src[1].fY) {
        index0 = 0;
        index1 = 1;
    } else {
        index0 = 1;
        index1 = 0;
    }

    SkPoint tmp[2];
    memcpy(tmp, src, sizeof(tmp));

    // now compute Y intersections
    if (tmp[index0].fY < clip.fTop) {
        tmp[index0].set(sect_with_horizontal(src, clip.fTop), clip.fTop);
    }
    if (tmp[index1].fY > clip.fBottom) {
        tmp[index1].set(sect_with_horizontal(src, clip.fBottom), clip.fBottom);
    }
    
    if (tmp[0].fX < tmp[1].fX) {
        index0 = 0;
        index1 = 1;
    } else {
        index0 = 1;
        index1 = 0;
    }

    // check for quick-reject in X again, now that we may have been chopped
    if ((tmp[index1].fX <= clip.fLeft || tmp[index0].fX >= clip.fRight) &&
        tmp[index0].fX < tmp[index1].fX) {
        // only reject if we have a non-zero width
        return false;
    }

    if (tmp[index0].fX < clip.fLeft) {
        tmp[index0].set(clip.fLeft, sect_with_vertical(src, clip.fLeft));
    }
    if (tmp[index1].fX > clip.fRight) {
        tmp[index1].set(clip.fRight, sect_with_vertical(src, clip.fRight));
    }
#ifdef SK_DEBUG
    bounds.set(tmp, 2);
    SkASSERT(containsNoEmptyCheck(clip, bounds));
#endif
    memcpy(dst, tmp, sizeof(tmp));
    return true;
}

int SkLineClipper::ClipLine(const SkPoint pts[], const SkRect& clip,
                            SkPoint lines[]) {
    int index0, index1;

    if (pts[0].fY < pts[1].fY) {
        index0 = 0;
        index1 = 1;
    } else {
        index0 = 1;
        index1 = 0;
    }

    // Check if we're completely clipped out in Y (above or below

    if (pts[index1].fY <= clip.fTop) {  // we're above the clip
        return 0;
    }
    if (pts[index0].fY >= clip.fBottom) {  // we're below the clip
        return 0;
    }
    
    // Chop in Y to produce a single segment, stored in tmp[0..1]

    SkPoint tmp[2];
    memcpy(tmp, pts, sizeof(tmp));

    // now compute intersections
    if (pts[index0].fY < clip.fTop) {
        tmp[index0].set(sect_with_horizontal(pts, clip.fTop), clip.fTop);
    }
    if (tmp[index1].fY > clip.fBottom) {
        tmp[index1].set(sect_with_horizontal(pts, clip.fBottom), clip.fBottom);
    }

    // Chop it into 1..3 segments that are wholly within the clip in X.

    // temp storage for up to 3 segments
    SkPoint resultStorage[kMaxPoints];
    SkPoint* result;    // points to our results, either tmp or resultStorage
    int lineCount = 1;
    bool reverse;

    if (pts[0].fX < pts[1].fX) {
        index0 = 0;
        index1 = 1;
        reverse = false;
    } else {
        index0 = 1;
        index1 = 0;
        reverse = true;
    }

    if (tmp[index1].fX <= clip.fLeft) {  // wholly to the left
        tmp[0].fX = tmp[1].fX = clip.fLeft;
        result = tmp;
        reverse = false;
    } else if (tmp[index0].fX >= clip.fRight) {    // wholly to the right
        tmp[0].fX = tmp[1].fX = clip.fRight;
        result = tmp;
        reverse = false;
    } else {
        result = resultStorage;
        SkPoint* r = result;
        
        if (tmp[index0].fX < clip.fLeft) {
            r->set(clip.fLeft, tmp[index0].fY);
            r += 1;
            r->set(clip.fLeft, sect_with_vertical(pts, clip.fLeft));
        } else {
            *r = tmp[index0];
        }
        r += 1;

        if (tmp[index1].fX > clip.fRight) {
            r->set(clip.fRight, sect_with_vertical(pts, clip.fRight));
            r += 1;
            r->set(clip.fRight, tmp[index1].fY);
        } else {
            *r = tmp[index1];
        }

        lineCount = r - result;
    }

    // Now copy the results into the caller's lines[] parameter
    if (reverse) {
        // copy the pts in reverse order to maintain winding order
        for (int i = 0; i <= lineCount; i++) {
            lines[lineCount - i] = result[i];
        }
    } else {
        memcpy(lines, result, (lineCount + 1) * sizeof(SkPoint));
    }
    return lineCount;
}

