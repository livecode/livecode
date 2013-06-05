#include <stdlib.h>
#include <stdio.h>

#if defined(_MACOSX)
#include <mach-o/dyld.h>
typedef const struct mach_header *module_t;
#define SYMBOL_PREFIX "_"
#elif defined(_LINUX)
#include <dlfcn.h>
typedef void *module_t;
#define SYMBOL_PREFIX
#elif defined(_WINDOWS)
#include <windows.h>
typedef HMODULE module_t;
#define SYMBOL_PREFIX
#endif

typedef void *handler_t;

extern "C"
{

static int module_load(const char *p_source, module_t *r_module)
{
  module_t t_module;
#if defined(_MACOSX)
  t_module  = NSAddImage(p_source, NSADDIMAGE_OPTION_RETURN_ON_ERROR | NSADDIMAGE_OPTION_WITH_SEARCHING);
#elif defined(_LINUX)
  t_module = dlopen(p_source, RTLD_LAZY);
#elif defined(_WINDOWS)
  t_module = LoadLibraryA(p_source);
#endif
  if (t_module == NULL)
    return 0;
  *r_module = t_module;
  return 1;
}

static int module_unload(module_t t_module)
{
  return 1;
}

static int module_resolve(module_t p_module, const char *p_name, handler_t *r_handler)
{
  handler_t t_handler = NULL;
#if defined(_MACOSX)
  NSSymbol t_symbol;
  t_symbol = NSLookupSymbolInImage(p_module, p_name, NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
  if (t_symbol != NULL)
    t_handler = (handler_t)NSAddressOfSymbol(t_symbol);
#elif defined(_LINUX)
  t_handler = (handler_t)dlsym(p_module, p_name);
#elif defined(_WINDOWS)
  t_handler = (handler_t)GetProcAddress(p_module, p_name);
#endif
  if (t_handler == NULL)
    return 0;
  *r_handler = t_handler;
  return 1;
}

typedef void *(*XftFontOpenName_t)(void *pArg1, int pArg2, void *pArg3);
XftFontOpenName_t XftFontOpenName_ptr = NULL;
typedef void (*XftFontClose_t)(void *pArg1, void *pArg2);
XftFontClose_t XftFontClose_ptr = NULL;
typedef void (*XftTextExtents16_t)(void *pArg1, void *pArg2, void *pArg3, int pArg4, void *pArg5);
XftTextExtents16_t XftTextExtents16_ptr = NULL;
typedef void (*XftTextExtents8_t)(void *pArg1, void *pArg2, void *pArg3, int pArg4, void *pArg5);
XftTextExtents8_t XftTextExtents8_ptr = NULL;
typedef void *(*XftDrawCreate_t)(void *pArg1, int pArg2, void *pArg3, int pArg4);
XftDrawCreate_t XftDrawCreate_ptr = NULL;
typedef void *(*XftDrawCreateAlpha_t)(void *pArg1, void *pArg2, int pArg3);
XftDrawCreateAlpha_t XftDrawCreateAlpha_ptr = NULL;
typedef void *(*XftDrawCreateBitmap_t)(void *pArg1, int pArg2);
XftDrawCreateBitmap_t XftDrawCreateBitmap_ptr = NULL;
typedef void (*XftDrawDestroy_t)(void *pArg1);
XftDrawDestroy_t XftDrawDestroy_ptr = NULL;
typedef int (*XftDrawSetClip_t)(void *pArg1, int pArg2);
XftDrawSetClip_t XftDrawSetClip_ptr = NULL;
typedef void (*XftDrawRect_t)(void *pArg1, void *pArg2, int pArg3, int pArg4, int pArg5, int pArg6);
XftDrawRect_t XftDrawRect_ptr = NULL;
typedef void (*XftDrawString16_t)(void *pArg1, void *pArg2, void *pArg3, int pArg4, int pArg5, void *pArg6, int pArg7);
XftDrawString16_t XftDrawString16_ptr = NULL;
typedef void (*XftDrawStringUtf8_t)(void *pArg1, void *pArg2, void *pArg3, int pArg4, int pArg5, void *pArg6, int pArg7);
XftDrawStringUtf8_t XftDrawStringUtf8_ptr = NULL;
typedef void (*XftDrawString8_t)(void *pArg1, void *pArg2, void *pArg3, int pArg4, int pArg5, void *pArg6, int pArg7);
XftDrawString8_t XftDrawString8_ptr = NULL;
typedef void (*XftDrawChange_t)(void *pArg1, void *pArg2);
XftDrawChange_t XftDrawChange_ptr = NULL;
typedef void (*XftColorFree_t)(void *pArg1, void *pArg2, void *pArg3, void *pArg4);
XftColorFree_t XftColorFree_ptr = NULL;
typedef int (*XftColorAllocValue_t)(void *pArg1, void *pArg2, int pArg3, void *pArg4, void *pArg5);
XftColorAllocValue_t XftColorAllocValue_ptr = NULL;
typedef void *(*XftFontOpenXlfd_t)(void *pArg1, int pArg2, void *pArg3);
XftFontOpenXlfd_t XftFontOpenXlfd_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_XFT_NAME ""
#elif defined(_LINUX)
#define MODULE_XFT_NAME "libXft.so.2"
#elif defined(_WINDOWS)
#define MODULE_XFT_NAME ""
#endif

static module_t module_xft = NULL;

int initialise_weak_link_xft(void)
{
#if defined(_LINUX)
  if (!module_load("libXft.so.2", &module_xft))
#else
  if (!module_load(MODULE_XFT_NAME, &module_xft))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libXft.so.2\n") ;
#endif
goto err;
}

  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftFontOpenName", (handler_t *)&XftFontOpenName_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftFontOpenName\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftFontClose", (handler_t *)&XftFontClose_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftFontClose\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftTextExtents16", (handler_t *)&XftTextExtents16_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftTextExtents16\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftTextExtents8", (handler_t *)&XftTextExtents8_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftTextExtents8\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftDrawCreate", (handler_t *)&XftDrawCreate_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftDrawCreate\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftDrawCreateAlpha", (handler_t *)&XftDrawCreateAlpha_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftDrawCreateAlpha\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftDrawCreateBitmap", (handler_t *)&XftDrawCreateBitmap_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftDrawCreateBitmap\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftDrawDestroy", (handler_t *)&XftDrawDestroy_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftDrawDestroy\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftDrawSetClip", (handler_t *)&XftDrawSetClip_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftDrawSetClip\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftDrawRect", (handler_t *)&XftDrawRect_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftDrawRect\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftDrawString16", (handler_t *)&XftDrawString16_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftDrawString16\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftDrawStringUtf8", (handler_t *)&XftDrawStringUtf8_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftDrawStringUtf8\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftDrawString8", (handler_t *)&XftDrawString8_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftDrawString8\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftDrawChange", (handler_t *)&XftDrawChange_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftDrawChange\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftColorFree", (handler_t *)&XftColorFree_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftColorFree\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftColorAllocValue", (handler_t *)&XftColorAllocValue_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftColorAllocValue\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_xft, SYMBOL_PREFIX "XftFontOpenXlfd", (handler_t *)&XftFontOpenXlfd_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XftFontOpenXlfd\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_xft != NULL)
    module_unload(module_xft);

  return 0;
}

void *XftFontOpenName(void *pArg1, int pArg2, void *pArg3)
{
  return XftFontOpenName_ptr(pArg1, pArg2, pArg3);
}

void XftFontClose(void *pArg1, void *pArg2)
{
  XftFontClose_ptr(pArg1, pArg2);
}

void XftTextExtents16(void *pArg1, void *pArg2, void *pArg3, int pArg4, void *pArg5)
{
  XftTextExtents16_ptr(pArg1, pArg2, pArg3, pArg4, pArg5);
}

void XftTextExtents8(void *pArg1, void *pArg2, void *pArg3, int pArg4, void *pArg5)
{
  XftTextExtents8_ptr(pArg1, pArg2, pArg3, pArg4, pArg5);
}

void *XftDrawCreate(void *pArg1, int pArg2, void *pArg3, int pArg4)
{
  return XftDrawCreate_ptr(pArg1, pArg2, pArg3, pArg4);
}

void *XftDrawCreateAlpha(void *pArg1, void *pArg2, int pArg3)
{
  return XftDrawCreateAlpha_ptr(pArg1, pArg2, pArg3);
}

void *XftDrawCreateBitmap(void *pArg1, int pArg2)
{
  return XftDrawCreateBitmap_ptr(pArg1, pArg2);
}

void XftDrawDestroy(void *pArg1)
{
  XftDrawDestroy_ptr(pArg1);
}

int XftDrawSetClip(void *pArg1, int pArg2)
{
  return XftDrawSetClip_ptr(pArg1, pArg2);
}

void XftDrawRect(void *pArg1, void *pArg2, int pArg3, int pArg4, int pArg5, int pArg6)
{
  XftDrawRect_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6);
}

void XftDrawString16(void *pArg1, void *pArg2, void *pArg3, int pArg4, int pArg5, void *pArg6, int pArg7)
{
  XftDrawString16_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7);
}

void XftDrawStringUtf8(void *pArg1, void *pArg2, void *pArg3, int pArg4, int pArg5, void *pArg6, int pArg7)
{
  XftDrawStringUtf8_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7);
}

void XftDrawString8(void *pArg1, void *pArg2, void *pArg3, int pArg4, int pArg5, void *pArg6, int pArg7)
{
  XftDrawString8_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7);
}

void XftDrawChange(void *pArg1, void *pArg2)
{
  XftDrawChange_ptr(pArg1, pArg2);
}

void XftColorFree(void *pArg1, void *pArg2, void *pArg3, void *pArg4)
{
  XftColorFree_ptr(pArg1, pArg2, pArg3, pArg4);
}

int XftColorAllocValue(void *pArg1, void *pArg2, int pArg3, void *pArg4, void *pArg5)
{
  return XftColorAllocValue_ptr(pArg1, pArg2, pArg3, pArg4, pArg5);
}

void *XftFontOpenXlfd(void *pArg1, int pArg2, void *pArg3)
{
  return XftFontOpenXlfd_ptr(pArg1, pArg2, pArg3);
}

typedef int (*esd_close_t)(int pArg1);
esd_close_t esd_close_ptr = NULL;
typedef int (*esd_play_stream_t)(int pArg1, int pArg2, void *pArg3, void *pArg4);
esd_play_stream_t esd_play_stream_ptr = NULL;
typedef int (*esd_open_sound_t)(void *pArg1);
esd_open_sound_t esd_open_sound_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_ESD_NAME ""
#elif defined(_LINUX)
#define MODULE_ESD_NAME "libesd.so.0"
#elif defined(_WINDOWS)
#define MODULE_ESD_NAME ""
#endif

static module_t module_esd = NULL;

int initialise_weak_link_esd(void)
{
#if defined(_LINUX)
  if (!module_load("libesd.so.0", &module_esd))
#else
  if (!module_load(MODULE_ESD_NAME, &module_esd))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libesd.so.0\n") ;
#endif
goto err;
}

  if (!module_resolve(module_esd, SYMBOL_PREFIX "esd_close", (handler_t *)&esd_close_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: esd_close\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_esd, SYMBOL_PREFIX "esd_play_stream", (handler_t *)&esd_play_stream_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: esd_play_stream\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_esd, SYMBOL_PREFIX "esd_open_sound", (handler_t *)&esd_open_sound_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: esd_open_sound\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_esd != NULL)
    module_unload(module_esd);

  return 0;
}

int esd_close(int pArg1)
{
  return esd_close_ptr(pArg1);
}

int esd_play_stream(int pArg1, int pArg2, void *pArg3, void *pArg4)
{
  return esd_play_stream_ptr(pArg1, pArg2, pArg3, pArg4);
}

int esd_open_sound(void *pArg1)
{
  return esd_open_sound_ptr(pArg1);
}

typedef int (*gnome_vfs_init_t)(void);
gnome_vfs_init_t gnome_vfs_init_ptr = NULL;
typedef int (*gnome_vfs_initialized_t)(void);
gnome_vfs_initialized_t gnome_vfs_initialized_ptr = NULL;
typedef void *(*gnome_vfs_get_mime_type_for_name_t)(void *pArg1);
gnome_vfs_get_mime_type_for_name_t gnome_vfs_get_mime_type_for_name_ptr = NULL;
typedef void *(*gnome_vfs_mime_get_default_application_t)(void *pArg1);
gnome_vfs_mime_get_default_application_t gnome_vfs_mime_get_default_application_ptr = NULL;
typedef void *(*gnome_vfs_mime_get_default_application_for_uri_t)(void *pArg1, void *pArg2);
gnome_vfs_mime_get_default_application_for_uri_t gnome_vfs_mime_get_default_application_for_uri_ptr = NULL;
typedef void *(*gnome_vfs_mime_application_get_exec_t)(void *pArg1);
gnome_vfs_mime_application_get_exec_t gnome_vfs_mime_application_get_exec_ptr = NULL;
typedef int (*gnome_vfs_mime_application_launch_t)(void *pArg1, void *pArg2);
gnome_vfs_mime_application_launch_t gnome_vfs_mime_application_launch_ptr = NULL;
typedef void *(*gnome_vfs_get_mime_type_from_uri_t)(void *pArg1);
gnome_vfs_get_mime_type_from_uri_t gnome_vfs_get_mime_type_from_uri_ptr = NULL;
typedef void *(*gnome_vfs_uri_new_t)(void *pArg1);
gnome_vfs_uri_new_t gnome_vfs_uri_new_ptr = NULL;
typedef void *(*gnome_vfs_make_uri_from_input_t)(void *pArg1);
gnome_vfs_make_uri_from_input_t gnome_vfs_make_uri_from_input_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_GNOME_VFS_NAME ""
#elif defined(_LINUX)
#define MODULE_GNOME_VFS_NAME "libgnomevfs-2.so.0"
#elif defined(_WINDOWS)
#define MODULE_GNOME_VFS_NAME ""
#endif

static module_t module_gnome_vfs = NULL;

