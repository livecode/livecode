/*
 * Copyright (C) 2006-2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SkPathMeasure.h"
#include "SkGeometry.h"
#include "SkPath.h"
#include "SkTSearch.h"

// these must be 0,1,2 since they are in our 2-bit field
enum {
    kLine_SegType,
    kCloseLine_SegType,
    kQuad_SegType,
    kCubic_SegType
};

#define kMaxTValue  32767

static inline SkScalar tValue2Scalar(int t) {
    SkASSERT((unsigned)t <= kMaxTValue);

#ifdef SK_SCALAR_IS_FLOAT
    return t * 3.05185e-5f; // t / 32767
#else
    return (t + (t >> 14)) << 1;
#endif
}

SkScalar SkPathMeasure::Segment::getScalarT() const {
    return tValue2Scalar(fTValue);
}

const SkPathMeasure::Segment* SkPathMeasure::NextSegment(const Segment* seg) {
    unsigned ptIndex = seg->fPtIndex;

    do {
        ++seg;
    } while (seg->fPtIndex == ptIndex);
    return seg;
}

///////////////////////////////////////////////////////////////////////////////

static inline int tspan_big_enough(int tspan) {
    SkASSERT((unsigned)tspan <= kMaxTValue);
    return tspan >> 10;
}

#if 0
static inline bool tangents_too_curvy(const SkVector& tan0, SkVector& tan1) {
    static const SkScalar kFlatEnoughTangentDotProd = SK_Scalar1 * 99 / 100;

    SkASSERT(kFlatEnoughTangentDotProd > 0 &&
             kFlatEnoughTangentDotProd < SK_Scalar1);

    return SkPoint::DotProduct(tan0, tan1) < kFlatEnoughTangentDotProd;
}
#endif

// can't use tangents, since we need [0..1..................2] to be seen
// as definitely not a line (it is when drawn, but not parametrically)
// so we compare midpoints
#define CHEAP_DIST_LIMIT    (SK_Scalar1/2)  // just made this value up

static bool quad_too_curvy(const SkPoint pts[3]) {
    // diff = (a/4 + b/2 + c/4) - (a/2 + c/2)
    // diff = -a/4 + b/2 - c/4
    SkScalar dx = SkScalarHalf(pts[1].fX) -
                        SkScalarHalf(SkScalarHalf(pts[0].fX + pts[2].fX));
    SkScalar dy = SkScalarHalf(pts[1].fY) -
                        SkScalarHalf(SkScalarHalf(pts[0].fY + pts[2].fY));

    SkScalar dist = SkMaxScalar(SkScalarAbs(dx), SkScalarAbs(dy));
    return dist > CHEAP_DIST_LIMIT;
}

static bool cheap_dist_exceeds_limit(const SkPoint& pt,
                                     SkScalar x, SkScalar y) {
    SkScalar dist = SkMaxScalar(SkScalarAbs(x - pt.fX), SkScalarAbs(y - pt.fY));
    // just made up the 1/2
    return dist > CHEAP_DIST_LIMIT;
}

static bool cubic_too_curvy(const SkPoint pts[4]) {
    return  cheap_dist_exceeds_limit(pts[1],
                         SkScalarInterp(pts[0].fX, pts[3].fX, SK_Scalar1/3),
                         SkScalarInterp(pts[0].fY, pts[3].fY, SK_Scalar1/3))
                         ||
            cheap_dist_exceeds_limit(pts[2],
                         SkScalarInterp(pts[0].fX, pts[3].fX, SK_Scalar1*2/3),
                         SkScalarInterp(pts[0].fY, pts[3].fY, SK_Scalar1*2/3));
}

SkScalar SkPathMeasure::compute_quad_segs(const SkPoint pts[3],
                          SkScalar distance, int mint, int maxt, int ptIndex) {
    if (tspan_big_enough(maxt - mint) && quad_too_curvy(pts)) {
        SkPoint tmp[5];
        int     halft = (mint + maxt) >> 1;

        SkChopQuadAtHalf(pts, tmp);
        distance = this->compute_quad_segs(tmp, distance, mint, halft, ptIndex);
        distance = this->compute_quad_segs(&tmp[2], distance, halft, maxt, ptIndex);
    } else {
        SkScalar d = SkPoint::Distance(pts[0], pts[2]);
        SkASSERT(d >= 0);
        if (!SkScalarNearlyZero(d)) {
            distance += d;
            Segment* seg = fSegments.append();
            seg->fDistance = distance;
            seg->fPtIndex = ptIndex;
            seg->fType = kQuad_SegType;
            seg->fTValue = maxt;
        }
    }
    return distance;
}

SkScalar SkPathMeasure::compute_cubic_segs(const SkPoint pts[4],
                           SkScalar distance, int mint, int maxt, int ptIndex) {
    if (tspan_big_enough(maxt - mint) && cubic_too_curvy(pts)) {
        SkPoint tmp[7];
        int     halft = (mint + maxt) >> 1;

        SkChopCubicAtHalf(pts, tmp);
        distance = this->compute_cubic_segs(tmp, distance, mint, halft, ptIndex);
        distance = this->compute_cubic_segs(&tmp[3], distance, halft, maxt, ptIndex);
    } else {
        SkScalar d = SkPoint::Distance(pts[0], pts[3]);
        SkASSERT(d >= 0);
        if (!SkScalarNearlyZero(d)) {
            distance += d;
            Segment* seg = fSegments.append();
            seg->fDistance = distance;
            seg->fPtIndex = ptIndex;
            seg->fType = kCubic_SegType;
            seg->fTValue = maxt;
        }
    }
    return distance;
}

void SkPathMeasure::buildSegments() {
    SkPoint         pts[4];
    int             ptIndex = fFirstPtIndex;
    SkScalar        d, distance = 0;
    bool            isClosed = fForceClosed;
    bool            firstMoveTo = ptIndex < 0;
    Segment*        seg;

    fSegments.reset();
    for (;;) {
        switch (fIter.next(pts)) {
            case SkPath::kMove_Verb:
                if (!firstMoveTo) {
                    goto DONE;
                }
            ptIndex += 1;
            firstMoveTo = false;
            break;

            case SkPath::kLine_Verb:
                d = SkPoint::Distance(pts[0], pts[1]);
                SkASSERT(d >= 0);
                if (!SkScalarNearlyZero(d)) {
                    distance += d;
                    seg = fSegments.append();
                    seg->fDistance = distance;
                    seg->fPtIndex = ptIndex;
                    seg->fType = fIter.isCloseLine() ?
                                    kCloseLine_SegType : kLine_SegType;
                    seg->fTValue = kMaxTValue;
                }
                ptIndex += !fIter.isCloseLine();
                break;

            case SkPath::kQuad_Verb:
                distance = this->compute_quad_segs(pts, distance, 0,
                                                   kMaxTValue, ptIndex);
                ptIndex += 2;
                break;

            case SkPath::kCubic_Verb:
                distance = this->compute_cubic_segs(pts, distance, 0,
                                                    kMaxTValue, ptIndex);
                ptIndex += 3;
                break;

            case SkPath::kClose_Verb:
                isClosed = true;
                break;
                
            case SkPath::kDone_Verb:
                goto DONE;
        }
    }
DONE:
    fLength = distance;
    fIsClosed = isClosed;
    fFirstPtIndex = ptIndex + 1;

#ifdef SK_DEBUG
    {
        const Segment* seg = fSegments.begin();
        const Segment* stop = fSegments.end();
        unsigned        ptIndex = 0;
        SkScalar        distance = 0;

        while (seg < stop) {
            SkASSERT(seg->fDistance > distance);
            SkASSERT(seg->fPtIndex >= ptIndex);
            SkASSERT(seg->fTValue > 0);

            const Segment* s = seg;
            while (s < stop - 1 && s[0].fPtIndex == s[1].fPtIndex) {
                SkASSERT(s[0].fType == s[1].fType);
                SkASSERT(s[0].fTValue < s[1].fTValue);
                s += 1;
            }

            distance = seg->fDistance;
            ptIndex = seg->fPtIndex;
            seg += 1;
        }
    //  SkDebugf("\n");
    }
#endif
}

// marked as a friend in SkPath.h
const SkPoint* sk_get_path_points(const SkPath& path, int index) {
    return &path.fPts[index];
}

static void compute_pos_tan(const SkPath& path, int firstPtIndex, int ptIndex,
                    int segType, SkScalar t, SkPoint* pos, SkVector* tangent) {
    const SkPoint*  pts = sk_get_path_points(path, ptIndex);

    switch (segType) {
        case kLine_SegType:
        case kCloseLine_SegType: {
            const SkPoint* endp = (segType == kLine_SegType) ?
                                    &pts[1] :
                                    sk_get_path_points(path, firstPtIndex);

            if (pos) {
                pos->set(SkScalarInterp(pts[0].fX, endp->fX, t),
                        SkScalarInterp(pts[0].fY, endp->fY, t));
            }
            if (tangent) {
                tangent->setNormalize(endp->fX - pts[0].fX, endp->fY - pts[0].fY);
            }
            break;
        }
        case kQuad_SegType:
            SkEvalQuadAt(pts, t, pos, tangent);
            if (tangent) {
                tangent->normalize();
            }
            break;
        case kCubic_SegType:
            SkEvalCubicAt(pts, t, pos, tangent, NULL);
            if (tangent) {
                tangent->normalize();
            }
            break;
        default:
            SkASSERT(!"unknown segType");
    }
}

static void seg_to(const SkPath& src, int firstPtIndex, int ptIndex,
                   int segType, SkScalar startT, SkScalar stopT, SkPath* dst) {
    SkASSERT(startT >= 0 && startT <= SK_Scalar1);
    SkASSERT(stopT >= 0 && stopT <= SK_Scalar1);
    SkASSERT(startT <= stopT);

    if (SkScalarNearlyZero(stopT - startT)) {
        return;
    }

    const SkPoint*  pts = sk_get_path_points(src, ptIndex);
    SkPoint         tmp0[7], tmp1[7];

    switch (segType) {
        case kLine_SegType:
        case kCloseLine_SegType: {
            const SkPoint* endp = (segType == kLine_SegType) ?
                                    &pts[1] :
                                    sk_get_path_points(src, firstPtIndex);

            if (stopT == kMaxTValue) {
                dst->lineTo(*endp);
            } else {
                dst->lineTo(SkScalarInterp(pts[0].fX, endp->fX, stopT),
                            SkScalarInterp(pts[0].fY, endp->fY, stopT));
            }
            break;
        }
        case kQuad_SegType:
            if (startT == 0) {
                if (stopT == SK_Scalar1) {
                    dst->quadTo(pts[1], pts[2]);
                } else {
                    SkChopQuadAt(pts, tmp0, stopT);
                    dst->quadTo(tmp0[1], tmp0[2]);
                }
            } else {
                SkChopQuadAt(pts, tmp0, startT);
                if (stopT == SK_Scalar1) {
                    dst->quadTo(tmp0[3], tmp0[4]);
                } else {
                    SkChopQuadAt(&tmp0[2], tmp1, SkScalarDiv(stopT - startT,
                                                         SK_Scalar1 - startT));
                    dst->quadTo(tmp1[1], tmp1[2]);
                }
            }
            break;
        case kCubic_SegType:
            if (startT == 0) {
                if (stopT == SK_Scalar1) {
                    dst->cubicTo(pts[1], pts[2], pts[3]);
                } else {
                    SkChopCubicAt(pts, tmp0, stopT);
                    dst->cubicTo(tmp0[1], tmp0[2], tmp0[3]);
                }
            } else {
                SkChopCubicAt(pts, tmp0, startT);
                if (stopT == SK_Scalar1) {
                    dst->cubicTo(tmp0[4], tmp0[5], tmp0[6]);
                } else {
                    SkChopCubicAt(&tmp0[3], tmp1, SkScalarDiv(stopT - startT,
                                                        SK_Scalar1 - startT));
                    dst->cubicTo(tmp1[1], tmp1[2], tmp1[3]);
                }
            }
            break;
        default:
            SkASSERT(!"unknown segType");
            sk_throw();
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

SkPathMeasure::SkPathMeasure() {
    fPath = NULL;
    fLength = -1;   // signal we need to compute it
    fForceClosed = false;
    fFirstPtIndex = -1;
}

SkPathMeasure::SkPathMeasure(const SkPath& path, bool forceClosed) {
    fPath = &path;
    fLength = -1;   // signal we need to compute it
    fForceClosed = forceClosed;
    fFirstPtIndex = -1;

    fIter.setPath(path, forceClosed);
}

SkPathMeasure::~SkPathMeasure() {}

/** Assign a new path, or null to have none.
*/
void SkPathMeasure::setPath(const SkPath* path, bool forceClosed) {
    fPath = path;
    fLength = -1;   // signal we need to compute it
    fForceClosed = forceClosed;
    fFirstPtIndex = -1;

    if (path) {
        fIter.setPath(*path, forceClosed);
    }
    fSegments.reset();
}

