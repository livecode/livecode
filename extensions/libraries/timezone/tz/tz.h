/* LIBRARY_DLLEXPORT should be applied to declarations.  LIBRARY_DLLEXPORT_DEF
 * should be applied to definitions. */
#ifdef _WIN32
/* On Windows, declaring something as having "dllexport" storage
 * modifies the naming of the corresponding symbol, so the export
 * attribute must be attached to declarations (and possibly to the
 * definition *as well* if no separate declaration appears) */
#  ifdef _MSC_VER
#    define LIBRARY_DLLEXPORT __declspec(dllexport)
#  else
#    define LIBRARY_DLLEXPORT __attribute__((dllexport))
#  endif
#  define LIBRARY_DLLEXPORT_DEF LIBRARY_DLLEXPORT
#else
/* On non-Windows platforms, the external visibility of a symbol is
 * simply a property of its definition (i.e. whether or not it should
 * appear in the list of exported symbols). */
#  define LIBRARY_DLLEXPORT
#  define LIBRARY_DLLEXPORT_DEF __attribute__((__visibility__("default"), __used__))
#endif


#ifndef __TZ_H__
#define __TZ_H__

/* Make this header file easier to include in C++ code */
#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_DLLEXPORT_DEF
void
tzsetlcl(char const *name);

LIBRARY_DLLEXPORT_DEF
struct tm *
localtime_r(const time_t *timep, struct tm *tmp);
    
LIBRARY_DLLEXPORT_DEF
struct tm *
gmtime_r(const time_t *timep, struct tm *tmp);
 
LIBRARY_DLLEXPORT_DEF
time_t
mktime_tz(struct tm *tmp);

LIBRARY_DLLEXPORT_DEF
bool tzsetdir(const char* dir);
    
#ifdef __cplusplus
}
#endif

#endif /* __TZ_H__ */