int initialise_weak_link_gnome_vfs(void)
{
#if defined(_LINUX)
  if (!module_load("libgnomevfs-2.so.0", &module_gnome_vfs))
#else
  if (!module_load(MODULE_GNOME_VFS_NAME, &module_gnome_vfs))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libgnomevfs-2.so.0\n") ;
#endif
goto err;
}

  if (!module_resolve(module_gnome_vfs, SYMBOL_PREFIX "gnome_vfs_init", (handler_t *)&gnome_vfs_init_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gnome_vfs_init\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gnome_vfs, SYMBOL_PREFIX "gnome_vfs_initialized", (handler_t *)&gnome_vfs_initialized_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gnome_vfs_initialized\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gnome_vfs, SYMBOL_PREFIX "gnome_vfs_get_mime_type_for_name", (handler_t *)&gnome_vfs_get_mime_type_for_name_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gnome_vfs_get_mime_type_for_name\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gnome_vfs, SYMBOL_PREFIX "gnome_vfs_mime_get_default_application", (handler_t *)&gnome_vfs_mime_get_default_application_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gnome_vfs_mime_get_default_application\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gnome_vfs, SYMBOL_PREFIX "gnome_vfs_mime_get_default_application_for_uri", (handler_t *)&gnome_vfs_mime_get_default_application_for_uri_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gnome_vfs_mime_get_default_application_for_uri\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gnome_vfs, SYMBOL_PREFIX "gnome_vfs_mime_application_get_exec", (handler_t *)&gnome_vfs_mime_application_get_exec_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gnome_vfs_mime_application_get_exec\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gnome_vfs, SYMBOL_PREFIX "gnome_vfs_mime_application_launch", (handler_t *)&gnome_vfs_mime_application_launch_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gnome_vfs_mime_application_launch\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gnome_vfs, SYMBOL_PREFIX "gnome_vfs_get_mime_type_from_uri", (handler_t *)&gnome_vfs_get_mime_type_from_uri_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gnome_vfs_get_mime_type_from_uri\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gnome_vfs, SYMBOL_PREFIX "gnome_vfs_uri_new", (handler_t *)&gnome_vfs_uri_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gnome_vfs_uri_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gnome_vfs, SYMBOL_PREFIX "gnome_vfs_make_uri_from_input", (handler_t *)&gnome_vfs_make_uri_from_input_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gnome_vfs_make_uri_from_input\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_gnome_vfs != NULL)
    module_unload(module_gnome_vfs);

  return 0;
}

int gnome_vfs_init(void)
{
  return gnome_vfs_init_ptr();
}

int gnome_vfs_initialized(void)
{
  return gnome_vfs_initialized_ptr();
}

void *gnome_vfs_get_mime_type_for_name(void *pArg1)
{
  return gnome_vfs_get_mime_type_for_name_ptr(pArg1);
}

void *gnome_vfs_mime_get_default_application(void *pArg1)
{
  return gnome_vfs_mime_get_default_application_ptr(pArg1);
}

void *gnome_vfs_mime_get_default_application_for_uri(void *pArg1, void *pArg2)
{
  return gnome_vfs_mime_get_default_application_for_uri_ptr(pArg1, pArg2);
}

void *gnome_vfs_mime_application_get_exec(void *pArg1)
{
  return gnome_vfs_mime_application_get_exec_ptr(pArg1);
}

int gnome_vfs_mime_application_launch(void *pArg1, void *pArg2)
{
  return gnome_vfs_mime_application_launch_ptr(pArg1, pArg2);
}

void *gnome_vfs_get_mime_type_from_uri(void *pArg1)
{
  return gnome_vfs_get_mime_type_from_uri_ptr(pArg1);
}

void *gnome_vfs_uri_new(void *pArg1)
{
  return gnome_vfs_uri_new_ptr(pArg1);
}

void *gnome_vfs_make_uri_from_input(void *pArg1)
{
  return gnome_vfs_make_uri_from_input_ptr(pArg1);
}

typedef void *(*pango_context_load_font_t)(void *pArg1, void *pArg2);
pango_context_load_font_t pango_context_load_font_ptr = NULL;
typedef void (*pango_context_list_families_t)(void *pArg1, void *pArg2, void *pArg3);
pango_context_list_families_t pango_context_list_families_ptr = NULL;
typedef void *(*pango_font_family_get_name_t)(void *pArg1);
pango_font_family_get_name_t pango_font_family_get_name_ptr = NULL;
typedef void (*pango_font_family_list_faces_t)(void *pArg1, void *pArg2, void *pArg3);
pango_font_family_list_faces_t pango_font_family_list_faces_ptr = NULL;
typedef void (*pango_font_face_list_sizes_t)(void *pArg1, void *pArg2, void *pArg3);
pango_font_face_list_sizes_t pango_font_face_list_sizes_ptr = NULL;
typedef void *(*pango_font_face_get_face_name_t)(void *pArg1);
pango_font_face_get_face_name_t pango_font_face_get_face_name_ptr = NULL;
typedef void *(*pango_layout_new_t)(void *pArg1);
pango_layout_new_t pango_layout_new_ptr = NULL;
typedef void (*pango_layout_set_text_t)(void *pArg1, void *pArg2, int pArg3);
pango_layout_set_text_t pango_layout_set_text_ptr = NULL;
typedef void *(*pango_layout_get_text_t)(void *pArg1);
pango_layout_get_text_t pango_layout_get_text_ptr = NULL;
typedef void (*pango_layout_set_font_description_t)(void *pArg1, void *pArg2);
pango_layout_set_font_description_t pango_layout_set_font_description_ptr = NULL;
typedef void (*pango_layout_get_pixel_extents_t)(void *pArg1, void *pArg2, void *pArg3);
pango_layout_get_pixel_extents_t pango_layout_get_pixel_extents_ptr = NULL;
typedef void *(*pango_layout_get_iter_t)(void *pArg1);
pango_layout_get_iter_t pango_layout_get_iter_ptr = NULL;
typedef void (*pango_layout_iter_free_t)(void *pArg1);
pango_layout_iter_free_t pango_layout_iter_free_ptr = NULL;
typedef void *(*pango_layout_iter_get_run_t)(void *pArg1);
pango_layout_iter_get_run_t pango_layout_iter_get_run_ptr = NULL;
typedef int (*pango_layout_iter_next_run_t)(void *pArg1);
pango_layout_iter_next_run_t pango_layout_iter_next_run_ptr = NULL;
typedef void *(*pango_layout_get_line_t)(void *pArg1, int pArg2);
pango_layout_get_line_t pango_layout_get_line_ptr = NULL;
typedef void (*pango_layout_line_get_pixel_extents_t)(void *pArg1, void *pArg2, void *pArg3);
pango_layout_line_get_pixel_extents_t pango_layout_line_get_pixel_extents_ptr = NULL;
typedef void (*pango_layout_get_line_readonly_t)(void);
pango_layout_get_line_readonly_t pango_layout_get_line_readonly_ptr = NULL;
typedef void *(*pango_font_description_from_string_t)(void *pArg1);
pango_font_description_from_string_t pango_font_description_from_string_ptr = NULL;
typedef void *(*pango_font_description_new_t)(void);
pango_font_description_new_t pango_font_description_new_ptr = NULL;
typedef void (*pango_font_description_free_t)(void *pArg1);
pango_font_description_free_t pango_font_description_free_ptr = NULL;
typedef void (*pango_font_description_set_family_t)(void *pArg1, void *pArg2);
pango_font_description_set_family_t pango_font_description_set_family_ptr = NULL;
typedef void (*pango_font_description_set_size_t)(void *pArg1, int pArg2);
pango_font_description_set_size_t pango_font_description_set_size_ptr = NULL;
typedef void (*pango_font_description_set_absolute_size_t)(void *pArg1, double pArg2);
pango_font_description_set_absolute_size_t pango_font_description_set_absolute_size_ptr = NULL;
typedef void (*pango_font_description_set_style_t)(void *pArg1, int pArg2);
pango_font_description_set_style_t pango_font_description_set_style_ptr = NULL;
typedef void (*pango_font_description_set_weight_t)(void *pArg1, int pArg2);
pango_font_description_set_weight_t pango_font_description_set_weight_ptr = NULL;
typedef void (*pango_font_description_set_stretch_t)(void *pArg1, int pArg2);
pango_font_description_set_stretch_t pango_font_description_set_stretch_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_PANGO_NAME ""
#elif defined(_LINUX)
#define MODULE_PANGO_NAME "libpango-1.0.so.0"
#elif defined(_WINDOWS)
#define MODULE_PANGO_NAME ""
#endif

static module_t module_pango = NULL;

int initialise_weak_link_pango(void)
{
#if defined(_LINUX)
  if (!module_load("libpango-1.0.so.0", &module_pango))
#else
  if (!module_load(MODULE_PANGO_NAME, &module_pango))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libpango-1.0.so.0\n") ;
#endif
goto err;
}

  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_context_load_font", (handler_t *)&pango_context_load_font_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_context_load_font\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_context_list_families", (handler_t *)&pango_context_list_families_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_context_list_families\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_font_family_get_name", (handler_t *)&pango_font_family_get_name_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_font_family_get_name\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_font_family_list_faces", (handler_t *)&pango_font_family_list_faces_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_font_family_list_faces\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_font_face_list_sizes", (handler_t *)&pango_font_face_list_sizes_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_font_face_list_sizes\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_font_face_get_face_name", (handler_t *)&pango_font_face_get_face_name_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_font_face_get_face_name\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_layout_new", (handler_t *)&pango_layout_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_layout_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_layout_set_text", (handler_t *)&pango_layout_set_text_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_layout_set_text\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_layout_get_text", (handler_t *)&pango_layout_get_text_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_layout_get_text\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_layout_set_font_description", (handler_t *)&pango_layout_set_font_description_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_layout_set_font_description\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_layout_get_pixel_extents", (handler_t *)&pango_layout_get_pixel_extents_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_layout_get_pixel_extents\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_layout_get_iter", (handler_t *)&pango_layout_get_iter_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_layout_get_iter\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_layout_iter_free", (handler_t *)&pango_layout_iter_free_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_layout_iter_free\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_layout_iter_get_run", (handler_t *)&pango_layout_iter_get_run_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_layout_iter_get_run\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_layout_iter_next_run", (handler_t *)&pango_layout_iter_next_run_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_layout_iter_next_run\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_layout_get_line", (handler_t *)&pango_layout_get_line_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_layout_get_line\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_layout_line_get_pixel_extents", (handler_t *)&pango_layout_line_get_pixel_extents_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_layout_line_get_pixel_extents\n"); 
#endif
goto err; 
}
module_resolve(module_pango, SYMBOL_PREFIX "pango_layout_get_line_readonly", (handler_t *)&pango_layout_get_line_readonly_ptr);
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_font_description_from_string", (handler_t *)&pango_font_description_from_string_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_font_description_from_string\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_font_description_new", (handler_t *)&pango_font_description_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_font_description_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_font_description_free", (handler_t *)&pango_font_description_free_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_font_description_free\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_font_description_set_family", (handler_t *)&pango_font_description_set_family_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_font_description_set_family\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_font_description_set_size", (handler_t *)&pango_font_description_set_size_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_font_description_set_size\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_font_description_set_absolute_size", (handler_t *)&pango_font_description_set_absolute_size_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_font_description_set_absolute_size\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_font_description_set_style", (handler_t *)&pango_font_description_set_style_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_font_description_set_style\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_font_description_set_weight", (handler_t *)&pango_font_description_set_weight_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_font_description_set_weight\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pango, SYMBOL_PREFIX "pango_font_description_set_stretch", (handler_t *)&pango_font_description_set_stretch_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_font_description_set_stretch\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_pango != NULL)
    module_unload(module_pango);

  return 0;
}

void *pango_context_load_font(void *pArg1, void *pArg2)
{
  return pango_context_load_font_ptr(pArg1, pArg2);
}

void pango_context_list_families(void *pArg1, void *pArg2, void *pArg3)
{
  pango_context_list_families_ptr(pArg1, pArg2, pArg3);
}

void *pango_font_family_get_name(void *pArg1)
{
  return pango_font_family_get_name_ptr(pArg1);
}

void pango_font_family_list_faces(void *pArg1, void *pArg2, void *pArg3)
{
  pango_font_family_list_faces_ptr(pArg1, pArg2, pArg3);
}

void pango_font_face_list_sizes(void *pArg1, void *pArg2, void *pArg3)
{
  pango_font_face_list_sizes_ptr(pArg1, pArg2, pArg3);
}

void *pango_font_face_get_face_name(void *pArg1)
{
  return pango_font_face_get_face_name_ptr(pArg1);
}

void *pango_layout_new(void *pArg1)
{
  return pango_layout_new_ptr(pArg1);
}

void pango_layout_set_text(void *pArg1, void *pArg2, int pArg3)
{
  pango_layout_set_text_ptr(pArg1, pArg2, pArg3);
}

void *pango_layout_get_text(void *pArg1)
{
  return pango_layout_get_text_ptr(pArg1);
}

void pango_layout_set_font_description(void *pArg1, void *pArg2)
{
  pango_layout_set_font_description_ptr(pArg1, pArg2);
}

void pango_layout_get_pixel_extents(void *pArg1, void *pArg2, void *pArg3)
{
  pango_layout_get_pixel_extents_ptr(pArg1, pArg2, pArg3);
}

void *pango_layout_get_iter(void *pArg1)
{
  return pango_layout_get_iter_ptr(pArg1);
}

void pango_layout_iter_free(void *pArg1)
{
  pango_layout_iter_free_ptr(pArg1);
}

void *pango_layout_iter_get_run(void *pArg1)
{
  return pango_layout_iter_get_run_ptr(pArg1);
}

int pango_layout_iter_next_run(void *pArg1)
{
  return pango_layout_iter_next_run_ptr(pArg1);
}

void *pango_layout_get_line(void *pArg1, int pArg2)
{
  return pango_layout_get_line_ptr(pArg1, pArg2);
}

void pango_layout_line_get_pixel_extents(void *pArg1, void *pArg2, void *pArg3)
{
  pango_layout_line_get_pixel_extents_ptr(pArg1, pArg2, pArg3);
}

void pango_layout_get_line_readonly(void)
{
  pango_layout_get_line_readonly_ptr();
}

void *pango_font_description_from_string(void *pArg1)
{
  return pango_font_description_from_string_ptr(pArg1);
}

void *pango_font_description_new(void)
{
  return pango_font_description_new_ptr();
}

void pango_font_description_free(void *pArg1)
{
  pango_font_description_free_ptr(pArg1);
}

void pango_font_description_set_family(void *pArg1, void *pArg2)
{
  pango_font_description_set_family_ptr(pArg1, pArg2);
}

void pango_font_description_set_size(void *pArg1, int pArg2)
{
  pango_font_description_set_size_ptr(pArg1, pArg2);
}

void pango_font_description_set_absolute_size(void *pArg1, double pArg2)
{
  pango_font_description_set_absolute_size_ptr(pArg1, pArg2);
}

void pango_font_description_set_style(void *pArg1, int pArg2)
{
  pango_font_description_set_style_ptr(pArg1, pArg2);
}

void pango_font_description_set_weight(void *pArg1, int pArg2)
{
  pango_font_description_set_weight_ptr(pArg1, pArg2);
}

void pango_font_description_set_stretch(void *pArg1, int pArg2)
{
  pango_font_description_set_stretch_ptr(pArg1, pArg2);
}

