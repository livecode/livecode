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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "player.h"
#include "osspec.h"
#include "uidc.h"
#include "util.h"
#include "aclip.h"
#include "variable.h"

#include "exec.h"
#include "stack.h"
#include "card.h"

#include "vclip.h"

#include "platform.h"

////////////////////////////////////////////////////////////////////////////////

struct MCMultimediaRecordFormat
{
    intenum_t format;
};

struct MCMultimediaRecordFormatDataState
{
    intenum_t format;
    MCStringRef label;
};

static bool get_record_format_id(void *context, intenum_t p_id, MCStringRef p_label)
{
    auto *t_state = static_cast<MCMultimediaRecordFormatDataState *>(context);
    
    if (MCStringIsEqualTo(t_state -> label, p_label, kMCCompareCaseless))
    {
        t_state -> format = p_id;
    }
    
    return true;
}

static bool get_record_format_label(void *context, intenum_t p_id, MCStringRef p_label)
{
    auto *t_state = static_cast<MCMultimediaRecordFormatDataState *>(context);
    
    if (t_state -> format == p_id)
    {
        t_state -> label = p_label;
    }
    
    return true;
}


static void MCMultimediaRecordFormatParse(MCExecContext& ctxt, MCStringRef p_input, MCMultimediaRecordFormat& r_format)
{
    intenum_t t_format = 0;
#ifdef FEATURE_PLATFORM_RECORDER
    extern MCPlatformSoundRecorderRef MCrecorder;
    
    if (MCrecorder == nil)
        MCPlatformSoundRecorderCreate(MCrecorder);
    
    if (MCrecorder != nil)
    {
        MCMultimediaRecordFormatDataState t_state = { 0, p_input };
        MCPlatformSoundRecorderListFormats(MCrecorder, get_record_format_id, &t_state);
        t_format = t_state.format;
    }
#else
    extern intenum_t MCQTGetRecordFormatId(MCStringRef);
    t_format = MCQTGetRecordFormatId(p_input);
#endif
    
    r_format.format = t_format;
}

static void MCMultimediaRecordFormatFormat(MCExecContext& ctxt, const MCMultimediaRecordFormat& p_format, MCStringRef& r_output)
{
    MCStringRef t_label = kMCEmptyString;
#ifdef FEATURE_PLATFORM_RECORDER
    extern MCPlatformSoundRecorderRef MCrecorder;
    
    if (MCrecorder == nil)
        MCPlatformSoundRecorderCreate(MCrecorder);
    
    if (MCrecorder != nil)
    {
        MCMultimediaRecordFormatDataState t_state = { p_format.format, nil };
        MCPlatformSoundRecorderListFormats(MCrecorder, get_record_format_label, &t_state);
        t_label = t_state.label;
    }
#else
    extern MCStringRef MCQTGetRecordFormatLabel(intenum_t);
    t_label = MCQTGetRecordFormatLabel(p_format.format);
#endif
    
    r_output = MCValueRetain(t_label);
}

static void MCMultimediaRecordFormatInit(MCExecContext& ctxt, MCMultimediaRecordFormat& r_format)
{
    MCMemoryClear(&r_format, sizeof(MCMultimediaRecordFormat));
}

static void MCMultimediaRecordFormatFree(MCExecContext& ctxt, MCMultimediaRecordFormat& p_format)
{
}

static void MCMultimediaRecordFormatCopy(MCExecContext& ctxt, const MCMultimediaRecordFormat& p_source, MCMultimediaRecordFormat& r_target)
{
}

static bool MCMultimediaRecordFormatIsEqualTo(const MCMultimediaRecordFormat& p_left, const MCMultimediaRecordFormat& p_right)
{
    return p_left . format == p_right . format;
}

static MCExecCustomTypeInfo _kMCMultimediaRecordFormatTypeInfo =
{
    "Multimedia.RecordFormat",
    sizeof(MCMultimediaRecordFormat),
    (void *)MCMultimediaRecordFormatParse,
    (void *)MCMultimediaRecordFormatFormat,
    (void *)MCMultimediaRecordFormatFree,
};

