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
#include "core.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "execpt.h"
#include "dispatch.h"
#include "stack.h"
#include "tooltip.h"
#include "card.h"
#include "field.h"
#include "button.h"
#include "image.h"
#include "aclip.h"
#include "vclip.h"
#include "stacklst.h"
#include "mcerror.h"
#include "hc.h"
#include "util.h"
#include "param.h"
#include "debug.h"
#include "statemnt.h"
#include "funcs.h"
#include "magnify.h"
#include "sellst.h"
#include "undolst.h"
#include "styledtext.h"
#include "external.h"
#include "osspec.h"
#include "flst.h"
#include "globals.h"
#include "license.h"
#include "mode.h"
#include "redraw.h"
#include "printer.h"
#include "font.h"
#include "stacksecurity.h"

#define UNLICENSED_TIME 6.0
#ifdef _DEBUG_MALLOC_INC
#define LICENSED_TIME 1.0
#else
#define LICENSED_TIME 3.0
#endif

MCImage *MCDispatch::imagecache;

#define VERSION_OFFSET 11

#define HEADERSIZE 255
static char header[HEADERSIZE] = "#!/bin/sh\n# MetaCard 2.4 stack\n# The following is not ASCII text,\n# so now would be a good time to q out of more\f\nexec mc $0 \"$@\"\n";

#define NEWHEADERSIZE 8
static const char *newheader = "REVO2700";
static const char *newheader5500 = "REVO5500";

MCDispatch::MCDispatch()
{
	license = NULL;
	stacks = NULL;
	fonts = NULL;
	setname_cstring("dispatch");
	handling = False;
	menu = NULL;
	panels = NULL;
	startdir = NULL;
	enginedir = NULL;
	flags = 0;

	m_drag_source = false;
	m_drag_target = false;
	m_drag_end_sent = false;

	m_externals = nil;
}

MCDispatch::~MCDispatch()
{	
	delete license;
	while (stacks != NULL)
	{
		MCStack *sptr = stacks->prev()->remove(stacks);
		delete sptr;
	}
	while (imagecache != NULL)
	{
		MCImage *iptr = imagecache->remove(imagecache);
		delete iptr;
	}
	delete fonts;
	
	delete startdir;
	delete enginedir;
	
	delete m_externals;
}

bool MCDispatch::isdragsource(void)
{
	return m_drag_source;
}

bool MCDispatch::isdragtarget(void)
{
	return m_drag_target;
}

Exec_stat MCDispatch::getprop(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
	switch (which)
	{
#ifdef /* MCDispatch::getprop */ LEGACY_EXEC
	case P_BACK_PIXEL:
		ep.setint(MCscreen->background_pixel.pixel & 0xFFFFFF);
		return ES_NORMAL;
	case P_TOP_PIXEL:
		ep.setint(MCscreen->white_pixel.pixel & 0xFFFFFF);
		return ES_NORMAL;
	case P_HILITE_PIXEL:
	case P_FORE_PIXEL:
	case P_BORDER_PIXEL:
	case P_BOTTOM_PIXEL:
	case P_SHADOW_PIXEL:
	case P_FOCUS_PIXEL:
		ep.setint(MCscreen->black_pixel.pixel & 0xFFFFFF);
		return ES_NORMAL;
	case P_BACK_COLOR:
	case P_HILITE_COLOR:
		ep.setstaticcstring("white");
		return ES_NORMAL;
	case P_FORE_COLOR:
	case P_BORDER_COLOR:
	case P_TOP_COLOR:
	case P_BOTTOM_COLOR:
	case P_SHADOW_COLOR:
	case P_FOCUS_COLOR:
		ep.setstaticcstring("black");
		return ES_NORMAL;
	case P_FORE_PATTERN:
	case P_BACK_PATTERN:
	case P_HILITE_PATTERN:
	case P_BORDER_PATTERN:
	case P_TOP_PATTERN:
	case P_BOTTOM_PATTERN:
	case P_SHADOW_PATTERN:
	case P_FOCUS_PATTERN:
		ep.clear();
		return ES_NORMAL;
	case P_TEXT_ALIGN:
		ep.setstaticcstring(MCleftstring);
		return ES_NORMAL;
	case P_TEXT_FONT:
		ep.setstaticcstring(DEFAULT_TEXT_FONT);
		return ES_NORMAL;
	case P_TEXT_HEIGHT:
		ep.setint(heightfromsize(DEFAULT_TEXT_SIZE));
		return ES_NORMAL;
	case P_TEXT_SIZE:
		ep.setint(DEFAULT_TEXT_SIZE);
		return ES_NORMAL;
	case P_TEXT_STYLE:
		ep.setstaticcstring(MCplainstring);
		return ES_NORMAL;
#endif /* MCDispatch::getprop */ 
	default:
		MCeerror->add(EE_OBJECT_GETNOPROP, 0, 0);
		return ES_ERROR;
	}
}

Exec_stat MCDispatch::setprop(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
#ifdef /* MCDispatch::setprop */ LEGACY_EXEC
	return ES_NORMAL;
#endif /* MCDispatch::setprop */
}

// bogus "cut" call actually checks license
Boolean MCDispatch::cut(Boolean home)
{
	if (home)
		return True;
	return MCnoui || (flags & F_WAS_LICENSED) != 0;
}

extern Exec_stat MCHandlePlatformMessage(Handler_type htype, const MCString& mess, MCParameter *params);
Exec_stat MCDispatch::handle(Handler_type htype, MCNameRef mess, MCParameter *params, MCObject *pass_from)
{
	Exec_stat stat = ES_NOT_HANDLED;

	bool t_has_passed;
	t_has_passed = false;
	
	if (MCcheckstack && MCU_abs(MCstackbottom - (char *)&stat) > MCrecursionlimit)
	{
		MCeerror->add(EE_RECURSION_LIMIT, 0, 0);
		MCerrorptr = stacks;
		return ES_ERROR;
	}

	// MW-2011-06-30: Move handling of library stacks from MCStack::handle.
	if (MCnusing > 0)
	{
		for (uint32_t i = MCnusing; i > 0 && (stat == ES_PASS || stat == ES_NOT_HANDLED); i -= 1)
		{
			stat = MCusing[i - 1]->handle(htype, mess, params, nil);

			// MW-2011-08-22: [[ Bug 9686 ]] Make sure we exit as soon as the
			//   message is handled.
			if (stat != ES_NOT_HANDLED && stat != ES_PASS)
				return stat;

			if (stat == ES_PASS)
				t_has_passed = true;
		}

		if (t_has_passed && stat == ES_NOT_HANDLED)
			stat = ES_PASS;
	}

	if ((stat == ES_NOT_HANDLED || stat == ES_PASS) && MCbackscripts != NULL)
	{
		MCObjectList *optr = MCbackscripts;
		do
		{
			if (!optr->getremoved())
			{
				stat = optr->getobject()->handle(htype, mess, params, nil);
				if (stat != ES_NOT_HANDLED && stat != ES_PASS)
					return stat;
				if (stat == ES_PASS)
					t_has_passed = true;
			}
			optr = optr->next();
		}
		while (optr != MCbackscripts);
	}

	if ((stat == ES_NOT_HANDLED || stat == ES_PASS) && m_externals != nil)
	{
		Exec_stat oldstat = stat;
		stat = m_externals -> Handle(this, htype, mess, params);

		// MW-2011-08-22: [[ Bug 9686 ]] Make sure we exit as soon as the
		//   message is handled.
		if (stat != ES_NOT_HANDLED && stat != ES_PASS)
			return stat;

		if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
			stat = ES_PASS;
	}

#ifdef TARGET_SUBPLATFORM_IPHONE
	extern Exec_stat MCIPhoneHandleMessage(MCNameRef message, MCParameter *params);
	if (stat == ES_NOT_HANDLED || stat == ES_PASS)
	{
		stat = MCIPhoneHandleMessage(mess, params);
		
		if (stat != ES_NOT_HANDLED && stat != ES_PASS)
			return stat;
	}
#endif

#ifdef _MOBILE
	if (stat == ES_NOT_HANDLED || stat == ES_PASS)
	{
		stat = MCHandlePlatformMessage(htype, MCNameGetOldString(mess), params);

		// MW-2011-08-22: [[ Bug 9686 ]] Make sure we exit as soon as the
		//   message is handled.
		if (stat != ES_NOT_HANDLED && stat != ES_PASS)
			return stat;
	}
#endif

	if (MCmessagemessages && stat != ES_PASS)
		MCtargetptr->sendmessage(htype, mess, False);
		
	if (t_has_passed)
		return ES_PASS;

	return stat;
}

