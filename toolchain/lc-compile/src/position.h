#ifndef __POSITION__
#define __POSITION__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef long PositionRef;
typedef struct File *FileRef;

void InitializePosition(void);
void FinalizePosition(void);

void AdvanceCurrentPosition(long delta);
void AdvanceCurrentPositionToNextRow(void);
void AdvanceCurrentPositionToFile(FileRef file);

void GetColumnOfPosition(PositionRef position, long *r_column);
void GetRowOfPosition(PositionRef position, long *r_row);
void GetFileOfPosition(PositionRef position, FileRef *r_file);

void GetCurrentPosition(PositionRef *r_result);
void yyGetPos(PositionRef *r_result);

void InitializeFiles(void);
void FinalizeFiles(void);

void AddImportedModuleDir(const char *dir);
int AddImportedModuleFile(const char *name);
    
void AddFile(const char *filename);
int MoveToNextFile(void);
void GetFilePath(FileRef file, const char **r_path);
void GetFileName(FileRef file, const char **r_name);
void GetFileIndex(FileRef file, long *r_index);
int GetFileWithIndex(long index, FileRef *r_file);
int GetCurrentFile(FileRef *r_file);

void SetOutputFile(const char *filename);
void SetManifestOutputFile(const char *filename);
void SetInterfaceOutputFile(const char *filename);
void SetTemplateFile(const char *filename);
FILE *OpenOutputFile(void);
FILE *OpenInterfaceOutputFile(void);
FILE *OpenManifestOutputFile(void);
FILE *OpenTemplateFile(void);
FILE *OpenImportedModuleFile(const char *module);

#ifdef __cplusplus
}
#endif

#endif