typedef void *(*gdk_drawable_get_image_t)(void *pArg1, int pArg2, int pArg3, int pArg4, int pArg5);
gdk_drawable_get_image_t gdk_drawable_get_image_ptr = NULL;
typedef void (*gdk_draw_rectangle_t)(void *pArg1, void *pArg2, int pArg3, int pArg4, int pArg5, int pArg6, int pArg7);
gdk_draw_rectangle_t gdk_draw_rectangle_ptr = NULL;
typedef void (*gdk_gc_set_ts_origin_t)(void *pArg1, int pArg2, int pArg3);
gdk_gc_set_ts_origin_t gdk_gc_set_ts_origin_ptr = NULL;
typedef void *(*gdk_pixmap_new_t)(void *pArg1, int pArg2, int pArg3, int pArg4);
gdk_pixmap_new_t gdk_pixmap_new_ptr = NULL;
typedef void (*gdk_window_clear_area_t)(void *pArg1, int pArg2, int pArg3, int pArg4, int pArg5);
gdk_window_clear_area_t gdk_window_clear_area_ptr = NULL;
typedef int (*gdk_window_is_visible_t)(void *pArg1);
gdk_window_is_visible_t gdk_window_is_visible_ptr = NULL;
typedef void (*gdk_window_set_back_pixmap_t)(void *pArg1, void *pArg2, int pArg3);
gdk_window_set_back_pixmap_t gdk_window_set_back_pixmap_ptr = NULL;
typedef void *(*gdk_display_get_default_t)(void);
gdk_display_get_default_t gdk_display_get_default_ptr = NULL;
typedef int (*gdk_x11_drawable_get_xid_t)(void *pArg1);
gdk_x11_drawable_get_xid_t gdk_x11_drawable_get_xid_ptr = NULL;
typedef void *(*gdk_x11_image_get_ximage_t)(void *pArg1);
gdk_x11_image_get_ximage_t gdk_x11_image_get_ximage_ptr = NULL;
typedef void *(*gdk_x11_display_get_xdisplay_t)(void *pArg1);
gdk_x11_display_get_xdisplay_t gdk_x11_display_get_xdisplay_ptr = NULL;
typedef void (*gdk_error_trap_push_t)(void);
gdk_error_trap_push_t gdk_error_trap_push_ptr = NULL;
typedef int (*gdk_error_trap_pop_t)(void);
gdk_error_trap_pop_t gdk_error_trap_pop_ptr = NULL;
typedef void (*gdk_flush_t)(void);
gdk_flush_t gdk_flush_ptr = NULL;
typedef int (*gdk_window_object_get_type_t)(void);
gdk_window_object_get_type_t gdk_window_object_get_type_ptr = NULL;
typedef void *(*gdk_event_get_t)(void);
gdk_event_get_t gdk_event_get_ptr = NULL;
typedef void *(*gdk_colormap_new_t)(void *pArg1, int pArg2);
gdk_colormap_new_t gdk_colormap_new_ptr = NULL;
typedef void *(*gdk_visual_get_best_with_depth_t)(int pArg1);
gdk_visual_get_best_with_depth_t gdk_visual_get_best_with_depth_ptr = NULL;
typedef void (*gdk_drawable_set_colormap_t)(void *pArg1, void *pArg2);
gdk_drawable_set_colormap_t gdk_drawable_set_colormap_ptr = NULL;
typedef void *(*gdk_gc_new_t)(void *pArg1);
gdk_gc_new_t gdk_gc_new_ptr = NULL;
typedef void (*gdk_gc_set_colormap_t)(void *pArg1, void *pArg2);
gdk_gc_set_colormap_t gdk_gc_set_colormap_ptr = NULL;
typedef void (*gdk_gc_set_rgb_fg_color_t)(void *pArg1, void *pArg2);
gdk_gc_set_rgb_fg_color_t gdk_gc_set_rgb_fg_color_ptr = NULL;
typedef int (*gdk_rectangle_intersect_t)(void *pArg1, void *pArg2, void *pArg3);
gdk_rectangle_intersect_t gdk_rectangle_intersect_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_GDK_NAME ""
#elif defined(_LINUX)
#define MODULE_GDK_NAME "libgdk-x11-2.0.so.0"
#elif defined(_WINDOWS)
#define MODULE_GDK_NAME ""
#endif

static module_t module_gdk = NULL;

int initialise_weak_link_gdk(void)
{
#if defined(_LINUX)
  if (!module_load("libgdk-x11-2.0.so.0", &module_gdk))
#else
  if (!module_load(MODULE_GDK_NAME, &module_gdk))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libgdk-x11-2.0.so.0\n") ;
#endif
goto err;
}

  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_drawable_get_image", (handler_t *)&gdk_drawable_get_image_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_drawable_get_image\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_draw_rectangle", (handler_t *)&gdk_draw_rectangle_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_draw_rectangle\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_gc_set_ts_origin", (handler_t *)&gdk_gc_set_ts_origin_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_gc_set_ts_origin\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_pixmap_new", (handler_t *)&gdk_pixmap_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_pixmap_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_window_clear_area", (handler_t *)&gdk_window_clear_area_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_window_clear_area\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_window_is_visible", (handler_t *)&gdk_window_is_visible_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_window_is_visible\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_window_set_back_pixmap", (handler_t *)&gdk_window_set_back_pixmap_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_window_set_back_pixmap\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_display_get_default", (handler_t *)&gdk_display_get_default_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_display_get_default\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_x11_drawable_get_xid", (handler_t *)&gdk_x11_drawable_get_xid_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_x11_drawable_get_xid\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_x11_image_get_ximage", (handler_t *)&gdk_x11_image_get_ximage_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_x11_image_get_ximage\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_x11_display_get_xdisplay", (handler_t *)&gdk_x11_display_get_xdisplay_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_x11_display_get_xdisplay\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_error_trap_push", (handler_t *)&gdk_error_trap_push_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_error_trap_push\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_error_trap_pop", (handler_t *)&gdk_error_trap_pop_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_error_trap_pop\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_flush", (handler_t *)&gdk_flush_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_flush\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_window_object_get_type", (handler_t *)&gdk_window_object_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_window_object_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_event_get", (handler_t *)&gdk_event_get_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_event_get\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_colormap_new", (handler_t *)&gdk_colormap_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_colormap_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_visual_get_best_with_depth", (handler_t *)&gdk_visual_get_best_with_depth_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_visual_get_best_with_depth\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_drawable_set_colormap", (handler_t *)&gdk_drawable_set_colormap_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_drawable_set_colormap\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_gc_new", (handler_t *)&gdk_gc_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_gc_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_gc_set_colormap", (handler_t *)&gdk_gc_set_colormap_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_gc_set_colormap\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_gc_set_rgb_fg_color", (handler_t *)&gdk_gc_set_rgb_fg_color_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_gc_set_rgb_fg_color\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gdk, SYMBOL_PREFIX "gdk_rectangle_intersect", (handler_t *)&gdk_rectangle_intersect_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_rectangle_intersect\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_gdk != NULL)
    module_unload(module_gdk);

  return 0;
}

void *gdk_drawable_get_image(void *pArg1, int pArg2, int pArg3, int pArg4, int pArg5)
{
  return gdk_drawable_get_image_ptr(pArg1, pArg2, pArg3, pArg4, pArg5);
}

void gdk_draw_rectangle(void *pArg1, void *pArg2, int pArg3, int pArg4, int pArg5, int pArg6, int pArg7)
{
  gdk_draw_rectangle_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7);
}

void gdk_gc_set_ts_origin(void *pArg1, int pArg2, int pArg3)
{
  gdk_gc_set_ts_origin_ptr(pArg1, pArg2, pArg3);
}

void *gdk_pixmap_new(void *pArg1, int pArg2, int pArg3, int pArg4)
{
  return gdk_pixmap_new_ptr(pArg1, pArg2, pArg3, pArg4);
}

void gdk_window_clear_area(void *pArg1, int pArg2, int pArg3, int pArg4, int pArg5)
{
  gdk_window_clear_area_ptr(pArg1, pArg2, pArg3, pArg4, pArg5);
}

int gdk_window_is_visible(void *pArg1)
{
  return gdk_window_is_visible_ptr(pArg1);
}

void gdk_window_set_back_pixmap(void *pArg1, void *pArg2, int pArg3)
{
  gdk_window_set_back_pixmap_ptr(pArg1, pArg2, pArg3);
}

void *gdk_display_get_default(void)
{
  return gdk_display_get_default_ptr();
}

int gdk_x11_drawable_get_xid(void *pArg1)
{
  return gdk_x11_drawable_get_xid_ptr(pArg1);
}

void *gdk_x11_image_get_ximage(void *pArg1)
{
  return gdk_x11_image_get_ximage_ptr(pArg1);
}

void *gdk_x11_display_get_xdisplay(void *pArg1)
{
  return gdk_x11_display_get_xdisplay_ptr(pArg1);
}

void gdk_error_trap_push(void)
{
  gdk_error_trap_push_ptr();
}

int gdk_error_trap_pop(void)
{
  return gdk_error_trap_pop_ptr();
}

void gdk_flush(void)
{
  gdk_flush_ptr();
}

int gdk_window_object_get_type(void)
{
  return gdk_window_object_get_type_ptr();
}

void *gdk_event_get(void)
{
  return gdk_event_get_ptr();
}

void *gdk_colormap_new(void *pArg1, int pArg2)
{
  return gdk_colormap_new_ptr(pArg1, pArg2);
}

void *gdk_visual_get_best_with_depth(int pArg1)
{
  return gdk_visual_get_best_with_depth_ptr(pArg1);
}

void gdk_drawable_set_colormap(void *pArg1, void *pArg2)
{
  gdk_drawable_set_colormap_ptr(pArg1, pArg2);
}

void *gdk_gc_new(void *pArg1)
{
  return gdk_gc_new_ptr(pArg1);
}

void gdk_gc_set_colormap(void *pArg1, void *pArg2)
{
  gdk_gc_set_colormap_ptr(pArg1, pArg2);
}

void gdk_gc_set_rgb_fg_color(void *pArg1, void *pArg2)
{
  gdk_gc_set_rgb_fg_color_ptr(pArg1, pArg2);
}

int gdk_rectangle_intersect(void *pArg1, void *pArg2, void *pArg3)
{
  return gdk_rectangle_intersect_ptr(pArg1, pArg2, pArg3);
}

typedef void *(*cmsOpenProfileFromFile_t)(void *pArg1, void *pArg2);
cmsOpenProfileFromFile_t cmsOpenProfileFromFile_ptr = NULL;
typedef void *(*cmsOpenProfileFromMem_t)(void *pArg1, int pArg2);
cmsOpenProfileFromMem_t cmsOpenProfileFromMem_ptr = NULL;
typedef int (*cmsCloseProfile_t)(void *pArg1);
cmsCloseProfile_t cmsCloseProfile_ptr = NULL;
typedef void *(*cmsCreateRGBProfile_t)(void *pArg1, void *pArg2, void *pArg3);
cmsCreateRGBProfile_t cmsCreateRGBProfile_ptr = NULL;
typedef void *(*cmsCreate_sRGBProfile_t)(void);
cmsCreate_sRGBProfile_t cmsCreate_sRGBProfile_ptr = NULL;
typedef void *(*cmsCreateTransform_t)(void *pArg1, int pArg2, void *pArg3, int pArg4, int pArg5, int pArg6);
cmsCreateTransform_t cmsCreateTransform_ptr = NULL;
typedef void (*cmsDeleteTransform_t)(void *pArg1);
cmsDeleteTransform_t cmsDeleteTransform_ptr = NULL;
typedef void (*cmsDoTransform_t)(void *pArg1, void *pArg2, void *pArg3, int pArg4);
cmsDoTransform_t cmsDoTransform_ptr = NULL;
typedef int (*cmsGetColorSpace_t)(void *pArg1);
cmsGetColorSpace_t cmsGetColorSpace_ptr = NULL;
typedef void *(*cmsBuildGamma_t)(int pArg1, double pArg2);
cmsBuildGamma_t cmsBuildGamma_ptr = NULL;
typedef void (*cmsFreeGamma_t)(void *pArg1);
cmsFreeGamma_t cmsFreeGamma_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_LCMS_NAME ""
#elif defined(_LINUX)
#define MODULE_LCMS_NAME "liblcms.so.1"
#elif defined(_WINDOWS)
#define MODULE_LCMS_NAME ""
#endif

static module_t module_lcms = NULL;

int initialise_weak_link_lcms(void)
{
#if defined(_LINUX)
  if (!module_load("liblcms.so.1", &module_lcms))
#else
  if (!module_load(MODULE_LCMS_NAME, &module_lcms))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: liblcms.so.1\n") ;
#endif
goto err;
}

  if (!module_resolve(module_lcms, SYMBOL_PREFIX "cmsOpenProfileFromFile", (handler_t *)&cmsOpenProfileFromFile_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: cmsOpenProfileFromFile\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_lcms, SYMBOL_PREFIX "cmsOpenProfileFromMem", (handler_t *)&cmsOpenProfileFromMem_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: cmsOpenProfileFromMem\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_lcms, SYMBOL_PREFIX "cmsCloseProfile", (handler_t *)&cmsCloseProfile_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: cmsCloseProfile\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_lcms, SYMBOL_PREFIX "cmsCreateRGBProfile", (handler_t *)&cmsCreateRGBProfile_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: cmsCreateRGBProfile\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_lcms, SYMBOL_PREFIX "cmsCreate_sRGBProfile", (handler_t *)&cmsCreate_sRGBProfile_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: cmsCreate_sRGBProfile\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_lcms, SYMBOL_PREFIX "cmsCreateTransform", (handler_t *)&cmsCreateTransform_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: cmsCreateTransform\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_lcms, SYMBOL_PREFIX "cmsDeleteTransform", (handler_t *)&cmsDeleteTransform_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: cmsDeleteTransform\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_lcms, SYMBOL_PREFIX "cmsDoTransform", (handler_t *)&cmsDoTransform_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: cmsDoTransform\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_lcms, SYMBOL_PREFIX "cmsGetColorSpace", (handler_t *)&cmsGetColorSpace_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: cmsGetColorSpace\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_lcms, SYMBOL_PREFIX "cmsBuildGamma", (handler_t *)&cmsBuildGamma_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: cmsBuildGamma\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_lcms, SYMBOL_PREFIX "cmsFreeGamma", (handler_t *)&cmsFreeGamma_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: cmsFreeGamma\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_lcms != NULL)
    module_unload(module_lcms);

  return 0;
}

void *cmsOpenProfileFromFile(void *pArg1, void *pArg2)
{
  return cmsOpenProfileFromFile_ptr(pArg1, pArg2);
}

void *cmsOpenProfileFromMem(void *pArg1, int pArg2)
{
  return cmsOpenProfileFromMem_ptr(pArg1, pArg2);
}

int cmsCloseProfile(void *pArg1)
{
  return cmsCloseProfile_ptr(pArg1);
}

void *cmsCreateRGBProfile(void *pArg1, void *pArg2, void *pArg3)
{
  return cmsCreateRGBProfile_ptr(pArg1, pArg2, pArg3);
}

