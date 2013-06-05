/* libs/graphics/ports/SkImageDecoder_Factory.cpp
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

#include "SkImageDecoder.h"
#include "SkMovie.h"
#include "SkStream.h"

extern SkImageDecoder* SkImageDecoder_GIF_Factory(SkStream*);
extern SkImageDecoder* SkImageDecoder_BMP_Factory(SkStream*);
extern SkImageDecoder* SkImageDecoder_ICO_Factory(SkStream*);
extern SkImageDecoder* SkImageDecoder_PNG_Factory(SkStream*);
extern SkImageDecoder* SkImageDecoder_WBMP_Factory(SkStream*);
extern SkImageDecoder* SkImageDecoder_JPEG_Factory(SkStream*);

typedef SkImageDecoder* (*SkImageDecoderFactoryProc)(SkStream*);

struct CodecFormat {
    SkImageDecoderFactoryProc   fProc;
    SkImageDecoder::Format      fFormat;
};

static const CodecFormat gPairs[] = {
#if 0
    { SkImageDecoder_GIF_Factory,   SkImageDecoder::kGIF_Format },
    { SkImageDecoder_PNG_Factory,   SkImageDecoder::kPNG_Format },
    { SkImageDecoder_ICO_Factory,   SkImageDecoder::kICO_Format },
    { SkImageDecoder_WBMP_Factory,  SkImageDecoder::kWBMP_Format },
    { SkImageDecoder_BMP_Factory,   SkImageDecoder::kBMP_Format },
    { SkImageDecoder_JPEG_Factory,  SkImageDecoder::kJPEG_Format }
#endif
};

SkImageDecoder* SkImageDecoder::Factory(SkStream* stream) {
    for (size_t i = 0; i < SK_ARRAY_COUNT(gPairs); i++) {
        SkImageDecoder* codec = gPairs[i].fProc(stream);
        stream->rewind();
        if (NULL != codec) {
            return codec;
        }
    }
    return NULL;
}

bool SkImageDecoder::SupportsFormat(Format format) {
    for (size_t i = 0; i < SK_ARRAY_COUNT(gPairs); i++) {
        if (gPairs[i].fFormat == format) {
            return true;
        }
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////

typedef SkMovie* (*SkMovieFactoryProc)(SkStream*);

extern SkMovie* SkMovie_GIF_Factory(SkStream*);

static const SkMovieFactoryProc gMovieProcs[] = {
#if 0
    SkMovie_GIF_Factory
#endif
};

SkMovie* SkMovie::DecodeStream(SkStream* stream) {
    for (unsigned i = 0; i < SK_ARRAY_COUNT(gMovieProcs); i++) {
        SkMovie* movie = gMovieProcs[i](stream);
        if (NULL != movie) {
            return movie;
        }
        stream->rewind();
    }
    return NULL;
}

/////////////////////////////////////////////////////////////////////////

extern SkImageEncoder* SkImageEncoder_JPEG_Factory();
extern SkImageEncoder* SkImageEncoder_PNG_Factory();

SkImageEncoder* SkImageEncoder::Create(Type t) {
    switch (t) {
#if 0
        case kJPEG_Type:
            return SkImageEncoder_JPEG_Factory();
        case kPNG_Type:
            return SkImageEncoder_PNG_Factory();
#endif
        default:
            return NULL;
    }
}