void MCDispatch::getmainstacknames(MCExecPoint &ep)
{
	ep.clear();
	MCExecPoint ep2(ep);
	MCStack *tstk = stacks;
	bool first;
	first = true;
	do
	{
		tstk->getprop(0, P_SHORT_NAME, ep2, False);
		ep.concatmcstring(ep2.getsvalue(), EC_RETURN, first);
		first = false;
		tstk = (MCStack *)tstk->next();
	}
	while (tstk != stacks);
}

void MCDispatch::appendstack(MCStack *sptr)
{
	sptr->appendto(stacks);
	
	// MW-2013-03-20: [[ MainStacksChanged ]]
	MCmainstackschanged = True;
}

void MCDispatch::removestack(MCStack *sptr)
{
	sptr->remove(stacks);
	
	// MW-2013-03-20: [[ MainStacksChanged ]]
	MCmainstackschanged = True;
}

void MCDispatch::destroystack(MCStack *sptr, Boolean needremove)
{
	if (needremove)
		removestack(sptr);
	if (sptr == MCstaticdefaultstackptr)
		MCstaticdefaultstackptr = stacks;
	if (sptr == MCdefaultstackptr)
		MCdefaultstackptr = MCstaticdefaultstackptr;
	if (MCacptr != NULL && MCacptr->getmessagestack() == sptr)
		MCacptr->setmessagestack(NULL);
	Boolean oldstate = MClockmessages;
	MClockmessages = True;
	delete sptr;
	MClockmessages = oldstate;
}

Boolean MCDispatch::openstartup(const char *sname,
                                char **outpath, IO_handle &stream)
{
	if (enginedir == nil)
		return False;

		uint4 l = MCU_max((uint4)strlen(enginedir), (uint4)strlen(startdir)) + strlen(sname) + 11;
		char *fullpath = new char[l];
		sprintf(fullpath, "%s/%s", startdir, sname);
		if ((stream = MCS_open(fullpath, IO_READ_MODE, True, False, 0)) != NULL)
		{
			*outpath = fullpath;
			return True;
		}
		sprintf(fullpath, "%s/%s", enginedir, sname);
		if ((stream = MCS_open(fullpath, IO_READ_MODE, True, False, 0)) != NULL)
		{
			*outpath = fullpath;
			return True;
		}
		delete fullpath;

	return False;
}

Boolean MCDispatch::openenv(const char *sname, const char *env,
                            char **outpath, IO_handle &stream, uint4 offset)
{
	if ((env = MCS_getenv(env)) != NULL)
	{
		char *pathstring = strclone(env);
		char *fullpath = new char[strlen(env) + strlen(sname) + 2];
		char *eptr = pathstring;
		while (eptr != NULL)
		{
			char *path = eptr;
			eptr = strchr(eptr, ENV_SEPARATOR);
			if (eptr != NULL)
				*eptr++ = '\0';
#ifdef _WIN32
			sprintf(fullpath, "%s\\%s", path, sname);
#else
			sprintf(fullpath, "%s/%s", path, sname);
#endif

			if ((stream = MCS_open(fullpath, IO_READ_MODE, True, False,
			                       offset)) != NULL)
			{
				delete pathstring;
				*outpath = fullpath;
				return True;
			}
		}
		delete pathstring;
		delete fullpath;
	}
	return False;
}

IO_stat readheader(IO_handle& stream, char *version)
{
	char tnewheader[NEWHEADERSIZE];
	uint4 size = NEWHEADERSIZE;

	if (IO_read(tnewheader, sizeof(char), size, stream) == IO_NORMAL)
	{
		// MW-2012-03-04: [[ StackFile5500 ]] Check for either the 2.7 or 5.5 header.
		if (strncmp(tnewheader, newheader, NEWHEADERSIZE) == 0 ||
			strncmp(tnewheader, newheader5500, NEWHEADERSIZE) == 0)
		{
			sprintf(version, "%c.%c.%c.%c", tnewheader[4], tnewheader[5], tnewheader[6], tnewheader[7]);
			if (tnewheader[7] == '0')
			{
				version[5] = '\0';
				if (tnewheader[6] == '0')
					version[3] = '\0';
			}
		}
		else
		{
			char theader[HEADERSIZE + 1];
			uint4 size = HEADERSIZE - NEWHEADERSIZE;
			theader[HEADERSIZE] = '\0';
			uint4 offset;
			strncpy(theader, tnewheader, NEWHEADERSIZE);
			if (IO_read(theader + NEWHEADERSIZE, sizeof(char), size, stream) == IO_NORMAL
		        && MCU_offset(SIGNATURE, theader, offset))
			{
				if (theader[offset - 1] != '\n' || theader[offset - 2] == '\r')
				{
					MCresult->sets("stack was corrupted by a non-binary file transfer");
					return IO_ERROR;
				}

				strncpy(version, &theader[offset + VERSION_OFFSET], 3);
				version[3] = '\0';
			}
			else
				return IO_ERROR;
		}
	}
	return IO_NORMAL;
}