void *cmsCreate_sRGBProfile(void)
{
  return cmsCreate_sRGBProfile_ptr();
}

void *cmsCreateTransform(void *pArg1, int pArg2, void *pArg3, int pArg4, int pArg5, int pArg6)
{
  return cmsCreateTransform_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6);
}

void cmsDeleteTransform(void *pArg1)
{
  cmsDeleteTransform_ptr(pArg1);
}

void cmsDoTransform(void *pArg1, void *pArg2, void *pArg3, int pArg4)
{
  cmsDoTransform_ptr(pArg1, pArg2, pArg3, pArg4);
}

int cmsGetColorSpace(void *pArg1)
{
  return cmsGetColorSpace_ptr(pArg1);
}

void *cmsBuildGamma(int pArg1, double pArg2)
{
  return cmsBuildGamma_ptr(pArg1, pArg2);
}

void cmsFreeGamma(void *pArg1)
{
  cmsFreeGamma_ptr(pArg1);
}

typedef void *(*gtk_print_unix_dialog_new_t)(void *pArg1, void *pArg2);
gtk_print_unix_dialog_new_t gtk_print_unix_dialog_new_ptr = NULL;
typedef void *(*gtk_print_unix_dialog_get_selected_printer_t)(void *pArg1);
gtk_print_unix_dialog_get_selected_printer_t gtk_print_unix_dialog_get_selected_printer_ptr = NULL;
typedef void *(*gtk_print_unix_dialog_get_settings_t)(void *pArg1);
gtk_print_unix_dialog_get_settings_t gtk_print_unix_dialog_get_settings_ptr = NULL;
typedef int (*gtk_print_settings_get_n_copies_t)(void *pArg1);
gtk_print_settings_get_n_copies_t gtk_print_settings_get_n_copies_ptr = NULL;
typedef int (*gtk_print_settings_get_collate_t)(void *pArg1);
gtk_print_settings_get_collate_t gtk_print_settings_get_collate_ptr = NULL;
typedef double (*gtk_print_settings_get_paper_width_t)(void *pArg1);
gtk_print_settings_get_paper_width_t gtk_print_settings_get_paper_width_ptr = NULL;
typedef double (*gtk_print_settings_get_paper_height_t)(void *pArg1);
gtk_print_settings_get_paper_height_t gtk_print_settings_get_paper_height_ptr = NULL;
typedef int (*gtk_print_settings_get_use_color_t)(void *pArg1);
gtk_print_settings_get_use_color_t gtk_print_settings_get_use_color_ptr = NULL;
typedef int (*gtk_print_settings_get_orientation_t)(void *pArg1);
gtk_print_settings_get_orientation_t gtk_print_settings_get_orientation_ptr = NULL;
typedef int (*gtk_print_settings_get_duplex_t)(void *pArg1);
gtk_print_settings_get_duplex_t gtk_print_settings_get_duplex_ptr = NULL;
typedef void *(*gtk_page_setup_unix_dialog_new_t)(void *pArg1, void *pArg2);
gtk_page_setup_unix_dialog_new_t gtk_page_setup_unix_dialog_new_ptr = NULL;
typedef void *(*gtk_page_setup_unix_dialog_get_page_setup_t)(void *pArg1);
gtk_page_setup_unix_dialog_get_page_setup_t gtk_page_setup_unix_dialog_get_page_setup_ptr = NULL;
typedef double (*gtk_page_setup_get_paper_width_t)(void *pArg1, int pArg2);
gtk_page_setup_get_paper_width_t gtk_page_setup_get_paper_width_ptr = NULL;
typedef double (*gtk_page_setup_get_paper_height_t)(void *pArg1, int pArg2);
gtk_page_setup_get_paper_height_t gtk_page_setup_get_paper_height_ptr = NULL;
typedef int (*gtk_page_setup_get_orientation_t)(void *pArg1);
gtk_page_setup_get_orientation_t gtk_page_setup_get_orientation_ptr = NULL;
typedef void *(*gtk_printer_get_name_t)(void *pArg1);
gtk_printer_get_name_t gtk_printer_get_name_ptr = NULL;
typedef int (*gtk_print_unix_dialog_get_type_t)(void);
gtk_print_unix_dialog_get_type_t gtk_print_unix_dialog_get_type_ptr = NULL;
typedef int (*gtk_page_setup_unix_dialog_get_type_t)(void);
gtk_page_setup_unix_dialog_get_type_t gtk_page_setup_unix_dialog_get_type_ptr = NULL;
typedef void *(*gtk_print_settings_get_output_bin_t)(void *pArg1);
gtk_print_settings_get_output_bin_t gtk_print_settings_get_output_bin_ptr = NULL;
typedef void *(*gtk_print_settings_get_t)(void *pArg1, void *pArg2);
gtk_print_settings_get_t gtk_print_settings_get_ptr = NULL;
typedef void *(*gtk_print_settings_get_page_ranges_t)(void *pArg1, void *pArg2);
gtk_print_settings_get_page_ranges_t gtk_print_settings_get_page_ranges_ptr = NULL;
typedef void (*gtk_print_unix_dialog_set_manual_capabilities_t)(void *pArg1, int pArg2);
gtk_print_unix_dialog_set_manual_capabilities_t gtk_print_unix_dialog_set_manual_capabilities_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_GTK_PRINT_DIALOG_NAME ""
#elif defined(_LINUX)
#define MODULE_GTK_PRINT_DIALOG_NAME "libgtk-x11-2.0.so.0"
#elif defined(_WINDOWS)
#define MODULE_GTK_PRINT_DIALOG_NAME ""
#endif

static module_t module_gtk_print_dialog = NULL;

int initialise_weak_link_gtk_print_dialog(void)
{
#if defined(_LINUX)
  if (!module_load("libgtk-x11-2.0.so.0", &module_gtk_print_dialog))
#else
  if (!module_load(MODULE_GTK_PRINT_DIALOG_NAME, &module_gtk_print_dialog))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libgtk-x11-2.0.so.0\n") ;
#endif
goto err;
}

  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_unix_dialog_new", (handler_t *)&gtk_print_unix_dialog_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_unix_dialog_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_unix_dialog_get_selected_printer", (handler_t *)&gtk_print_unix_dialog_get_selected_printer_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_unix_dialog_get_selected_printer\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_unix_dialog_get_settings", (handler_t *)&gtk_print_unix_dialog_get_settings_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_unix_dialog_get_settings\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_settings_get_n_copies", (handler_t *)&gtk_print_settings_get_n_copies_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_settings_get_n_copies\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_settings_get_collate", (handler_t *)&gtk_print_settings_get_collate_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_settings_get_collate\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_settings_get_paper_width", (handler_t *)&gtk_print_settings_get_paper_width_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_settings_get_paper_width\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_settings_get_paper_height", (handler_t *)&gtk_print_settings_get_paper_height_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_settings_get_paper_height\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_settings_get_use_color", (handler_t *)&gtk_print_settings_get_use_color_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_settings_get_use_color\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_settings_get_orientation", (handler_t *)&gtk_print_settings_get_orientation_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_settings_get_orientation\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_settings_get_duplex", (handler_t *)&gtk_print_settings_get_duplex_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_settings_get_duplex\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_page_setup_unix_dialog_new", (handler_t *)&gtk_page_setup_unix_dialog_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_page_setup_unix_dialog_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_page_setup_unix_dialog_get_page_setup", (handler_t *)&gtk_page_setup_unix_dialog_get_page_setup_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_page_setup_unix_dialog_get_page_setup\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_page_setup_get_paper_width", (handler_t *)&gtk_page_setup_get_paper_width_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_page_setup_get_paper_width\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_page_setup_get_paper_height", (handler_t *)&gtk_page_setup_get_paper_height_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_page_setup_get_paper_height\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_page_setup_get_orientation", (handler_t *)&gtk_page_setup_get_orientation_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_page_setup_get_orientation\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_printer_get_name", (handler_t *)&gtk_printer_get_name_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_printer_get_name\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_unix_dialog_get_type", (handler_t *)&gtk_print_unix_dialog_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_unix_dialog_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_page_setup_unix_dialog_get_type", (handler_t *)&gtk_page_setup_unix_dialog_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_page_setup_unix_dialog_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_settings_get_output_bin", (handler_t *)&gtk_print_settings_get_output_bin_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_settings_get_output_bin\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_settings_get", (handler_t *)&gtk_print_settings_get_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_settings_get\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_settings_get_page_ranges", (handler_t *)&gtk_print_settings_get_page_ranges_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_settings_get_page_ranges\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_print_dialog, SYMBOL_PREFIX "gtk_print_unix_dialog_set_manual_capabilities", (handler_t *)&gtk_print_unix_dialog_set_manual_capabilities_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_print_unix_dialog_set_manual_capabilities\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_gtk_print_dialog != NULL)
    module_unload(module_gtk_print_dialog);

  return 0;
}

void *gtk_print_unix_dialog_new(void *pArg1, void *pArg2)
{
  return gtk_print_unix_dialog_new_ptr(pArg1, pArg2);
}

void *gtk_print_unix_dialog_get_selected_printer(void *pArg1)
{
  return gtk_print_unix_dialog_get_selected_printer_ptr(pArg1);
}

void *gtk_print_unix_dialog_get_settings(void *pArg1)
{
  return gtk_print_unix_dialog_get_settings_ptr(pArg1);
}

int gtk_print_settings_get_n_copies(void *pArg1)
{
  return gtk_print_settings_get_n_copies_ptr(pArg1);
}

int gtk_print_settings_get_collate(void *pArg1)
{
  return gtk_print_settings_get_collate_ptr(pArg1);
}

double gtk_print_settings_get_paper_width(void *pArg1)
{
  return gtk_print_settings_get_paper_width_ptr(pArg1);
}

double gtk_print_settings_get_paper_height(void *pArg1)
{
  return gtk_print_settings_get_paper_height_ptr(pArg1);
}

int gtk_print_settings_get_use_color(void *pArg1)
{
  return gtk_print_settings_get_use_color_ptr(pArg1);
}

int gtk_print_settings_get_orientation(void *pArg1)
{
  return gtk_print_settings_get_orientation_ptr(pArg1);
}

int gtk_print_settings_get_duplex(void *pArg1)
{
  return gtk_print_settings_get_duplex_ptr(pArg1);
}

void *gtk_page_setup_unix_dialog_new(void *pArg1, void *pArg2)
{
  return gtk_page_setup_unix_dialog_new_ptr(pArg1, pArg2);
}

void *gtk_page_setup_unix_dialog_get_page_setup(void *pArg1)
{
  return gtk_page_setup_unix_dialog_get_page_setup_ptr(pArg1);
}

double gtk_page_setup_get_paper_width(void *pArg1, int pArg2)
{
  return gtk_page_setup_get_paper_width_ptr(pArg1, pArg2);
}

double gtk_page_setup_get_paper_height(void *pArg1, int pArg2)
{
  return gtk_page_setup_get_paper_height_ptr(pArg1, pArg2);
}

int gtk_page_setup_get_orientation(void *pArg1)
{
  return gtk_page_setup_get_orientation_ptr(pArg1);
}

void *gtk_printer_get_name(void *pArg1)
{
  return gtk_printer_get_name_ptr(pArg1);
}

int gtk_print_unix_dialog_get_type(void)
{
  return gtk_print_unix_dialog_get_type_ptr();
}

int gtk_page_setup_unix_dialog_get_type(void)
{
  return gtk_page_setup_unix_dialog_get_type_ptr();
}

void *gtk_print_settings_get_output_bin(void *pArg1)
{
  return gtk_print_settings_get_output_bin_ptr(pArg1);
}

void *gtk_print_settings_get(void *pArg1, void *pArg2)
{
  return gtk_print_settings_get_ptr(pArg1, pArg2);
}

void *gtk_print_settings_get_page_ranges(void *pArg1, void *pArg2)
{
  return gtk_print_settings_get_page_ranges_ptr(pArg1, pArg2);
}

void gtk_print_unix_dialog_set_manual_capabilities(void *pArg1, int pArg2)
{
  gtk_print_unix_dialog_set_manual_capabilities_ptr(pArg1, pArg2);
}

typedef void (*pango_fc_font_lock_face_t)(void *pArg1);
pango_fc_font_lock_face_t pango_fc_font_lock_face_ptr = NULL;
typedef void (*pango_fc_font_unlock_face_t)(void *pArg1);
pango_fc_font_unlock_face_t pango_fc_font_unlock_face_ptr = NULL;
typedef int (*pango_fc_font_get_type_t)(void *pArg1);
pango_fc_font_get_type_t pango_fc_font_get_type_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_PANGOFT2_NAME ""
#elif defined(_LINUX)
#define MODULE_PANGOFT2_NAME "libpangoft2-1.0.so.0"
#elif defined(_WINDOWS)
#define MODULE_PANGOFT2_NAME ""
#endif

static module_t module_pangoft2 = NULL;

int initialise_weak_link_pangoft2(void)
{
#if defined(_LINUX)
  if (!module_load("libpangoft2-1.0.so.0", &module_pangoft2))
#else
  if (!module_load(MODULE_PANGOFT2_NAME, &module_pangoft2))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libpangoft2-1.0.so.0\n") ;
#endif
goto err;
}

  if (!module_resolve(module_pangoft2, SYMBOL_PREFIX "pango_fc_font_lock_face", (handler_t *)&pango_fc_font_lock_face_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_fc_font_lock_face\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pangoft2, SYMBOL_PREFIX "pango_fc_font_unlock_face", (handler_t *)&pango_fc_font_unlock_face_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_fc_font_unlock_face\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pangoft2, SYMBOL_PREFIX "pango_fc_font_get_type", (handler_t *)&pango_fc_font_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_fc_font_get_type\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_pangoft2 != NULL)
    module_unload(module_pangoft2);

  return 0;
}

void pango_fc_font_lock_face(void *pArg1)
{
  pango_fc_font_lock_face_ptr(pArg1);
}

void pango_fc_font_unlock_face(void *pArg1)
{
  pango_fc_font_unlock_face_ptr(pArg1);
}

int pango_fc_font_get_type(void *pArg1)
{
  return pango_fc_font_get_type_ptr(pArg1);
}

