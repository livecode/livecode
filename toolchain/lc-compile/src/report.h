#ifndef __REPORT__
#define __REPORT__

#ifdef __cplusplus
extern "C" {
#endif

void InitializeReports(void);
void FinalizeReports(void);

int ErrorsDidOccur(void);

void Fatal_OutOfMemory(void);
void Fatal_InternalInconsistency(const char *message);

void Error_CouldNotOpenInputFile(const char *path);
void Error_MalformedToken(long position, const char *token);
void Error_MalformedSyntax(long position);
    
#ifdef __cplusplus
}
#endif

#endif
