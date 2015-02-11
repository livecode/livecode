#ifndef __CODER__
#define __CODER__

typedef struct Coder *CoderRef;

bool CoderStart(const char *p_filename, CoderRef& r_coder);
bool CoderFinish(CoderRef self);

void CoderCancel(CoderRef self);

void CoderWriteLine(CoderRef self, const char *p_format, ...);
void CoderWrite(CoderRef self, const char *p_format, ...);

void CoderWriteStatement(CoderRef self, const char *p_format, ...);

void CoderBeginStatement(CoderRef self);
void CoderEndStatement(CoderRef self);

void CoderBeginPreprocessor(CoderRef self, const char *p_format, ...);
void CoderEndPreprocessor(CoderRef self, const char *p_format, ...);

void CoderBegin(CoderRef self, const char *p_format, ...);
void CoderEndBegin(CoderRef self, const char *p_format, ...);
void CoderEnd(CoderRef self, const char *p_format, ...);

void CoderBeginIf(CoderRef self, const char *p_format, ...);
void CoderElse(CoderRef self);
void CoderEndIf(CoderRef self);

void CoderPad(CoderRef self);

#endif
