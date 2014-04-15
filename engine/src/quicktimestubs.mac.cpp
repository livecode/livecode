#include <stdlib.h>
#include <stdio.h>
#include <cstring>

#if defined(_MACOSX)
#include <mach-o/dyld.h>
typedef const struct mach_header *module_t;
#define SYMBOL_PREFIX "_"
#elif defined(TARGET_SUBPLATFORM_IPHONE)
typedef void *module_t;
extern "C" void *IOS_LoadModule(const char *);
extern "C" void *IOS_ResolveSymbol(void *, const char *);
#define SYMBOL_PREFIX
#elif defined(_LINUX) || defined(TARGET_SUBPLATFORM_ANDROID)
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
  if (t_module == NULL)
  {
    uint32_t t_buffer_size;
    t_buffer_size = 0;
    _NSGetExecutablePath(NULL, &t_buffer_size);
    char *t_module_path;
    t_module_path = (char *) malloc(t_buffer_size + strlen(p_source) + 1);
    if (t_module_path != NULL)
    {
      if (_NSGetExecutablePath(t_module_path, &t_buffer_size) == 0)
      {
        char *t_last_slash;
        t_last_slash = t_module_path + t_buffer_size;
        for (uint32_t i = 0; i < t_buffer_size; i++)
        {
          if (*t_last_slash == '/')
          {
            *(t_last_slash + 1) = '\0';
            break;
          }
          t_last_slash--;
        }
        strcat(t_module_path, p_source);
        t_module = NSAddImage(t_module_path, NSADDIMAGE_OPTION_RETURN_ON_ERROR | NSADDIMAGE_OPTION_WITH_SEARCHING);
      }
      free(t_module_path);
    }
  }
#elif defined(TARGET_SUBPLATFORM_IPHONE)
  t_module = IOS_LoadModule(p_source);
#elif defined(_LINUX) || defined(TARGET_SUBPLATFORM_ANDROID)
  t_module = dlopen(p_source, RTLD_LAZY);
#elif defined(_WINDOWS)
  t_module = LoadLibraryA(p_source);
#endif
  if (t_module == NULL)
    return 0;
  *r_module = t_module;
  return 1;
}

static int module_unload(module_t p_module)
{
#if defined(TARGET_SUBPLATFORM_ANDROID)
  if (p_module != NULL)
    dlclose(p_module);
#endif
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
#elif defined(TARGET_SUBPLATFORM_IPHONE)
  t_handler = (handler_t)IOS_ResolveSymbol(p_module, p_name);
#elif defined(_LINUX) || defined(TARGET_SUBPLATFORM_ANDROID)
  t_handler = (handler_t)dlsym(p_module, p_name);
#elif defined(_WINDOWS)
  t_handler = (handler_t)GetProcAddress(p_module, p_name);
#endif
  if (t_handler == NULL)
    return 0;
  *r_handler = t_handler;
  return 1;
}