SkScalar SkPathMeasure::getLength() {
    if (fPath == NULL) {
        return 0;
    }
    if (fLength < 0) {
        this->buildSegments();
    }
    SkASSERT(fLength >= 0);
    return fLength;
}

const SkPathMeasure::Segment* SkPathMeasure::distanceToSegment(
                                            SkScalar distance, SkScalar* t) {
    SkDEBUGCODE(SkScalar length = ) this->getLength();
    SkASSERT(distance >= 0 && distance <= length);

    const Segment*  seg = fSegments.begin();
    int             count = fSegments.count();

    int index = SkTSearch<SkScalar>(&seg->fDistance, count, distance,
                                    sizeof(Segment));
    // don't care if we hit an exact match or not, so we xor index if it is negative
    index ^= (index >> 31);
    seg = &seg[index];

    // now interpolate t-values with the prev segment (if possible)
    SkScalar    startT = 0, startD = 0;
    // check if the prev segment is legal, and references the same set of points
    if (index > 0) {
        startD = seg[-1].fDistance;
        if (seg[-1].fPtIndex == seg->fPtIndex) {
            SkASSERT(seg[-1].fType == seg->fType);
            startT = seg[-1].getScalarT();
        }
    }

    SkASSERT(seg->getScalarT() > startT);
    SkASSERT(distance >= startD);
    SkASSERT(seg->fDistance > startD);

    *t = startT + SkScalarMulDiv(seg->getScalarT() - startT,
                                 distance - startD,
                                 seg->fDistance - startD);
    return seg;
}