// This method reads a stack from the given stream. The stack is set to
// have parent MCDispatch, and filename MCcmd. It is designed to be used
// for embedded stacks/deployed stacks/revlet stacks.
IO_stat MCDispatch::readstartupstack(IO_handle stream, MCStack*& r_stack)
{
	char version[8];
	uint1 charset, type;
	char *newsf;
	if (readheader(stream, version) != IO_NORMAL
	        || IO_read_uint1(&charset, stream) != IO_NORMAL
	        || IO_read_uint1(&type, stream) != IO_NORMAL
	        || IO_read_string(newsf, stream) != IO_NORMAL)
		return IO_ERROR;

	// MW-2008-10-20: [[ ParentScripts ]] Set the boolean flag that tells us whether
	//   parentscript resolution is required to false.
	s_loaded_parent_script_reference = false;

	MCtranslatechars = charset != CHARSET;
	delete newsf; // stackfiles is obsolete

	MCStack *t_stack = nil;
	/* UNCHECKED */ MCStackSecurityCreateStack(t_stack);
	t_stack -> setparent(this);
	t_stack -> setfilename(strclone(MCcmd));
	if (IO_read_uint1(&type, stream) != IO_NORMAL
	        || type != OT_STACK && type != OT_ENCRYPT_STACK
	        || t_stack->load(stream, version, type) != IO_NORMAL)
	{
		delete t_stack;
		return IO_ERROR;
	}

	if (t_stack->load_substacks(stream, version) != IO_NORMAL
	        || IO_read_uint1(&type, stream) != IO_NORMAL
	        || type != OT_END)
	{
		delete t_stack;
		return IO_ERROR;
	}

	// We are reading the startup stack, so this becomes the root of the
	// stack list.
	stacks = t_stack;

	r_stack = t_stack;

#ifndef _MOBILE
	// Make sure parent script references are up to date.
	if (s_loaded_parent_script_reference)
		t_stack -> resolveparentscripts();
#else
	// Mark the stack as needed parentscript resolution. This is done after
	// aux stacks have been loaded.
	if (s_loaded_parent_script_reference)
		t_stack -> setextendedstate(True, ECS_USES_PARENTSCRIPTS);
#endif

	return IO_NORMAL;
}

// MW-2012-02-17: [[ LogFonts ]] Load a stack file, ensuring we clear up any
//   font table afterwards - regardless of errors.
IO_stat MCDispatch::readfile(const char *openpath, const char *inname, IO_handle &stream, MCStack *&sptr)
{
	IO_stat stat;
	stat = doreadfile(openpath, inname, stream, sptr);

	MCLogicalFontTableFinish();

	return stat;
}

// MW-2012-02-17: [[ LogFonts ]] Actually load the stack file (wrapped by readfile
//   to handle font table cleanup).
IO_stat MCDispatch::doreadfile(const char *openpath, const char *inname, IO_handle &stream, MCStack *&sptr)
{
	Boolean loadhome = False;
	char version[8];

	if (readheader(stream, version) == IO_NORMAL)
	{
		if (strcmp(version, MCversionstring) > 0)
		{
			MCresult->sets("stack was produced by a newer version");
			return IO_ERROR;
		}

		// MW-2008-10-20: [[ ParentScripts ]] Set the boolean flag that tells us whether
		//   parentscript resolution is required to false.
		s_loaded_parent_script_reference = false;

		uint1 charset, type;
		char *newsf;
		if (IO_read_uint1(&charset, stream) != IO_NORMAL
		        || IO_read_uint1(&type, stream) != IO_NORMAL
		        || IO_read_string(newsf, stream) != IO_NORMAL)
		{
			MCresult->sets("stack is corrupted, check for ~ backup file");
			return IO_ERROR;
		}
		delete newsf; // stackfiles is obsolete
		MCtranslatechars = charset != CHARSET;
		sptr = nil;
		/* UNCHECKED */ MCStackSecurityCreateStack(sptr);
		if (stacks == NULL)
			sptr->setparent(this);
		else
			sptr->setparent(stacks);
		sptr->setfilename(strclone(openpath));

		if (MCModeCanLoadHome() && type == OT_HOME)
		{
			char *lstring = NULL;
			char *cstring = NULL;
			IO_read_string(lstring, stream);
			IO_read_string(cstring, stream);
			delete lstring;
			delete cstring;
		}

		MCresult -> clear();

		if (IO_read_uint1(&type, stream) != IO_NORMAL
		    || type != OT_STACK && type != OT_ENCRYPT_STACK
		    || sptr->load(stream, version, type) != IO_NORMAL)
		{
			if (MCresult -> isclear())
				MCresult->sets("stack is corrupted, check for ~ backup file");
			destroystack(sptr, False);
			sptr = NULL;
			return IO_ERROR;
		}
		
		// MW-2011-08-09: [[ Groups ]] Make sure F_GROUP_SHARED is set
		//   appropriately.
		sptr -> checksharedgroups();
		
		if (sptr->load_substacks(stream, version) != IO_NORMAL
		        || IO_read_uint1(&type, stream) != IO_NORMAL
		        || type != OT_END)
		{
			if (MCresult -> isclear())
				MCresult->sets("stack is corrupted, check for ~ backup file");
			destroystack(sptr, False);
			sptr = NULL;
			return IO_ERROR;
		}

		if (stacks != NULL)
		{
			MCStack *tstk = stacks;
			do
			{
				if (sptr->hasname(tstk->getname()))
				{
					MCAutoNameRef t_stack_name;
					/* UNCHECKED */ t_stack_name . Clone(sptr -> getname());

					delete sptr;
					sptr = NULL;

					if (strequal(tstk->getfilename(), openpath))
						sptr = tstk;
					else
					{
						MCdefaultstackptr->getcard()->message_with_args(MCM_reload_stack, MCNameGetOldString(tstk->getname()), openpath);
						tstk = stacks;
						do
						{
							if (MCNameIsEqualTo(t_stack_name, tstk->getname(), kMCCompareCaseless))
							{
								sptr = tstk;
								break;
							}
							tstk = (MCStack *)tstk->next();
						}
						while (tstk != stacks);
					}

					return IO_NORMAL;
				}
				tstk = (MCStack *)tstk->next();
			}
			while (tstk != stacks);
		}
		
		appendstack(sptr);
		
		sptr->extraopen(false);

		// MW-2008-10-28: [[ ParentScript ]]
		// We just loaded a stackfile, so check to see if parentScript resolution
		// is required and if so do it.
		// MW-2009-01-28: [[ Inherited parentScripts ]]
		// Resolving parentScripts may allocate memory, so 'resolveparentscripts'
		// will return false if it fails to allocate what it needs. At some point
		// this needs to be dealt with by deleting the stack and returning an error,
		// *However* at the time of writing, 'readfile' isn't designed to handle
		// this - so we just ignore the result for now (note that all the 'load'
		// methods *fail* to check for no-memory errors!).
		if (s_loaded_parent_script_reference)
			sptr -> resolveparentscripts();
		
	}
	else
	{
		MCS_seek_set(stream, 0);
		if (stacks == NULL)
		{
			MCnoui = True;
			MCscreen = new MCUIDC;
			/* UNCHECKED */ MCStackSecurityCreateStack(stacks);
			MCdefaultstackptr = MCstaticdefaultstackptr = stacks;
			stacks->setparent(this);
			stacks->setname_cstring("revScript");
			uint4 size = (uint4)MCS_fsize(stream);
			char *script = new char[size + 2];
			script[size] = '\n';
			script[size + 1] = '\0';
			if (IO_read(script, sizeof(char), size, stream) != IO_NORMAL
			        || !stacks->setscript(script))
			{
				delete script;
				return IO_ERROR;
			}
		}
		else
		{
			char *tname = strclone(inname);
			
			// MW-2008-06-12: [[ Bug 6476 ]] Media won't open HC stacks
			if (!MCdispatcher->cut(True) || hc_import(tname, stream, sptr) != IO_NORMAL)
			{
				MCresult->sets("file is not a stack");
				delete tname;
				return IO_ERROR;
			}
		}
	}
	return IO_NORMAL;
}