typedef void *(*gtk_file_filter_new_t)(void);
gtk_file_filter_new_t gtk_file_filter_new_ptr = NULL;
typedef void (*gtk_file_filter_set_name_t)(void *pArg1, void *pArg2);
gtk_file_filter_set_name_t gtk_file_filter_set_name_ptr = NULL;
typedef void (*gtk_file_filter_add_pattern_t)(void *pArg1, void *pArg2);
gtk_file_filter_add_pattern_t gtk_file_filter_add_pattern_ptr = NULL;
typedef void *(*gtk_file_filter_get_name_t)(void *pArg1);
gtk_file_filter_get_name_t gtk_file_filter_get_name_ptr = NULL;
void *gtk_file_chooser_dialog_new_ptr = NULL;
typedef void (*gtk_file_chooser_add_filter_t)(void *pArg1, void *pArg2);
gtk_file_chooser_add_filter_t gtk_file_chooser_add_filter_ptr = NULL;
typedef void *(*gtk_file_chooser_get_filter_t)(void *pArg1);
gtk_file_chooser_get_filter_t gtk_file_chooser_get_filter_ptr = NULL;
typedef int (*gtk_file_chooser_set_filename_t)(void *pArg1, void *pArg2);
gtk_file_chooser_set_filename_t gtk_file_chooser_set_filename_ptr = NULL;
typedef void *(*gtk_file_chooser_get_filename_t)(void *pArg1);
gtk_file_chooser_get_filename_t gtk_file_chooser_get_filename_ptr = NULL;
typedef void *(*gtk_file_chooser_get_filenames_t)(void *pArg1);
gtk_file_chooser_get_filenames_t gtk_file_chooser_get_filenames_ptr = NULL;
typedef int (*gtk_file_chooser_set_current_folder_t)(void *pArg1, void *pArg2);
gtk_file_chooser_set_current_folder_t gtk_file_chooser_set_current_folder_ptr = NULL;
typedef void *(*gtk_file_chooser_get_current_folder_t)(void *pArg1);
gtk_file_chooser_get_current_folder_t gtk_file_chooser_get_current_folder_ptr = NULL;
typedef int (*gtk_file_chooser_set_current_name_t)(void *pArg1, void *pArg2);
gtk_file_chooser_set_current_name_t gtk_file_chooser_set_current_name_ptr = NULL;
typedef void (*gtk_file_chooser_set_select_multiple_t)(void *pArg1, int pArg2);
gtk_file_chooser_set_select_multiple_t gtk_file_chooser_set_select_multiple_ptr = NULL;
typedef int (*gtk_file_chooser_get_select_multiple_t)(void *pArg1);
gtk_file_chooser_get_select_multiple_t gtk_file_chooser_get_select_multiple_ptr = NULL;
typedef int (*gtk_file_chooser_get_type_t)(void);
gtk_file_chooser_get_type_t gtk_file_chooser_get_type_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_GTK_FILE_DIALOG_NAME ""
#elif defined(_LINUX)
#define MODULE_GTK_FILE_DIALOG_NAME "libgtk-x11-2.0.so.0"
#elif defined(_WINDOWS)
#define MODULE_GTK_FILE_DIALOG_NAME ""
#endif

static module_t module_gtk_file_dialog = NULL;

int initialise_weak_link_gtk_file_dialog(void)
{
#if defined(_LINUX)
  if (!module_load("libgtk-x11-2.0.so.0", &module_gtk_file_dialog))
#else
  if (!module_load(MODULE_GTK_FILE_DIALOG_NAME, &module_gtk_file_dialog))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libgtk-x11-2.0.so.0\n") ;
#endif
goto err;
}

  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_filter_new", (handler_t *)&gtk_file_filter_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_filter_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_filter_set_name", (handler_t *)&gtk_file_filter_set_name_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_filter_set_name\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_filter_add_pattern", (handler_t *)&gtk_file_filter_add_pattern_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_filter_add_pattern\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_filter_get_name", (handler_t *)&gtk_file_filter_get_name_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_filter_get_name\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_chooser_dialog_new", (handler_t *)&gtk_file_chooser_dialog_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_chooser_dialog_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_chooser_add_filter", (handler_t *)&gtk_file_chooser_add_filter_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_chooser_add_filter\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_chooser_get_filter", (handler_t *)&gtk_file_chooser_get_filter_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_chooser_get_filter\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_chooser_set_filename", (handler_t *)&gtk_file_chooser_set_filename_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_chooser_set_filename\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_chooser_get_filename", (handler_t *)&gtk_file_chooser_get_filename_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_chooser_get_filename\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_chooser_get_filenames", (handler_t *)&gtk_file_chooser_get_filenames_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_chooser_get_filenames\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_chooser_set_current_folder", (handler_t *)&gtk_file_chooser_set_current_folder_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_chooser_set_current_folder\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_chooser_get_current_folder", (handler_t *)&gtk_file_chooser_get_current_folder_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_chooser_get_current_folder\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_chooser_set_current_name", (handler_t *)&gtk_file_chooser_set_current_name_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_chooser_set_current_name\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_chooser_set_select_multiple", (handler_t *)&gtk_file_chooser_set_select_multiple_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_chooser_set_select_multiple\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_chooser_get_select_multiple", (handler_t *)&gtk_file_chooser_get_select_multiple_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_chooser_get_select_multiple\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_file_dialog, SYMBOL_PREFIX "gtk_file_chooser_get_type", (handler_t *)&gtk_file_chooser_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_file_chooser_get_type\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_gtk_file_dialog != NULL)
    module_unload(module_gtk_file_dialog);

  return 0;
}

void *gtk_file_filter_new(void)
{
  return gtk_file_filter_new_ptr();
}

void gtk_file_filter_set_name(void *pArg1, void *pArg2)
{
  gtk_file_filter_set_name_ptr(pArg1, pArg2);
}

void gtk_file_filter_add_pattern(void *pArg1, void *pArg2)
{
  gtk_file_filter_add_pattern_ptr(pArg1, pArg2);
}

void *gtk_file_filter_get_name(void *pArg1)
{
  return gtk_file_filter_get_name_ptr(pArg1);
}

void gtk_file_chooser_add_filter(void *pArg1, void *pArg2)
{
  gtk_file_chooser_add_filter_ptr(pArg1, pArg2);
}

void *gtk_file_chooser_get_filter(void *pArg1)
{
  return gtk_file_chooser_get_filter_ptr(pArg1);
}

int gtk_file_chooser_set_filename(void *pArg1, void *pArg2)
{
  return gtk_file_chooser_set_filename_ptr(pArg1, pArg2);
}

void *gtk_file_chooser_get_filename(void *pArg1)
{
  return gtk_file_chooser_get_filename_ptr(pArg1);
}

void *gtk_file_chooser_get_filenames(void *pArg1)
{
  return gtk_file_chooser_get_filenames_ptr(pArg1);
}

int gtk_file_chooser_set_current_folder(void *pArg1, void *pArg2)
{
  return gtk_file_chooser_set_current_folder_ptr(pArg1, pArg2);
}

void *gtk_file_chooser_get_current_folder(void *pArg1)
{
  return gtk_file_chooser_get_current_folder_ptr(pArg1);
}

int gtk_file_chooser_set_current_name(void *pArg1, void *pArg2)
{
  return gtk_file_chooser_set_current_name_ptr(pArg1, pArg2);
}

void gtk_file_chooser_set_select_multiple(void *pArg1, int pArg2)
{
  gtk_file_chooser_set_select_multiple_ptr(pArg1, pArg2);
}

int gtk_file_chooser_get_select_multiple(void *pArg1)
{
  return gtk_file_chooser_get_select_multiple_ptr(pArg1);
}

int gtk_file_chooser_get_type(void)
{
  return gtk_file_chooser_get_type_ptr();
}

typedef int (*XcursorSupportsARGB_t)(void *pArg1);
XcursorSupportsARGB_t XcursorSupportsARGB_ptr = NULL;
typedef int (*XcursorGetDefaultSize_t)(void *pArg1);
XcursorGetDefaultSize_t XcursorGetDefaultSize_ptr = NULL;
typedef int (*XcursorImageLoadCursor_t)(void *pArg1, void *pArg2);
XcursorImageLoadCursor_t XcursorImageLoadCursor_ptr = NULL;
typedef int (*XcursorShapeLoadCursor_t)(void *pArg1, int pArg2);
XcursorShapeLoadCursor_t XcursorShapeLoadCursor_ptr = NULL;
typedef void (*XcursorSetTheme_t)(void *pArg1, void *pArg2);
XcursorSetTheme_t XcursorSetTheme_ptr = NULL;
typedef void (*XcursorSetDefaultSize_t)(void *pArg1, int pArg2);
XcursorSetDefaultSize_t XcursorSetDefaultSize_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_LIBXCURSOR_NAME ""
#elif defined(_LINUX)
#define MODULE_LIBXCURSOR_NAME "libXcursor.so.1"
#elif defined(_WINDOWS)
#define MODULE_LIBXCURSOR_NAME ""
#endif

static module_t module_libxcursor = NULL;

int initialise_weak_link_libxcursor(void)
{
#if defined(_LINUX)
  if (!module_load("libXcursor.so.1", &module_libxcursor))
#else
  if (!module_load(MODULE_LIBXCURSOR_NAME, &module_libxcursor))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libXcursor.so.1\n") ;
#endif
goto err;
}

  if (!module_resolve(module_libxcursor, SYMBOL_PREFIX "XcursorSupportsARGB", (handler_t *)&XcursorSupportsARGB_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XcursorSupportsARGB\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_libxcursor, SYMBOL_PREFIX "XcursorGetDefaultSize", (handler_t *)&XcursorGetDefaultSize_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XcursorGetDefaultSize\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_libxcursor, SYMBOL_PREFIX "XcursorImageLoadCursor", (handler_t *)&XcursorImageLoadCursor_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XcursorImageLoadCursor\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_libxcursor, SYMBOL_PREFIX "XcursorShapeLoadCursor", (handler_t *)&XcursorShapeLoadCursor_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XcursorShapeLoadCursor\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_libxcursor, SYMBOL_PREFIX "XcursorSetTheme", (handler_t *)&XcursorSetTheme_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XcursorSetTheme\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_libxcursor, SYMBOL_PREFIX "XcursorSetDefaultSize", (handler_t *)&XcursorSetDefaultSize_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XcursorSetDefaultSize\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_libxcursor != NULL)
    module_unload(module_libxcursor);

  return 0;
}

int XcursorSupportsARGB(void *pArg1)
{
  return XcursorSupportsARGB_ptr(pArg1);
}

int XcursorGetDefaultSize(void *pArg1)
{
  return XcursorGetDefaultSize_ptr(pArg1);
}

int XcursorImageLoadCursor(void *pArg1, void *pArg2)
{
  return XcursorImageLoadCursor_ptr(pArg1, pArg2);
}

int XcursorShapeLoadCursor(void *pArg1, int pArg2)
{
  return XcursorShapeLoadCursor_ptr(pArg1, pArg2);
}

void XcursorSetTheme(void *pArg1, void *pArg2)
{
  XcursorSetTheme_ptr(pArg1, pArg2);
}

void XcursorSetDefaultSize(void *pArg1, int pArg2)
{
  XcursorSetDefaultSize_ptr(pArg1, pArg2);
}

typedef void *(*g_list_append_t)(void *pArg1, void *pArg2);
g_list_append_t g_list_append_ptr = NULL;
typedef void (*g_list_free_t)(void *pArg1);
g_list_free_t g_list_free_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_GLIB_NAME ""
#elif defined(_LINUX)
#define MODULE_GLIB_NAME "libglib-2.0.so.0"
#elif defined(_WINDOWS)
#define MODULE_GLIB_NAME ""
#endif

static module_t module_glib = NULL;

int initialise_weak_link_glib(void)
{
#if defined(_LINUX)
  if (!module_load("libglib-2.0.so.0", &module_glib))
#else
  if (!module_load(MODULE_GLIB_NAME, &module_glib))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libglib-2.0.so.0\n") ;
#endif
goto err;
}

  if (!module_resolve(module_glib, SYMBOL_PREFIX "g_list_append", (handler_t *)&g_list_append_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_list_append\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_glib, SYMBOL_PREFIX "g_list_free", (handler_t *)&g_list_free_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_list_free\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_glib != NULL)
    module_unload(module_glib);

  return 0;
}

void *g_list_append(void *pArg1, void *pArg2)
{
  return g_list_append_ptr(pArg1, pArg2);
}

void g_list_free(void *pArg1)
{
  g_list_free_ptr(pArg1);
}

typedef int (*gnome_url_show_t)(void *pArg1, void *pArg2);
gnome_url_show_t gnome_url_show_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_LIBGNOME_NAME ""
#elif defined(_LINUX)
#define MODULE_LIBGNOME_NAME "libgnome-2.so.0"
#elif defined(_WINDOWS)
#define MODULE_LIBGNOME_NAME ""
#endif

static module_t module_libgnome = NULL;

int initialise_weak_link_libgnome(void)
{
#if defined(_LINUX)
  if (!module_load("libgnome-2.so.0", &module_libgnome))
#else
  if (!module_load(MODULE_LIBGNOME_NAME, &module_libgnome))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libgnome-2.so.0\n") ;
#endif
goto err;
}

  if (!module_resolve(module_libgnome, SYMBOL_PREFIX "gnome_url_show", (handler_t *)&gnome_url_show_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gnome_url_show\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_libgnome != NULL)
    module_unload(module_libgnome);

  return 0;
}

int gnome_url_show(void *pArg1, void *pArg2)
{
  return gnome_url_show_ptr(pArg1, pArg2);
}