bool SkPathMeasure::getPosTan(SkScalar distance, SkPoint* pos,
                              SkVector* tangent) {
    SkASSERT(fPath);
    if (fPath == NULL) {
    EMPTY:
        return false;
    }

    SkScalar    length = this->getLength(); // call this to force computing it
    int         count = fSegments.count();

    if (count == 0 || length == 0) {
        goto EMPTY;
    }

    // pin the distance to a legal range
    if (distance < 0) {
        distance = 0;
    } else if (distance > length) {
        distance = length;
    }
    
    SkScalar        t;
    const Segment*  seg = this->distanceToSegment(distance, &t);

    compute_pos_tan(*fPath, fSegments[0].fPtIndex, seg->fPtIndex, seg->fType,
                    t, pos, tangent);
    return true;
}

bool SkPathMeasure::getMatrix(SkScalar distance, SkMatrix* matrix,
                              MatrixFlags flags) {
    SkPoint     position;
    SkVector    tangent;

    if (this->getPosTan(distance, &position, &tangent)) {
        if (matrix) {
            if (flags & kGetTangent_MatrixFlag) {
                matrix->setSinCos(tangent.fY, tangent.fX, 0, 0);
            } else {
                matrix->reset();
            }
            if (flags & kGetPosition_MatrixFlag) {
                matrix->postTranslate(position.fX, position.fY);
            }
        }
        return true;
    }
    return false;
}