IO_stat MCDispatch::loadfile(const char *inname, MCStack *&sptr)
{
	IO_handle stream;
	char *openpath = NULL;
	char *fname = strclone(inname);
	if ((stream = MCS_open(fname, IO_READ_MODE, True, False, 0)) != NULL)
		if (fname[0] != PATH_SEPARATOR && fname[1] != ':')
		{
			char *curpath = MCS_getcurdir();
			if (curpath[strlen(curpath) - 1] == '/')
				curpath[strlen(curpath) - 1] = '\0';
			openpath = new char[strlen(curpath) + strlen(fname) + 2];
			sprintf(openpath, "%s/%s", curpath, fname);
			delete curpath;
		}
		else
			openpath = strclone(fname);
	else
	{
		char *tmparray = new char[strlen(fname) + 1];
		strcpy(tmparray, fname);
		char *tname = strrchr(tmparray, PATH_SEPARATOR);
		if (tname == NULL)
			tname = tmparray;
		else
			tname++;
		if ((stream = MCS_open(tname, IO_READ_MODE, True, False, 0)) != NULL)
		{
			char *curpath = MCS_getcurdir();
			openpath = new char[strlen(curpath) + strlen(tname) + 2];
			sprintf(openpath, "%s/%s", curpath, tname);
			delete curpath;
		}
		else
		{
			if (!openstartup(tname, &openpath, stream)
			        && !openenv(tname, "MCPATH", &openpath, stream, 0)
			        && !openenv(tname, "PATH", &openpath, stream, 0))
			{
				char *homename;
				if ((homename = MCS_getenv("HOME")) != NULL)
				{
					openpath = new char[strlen(homename) + strlen(tname) + 13];
					if (homename[strlen(homename) - 1] == '/')
						homename[strlen(homename) - 1] = '\0';
					sprintf(openpath, "%s/%s", homename,  tname);
					if ((stream = MCS_open(openpath, IO_READ_MODE, True,
					                       False, 0)) == NULL)
					{
						sprintf(openpath, "%s/stacks/%s", homename, tname);
						if ((stream = MCS_open(openpath, IO_READ_MODE, True,
						                       False, 0)) == NULL)
						{
							sprintf(openpath, "%s/components/%s", homename, tname);
							if ((stream = MCS_open(openpath, IO_READ_MODE, True,
							                       False, 0)) == NULL)
							{
								delete openpath;
								openpath = NULL;
							}
						}
					}
				}
			}
		}
		delete tmparray;
	}
	if (stream == NULL)
	{
		if (openpath != NULL)
			delete openpath;
		delete fname;
		return IO_ERROR;
	}
	delete fname;
	IO_stat stat = readfile(openpath, inname, stream, sptr);
	delete openpath;
	MCS_close(stream);
	return stat;
}

void MCDispatch::cleanup(IO_handle stream, char *linkname, char *bname)
{
	if (stream != NULL)
		MCS_close(stream);
	MCS_unlink(linkname);
	if (bname != NULL)
		MCS_unbackup(bname, linkname);
	delete linkname;
	delete bname;
}

IO_stat MCDispatch::savestack(MCStack *sptr, const MCString& fname)
{
	IO_stat stat;
	stat = dosavestack(sptr, fname);

	MCLogicalFontTableFinish();

	return stat;
}

IO_stat MCDispatch::dosavestack(MCStack *sptr, const MCString &fname)
{
	if (MCModeCheckSaveStack(sptr, fname) != IO_NORMAL)
		return IO_ERROR;
	
	char *linkname;
	if (fname.getlength() != 0)
		linkname = fname.clone();
	else
		if ((linkname = strclone(sptr->getfilename())) == NULL)
		{
			MCresult->sets("stack does not have a filename");
			return IO_ERROR;
		}
	if (linkname == NULL)
	{
		MCresult->sets("can't open stack file, bad path");
		return IO_ERROR;
	}
	if (MCS_noperm(linkname))
	{
		MCresult->sets("can't open stack file, no permission");
		delete linkname;
		return IO_ERROR;
	}
	char *oldfiletype = MCfiletype;
	MCfiletype = MCstackfiletype;
	char *backup = new char[strlen(linkname) + 2];
	strcpy(backup, linkname);
	strcat(backup, "~");
	MCS_unlink(backup);
	if (MCS_exists(linkname, True) && !MCS_backup(linkname, backup))
	{
		MCresult->sets("can't open stack backup file");
		MCfiletype = oldfiletype;
		delete linkname;
		delete backup;
		return IO_ERROR;
	}
	IO_handle stream;
	if ((stream = MCS_open(linkname, IO_WRITE_MODE, True, False, 0)) == NULL)
	{
		MCresult->sets("can't open stack file");
		cleanup(stream, linkname, backup);
		MCfiletype = oldfiletype;
		return IO_ERROR;
	}
	MCfiletype = oldfiletype;
	MCString errstring = "Error writing stack (disk full?)";
	
	// MW-2012-03-04: [[ StackFile5500 ]] Work out what header to emit, and the size.
	const char *t_header;
	uint32_t t_header_size;
	if (MCstackfileversion >= 5500)
		t_header = newheader5500, t_header_size = 8;
	else if (MCstackfileversion >= 2700)
		t_header = newheader, t_header_size = 8;
	else
		t_header = header, t_header_size = HEADERSIZE;
	
	if (IO_write(t_header, sizeof(char), t_header_size, stream) != IO_NORMAL
	        || IO_write_uint1(CHARSET, stream) != IO_NORMAL)
	{
		MCresult->sets(errstring);
		cleanup(stream, linkname, backup);
		return IO_ERROR;
	}

	if (IO_write_uint1(OT_NOTHOME, stream) != IO_NORMAL
	        || IO_write_string(NULL, stream) != IO_NORMAL)
	{ // was stackfiles
		MCresult->sets(errstring);
		cleanup(stream, linkname, backup);
		return IO_ERROR;
	}
	
	// MW-2012-02-22; [[ NoScrollSave ]] Adjust the rect by the current group offset.
	MCgroupedobjectoffset . x = 0;
	MCgroupedobjectoffset . y = 0;
	
	MCresult -> clear();
	if (sptr->save(stream, 0, false) != IO_NORMAL
	        || IO_write_uint1(OT_END, stream) != IO_NORMAL)
	{
		if (MCresult -> isclear())
			MCresult->sets(errstring);
		cleanup(stream, linkname, backup);
		return IO_ERROR;
	}
	MCS_close(stream);
	uint2 oldmask = MCS_umask(0);
	uint2 newmask = ~oldmask & 00777;
	if (oldmask & 00400)
		newmask &= ~00100;
	if (oldmask & 00040)
		newmask &= ~00010;
	if (oldmask & 00004)
		newmask &= ~00001;
	MCS_umask(oldmask);
	MCS_chmod(linkname, newmask);
	if (sptr->getfilename() != NULL && !strequal(linkname, sptr->getfilename()))
		MCS_copyresourcefork(sptr->getfilename(), linkname);
	else if (sptr -> getfilename() != NULL)
		MCS_copyresourcefork(backup, linkname);
	sptr->setfilename(linkname);
	if (backup != NULL)
	{
		MCS_unlink(backup);
		delete backup;
	}
	return IO_NORMAL;
}