typedef int (*CDSequenceEnd_t)(int pArg1);
CDSequenceEnd_t CDSequenceEnd_ptr = NULL;
typedef int (*CDSequenceNewDataSource_t)(int pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7);
CDSequenceNewDataSource_t CDSequenceNewDataSource_ptr = NULL;
typedef int (*CDSequenceSetSourceData_t)(int pArg1, void *pArg2, int pArg3);
CDSequenceSetSourceData_t CDSequenceSetSourceData_ptr = NULL;
typedef int (*CDSequenceSetTimeBase_t)(int pArg1, void *pArg2);
CDSequenceSetTimeBase_t CDSequenceSetTimeBase_ptr = NULL;
typedef int (*ConvertMovieToDataRef_t)(void *pArg1, void *pArg2, void *pArg3, int pArg4, int pArg5, int pArg6, int pArg7, void *pArg8);
ConvertMovieToDataRef_t ConvertMovieToDataRef_ptr = NULL;
typedef int (*DecompressSequenceBeginS_t)(void *pArg1, void *pArg2, void *pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, void *pArg8, int pArg9, void *pArg10, int pArg11, int pArg12, void *pArg13);
DecompressSequenceBeginS_t DecompressSequenceBeginS_ptr = NULL;
typedef int (*DecompressSequenceFrameWhen_t)(int pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7);
DecompressSequenceFrameWhen_t DecompressSequenceFrameWhen_ptr = NULL;
typedef void (*DisposeMovie_t)(void *pArg1);
DisposeMovie_t DisposeMovie_ptr = NULL;
typedef void (*DisposeTimeBase_t)(void *pArg1);
DisposeTimeBase_t DisposeTimeBase_ptr = NULL;
typedef int (*EnterMovies_t)(void);
EnterMovies_t EnterMovies_ptr = NULL;
typedef void *(*GetMediaHandler_t)(void *pArg1);
GetMediaHandler_t GetMediaHandler_ptr = NULL;
typedef void (*GetMediaHandlerDescription_t)(void *pArg1, void *pArg2, void *pArg3, void *pArg4);
GetMediaHandlerDescription_t GetMediaHandlerDescription_ptr = NULL;
typedef void (*GetMediaSampleDescription_t)(void *pArg1, int pArg2, void *pArg3);
GetMediaSampleDescription_t GetMediaSampleDescription_ptr = NULL;
typedef void *(*GetMovieIndTrack_t)(void *pArg1, int pArg2);
GetMovieIndTrack_t GetMovieIndTrack_ptr = NULL;
typedef void *(*GetMovieIndTrackType_t)(void *pArg1, int pArg2, int pArg3, int pArg4);
GetMovieIndTrackType_t GetMovieIndTrackType_ptr = NULL;
typedef int (*GetMovieTrackCount_t)(void *pArg1);
GetMovieTrackCount_t GetMovieTrackCount_ptr = NULL;
typedef void *(*GetMovieUserData_t)(void *pArg1);
GetMovieUserData_t GetMovieUserData_ptr = NULL;
typedef int (*GetMovieVisualContext_t)(void *pArg1, void *pArg2);
GetMovieVisualContext_t GetMovieVisualContext_ptr = NULL;
typedef int (*GetMoviesError_t)(void);
GetMoviesError_t GetMoviesError_ptr = NULL;
typedef int (*GetTrackDuration_t)(void *pArg1);
GetTrackDuration_t GetTrackDuration_ptr = NULL;
typedef int (*GetTrackEnabled_t)(void *pArg1);
GetTrackEnabled_t GetTrackEnabled_ptr = NULL;
typedef int (*GetTrackID_t)(void *pArg1);
GetTrackID_t GetTrackID_ptr = NULL;
typedef void *(*GetTrackMedia_t)(void *pArg1);
GetTrackMedia_t GetTrackMedia_ptr = NULL;
typedef int (*GetTrackOffset_t)(void *pArg1);
GetTrackOffset_t GetTrackOffset_ptr = NULL;
typedef int (*GetUserDataItem_t)(void *pArg1, void *pArg2, int pArg3, int pArg4, int pArg5);
GetUserDataItem_t GetUserDataItem_ptr = NULL;
typedef int (*MCMovieChanged_t)(void *pArg1, void *pArg2);
MCMovieChanged_t MCMovieChanged_ptr = NULL;
typedef int (*MCSetActionFilterWithRefCon_t)(void *pArg1, void *pArg2, int pArg3);
MCSetActionFilterWithRefCon_t MCSetActionFilterWithRefCon_ptr = NULL;
typedef int (*MakeImageDescriptionForEffect_t)(int pArg1, void *pArg2);
MakeImageDescriptionForEffect_t MakeImageDescriptionForEffect_ptr = NULL;
typedef int (*MakeImageDescriptionForPixMap_t)(void *pArg1, void *pArg2);
MakeImageDescriptionForPixMap_t MakeImageDescriptionForPixMap_ptr = NULL;
typedef int (*MediaGetName_t)(void *pArg1, void *pArg2, int pArg3, void *pArg4);
MediaGetName_t MediaGetName_ptr = NULL;
typedef int (*MovieExportSetSampleDescription_t)(void *pArg1, void *pArg2, int pArg3);
MovieExportSetSampleDescription_t MovieExportSetSampleDescription_ptr = NULL;
typedef int (*NewMovieFromProperties_t)(int pArg1, void *pArg2, int pArg3, void *pArg4, void *pArg5);
NewMovieFromProperties_t NewMovieFromProperties_ptr = NULL;
typedef void *(*NewTimeBase_t)(void);
NewTimeBase_t NewTimeBase_ptr = NULL;
typedef int (*QTCopyAtomDataToPtr_t)(void *pArg1, int pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6);
QTCopyAtomDataToPtr_t QTCopyAtomDataToPtr_ptr = NULL;
typedef int (*QTCountChildrenOfType_t)(void *pArg1, int pArg2, int pArg3);
QTCountChildrenOfType_t QTCountChildrenOfType_ptr = NULL;
typedef int (*QTCreateStandardParameterDialog_t)(void *pArg1, void *pArg2, int pArg3, void *pArg4);
QTCreateStandardParameterDialog_t QTCreateStandardParameterDialog_ptr = NULL;
typedef int (*QTDismissStandardParameterDialog_t)(int pArg1);
QTDismissStandardParameterDialog_t QTDismissStandardParameterDialog_ptr = NULL;
typedef int (*QTDisposeAtomContainer_t)(void *pArg1);
QTDisposeAtomContainer_t QTDisposeAtomContainer_ptr = NULL;
typedef int (*QTFindChildByID_t)(void *pArg1, int pArg2, int pArg3, int pArg4, void *pArg5);
QTFindChildByID_t QTFindChildByID_ptr = NULL;
typedef int (*QTFindChildByIndex_t)(void *pArg1, int pArg2, int pArg3, int pArg4, void *pArg5);
QTFindChildByIndex_t QTFindChildByIndex_ptr = NULL;
typedef int (*QTGetAtomDataPtr_t)(void *pArg1, int pArg2, void *pArg3, void *pArg4);
QTGetAtomDataPtr_t QTGetAtomDataPtr_ptr = NULL;
typedef int (*QTGetEffectsList_t)(void *pArg1, int pArg2, int pArg3, int pArg4);
QTGetEffectsList_t QTGetEffectsList_ptr = NULL;
typedef int (*QTGetPixMapHandleRowBytes_t)(void *pArg1);
QTGetPixMapHandleRowBytes_t QTGetPixMapHandleRowBytes_ptr = NULL;
typedef int (*QTInsertChild_t)(void *pArg1, int pArg2, int pArg3, int pArg4, int pArg5, int pArg6, void *pArg7, void *pArg8);
QTInsertChild_t QTInsertChild_ptr = NULL;
typedef int (*QTIsStandardParameterDialogEvent_t)(void *pArg1, int pArg2);
QTIsStandardParameterDialogEvent_t QTIsStandardParameterDialogEvent_ptr = NULL;
typedef int (*QTLockContainer_t)(void *pArg1);
QTLockContainer_t QTLockContainer_ptr = NULL;
typedef int (*QTNewAtomContainer_t)(void *pArg1);
QTNewAtomContainer_t QTNewAtomContainer_ptr = NULL;
typedef int (*QTNewDataReferenceFromFullPathCFString_t)(void *pArg1, int pArg2, int pArg3, void *pArg4, void *pArg5);
QTNewDataReferenceFromFullPathCFString_t QTNewDataReferenceFromFullPathCFString_ptr = NULL;
typedef int (*QTNewGWorld_t)(void *pArg1, int pArg2, void *pArg3, void *pArg4, void *pArg5, int pArg6);
QTNewGWorld_t QTNewGWorld_ptr = NULL;
typedef int (*QTNewGWorldFromPtr_t)(void *pArg1, int pArg2, void *pArg3, void *pArg4, void *pArg5, int pArg6, void *pArg7, int pArg8);
QTNewGWorldFromPtr_t QTNewGWorldFromPtr_ptr = NULL;
typedef int (*QTSwapAtoms_t)(void *pArg1, int pArg2, int pArg3);
QTSwapAtoms_t QTSwapAtoms_ptr = NULL;
typedef int (*QTUnlockContainer_t)(void *pArg1);
QTUnlockContainer_t QTUnlockContainer_ptr = NULL;
typedef int (*QTVisualContextCopyImageForTime_t)(void *pArg1, void *pArg2, void *pArg3, void *pArg4);
QTVisualContextCopyImageForTime_t QTVisualContextCopyImageForTime_ptr = NULL;
typedef void (*RectMatrix_t)(void *pArg1, void *pArg2, void *pArg3);
RectMatrix_t RectMatrix_ptr = NULL;
typedef int (*SCGetInfo_t)(void *pArg1, int pArg2, void *pArg3);
SCGetInfo_t SCGetInfo_ptr = NULL;
typedef int (*SCRequestImageSettings_t)(void *pArg1);
SCRequestImageSettings_t SCRequestImageSettings_ptr = NULL;
typedef int (*SCSetInfo_t)(void *pArg1, int pArg2, void *pArg3);
SCSetInfo_t SCSetInfo_ptr = NULL;
typedef int (*SGGetSoundInputDriver_t)(void *pArg1);
SGGetSoundInputDriver_t SGGetSoundInputDriver_ptr = NULL;
typedef int (*SGIdle_t)(void *pArg1);
SGIdle_t SGIdle_ptr = NULL;
typedef int (*SGInitialize_t)(void *pArg1);
SGInitialize_t SGInitialize_ptr = NULL;
typedef int (*SGNewChannel_t)(void *pArg1, int pArg2, void *pArg3);
SGNewChannel_t SGNewChannel_ptr = NULL;
typedef int (*SGSetChannelUsage_t)(void *pArg1, int pArg2);
SGSetChannelUsage_t SGSetChannelUsage_ptr = NULL;
typedef int (*SGSetDataOutput_t)(void *pArg1, void *pArg2, int pArg3);
SGSetDataOutput_t SGSetDataOutput_ptr = NULL;
typedef int (*SGSetSoundInputParameters_t)(void *pArg1, int pArg2, int pArg3, int pArg4);
SGSetSoundInputParameters_t SGSetSoundInputParameters_ptr = NULL;
typedef int (*SGSetSoundInputRate_t)(void *pArg1, int pArg2);
SGSetSoundInputRate_t SGSetSoundInputRate_ptr = NULL;
typedef int (*SGSoundInputDriverChanged_t)(void *pArg1);
SGSoundInputDriverChanged_t SGSoundInputDriverChanged_ptr = NULL;
typedef int (*SGStartRecord_t)(void *pArg1);
SGStartRecord_t SGStartRecord_ptr = NULL;
typedef int (*SGStop_t)(void *pArg1);
SGStop_t SGStop_ptr = NULL;
typedef void (*SetMovieDrawingCompleteProc_t)(void *pArg1, int pArg2, void *pArg3, int pArg4);
SetMovieDrawingCompleteProc_t SetMovieDrawingCompleteProc_ptr = NULL;
typedef void (*SetTimeBaseRate_t)(void *pArg1, int pArg2);
SetTimeBaseRate_t SetTimeBaseRate_ptr = NULL;
typedef void (*SetTimeBaseValue_t)(void *pArg1, int pArg2, int pArg3);
SetTimeBaseValue_t SetTimeBaseValue_ptr = NULL;
typedef void (*SetTrackEnabled_t)(void *pArg1, int pArg2);
SetTrackEnabled_t SetTrackEnabled_ptr = NULL;