bool SkPathMeasure::getSegment(SkScalar startD, SkScalar stopD, SkPath* dst,
                               bool startWithMoveTo) {
    SkASSERT(dst);

    SkScalar length = this->getLength();    // ensure we have built our segments

    if (startD < 0) {
        startD = 0;
    }
    if (stopD > length) {
        stopD = length;
    }
    if (startD >= stopD) {
        return false;
    }

    SkPoint  p;
    SkScalar startT, stopT;
    const Segment* seg = this->distanceToSegment(startD, &startT);
    const Segment* stopSeg = this->distanceToSegment(stopD, &stopT);
    SkASSERT(seg <= stopSeg);

    if (startWithMoveTo) {
        compute_pos_tan(*fPath, fSegments[0].fPtIndex, seg->fPtIndex,
                        seg->fType, startT, &p, NULL);
        dst->moveTo(p);
    }

    if (seg->fPtIndex == stopSeg->fPtIndex) {
        seg_to(*fPath, fSegments[0].fPtIndex, seg->fPtIndex, seg->fType,
               startT, stopT, dst);
    } else {
        do {
            seg_to(*fPath, fSegments[0].fPtIndex, seg->fPtIndex, seg->fType,
                   startT, SK_Scalar1, dst);
            seg = SkPathMeasure::NextSegment(seg);
            startT = 0;
        } while (seg->fPtIndex < stopSeg->fPtIndex);
        seg_to(*fPath, fSegments[0].fPtIndex, seg->fPtIndex, seg->fType,
               0, stopT, dst);
    }
    return true;
}

bool SkPathMeasure::isClosed() {
    (void)this->getLength();
    return fIsClosed;
}

/** Move to the next contour in the path. Return true if one exists, or false if
    we're done with the path.
*/
bool SkPathMeasure::nextContour() {
    fLength = -1;
    return this->getLength() > 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef SK_DEBUG

void SkPathMeasure::dump() {
    SkDebugf("pathmeas: length=%g, segs=%d\n", fLength, fSegments.count());

    for (int i = 0; i < fSegments.count(); i++) {
        const Segment* seg = &fSegments[i];
        SkDebugf("pathmeas: seg[%d] distance=%g, point=%d, t=%g, type=%d\n",
                i, seg->fDistance, seg->fPtIndex, seg->getScalarT(),
                 seg->fType);
    }
}

#endif