typedef int (*gtk_object_get_type_t)(void);
gtk_object_get_type_t gtk_object_get_type_ptr = NULL;
typedef int (*gtk_widget_get_type_t)(void);
gtk_widget_get_type_t gtk_widget_get_type_ptr = NULL;
typedef int (*gtk_container_get_type_t)(void);
gtk_container_get_type_t gtk_container_get_type_ptr = NULL;
typedef int (*gtk_option_menu_get_type_t)(void);
gtk_option_menu_get_type_t gtk_option_menu_get_type_ptr = NULL;
typedef int (*gtk_bin_get_type_t)(void);
gtk_bin_get_type_t gtk_bin_get_type_ptr = NULL;
typedef int (*gtk_hbox_get_type_t)(void);
gtk_hbox_get_type_t gtk_hbox_get_type_ptr = NULL;
typedef int (*gtk_arrow_get_type_t)(void);
gtk_arrow_get_type_t gtk_arrow_get_type_ptr = NULL;
typedef int (*gtk_separator_get_type_t)(void);
gtk_separator_get_type_t gtk_separator_get_type_ptr = NULL;
typedef int (*gtk_entry_get_type_t)(void);
gtk_entry_get_type_t gtk_entry_get_type_ptr = NULL;
typedef int (*gtk_range_get_type_t)(void);
gtk_range_get_type_t gtk_range_get_type_ptr = NULL;
typedef int (*gtk_scale_get_type_t)(void);
gtk_scale_get_type_t gtk_scale_get_type_ptr = NULL;
typedef int (*gtk_scrollbar_get_type_t)(void);
gtk_scrollbar_get_type_t gtk_scrollbar_get_type_ptr = NULL;
typedef int (*gtk_button_get_type_t)(void);
gtk_button_get_type_t gtk_button_get_type_ptr = NULL;
typedef int (*gtk_toggle_button_get_type_t)(void);
gtk_toggle_button_get_type_t gtk_toggle_button_get_type_ptr = NULL;
typedef int (*gtk_progress_bar_get_type_t)(void);
gtk_progress_bar_get_type_t gtk_progress_bar_get_type_ptr = NULL;
typedef int (*gtk_misc_get_type_t)(void);
gtk_misc_get_type_t gtk_misc_get_type_ptr = NULL;
typedef int (*gtk_dialog_get_type_t)(void);
gtk_dialog_get_type_t gtk_dialog_get_type_ptr = NULL;
typedef int (*gtk_handle_box_get_type_t)(void);
gtk_handle_box_get_type_t gtk_handle_box_get_type_ptr = NULL;
typedef int (*gtk_menu_item_get_type_t)(void);
gtk_menu_item_get_type_t gtk_menu_item_get_type_ptr = NULL;
typedef void *(*gtk_settings_get_default_t)(void);
gtk_settings_get_default_t gtk_settings_get_default_ptr = NULL;
typedef void (*gtk_adjustment_changed_t)(void *pArg1);
gtk_adjustment_changed_t gtk_adjustment_changed_ptr = NULL;
typedef void (*gtk_object_sink_t)(void *pArg1);
gtk_object_sink_t gtk_object_sink_ptr = NULL;
typedef void *(*gtk_adjustment_new_t)(double pArg1, double pArg2, double pArg3, double pArg4, double pArg5, double pArg6);
gtk_adjustment_new_t gtk_adjustment_new_ptr = NULL;
typedef void *(*gtk_arrow_new_t)(int pArg1, int pArg2);
gtk_arrow_new_t gtk_arrow_new_ptr = NULL;
typedef void *(*gtk_button_new_t)(void);
gtk_button_new_t gtk_button_new_ptr = NULL;
typedef void *(*gtk_button_new_with_label_t)(void *pArg1);
gtk_button_new_with_label_t gtk_button_new_with_label_ptr = NULL;
typedef void *(*gtk_check_button_new_with_label_t)(void *pArg1);
gtk_check_button_new_with_label_t gtk_check_button_new_with_label_ptr = NULL;
typedef void *(*gtk_toggle_button_new_t)(void);
gtk_toggle_button_new_t gtk_toggle_button_new_ptr = NULL;
typedef void *(*gtk_entry_new_t)(void);
gtk_entry_new_t gtk_entry_new_ptr = NULL;
typedef void *(*gtk_fixed_new_t)(void);
gtk_fixed_new_t gtk_fixed_new_ptr = NULL;
typedef void *(*gtk_frame_new_t)(void *pArg1);
gtk_frame_new_t gtk_frame_new_ptr = NULL;
typedef void *(*gtk_handle_box_new_t)(void);
gtk_handle_box_new_t gtk_handle_box_new_ptr = NULL;
typedef void *(*gtk_hscale_new_t)(void *pArg1);
gtk_hscale_new_t gtk_hscale_new_ptr = NULL;
typedef void *(*gtk_hscrollbar_new_t)(void *pArg1);
gtk_hscrollbar_new_t gtk_hscrollbar_new_ptr = NULL;
typedef void (*gtk_init_t)(void *pArg1, void *pArg2);
gtk_init_t gtk_init_ptr = NULL;
typedef void *(*gtk_label_new_t)(void *pArg1);
gtk_label_new_t gtk_label_new_ptr = NULL;
typedef void *(*gtk_menu_item_new_with_label_t)(void *pArg1);
gtk_menu_item_new_with_label_t gtk_menu_item_new_with_label_ptr = NULL;
typedef void *(*gtk_notebook_new_t)(void);
gtk_notebook_new_t gtk_notebook_new_ptr = NULL;
typedef void *(*gtk_option_menu_new_t)(void);
gtk_option_menu_new_t gtk_option_menu_new_ptr = NULL;
typedef void *(*gtk_vscale_new_t)(void *pArg1);
gtk_vscale_new_t gtk_vscale_new_ptr = NULL;
typedef void *(*gtk_vscrollbar_new_t)(void *pArg1);
gtk_vscrollbar_new_t gtk_vscrollbar_new_ptr = NULL;
typedef void *(*gtk_tooltips_new_t)(void);
gtk_tooltips_new_t gtk_tooltips_new_ptr = NULL;
typedef void *(*gtk_spin_button_new_t)(void *pArg1, double pArg2, int pArg3);
gtk_spin_button_new_t gtk_spin_button_new_ptr = NULL;
typedef void *(*gtk_radio_button_new_with_label_t)(void *pArg1, void *pArg2);
gtk_radio_button_new_with_label_t gtk_radio_button_new_with_label_ptr = NULL;
typedef void *(*gtk_progress_bar_new_t)(void);
gtk_progress_bar_new_t gtk_progress_bar_new_ptr = NULL;
typedef void *(*gtk_statusbar_new_t)(void);
gtk_statusbar_new_t gtk_statusbar_new_ptr = NULL;
typedef void (*gtk_paint_arrow_t)(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11, int pArg12, int pArg13);
gtk_paint_arrow_t gtk_paint_arrow_ptr = NULL;
typedef void (*gtk_paint_box_t)(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11);
gtk_paint_box_t gtk_paint_box_ptr = NULL;
typedef void (*gtk_paint_flat_box_t)(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11);
gtk_paint_flat_box_t gtk_paint_flat_box_ptr = NULL;
typedef void (*gtk_paint_box_gap_t)(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11, int pArg12, int pArg13, int pArg14);
gtk_paint_box_gap_t gtk_paint_box_gap_ptr = NULL;
typedef void (*gtk_paint_check_t)(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11);
gtk_paint_check_t gtk_paint_check_ptr = NULL;
typedef void (*gtk_paint_option_t)(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11);
gtk_paint_option_t gtk_paint_option_ptr = NULL;
typedef void (*gtk_paint_tab_t)(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11);
gtk_paint_tab_t gtk_paint_tab_ptr = NULL;
typedef void (*gtk_paint_extension_t)(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11, int pArg12);
gtk_paint_extension_t gtk_paint_extension_ptr = NULL;
typedef void (*gtk_paint_focus_t)(void *pArg1, void *pArg2, int pArg3, void *pArg4, void *pArg5, void *pArg6, int pArg7, int pArg8, int pArg9, int pArg10);
gtk_paint_focus_t gtk_paint_focus_ptr = NULL;
typedef void (*gtk_paint_shadow_t)(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11);
gtk_paint_shadow_t gtk_paint_shadow_ptr = NULL;
typedef void (*gtk_paint_slider_t)(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11, int pArg12);
gtk_paint_slider_t gtk_paint_slider_ptr = NULL;
typedef void (*gtk_progress_bar_set_orientation_t)(void *pArg1, int pArg2);
gtk_progress_bar_set_orientation_t gtk_progress_bar_set_orientation_ptr = NULL;
typedef void *(*gtk_range_get_adjustment_t)(void *pArg1);
gtk_range_get_adjustment_t gtk_range_get_adjustment_ptr = NULL;
typedef void (*gtk_style_apply_default_background_t)(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, int pArg6, int pArg7, int pArg8, int pArg9);
gtk_style_apply_default_background_t gtk_style_apply_default_background_ptr = NULL;
typedef void *(*gtk_style_attach_t)(void *pArg1, void *pArg2);
gtk_style_attach_t gtk_style_attach_ptr = NULL;
typedef void (*gtk_toggle_button_set_active_t)(void *pArg1, int pArg2);
gtk_toggle_button_set_active_t gtk_toggle_button_set_active_ptr = NULL;
typedef void (*gtk_toggle_button_set_inconsistent_t)(void *pArg1, int pArg2);
gtk_toggle_button_set_inconsistent_t gtk_toggle_button_set_inconsistent_ptr = NULL;
typedef void (*gtk_tooltips_force_window_t)(void *pArg1);
gtk_tooltips_force_window_t gtk_tooltips_force_window_ptr = NULL;
typedef void (*gtk_container_add_t)(void *pArg1, void *pArg2);
gtk_container_add_t gtk_container_add_ptr = NULL;
typedef void (*gtk_widget_destroy_t)(void *pArg1);
gtk_widget_destroy_t gtk_widget_destroy_ptr = NULL;
typedef void (*gtk_widget_realize_t)(void *pArg1);
gtk_widget_realize_t gtk_widget_realize_ptr = NULL;
typedef void (*gtk_widget_set_state_t)(void *pArg1, int pArg2);
gtk_widget_set_state_t gtk_widget_set_state_ptr = NULL;
typedef void (*gtk_widget_set_style_t)(void *pArg1, void *pArg2);
gtk_widget_set_style_t gtk_widget_set_style_ptr = NULL;
typedef void (*gtk_widget_set_name_t)(void *pArg1, void *pArg2);
gtk_widget_set_name_t gtk_widget_set_name_ptr = NULL;
typedef void (*gtk_widget_set_sensitive_t)(void *pArg1, int pArg2);
gtk_widget_set_sensitive_t gtk_widget_set_sensitive_ptr = NULL;
typedef void (*gtk_widget_class_install_style_property_t)(void *pArg1, void *pArg2);
gtk_widget_class_install_style_property_t gtk_widget_class_install_style_property_ptr = NULL;
void *gtk_widget_style_get_ptr = NULL;
typedef void *(*gtk_window_new_t)(int pArg1);
gtk_window_new_t gtk_window_new_ptr = NULL;
typedef void (*gtk_object_unref_t)(void *pArg1);
gtk_object_unref_t gtk_object_unref_ptr = NULL;
typedef void (*gtk_main_do_event_t)(void *pArg1);
gtk_main_do_event_t gtk_main_do_event_ptr = NULL;
typedef void (*gtk_widget_set_colormap_t)(void *pArg1, void *pArg2);
gtk_widget_set_colormap_t gtk_widget_set_colormap_ptr = NULL;
typedef int (*gtk_dialog_run_t)(void *pArg1);
gtk_dialog_run_t gtk_dialog_run_ptr = NULL;
typedef int (*gtk_events_pending_t)(void);
gtk_events_pending_t gtk_events_pending_ptr = NULL;
typedef int (*gtk_main_iteration_t)(void);
gtk_main_iteration_t gtk_main_iteration_ptr = NULL;
typedef void (*gtk_border_free_t)(void *pArg1);
gtk_border_free_t gtk_border_free_ptr = NULL;
typedef void (*gtk_window_set_keep_above_t)(void *pArg1, int pArg2);
gtk_window_set_keep_above_t gtk_window_set_keep_above_ptr = NULL;
typedef void (*gtk_window_set_modal_t)(void *pArg1, int pArg2);
gtk_window_set_modal_t gtk_window_set_modal_ptr = NULL;
typedef int (*gtk_window_get_type_t)(void);
gtk_window_get_type_t gtk_window_get_type_ptr = NULL;
typedef void *(*gtk_widget_get_parent_window_t)(void *pArg1);
gtk_widget_get_parent_window_t gtk_widget_get_parent_window_ptr = NULL;
typedef void *(*gdk_window_foreign_new_t)(void *pArg1);
gdk_window_foreign_new_t gdk_window_foreign_new_ptr = NULL;
typedef void (*gtk_requisition_free_t)(void *pArg1);
gtk_requisition_free_t gtk_requisition_free_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_GTK_NAME ""
#elif defined(_LINUX)
#define MODULE_GTK_NAME "libgtk-x11-2.0.so.0"
#elif defined(_WINDOWS)
#define MODULE_GTK_NAME ""
#endif

static module_t module_gtk = NULL;