#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)
#define MODULE_QUICKTIME_NAME "QuickTime.framework"
#elif defined(_LINUX)
#define MODULE_QUICKTIME_NAME "none"
#elif defined(TARGET_SUBPLATFORM_ANDROID)
#define MODULE_QUICKTIME_NAME "libnone"
#elif defined(_WINDOWS)
#define MODULE_QUICKTIME_NAME "none"
#endif

static module_t module_QuickTime = NULL;

void finalise_weak_link_QuickTime(void)
{
  module_unload(module_QuickTime);
  module_QuickTime = NULL;
}

int initialise_weak_link_QuickTime_with_path(const char *p_path)
{
  module_QuickTime = NULL;
  if (!module_load(p_path, &module_QuickTime))
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: %s\n", p_path);
#endif
goto err;
}

  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "CDSequenceEnd", (handler_t *)&CDSequenceEnd_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: CDSequenceEnd\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "CDSequenceNewDataSource", (handler_t *)&CDSequenceNewDataSource_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: CDSequenceNewDataSource\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "CDSequenceSetSourceData", (handler_t *)&CDSequenceSetSourceData_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: CDSequenceSetSourceData\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "CDSequenceSetTimeBase", (handler_t *)&CDSequenceSetTimeBase_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: CDSequenceSetTimeBase\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "ConvertMovieToDataRef", (handler_t *)&ConvertMovieToDataRef_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: ConvertMovieToDataRef\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "DecompressSequenceBeginS", (handler_t *)&DecompressSequenceBeginS_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: DecompressSequenceBeginS\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "DecompressSequenceFrameWhen", (handler_t *)&DecompressSequenceFrameWhen_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: DecompressSequenceFrameWhen\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "DisposeMovie", (handler_t *)&DisposeMovie_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: DisposeMovie\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "DisposeTimeBase", (handler_t *)&DisposeTimeBase_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: DisposeTimeBase\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "EnterMovies", (handler_t *)&EnterMovies_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: EnterMovies\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetMediaHandler", (handler_t *)&GetMediaHandler_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetMediaHandler\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetMediaHandlerDescription", (handler_t *)&GetMediaHandlerDescription_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetMediaHandlerDescription\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetMediaSampleDescription", (handler_t *)&GetMediaSampleDescription_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetMediaSampleDescription\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetMovieIndTrack", (handler_t *)&GetMovieIndTrack_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetMovieIndTrack\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetMovieIndTrackType", (handler_t *)&GetMovieIndTrackType_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetMovieIndTrackType\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetMovieTrackCount", (handler_t *)&GetMovieTrackCount_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetMovieTrackCount\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetMovieUserData", (handler_t *)&GetMovieUserData_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetMovieUserData\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetMovieVisualContext", (handler_t *)&GetMovieVisualContext_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetMovieVisualContext\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetMoviesError", (handler_t *)&GetMoviesError_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetMoviesError\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetTrackDuration", (handler_t *)&GetTrackDuration_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetTrackDuration\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetTrackEnabled", (handler_t *)&GetTrackEnabled_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetTrackEnabled\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetTrackID", (handler_t *)&GetTrackID_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetTrackID\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetTrackMedia", (handler_t *)&GetTrackMedia_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetTrackMedia\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetTrackOffset", (handler_t *)&GetTrackOffset_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetTrackOffset\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "GetUserDataItem", (handler_t *)&GetUserDataItem_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: GetUserDataItem\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "MCMovieChanged", (handler_t *)&MCMovieChanged_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: MCMovieChanged\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "MCSetActionFilterWithRefCon", (handler_t *)&MCSetActionFilterWithRefCon_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: MCSetActionFilterWithRefCon\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "MakeImageDescriptionForEffect", (handler_t *)&MakeImageDescriptionForEffect_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: MakeImageDescriptionForEffect\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "MakeImageDescriptionForPixMap", (handler_t *)&MakeImageDescriptionForPixMap_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: MakeImageDescriptionForPixMap\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "MediaGetName", (handler_t *)&MediaGetName_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: MediaGetName\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "MovieExportSetSampleDescription", (handler_t *)&MovieExportSetSampleDescription_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: MovieExportSetSampleDescription\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "NewMovieFromProperties", (handler_t *)&NewMovieFromProperties_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: NewMovieFromProperties\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "NewTimeBase", (handler_t *)&NewTimeBase_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: NewTimeBase\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTCopyAtomDataToPtr", (handler_t *)&QTCopyAtomDataToPtr_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTCopyAtomDataToPtr\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTCountChildrenOfType", (handler_t *)&QTCountChildrenOfType_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTCountChildrenOfType\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTCreateStandardParameterDialog", (handler_t *)&QTCreateStandardParameterDialog_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTCreateStandardParameterDialog\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTDismissStandardParameterDialog", (handler_t *)&QTDismissStandardParameterDialog_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTDismissStandardParameterDialog\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTDisposeAtomContainer", (handler_t *)&QTDisposeAtomContainer_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTDisposeAtomContainer\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTFindChildByID", (handler_t *)&QTFindChildByID_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTFindChildByID\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTFindChildByIndex", (handler_t *)&QTFindChildByIndex_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTFindChildByIndex\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTGetAtomDataPtr", (handler_t *)&QTGetAtomDataPtr_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTGetAtomDataPtr\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTGetEffectsList", (handler_t *)&QTGetEffectsList_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTGetEffectsList\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTGetPixMapHandleRowBytes", (handler_t *)&QTGetPixMapHandleRowBytes_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTGetPixMapHandleRowBytes\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTInsertChild", (handler_t *)&QTInsertChild_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTInsertChild\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTIsStandardParameterDialogEvent", (handler_t *)&QTIsStandardParameterDialogEvent_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTIsStandardParameterDialogEvent\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTLockContainer", (handler_t *)&QTLockContainer_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTLockContainer\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTNewAtomContainer", (handler_t *)&QTNewAtomContainer_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTNewAtomContainer\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTNewDataReferenceFromFullPathCFString", (handler_t *)&QTNewDataReferenceFromFullPathCFString_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTNewDataReferenceFromFullPathCFString\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTNewGWorld", (handler_t *)&QTNewGWorld_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTNewGWorld\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTNewGWorldFromPtr", (handler_t *)&QTNewGWorldFromPtr_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTNewGWorldFromPtr\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTSwapAtoms", (handler_t *)&QTSwapAtoms_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTSwapAtoms\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTUnlockContainer", (handler_t *)&QTUnlockContainer_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTUnlockContainer\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "QTVisualContextCopyImageForTime", (handler_t *)&QTVisualContextCopyImageForTime_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTVisualContextCopyImageForTime\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "RectMatrix", (handler_t *)&RectMatrix_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: RectMatrix\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SCGetInfo", (handler_t *)&SCGetInfo_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SCGetInfo\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SCRequestImageSettings", (handler_t *)&SCRequestImageSettings_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SCRequestImageSettings\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SCSetInfo", (handler_t *)&SCSetInfo_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SCSetInfo\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SGGetSoundInputDriver", (handler_t *)&SGGetSoundInputDriver_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SGGetSoundInputDriver\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SGIdle", (handler_t *)&SGIdle_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SGIdle\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SGInitialize", (handler_t *)&SGInitialize_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SGInitialize\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SGNewChannel", (handler_t *)&SGNewChannel_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SGNewChannel\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SGSetChannelUsage", (handler_t *)&SGSetChannelUsage_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SGSetChannelUsage\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SGSetDataOutput", (handler_t *)&SGSetDataOutput_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SGSetDataOutput\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SGSetSoundInputParameters", (handler_t *)&SGSetSoundInputParameters_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SGSetSoundInputParameters\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SGSetSoundInputRate", (handler_t *)&SGSetSoundInputRate_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SGSetSoundInputRate\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SGSoundInputDriverChanged", (handler_t *)&SGSoundInputDriverChanged_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SGSoundInputDriverChanged\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SGStartRecord", (handler_t *)&SGStartRecord_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SGStartRecord\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SGStop", (handler_t *)&SGStop_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SGStop\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SetMovieDrawingCompleteProc", (handler_t *)&SetMovieDrawingCompleteProc_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SetMovieDrawingCompleteProc\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SetTimeBaseRate", (handler_t *)&SetTimeBaseRate_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SetTimeBaseRate\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SetTimeBaseValue", (handler_t *)&SetTimeBaseValue_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SetTimeBaseValue\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QuickTime, SYMBOL_PREFIX "SetTrackEnabled", (handler_t *)&SetTrackEnabled_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: SetTrackEnabled\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_QuickTime != NULL)
    module_unload(module_QuickTime);

  return 0;
}

