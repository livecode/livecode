/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "graphics.h"
#include "stack.h"
#include "execpt.h"
#include "player.h"
#include "util.h"
#include "osspec.h"

#ifdef _WINDOWS_DESKTOP
#include "w32prefix.h"
#include "w32dc.h"
#include "w32context.h"

#include "digitalv.h"
#include "QTML.h"
#include <Movies.h>
#include <MediaHandlers.h>
#include <QuickTimeVR.h>
#include <QuickTimeVRFormat.h>
#include <Endian.h>
#include <QuickTimeComponents.h>
#include <ImageCodec.h>

#define PIXEL_FORMAT_32 k32BGRAPixelFormat

OSErr MCS_path2FSSpec(const char *fname, FSSpec *fspec);

#elif defined(_MAC_DESKTOP)
#include "osxprefix.h"

#ifdef __LITTLE_ENDIAN__
#define PIXEL_FORMAT_32 k32BGRAPixelFormat
#else
#define PIXEL_FORMAT_32 k32ARGBPixelFormat
#endif

#endif

#ifdef FEATURE_QUICKTIME_EFFECTS

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	char *token;
	long type;
}
QTEffect;

static bool s_qt_initted = false;
static QTEffect *qteffects = NULL;
static uint2 neffects = 0;

bool MCQTInit(void);
static void MCQTFinit(void);

//////////

extern "C" int initialise_weak_link_QuickTime(void);
extern "C" int initialise_weak_link_QTKit(void);

/////////

bool MCQTInit(void)
{
	if (MCdontuseQT)
		return false;
	
	if (s_qt_initted)
		return true;
	
    if (initialise_weak_link_QuickTime() == 0 ||
        initialise_weak_link_QTKit() == 0)
    {
        s_qt_initted = false;
        return false;
    }
        
#ifdef _WINDOWS
	if (InitializeQTML(0L) != noErr || EnterMovies() != noErr)
		s_qt_initted = false;
	else
	{
		s_qt_initted = true;
	}
#elif defined _MACOSX
	if (EnterMovies() != noErr)
		s_qt_initted = false;
	else
	{
		s_qt_initted = true;
	}
#endif
	
	return s_qt_initted;
}