MCExecCustomTypeInfo *kMCMultimediaRecordFormatTypeInfo = &_kMCMultimediaRecordFormatTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCMultimediaEvalQTVersion(MCExecContext& ctxt, MCStringRef& r_string)
{
    if (!MCtemplateplayer->getversion(r_string))
        ctxt.Throw();
}

// SN-2014-06-25: [[ PlatformPlayer ]] Refactoring functions from quicktime.cpp
void MCMultimediaEvalQTEffects(MCExecContext& ctxt, MCStringRef& r_result)
{
    extern void MCQTEffectsList(MCStringRef &r_string);
    MCQTEffectsList(r_result);
}

////////////////////////////////////////////////////////////////////////////////

#ifdef FEATURE_PLATFORM_RECORDER
static bool list_compressors_callback(void *context, unsigned int id, const char *label)
{
    MCListRef *t_state = static_cast<MCListRef *>(context);
    
    uint32_t t_id;
    t_id = MCSwapInt32NetworkToHost(id);
    
    char t_code[] = "????";
    memcpy(t_code, (char *)&t_id, 4);
    
    MCAutoStringRef t_compressor_info;
    MCStringFormat(&t_compressor_info, "%s,%s", label, t_code);
    
    MCListAppend(*t_state, *t_compressor_info);
    return true;
}

static bool list_formats_callback(void *context, intenum_t id, MCStringRef label)
{
    MCListRef *t_state = static_cast<MCListRef *>(context);
    return MCListAppend(*t_state, label);
}
#endif /* FEATURE_PLATFORM_RECORDER */

// SN-2014-06-25: [[ PlatformPlayer ]] Now calling the function from quicktime.cpp
void MCMultimediaEvalRecordCompressionTypes(MCExecContext& ctxt, MCStringRef& r_string)
{
#ifdef FEATURE_PLATFORM_RECORDER
    extern MCPlatformSoundRecorderRef MCrecorder;
    
    if (MCrecorder == nil)
        MCPlatformSoundRecorderCreate(MCrecorder);
    
    if (MCrecorder != nil)
    {
        MCListRef t_state;
        MCListCreateMutable('\n', t_state);
        
        MCPlatformSoundRecorderListCompressors(MCrecorder, list_compressors_callback, &t_state);
        
        MCListCopyAsString(t_state, r_string);
        MCValueRelease(t_state);
    }
	else
		// PM-2015-10-28: [[ Bug 16292 ]] Prevent crash and return empty if QT is not used
		r_string = MCValueRetain(kMCEmptyString);
#else
    extern void MCQTGetRecordCompressionList(MCStringRef &r_string);
    MCQTGetRecordCompressionList(r_string);
#endif
}

void MCMultimediaEvalRecordFormats(MCExecContext& ctxt, MCStringRef& r_string)
{
#ifdef FEATURE_PLATFORM_RECORDER
    extern MCPlatformSoundRecorderRef MCrecorder;
    
    if (MCrecorder == nullptr)
        MCPlatformSoundRecorderCreate(MCrecorder);
    
    if (MCrecorder == nullptr)
    {
        r_string = MCValueRetain(kMCEmptyString);
        return;
    }
    
    MCListRef t_state = nullptr;
    if (MCListCreateMutable('\n', t_state) &&
        MCPlatformSoundRecorderListFormats(MCrecorder,
                                           list_formats_callback,
                                           &t_state) &&
        MCListCopyAsStringAndRelease(t_state, r_string))
        return;
    
    MCValueRelease(t_state);
#else
    extern bool MCQTGetRecordFormatList(MCStringRef &r_string);
    if (MCQTGetRecordFormatList(r_string))
        return;
#endif
    ctxt . Throw();
}

// SN-2014-06-25: [[ PlatformPlayer ]] Now calling the function from quicktime.cpp
void MCMultimediaEvalRecordLoudness(MCExecContext& ctxt, integer_t& r_loudness)
{
#ifdef FEATURE_PLATFORM_RECORDER
    extern MCPlatformSoundRecorderRef MCrecorder;
    
    double t_loudness;
    t_loudness = 0;
    
    if (MCrecorder != nil)
        t_loudness = MCPlatformSoundRecorderGetLoudness(MCrecorder);
    
    r_loudness = floor(t_loudness);
    
#else
	extern void MCQTGetRecordLoudness(integer_t &r_loudness);
    MCQTGetRecordLoudness(r_loudness);
#endif
}