#ifdef FEATURE_RELAUNCH_SUPPORT
extern bool relaunch_startup(const char *p_id);
#endif

void send_relaunch(void)
{
#ifdef FEATURE_RELAUNCH_SUPPORT
	bool t_do_relaunch;
	t_do_relaunch = false;
	const char *t_id;

	t_do_relaunch = MCModeHandleRelaunch(t_id);

	if (t_do_relaunch)
		if (relaunch_startup(t_id))
			exit(0);
#endif
}

void send_startup_message(bool p_do_relaunch = true)
{
	if (p_do_relaunch)
		send_relaunch();

	MCdefaultstackptr -> setextendedstate(true, ECS_DURING_STARTUP);

	MCdefaultstackptr -> getcard() -> message(MCM_start_up);

	MCdefaultstackptr -> setextendedstate(false, ECS_DURING_STARTUP);
}

void MCDispatch::wclose(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL && !target -> getextendedstate(ECS_DISABLED_FOR_MODAL))
	{
		Exec_stat stat = target->getcurcard()->message(MCM_close_stack_request);
		if (stat == ES_NOT_HANDLED || stat == ES_PASS)
		{
			target->kunfocus();
			target->close();
			target->checkdestroy();
		}
	}
}

void MCDispatch::wkfocus(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL)
		target->kfocus();
}

void MCDispatch::wkunfocus(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL)
		target->kunfocus();
}

Boolean MCDispatch::wkdown(Window w, const char *string, KeySym key)
{
	if (menu != NULL)
		return menu->kdown(string, key);

	MCStack *target = findstackd(w);
	if (target == NULL || !target->kdown(string, key))
	{
		if (MCmodifierstate & MS_MOD1)
		{
			MCButton *bptr = MCstacks->findmnemonic(MCS_tolower(string[0]));
			if (bptr != NULL)
			{
				bptr->activate(True, (uint2)key);
				return True;
			}
		}
	}
	else
		if (target != NULL)
			return True;
	return False;
}

void MCDispatch::wkup(Window w, const char *string, KeySym key)
{
	if (menu != NULL)
		menu->kup(string, key);
	else
	{
		MCStack *target = findstackd(w);
		if (target != NULL)
			target->kup(string, key);
	}
}

void MCDispatch::wmfocus_stack(MCStack *target, int2 x, int2 y)
{
	if (menu != NULL)
		menu->mfocus(x, y);
	else
	{
		if (target != NULL)
			target->mfocus(x, y);
	}
}

void MCDispatch::wmfocus(Window w, int2 x, int2 y)
{
	MCStack *target = findstackd(w);
	wmfocus_stack(target, x, y);
}

void MCDispatch::wmunfocus(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL)
		target->munfocus();
}

void MCDispatch::wmdrag(Window w)
{
	if (!MCModeMakeLocalWindows())
		return;

	if (isdragsource())
		return;

	MCStack *target = findstackd(w);

	if (target != NULL)
		target->mdrag();

	MCPasteboard *t_pasteboard;
	t_pasteboard = MCdragdata -> GetSource();

	// OK-2009-03-13: [[Bug 7776]] - Check for null MCdragtargetptr to hopefully fix crash.
	if (t_pasteboard != NULL && MCdragtargetptr != NULL)
	{
		m_drag_source = true;
		m_drag_end_sent = false;

		// MW-2009-02-02: [[ Improved image search ]]
		// Search for the appropriate image object using the standard method - note
		// here we search relative to the target of the dragStart message.
		MCImage *t_image;
		t_image = NULL;
		if (MCdragimageid != 0)
			t_image = MCdragtargetptr != NULL ? MCdragtargetptr -> resolveimageid(MCdragimageid) : resolveimageid(MCdragimageid);
		
		MCdragsource = MCdragtargetptr;

		if (MCdragtargetptr->gettype() > CT_CARD)
		{
			MCControl *cptr = (MCControl *)MCdragtargetptr;
			cptr->munfocus();
			cptr->getcard()->ungrab();
		}
		MCdragtargetptr->getstack()->resetcursor(True);
		MCdragtargetptr -> getstack() -> munfocus();

		MCdragaction = MCscreen -> dodragdrop(t_pasteboard, MCallowabledragactions, t_image, t_image != NULL ? &MCdragimageoffset : NULL);

		dodrop(true);
		MCdragdata -> ResetSource();

		MCdragsource = NULL;
		MCdragdest = NULL;
		MCdropfield = NULL;
		MCdragtargetptr = NULL;
		m_drag_source = false;
	}
	else
	{
		MCdragdata -> ResetSource();
		MCdragsource = NULL;
		MCdragdest = NULL;
		MCdropfield = NULL;
		MCdragtargetptr = NULL;
		m_drag_source = false;
	}
}

void MCDispatch::wmdown_stack(MCStack *target, uint2 which)
{
	if (menu != NULL)
		menu -> mdown(which);
	else
	{
		if (!isdragsource())
		{
			MCallowabledragactions = DRAG_ACTION_COPY;
			MCdragaction = DRAG_ACTION_NONE;
			MCdragimageid = 0;
			MCdragimageoffset . x = 0;
			MCdragimageoffset . y = 0;
			MCdragdata -> ResetSource();
		}
		
		if (target != NULL)
			target->mdown(which);
	}
}

void MCDispatch::wmdown(Window w, uint2 which)
{
	MCStack *target = findstackd(w);
	wmdown_stack(target, which);
}

void MCDispatch::wmup_stack(MCStack *target, uint2 which)
{
	if (menu != NULL)
		menu->mup(which);
	else
	{
		if (target != NULL)
			target->mup(which);
	}
}

void MCDispatch::wmup(Window w, uint2 which)
{
	MCStack *target = findstackd(w);
	wmup_stack(target, which);
}

void MCDispatch::wdoubledown(Window w, uint2 which)
{
	if (menu != NULL)
		menu->doubledown(which);
	else
	{
		MCStack *target = findstackd(w);
		if (target != NULL)
			target->doubledown(which);
	}
}

void MCDispatch::wdoubleup(Window w, uint2 which)
{
	if (menu != NULL)
		menu->doubleup(which);
	else
	{
		MCStack *target = findstackd(w);
		if (target != NULL)
			target->doubleup(which);
	}
}

void MCDispatch::kfocusset(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL)
		target->kfocusset(NULL);
}

void MCDispatch::wmdragenter(Window w, MCPasteboard *p_data)
{
	
	MCStack *target = findstackd(w);
	
	m_drag_target = true;

	if (m_drag_source)
		MCdragdata -> SetTarget(MCdragdata -> GetSource());
	else
		MCdragdata -> SetTarget(p_data);

	if (MCmousestackptr != NULL && target != MCmousestackptr)
		MCmousestackptr -> munfocus();

	MCmousestackptr = target;
}