int initialise_weak_link_QuickTime(void)
{
#if defined(_LINUX)
  if (!initialise_weak_link_QuickTime_with_path("none"))
#else
  if (!initialise_weak_link_QuickTime_with_path(MODULE_QUICKTIME_NAME))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: none\n") ;
#endif
return 0;
}

return -1;
}

int CDSequenceEnd(int pArg1)
{
  return CDSequenceEnd_ptr(pArg1);
}

int CDSequenceNewDataSource(int pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7)
{
  return CDSequenceNewDataSource_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7);
}

int CDSequenceSetSourceData(int pArg1, void *pArg2, int pArg3)
{
  return CDSequenceSetSourceData_ptr(pArg1, pArg2, pArg3);
}

int CDSequenceSetTimeBase(int pArg1, void *pArg2)
{
  return CDSequenceSetTimeBase_ptr(pArg1, pArg2);
}

int ConvertMovieToDataRef(void *pArg1, void *pArg2, void *pArg3, int pArg4, int pArg5, int pArg6, int pArg7, void *pArg8)
{
  return ConvertMovieToDataRef_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8);
}

int DecompressSequenceBeginS(void *pArg1, void *pArg2, void *pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7, void *pArg8, int pArg9, void *pArg10, int pArg11, int pArg12, void *pArg13)
{
  return DecompressSequenceBeginS_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8, pArg9, pArg10, pArg11, pArg12, pArg13);
}

