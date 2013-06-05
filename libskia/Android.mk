LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libskia

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	$(addprefix src/core/, \
		Sk64.cpp \
		SkAlphaRuns.cpp \
		SkBitmap.cpp \
		SkBitmap_scroll.cpp \
		SkBitmapProcShader.cpp \
		SkBitmapProcState.cpp \
		SkBitmapProcState_matrixProcs.cpp \
		SkBitmapSampler.cpp \
		SkBlitRow_D16.cpp \
		SkBlitRow_D32.cpp \
		SkBlitRow_D4444.cpp \
		SkBlitter.cpp \
		SkBlitter_4444.cpp \
		SkBlitter_A1.cpp \
		SkBlitter_A8.cpp \
		SkBlitter_ARGB32.cpp \
		SkBlitter_ARGB32_Subpixel.cpp \
		SkBlitter_RGB16.cpp \
		SkBlitter_Sprite.cpp \
		SkBuffer.cpp \
		SkCanvas.cpp \
		SkChunkAlloc.cpp \
		SkColor.cpp \
		SkColorFilter.cpp \
		SkColorTable.cpp \
		SkComposeShader.cpp \
		SkConcaveToTriangles.cpp \
		SkCordic.cpp \
		SkCubicClipper.cpp \
		SkDebug.cpp \
		SkDeque.cpp \
		SkDevice.cpp \
		SkDither.cpp \
		SkDraw.cpp \
		SkEdge.cpp \
		SkEdgeBuilder.cpp \
		SkEdgeClipper.cpp \
		SkFilterProc.cpp \
		SkFlattenable.cpp \
		SkFloat.cpp \
		SkFloatBits.cpp \
		SkFontHost.cpp \
		SkGeometry.cpp \
		SkGlobals.cpp \
		SkGlyphCache.cpp \
		SkGraphics.cpp \
		SkLineClipper.cpp \
		SkMask.cpp \
		SkMaskFilter.cpp \
		SkMath.cpp \
		SkMatrix.cpp \
		SkMMapStream.cpp \
		SkPackBits.cpp \
		SkPaint.cpp \
		SkPath.cpp \
		SkPathEffect.cpp \
		SkPathHeap.cpp \
		SkPathMeasure.cpp \
		SkPicture.cpp \
		SkPictureFlat.cpp \
		SkPicturePlayback.cpp \
		SkPictureRecord.cpp \
		SkPixelRef.cpp \
		SkPoint.cpp \
		SkProcSpriteBlitter.cpp \
		SkPtrRecorder.cpp \
		SkQuadClipper.cpp \
		SkRasterizer.cpp \
		SkRect.cpp \
		SkRefCnt.cpp \
		SkRegion.cpp \
		SkRegion_path.cpp \
		SkScalerContext.cpp \
		SkScan.cpp \
		SkScan_Antihair.cpp \
		SkScan_AntiPath.cpp \
		SkScan_Hairline.cpp \
		SkScan_Path.cpp \
		SkShader.cpp \
		SkShape.cpp \
		SkSpriteBlitter_ARGB32.cpp \
		SkSpriteBlitter_RGB16.cpp \
		SkStream.cpp \
		SkString.cpp \
		SkStroke.cpp \
		SkStrokerPriv.cpp \
		SkTSearch.cpp \
		SkTypeface.cpp \
		SkUnPreMultiply.cpp \
		SkUtils.cpp \
		SkWriter32.cpp \
		SkXfermode.cpp \
		SkMemory_stdlib.cpp \
		)

LOCAL_SRC_FILES+=$(addprefix src/effects/, \
		SkDashPathEffect.cpp \
		)
		
LOCAL_SRC_FILES+=$(addprefix src/opts/, \
		SkBitmapProcState_opts_none.cpp \
		SkBlitRow_opts_none.cpp \
		SkUtils_opts_none.cpp \
		)
		
LOCAL_SRC_FILES+=$(addprefix src/ports/, \
		SkDebug_stdio.cpp \
		SkFontHost_android.cpp \
		SkFontHost_FreeType.cpp \
		SkFontHost_FreeType_Subpixel.cpp \
		SkFontHost_gamma_none.cpp \
		SkGlobals_global.cpp \
		SkOSFile_stdio.cpp \
		SkThread_none.cpp \
		SkTime_Unix.cpp \
		)

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/src/core \
		$(LOCAL_PATH)/include/core \
		$(LOCAL_PATH)/include/effects \
		$(LOCAL_PATH)/include/config \
		$(LOCAL_PATH)/include/ports \
		$(LOCAL_PATH)/../libfreetype/include

include $(BUILD_STATIC_LIBRARY)