MCDragAction MCDispatch::wmdragmove(Window w, int2 x, int2 y)
{
	// We must also issue a new focus event if the modifierstate
	// changes.
	static uint4 s_old_modifiers = 0;

	MCStack *target = findstackd(w);
	if (MCmousex != x || MCmousey != y || MCmodifierstate != s_old_modifiers)
	{
		MCmousex = x;
		MCmousey = y;
		s_old_modifiers = MCmodifierstate;
		target -> mfocus(x, y);
	}
	return MCdragaction;
}

void MCDispatch::wmdragleave(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL && target == MCmousestackptr)
	{
		MCmousestackptr -> munfocus();
		MCmousestackptr = NULL;
	}
	MCdragdata -> ResetTarget();
	m_drag_target = false;
}

MCDragAction MCDispatch::wmdragdrop(Window w)
{
	MCStack *target;
	target = findstackd(w);
	
	// MW-2011-02-08: Make sure we store the drag action that is in effect now
	//   otherwise it can change as a result of message sends which is bad :o)
	uint32_t t_drag_action;
	t_drag_action = MCdragaction;
	
	if (t_drag_action != DRAG_ACTION_NONE)
		dodrop(false);

	MCmousestackptr = NULL;
	MCdragdata -> ResetTarget();
	m_drag_target = false;

	return t_drag_action;
}

void MCDispatch::property(Window w, Atom atom)
{
}

void MCDispatch::configure(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL)
		target->configure(True);
}

void MCDispatch::enter(Window w)
{
	MCStack *target = findstackd(w);
	if (target != NULL)
		target->enter();
}

void MCDispatch::redraw(Window w, MCRegionRef p_dirty_region)
{
	MCStack *target = findstackd(w);
	if (target == NULL)
		return;

	target -> updatewindow(p_dirty_region);
}

MCFontStruct *MCDispatch::loadfont(const MCString &fname, uint2 &size,
                                   uint2 style, Boolean printer)
{
#ifdef _LINUX
	if (fonts == NULL)
		fonts = MCFontlistCreateNew();
	if (fonts == NULL)
		fonts = MCFontlistCreateOld();
#else
	if (fonts == nil)
		fonts = new MCFontlist;
#endif
	return fonts->getfont(fname, size, style, printer);
}