void MCQTFinit(void)
{	
	if (qteffects != NULL)
	{
		uint2 i;
		for (i=0;i < neffects; i++)
			delete qteffects[i].token;
		delete qteffects;
		qteffects = NULL;
		neffects = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////

// Related class variables
static void *sgSoundComp = NULL;
static long sgSndDriver = 0;
static const char *recordtempfile = NULL;
static char *recordexportfile = NULL;

// Utility functions
static SampleDescriptionHandle scanSoundTracks(Movie tmovie)
{
	short trackCount, index;
	SampleDescriptionHandle aDesc = NULL;
	trackCount = (short)GetMovieTrackCount(tmovie);
	for (index = 1 ; index <= trackCount ; index++)
	{
		OSType aTrackType;
		Track aTrack = NULL;
		Media aMedia = NULL;
		aTrack = GetMovieIndTrack(tmovie, index);
		aMedia = GetTrackMedia(aTrack);
		GetMediaHandlerDescription(aMedia, &aTrackType, 0, 0);
		if (aTrackType == SoundMediaType)
		{
			aDesc = (SampleDescriptionHandle)NewHandle(sizeof(SoundDescription));
			GetMediaSampleDescription(aMedia, 1, aDesc);
			if (GetMoviesError() != noErr)
			{
				DisposeHandle((Handle)aDesc);
				aDesc = NULL;
				continue;
			}
		}
	}
	return aDesc;
}

static bool path_to_dataref(const char *p_path, DataReferenceRecord& r_rec)
{
	bool t_success = true;
	CFStringRef t_cf_path = NULL;
	t_cf_path = CFStringCreateWithCString(NULL, p_path, kCFStringEncodingWindowsLatin1);
	t_success = (t_cf_path != NULL);
	if (t_success)
	{
		OSErr t_error;
		t_error = QTNewDataReferenceFromFullPathCFString(t_cf_path, kQTNativeDefaultPathStyle, 0, &r_rec . dataRef, &r_rec . dataRefType);
		t_success = noErr == t_error;
	}
	CFRelease(t_cf_path);
	return t_success;
}

static void exportToSoundFile(const char *sourcefile, const char *destfile)
{
	bool t_success = true;
	SoundDescriptionHandle myDesc = NULL;
	ComponentResult result = 0;
	Movie tmovie = nil;
	
	char *t_src_resolved = NULL;
	char *t_dst_resolved = NULL;
	t_src_resolved = MCS_resolvepath(sourcefile);
	t_dst_resolved = MCS_resolvepath(destfile);
	t_success = (t_src_resolved != NULL && t_dst_resolved != NULL);
	
	DataReferenceRecord t_src_rec, t_dst_rec;
	t_src_rec.dataRef = NULL;
	t_dst_rec.dataRef = NULL;
	
	if (t_success)
	{
		t_success = path_to_dataref(t_src_resolved, t_src_rec) &&
		path_to_dataref(t_dst_resolved, t_dst_rec);
	}
	
	free(t_src_resolved);
	free(t_dst_resolved);
	
	Boolean isActive = true;
	QTVisualContextRef aVisualContext = NULL;
	QTNewMoviePropertyElement aMovieProperties[] = {
		{kQTPropertyClass_DataLocation, kQTDataLocationPropertyID_DataReference, sizeof(t_src_rec), &t_src_rec, 0},
		{kQTPropertyClass_NewMovieProperty, kQTNewMoviePropertyID_Active, sizeof(isActive), &isActive, 0},
		{kQTPropertyClass_Context, kQTContextPropertyID_VisualContext, sizeof(aVisualContext), &aVisualContext, 0},
	};
	
	if (t_success)
		t_success = noErr == NewMovieFromProperties(3, aMovieProperties, 0, NULL, &tmovie);
	
	if (t_success)
		myDesc = (SoundDescriptionHandle)scanSoundTracks(tmovie);
	
	if (myDesc)
	{
		//open movie export component
		MovieExportComponent exporter;
		Component c;
		ComponentDescription cd;
		cd.componentType = MovieExportType;
		switch (MCrecordformat)
		{
			case EX_WAVE:
				cd.componentSubType = kQTFileTypeWave;
				break;
			case EX_ULAW:
				cd.componentSubType = kQTFileTypeMuLaw;
				break;
			case EX_AIFF:
				cd.componentSubType = kQTFileTypeAIFF;
				break;
			default:
				cd.componentSubType = kQTFileTypeMovie;
				break;
		}
		cd.componentManufacturer = SoundMediaType;
		cd.componentFlags = canMovieExportFiles;
		cd.componentFlagsMask = canMovieExportFiles;
		c = FindNextComponent(nil, &cd);
		(**myDesc).numChannels = MCrecordchannels;
		(**myDesc).sampleSize = MCrecordsamplesize;
		(**myDesc).sampleRate = (uint32_t)(MCrecordrate * 1000 * 65536);
		exporter = nil;
		exporter = OpenComponent(c);
		result = MovieExportSetSampleDescription(exporter, (SampleDescriptionHandle)myDesc,
												 SoundMediaType);
		errno = ConvertMovieToDataRef(tmovie, 0, t_dst_rec . dataRef, t_dst_rec . dataRefType, cd.componentSubType,
									  0, 0, exporter);
		// try showUserSettingsDialog | movieToFileOnlyExport | movieFileSpecValid
		DisposeHandle((Handle) myDesc);
		if (exporter)
			CloseComponent(exporter);
		if (errno != noErr)
		{
			char buffer[26 + U4L];
			sprintf(buffer, "error %d exporting recording", errno);
			MCresult->copysvalue(buffer);
		}
	}
	
	if (t_src_rec.dataRef != NULL)
		DisposeHandle(t_src_rec.dataRef);
	if (t_dst_rec.dataRef != NULL)
		DisposeHandle(t_dst_rec.dataRef);
	
	DisposeMovie(tmovie);
}

void MCQTHandleRecord(void)
{
	if (MCrecording)
		SGIdle((SeqGrabComponent)sgSoundComp);
}

void MCQTStopRecording(void)
{
	if (MCrecording)
	{
		MCresult->clear();
		MCrecording = False;
		SGStop((SeqGrabComponent)sgSoundComp);
		if (sgSoundComp != NULL)
		{
			CloseComponent((SeqGrabComponent)sgSoundComp);
			sgSoundComp = NULL;
		}
#ifdef _WINDOWS
		if (MCrecordformat == EX_MOVIE)
			CopyFileA(recordtempfile,recordexportfile,False);
		else
#endif
		{
			MCS_unlink(recordexportfile);
			exportToSoundFile(recordtempfile, recordexportfile);
			MCS_unlink(recordtempfile);
		}
		recordexportfile = NULL;
		delete recordexportfile;
	}
}

void MCQTRecordSound(char *fname)
{
	if (!MCQTInit())
	{
		MCresult->sets("could not initialize quicktime");
		return;
	}
	MCQTStopRecording();//just in case
	FSSpec fspec;
	recordtempfile = MCS_tmpnam();
	recordexportfile = fname;
	MCS_path2FSSpec(recordtempfile, &fspec);
	OSType compressionType, inputSource;
	memcpy(&compressionType, MCrecordcompression, 4);
	compressionType = EndianU32_NtoB(compressionType);
	if (strequal(MCrecordinput, "dflt"))
		inputSource = 0; // fake "default" entry
	else
	{
		memcpy(&inputSource, MCrecordinput, 4);
		inputSource = EndianU32_NtoB(inputSource);
	}
	// bug in component can't sample at anything except 44.1KHz
	
	UnsignedFixed sampleRate = 44100 << 16;
#ifdef _WINDOWS
	
	if (MCrecordformat == EX_MOVIE)
	{
		short denominator = (short)(MAXINT2 / MCrecordrate);
		short numerator = (short)(MCrecordrate * denominator);
		sampleRate = FixRatio(numerator, denominator) * 1000;
	}
#endif
	
	short sampleSize = MCrecordsamplesize;
	short numChannels = MCrecordchannels;
	
	int t_flags;
	
	// MW-2008-03-15: [[ Bug 6076 ]] Make sure we create the file before we start recording to it
	//   otherwise no recording happens.
	FILE *t_file;
	t_file = fopen(recordtempfile, "w");
	if (t_file != NULL)
		fclose(t_file);
	
	t_flags = seqGrabDontPreAllocateFileSize | seqGrabAppendToFile;
	
	sgSoundComp = OpenDefaultComponent(SeqGrabComponentType, 0);
	errno = SGInitialize((SeqGrabComponent)sgSoundComp);
	if (errno == noErr)
	{
		SGChannel sgSoundChan;
		if ((errno = SGNewChannel((SeqGrabComponent)sgSoundComp, SoundMediaType, &sgSoundChan)) == noErr
			&& (errno = SGSetChannelUsage(sgSoundChan, seqGrabRecord)) == noErr
			&& (errno = SGSetSoundInputRate(sgSoundChan, sampleRate)) == noErr
			&& (errno = SGSetSoundInputParameters(sgSoundChan, sampleSize,
												  numChannels, compressionType)) == noErr
			&& (errno = SGSetDataOutput((SeqGrabComponent)sgSoundComp, &fspec, seqGrabToDisk | t_flags)) == noErr
			&& (!inputSource
				|| (errno = SPBSetDeviceInfo(SGGetSoundInputDriver(sgSoundChan),
											 siOSTypeInputSource, &inputSource)) == noErr)
			&& (errno = SGSoundInputDriverChanged(sgSoundChan)) == noErr)
		{
			sgSndDriver = SGGetSoundInputDriver(sgSoundChan);
			//turn on sound input metering
			uint2 meterState = 1;
			if ((errno = SPBSetDeviceInfo(sgSndDriver, siLevelMeterOnOff,
			                              (char *)&meterState)) == noErr)
				
				errno = SGStartRecord((SeqGrabComponent)sgSoundComp);
		}
	}
	
	if (errno == noErr)
	{
		MCrecording = True;
		MCresult->clear(False);
	}
	else
	{
		char buffer[21 + U4L];
		sprintf(buffer, "error %d starting recording", errno);
		MCresult->copysvalue(buffer);
		if (sgSoundComp != NULL)
		{
			CloseComponent((SeqGrabComponent)sgSoundComp);
			sgSoundComp = NULL;
		}
	}
}

void MCQTGetRecordLoudness(MCExecPoint &ep)
{
	uint2 rloudness = 0;
	if (MCrecording)
	{
		uint2 meterState[2];
		SPBGetDeviceInfo(sgSndDriver, siLevelMeterOnOff, (char *)&meterState);
		rloudness = (uint2)((meterState[1] * 100) / 255);
	}
	ep.setint(rloudness);
}

void MCQTGetRecordCompressionList(MCExecPoint &ep)
{
	ep.clear();
	
	if (!MCQTInit())
	{
		MCresult->sets("could not initialize quicktime");
		return;
	}
	Component component = 0;
	ComponentDescription desc, info;
	Handle name = NewHandle(0);
	desc.componentType = kSoundCompressor;
	desc.componentSubType = 0;
	desc.componentManufacturer = 0;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	ep.concatcstring("No compression,raw ", EC_RETURN, true);
	while ((component = FindNextComponent(component, &desc)) != NULL)
	{
		GetComponentInfo(component, &info, name, 0, 0);
		if (GetHandleSize(name))
		{
			HLock(name);
			ep.concatcstring(p2cstr((unsigned char *)*name), EC_RETURN, false);
			char ssubtype[] = "????";
			long compType;
			compType = EndianU32_BtoN(info.componentSubType);
			memcpy(ssubtype, (char *)&compType, sizeof(OSType));
			ep.concatcstring(ssubtype, EC_COMMA, false);
			HUnlock(name);
		}
	}
	DisposeHandle(name);
}

// MW-2005-05-15: For consistency, added title field
void MCQTRecordDialog(MCExecPoint& ep, const char *p_title, Boolean sheet)
{
	if (!MCQTInit())
	{
		MCresult->sets("could not initialize quicktime");
		return;
	}
	ComponentInstance ci = OpenDefaultComponent(StandardCompressionType,
												StandardCompressionSubTypeSound);
	if (ci == NULL)
	{
		MCresult->sets("can't open dialog");
		return;
	}
	short denominator = (short)(MAXINT2 / MCrecordrate);
	short numerator = (short)(MCrecordrate * denominator);
	UnsignedFixed sampleRate = FixRatio(numerator, denominator) * 1000;
	short sampleSize = MCrecordsamplesize;
	short numChannels = MCrecordchannels;
	OSType compressionType;
	memcpy(&compressionType, MCrecordcompression, 4);
	compressionType = EndianU32_NtoB(compressionType);
	SCSetInfo(ci, scSoundSampleRateType, &sampleRate);
	SCSetInfo(ci, scSoundSampleSizeType, &sampleSize);
	SCSetInfo(ci, scSoundChannelCountType, &numChannels);
	SCSetInfo(ci, scSoundCompressionType, &compressionType);
	errno = SCRequestImageSettings(ci);
	if (errno == noErr)
	{
		SCGetInfo(ci, scSoundSampleRateType, &sampleRate);
		SCGetInfo(ci, scSoundSampleSizeType, &sampleSize);
		SCGetInfo(ci, scSoundChannelCountType, &numChannels);
		SCGetInfo(ci, scSoundCompressionType, &compressionType);
		MCrecordrate = (HiWord(sampleRate) + LoWord(sampleRate)
		                / (real8)MAXINT2) / 1000.0;
		compressionType = EndianU32_BtoN(compressionType);
		memcpy(MCrecordcompression, &compressionType, 4);
		MCrecordsamplesize = sampleSize;
		MCrecordchannels = numChannels;
	}
	else
		if (errno == userCanceledErr)
		{
			MCresult->sets(MCcancelstring);
			return;
		}
		else
		{
			char buffer[22 + U4L];
			sprintf(buffer, "error %d opening dialog", errno);
			MCresult->copysvalue(buffer);
			return;
		}
	CloseComponent(ci);
}

////////////////////////////////////////////////////////////////////////////////

static CGrafPtr s_qt_target_port = nil;

static MCGImageRef s_qt_start_image = nil;
static CGrafPtr s_qt_start_port = NULL;

static MCGImageRef s_qt_end_image = nil;
static CGrafPtr s_qt_end_port = NULL;

static QTAtomContainer s_qt_effect_desc = NULL;

static ImageDescriptionHandle s_qt_sample_desc = NULL;
static ImageDescriptionHandle s_qt_start_desc = NULL;
static ImageDescriptionHandle s_qt_end_desc = NULL;
static TimeBase s_qt_timebase = NULL;
static ImageSequence s_qt_effect_seq = NULL;

static Boolean s_qt_reverse = False;

void MCQTEffectEnd(void);
static void QTEffectsQuery(void **effectatomptr);

Boolean MCQTEffectsDialog(MCExecPoint &ep, const char *p_title, Boolean sheet)
{
	ep.clear();
	if (!MCQTInit())
		return False;
	QTAtomContainer effectlist = NULL;
	QTEffectsQuery((void **)&effectlist);
	if (effectlist == NULL)
	{
		MCresult->sets("can't get effect list");
		return False;
	}
	OSErr result;
	QTAtomContainer effectdesc = NULL;
	QTParameterDialog createdDialogID;
	if (QTNewAtomContainer(&effectdesc) != noErr)
		return False;
	result = QTCreateStandardParameterDialog(effectlist, effectdesc,
											 0, &createdDialogID);
	while (result == noErr)
	{
		EventRecord theEvent;
		WaitNextEvent(everyEvent, &theEvent, 0, nil);
		result = QTIsStandardParameterDialogEvent(&theEvent, createdDialogID);
		switch (result)
		{
			case featureUnsupported:
				
			{
				result = noErr;
				switch (theEvent.what)
				{
					case updateEvt:
						BeginUpdate((WindowPtr)theEvent.message);
						EndUpdate((WindowPtr)theEvent.message);
						break;
				}
				break;
			}
			case codecParameterDialogConfirm:
			case userCanceledErr:
				QTDismissStandardParameterDialog(createdDialogID);
				createdDialogID =nil;
				break;
		}
	}
	if (result == userCanceledErr)
	{
		MCresult->sets(MCcancelstring);
		QTDisposeAtomContainer(effectlist);
		return False;
	}
	HLock((Handle)effectdesc);
	uint4 datasize = GetHandleSize(effectdesc) + sizeof(long) * 2;
	char *dataptr = ep.getbuffer(datasize);
	long *aLong = (long *)dataptr;
	HLock((Handle)effectdesc);
	aLong[0] = EndianU32_NtoB(datasize);
	aLong[1] = EndianU32_NtoB('qtfx');
	memcpy((char *)(dataptr + (sizeof(long) * 2)),
	       *effectdesc ,GetHandleSize(effectdesc));
	HUnlock((Handle)effectdesc);
	ep.setlength(datasize);
	MCU_base64encode(ep);
	QTDisposeAtomContainer(effectdesc);
	QTDisposeAtomContainer(effectlist);
	return True;
}


static int compare_qteffect(const void *a, const void *b)
{
	const QTEffect *qa;
	const QTEffect *qb;
	qa = (QTEffect *)a;
	qb = (QTEffect *)b;
	return strcmp(qa -> token, qb -> token);
}

void MCQTEffectsList(MCExecPoint &ep)
{
	ep.clear();
	
	if (!MCQTInit())
		return;
	
	QTEffectsQuery(NULL);
	
	// MW-2008-01-08: [[ Bug 5700 ]] Make sure the effect list is sorted alphabetically
	qsort(qteffects, neffects, sizeof(QTEffect), compare_qteffect);
	
	uint2 i;
	for (i = 0; i < neffects; i++)
		ep.concatcstring(qteffects[i].token, EC_RETURN, i == 0);
}

static void QTEffectsQuery(void **effectatomptr)
{
	if (qteffects != NULL && effectatomptr == NULL)
		return;
	QTAtomContainer effectatom = NULL;
	uint2 numeffects;
	// get a list of the available effects
	if  (QTNewAtomContainer(&effectatom) != noErr
		 || QTGetEffectsList(&effectatom, 2, -1, 0L) != noErr)
	{
		if (effectatom != NULL)
			QTDisposeAtomContainer(effectatom);
		return;
	}
	if (effectatomptr != NULL)
		*effectatomptr = effectatom;
	if (qteffects != NULL)
		return;
	
#if defined(_MACOSX) && defined(__LITTLE_ENDIAN__)
	// MW-2007-12-17: [[ Bug 3851 ]] For some reason the dissolve effect doesn't appear in the
	//   effects dialog on Mac Intel. So we just add it oursleves!
	
	OSType t_type;
	t_type = kCrossFadeTransitionType;
	QTInsertChild(effectatom, kParentAtomIsContainer, kEffectTypeAtom, 0, 0, sizeof(OSType), &t_type, NULL);
	QTInsertChild(effectatom, kParentAtomIsContainer, kEffectNameAtom, 0, 0, 10, (void *)"Cross Fade", NULL);
	t_type = kAppleManufacturer;
	QTInsertChild(effectatom, kParentAtomIsContainer, kEffectManufacturerAtom, 0, 0, sizeof(OSType), &t_type, NULL);
#endif
	
	// the returned effects list contains (at least) two atoms for each available effect component,
	// a name atom and a type atom; happily, this list is already sorted alphabetically by effect name
	numeffects = QTCountChildrenOfType(effectatom, kParentAtomIsContainer,
	                                   kEffectNameAtom);
	neffects = 0;
	qteffects = new QTEffect[numeffects];
	uint2 i;
	for (i = 1; i <= numeffects; i++)
	{
		QTAtom				nameatom = 0L;
		QTAtom				typeatom = 0L;
		nameatom = QTFindChildByIndex(effectatom, kParentAtomIsContainer,
		                              kEffectNameAtom, i, NULL);
		typeatom = QTFindChildByIndex(effectatom, kParentAtomIsContainer,
		                              kEffectTypeAtom, i, NULL);
		
		if (nameatom != 0L && typeatom != 0L)
		{
			long datasize;
			char *sptr;
			QTCopyAtomDataToPtr(effectatom, typeatom, false, sizeof(OSType),
			                    &qteffects[neffects].type, NULL);
			QTLockContainer(effectatom);
			QTGetAtomDataPtr(effectatom, nameatom, &datasize, (Ptr *)&sptr);
			qteffects[neffects].token = new char[datasize+1];
			memcpy(qteffects[neffects].token,sptr,datasize);
			qteffects[neffects].token[datasize] = '\0';
			QTUnlockContainer(effectatom);
			neffects++;
		}
	}
	
	if (effectatomptr == NULL)
		QTDisposeAtomContainer(effectatom);
}

static void QTEffectAddParameters(QTAtomContainer effectdescription,OSType theEffectType, Visual_effects dir,Boolean &reverse)
{
	OSType paramtype;
	reverse = False;
	long param = 0;
	switch (theEffectType)
	{
		case 'push'://push [right] [left] [top] [bottom]
		{
			switch (dir)
			{
				case VE_BOTTOM:
					param = 1;
					break;
				case VE_LEFT:
					param = 2;
					break;
				case VE_UP:
					param = 3;
					break;
				case VE_RIGHT:
					param = 4;
				default:
					break;
			}
			paramtype = 'from';
		}
			break;
		case 'smpt'://wipe [right] [left] [top] [bottom]
		{
			switch (dir)
			{
				case VE_LEFT:
					reverse = True;
				case VE_RIGHT:
					param = 1;
					break;
				case VE_UP:
					reverse = True;
				case VE_DOWN:
					param = 2;
				default:
					break;
			}
		}
		case 'smp2'://iris [open] [close]
		{
			switch (dir)
			{
				case VE_CLOSE:
					reverse = True;
				case VE_OPEN:
					param = 101;
				default:
					break;
			}
		}
		case 'smp3':
		case 'smp4':
			paramtype = 'wpID';
			break;
		default:
			paramtype = 0;
	}
	
	if (paramtype != 0 && param != 0)
	{
		param = EndianU32_NtoB(param);
		QTInsertChild(effectdescription, kParentAtomIsContainer, paramtype, 1, 0, sizeof(param), &param, NULL);
	}
	if (reverse)
	{
		QTAtom source1 = QTFindChildByIndex(effectdescription, kParentAtomIsContainer, kEffectSourceName, 1, NULL );
		QTAtom source2 = QTFindChildByIndex(effectdescription, kParentAtomIsContainer, kEffectSourceName, 2, NULL );
		if (source2)
			QTSwapAtoms(effectdescription,source1,source2);
	}
}

bool MCQTEffectBegin(Visual_effects p_type, const char *p_name, Visual_effects p_direction, MCGImageRef p_start, MCGImageRef p_end, const MCRectangle& p_area)
{
	if (MCdontuseQTeffects || !MCQTInit())
		return false;
	
	OSType qteffect;
	qteffect = 0;
	
	switch (p_type)
	{
		case VE_DISSOLVE:
			qteffect = 'dslv';
			break;
			
		case VE_IRIS:
			qteffect = 'smp2';
			return false;
			break;
			
		case VE_PUSH:
			qteffect = 'push';
			return false;
			break;
			
		case VE_WIPE:
			qteffect = 'smpt';
			return false;
			break;
			
		case VE_UNDEFINED:
		{
			uint2 i;
			QTEffectsQuery(NULL);
			QTEffect *teffects = qteffects;
			uint2 tsize = neffects;
			
			MCString effectname(p_name);
			for (i = 0 ; i < tsize; i++)
			{
				if (effectname == teffects[i].token)
				{
					qteffect = teffects[i].type;
					break;
				}
			}
			if (!qteffect && effectname.getlength() == 4)
			{
				memcpy(&qteffect, p_name, sizeof(OSType));
				qteffect = EndianU32_NtoB(qteffect);
			}
			else
			{
				MCExecPoint ep;
				ep.setsvalue(effectname);
				MCU_base64decode(ep);
				if (ep.getsvalue().getlength() > 8)
				{
					const char *dataptr = ep.getsvalue().getstring();
					long *aLong = (long *)dataptr;
					long datasize = EndianU32_BtoN(aLong[0]) - (sizeof(long)*2);
					OSType ostype = EndianU32_BtoN(aLong[1]);
					if (ostype == 'qtfx')
					{
						s_qt_effect_desc = NewHandle(datasize);
						HLock(s_qt_effect_desc);
						memcpy(*s_qt_effect_desc, (char *)(dataptr + (sizeof(long) * 2)), datasize);
						HUnlock(s_qt_effect_desc);
						QTAtom whatAtom = QTFindChildByID(s_qt_effect_desc, kParentAtomIsContainer, kParameterWhatName, kParameterWhatID, NULL);
						if (whatAtom)
						{
							QTCopyAtomDataToPtr(s_qt_effect_desc, whatAtom, true, sizeof(qteffect), &qteffect, NULL);
							qteffect = EndianU32_BtoN(qteffect);
						}
					}
				}
			}
		}
			break;
		default:
			break;
	}
	
	Rect t_src_rect, t_dst_rect;
	MacSetRect(&t_src_rect, 0, 0, p_area . width, p_area . height);
	MacSetRect(&t_dst_rect, 0, 0, p_area . width, p_area . height);
	
	if (qteffect != 0)
	{
		MCGRaster t_start_raster, t_end_raster;
		/* UNCHECKED */ MCGImageGetRaster(p_start, t_start_raster);
		QTNewGWorldFromPtr(&s_qt_start_port, PIXEL_FORMAT_32, &t_src_rect, nil, nil, 0, t_start_raster.pixels, t_start_raster.stride);
		
		/* UNCHECKED */ MCGImageGetRaster(p_end, t_end_raster);
		QTNewGWorldFromPtr(&s_qt_end_port, PIXEL_FORMAT_32, &t_src_rect, nil, nil, 0, t_end_raster.pixels, t_end_raster.stride);
		
		QTNewGWorld(&s_qt_target_port, PIXEL_FORMAT_32, &t_src_rect, nil, nil, 0);
	}
	
	if (s_qt_target_port != nil && s_qt_start_port != NULL && s_qt_end_port != NULL)
	{
		OSType effecttype;
		if (s_qt_effect_desc == NULL)
		{
			QTNewAtomContainer(&s_qt_effect_desc);
			effecttype = EndianU32_NtoB(qteffect);
			QTInsertChild(s_qt_effect_desc, kParentAtomIsContainer, kParameterWhatName, kParameterWhatID, 0, sizeof(effecttype), &effecttype, NULL);
		}
		
		effecttype = EndianU32_NtoB('srcA'); //source 1
		QTInsertChild(s_qt_effect_desc, kParentAtomIsContainer, kEffectSourceName, 1, 0, sizeof(effecttype), &effecttype, NULL);
		
		effecttype = EndianU32_NtoB('srcB'); //source 2
		QTInsertChild(s_qt_effect_desc, kParentAtomIsContainer, kEffectSourceName, 2, 0, sizeof(effecttype), &effecttype, NULL);
	}
	
	if (s_qt_effect_desc != NULL)
		MakeImageDescriptionForEffect(qteffect, &s_qt_sample_desc);
	
	if (s_qt_sample_desc != NULL)
	{				
		(**s_qt_sample_desc).vendor = kAppleManufacturer;
		(**s_qt_sample_desc).temporalQuality = codecNormalQuality;
		(**s_qt_sample_desc).spatialQuality = codecNormalQuality;
		(**s_qt_sample_desc).width = p_area . width;
		(**s_qt_sample_desc).height = p_area . height;
		QTEffectAddParameters(s_qt_effect_desc, qteffect, p_direction,	s_qt_reverse);
		
		MatrixRecord t_matrix;
		RectMatrix(&t_matrix, &t_src_rect, &t_dst_rect);
		
		HLock((Handle)s_qt_effect_desc);
		DecompressSequenceBeginS(&s_qt_effect_seq, s_qt_sample_desc,
								 *s_qt_effect_desc, GetHandleSize(s_qt_effect_desc),
								 s_qt_target_port, nil,
								 nil, &t_matrix, ditherCopy, nil,
								 0, codecNormalQuality, nil);
		HUnlock((Handle)s_qt_effect_desc);
	}
	
	if (s_qt_effect_seq != 0)
	{
		ImageSequenceDataSource t_src_sequence;
		
		t_src_sequence = 0;
		PixMapHandle t_src_pixmap = GetGWorldPixMap(s_qt_start_port);
		MakeImageDescriptionForPixMap(t_src_pixmap, &s_qt_start_desc);
		CDSequenceNewDataSource(s_qt_effect_seq, &t_src_sequence, 'srcA', 1, (Handle)s_qt_start_desc, nil, 0);
		CDSequenceSetSourceData(t_src_sequence, GetPixBaseAddr(t_src_pixmap), (**s_qt_start_desc) . dataSize);
	}
	
	if (s_qt_start_desc != NULL)
	{
		ImageSequenceDataSource t_src_sequence;
		
		t_src_sequence = 0;
		PixMapHandle t_end_pixmap = GetGWorldPixMap(s_qt_end_port);
		MakeImageDescriptionForPixMap(t_end_pixmap, &s_qt_end_desc);
		CDSequenceNewDataSource(s_qt_effect_seq, &t_src_sequence, 'srcB', 1, (Handle)s_qt_end_desc, nil, 0);
		CDSequenceSetSourceData(t_src_sequence, GetPixBaseAddr(t_end_pixmap), (**s_qt_end_desc) . dataSize);
	}
	
	if (s_qt_end_desc != NULL)
	{
		s_qt_timebase = NewTimeBase();
		SetTimeBaseRate(s_qt_timebase, 0);
		CDSequenceSetTimeBase(s_qt_effect_seq, s_qt_timebase);
	}
	
	if (s_qt_timebase == NULL)
	{
		MCQTEffectEnd();
		return false;
	}
	
	return true;
}

bool MCQTEffectStep(const MCRectangle &drect, MCStackSurface *p_target, uint4 p_delta, uint4 p_duration)
{
	ICMFrameTimeRecord t_frame_time;
	memset((char *)&t_frame_time, 0, sizeof(ICMFrameTimeRecord));
	SetTimeBaseValue(s_qt_timebase, p_delta, p_duration);
	
	if (s_qt_reverse)
		p_delta = p_duration - p_delta;
	else if (p_delta == 0)
		p_delta = 1;
	
	t_frame_time . recordSize = sizeof(ICMFrameTimeRecord);
	t_frame_time . flags = icmFrameTimeHasVirtualStartTimeAndDuration;
	t_frame_time . frameNumber = 1;
	t_frame_time . value . lo = p_delta;
	t_frame_time . scale = t_frame_time . duration = t_frame_time . virtualDuration = p_duration;
	HLock((Handle)s_qt_effect_desc);
	DecompressSequenceFrameWhen(s_qt_effect_seq, *(Handle)s_qt_effect_desc, GetHandleSize((Handle)s_qt_effect_desc), 0, 0, nil, &t_frame_time);
	HUnlock((Handle)s_qt_effect_desc);
	
	PixMapHandle t_pixmap = GetGWorldPixMap(s_qt_target_port);
	LockPixels(t_pixmap);
	void *t_bits = GetPixBaseAddr(t_pixmap);
	uint32_t t_stride = QTGetPixMapHandleRowBytes(t_pixmap);
	
	MCGRaster t_raster;
	t_raster.width = drect.width;
	t_raster.height = drect.height;
	t_raster.pixels = t_bits;
	t_raster.stride = t_stride;
	t_raster.format = kMCGRasterFormat_xRGB;
	
	MCGImageRef t_image = nil;
	/* UNCHECKED */ MCGImageCreateWithRasterNoCopy(t_raster, t_image);
	
	MCGRectangle t_src_rect, t_dst_rect;
	t_src_rect = MCGRectangleMake(0, 0, drect.width, drect.height);
	t_dst_rect = MCGRectangleTranslate(t_src_rect, drect.x, drect.y);
	
	p_target->Composite(t_dst_rect, t_image, t_src_rect, 1.0, kMCGBlendModeCopy);
	MCGImageRelease(t_image);
	
	UnlockPixels(t_pixmap);
	
	return true;
}

void MCQTEffectEnd(void)
{
	if (s_qt_effect_seq != 0)
		CDSequenceEnd(s_qt_effect_seq), s_qt_effect_seq = NULL;
	
	if (s_qt_timebase != NULL)
		DisposeTimeBase(s_qt_timebase), s_qt_timebase = NULL;
	
	if (s_qt_end_desc != NULL)
		DisposeHandle((Handle)s_qt_end_desc), s_qt_end_desc = NULL;
	
	if (s_qt_start_desc != NULL)
		DisposeHandle((Handle)s_qt_start_desc), s_qt_start_desc = NULL;
	
	if (s_qt_sample_desc != NULL)
		DisposeHandle((Handle)s_qt_sample_desc), s_qt_sample_desc = NULL;
	
	if (s_qt_target_port != NULL)
		DisposeGWorld(s_qt_target_port), s_qt_target_port = NULL;
	
	if (s_qt_end_port != NULL)
		DisposeGWorld(s_qt_end_port), s_qt_end_port = NULL;
	
	if (s_qt_start_port != NULL)
		DisposeGWorld(s_qt_start_port), s_qt_start_port = NULL;
	
	if (s_qt_effect_desc != NULL)
	{
		QTDisposeAtomContainer(s_qt_effect_desc);
		s_qt_effect_desc = NULL;
	}
}

#else

void MCQTEffectsList(MCExecPoint& ep)
{
	ep . clear();
}

Boolean MCQTEffectsDialog(MCExecPoint& ep, const char *p_title, Boolean p_sheet)
{
	return True;
}

void MCQTRecordSound(char *file)
{
	delete file;
	MCresult -> sets("not supported");
}

void MCQTGetRecordLoudness(MCExecPoint& ep)
{
	MCresult -> sets("not supported");
}

void MCQTGetRecordCompressionList(MCExecPoint& ep)
{
	MCresult -> sets("not supported");
}

void MCQTStopRecording(void)
{
}

void MCQTRecordDialog(MCExecPoint& ep, const char *p_title, Boolean sheet)
{
}

#endif