int DecompressSequenceFrameWhen(int pArg1, void *pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6, void *pArg7)
{
  return DecompressSequenceFrameWhen_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7);
}

void DisposeMovie(void *pArg1)
{
  DisposeMovie_ptr(pArg1);
}

void DisposeTimeBase(void *pArg1)
{
  DisposeTimeBase_ptr(pArg1);
}

int EnterMovies(void)
{
  return EnterMovies_ptr();
}

void *GetMediaHandler(void *pArg1)
{
  return GetMediaHandler_ptr(pArg1);
}

void GetMediaHandlerDescription(void *pArg1, void *pArg2, void *pArg3, void *pArg4)
{
  GetMediaHandlerDescription_ptr(pArg1, pArg2, pArg3, pArg4);
}

void GetMediaSampleDescription(void *pArg1, int pArg2, void *pArg3)
{
  GetMediaSampleDescription_ptr(pArg1, pArg2, pArg3);
}

void *GetMovieIndTrack(void *pArg1, int pArg2)
{
  return GetMovieIndTrack_ptr(pArg1, pArg2);
}

void *GetMovieIndTrackType(void *pArg1, int pArg2, int pArg3, int pArg4)
{
  return GetMovieIndTrackType_ptr(pArg1, pArg2, pArg3, pArg4);
}

