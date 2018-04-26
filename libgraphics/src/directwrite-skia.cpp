/* Copyright (C) 2003-2015 LiveCode Ltd.
 
 This file is part of LiveCode.
 
 LiveCode is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License v3 as published by the Free
 Software Foundation.
 
 LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.
 
 You should have received a copy of the GNU General Public License
 along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "graphics.h"
#include "graphics-internal.h"

#include <atomic>

#include <dwrite.h>

#include <SkFont.h>
#include <SkTypeface.h>
#include <SkTypeface_win.h>
#include <SkTypeface_win_dw.h>

#pragma comment(lib, "DWrite")

////////////////////////////////////////////////////////////////////////////////

// Class implementing the DirectWrite renderer interface
template <class Callback>
class MCGDWRenderer : 
	public IDWriteTextRenderer
{
public:

	MCGDWRenderer(Callback p_callback) :
		m_callback(p_callback)
	{
	}

	// IUnknown
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(QueryInterface)(REFIID, void**);

	// IDWritePixelSnapping
	STDMETHOD(GetCurrentTransform)(void*, DWRITE_MATRIX*);
	STDMETHOD(GetPixelsPerDip)(void*, FLOAT*);
	STDMETHOD(IsPixelSnappingDisabled)(void*, BOOL*);

	// IDWriteTextRenderer
	STDMETHOD(DrawGlyphRun)(void*, FLOAT, FLOAT, DWRITE_MEASURING_MODE, const DWRITE_GLYPH_RUN*, const DWRITE_GLYPH_RUN_DESCRIPTION*, IUnknown*);
	STDMETHOD(DrawInlineObject)(void*, FLOAT, FLOAT, IDWriteInlineObject*, BOOL, BOOL, IUnknown*);
	STDMETHOD(DrawStrikethrough)(void*, FLOAT, FLOAT, const DWRITE_STRIKETHROUGH*, IUnknown*);
	STDMETHOD(DrawUnderline)(void*, FLOAT, FLOAT, const DWRITE_UNDERLINE*, IUnknown*);

private:

	std::atomic<ULONG> m_refcount = 1;
	Callback m_callback;
};

////////////////////////////////////////////////////////////////////////////////

template <class Callback>
STDMETHODIMP_(ULONG) MCGDWRenderer<Callback>::AddRef()
{
	return ++m_refcount;
}

template <class Callback>
STDMETHODIMP_(ULONG) MCGDWRenderer<Callback>::Release()
{
	ULONG t_remaining = --m_refcount;
	if (t_remaining == 0)
		delete this;

	return t_remaining;
}

template <class Callback>
STDMETHODIMP MCGDWRenderer<Callback>::QueryInterface(REFIID riid, void** ppvObject)
{
	if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWritePixelSnapping) || riid == __uuidof(IDWriteTextRenderer))
	{
		*ppvObject = this;
		return S_OK;
	}

	// Not a recognised interface
	return E_NOINTERFACE;
}

template <class Callback>
STDMETHODIMP MCGDWRenderer<Callback>::GetCurrentTransform(void* context, DWRITE_MATRIX* transform)
{
	// Get the graphics context
	MCGContextRef t_context = static_cast<MCGContextRef>(context);

	// Get the transform used for drawing
	MCGAffineTransform t_transform = MCGContextGetDeviceTransform(t_context);

	// Translate to the DirectWrite matrix format
	transform->m11 = t_transform.a;
	transform->m12 = t_transform.b;
	transform->m21 = t_transform.c;
	transform->m22 = t_transform.d;
	transform->dx  = t_transform.tx;
	transform->dy  = t_transform.ty;

	return S_OK;
}

template <class Callback>
STDMETHODIMP MCGDWRenderer<Callback>::GetPixelsPerDip(void* context, FLOAT* pixelsPerDip)
{
	// Get the graphics context
	MCGContextRef t_context = static_cast<MCGContextRef>(context);

	// Get the transform being used for drawing
	MCGAffineTransform t_transform = MCGContextGetDeviceTransform(t_context);
	
	// The scale factor can be derived from the transform
	*pixelsPerDip = sqrt((t_transform.a*t_transform.a
						+ t_transform.b*t_transform.b
						+ t_transform.c*t_transform.c
						+ t_transform.d*t_transform.d) / 2);
	
	return S_OK;
}

template <class Callback>
STDMETHODIMP MCGDWRenderer<Callback>::IsPixelSnappingDisabled(void* context, BOOL* isDisabled)
{
	// DirectWrite recommends this be enabled unless there is a good reason to disable it
	*isDisabled = TRUE;
	return S_OK;
}

template <class Callback>
STDMETHODIMP MCGDWRenderer<Callback>::DrawGlyphRun(void* context, FLOAT baselineOriginX, FLOAT baselineOriginY, DWRITE_MEASURING_MODE measuringMode, const DWRITE_GLYPH_RUN* glyphRun, const DWRITE_GLYPH_RUN_DESCRIPTION* glyphRunDescription, IUnknown* clientDrawingEffect)
{
	// Forward to the callback
	m_callback(baselineOriginX, baselineOriginY, measuringMode, glyphRun, glyphRunDescription);
	return S_OK;
}

template <class Callback>
STDMETHODIMP MCGDWRenderer<Callback>::DrawInlineObject(void* context, FLOAT originX, FLOAT originY, IDWriteInlineObject* inlineObject, BOOL isSideways, BOOL isRightToLeft, IUnknown* clientDrawingEffect)
{
	// Ignored
	return S_OK;
}

template <class Callback>
STDMETHODIMP MCGDWRenderer<Callback>::DrawStrikethrough(void* context, FLOAT baselineOriginX, FLOAT baselineOriginY, const DWRITE_STRIKETHROUGH* strikethrough, IUnknown* clientDrawingEffect)
{
	// Ignored
	return S_OK;
}

template <class Callback>
STDMETHODIMP MCGDWRenderer<Callback>::DrawUnderline(void* context, FLOAT baselineOriginX, FLOAT baselineOriginY, const DWRITE_UNDERLINE* underline, IUnknown* clientDrawingRect)
{
	// Ignored
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

class MCGDWFontCollectionLoader :
	public IDWriteFontCollectionLoader
{
public:
	// IUnknown
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(QueryInterface)(REFIID, void**);

	// IDWriteFontCollectionLoader
	STDMETHOD(CreateEnumeratorFromKey)(IDWriteFactory* factory, void const* collectionKey, UINT32 collectionKeySize, IDWriteFontFileEnumerator** fontFileEnumerator);

private:
	std::atomic<ULONG> m_refcount = 1;
};

class MCGDWFontFileEnumerator :
	public IDWriteFontFileEnumerator
{
public:
	MCGDWFontFileEnumerator(IDWriteFactory *factory, MCProperListRef files);
	~MCGDWFontFileEnumerator();

	// IUnknown
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(QueryInterface)(REFIID, void**);

	// IDWriteFontFileEnumerator
	STDMETHOD(MoveNext)(BOOL* hasCurrentFile);
	STDMETHOD(GetCurrentFontFile)(IDWriteFontFile** fontFile);

private:
	MCProperListRef m_files;
	uindex_t m_file_index;

	IDWriteFontFile *m_current_file;
	IDWriteFactory *m_factory;
	std::atomic<ULONG> m_refcount = 1;
};

////////////////////////////////////////////////////////////////////////////////

// IUnknown
STDMETHODIMP_(ULONG) MCGDWFontCollectionLoader::AddRef()
{
	return ++m_refcount;
}

STDMETHODIMP_(ULONG) MCGDWFontCollectionLoader::Release()
{
	ULONG t_remaining = --m_refcount;
	if (t_remaining == 0)
		delete this;

	return t_remaining;
}

STDMETHODIMP MCGDWFontCollectionLoader::QueryInterface(REFIID riid, void **ppvObject)
{
	if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWriteFontCollectionLoader))
	{
		*ppvObject = this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

// IDWriteFontCollectionLoader
STDMETHODIMP MCGDWFontCollectionLoader::CreateEnumeratorFromKey(IDWriteFactory* factory, void const* collectionKey, UINT32 collectionKeySize, IDWriteFontFileEnumerator** fontFileEnumerator)
{
	MCProperListRef t_font_files = *static_cast<MCProperListRef const*>(collectionKey);
	MCAssert(collectionKeySize == sizeof(MCProperListRef));
	MCAssert(MCValueGetTypeCode(t_font_files) == kMCValueTypeCodeProperList);

	MCGDWFontFileEnumerator *t_enum = new(std::nothrow) MCGDWFontFileEnumerator(factory, t_font_files);
	if (t_enum == nil)
		return E_OUTOFMEMORY;

	*fontFileEnumerator = t_enum;
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

MCGDWFontFileEnumerator::MCGDWFontFileEnumerator(IDWriteFactory *p_factory, MCProperListRef p_files)
{
	m_current_file = nil;

	m_factory = p_factory;
	p_factory->AddRef();

	m_files = MCValueRetain(p_files);
	m_file_index = 0;
}

MCGDWFontFileEnumerator::~MCGDWFontFileEnumerator()
{
	m_factory->Release();
	MCValueRelease(m_files);

	if (m_current_file != nil)
		m_current_file->Release();
}

// IUnknown
STDMETHODIMP_(ULONG) MCGDWFontFileEnumerator::AddRef()
{
	return ++m_refcount;
}

STDMETHODIMP_(ULONG) MCGDWFontFileEnumerator::Release()
{
	ULONG t_remaining = --m_refcount;
	if (t_remaining == 0)
		delete this;

	return t_remaining;
}

STDMETHODIMP MCGDWFontFileEnumerator::QueryInterface(REFIID riid, void **ppvObject)
{
	if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWriteFontFileEnumerator))
	{
		*ppvObject = this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

// IDWriteFontFileEnumerator
STDMETHODIMP MCGDWFontFileEnumerator::MoveNext(BOOL *r_has_current_file)
{
	if (m_file_index >= MCProperListGetLength(m_files))
	{
		*r_has_current_file = FALSE;
		return S_OK;
	}

	MCStringRef t_fname;
	t_fname = static_cast<MCStringRef>(MCProperListFetchElementAtIndex(m_files, m_file_index));

	MCAssert(MCValueGetTypeCode(t_fname) == kMCValueTypeCodeString);

	MCAutoStringRefAsUTF16String t_utf16_fname;
	if (!t_utf16_fname.Lock(t_fname))
		return E_OUTOFMEMORY;

	IDWriteFontFile *t_font_file = nil;

	HRESULT t_result;
	t_result = m_factory->CreateFontFileReference(t_utf16_fname.Ptr(), NULL, &t_font_file);

	if (!SUCCEEDED(t_result))
		return t_result;

	m_file_index++;

	if (m_current_file != nil)
		m_current_file->Release();

	m_current_file = t_font_file;
	*r_has_current_file = TRUE;

	return t_result;
}

STDMETHODIMP MCGDWFontFileEnumerator::GetCurrentFontFile(IDWriteFontFile **r_current_file)
{
	if (m_current_file == nil)
		return E_FAIL;

	*r_current_file = m_current_file;
	m_current_file->AddRef();

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

// Factory object for creating DirectWrite objects
static IDWriteFactory* s_DWFactory = nil;

// Custom loader for non-system fonts
static IDWriteFontCollectionLoader* s_DWFontCollectionLoader = nil;
// Custom font collection
static IDWriteFontCollection* s_DWFontCollection = nil;
// Set of custom fonts used by loader
static MCProperListRef s_FontFiles = nil;

// GDI interoperation object
static IDWriteGdiInterop* s_GdiInterop = nil;

// DirectWrite text analyser object
static IDWriteTextAnalyzer* s_DWTextAnalyzer = nil;

void MCGPlatformInitialize(void)
{
	bool t_success = true;

	// Create the DirectWrite factory object
	if (t_success)
		t_success = SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&s_DWFactory)));

	// Create the GDI interoperation object
	if (t_success)
		t_success = SUCCEEDED(s_DWFactory->GetGdiInterop(&s_GdiInterop));

	// Create the DirectWrite text analyser
	if (t_success)
		t_success = SUCCEEDED(s_DWFactory->CreateTextAnalyzer(&s_DWTextAnalyzer));

	if (t_success)
		t_success = nil != (s_DWFontCollectionLoader = new(std::nothrow) MCGDWFontCollectionLoader());

	if (t_success)
		t_success = SUCCEEDED(s_DWFactory->RegisterFontCollectionLoader(s_DWFontCollectionLoader));

	if (t_success)
		s_FontFiles = MCValueRetain(kMCEmptyProperList);
}

void MCGPlatformFinalize(void)
{
	// Release the custom font list
	if (s_FontFiles)
		MCValueRelease(s_FontFiles);

	// Release the custom font collection
	if (s_DWFontCollection)
		s_DWFontCollection->Release();

	// Release the custom font loader
	if (s_DWFontCollectionLoader)
	{
		s_DWFactory->UnregisterFontCollectionLoader(s_DWFontCollectionLoader);
		s_DWFontCollectionLoader->Release();
	}

	// Release the text analyser
	if (s_DWTextAnalyzer)
		s_DWTextAnalyzer->Release();
	
	// Release the GDI interop object
	if (s_GdiInterop)
		s_GdiInterop->Release();

	// Release the DirectWrite factory object
	if (s_DWFactory)
		s_DWFactory->Release();
}

////////////////////////////////////////////////////////////////////////////////

bool MCGDWSetFontFiles(MCProperListRef p_files)
{
	IDWriteFontCollection *t_new_collection = nil;
	if (!SUCCEEDED(s_DWFactory->CreateCustomFontCollection(s_DWFontCollectionLoader, &p_files, sizeof(MCProperListRef), &t_new_collection)))
		return false;

	MCValueAssign(s_FontFiles, p_files);

	if (s_DWFontCollection)
		s_DWFontCollection->Release();

	s_DWFontCollection = t_new_collection;

	return true;
}

bool MCGFontAddPlatformFileResource(MCStringRef p_file)
{
	uindex_t t_offset;
	if (MCProperListFirstIndexOfElement(s_FontFiles, p_file, 0, t_offset))
		/* Font file already in collection */
		return true;

	MCAutoProperListRef t_files;
	if (!MCProperListMutableCopy(s_FontFiles, &t_files))
		return false;

	if (!MCProperListPushElementOntoBack(*t_files, p_file))
		return false;

	return MCGDWSetFontFiles(*t_files);
}