int initialise_weak_link_gtk(void)
{
#if defined(_LINUX)
  if (!module_load("libgtk-x11-2.0.so.0", &module_gtk))
#else
  if (!module_load(MODULE_GTK_NAME, &module_gtk))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libgtk-x11-2.0.so.0\n") ;
#endif
goto err;
}

  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_object_get_type", (handler_t *)&gtk_object_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_object_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_widget_get_type", (handler_t *)&gtk_widget_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_widget_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_container_get_type", (handler_t *)&gtk_container_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_container_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_option_menu_get_type", (handler_t *)&gtk_option_menu_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_option_menu_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_bin_get_type", (handler_t *)&gtk_bin_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_bin_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_hbox_get_type", (handler_t *)&gtk_hbox_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_hbox_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_arrow_get_type", (handler_t *)&gtk_arrow_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_arrow_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_separator_get_type", (handler_t *)&gtk_separator_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_separator_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_entry_get_type", (handler_t *)&gtk_entry_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_entry_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_range_get_type", (handler_t *)&gtk_range_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_range_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_scale_get_type", (handler_t *)&gtk_scale_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_scale_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_scrollbar_get_type", (handler_t *)&gtk_scrollbar_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_scrollbar_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_button_get_type", (handler_t *)&gtk_button_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_button_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_toggle_button_get_type", (handler_t *)&gtk_toggle_button_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_toggle_button_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_progress_bar_get_type", (handler_t *)&gtk_progress_bar_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_progress_bar_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_misc_get_type", (handler_t *)&gtk_misc_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_misc_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_dialog_get_type", (handler_t *)&gtk_dialog_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_dialog_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_handle_box_get_type", (handler_t *)&gtk_handle_box_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_handle_box_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_menu_item_get_type", (handler_t *)&gtk_menu_item_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_menu_item_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_settings_get_default", (handler_t *)&gtk_settings_get_default_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_settings_get_default\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_adjustment_changed", (handler_t *)&gtk_adjustment_changed_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_adjustment_changed\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_object_sink", (handler_t *)&gtk_object_sink_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_object_sink\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_adjustment_new", (handler_t *)&gtk_adjustment_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_adjustment_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_arrow_new", (handler_t *)&gtk_arrow_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_arrow_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_button_new", (handler_t *)&gtk_button_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_button_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_button_new_with_label", (handler_t *)&gtk_button_new_with_label_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_button_new_with_label\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_check_button_new_with_label", (handler_t *)&gtk_check_button_new_with_label_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_check_button_new_with_label\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_toggle_button_new", (handler_t *)&gtk_toggle_button_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_toggle_button_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_entry_new", (handler_t *)&gtk_entry_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_entry_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_fixed_new", (handler_t *)&gtk_fixed_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_fixed_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_frame_new", (handler_t *)&gtk_frame_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_frame_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_handle_box_new", (handler_t *)&gtk_handle_box_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_handle_box_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_hscale_new", (handler_t *)&gtk_hscale_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_hscale_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_hscrollbar_new", (handler_t *)&gtk_hscrollbar_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_hscrollbar_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_init", (handler_t *)&gtk_init_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_init\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_label_new", (handler_t *)&gtk_label_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_label_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_menu_item_new_with_label", (handler_t *)&gtk_menu_item_new_with_label_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_menu_item_new_with_label\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_notebook_new", (handler_t *)&gtk_notebook_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_notebook_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_option_menu_new", (handler_t *)&gtk_option_menu_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_option_menu_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_vscale_new", (handler_t *)&gtk_vscale_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_vscale_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_vscrollbar_new", (handler_t *)&gtk_vscrollbar_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_vscrollbar_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_tooltips_new", (handler_t *)&gtk_tooltips_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_tooltips_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_spin_button_new", (handler_t *)&gtk_spin_button_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_spin_button_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_radio_button_new_with_label", (handler_t *)&gtk_radio_button_new_with_label_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_radio_button_new_with_label\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_progress_bar_new", (handler_t *)&gtk_progress_bar_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_progress_bar_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_statusbar_new", (handler_t *)&gtk_statusbar_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_statusbar_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_paint_arrow", (handler_t *)&gtk_paint_arrow_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_paint_arrow\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_paint_box", (handler_t *)&gtk_paint_box_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_paint_box\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_paint_flat_box", (handler_t *)&gtk_paint_flat_box_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_paint_flat_box\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_paint_box_gap", (handler_t *)&gtk_paint_box_gap_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_paint_box_gap\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_paint_check", (handler_t *)&gtk_paint_check_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_paint_check\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_paint_option", (handler_t *)&gtk_paint_option_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_paint_option\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_paint_tab", (handler_t *)&gtk_paint_tab_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_paint_tab\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_paint_extension", (handler_t *)&gtk_paint_extension_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_paint_extension\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_paint_focus", (handler_t *)&gtk_paint_focus_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_paint_focus\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_paint_shadow", (handler_t *)&gtk_paint_shadow_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_paint_shadow\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_paint_slider", (handler_t *)&gtk_paint_slider_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_paint_slider\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_progress_bar_set_orientation", (handler_t *)&gtk_progress_bar_set_orientation_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_progress_bar_set_orientation\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_range_get_adjustment", (handler_t *)&gtk_range_get_adjustment_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_range_get_adjustment\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_style_apply_default_background", (handler_t *)&gtk_style_apply_default_background_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_style_apply_default_background\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_style_attach", (handler_t *)&gtk_style_attach_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_style_attach\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_toggle_button_set_active", (handler_t *)&gtk_toggle_button_set_active_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_toggle_button_set_active\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_toggle_button_set_inconsistent", (handler_t *)&gtk_toggle_button_set_inconsistent_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_toggle_button_set_inconsistent\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_tooltips_force_window", (handler_t *)&gtk_tooltips_force_window_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_tooltips_force_window\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_container_add", (handler_t *)&gtk_container_add_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_container_add\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_widget_destroy", (handler_t *)&gtk_widget_destroy_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_widget_destroy\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_widget_realize", (handler_t *)&gtk_widget_realize_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_widget_realize\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_widget_set_state", (handler_t *)&gtk_widget_set_state_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_widget_set_state\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_widget_set_style", (handler_t *)&gtk_widget_set_style_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_widget_set_style\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_widget_set_name", (handler_t *)&gtk_widget_set_name_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_widget_set_name\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_widget_set_sensitive", (handler_t *)&gtk_widget_set_sensitive_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_widget_set_sensitive\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_widget_class_install_style_property", (handler_t *)&gtk_widget_class_install_style_property_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_widget_class_install_style_property\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_widget_style_get", (handler_t *)&gtk_widget_style_get_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_widget_style_get\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_window_new", (handler_t *)&gtk_window_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_window_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_object_unref", (handler_t *)&gtk_object_unref_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_object_unref\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_main_do_event", (handler_t *)&gtk_main_do_event_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_main_do_event\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_widget_set_colormap", (handler_t *)&gtk_widget_set_colormap_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_widget_set_colormap\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_dialog_run", (handler_t *)&gtk_dialog_run_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_dialog_run\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_events_pending", (handler_t *)&gtk_events_pending_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_events_pending\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_main_iteration", (handler_t *)&gtk_main_iteration_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_main_iteration\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_border_free", (handler_t *)&gtk_border_free_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_border_free\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_window_set_keep_above", (handler_t *)&gtk_window_set_keep_above_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_window_set_keep_above\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_window_set_modal", (handler_t *)&gtk_window_set_modal_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_window_set_modal\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_window_get_type", (handler_t *)&gtk_window_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_window_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_widget_get_parent_window", (handler_t *)&gtk_widget_get_parent_window_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_widget_get_parent_window\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gdk_window_foreign_new", (handler_t *)&gdk_window_foreign_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gdk_window_foreign_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk, SYMBOL_PREFIX "gtk_requisition_free", (handler_t *)&gtk_requisition_free_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_requisition_free\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_gtk != NULL)
    module_unload(module_gtk);

  return 0;
}

int gtk_object_get_type(void)
{
  return gtk_object_get_type_ptr();
}

int gtk_widget_get_type(void)
{
  return gtk_widget_get_type_ptr();
}

int gtk_container_get_type(void)
{
  return gtk_container_get_type_ptr();
}

int gtk_option_menu_get_type(void)
{
  return gtk_option_menu_get_type_ptr();
}

int gtk_bin_get_type(void)
{
  return gtk_bin_get_type_ptr();
}

int gtk_hbox_get_type(void)
{
  return gtk_hbox_get_type_ptr();
}

int gtk_arrow_get_type(void)
{
  return gtk_arrow_get_type_ptr();
}

int gtk_separator_get_type(void)
{
  return gtk_separator_get_type_ptr();
}

int gtk_entry_get_type(void)
{
  return gtk_entry_get_type_ptr();
}

int gtk_range_get_type(void)
{
  return gtk_range_get_type_ptr();
}

int gtk_scale_get_type(void)
{
  return gtk_scale_get_type_ptr();
}

int gtk_scrollbar_get_type(void)
{
  return gtk_scrollbar_get_type_ptr();
}

int gtk_button_get_type(void)
{
  return gtk_button_get_type_ptr();
}

int gtk_toggle_button_get_type(void)
{
  return gtk_toggle_button_get_type_ptr();
}

int gtk_progress_bar_get_type(void)
{
  return gtk_progress_bar_get_type_ptr();
}

int gtk_misc_get_type(void)
{
  return gtk_misc_get_type_ptr();
}

int gtk_dialog_get_type(void)
{
  return gtk_dialog_get_type_ptr();
}

int gtk_handle_box_get_type(void)
{
  return gtk_handle_box_get_type_ptr();
}

int gtk_menu_item_get_type(void)
{
  return gtk_menu_item_get_type_ptr();
}

void *gtk_settings_get_default(void)
{
  return gtk_settings_get_default_ptr();
}

void gtk_adjustment_changed(void *pArg1)
{
  gtk_adjustment_changed_ptr(pArg1);
}

void gtk_object_sink(void *pArg1)
{
  gtk_object_sink_ptr(pArg1);
}

void *gtk_adjustment_new(double pArg1, double pArg2, double pArg3, double pArg4, double pArg5, double pArg6)
{
  return gtk_adjustment_new_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6);
}

void *gtk_arrow_new(int pArg1, int pArg2)
{
  return gtk_arrow_new_ptr(pArg1, pArg2);
}

void *gtk_button_new(void)
{
  return gtk_button_new_ptr();
}

void *gtk_button_new_with_label(void *pArg1)
{
  return gtk_button_new_with_label_ptr(pArg1);
}

void *gtk_check_button_new_with_label(void *pArg1)
{
  return gtk_check_button_new_with_label_ptr(pArg1);
}

void *gtk_toggle_button_new(void)
{
  return gtk_toggle_button_new_ptr();
}

void *gtk_entry_new(void)
{
  return gtk_entry_new_ptr();
}

void *gtk_fixed_new(void)
{
  return gtk_fixed_new_ptr();
}

void *gtk_frame_new(void *pArg1)
{
  return gtk_frame_new_ptr(pArg1);
}

void *gtk_handle_box_new(void)
{
  return gtk_handle_box_new_ptr();
}

void *gtk_hscale_new(void *pArg1)
{
  return gtk_hscale_new_ptr(pArg1);
}

void *gtk_hscrollbar_new(void *pArg1)
{
  return gtk_hscrollbar_new_ptr(pArg1);
}

void gtk_init(void *pArg1, void *pArg2)
{
  gtk_init_ptr(pArg1, pArg2);
}

void *gtk_label_new(void *pArg1)
{
  return gtk_label_new_ptr(pArg1);
}

void *gtk_menu_item_new_with_label(void *pArg1)
{
  return gtk_menu_item_new_with_label_ptr(pArg1);
}

void *gtk_notebook_new(void)
{
  return gtk_notebook_new_ptr();
}

void *gtk_option_menu_new(void)
{
  return gtk_option_menu_new_ptr();
}

void *gtk_vscale_new(void *pArg1)
{
  return gtk_vscale_new_ptr(pArg1);
}

void *gtk_vscrollbar_new(void *pArg1)
{
  return gtk_vscrollbar_new_ptr(pArg1);
}

void *gtk_tooltips_new(void)
{
  return gtk_tooltips_new_ptr();
}

void *gtk_spin_button_new(void *pArg1, double pArg2, int pArg3)
{
  return gtk_spin_button_new_ptr(pArg1, pArg2, pArg3);
}

void *gtk_radio_button_new_with_label(void *pArg1, void *pArg2)
{
  return gtk_radio_button_new_with_label_ptr(pArg1, pArg2);
}

void *gtk_progress_bar_new(void)
{
  return gtk_progress_bar_new_ptr();
}

void *gtk_statusbar_new(void)
{
  return gtk_statusbar_new_ptr();
}

void gtk_paint_arrow(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11, int pArg12, int pArg13)
{
  gtk_paint_arrow_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8, pArg9, pArg10, pArg11, pArg12, pArg13);
}

void gtk_paint_box(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11)
{
  gtk_paint_box_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8, pArg9, pArg10, pArg11);
}

void gtk_paint_flat_box(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11)
{
  gtk_paint_flat_box_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8, pArg9, pArg10, pArg11);
}

void gtk_paint_box_gap(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11, int pArg12, int pArg13, int pArg14)
{
  gtk_paint_box_gap_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8, pArg9, pArg10, pArg11, pArg12, pArg13, pArg14);
}

void gtk_paint_check(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11)
{
  gtk_paint_check_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8, pArg9, pArg10, pArg11);
}

void gtk_paint_option(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11)
{
  gtk_paint_option_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8, pArg9, pArg10, pArg11);
}

void gtk_paint_tab(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11)
{
  gtk_paint_tab_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8, pArg9, pArg10, pArg11);
}

void gtk_paint_extension(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11, int pArg12)
{
  gtk_paint_extension_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8, pArg9, pArg10, pArg11, pArg12);
}

void gtk_paint_focus(void *pArg1, void *pArg2, int pArg3, void *pArg4, void *pArg5, void *pArg6, int pArg7, int pArg8, int pArg9, int pArg10)
{
  gtk_paint_focus_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8, pArg9, pArg10);
}

void gtk_paint_shadow(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11)
{
  gtk_paint_shadow_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8, pArg9, pArg10, pArg11);
}

void gtk_paint_slider(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, int pArg8, int pArg9, int pArg10, int pArg11, int pArg12)
{
  gtk_paint_slider_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8, pArg9, pArg10, pArg11, pArg12);
}

void gtk_progress_bar_set_orientation(void *pArg1, int pArg2)
{
  gtk_progress_bar_set_orientation_ptr(pArg1, pArg2);
}

void *gtk_range_get_adjustment(void *pArg1)
{
  return gtk_range_get_adjustment_ptr(pArg1);
}

void gtk_style_apply_default_background(void *pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, int pArg6, int pArg7, int pArg8, int pArg9)
{
  gtk_style_apply_default_background_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8, pArg9);
}

void *gtk_style_attach(void *pArg1, void *pArg2)
{
  return gtk_style_attach_ptr(pArg1, pArg2);
}

void gtk_toggle_button_set_active(void *pArg1, int pArg2)
{
  gtk_toggle_button_set_active_ptr(pArg1, pArg2);
}

void gtk_toggle_button_set_inconsistent(void *pArg1, int pArg2)
{
  gtk_toggle_button_set_inconsistent_ptr(pArg1, pArg2);
}

void gtk_tooltips_force_window(void *pArg1)
{
  gtk_tooltips_force_window_ptr(pArg1);
}

void gtk_container_add(void *pArg1, void *pArg2)
{
  gtk_container_add_ptr(pArg1, pArg2);
}

void gtk_widget_destroy(void *pArg1)
{
  gtk_widget_destroy_ptr(pArg1);
}

void gtk_widget_realize(void *pArg1)
{
  gtk_widget_realize_ptr(pArg1);
}

void gtk_widget_set_state(void *pArg1, int pArg2)
{
  gtk_widget_set_state_ptr(pArg1, pArg2);
}

void gtk_widget_set_style(void *pArg1, void *pArg2)
{
  gtk_widget_set_style_ptr(pArg1, pArg2);
}

void gtk_widget_set_name(void *pArg1, void *pArg2)
{
  gtk_widget_set_name_ptr(pArg1, pArg2);
}

void gtk_widget_set_sensitive(void *pArg1, int pArg2)
{
  gtk_widget_set_sensitive_ptr(pArg1, pArg2);
}

void gtk_widget_class_install_style_property(void *pArg1, void *pArg2)
{
  gtk_widget_class_install_style_property_ptr(pArg1, pArg2);
}

void *gtk_window_new(int pArg1)
{
  return gtk_window_new_ptr(pArg1);
}

void gtk_object_unref(void *pArg1)
{
  gtk_object_unref_ptr(pArg1);
}

void gtk_main_do_event(void *pArg1)
{
  gtk_main_do_event_ptr(pArg1);
}

void gtk_widget_set_colormap(void *pArg1, void *pArg2)
{
  gtk_widget_set_colormap_ptr(pArg1, pArg2);
}

int gtk_dialog_run(void *pArg1)
{
  return gtk_dialog_run_ptr(pArg1);
}

int gtk_events_pending(void)
{
  return gtk_events_pending_ptr();
}

int gtk_main_iteration(void)
{
  return gtk_main_iteration_ptr();
}

void gtk_border_free(void *pArg1)
{
  gtk_border_free_ptr(pArg1);
}

void gtk_window_set_keep_above(void *pArg1, int pArg2)
{
  gtk_window_set_keep_above_ptr(pArg1, pArg2);
}

void gtk_window_set_modal(void *pArg1, int pArg2)
{
  gtk_window_set_modal_ptr(pArg1, pArg2);
}

int gtk_window_get_type(void)
{
  return gtk_window_get_type_ptr();
}

void *gtk_widget_get_parent_window(void *pArg1)
{
  return gtk_widget_get_parent_window_ptr(pArg1);
}

void *gdk_window_foreign_new(void *pArg1)
{
  return gdk_window_foreign_new_ptr(pArg1);
}

void gtk_requisition_free(void *pArg1)
{
  gtk_requisition_free_ptr(pArg1);
}

