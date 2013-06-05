#include "SkRectShape.h"
#include "SkCanvas.h"

SkPaintShape::SkPaintShape() {
    fPaint.setAntiAlias(true);
}

SkRectShape::SkRectShape() {
    fBounds.setEmpty();
    fRadii.set(0, 0);
}

void SkRectShape::setRect(const SkRect& bounds) {
    fBounds = bounds;
    fRadii.set(0, 0);
}

void SkRectShape::setOval(const SkRect& bounds) {
    fBounds = bounds;
    fRadii.set(-SK_Scalar1, -SK_Scalar1);
}

void SkRectShape::setCircle(SkScalar cx, SkScalar cy, SkScalar radius) {
    fBounds.set(cx - radius, cy - radius, cx + radius, cy + radius);
    fRadii.set(-SK_Scalar1, -SK_Scalar1);
}

void SkRectShape::setRRect(const SkRect& bounds, SkScalar rx, SkScalar ry) {
    if (rx < 0) {
        rx = 0;
    }
    if (ry < 0) {
        ry = 0;
    }

    fBounds = bounds;
    fRadii.set(rx, ry);
}

///////////////////////////////////////////////////////////////////////////////

void SkRectShape::onDraw(SkCanvas* canvas) {
    const SkPaint& paint = this->paint();

    if (fRadii.fWidth < 0) {
        canvas->drawOval(fBounds, paint);
    } else if (fRadii.isZero()) {
        canvas->drawRect(fBounds, paint);
    } else {
        canvas->drawRoundRect(fBounds, fRadii.fWidth, fRadii.fHeight, paint);
    }
}

SkFlattenable::Factory SkRectShape::getFactory() {
    return CreateProc;
}

void SkRectShape::flatten(SkFlattenableWriteBuffer& buffer) {
    this->INHERITED::flatten(buffer);

    buffer.writeRect(fBounds);
    *(SkSize*)buffer.reserve(sizeof(SkSize)) = fRadii;
}

SkRectShape::SkRectShape(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {    
    buffer.read(&fBounds, sizeof(fBounds));
    buffer.read(&fRadii, sizeof(fRadii));
}

SkFlattenable* SkRectShape::CreateProc(SkFlattenableReadBuffer& buffer) {
    return SkNEW_ARGS(SkRectShape, (buffer));
}

///////////////////////////////////////////////////////////////////////////////

void SkPaintShape::flatten(SkFlattenableWriteBuffer& buffer) {
    this->INHERITED::flatten(buffer);
    
    fPaint.flatten(buffer);
}

SkPaintShape::SkPaintShape(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {
    fPaint.unflatten(buffer);
}

static SkFlattenable::Registrar gReg("SkRectShape", SkRectShape::CreateProc);