MCStack *MCDispatch::findstackname(const MCString &s)
{
	if (s.getlength() == 0)
		return NULL;

	MCStack *tstk = stacks;
	if (tstk != NULL)
	{
		do
		{
			MCStack *foundstk;
			if ((foundstk = (MCStack *)tstk->findsubstackname(s)) != NULL)
				return foundstk;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}

	tstk = stacks;
	if (tstk != NULL)
	{
		do
		{
			MCStack *foundstk;
			if ((foundstk = (MCStack *)tstk->findstackfile(s)) != NULL)
				return foundstk;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}

	char *sname = s.clone();
	if (loadfile(sname, tstk) != IO_NORMAL)
	{
		char *buffer = new char[s.getlength() + 5];
		MCU_lower(buffer, s);
		strcpy(&buffer[s.getlength()], ".mc");
		delete sname;
		char *sptr = buffer;
		while (*sptr)
		{
			if (strchr("\r\n\t *?*<>/\\()[]{}|'`\"", *sptr) != NULL)
				*sptr = '_';
			sptr++;
		}
		if (loadfile(buffer, tstk) != IO_NORMAL)
		{
			strcpy(&buffer[s.getlength()], ".rev");
			if (loadfile(buffer, tstk) != IO_NORMAL)
			{
				delete buffer;
				return NULL;
			}
		}
		delete buffer;
	}
	else
		delete sname;

	return tstk;
}

MCStack *MCDispatch::findstackid(uint4 fid)
{
	if (fid == 0)
		return NULL;
	MCStack *tstk = stacks;
	do
	{
		MCStack *foundstk;
		if ((foundstk = (MCStack *)tstk->findsubstackid(fid)) != NULL)
			return foundstk;
		tstk = (MCStack *)tstk->next();
	}
	while (tstk != stacks);
	return NULL;
}

MCStack *MCDispatch::findchildstackd(Window w,uint2 cindex)
{
	uint2 ccount = 0;
	if (stacks != NULL)
	{
		MCStack *tstk = stacks;
		do
		{
			MCStack *foundstk;
			if ((foundstk =
			            (MCStack *)tstk->findchildstackd(w,ccount,cindex))!= NULL)
				return foundstk;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}
	return NULL;
}

MCStack *MCDispatch::findstackd(Window w)
{
	if (w == DNULL)
		return NULL;
	
	if (stacks != NULL)
	{
		MCStack *tstk = stacks;
		do
		{
			MCStack *foundstk;
			if ((foundstk = tstk->findstackd(w)) != NULL)
				return foundstk;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}
	if (panels != NULL)
	{
		MCStack *tstk = panels;
		do
		{
			MCStack *foundstk;
			if ((foundstk = tstk->findstackd(w)) != NULL)
				return foundstk;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != panels);
	}

	// MW-2006-04-24: [[ Purify ]] It is possible to get here after MCtooltip has been
	//   deleted. So MCtooltip is now NULL in this situation and we test for it here.
	if (MCtooltip != NULL && MCtooltip->findstackd(w))
		return MCtooltip;
	return NULL;
}

MCObject *MCDispatch::getobjid(Chunk_term type, uint4 inid)
{
	if (stacks != NULL)
	{
		MCStack *tstk = stacks;
		do
		{
			MCObject *optr;
			if ((optr = tstk->getsubstackobjid(type, inid)) != NULL)
				return optr;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}
	return NULL;
}

MCObject *MCDispatch::getobjname(Chunk_term type, const MCString &s)
{
	if (stacks != NULL)
	{
		MCStack *tstk = stacks;
		do
		{
			MCObject *optr;
			if ((optr = tstk->getsubstackobjname(type, s)) != NULL)
				return optr;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}

	if (type == CT_IMAGE)
	{
		const char *sptr = s.getstring();
		uint4 l = s.getlength();

		MCAutoNameRef t_image_name;
		if (MCU_strchr(sptr, l, ':'))
			/* UNCHECKED */ t_image_name . CreateWithOldString(s);

		MCImage *iptr = imagecache;
		if (iptr != NULL)
		{
			do
			{
check:
				if (t_image_name != nil && iptr -> hasname(t_image_name))
					return iptr;
				if (!iptr->getopened())
				{
					iptr->remove(imagecache);
					delete iptr;
					iptr = imagecache;
					if (iptr == NULL)
						break;
					goto check;
				}
				iptr = (MCImage *)iptr->next();
			}
			while (iptr != imagecache);
		}

		if (MCU_strchr(sptr, l, ':'))
		{
			MCresult->clear(False);
			MCExecPoint ep(MCdefaultstackptr, NULL, NULL);
			MCExecPoint *epptr = MCEPptr == NULL ? &ep : MCEPptr;
			epptr->setsvalue(s);
			MCU_geturl(*epptr);
			if (MCresult->isempty())
			{
				iptr = new MCImage;
				iptr->appendto(imagecache);
				iptr->setprop(0, P_TEXT, *epptr, False);
				iptr->setname(t_image_name);
				return iptr;
			}
		}
	}
	return NULL;
}

MCStack *MCDispatch::gethome()
{
	return stacks;
}

Boolean MCDispatch::ismainstack(MCStack *sptr)
{
	if (stacks != NULL)
	{
		MCStack *tstk = stacks;
		do
		{
			if (tstk == sptr)
				return True;
			tstk = (MCStack *)tstk->next();
		}
		while (tstk != stacks);
	}
	return False;
}

void MCDispatch::addmenu(MCObject *target)
{
	menu = target;
	target->getcard()->ungrab();
}

void MCDispatch::removemenu()
{
	//  menu->getstack()->mfocus(MCmousex, MCmousey); //disrupts card kfocus
	menu = NULL;
}

void MCDispatch::closemenus()
{
	if (menu != NULL)
		menu->closemenu(True, True);
}

void MCDispatch::appendpanel(MCStack *sptr)
{
	sptr->appendto(panels);
}

void MCDispatch::removepanel(MCStack *sptr)
{
	sptr->remove(panels);
}

///////////////////////////////////////////////////////////////////////////////

bool MCDispatch::loadexternal(const char *p_external)
{
	char *t_filename;
#if defined(TARGET_SUBPLATFORM_ANDROID)
	extern bool revandroid_loadExternalLibrary(const char *p_external, char*& r_filename);
	// MW-2013-08-07: [[ ExternalsApiV5 ]] Make sure we only use the leaf name
	//   of the external when loading.
	if (strrchr(p_external, '/') != nil)
		p_external = strrchr(p_external, '/') + 1;
	if (!revandroid_loadExternalLibrary(p_external, t_filename))
		return false;

	// Don't try and load any drivers as externals.
	if (strncmp(p_external, "db", 2) == 0)
	{
		delete t_filename;
		return true;
	}
#elif !defined(_SERVER)
	if (p_external[0] == '/')
	{
		if (!MCCStringClone(p_external, t_filename))
			return false;
	}
	else if (!MCCStringFormat(t_filename, "%.*s/%s", strrchr(MCcmd, '/') - MCcmd, MCcmd, p_external))
		return false;
#else
	if (!MCCStringClone(p_external, t_filename))
		return false;
#endif
	
	if (m_externals == nil)
		m_externals = new MCExternalHandlerList;
	
	bool t_loaded;
	t_loaded = m_externals -> Load(t_filename);
	delete t_filename;
	
	if (m_externals -> IsEmpty())
	{
		delete m_externals;
		m_externals = nil;
}

	return t_loaded;
}

///////////////////////////////////////////////////////////////////////////////

// We have three contexts to be concerned with:
//   - text editing : MCactivefield != NULL
//   - image editing : MCactiveimage != NULL
//   - stack editing
//
// We try each of these in turn, attempting appropriate things in each case.
//

bool MCDispatch::dopaste(MCObject*& r_objptr, bool p_explicit)
{
	r_objptr = NULL;

	if (MCactivefield != NULL)
	{
		MCParagraph *t_paragraphs;
		t_paragraphs = MCclipboarddata -> FetchParagraphs(MCactivefield);

		//

		if (t_paragraphs != NULL)
		{
			// MW-2012-03-16: [[ Bug ]] Fetch the current active field since it can be
			//   unset as a result of pasting (due to scrollbarDrag).
			MCField *t_field;
			t_field = MCactivefield;

			// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
			//   textChanged message by a lock screen pair.
			MCRedrawLockScreen();
			t_field -> pastetext(t_paragraphs, true);
			MCRedrawUnlockScreen();

			// MW-2012-02-08: [[ TextChanged ]] Invoke textChanged as this method
			//   was called as a result of a user action (paste cmd, paste key).
			t_field -> textchanged();
			return true;
		}
	}
	
	if (MCactiveimage != NULL && MCclipboarddata -> HasImage())
	{
		MCSharedString *t_data;
		t_data = MCclipboarddata -> Fetch(TRANSFER_TYPE_IMAGE);
		if (t_data != NULL)
		{
			MCExecPoint ep(NULL, NULL, NULL);
			ep . setsvalue(t_data -> Get());

			MCImage *t_image;
			t_image = new MCImage;
			t_image -> open();
			t_image -> openimage();
			t_image -> setprop(0, P_TEXT, ep, False);
			MCactiveimage -> pasteimage(t_image);
			t_image -> closeimage();
			t_image -> close();

			delete t_image;

			t_data -> Release();
		}

		return true;
	}
	
	if (MCdefaultstackptr != NULL && (p_explicit || MCdefaultstackptr -> gettool(MCdefaultstackptr) == T_POINTER))
	{
		MCObject *t_objects;
		t_objects = NULL;

		if (!MCclipboarddata -> Lock())
			return false;

		if (MCclipboarddata -> HasObjects())
		{
			MCSharedString *t_data;
			t_data = MCclipboarddata -> Fetch(TRANSFER_TYPE_OBJECTS);
			if (t_data != NULL)
			{
				t_objects = MCObject::unpickle(t_data, MCdefaultstackptr);
				t_data -> Release();
			}
		}
		else if (MCclipboarddata -> HasImage())
		{
			MCSharedString *t_data;
			t_data = MCclipboarddata -> Fetch(TRANSFER_TYPE_IMAGE);
			if (t_data != NULL)
			{
				MCExecPoint ep(NULL, NULL, NULL);
				ep . setsvalue(t_data -> Get());

				t_objects = new MCImage(*MCtemplateimage);
				t_objects -> open();
				t_objects -> setprop(0, P_TEXT, ep, False);
				t_objects -> close();

				t_data -> Release();
			}
		}

		MCclipboarddata -> Unlock();

		//

		if (t_objects != NULL)
		{
			MCselected -> clear(False);
			MCselected -> lockclear();

			while(t_objects != NULL)
			{
				MCObject *t_object;
				t_object = t_objects -> remove(t_objects);
				t_object -> paste();

				// OK-2009-04-02: [[Bug 7881]] - Parentscripts broken by cut and pasting object
				t_object -> resolveparentscript();

				if (t_object -> getparent() == NULL)
					delete t_object;
				else
					r_objptr = t_object;
			}

			MCselected -> unlockclear();
	
			return true;
		}
	}

	return false;
}

void MCDispatch::dodrop(bool p_source)
{
	if (!m_drag_end_sent && MCdragsource != NULL && (MCdragdest == NULL || MCdragaction == DRAG_ACTION_NONE))
	{
		// We are only the source
		m_drag_end_sent = true;
		MCdragsource -> message(MCM_drag_end);

		// OK-2008-10-21 : [[Bug 7316]] - Cursor in script editor follows mouse after dragging to non-Revolution target.
		// I have no idea why this apparently only happens in the script editor, but this seems to fix it and doesn't seem too risky :)
		// MW-2008-10-28: [[ Bug 7316 ]] - This happens because the script editor is doing stuff with drag messages
		//   causing the default engine behaviour to be overriden. In this case, some things have to happen to the field
		//   when the drag is over. Note that we have to check that the source was a field in this case since we don't
		//   need to do anything if it is not!
		if (MCdragsource -> gettype() == CT_FIELD)
		{
			MCField *t_field;
			t_field = static_cast<MCField *>(MCdragsource);
			t_field -> setstate(False, CS_DRAG_TEXT);
			t_field -> computedrag();
			t_field -> getstack() -> resetcursor(True);
		}

		return;
	}
	
	if (p_source)
		return;

	// Setup global variables for a field drop
	MCdropfield = NULL;
	MCdropchar = 0;

	int4 t_start_index, t_end_index;
	t_start_index = t_end_index = 0;
	if (MCdragdest != NULL && MCdragdest -> gettype() == CT_FIELD)
	{
		MCdropfield = static_cast<MCField *>(MCdragdest);
		if (MCdragdest -> getstate(CS_DRAG_TEXT))
		{
			MCdropfield -> locmark(False, False, False, False, True, t_start_index, t_end_index);
			MCdropchar = t_start_index;
		}
	}

	// If source is a field and the engine handled the start of the drag operation
	bool t_auto_source;
	t_auto_source = MCdragsource != NULL && MCdragsource -> gettype() == CT_FIELD && MCdragsource -> getstate(CS_SOURCE_TEXT);

	// If dest is a field and the engine handled the accepting of the operation
	bool t_auto_dest;
	t_auto_dest = MCdragdest != NULL && MCdragdest -> gettype() == CT_FIELD && MCdragdest -> getstate(CS_DRAG_TEXT);

	if (t_auto_source && t_auto_dest && MCdragsource == MCdragdest)
	{
		// Source and target is the same field
		MCField *t_field;
		t_field = static_cast<MCField *>(MCdragsource);

		int4 t_from_start_index, t_from_end_index;
		t_field -> selectedmark(False, t_from_start_index, t_from_end_index, False, False);

		// We are dropping in the target selection - so just send the messages and do nothing
		if (t_start_index >= t_from_start_index && t_start_index < t_from_end_index)
		{
			t_field -> message(MCM_drag_drop);
			t_field -> message(MCM_drag_end);
			t_field -> setstate(False, CS_DRAG_TEXT);
			t_field -> computedrag();
			t_field -> getstack() -> resetcursor(True);
			return;
		}

		if (t_field -> message(MCM_drag_drop) != ES_NORMAL)
		{
			MCParagraph *t_paragraphs;
			t_paragraphs = MCdragdata -> FetchParagraphs(MCdropfield);

			// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
			//   textChanged message by a lock screen pair.
			MCRedrawLockScreen();

			if (MCdragaction == DRAG_ACTION_MOVE)
			{
				MCdropfield -> movetext(t_paragraphs, t_start_index);
				Ustruct *us = MCundos->getstate();
				if (us != NULL && us->type == UT_MOVE_TEXT)
					MCdropfield->seltext(us -> ud.text.index, us -> ud.text.index + us->ud.text.newchars, False, True);
			}
			else
			{
				MCdropfield -> seltext(t_start_index, t_start_index, True);

				MCdropfield -> pastetext(t_paragraphs, true);

				Ustruct *us = MCundos->getstate();
				if (us != NULL && us->type == UT_TYPE_TEXT)
					MCdropfield->seltext(t_start_index, t_start_index + us->ud.text.newchars, False, True);
			}

			// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
			//   textChanged message by a lock screen pair.
			MCRedrawUnlockScreen();
			
			// MW-2012-02-08: [[ TextChanged ]] Invoke textChanged as this method
			//   was called as a result of a user action (result of drop in field).
			MCactivefield -> textchanged();
		}

		MCdropfield->setstate(False, CS_DRAG_TEXT);
		MCdropfield->computedrag();
		MCdropfield -> getstack() -> resetcursor(True);

		return;
	}

	int4 t_src_start, t_src_end;
	t_src_start = t_src_end = 0;
	if (t_auto_source)
		static_cast<MCField *>(MCdragsource) -> selectedmark(False, t_src_start, t_src_end, False, False);

	bool t_auto_drop;
	t_auto_drop = MCdragdest != NULL && MCdragdest -> message(MCM_drag_drop) != ES_NORMAL;

	if (t_auto_dest && t_auto_drop && MCdragdata != NULL && MCdropfield != NULL)
	{
		// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
		//   textChanged message by a lock screen pair.
		MCRedrawLockScreen();

		// Process an automatic drop action
		MCdropfield -> seltext(t_start_index, t_start_index, True);

		MCParagraph *t_paragraphs;
		t_paragraphs = MCdragdata -> FetchParagraphs(MCdropfield);
		MCdropfield -> pastetext(t_paragraphs, true);

		Ustruct *us = MCundos->getstate();
		if (us != NULL && us->type == UT_TYPE_TEXT)
			MCdropfield->seltext(t_start_index, t_start_index + us->ud.text.newchars, False, True);
		MCdropfield->setstate(False, CS_DRAG_TEXT);
		MCdropfield->computedrag();
		MCdropfield -> getstack() -> resetcursor(True);
		
		// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
		//   textChanged message by a lock screen pair.
		MCRedrawUnlockScreen();

		// MW-2012-02-08: [[ TextChanged ]] Invoke textChanged as this method
		//   was called as a result of a user action (drop from different field).
		MCactivefield -> textchanged();
	}
	else if (MCdropfield != NULL)
	{
		MCdropfield->setstate(False, CS_DRAG_TEXT);
		MCdropfield->computedrag();
		MCdropfield -> getstack() -> resetcursor(True);
	}

	bool t_auto_end;
	if (MCdragsource != NULL)
	{
		m_drag_end_sent = true;
		t_auto_end = MCdragsource -> message(MCM_drag_end) != ES_NORMAL;
	}
	else
		t_auto_end = false;

	if (t_auto_source && t_auto_end && MCdragsource != NULL && MCdragaction == DRAG_ACTION_MOVE)
	{
		// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
		//   textChanged message by a lock screen pair.
		MCRedrawLockScreen();
		static_cast<MCField *>(MCdragsource) -> deletetext(t_src_start, t_src_end);
		MCRedrawUnlockScreen();

		// MW-2012-02-08: [[ TextChanged ]] Invoke textChanged as this method
		//   was called as a result of a user action (move from one field to another).
		MCactivefield -> textchanged();
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCDispatch::clearcursors(void)
{
	for(uint32_t i = 0; i < PI_NCURSORS; i++)
	{
		if (MCcursor == MCcursors[i])
			MCcursor = nil;
		if (MCdefaultcursor = MCcursors[i])
			MCdefaultcursor = nil;
	}

	MCStack *t_stack;
	t_stack = stacks;
	do
	{
		t_stack -> clearcursor();
		t_stack = t_stack -> next();
	}
	while(t_stack != stacks -> prev());
}

////////////////////////////////////////////////////////////////////////////////

void MCDispatch::changehome(MCStack *stack)
{
	MCStack *t_stack;
	t_stack = stacks;
	do
	{
		if (t_stack -> getparent() == stacks)
			t_stack -> setparent(stack);
		t_stack = t_stack -> next();
	}
	while(t_stack != stacks -> prev());

	stack -> setparent(this);
	stack -> totop(stacks);
}

////////////////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS_DESKTOP
void MCDispatch::freeprinterfonts()
{
	fonts->freeprinterfonts();
}
#endif

void MCDispatch::flushfonts(void)
{
	delete fonts;
	fonts = nil;
}

MCFontlist *MCFontlistGetCurrent(void)
{
	return MCdispatcher -> getfontlist();
}

////////////////////////////////////////////////////////////////////////////////