typedef void *(*gtk_color_selection_dialog_new_t)(void *pArg1);
gtk_color_selection_dialog_new_t gtk_color_selection_dialog_new_ptr = NULL;
typedef void (*gtk_color_selection_set_current_color_t)(void *pArg1, void *pArg2);
gtk_color_selection_set_current_color_t gtk_color_selection_set_current_color_ptr = NULL;
typedef void (*gtk_color_selection_get_current_color_t)(void *pArg1, void *pArg2);
gtk_color_selection_get_current_color_t gtk_color_selection_get_current_color_ptr = NULL;
typedef int (*gtk_color_selection_dialog_get_type_t)(void);
gtk_color_selection_dialog_get_type_t gtk_color_selection_dialog_get_type_ptr = NULL;
typedef int (*gtk_color_selection_get_type_t)(void);
gtk_color_selection_get_type_t gtk_color_selection_get_type_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_GTK_COLOR_DIALOG_NAME ""
#elif defined(_LINUX)
#define MODULE_GTK_COLOR_DIALOG_NAME "libgtk-x11-2.0.so.0"
#elif defined(_WINDOWS)
#define MODULE_GTK_COLOR_DIALOG_NAME ""
#endif

static module_t module_gtk_color_dialog = NULL;

int initialise_weak_link_gtk_color_dialog(void)
{
#if defined(_LINUX)
  if (!module_load("libgtk-x11-2.0.so.0", &module_gtk_color_dialog))
#else
  if (!module_load(MODULE_GTK_COLOR_DIALOG_NAME, &module_gtk_color_dialog))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libgtk-x11-2.0.so.0\n") ;
#endif
goto err;
}

  if (!module_resolve(module_gtk_color_dialog, SYMBOL_PREFIX "gtk_color_selection_dialog_new", (handler_t *)&gtk_color_selection_dialog_new_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_color_selection_dialog_new\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_color_dialog, SYMBOL_PREFIX "gtk_color_selection_set_current_color", (handler_t *)&gtk_color_selection_set_current_color_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_color_selection_set_current_color\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_color_dialog, SYMBOL_PREFIX "gtk_color_selection_get_current_color", (handler_t *)&gtk_color_selection_get_current_color_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_color_selection_get_current_color\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_color_dialog, SYMBOL_PREFIX "gtk_color_selection_dialog_get_type", (handler_t *)&gtk_color_selection_dialog_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_color_selection_dialog_get_type\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gtk_color_dialog, SYMBOL_PREFIX "gtk_color_selection_get_type", (handler_t *)&gtk_color_selection_get_type_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: gtk_color_selection_get_type\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_gtk_color_dialog != NULL)
    module_unload(module_gtk_color_dialog);

  return 0;
}

void *gtk_color_selection_dialog_new(void *pArg1)
{
  return gtk_color_selection_dialog_new_ptr(pArg1);
}

void gtk_color_selection_set_current_color(void *pArg1, void *pArg2)
{
  gtk_color_selection_set_current_color_ptr(pArg1, pArg2);
}

void gtk_color_selection_get_current_color(void *pArg1, void *pArg2)
{
  gtk_color_selection_get_current_color_ptr(pArg1, pArg2);
}

int gtk_color_selection_dialog_get_type(void)
{
  return gtk_color_selection_dialog_get_type_ptr();
}

int gtk_color_selection_get_type(void)
{
  return gtk_color_selection_get_type_ptr();
}

typedef int (*XineramaIsActive_t)(void *pArg1);
XineramaIsActive_t XineramaIsActive_ptr = NULL;
typedef void *(*XineramaQueryScreens_t)(void *pArg1, void *pArg2);
XineramaQueryScreens_t XineramaQueryScreens_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_XINERAMA_NAME ""
#elif defined(_LINUX)
#define MODULE_XINERAMA_NAME "libXinerama.so.1"
#elif defined(_WINDOWS)
#define MODULE_XINERAMA_NAME ""
#endif

static module_t module_Xinerama = NULL;

int initialise_weak_link_Xinerama(void)
{
#if defined(_LINUX)
  if (!module_load("libXinerama.so.1", &module_Xinerama))
#else
  if (!module_load(MODULE_XINERAMA_NAME, &module_Xinerama))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libXinerama.so.1\n") ;
#endif
goto err;
}

  if (!module_resolve(module_Xinerama, SYMBOL_PREFIX "XineramaIsActive", (handler_t *)&XineramaIsActive_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XineramaIsActive\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_Xinerama, SYMBOL_PREFIX "XineramaQueryScreens", (handler_t *)&XineramaQueryScreens_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XineramaQueryScreens\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_Xinerama != NULL)
    module_unload(module_Xinerama);

  return 0;
}

int XineramaIsActive(void *pArg1)
{
  return XineramaIsActive_ptr(pArg1);
}

void *XineramaQueryScreens(void *pArg1, void *pArg2)
{
  return XineramaQueryScreens_ptr(pArg1, pArg2);
}

typedef int (*XvQueryAdaptors_t)(void *pArg1, int pArg2, void *pArg3, void *pArg4);
XvQueryAdaptors_t XvQueryAdaptors_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_LIBXV_NAME ""
#elif defined(_LINUX)
#define MODULE_LIBXV_NAME "libXv.so.1"
#elif defined(_WINDOWS)
#define MODULE_LIBXV_NAME ""
#endif

static module_t module_libxv = NULL;

int initialise_weak_link_libxv(void)
{
#if defined(_LINUX)
  if (!module_load("libXv.so.1", &module_libxv))
#else
  if (!module_load(MODULE_LIBXV_NAME, &module_libxv))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libXv.so.1\n") ;
#endif
goto err;
}

  if (!module_resolve(module_libxv, SYMBOL_PREFIX "XvQueryAdaptors", (handler_t *)&XvQueryAdaptors_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: XvQueryAdaptors\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_libxv != NULL)
    module_unload(module_libxv);

  return 0;
}

int XvQueryAdaptors(void *pArg1, int pArg2, void *pArg3, void *pArg4)
{
  return XvQueryAdaptors_ptr(pArg1, pArg2, pArg3, pArg4);
}

typedef int (*g_signal_connect_data_t)(void *pArg1, void *pArg2, void *pArg3, void *pArg4, void *pArg5, int pArg6);
g_signal_connect_data_t g_signal_connect_data_ptr = NULL;
typedef void *(*g_malloc_t)(int pArg1);
g_malloc_t g_malloc_ptr = NULL;
typedef void (*g_free_t)(void *pArg1);
g_free_t g_free_ptr = NULL;
typedef void *(*g_object_ref_t)(void *pArg1);
g_object_ref_t g_object_ref_ptr = NULL;
typedef void *(*g_object_unref_t)(void *pArg1);
g_object_unref_t g_object_unref_ptr = NULL;
typedef void *(*g_type_class_peek_t)(int pArg1);
g_type_class_peek_t g_type_class_peek_ptr = NULL;
typedef void *(*g_type_class_ref_t)(int pArg1);
g_type_class_ref_t g_type_class_ref_ptr = NULL;
typedef void (*g_type_class_unref_t)(void *pArg1);
g_type_class_unref_t g_type_class_unref_ptr = NULL;
typedef void *(*g_type_check_instance_cast_t)(void *pArg1, int pArg2);
g_type_check_instance_cast_t g_type_check_instance_cast_ptr = NULL;
typedef int (*g_type_check_instance_is_a_t)(void *pArg1, int pArg2);
g_type_check_instance_is_a_t g_type_check_instance_is_a_ptr = NULL;
typedef void (*g_type_init_t)(void);
g_type_init_t g_type_init_ptr = NULL;
void *g_object_get_ptr = NULL;
typedef void (*g_object_get_valist_t)(void *pArg1, void *pArg2, void *pArg3);
g_object_get_valist_t g_object_get_valist_ptr = NULL;
typedef void (*g_object_add_weak_pointer_t)(void *pArg1, void *pArg2);
g_object_add_weak_pointer_t g_object_add_weak_pointer_ptr = NULL;
typedef void (*g_object_set_data_t)(void *pArg1, void *pArg2, void *pArg3);
g_object_set_data_t g_object_set_data_ptr = NULL;
typedef void *(*g_param_spec_boolean_t)(void *pArg1, void *pArg2, void *pArg3, int pArg4, int pArg5);
g_param_spec_boolean_t g_param_spec_boolean_ptr = NULL;
typedef int (*g_timeout_add_t)(int pArg1, void *pArg2, void *pArg3);
g_timeout_add_t g_timeout_add_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_GOBJECT_NAME ""
#elif defined(_LINUX)
#define MODULE_GOBJECT_NAME "libgobject-2.0.so.0"
#elif defined(_WINDOWS)
#define MODULE_GOBJECT_NAME ""
#endif

static module_t module_gobject = NULL;

int initialise_weak_link_gobject(void)
{
#if defined(_LINUX)
  if (!module_load("libgobject-2.0.so.0", &module_gobject))
#else
  if (!module_load(MODULE_GOBJECT_NAME, &module_gobject))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libgobject-2.0.so.0\n") ;
#endif
goto err;
}

  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_signal_connect_data", (handler_t *)&g_signal_connect_data_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_signal_connect_data\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_malloc", (handler_t *)&g_malloc_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_malloc\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_free", (handler_t *)&g_free_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_free\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_object_ref", (handler_t *)&g_object_ref_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_object_ref\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_object_unref", (handler_t *)&g_object_unref_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_object_unref\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_type_class_peek", (handler_t *)&g_type_class_peek_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_type_class_peek\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_type_class_ref", (handler_t *)&g_type_class_ref_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_type_class_ref\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_type_class_unref", (handler_t *)&g_type_class_unref_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_type_class_unref\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_type_check_instance_cast", (handler_t *)&g_type_check_instance_cast_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_type_check_instance_cast\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_type_check_instance_is_a", (handler_t *)&g_type_check_instance_is_a_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_type_check_instance_is_a\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_type_init", (handler_t *)&g_type_init_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_type_init\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_object_get", (handler_t *)&g_object_get_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_object_get\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_object_get_valist", (handler_t *)&g_object_get_valist_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_object_get_valist\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_object_add_weak_pointer", (handler_t *)&g_object_add_weak_pointer_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_object_add_weak_pointer\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_object_set_data", (handler_t *)&g_object_set_data_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_object_set_data\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_param_spec_boolean", (handler_t *)&g_param_spec_boolean_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_param_spec_boolean\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_gobject, SYMBOL_PREFIX "g_timeout_add", (handler_t *)&g_timeout_add_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: g_timeout_add\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_gobject != NULL)
    module_unload(module_gobject);

  return 0;
}

int g_signal_connect_data(void *pArg1, void *pArg2, void *pArg3, void *pArg4, void *pArg5, int pArg6)
{
  return g_signal_connect_data_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6);
}

void *g_malloc(int pArg1)
{
  return g_malloc_ptr(pArg1);
}

void g_free(void *pArg1)
{
  g_free_ptr(pArg1);
}

void *g_object_ref(void *pArg1)
{
  return g_object_ref_ptr(pArg1);
}

void *g_object_unref(void *pArg1)
{
  return g_object_unref_ptr(pArg1);
}

void *g_type_class_peek(int pArg1)
{
  return g_type_class_peek_ptr(pArg1);
}

void *g_type_class_ref(int pArg1)
{
  return g_type_class_ref_ptr(pArg1);
}

void g_type_class_unref(void *pArg1)
{
  g_type_class_unref_ptr(pArg1);
}

void *g_type_check_instance_cast(void *pArg1, int pArg2)
{
  return g_type_check_instance_cast_ptr(pArg1, pArg2);
}

int g_type_check_instance_is_a(void *pArg1, int pArg2)
{
  return g_type_check_instance_is_a_ptr(pArg1, pArg2);
}

void g_type_init(void)
{
  g_type_init_ptr();
}

void g_object_get_valist(void *pArg1, void *pArg2, void *pArg3)
{
  g_object_get_valist_ptr(pArg1, pArg2, pArg3);
}

void g_object_add_weak_pointer(void *pArg1, void *pArg2)
{
  g_object_add_weak_pointer_ptr(pArg1, pArg2);
}

void g_object_set_data(void *pArg1, void *pArg2, void *pArg3)
{
  g_object_set_data_ptr(pArg1, pArg2, pArg3);
}

void *g_param_spec_boolean(void *pArg1, void *pArg2, void *pArg3, int pArg4, int pArg5)
{
  return g_param_spec_boolean_ptr(pArg1, pArg2, pArg3, pArg4, pArg5);
}

int g_timeout_add(int pArg1, void *pArg2, void *pArg3)
{
  return g_timeout_add_ptr(pArg1, pArg2, pArg3);
}

typedef void (*pango_xft_render_layout_t)(void *pArg1, void *pArg2, void *pArg3, int pArg4, int pArg5);
pango_xft_render_layout_t pango_xft_render_layout_ptr = NULL;
typedef void (*pango_xft_render_layout_line_t)(void *pArg1, void *pArg2, void *pArg3, int pArg4, int pArg5);
pango_xft_render_layout_line_t pango_xft_render_layout_line_ptr = NULL;
typedef void *(*pango_xft_get_context_t)(void *pArg1, int pArg2);
pango_xft_get_context_t pango_xft_get_context_ptr = NULL;

#if defined(_MACOSX)
#define MODULE_PANGOXFT_NAME ""
#elif defined(_LINUX)
#define MODULE_PANGOXFT_NAME "libpangoxft-1.0.so.0"
#elif defined(_WINDOWS)
#define MODULE_PANGOXFT_NAME ""
#endif

static module_t module_pangoxft = NULL;

int initialise_weak_link_pangoxft(void)
{
#if defined(_LINUX)
  if (!module_load("libpangoxft-1.0.so.0", &module_pangoxft))
#else
  if (!module_load(MODULE_PANGOXFT_NAME, &module_pangoxft))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: libpangoxft-1.0.so.0\n") ;
#endif
goto err;
}

  if (!module_resolve(module_pangoxft, SYMBOL_PREFIX "pango_xft_render_layout", (handler_t *)&pango_xft_render_layout_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_xft_render_layout\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pangoxft, SYMBOL_PREFIX "pango_xft_render_layout_line", (handler_t *)&pango_xft_render_layout_line_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_xft_render_layout_line\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_pangoxft, SYMBOL_PREFIX "pango_xft_get_context", (handler_t *)&pango_xft_get_context_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: pango_xft_get_context\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_pangoxft != NULL)
    module_unload(module_pangoxft);

  return 0;
}

void pango_xft_render_layout(void *pArg1, void *pArg2, void *pArg3, int pArg4, int pArg5)
{
  pango_xft_render_layout_ptr(pArg1, pArg2, pArg3, pArg4, pArg5);
}

void pango_xft_render_layout_line(void *pArg1, void *pArg2, void *pArg3, int pArg4, int pArg5)
{
  pango_xft_render_layout_line_ptr(pArg1, pArg2, pArg3, pArg4, pArg5);
}

void *pango_xft_get_context(void *pArg1, int pArg2)
{
  return pango_xft_get_context_ptr(pArg1, pArg2);
}

}