int GetMovieTrackCount(void *pArg1)
{
  return GetMovieTrackCount_ptr(pArg1);
}

void *GetMovieUserData(void *pArg1)
{
  return GetMovieUserData_ptr(pArg1);
}

int GetMovieVisualContext(void *pArg1, void *pArg2)
{
  return GetMovieVisualContext_ptr(pArg1, pArg2);
}

int GetMoviesError(void)
{
  return GetMoviesError_ptr();
}

int GetTrackDuration(void *pArg1)
{
  return GetTrackDuration_ptr(pArg1);
}

int GetTrackEnabled(void *pArg1)
{
  return GetTrackEnabled_ptr(pArg1);
}

int GetTrackID(void *pArg1)
{
  return GetTrackID_ptr(pArg1);
}

void *GetTrackMedia(void *pArg1)
{
  return GetTrackMedia_ptr(pArg1);
}

int GetTrackOffset(void *pArg1)
{
  return GetTrackOffset_ptr(pArg1);
}

int GetUserDataItem(void *pArg1, void *pArg2, int pArg3, int pArg4, int pArg5)
{
  return GetUserDataItem_ptr(pArg1, pArg2, pArg3, pArg4, pArg5);
}

int MCMovieChanged(void *pArg1, void *pArg2)
{
  return MCMovieChanged_ptr(pArg1, pArg2);
}

int MCSetActionFilterWithRefCon(void *pArg1, void *pArg2, int pArg3)
{
  return MCSetActionFilterWithRefCon_ptr(pArg1, pArg2, pArg3);
}

int MakeImageDescriptionForEffect(int pArg1, void *pArg2)
{
  return MakeImageDescriptionForEffect_ptr(pArg1, pArg2);
}

int MakeImageDescriptionForPixMap(void *pArg1, void *pArg2)
{
  return MakeImageDescriptionForPixMap_ptr(pArg1, pArg2);
}

int MediaGetName(void *pArg1, void *pArg2, int pArg3, void *pArg4)
{
  return MediaGetName_ptr(pArg1, pArg2, pArg3, pArg4);
}

int MovieExportSetSampleDescription(void *pArg1, void *pArg2, int pArg3)
{
  return MovieExportSetSampleDescription_ptr(pArg1, pArg2, pArg3);
}

int NewMovieFromProperties(int pArg1, void *pArg2, int pArg3, void *pArg4, void *pArg5)
{
  return NewMovieFromProperties_ptr(pArg1, pArg2, pArg3, pArg4, pArg5);
}

void *NewTimeBase(void)
{
  return NewTimeBase_ptr();
}

int QTCopyAtomDataToPtr(void *pArg1, int pArg2, int pArg3, int pArg4, void *pArg5, void *pArg6)
{
  return QTCopyAtomDataToPtr_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6);
}

int QTCountChildrenOfType(void *pArg1, int pArg2, int pArg3)
{
  return QTCountChildrenOfType_ptr(pArg1, pArg2, pArg3);
}

int QTCreateStandardParameterDialog(void *pArg1, void *pArg2, int pArg3, void *pArg4)
{
  return QTCreateStandardParameterDialog_ptr(pArg1, pArg2, pArg3, pArg4);
}

int QTDismissStandardParameterDialog(int pArg1)
{
  return QTDismissStandardParameterDialog_ptr(pArg1);
}

int QTDisposeAtomContainer(void *pArg1)
{
  return QTDisposeAtomContainer_ptr(pArg1);
}

int QTFindChildByID(void *pArg1, int pArg2, int pArg3, int pArg4, void *pArg5)
{
  return QTFindChildByID_ptr(pArg1, pArg2, pArg3, pArg4, pArg5);
}

