/* Copyright 2008, The Android Open Source Project
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

#include <Carbon/Carbon.h>
#include "SkImageDecoder.h"
#include "SkImageEncoder.h"
#include "SkMovie.h"
#include "SkStream.h"
#include "SkTemplates.h"

static void malloc_release_proc(void* info, const void* data, size_t size) {
    sk_free(info);
}

static CGDataProviderRef SkStreamToDataProvider(SkStream* stream) {
    // TODO: use callbacks, so we don't have to load all the data into RAM
    size_t len = stream->getLength();
    void* data = sk_malloc_throw(len);
    stream->read(data, len);
    
    return CGDataProviderCreateWithData(data, data, len, malloc_release_proc);
}

static CGImageSourceRef SkStreamToCGImageSource(SkStream* stream) {
    CGDataProviderRef data = SkStreamToDataProvider(stream);
    CGImageSourceRef imageSrc = CGImageSourceCreateWithDataProvider(data, 0);
    CGDataProviderRelease(data);
    return imageSrc;
}

class SkImageDecoder_CG : public SkImageDecoder {
protected:
    virtual bool onDecode(SkStream* stream, SkBitmap* bm, Mode);
};

#define BITMAP_INFO (kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast)

bool SkImageDecoder_CG::onDecode(SkStream* stream, SkBitmap* bm, Mode mode) {
    CGImageSourceRef imageSrc = SkStreamToCGImageSource(stream);

    if (NULL == imageSrc) {
        return false;
    }
    SkAutoTCallVProc<const void, CFRelease> arsrc(imageSrc);
    
    CGImageRef image = CGImageSourceCreateImageAtIndex(imageSrc, 0, NULL);
    if (NULL == image) {
        return false;
    }
    SkAutoTCallVProc<CGImage, CGImageRelease> arimage(image);
    
    const int width = CGImageGetWidth(image);
    const int height = CGImageGetHeight(image);
    bm->setConfig(SkBitmap::kARGB_8888_Config, width, height);
    if (SkImageDecoder::kDecodeBounds_Mode == mode) {
        return true;
    }
    
    if (!this->allocPixelRef(bm, NULL)) {
        return false;
    }
    
    bm->lockPixels();
    bm->eraseColor(0);

    CGColorSpaceRef cs = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    CGContextRef cg = CGBitmapContextCreate(bm->getPixels(), width, height,
                                            8, bm->rowBytes(), cs, BITMAP_INFO);
    CGContextDrawImage(cg, CGRectMake(0, 0, width, height), image);
    CGContextRelease(cg);
    CGColorSpaceRelease(cs);

    bm->unlockPixels();
    return true;
}

///////////////////////////////////////////////////////////////////////////////

SkImageDecoder* SkImageDecoder::Factory(SkStream* stream) {
    return SkNEW(SkImageDecoder_CG);
}

/////////////////////////////////////////////////////////////////////////

SkMovie* SkMovie::DecodeStream(SkStream* stream) {
    return NULL;
}

/////////////////////////////////////////////////////////////////////////

static size_t consumer_put(void* info, const void* buffer, size_t count) {
    SkWStream* stream = reinterpret_cast<SkWStream*>(info);
    return stream->write(buffer, count) ? count : 0;
}

static void consumer_release(void* info) {
    // we do nothing, since by design we don't "own" the stream (i.e. info)
}

static CGDataConsumerRef SkStreamToCGDataConsumer(SkWStream* stream) {
    CGDataConsumerCallbacks procs;
    procs.putBytes = consumer_put;
    procs.releaseConsumer = consumer_release;
    // we don't own/reference the stream, so it our consumer must not live
    // longer that our caller's ownership of the stream
    return CGDataConsumerCreate(stream, &procs);
}

static CGImageDestinationRef SkStreamToImageDestination(SkWStream* stream,
                                                        CFStringRef type) {
    CGDataConsumerRef consumer = SkStreamToCGDataConsumer(stream);
    if (NULL == consumer) {
        return NULL;
    }
    SkAutoTCallVProc<const void, CFRelease> arconsumer(consumer);
    
    return CGImageDestinationCreateWithDataConsumer(consumer, type, 1, NULL);
}

class SkImageEncoder_CG : public SkImageEncoder {
public:
    SkImageEncoder_CG(Type t) : fType(t) {}

protected:
    virtual bool onEncode(SkWStream* stream, const SkBitmap& bm, int quality);
    
private:
    Type fType;
};

extern CGImageRef SkCreateCGImageRef(const SkBitmap&);

/*  Encode bitmaps via CGImageDestination. We setup a DataConsumer which writes
    to our SkWStream. Since we don't reference/own the SkWStream, our consumer
    must only live for the duration of the onEncode() method.
 */
bool SkImageEncoder_CG::onEncode(SkWStream* stream, const SkBitmap& bm,
                                 int quality) {
    CFStringRef type;
    switch (fType) {
        case kJPEG_Type:
            type = kUTTypeJPEG;
            break;
        case kPNG_Type:
            type = kUTTypePNG;
            break;
        default:
            return false;
    }
    
    CGImageDestinationRef dst = SkStreamToImageDestination(stream, type);
    if (NULL == dst) {
        return false;
    }
    SkAutoTCallVProc<const void, CFRelease> ardst(dst);

    CGImageRef image = SkCreateCGImageRef(bm);
    if (NULL == image) {
        return false;
    }
    SkAutoTCallVProc<CGImage, CGImageRelease> agimage(image);
    
	CGImageDestinationAddImage(dst, image, NULL);
	CGImageDestinationFinalize(dst);
    return true;
}

SkImageEncoder* SkImageEncoder::Create(Type t) {
    switch (t) {
        case kJPEG_Type:
        case kPNG_Type:
            break;
        default:
            return NULL;
    }
    return SkNEW_ARGS(SkImageEncoder_CG, (t));
}

