#ifndef _HAD_ZIPINTW32_H
#define _HAD_ZIPINTW32_H

typedef unsigned short mode_t;

#define fseeko fseek
#define strcasecmp _stricmp
#define snprintf _snprintf
#define fdopen _fdopen
#define strdup _strdup
#define fileno _fileno

#endif /* _HAD_ZIPINTW32_H */