int QTFindChildByIndex(void *pArg1, int pArg2, int pArg3, int pArg4, void *pArg5)
{
  return QTFindChildByIndex_ptr(pArg1, pArg2, pArg3, pArg4, pArg5);
}

int QTGetAtomDataPtr(void *pArg1, int pArg2, void *pArg3, void *pArg4)
{
  return QTGetAtomDataPtr_ptr(pArg1, pArg2, pArg3, pArg4);
}

int QTGetEffectsList(void *pArg1, int pArg2, int pArg3, int pArg4)
{
  return QTGetEffectsList_ptr(pArg1, pArg2, pArg3, pArg4);
}

int QTGetPixMapHandleRowBytes(void *pArg1)
{
  return QTGetPixMapHandleRowBytes_ptr(pArg1);
}

int QTInsertChild(void *pArg1, int pArg2, int pArg3, int pArg4, int pArg5, int pArg6, void *pArg7, void *pArg8)
{
  return QTInsertChild_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8);
}

int QTIsStandardParameterDialogEvent(void *pArg1, int pArg2)
{
  return QTIsStandardParameterDialogEvent_ptr(pArg1, pArg2);
}

int QTLockContainer(void *pArg1)
{
  return QTLockContainer_ptr(pArg1);
}

int QTNewAtomContainer(void *pArg1)
{
  return QTNewAtomContainer_ptr(pArg1);
}

int QTNewDataReferenceFromFullPathCFString(void *pArg1, int pArg2, int pArg3, void *pArg4, void *pArg5)
{
  return QTNewDataReferenceFromFullPathCFString_ptr(pArg1, pArg2, pArg3, pArg4, pArg5);
}

int QTNewGWorld(void *pArg1, int pArg2, void *pArg3, void *pArg4, void *pArg5, int pArg6)
{
  return QTNewGWorld_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6);
}

int QTNewGWorldFromPtr(void *pArg1, int pArg2, void *pArg3, void *pArg4, void *pArg5, int pArg6, void *pArg7, int pArg8)
{
  return QTNewGWorldFromPtr_ptr(pArg1, pArg2, pArg3, pArg4, pArg5, pArg6, pArg7, pArg8);
}

int QTSwapAtoms(void *pArg1, int pArg2, int pArg3)
{
  return QTSwapAtoms_ptr(pArg1, pArg2, pArg3);
}

int QTUnlockContainer(void *pArg1)
{
  return QTUnlockContainer_ptr(pArg1);
}

int QTVisualContextCopyImageForTime(void *pArg1, void *pArg2, void *pArg3, void *pArg4)
{
  return QTVisualContextCopyImageForTime_ptr(pArg1, pArg2, pArg3, pArg4);
}

void RectMatrix(void *pArg1, void *pArg2, void *pArg3)
{
  RectMatrix_ptr(pArg1, pArg2, pArg3);
}

int SCGetInfo(void *pArg1, int pArg2, void *pArg3)
{
  return SCGetInfo_ptr(pArg1, pArg2, pArg3);
}

int SCRequestImageSettings(void *pArg1)
{
  return SCRequestImageSettings_ptr(pArg1);
}

int SCSetInfo(void *pArg1, int pArg2, void *pArg3)
{
  return SCSetInfo_ptr(pArg1, pArg2, pArg3);
}

int SGGetSoundInputDriver(void *pArg1)
{
  return SGGetSoundInputDriver_ptr(pArg1);
}

int SGIdle(void *pArg1)
{
  return SGIdle_ptr(pArg1);
}

int SGInitialize(void *pArg1)
{
  return SGInitialize_ptr(pArg1);
}

int SGNewChannel(void *pArg1, int pArg2, void *pArg3)
{
  return SGNewChannel_ptr(pArg1, pArg2, pArg3);
}

int SGSetChannelUsage(void *pArg1, int pArg2)
{
  return SGSetChannelUsage_ptr(pArg1, pArg2);
}

int SGSetDataOutput(void *pArg1, void *pArg2, int pArg3)
{
  return SGSetDataOutput_ptr(pArg1, pArg2, pArg3);
}

int SGSetSoundInputParameters(void *pArg1, int pArg2, int pArg3, int pArg4)
{
  return SGSetSoundInputParameters_ptr(pArg1, pArg2, pArg3, pArg4);
}

int SGSetSoundInputRate(void *pArg1, int pArg2)
{
  return SGSetSoundInputRate_ptr(pArg1, pArg2);
}

int SGSoundInputDriverChanged(void *pArg1)
{
  return SGSoundInputDriverChanged_ptr(pArg1);
}

int SGStartRecord(void *pArg1)
{
  return SGStartRecord_ptr(pArg1);
}

int SGStop(void *pArg1)
{
  return SGStop_ptr(pArg1);
}

void SetMovieDrawingCompleteProc(void *pArg1, int pArg2, void *pArg3, int pArg4)
{
  SetMovieDrawingCompleteProc_ptr(pArg1, pArg2, pArg3, pArg4);
}

void SetTimeBaseRate(void *pArg1, int pArg2)
{
  SetTimeBaseRate_ptr(pArg1, pArg2);
}