bool MCGFontRemovePlatformFileResource(MCStringRef p_file)
{
	uindex_t t_offset;
	if (!MCProperListFirstIndexOfElement(s_FontFiles, p_file, 0, t_offset))
		/* Font file not in collection */
		return true;

	MCAutoProperListRef t_files;
	if (!MCProperListMutableCopy(s_FontFiles, &t_files))
		return false;

	if (!MCProperListRemoveElement(*t_files, t_offset))
		return false;

	return MCGDWSetFontFiles(*t_files);
}

////////////////////////////////////////////////////////////////////////////////

static bool MCGDWGetDefaultLocalizedString(IDWriteLocalizedStrings *p_strings, MCStringRef &r_string)
{
	UINT32 t_index = 0;
	BOOL t_found = FALSE;
	if (!SUCCEEDED(p_strings->FindLocaleName(L"en-us", &t_index, &t_found)))
		return false;

	// Fallback to first name in list
	if (!t_found)
		t_index = 0;

	UINT32 t_name_length;
	if (!SUCCEEDED(p_strings->GetStringLength(t_index, &t_name_length)))
		return false;

	MCAutoArray<WCHAR> t_buffer;
	if (!t_buffer.New(t_name_length + 1))
		return false;
	if (!SUCCEEDED(p_strings->GetString(t_index, t_buffer.Ptr(), t_name_length + 1)))
		return false;

	return MCStringCreateWithWString(t_buffer.Ptr(), r_string);
}