////////////////////////////////////////////////////////////////////////////////

void MCMultimediaEvalMovie(MCExecContext& ctxt, MCStringRef& r_string)
{
#ifdef X11
	IO_cleanprocesses();
#else

	real8 ctime = MCS_time();
	real8 etime = ctime;
	MCscreen->handlepending(ctime, etime, True);
#endif

	MCAutoListRef t_list;
	bool t_playing = false;
	bool t_success = true;
	if (MCplayers)
	{
		t_success = MCListCreateMutable('\n', &t_list);

		MCPlayerHandle t_player = MCplayers;
		while (t_success && t_player.IsValid())
		{
			if (t_player->isdisposable())
			{
				MCAutoValueRef t_string;
				t_success = t_player->names(P_NAME, &t_string) && MCListAppend(*t_list, *t_string);
				t_playing = true;
			}
			t_player = t_player->getnextplayer();
		}
	}
	if (t_success)
	{
		if (t_playing)
			t_success = MCListCopyAsString(*t_list, r_string);
		else
			t_success = MCStringCopy(MCNameGetString(MCN_done), r_string);
	}
	if (t_success)
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCMultimediaEvalMCISendString(MCExecContext& ctxt, MCStringRef p_command, MCStringRef& r_result)
{
	MCAutoStringRef t_result;
	bool t_error;
    t_error = false;
	if (MCS_mcisendstring(p_command, &t_result, t_error))
	{
		if (!t_error)
		{
			r_result = MCValueRetain(*t_result);
			ctxt.SetTheResultToEmpty();
		}
		else
		{
			r_result = MCValueRetain(kMCEmptyString);
			ctxt.SetTheResultToValue(*t_result);
		}

		return;
	}

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCMultimediaEvalSound(MCExecContext& ctxt, MCStringRef& r_sound)
{
#ifdef _MOBILE
	extern void MCSystemGetPlayingSound(MCStringRef &r_sound);
	MCStringRef t_sound;
	MCSystemGetPlayingSound(t_sound);
	if (MCStringIsEmpty(t_sound))
	{
		MCValueAssign(t_sound, MCSTR("done"));
	}
	r_sound = t_sound;
	return;
#else
    //SN-2014-12-11: [[ Merge-6.7.1-rc-4 ]]
    // Keep old behaviour if FEATURE_PLATFORM_AUDIO is not defined
#ifndef FEATURE_PLATFORM_AUDIO
    MCU_play();
#endif
    if (MCacptr && MCacptr -> isPlaying())
	{
		MCacptr -> getstringprop(ctxt, 0, P_NAME, False, r_sound);
		return;
	}
	else
	{
		/* UNCHECKED */ MCStringCreateWithCString("done", r_sound);
		return;
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////

void MCMultimediaExecStartPlayer(MCExecContext& ctxt, MCPlayer *p_target)
{
	p_target->playstart(kMCEmptyString);
}

////////////////////////////////////////////////////////////////////////////////

void MCMultimediaExecStopPlaying(MCExecContext& ctxt)
{
	MCU_play_stop();
}

void MCMultimediaExecStopPlayingObject(MCExecContext& ctxt, MCObject *p_object)
{
	if (p_object->gettype() == CT_PLAYER)
	{
		MCPlayer *player = (MCPlayer *)p_object;
		if (player->isdisposable())
			player->playstop();
		else
			player->playpause(True);
	}
	else
		MCU_play_stop();
}

// SN-2014-06-25 [[ PlatformPlayer ]] Now calling the function from quicktime.cpp
void MCMultimediaExecStopRecording(MCExecContext& ctxt)
{
#ifdef FEATURE_PLATFORM_RECORDER
    extern MCPlatformSoundRecorderRef MCrecorder;
    if (MCrecorder != nil)
        MCPlatformSoundRecorderStop(MCrecorder);
#else
    extern void MCQTStopRecording(void);
    MCQTStopRecording();
#endif
}

////////////////////////////////////////////////////////////////////////////////

// SN-2014-06-25: [[ PlatformPlayer ]] Now calling the function from quicktime.cpp
void MCMultimediaExecRecord(MCExecContext& ctxt, MCStringRef p_filename)
{
	if (!ctxt . EnsurePrivacyIsAllowed())
		return;

	MCAutoStringRef soundfile;
    MCS_resolvepath(p_filename, &soundfile);
    
#ifdef FEATURE_PLATFORM_RECORDER
    extern MCPlatformSoundRecorderRef MCrecorder;
    
    if (MCrecorder == nil)
        MCPlatformSoundRecorderCreate(MCrecorder);
    
    if (MCrecorder != nil)
        MCPlatformSoundRecorderStart(MCrecorder, *soundfile);
#else
	extern void MCQTRecordSound(MCStringRef soundfile);
	MCQTRecordSound(*soundfile);
#endif
}

void MCMultimediaExecRecordPause(MCExecContext& ctxt)
{
#ifdef FEATURE_PLATFORM_RECORDER
    extern MCPlatformSoundRecorderRef MCrecorder;
    if (MCrecorder != nil)
        MCPlatformSoundRecorderPause(MCrecorder);
#else
    extern void MCQTRecordPause(void);
    MCQTRecordPause();
#endif
}

void MCMultimediaExecRecordResume(MCExecContext& ctxt)
{
#ifdef FEATURE_PLATFORM_RECORDER
    extern MCPlatformSoundRecorderRef MCrecorder;
    if (MCrecorder != nil)
        MCPlatformSoundRecorderResume(MCrecorder);
#else
    extern void MCQTRecordResume(void);
    MCQTRecordResume();
#endif
}

////////////////////////////////////////////////////////////////////////////////

// SN-2014-06-25: [[ PlatformPlayer ]] Now calling the function from quicktime.cpp
void MCMultimediaExecAnswerEffect(MCExecContext &ctxt)
{
    MCresult -> clear(False);
    extern Boolean MCQTEffectsDialog(MCStringRef &r_data);
    
    MCAutoStringRef t_value;    
    if (MCQTEffectsDialog(&t_value))
        ctxt . SetItToValue(*t_value);
}

// SN-2014-06-25: [[ PlatformPlayer ]] Now calling the function from quicktime.cpp
void MCMultimediaExecAnswerRecord(MCExecContext &ctxt)
{
    MCresult -> clear(False);

#ifdef FEATURE_PLATFORM_RECORDER
    
    extern MCPlatformSoundRecorderRef MCrecorder;
    if (MCrecorder == nil)
        MCPlatformSoundRecorderCreate(MCrecorder);
    
    if (MCrecorder != nil)
    {
        MCPlatformSoundRecorderBeginConfigurationDialog(MCrecorder);
        
        MCPlatformDialogResult t_result;
        
        for (;;)
        {
            t_result = MCPlatformSoundRecorderEndConfigurationDialog(MCrecorder);
            if (t_result != kMCPlatformDialogResultContinue)
                break;
            
            MCscreen -> wait(REFRESH_INTERVAL, True, True);
        }

        if (t_result == kMCPlatformDialogResultCancel)
            MCresult->sets(MCcancelstring);
    }
#else
    extern void MCQTRecordDialog();
    MCQTRecordDialog();
#endif
}

////////////////////////////////////////////////////////////////////////////////

static MCPlayer* MCMultimediaExecGetClip(MCExecContext& ctxt, MCStringRef p_clip, int p_chunk_type)
{
	IO_cleanprocesses();
	// Lookup the name we are searching for. If it doesn't exist, then no object can
	// have it as a name.
	if (p_chunk_type == CT_EXPRESSION)
	{
        // AL-2014-05-27: [[ Bug 12517 ]] MCNameLookup does not increase the ref count
        MCNameRef t_name = MCNameLookupCaseless(p_clip);
        if (t_name == nullptr)
            return nullptr;
		return MCPlayer::FindPlayerByName(t_name);
	}
	
    if (p_chunk_type == CT_ID)
	{
        uint4 t_id;
        if (!MCU_stoui4(p_clip, t_id))
        {
        	ctxt . LegacyThrow(EE_PLAY_BADCLIP);
            return nullptr;
        }

        return MCPlayer::FindPlayerById(t_id);
	}

    MCUnreachableReturn(nullptr);
}

void MCMultimediaExecLoadVideoClip(MCExecContext& ctxt, MCStack *p_target, int p_chunk_type, MCStringRef p_filename, bool p_looping, MCPoint *p_at, MCStringRef p_options, bool p_prepare)
{
	MCAutoStringRef t_video_name;
	MCAutoStringRef t_temp;
	Boolean tmpfile = False;
	MCVideoClip *vcptr;
	MCPlayer *tptr;
	real8 scale;
	Boolean dontrefresh;
	MCStack *sptr;

	sptr = p_target != nil ? p_target : MCdefaultstackptr;

	MCNewAutoNameRef t_filename;
	/* UNCHECKED */ MCNameCreate(p_filename, &t_filename);
	if ((vcptr = (MCVideoClip *)(sptr->getAV((Chunk_term)p_chunk_type, p_filename, CT_VIDEO_CLIP))) == NULL &&
		(vcptr = (MCVideoClip *)(sptr->getobjname(CT_VIDEO_CLIP, *t_filename))) == NULL)
	{
		MCAutoValueRef t_file;
		bool t_url = true;
		if (MCStringGetLength(p_filename) < 4096)
		{
			/* UNCHECKED */ MCStringCopy(p_filename, &t_video_name);
			if (!MCS_exists(p_filename, True))
			{
				MCU_geturl(ctxt, p_filename, &t_file);
				if (MCValueIsEmpty(*t_file))
				{
					ctxt . SetTheResultToStaticCString("no data in videoClip");
					return;
				}
			}
			else
            {
                // AL-2014-05-27: [[ Bug 12517 ]] Set t_temp if this isn't a url
				t_url = false;
                t_temp = p_filename;
            }
		}
		else
            /* UNCHECKED */ t_file = p_filename;
		if (t_url)
		{
			/* UNCHECKED */ MCS_tmpnam(&t_temp);
			IO_handle t_stream;
			
			if ((t_stream = MCS_open(*t_temp, kMCOpenFileModeWrite, False, False, 0)) == NULL)
			{
				ctxt . SetTheResultToStaticCString("error opening temp file");
				return;
			}
			// TODO: The code around here looks quite wrong - it needs to be compared to the original.
			IO_stat stat = IO_ERROR; //IO_write_stringref_utf8(*t_file, t_stream);
			MCS_close(t_stream);
			if (stat != IO_NORMAL)
			{
				MCS_unlink(*t_temp);
				ctxt . SetTheResultToStaticCString("error writing videoClip");
				return;
			}
			tmpfile = True;
		}
		scale = MCtemplatevideo->getscale();
		dontrefresh = MCtemplatevideo->getflag(F_DONT_REFRESH);
	}
	else
	{
		/* UNCHECKED */ MCStringCopy(MCNameGetString(vcptr->getname()), &t_video_name);
	    /* UNCHECKED */ vcptr->getfile(&t_temp);
		scale = vcptr->getscale();
		dontrefresh = vcptr->getflag(F_DONT_REFRESH);
		tmpfile = True;
	}
	tptr = (MCPlayer *)MCtemplateplayer->clone(False, OP_NONE, false);
    tptr -> setboolprop(ctxt, 0, P_SHOW_BORDER, False, false);
	tptr->setfilename(*t_video_name, *t_temp, tmpfile);
	tptr->open();
	if (p_prepare)
		tptr->setflag(False, F_VISIBLE);
	MCRectangle trect = tptr->getrect();
	
	if (p_at != nil)
	{
		trect.x = p_at->x;
		trect.y = p_at->y;
	}
	else
	{
		MCRectangle crect = tptr->getcard()->getrect();
		trect.x = crect.width >> 1;
		trect.y = crect.height >> 1;
	}
	trect.width = trect.height = 1;
	tptr->setrect(trect);
	tptr->setscale(scale);
	tptr->setflag(dontrefresh, F_DONT_REFRESH);
	if (p_looping)
		tptr->setflag(True, F_LOOPING);
	if ((p_prepare && !tptr->prepare(p_options == nil ? kMCEmptyString : p_options)) ||
	    (!p_prepare && !tptr->playstart(p_options == nil ? kMCEmptyString : p_options)))
	{
		if (tptr->isdisposable())
			delete tptr;
	}
}

void MCMultimediaExecPrepareVideoClip(MCExecContext& ctxt, MCStack *p_target, int p_chunk_type, MCStringRef p_clip, bool p_looping, MCPoint *p_at, MCStringRef p_options)
{
	MCMultimediaExecLoadVideoClip(ctxt, p_target, p_chunk_type, p_clip, p_looping, p_at, p_options, true);
}

void MCMultimediaExecPlayAudioClip(MCExecContext& ctxt, MCStack *p_target, int p_chunk_type, MCStringRef p_clip, bool p_looping)
{
    // AL-2014-09-12: [[ Bug 13428 ]] Missed an MCU_play_stop() in the refactoring
    MCU_play_stop();
    
	MCStack *sptr;
	sptr = p_target != nil ? p_target : MCdefaultstackptr;

	if (!MCtemplateaudio->issupported())
	{
#ifdef _MOBILE
		extern bool MCSystemPlaySound(MCStringRef p_string, bool p_looping);
		if (!MCSystemPlaySound(p_clip, p_looping))
		MCresult->sets("no sound support");
#endif
		return;
	}
	
	MCNewAutoNameRef t_clipname;
	/* UNCHECKED */ MCNameCreate(p_clip, &t_clipname);
	if (!(MCacptr = MCObjectCast<MCAudioClip>(sptr->getAV((Chunk_term)p_chunk_type, p_clip, CT_AUDIO_CLIP)))
		&& !(MCacptr = MCObjectCast<MCAudioClip>(sptr->getobjname(CT_AUDIO_CLIP, *t_clipname))))
	{
		IO_handle stream;
		
		if (!MCS_exists(p_clip, True)
		        || (stream = MCS_open(p_clip, kMCOpenFileModeRead, True, False, 0)) == NULL)
		{
			MCAutoValueRef t_url;
            MCAutoDataRef t_data;
			MCU_geturl(ctxt, p_clip, &t_url);
			if (MCValueIsEmpty(*t_url))
			{
				ctxt . SetTheResultToStaticCString("no data in audioClip");
				return;
			}
            /* UNCHECKED */ ctxt . ConvertToData(*t_url, &t_data);
            stream = MCS_fakeopen(MCDataGetBytePtr(*t_data), MCDataGetLength(*t_data));
		}
		MCacptr = new (nothrow) MCAudioClip;
		MCacptr->setdisposable();
		if (!MCacptr->import(p_clip, stream))
		{
			MCS_close(stream);
			MCresult->sets("error reading audioClip");
			delete MCacptr;
			MCacptr = nil;
			ctxt . Throw();
			return;
		}
		MCS_close(stream);
	}
	MCacptr->setlooping(p_looping);
	MCU_play();
	if (MCacptr)
		MCscreen->addtimer(MCacptr, MCM_internal, p_looping ? LOOP_RATE : PLAY_RATE);
}

void MCMultimediaExecPlayStopAudio(MCExecContext& ctxt)
{
	MCU_play_stop(); 
}

void MCMultimediaExecPlayOperation(MCExecContext& ctxt, MCPlayer *p_player, int p_operation)
{
	if (p_player != nil)
	{	
		// PM-2015-07-10: [[ Bug 15472 ]] Preserve the visible/invisible state of the player
		switch (p_operation)
		{
		case PP_FORWARD:
			p_player->playstepforward();
			break;
		case PP_BACK:
			p_player->playstepback();
			break;
		case PP_PAUSE:
			p_player->playpause(True);
			break;
		case PP_RESUME:
		case PP_UNDEFINED:
			if (!p_player->playpause(False))
				p_player->playstop();
			break;
		case PP_STOP:
			if (p_player->isdisposable())
				p_player->playstop();
			else
				p_player->playpause(True);
			break;
		default:
			break;
		}
	}
	else
		ctxt . SetTheResultToStaticCString("videoClip is not playing");
}

void MCMultimediaExecPlayVideoClip(MCExecContext& ctxt, MCStack *p_target, int p_chunk_type, MCStringRef p_clip, bool p_looping, MCPoint *p_at, MCStringRef p_options)
{
#ifdef _MOBILE
	// PM-2015-09-22: [[ Bug 15969 ]] Playing a video on iOS crashes when touching the screen
	extern MCExecContext *MCECptr;
	
	// Add a new entry in the execution contexts
	MCExecContext *oldctxt = MCECptr;
	MCECptr = &ctxt;
	
	extern bool MCSystemPlayVideo(MCStringRef p_video);
	if (!MCSystemPlayVideo(p_clip))
		MCresult->sets("no video support");
	
	// Remove our entry from the contexts list
	MCECptr = oldctxt;
	
	return;
#endif

	MCPlayer *tptr = MCMultimediaExecGetClip(ctxt, p_clip, p_chunk_type);
	
	if (ctxt . HasError())
		return;
	
	if (tptr != nil)
		MCMultimediaExecPlayOperation(ctxt, tptr, PP_UNDEFINED);
	else
		MCMultimediaExecLoadVideoClip(ctxt, p_target, p_chunk_type, p_clip, p_looping, p_at, p_options, false);
}

void MCMultimediaExecPlayPlayerOperation(MCExecContext& ctxt, MCStack *p_target, int p_chunk_type, MCStringRef p_player, int p_player_chunk_type, int p_operation)
{
	IO_cleanprocesses();
	MCStack *t_stack = p_target == nil ? MCdefaultstackptr : p_target;
	MCPlayer *tptr = (MCPlayer *)t_stack->getcard()->getchild((Chunk_term)p_chunk_type, p_player, CT_PLAYER, (Chunk_term)p_player_chunk_type);
	MCMultimediaExecPlayOperation(ctxt, tptr, p_operation);
}

void MCMultimediaExecPlayVideoOperation(MCExecContext& ctxt, MCStack *p_target, int p_chunk_type, MCStringRef p_clip, int p_operation)
{
#ifdef _MOBILE
		extern bool MCSystemStopVideo();
		if (!MCSystemStopVideo())
			MCresult->sets("no video support");
		return;
#endif
	MCPlayer *tptr = MCMultimediaExecGetClip(ctxt, p_clip, p_chunk_type);
	
	if (ctxt . HasError())
		return;
	
	MCMultimediaExecPlayOperation(ctxt, tptr, p_operation);
}

void MCMultimediaExecPlayLastVideoOperation(MCExecContext& ctxt, int p_operation)
{
	MCMultimediaExecPlayOperation(ctxt, MCplayers, p_operation);
}

////////////////////////////////////////////////////////////////////////////////

void MCMultimediaGetRecordFormat(MCExecContext& ctxt, MCMultimediaRecordFormat& r_value)
{
    r_value.format = MCrecordformat;
}

void MCMultimediaSetRecordFormat(MCExecContext& ctxt, const MCMultimediaRecordFormat& p_value)
{
    MCrecordformat = p_value.format;
}

void MCMultimediaGetRecordCompression(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (MCStringCreateWithNativeChars((const char_t*)MCrecordcompression, 4, r_value))
		return;

	ctxt . Throw();
}

void MCMultimediaSetRecordCompression(MCExecContext& ctxt, MCStringRef p_value)
{
    MCAutoPointer<char> t_value;
    /* UNCHECKED */ MCStringConvertToCString(p_value, &t_value);
	memcpy(MCrecordcompression, *t_value, 4);
}

void MCMultimediaGetRecordInput(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (MCStringCreateWithNativeChars((const char_t*)MCrecordinput, 4, r_value))
		return;

	ctxt . Throw();
}

void MCMultimediaSetRecordInput(MCExecContext& ctxt, MCStringRef p_value)
{
    MCAutoPointer<char> t_value;
    /* UNCHECKED */ MCStringConvertToCString(p_value, &t_value);
	memcpy(MCrecordinput, *t_value, 4);
}

void MCMultimediaGetRecordSampleSize(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCrecordsamplesize;
}

void MCMultimediaSetRecordSampleSize(MCExecContext& ctxt, uinteger_t p_value)
{
	MCrecordsamplesize = (p_value <= 8) ? 8 : 16;
}

void MCMultimediaGetRecordChannels(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCrecordchannels;
}

void MCMultimediaSetRecordChannels(MCExecContext& ctxt, uinteger_t p_value)
{
	MCrecordchannels = (p_value <= 1) ? 1 : 2;
}

void MCMultimediaGetRecordRate(MCExecContext& ctxt, double& r_value)
{
	r_value = MCrecordrate;
}

void MCMultimediaSetRecordRate(MCExecContext& ctxt, double p_value)
{
	if (p_value <= (8.000 + 11.025) / 2.0)
		MCrecordrate = 8.000;
	else
		if (p_value <= (11.025 + 11.127) / 2.0)
			MCrecordrate = 11.025;
		else
			
			if (p_value <= (11.127 + 22.050) / 2.0)
				MCrecordrate = 12.000;
			else
				if (p_value <= (22.050 + 22.255) / 2.0)
					MCrecordrate = 22.050;
				else
					if (p_value <= (22.255 + 32.000) / 2.0)
						MCrecordrate = 24.000;
					else
						if (p_value <= (32.000 + 44.100) / 2.0)
							MCrecordrate = 32.000;
						else
							if (p_value <= (44.100 + 48.000) / 2.0)
								MCrecordrate = 44.100;
							else
								MCrecordrate = 48.000;
}

////////////////////////////////////////////////////////////////////////////////

void MCMultimediaGetPlayDestination(MCExecContext& ctxt, intenum_t& r_dest)
{
	MCtemplateaudio -> GetPlayDestination(ctxt, r_dest);
}

void MCMultimediaSetPlayDestination(MCExecContext& ctxt, intenum_t p_dest)
{
	MCtemplateaudio -> SetPlayDestination(ctxt, p_dest);
}

void MCMultimediaGetPlayLoudness(MCExecContext& ctxt, uinteger_t& r_loudness)
{
    // AL-2014-08-12: [[ Bug 13161 ]] Get the global playLoudness rather than templateAudioClip playLoudness
    uint2 t_loudness = 0;
    extern bool MCSystemGetPlayLoudness(uint2& r_loudness);
#ifdef _MOBILE
    if (MCSystemGetPlayLoudness(t_loudness))
#else
        if (false)
#endif
            ;
        else
            t_loudness = MCS_getplayloudness();
    
    r_loudness = t_loudness;
}

void MCMultimediaSetPlayLoudness(MCExecContext& ctxt, uinteger_t p_loudness)
{
    // AL-2014-08-12: [[ Bug 13161 ]] Setting templateAudioClip playLoudness shouldn't set the global playLoudness
    p_loudness = MCU_max(MCU_min((uint16_t)p_loudness, 100), 0);
        
    extern bool MCSystemSetPlayLoudness(uint2 loudness);
#ifdef _MOBILE
    if (MCSystemSetPlayLoudness(p_loudness))
        return;
#endif
    
    MCPlayer::SetPlayersVolume(p_loudness);
    
    MCS_setplayloudness(p_loudness);

	MCtemplateaudio -> SetPlayLoudness(ctxt, p_loudness);
}

////////////////////////////////////////////////////////////////////////////////

void MCMultimediaGetQtIdleRate(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCqtidlerate;
}

void MCMultimediaSetQtIdleRate(MCExecContext& ctxt, uinteger_t p_value)
{
	MCqtidlerate = MCU_max(p_value, (uint4)1);
}

void MCMultimediaGetDontUseQt(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCdontuseQT == True;
}

void MCMultimediaSetDontUseQt(MCExecContext& ctxt, bool p_value)
{
	MCdontuseQT = p_value ? True : False;
}

void MCMultimediaGetDontUseQtEffects(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCdontuseQTeffects == True;
}

void MCMultimediaSetDontUseQtEffects(MCExecContext& ctxt, bool p_value)
{
	MCdontuseQTeffects = p_value ? True : False;
}

////////////////////////////////////////////////////////////////////////////////

void MCMultimediaGetRecording(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCrecording == True;
}

// SN-2014-06-25: [[ PlatformPlayer ]] Refactoring functions from quicktime.cpp
void MCMultimediaSetRecording(MCExecContext& ctxt, bool p_value)
{
	if (!p_value)
    {
#ifdef FEATURE_PLATFORM_RECORDER
        extern MCPlatformSoundRecorderRef MCrecorder;
        if (MCrecorder != nil)
            MCPlatformSoundRecorderStop(MCrecorder);
#else
        extern void MCQTStopRecording(void);
        MCQTStopRecording();
#endif
    }
}