void SetTimeBaseValue(void *pArg1, int pArg2, int pArg3)
{
  SetTimeBaseValue_ptr(pArg1, pArg2, pArg3);
}

void SetTrackEnabled(void *pArg1, int pArg2)
{
  SetTrackEnabled_ptr(pArg1, pArg2);
}

void *QTMovieSelectionDidChangeNotification_ptr = NULL;
void *QTMovieDidEndNotification_ptr = NULL;
void *QTMovieTimeDidChangeNotification_ptr = NULL;
void *QTMovieRateDidChangeNotification_ptr = NULL;
void *QTMakeTime_ptr = NULL;
void *QTMovieFileNameAttribute_ptr = NULL;
void *QTMovieLoopsAttribute_ptr = NULL;
void *QTMovieNaturalSizeAttribute_ptr = NULL;
void *QTMovieOpenAsyncOKAttribute_ptr = NULL;
void *QTMovieOpenAsyncRequiredAttribute_ptr = NULL;
void *QTMoviePlaysSelectionOnlyAttribute_ptr = NULL;
void *QTMovieURLAttribute_ptr = NULL;
void *QTTimeCompare_ptr = NULL;

#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)
#define MODULE_QTKIT_NAME "QTKit.framework"
#elif defined(_LINUX)
#define MODULE_QTKIT_NAME "none"
#elif defined(TARGET_SUBPLATFORM_ANDROID)
#define MODULE_QTKIT_NAME "libnone"
#elif defined(_WINDOWS)
#define MODULE_QTKIT_NAME "none"
#endif

static module_t module_QTKit = NULL;

void finalise_weak_link_QTKit(void)
{
  module_unload(module_QTKit);
  module_QTKit = NULL;
}

int initialise_weak_link_QTKit_with_path(const char *p_path)
{
  module_QTKit = NULL;
  if (!module_load(p_path, &module_QTKit))
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: %s\n", p_path);
#endif
goto err;
}

  if (!module_resolve(module_QTKit, SYMBOL_PREFIX "QTMovieSelectionDidChangeNotification", (handler_t *)&QTMovieSelectionDidChangeNotification_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTMovieSelectionDidChangeNotification\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QTKit, SYMBOL_PREFIX "QTMovieDidEndNotification", (handler_t *)&QTMovieDidEndNotification_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTMovieDidEndNotification\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QTKit, SYMBOL_PREFIX "QTMovieTimeDidChangeNotification", (handler_t *)&QTMovieTimeDidChangeNotification_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTMovieTimeDidChangeNotification\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QTKit, SYMBOL_PREFIX "QTMovieRateDidChangeNotification", (handler_t *)&QTMovieRateDidChangeNotification_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTMovieRateDidChangeNotification\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QTKit, SYMBOL_PREFIX "QTMakeTime", (handler_t *)&QTMakeTime_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTMakeTime\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QTKit, SYMBOL_PREFIX "QTMovieFileNameAttribute", (handler_t *)&QTMovieFileNameAttribute_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTMovieFileNameAttribute\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QTKit, SYMBOL_PREFIX "QTMovieLoopsAttribute", (handler_t *)&QTMovieLoopsAttribute_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTMovieLoopsAttribute\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QTKit, SYMBOL_PREFIX "QTMovieNaturalSizeAttribute", (handler_t *)&QTMovieNaturalSizeAttribute_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTMovieNaturalSizeAttribute\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QTKit, SYMBOL_PREFIX "QTMovieOpenAsyncOKAttribute", (handler_t *)&QTMovieOpenAsyncOKAttribute_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTMovieOpenAsyncOKAttribute\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QTKit, SYMBOL_PREFIX "QTMovieOpenAsyncRequiredAttribute", (handler_t *)&QTMovieOpenAsyncRequiredAttribute_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTMovieOpenAsyncRequiredAttribute\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QTKit, SYMBOL_PREFIX "QTMoviePlaysSelectionOnlyAttribute", (handler_t *)&QTMoviePlaysSelectionOnlyAttribute_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTMoviePlaysSelectionOnlyAttribute\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QTKit, SYMBOL_PREFIX "QTMovieURLAttribute", (handler_t *)&QTMovieURLAttribute_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTMovieURLAttribute\n"); 
#endif
goto err; 
}
  if (!module_resolve(module_QTKit, SYMBOL_PREFIX "QTTimeCompare", (handler_t *)&QTTimeCompare_ptr))
{
#ifdef _DEBUG
fprintf(stderr, "Unable to load: QTTimeCompare\n"); 
#endif
goto err; 
}

  return 1;

err:
  if (module_QTKit != NULL)
    module_unload(module_QTKit);

  return 0;
}

int initialise_weak_link_QTKit(void)
{
#if defined(_LINUX)
  if (!initialise_weak_link_QTKit_with_path("none"))
#else
  if (!initialise_weak_link_QTKit_with_path(MODULE_QTKIT_NAME))
#endif
{
#ifdef _DEBUG
    fprintf(stderr, "Unable to load library: none\n") ;
#endif
return 0;
}

return -1;
}

}