static bool MCGDWAddCollectionFontsToList(IDWriteFontCollection *p_collection, MCProperListRef p_list)
{
	if (p_collection == nil)
		return true;

	bool t_success = true;

	UINT32 t_count;
	t_count = p_collection->GetFontFamilyCount();

	for (uint32_t i = 0; t_success && i < t_count; i++)
	{
		IDWriteFontFamily *t_family = nil;
		t_success = SUCCEEDED(p_collection->GetFontFamily(i, &t_family));

		IDWriteLocalizedStrings *t_names = nil;
		if (t_success)
			t_success = SUCCEEDED(t_family->GetFamilyNames(&t_names));

		MCAutoStringRef t_name;
		if (t_success)
			t_success = MCGDWGetDefaultLocalizedString(t_names, &t_name);

		if (t_success)
			t_success = MCProperListPushElementOntoBack(p_list, *t_name);

		if (t_names != nil)
			t_names->Release();
		if (t_family != nil)
			t_family->Release();
	}
}

bool MCGFontGetPlatformFontList(MCProperListRef &r_fonts)
{
	bool t_success = true;

	MCAutoProperListRef t_fonts;
	t_success = MCProperListCreateMutable(&t_fonts);

	IDWriteFontCollection *t_system_collection = nil;
	if (t_success)
		t_success = SUCCEEDED(s_DWFactory->GetSystemFontCollection(&t_system_collection));
	if (t_success)
		t_success = MCGDWAddCollectionFontsToList(t_system_collection, *t_fonts);

	if (t_success && s_DWFontCollection != nil)
		t_success = MCGDWAddCollectionFontsToList(s_DWFontCollection, *t_fonts);

	if (t_success)
		r_fonts = t_fonts.Take();

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

// Creates a DirectWrite text format object from an HFONT
static IDWriteTextFormat* MCGDWTextFormatFromHFONT(HFONT p_hfont, MCGFloat p_size)
{
	// Get the LOGFONT information for the given HFONT
	// Note that we don't trust the height from this as it is the cell height, not "normal" height
	LOGFONTW t_logfont;
	GetObjectW(p_hfont, sizeof(LOGFONTW), &t_logfont);

	// Extract the important parameters
	// Note: same weight values in GDI and DirectWrite
	DWRITE_FONT_WEIGHT t_font_weight = static_cast<DWRITE_FONT_WEIGHT>(t_logfont.lfWeight);
	DWRITE_FONT_STRETCH t_font_stretch = DWRITE_FONT_STRETCH_NORMAL;
	FLOAT t_font_size = p_size;
	DWRITE_FONT_STYLE t_font_style = DWRITE_FONT_STYLE_NORMAL;
	if (t_logfont.lfItalic)
		t_font_style = DWRITE_FONT_STYLE_ITALIC;
	else if (t_logfont.lfOrientation > 0)
		t_font_style = DWRITE_FONT_STYLE_OBLIQUE;

	// Create the text format object
	IDWriteTextFormat* t_format = nil;
	// First try custom font collection, then fall back to system collection
	if (s_DWFontCollection != nil)
	{
		UINT32 t_index = 0;
		BOOL t_found = FALSE;

		s_DWFontCollection->FindFamilyName(t_logfont.lfFaceName, &t_index, &t_found);
		if (t_found && SUCCEEDED(s_DWFactory->CreateTextFormat(t_logfont.lfFaceName, s_DWFontCollection, t_font_weight, t_font_style, t_font_stretch, t_font_size, L"en-us", &t_format)))
			return t_format;
	}

	if (!SUCCEEDED(s_DWFactory->CreateTextFormat(t_logfont.lfFaceName, nil, t_font_weight, t_font_style, t_font_stretch, t_font_size, L"en-us", &t_format)))
		return nil;

	return t_format;
}

// Creates an SkPaint from a DirectWrite font face
static inline void MCGSkPaintFromDWFontFace(IDWriteFontFace* p_face, FLOAT p_emsize, const MCGAffineTransform& p_transform, SkPaint& x_paint)
{
	// Map from font face to font (requires the font collection containing the font)
	// Search the custom collection for the font
	IDWriteFont* t_font = nil;
	if (s_DWFontCollection != nil)
		s_DWFontCollection->GetFontFromFontFace(p_face, &t_font);

	if (t_font == nil)
	{
		// Fallback to checking system font collection
		IDWriteFontCollection *t_sys_collection = nil;
		s_DWFactory->GetSystemFontCollection(&t_sys_collection);
		t_sys_collection->GetFontFromFontFace(p_face, &t_font);
		t_sys_collection->Release();
	}
	// Get the family from the font
	IDWriteFontFamily* t_family;
	t_font->GetFontFamily(&t_family);
	
	// Create a Skia typeface from the DirectWrite font face
	sk_sp<DWriteFontTypeface> t_typeface(DWriteFontTypeface::Create(s_DWFactory, p_face, t_font, t_family));

	// Clean up the DirectWrite resources
	t_family->Release();
	t_font->Release();

	// Configurethe paint for text rendering.
	// Both LCD font smoothing and embedded bitmap rendering are enable
	x_paint.setTypeface(t_typeface);
	x_paint.setLCDRenderText(true);
	x_paint.setEmbeddedBitmapText(true);

	// Set the size, converting device independent pixels (DIPs) to points
	x_paint.setTextSize(p_emsize);

	// Set the transform we're using
	SkMatrix t_matrix;
	MCGAffineTransformToSkMatrix(p_transform, t_matrix);
	x_paint.setTextMatrix(&t_matrix);
}

////////////////////////////////////////////////////////////////////////////////

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform)
{
	// Calculated width
	FLOAT t_width = 0.0f;
	
	// Create a text format object describing the font
	IDWriteTextFormat* t_format = MCGDWTextFormatFromHFONT(static_cast<HFONT>(p_font.fid), p_font.size);
    if (t_format != nullptr)
    {
	    // Create a text layout for the string and this font
	    IDWriteTextLayout* t_layout = nil;
	    if (SUCCEEDED(s_DWFactory->CreateTextLayout(p_text, p_length / sizeof(unichar_t), t_format, INFINITY, INFINITY, &t_layout)))
	    {
		    // Get the text metrics for the layout
		    DWRITE_TEXT_METRICS t_metrics;
		    if (SUCCEEDED(t_layout->GetMetrics(&t_metrics)))
		    {
			    t_width = t_metrics.widthIncludingTrailingWhitespace;
		    }

            t_layout->Release();
	    }

	    t_format->Release();
    }

	return t_width;
}

