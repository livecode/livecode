#ifndef __POSITION__
#define __POSITION__

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

void AddFile(const char *filename);
int MoveToNextFile(void);
void GetFilePath(FileRef file, const char **r_path);
void GetFileName(FileRef file, const char **r_name);
void GetFileIndex(FileRef file, long *r_index);
int GetFileWithIndex(long index, FileRef *r_file);
int GetCurrentFile(FileRef *r_file);

#endif