bool MCGContextMeasurePlatformTextImageBounds(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform, MCGRectangle &r_bounds)
{
	// Create a text format object describing the font
	IDWriteTextFormat* t_format = MCGDWTextFormatFromHFONT(static_cast<HFONT>(p_font.fid), p_font.size);
	if (t_format == nil)
		return false;

	// Create a text layout for the string and this font
	bool t_success = false;
	IDWriteTextLayout* t_layout = nil;
	if (SUCCEEDED(s_DWFactory->CreateTextLayout(p_text, p_length / sizeof(unichar_t), t_format, INFINITY, INFINITY, &t_layout)))
	{
		// Get the text metrics for the layout
		DWRITE_LINE_METRICS t_line_metrics;
		UINT32 t_line_metrics_count = 1;

		DWRITE_TEXT_METRICS t_metrics;
		if (SUCCEEDED(t_layout->GetMetrics(&t_metrics)) && SUCCEEDED(t_layout->GetLineMetrics(&t_line_metrics, 1, &t_line_metrics_count)))
		{
			r_bounds = MCGRectangleMake(t_metrics.left, -t_line_metrics.baseline, t_metrics.width, t_metrics.height);
			t_success = true;
		}
	}

	// Release the created objects and return
	if (t_layout != NULL)
		t_layout->Release();
	if (t_format != NULL)
		t_format->Release();

	return t_success;
}

void MCGContextDrawPlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font, bool p_rtl)
{
	if (!MCGContextIsValid(self))
		return;

	// The transform that drawing will be using
	MCGAffineTransform t_transform = MCGContextGetDeviceTransform(self);

	// Create a text format object describing the font
	IDWriteTextFormat* t_format = MCGDWTextFormatFromHFONT(static_cast<HFONT>(p_font.fid), p_font.size);
	if (t_format == nil)
		return;

	// Create a text layout for the string and this font
	bool t_success = false;
	IDWriteTextLayout* t_layout = nil;
	if (SUCCEEDED(s_DWFactory->CreateTextLayout(p_text, p_length / sizeof(unichar_t), t_format, INFINITY, INFINITY, &t_layout)))
	{
		MCGFloat t_advance = 0.0f;
		MCGFloat t_advance_direction = 1.0f;

		// If the text is RTL, the advance starts from the end and decreases
		if (p_rtl)
		{
			DWRITE_TEXT_METRICS t_metrics;
			t_layout->GetMetrics(&t_metrics);
			t_advance = t_metrics.width;
			t_advance_direction = -1.0f;
		}
		
        // Setup the paint
        SkPaint t_paint;
        if (MCGContextSetupFill(self, t_paint))
        {
            // Drawing callback
            auto t_callback = [=, &t_advance, &t_paint](FLOAT p_x, FLOAT p_y, DWRITE_MEASURING_MODE p_mode, const DWRITE_GLYPH_RUN* p_run, const DWRITE_GLYPH_RUN_DESCRIPTION* p_description)
            {
                // Apply the font face to the Skia paint object
                MCGSkPaintFromDWFontFace(p_run->fontFace, p_run->fontEmSize, t_transform, t_paint);

                // disable LCD rendering when backing layer may be transparent
                if (!MCGContextIsLayerOpaque(self))
                    t_paint.setLCDRenderText(false);

                // Force anti-aliasing on so that we get nicely-rendered text
                t_paint.setAntiAlias(true);

                // Text will be specified by glyph index, not codepoint/codeunit
                t_paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);

                // Loop over the glyphs in the run
                for (UINT32 i = 0; i < p_run->glyphCount; i++)
                {
                    if (p_rtl)
                    {
                        // Advance the position for the next glyph
                        t_advance += t_advance_direction * p_run->glyphAdvances[i];
                    }
                    
                    // Position for this glyph
                    MCGFloat t_x = p_location.x + t_advance;
                    MCGFloat t_y = p_location.y;
                    if (p_run->glyphOffsets)
                    {
                        t_x += t_advance_direction * p_run->glyphOffsets[i].advanceOffset;
                        t_y += p_run->glyphOffsets[i].ascenderOffset;
                    }

                    if (!p_rtl)
                    {
                        // Advance the position for the next glyph
                        t_advance += t_advance_direction * p_run->glyphAdvances[i];
                    }

                    // Render this glyph to the canvas
                    // Relies on DirectWrite and Skia both using 16-bit integers for glyph indices
                    self->layer->canvas->drawText(&p_run->glyphIndices[i], sizeof(SkGlyphID), t_x, t_y, t_paint);
                }
            };
            
            // Create an object to receive the "Draw" callbacks
            auto* t_renderer = new MCGDWRenderer<decltype(t_callback)>(t_callback);

            // Perform the drawing
            t_layout->Draw(self, t_renderer, 0.0f, 0.0f);

            // Free the renderer object
            t_renderer->Release();
        }
	}

	// Free the DirectWrite objects
	if (t_layout != nil)
		t_layout->Release();
	if (t_format != nil)
		t_format->Release();
}
