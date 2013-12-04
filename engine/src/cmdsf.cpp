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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcio.h"

#include "dispatch.h"
#include "handler.h"
#include "scriptpt.h"
#include "execpt.h"
#include "cmds.h"
#include "chunk.h"
#include "mcerror.h"
#include "object.h"
#include "control.h"
#include "aclip.h"
#include "vclip.h"
#include "eps.h"
#include "image.h"
#include "stack.h"
#include "card.h"
#include "stacklst.h"
#include "sellst.h"
#include "util.h"
#include "printer.h"
#include "hc.h"
#include "globals.h"
#include "securemode.h"
#include "mode.h"
#include "context.h"
#include "osspec.h"
#include "regex.h"

#include "socket.h"
#include "mcssl.h"

#include "iquantization.h"

#include "core.h"

#include "resolution.h"

MCClose::~MCClose()
{
	delete fname;
	delete stack;
}

Parse_stat MCClose::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_CLOSE_NOTYPE, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_OPEN, te) != PS_NORMAL)
	{
		if (sp.lookup(SP_VISUAL, te) == PS_NORMAL)
		{
			sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_WINDOW);
			return PS_NORMAL;
		}
		else
		{
			sp.backup();
			stack = new MCChunk(False);
			if (stack->parse(sp, False) != PS_NORMAL)
			{
				MCperror->add
				(PE_CLOSE_BADEXP, sp);
				return PS_ERROR;
			}
		}
	}
	else
	{
		arg = (Open_argument)te->which;
		if (arg != OA_PRINTING)
		{
			if (sp.parseexp(False, True, &fname) != PS_NORMAL)
			{
				MCperror->add
				(PE_CLOSE_BADNAME, sp);
				return PS_ERROR;
			}
		}
	}
	return PS_NORMAL;
}

Exec_stat MCClose::exec(MCExecPoint &ep)
{
#ifdef /* MCClose */ LEGACY_EXEC
	char *name;
	uint2 index;

	switch (arg)
	{
	case OA_PRINTING:
		MCprinter -> Close();
		if (MCsystemprinter != MCprinter)
		{
			delete MCprinter;
			MCprinter = MCsystemprinter;
		}
	break;
		
	case OA_DRIVER:
	case OA_FILE:
		if (fname->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_CLOSE_BADNAME, line, pos);
			return ES_ERROR;
		}
		name = ep.getsvalue().clone();
		if (IO_closefile(name))
			MCresult->clear(False);
		else
			MCresult->sets("file is not open");
		delete name;
	break;
		
	case OA_PROCESS:
		if (fname->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_CLOSE_BADNAME, line, pos);
			return ES_ERROR;
		}
		name = ep.getsvalue().clone();
		if (IO_findprocess(name, index))
			MCS_closeprocess(index);
		else
			MCresult->sets("process is not open");
		delete name;
	break;
		
	case OA_SOCKET:
		if (fname->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_CLOSE_BADNAME, line, pos);
			return ES_ERROR;
		}
		name = ep.getsvalue().clone();
		if (IO_findsocket(name, index))
		{
			MCS_close_socket(MCsockets[index]);
			MCresult->clear(False);
		}
		else
			MCresult->sets("socket is not open");
		delete name;
	break;
		
	default:
		MCStack *sptr;
		if (stack == NULL)
			sptr = MCdefaultstackptr;
		else
		{
			MCObject *optr;
			uint4 parid;
			if (stack->getobj(ep, optr, parid, True) != ES_NORMAL
			        || optr->gettype() != CT_STACK)
			{
				MCeerror->add
				(EE_CLOSE_NOOBJ, line, pos);
				return ES_ERROR;
			}
			sptr = (MCStack *)optr;
		}
		sptr->close();
		sptr->checkdestroy();
	}
	return ES_NORMAL;
#endif /* MCClose */
}




MCEncryptionOp::~MCEncryptionOp()
{
	delete ciphername;
	delete source;
	delete keystr;
	delete keylen;
	delete it;
	delete salt;
	delete iv;

	delete rsa_key;
	delete rsa_passphrase;
}


Parse_stat MCEncryptionOp::parse(MCScriptPoint &sp)
{
	//ENCRYPT|DECRYPT <SOURCE> USING <CIPHER> WITH [KEY] <PASSWORD|KEY> [AND SALT SALTVALUE] [AND IV IVVALUE] [AT BIT BIT]
	initpoint(sp);

	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add(PE_ENCRYPTION_BADSOURCE, sp);
		return PS_ERROR;
	}

	if (sp.skip_token(SP_ENCRYPTION, TT_UNDEFINED, ENCRT_USING) != PS_NORMAL)
	{
		MCperror->add(PE_ENCRYPTION_NOCIPHER, sp);
		return PS_ERROR;
	}

	if (sp.skip_token(SP_ENCRYPTION, TT_UNDEFINED, ENCRT_RSA) == PS_NORMAL)
	{
		is_rsa = true;
	}
	else
	{
		is_rsa = false;
		if (sp.parseexp(False, True, &ciphername) != PS_NORMAL)
		{
			MCperror->add(PE_ENCRYPTION_BADCIPHER, sp);
			return PS_ERROR;
		}
	}

	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) != PS_NORMAL)
	{
		MCperror->add(PE_ENCRYPTION_NOKEY, sp);
		return PS_ERROR;
	}

	if (is_rsa)
	{
		if (sp.skip_token(SP_ENCRYPTION, TT_UNDEFINED, ENCRT_PUBLIC) == PS_NORMAL)
			rsa_keytype = RSAKEY_PUBKEY;
		else if (sp.skip_token(SP_ENCRYPTION, TT_UNDEFINED, ENCRT_PRIVATE) == PS_NORMAL)
			rsa_keytype = RSAKEY_PRIVKEY;
		else
		{
			MCperror->add(PE_ENCRYPTION_BADPARAM, sp);
			return PS_ERROR;
		}
		if (sp.skip_token(SP_ENCRYPTION, TT_UNDEFINED, ENCRT_KEY) != PS_NORMAL)
		{
			MCperror->add(PE_ENCRYPTION_BADPARAM, sp);
			return PS_ERROR;
		}
		if (sp.parseexp(True, False, &rsa_key) != PS_NORMAL)
		{
			MCperror->add(PE_ENCRYPTION_BADKEY, sp);
			return PS_ERROR;
		}
		if (sp.skip_token(SP_FACTOR, TT_BINOP, O_AND) == PS_NORMAL)
		{
			if (sp.skip_token(SP_ENCRYPTION, TT_UNDEFINED, ENCRT_PASSPHRASE) != PS_NORMAL)
			{
				MCperror->add(PE_ENCRYPTION_BADPARAM, sp);
				return PS_ERROR;
			}
			if (sp.parseexp(True, False, &rsa_passphrase) != PS_NORMAL)
			{
				MCperror->add(PE_ENCRYPTION_BADKEY, sp);
				return PS_ERROR;
			}
		}
	}
	else
	{
		if (sp.skip_token(SP_ENCRYPTION, TT_UNDEFINED, ENCRT_KEY) == PS_NORMAL)
			ispassword = False;
		else
		{
			ispassword = True;
			sp.skip_token(SP_ENCRYPTION, TT_UNDEFINED, ENCRT_PASSWORD);
		}

		if (sp.parseexp(True, False, &keystr) != PS_NORMAL)
		{
			MCperror->add(PE_ENCRYPTION_BADKEY, sp);
			return PS_ERROR;
		}

		// MW-2004-12-01: Bug 2406 - parsing error when attempting to compile 'IV'
		if (sp.skip_token(SP_FACTOR, TT_BINOP, O_AND) == PS_NORMAL)
		{
			Bool expect_iv; // True if we should check for 'IV' clause

			if (sp.skip_token(SP_ENCRYPTION, TT_UNDEFINED, ENCRT_SALT) == PS_NORMAL)
			{
				if (ispassword)
				{
					if (sp.parseexp(True, False, &salt) != PS_NORMAL)
					{
						MCperror->add(PE_ENCRYPTION_BADSALT, sp);
						return PS_ERROR;
					}
				}
				else
				{
					MCperror->add(PE_ENCRYPTION_BADPARAM, sp);
					return PS_ERROR;
				}

				expect_iv = sp.skip_token(SP_FACTOR, TT_BINOP, O_AND) == PS_NORMAL;
			}
			else
				expect_iv = True;

			// MW-2004-12-29: Relax syntax slightly to allow specifying an IV as well as password and salt
			if (expect_iv)
			{
				if (sp.skip_token(SP_ENCRYPTION, TT_UNDEFINED, ENCRT_IV) == PS_NORMAL)
				{
					if (sp.parseexp(True, False, &iv) != PS_NORMAL)
					{
						MCperror->add(PE_ENCRYPTION_BADIV, sp);
						return PS_ERROR;
					}
				}
				else
				{
					MCperror->add(PE_ENCRYPTION_BADPARAM, sp);
					return PS_ERROR;
				}
			}
		}

		if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AT) == PS_NORMAL)
		{
			if (sp.parseexp(False, True, &keylen) != PS_NORMAL)
			{
				MCperror->add(PE_ENCRYPTION_BADBIT, sp);
				return PS_ERROR;
			}
			if (sp.skip_token(SP_ENCRYPTION, TT_UNDEFINED, ENCRT_BIT) != PS_NORMAL)
			{
				MCperror->add(PE_ENCRYPTION_NOBIT, sp);
				return PS_ERROR;
			}
		}
	}
	getit(sp, it);
	return PS_NORMAL;

}

#ifdef /* MCEncryptionOp::exec_rsa */ LEGACY_EXEC
Exec_stat MCEncryptionOp::exec_rsa(MCExecPoint &ep)
{
	Exec_stat t_status = ES_NORMAL;
	char *t_rsa_key = nil;
	uint32_t t_rsa_keylength;
	char *t_rsa_passphrase = nil;
	char *t_rsa_out = nil;
	uint32_t t_rsa_out_length;
	char *t_result = nil;
	uint32_t t_error = EE_UNDEFINED;

	if (rsa_key->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_OPEN_BADNAME, line, pos);
		t_status = ES_ERROR;
	}
	if (t_status == ES_NORMAL)
	{
		const MCString &t_str = ep.getsvalue();
		t_rsa_keylength = t_str.getlength();
		if (!MCCStringCloneSubstring(t_str.getstring(), t_rsa_keylength, t_rsa_key))
		{
			MCeerror->add(EE_NO_MEMORY, line, pos);
			t_status = ES_ERROR;
		}
	}

	if (t_status == ES_NORMAL && rsa_passphrase != nil)
	{
		if (rsa_passphrase->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_OPEN_BADNAME, line, pos);
			t_status = ES_ERROR;
		}
		if (t_status == ES_NORMAL)
		{
			const MCString &t_str = ep.getsvalue();
			if (!MCCStringCloneSubstring(t_str.getstring(), t_str.getlength(), t_rsa_passphrase))
			{
				MCeerror->add(EE_NO_MEMORY, line, pos);
				t_status = ES_ERROR;
			}
		}
	}

	if (t_status == ES_NORMAL)
	{
		if (source->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_OPEN_BADNAME, line, pos);
			t_status = ES_ERROR;
		}
	}
	if (t_status == ES_NORMAL)
	{
		const MCString &t_string = ep.getsvalue();
		if (MCCrypt_rsa_op(!isdecrypt, rsa_keytype, t_string.getstring(), t_string.getlength(),
			t_rsa_key, t_rsa_keylength, t_rsa_passphrase, t_rsa_out, t_rsa_out_length, t_result, t_error))
		{
			MCVariable *t_var;
			t_var = it -> evalvar(ep);
			if (t_var != NULL)
				t_var -> grab(t_rsa_out, t_rsa_out_length);
			else
				free(t_rsa_out);
			MCresult->clear();
		}
		else
		{
			if (t_result != nil)
				MCresult->grab(t_result, MCCStringLength(t_result));
			else
			{
				MCeerror->add(t_error, line, pos);
				t_status = ES_ERROR;
			}
		}
	}

	MCCStringFree(t_rsa_key);
	MCCStringFree(t_rsa_passphrase);

	return t_status;
}
#endif /* MCEncryptionOp::exec_rsa */

Exec_stat MCEncryptionOp::exec(MCExecPoint &ep)
{
#ifdef /* MCEncryptionOp */ LEGACY_EXEC
	MCresult->clear(False);

	if (is_rsa)
		return exec_rsa(ep);

	if (ciphername->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_OPEN_BADNAME, line, pos);
		return ES_ERROR;
	}
	char *cipherstring= ep.getsvalue().clone();
	if (keystr->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_OPEN_BADNAME, line, pos);
		return ES_ERROR;
	}
	uint4 keystringlength = ep.getsvalue().getlength();
	char *keystring = new char[keystringlength+1];
	memcpy(keystring,ep.getsvalue().getstring(),keystringlength);
	uint2 keybits = 0;
	if (keylen)
	{
		if (keylen->eval(ep) != ES_NORMAL || !ep.ton())
		{
			MCeerror->add(EE_OPEN_BADNAME, line, pos);
			return ES_ERROR;
		}
		keybits = ep.getuint2();
	}
	char *ivstr,*saltstr;
	ivstr = saltstr = NULL;
	uint2 saltlen,ivlen;
	saltlen = ivlen = 0;
	if (salt)
	{
		if (salt->eval(ep) != ES_NORMAL)
		{
			delete cipherstring;
			delete keystring;
			MCeerror->add(EE_OPEN_BADNAME, line, pos);
			return ES_ERROR;
		}
		saltstr = ep.getsvalue().clone();
		saltlen = ep.getsvalue().getlength();
	}
	if (iv)
	{
		if (iv->eval(ep) != ES_NORMAL)
		{
			delete cipherstring;
			delete keystring;
			delete saltstr;
			MCeerror->add(EE_OPEN_BADNAME, line, pos);
			return ES_ERROR;
		}
		ivstr = ep.getsvalue().clone();
		ivlen = ep.getsvalue().getlength();
	}


	if (source->eval(ep) != ES_NORMAL)
	{
		delete cipherstring;
		delete keystring;
		delete saltstr;
		delete ivstr;
		MCeerror->add(EE_READ_BADAT, line, pos);
		return ES_ERROR;
	}

	uint4 outlen;
	char *outstr = SSL_encode(isdecrypt, cipherstring,
	                          ep.getsvalue().getstring(), ep.getsvalue().getlength(), outlen,
	                          keystring, keystringlength, ispassword, keybits,saltstr,saltlen,ivstr,ivlen);
	if (!outstr)
	{
		char sslerrbuf[256];
		if (outlen == 789)
			strcpy(sslerrbuf,"invalid cipher name");
		else if (outlen == 790)
			strcpy(sslerrbuf,"invalid keystring for specified keysize");
		else if (outlen == 791)
		{
			delete cipherstring;
			delete keystring;
			delete saltstr;
			delete ivstr;
			MCeerror -> add(EE_NO_MEMORY, line, pos);
			return ES_ERROR;
		}
		else
			SSLError(sslerrbuf);

		MCresult->copysvalue(sslerrbuf);
	}
	else
	{
		MCVariable *t_var;
		t_var = it -> evalvar(ep);
		if (t_var != NULL)
			t_var -> grab(outstr, outlen);
		else
			free(outstr);
		MCresult->clear(False);
	}
	delete cipherstring;
	delete keystring;
	delete saltstr;
	delete ivstr;
	return ES_NORMAL;
#endif /* MCEncryptionOp */
}

MCExport::~MCExport()
{
	delete fname;
	delete exsdisplay;
	delete exsstack;
	delete exsrect;
	delete mname;
	delete image;
	delete dest;
	delete palette_color_count;
	delete palette_color_list;
	delete size;
}

Parse_stat MCExport::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_EXPORT_NOTYPE, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_EXPORT, te) == PS_NORMAL)
		sformat = (Export_format)te->which;
	else
	{
		sp.backup();
		image = new MCChunk(False);
		if (image->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_EXPORT_BADTYPE, sp);
			return PS_ERROR;
		}
	}
	if (sformat == EX_SNAPSHOT)
	{
		if (sp.skip_token(SP_FACTOR, TT_FROM) == PS_NORMAL)
		{
			bool t_has_rectangle;
				
			if (sp.skip_token(SP_FACTOR, TT_PROPERTY, P_RECTANGLE) == PS_NORMAL)
			{
				if (sp.parseexp(False, True, &exsrect) != PS_NORMAL)
				{
					MCperror->add(PE_EXPORT_BADTYPE, sp);
					return PS_ERROR;
				}
				t_has_rectangle = true;
			}
			else
				t_has_rectangle = false;
			
			if (!t_has_rectangle ||	sp.skip_token(SP_FACTOR, TT_OF) == PS_NORMAL)
			{
				Symbol_type type;
				const LT *te = NULL;

				// MW-2006-05-04: Bug 3506 - crash in specific case due to not checking result of sp.lookup
				// MW-2007-09-11: [[ Bug 5242 ]] - the alternate of this if used to fail if te == NULL, this
				//   can happen though if we are looking at a variable chunk.
				if (sp.next(type) == PS_NORMAL && sp.lookup(SP_FACTOR, te) == PS_NORMAL && te -> type == TT_CHUNK && te -> which == CT_STACK)
				{
					if (sp.parseexp(False, True, &exsstack) != PS_NORMAL)
					{
						MCperror->add(PE_IMPORT_BADFILENAME, sp);
						return PS_ERROR;
					}
					
					sp.skip_token(SP_FACTOR, TT_OF);
					if (sp.skip_token(SP_EXPORT, TT_UNDEFINED, EX_DISPLAY) == PS_NORMAL)
						if (sp.parseexp(False, True, &exsdisplay) != PS_NORMAL)
						{
							MCperror->add(PE_EXPORT_BADTYPE, sp);
							return PS_ERROR;
						}
				}
				else if (te == NULL || te -> type != TT_TO)
				{
					sp . backup();
					image = new MCChunk(False);
					if (image -> parse(sp, False) != PS_NORMAL)
					{
						MCperror -> add(PE_IMPORT_BADFILENAME, sp);
						return PS_ERROR;
					}
					
					bool t_need_effects;
					if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
						t_need_effects = true, with_effects = true;
					else if (sp . skip_token(SP_SUGAR, TT_PREP, PT_WITHOUT) == PS_NORMAL)
						t_need_effects = true, with_effects = false;
					else
						t_need_effects = false;

					if (t_need_effects &&
						sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_EFFECTS) != PS_NORMAL)
					{
						MCperror -> add(PE_IMPORT_BADFILENAME, sp);
						return PS_ERROR;
					}
					
					if (sp . skip_token(SP_FACTOR, TT_PREP, PT_AT) == PS_NORMAL)
					{
						if (sp . skip_token(SP_FACTOR, TT_PROPERTY, P_SIZE) != PS_NORMAL ||
							sp . parseexp(False, True, &size) != PS_NORMAL)
						{
							MCperror -> add(PE_IMPORT_BADFILENAME, sp);
							return PS_ERROR;
						}
					}
				}
			}
		}
	}

	if (sp.skip_token(SP_FACTOR, TT_TO) != PS_NORMAL)
	{
		MCperror->add(PE_EXPORT_NOTO, sp);
		return PS_ERROR;
	}

	if (sp.skip_token(SP_OPEN, TT_UNDEFINED) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, &fname) != PS_NORMAL)
		{
			MCperror->add(PE_EXPORT_BADFILENAME, sp);
			return PS_ERROR;
		}
	}
	else
	{
		dest = new MCChunk(True);
		if (dest->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_EXPORT_NOFILE, sp);
			return PS_ERROR;
		}
	}

	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		if (sp.skip_token(SP_EXPORT, TT_UNDEFINED) != PS_NORMAL)
		{
			MCperror->add(PE_EXPORT_NOMASK, sp);
			return PS_ERROR;
		}
		if (sp.parseexp(False, True, &mname) != PS_NORMAL)
		{
			MCperror->add(PE_EXPORT_BADMASKNAME, sp);
			return PS_ERROR;
		}
	}

	if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AS) == PS_NORMAL)
	{
		if (sp.next(type) != PS_NORMAL || sp.lookup(SP_EXPORT, te) != PS_NORMAL)
		{
			MCperror->add(PE_EXPORT_BADTYPE, sp);
			return PS_ERROR;
		}
		format = (Export_format)te->which;
		if (format == EX_RAW)
		{
			Export_format t_format_list[] = {EX_RAW_ARGB, EX_RAW_ABGR, EX_RAW_RGBA, EX_RAW_BGRA};
			Export_format t_format = EX_RAW;
			for (uint32_t i=0; i<(sizeof(t_format_list) / sizeof(t_format_list[0])); i++)
			{
				if (sp.skip_token(SP_EXPORT, TT_UNDEFINED, t_format_list[i]) == PS_NORMAL)
				{
					t_format = t_format_list[i];
					break;
				}
			}
			format = t_format;
		}
		if (format == EX_GIF || format == EX_PNG || MCU_israwimageformat(format))
		{
			if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
			{
				if (sp.skip_token(SP_COMMAND, TT_STATEMENT, S_PALETTE) == PS_NORMAL)
				{
					if (sp.parseexp(False, True, &palette_color_list) != PS_NORMAL)
					{
						MCperror->add(PE_EXPORT_BADPALETTE, sp);
						return PS_ERROR;
					}
					palette_type = kMCImagePaletteTypeCustom;
					// palette specified.  if raw format then export as indexed data
					if (format == EX_RAW)
						format = EX_RAW_INDEXED;
				}
				else if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_STANDARD) == PS_NORMAL)
				{
					if (sp.skip_token(SP_COMMAND, TT_STATEMENT, S_PALETTE) != PS_NORMAL)
					{
						MCperror->add(PE_EXPORT_BADPALETTE, sp);
						return PS_ERROR;
					}
					palette_type = kMCImagePaletteTypeWebSafe;
				}
				else if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_OPTIMIZED) == PS_NORMAL)
				{
					if (sp.skip_token(SP_COMMAND, TT_STATEMENT, S_PALETTE) != PS_NORMAL)
					{
						MCperror->add(PE_EXPORT_BADPALETTE, sp);
						return PS_ERROR;
					}
					palette_type = kMCImagePaletteTypeOptimal;
				}
				else if (sp.parseexp(False, True, &palette_color_count) == PS_NORMAL)
				{
					if (sp.skip_token(SP_VALIDATION, TT_UNDEFINED, IV_COLOR) != PS_NORMAL)
					{
						MCperror->add(PE_EXPORT_BADPALETTE, sp);
						return PS_ERROR;
					}
					sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_OPTIMIZED);
					if (sp.skip_token(SP_COMMAND, TT_STATEMENT, S_PALETTE) != PS_NORMAL)
					{
						MCperror->add(PE_EXPORT_BADPALETTE, sp);
						return PS_ERROR;
					}
					palette_type = kMCImagePaletteTypeOptimal;
				}
				else
				{
					MCperror->add(PE_EXPORT_BADPALETTE, sp);
					return PS_ERROR;
				}
			}
			if (format == EX_RAW)
				format = EX_RAW_ARGB;
		}
	}
	return PS_NORMAL;
}

Exec_stat MCExport::exec(MCExecPoint &ep)
{
#ifdef /* MCExport */ LEGACY_EXEC
	MCImageBitmap *t_bitmap = nil;
	MCObject *optr = NULL;

	Exec_stat t_status = ES_NORMAL;
	bool t_delete_file_on_error = false;

	if (image != NULL)
	{
		//get image from chunk
		uint4 parid;
		if (image->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_EXPORT_NOSELECTED, line, pos);
			return ES_ERROR;
		}
	}
	
	// MW-2013-05-20: [[ Bug 10897 ]] Object snapshot returns a premultipled
	//   bitmap, which needs to be processed before compression. This flag
	//   indicates to do this processing later on in the method.
	bool t_needs_unpremultiply;
	t_needs_unpremultiply = false;
	if (sformat == EX_SNAPSHOT)
	{
		char *srect = NULL;
		if (exsrect != NULL)
		{
			if (exsrect->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_EXPORT_NOSELECTED, line, pos);
				return ES_ERROR;
			}
			srect = ep.getsvalue().clone();
		}

		char *sstack = NULL;
		if (exsstack != NULL)
		{
			if (exsstack->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_EXPORT_NOSELECTED, line, pos);
				delete srect;
				return ES_ERROR;
			}
			sstack = ep.getsvalue().clone();
		}

		char *sdisp = NULL;
		if (exsdisplay != NULL)
		{
			if (exsdisplay->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_EXPORT_NOSELECTED, line, pos);
				return ES_ERROR;
			}
			sdisp = ep.getsvalue().clone();
		}
		
		MCPoint t_wanted_size;
		if (size != NULL)
		{
			if (size -> eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_EXPORT_NOSELECTED, line, pos);
				return ES_ERROR;
			}
			if (!MCU_stoi2x2(ep . getsvalue(), t_wanted_size . x, t_wanted_size . y))
			{
				MCeerror->add(EE_EXPORT_NOSELECTED, line, pos);
				return ES_ERROR;
			}
		}

		MCRectangle r;
		r.x = r.y = -32768;
		r.width = r.height = 0;
		if (srect != NULL)
		{
			int2 i1, i2, i3, i4;
			if (!MCU_stoi2x4(srect, i1, i2, i3, i4))
			{
				MCeerror->add(EE_EXPORT_NOSELECTED, line, pos);
				delete srect;
				delete sdisp;
				delete sstack;
				return ES_ERROR;
			}
			r.x = i1;
			r.y = i2;
			r.width = MCU_max(i3 - i1, 0);
			r.height = MCU_max(i4 - i2, 0);
		}
		delete srect;

		uint4 w = 0;
		if (sstack != NULL)
		{
			if (!MCU_stoui4(sstack, w))
			{
				delete sstack;
				delete sdisp;
				MCeerror->add(EE_EXPORT_NOSELECTED, line, pos);
				return ES_ERROR;
			}
		}
		delete sstack;

		if (optr != NULL)
		{
			/* UNCHECKED */ t_bitmap = optr -> snapshot(exsrect == NULL ? nil : &r, size == NULL ? nil : &t_wanted_size, 1.0, with_effects);
			// OK-2007-04-24: Bug found in ticket 2006072410002591, when exporting a snapshot of an object
			// while the object is being moved in the IDE, it is possible for the snapshot rect not to intersect with
			// the rect of the object, causing optr -> snapshot() to return NULL, and a crash.
			if (t_bitmap == nil)
			{
				delete sdisp;
				MCeerror -> add(EE_EXPORT_EMPTYRECT, line, pos);
				return ES_ERROR;
			}
			
			// MW-2013-05-20: [[ Bug 10897 ]] The 'snapshot' command produces a premultiplied bitmap
			//   so mark it to be unpremultiplied for later on.
			t_needs_unpremultiply = true;
		}
		else
		{
			t_bitmap = MCscreen->snapshot(r, 1.0, w, sdisp);
			if (t_bitmap == nil)
			{
				delete sdisp;
				MCeerror->add(EE_EXPORT_NOSELECTED, line, pos);
				return ES_ERROR;
			}
		}
		delete sdisp;
	}
	else if (optr == NULL)
	{
		optr = MCselected->getfirst();
		if (optr == NULL)
		{
			MCCard *cardptr = MCdefaultstackptr->getchild(CT_THIS, MCnullmcstring, CT_CARD);
			optr = cardptr->getchild(CT_LAST, MCnullmcstring, CT_IMAGE, CT_UNDEFINED);
		}
		if (optr == NULL || !optr->getopened())
		{
			MCeerror->add(EE_EXPORT_NOSELECTED, line, pos);
			return ES_ERROR;
		}
		if (optr->gettype() != CT_IMAGE)
		{
			MCeerror->add(EE_EXPORT_NOTANIMAGE, line, pos);
			return ES_ERROR;
		}
	}

	IO_handle stream = NULL;
	char *name = NULL;
	if (fname != NULL)
	{
		if (fname->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_EXPORT_BADNAME, line, pos);
			return ES_ERROR;
		}
		if (MCsecuremode & MC_SECUREMODE_DISK)
		{
			MCeerror->add(EE_DISK_NOPERM, line, pos, name);
			return ES_ERROR;
		}
		name = ep.getsvalue().clone();
		if ((stream = MCS_open(name, IO_WRITE_MODE, False, False, 0)) == NULL)
		{
			MCeerror->add(EE_EXPORT_CANTOPEN, line, pos, name);
			delete name;
			return ES_ERROR;
		}
	}
	IO_handle mstream = NULL;
	if (mname != NULL)
	{
		if (mname->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_EXPORT_BADNAME, line, pos);
			return ES_ERROR;
		}
		if (MCsecuremode & MC_SECUREMODE_DISK)
		{
			MCeerror->add(EE_DISK_NOPERM, line, pos, name);
			return ES_ERROR;
		}
		char *mfile = ep.getsvalue().clone();
		if ((mstream = MCS_open(mfile, IO_WRITE_MODE, False, False, 0)) == NULL)
		{
			MCeerror->add(EE_EXPORT_CANTOPEN, line, pos, mfile);
			delete mfile;
			return ES_ERROR;
		}
		delete mfile;
	}

	bool t_dither = false;
	bool t_image_locked = false;
	if (t_bitmap == nil)
	{
		MCImage *t_img = static_cast<MCImage*>(optr);
		
		// IM-2013-07-26: [[ ResIndependence ]] the exported image needs to be unscaled,
		// so if the image has a scale factor we need to get a 1:1 copy
		if (t_img->getscalefactor() == 1.0)
		{
			/* UNCHECKED */ t_img->lockbitmap(t_bitmap, false);
			t_image_locked = true;
		}
		else
		{
			/* UNCHECKED */ t_img->copybitmap(1.0, false, t_bitmap);
		}
		t_dither = !t_img->getflag(F_DONT_DITHER);
	}
	else
	{
		// MW-2013-05-20: [[ Bug 10897 ]] Make sure we unpremultiply if needed.
		if (t_needs_unpremultiply)
			MCImageBitmapUnpremultiply(t_bitmap);
		MCImageBitmapCheckTransparency(t_bitmap);
		t_dither = !MCtemplateimage->getflag(F_DONT_DITHER);
	}

	IO_handle t_out_stream = nil;
	if (stream != nil)
		t_out_stream = stream;
	else
		/* UNCHECKED */ t_out_stream = MCS_fakeopenwrite();

	if (t_bitmap == nil || t_bitmap->width == 0 || t_bitmap->height == 0)
		ep.clear();
	else
	{
		MCImagePaletteSettings t_palette_settings = {kMCImagePaletteTypeEmpty, NULL, 0};
		MCImagePaletteSettings *t_ps_ptr = &t_palette_settings;
		t_palette_settings.type = palette_type;
		switch (palette_type)
		{
		case kMCImagePaletteTypeCustom:
			if (palette_color_list->eval(ep) != ES_NORMAL || ep.isempty() || !MCImageParseColourList(ep.getsvalue(), t_palette_settings.ncolors, t_palette_settings.colors))
			{
				MCeerror -> add(EE_EXPORT_BADPALETTE, line, pos);
				t_status = ES_ERROR;
			}
			break;
		case kMCImagePaletteTypeOptimal:
			if (palette_color_count == NULL)
				t_palette_settings.ncolors = 256;
			else
			{
				if (palette_color_count->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
				{
					MCeerror -> add(EE_EXPORT_BADPALETTESIZE, line, pos);
					t_status = ES_ERROR;
				}
				else
				{
					int32_t t_ncolours = ep.getint4();
					if (t_ncolours < 1 || t_ncolours > 256)
					{
						MCeerror -> add(EE_EXPORT_BADPALETTESIZE, line, pos);
						t_status = ES_ERROR;
					}
					else
						t_palette_settings.ncolors = t_ncolours;
				}
			}
			break;
		case kMCImagePaletteTypeWebSafe:
			break;
		case kMCImagePaletteTypeEmpty:
			t_ps_ptr = NULL;
		}
		if (t_status == ES_NORMAL)
		{
			if (!MCImageExport(t_bitmap, format, &t_palette_settings, t_dither, t_out_stream, mstream))
			{
				t_delete_file_on_error = true;
				MCeerror->add(EE_EXPORT_CANTWRITE, line, pos, name);
				t_status = ES_ERROR;
			}
		}
		delete t_palette_settings.colors;
	}

	if (t_image_locked)
		static_cast<MCImage*>(optr)->unlockbitmap(t_bitmap);
	else
		MCImageFreeBitmap(t_bitmap);

	if (stream == nil && t_out_stream != nil)
	{
		char *t_buffer;
		uint32_t t_buffer_length;
		MCS_fakeclosewrite(t_out_stream, t_buffer, t_buffer_length);
		ep.grabbuffer(t_buffer, t_buffer_length);
	}

	if (fname != NULL)
	{
		MCS_close(stream);
		if (t_status != ES_NORMAL && t_delete_file_on_error)
			MCS_unlink(name);
		delete name;
	}
	else
		if (t_status == ES_NORMAL)
		{
			if (dest->set(ep, PT_INTO) != ES_NORMAL)
			{
				MCeerror->add(EE_EXPORT_CANTWRITE, line, pos);
				t_status = ES_ERROR;
			}
		}
	return t_status;
#endif /* MCExport */
}

////////////////////////////////////////////////////////////////////////////////

// JS-2013-07-01: [[ EnhancedFilter ]] Implementation of pattern matching classes.

MCPatternMatcher::~MCPatternMatcher()
{
	delete pattern;
}

Exec_stat MCRegexMatcher::compile(uint2 line, uint2 pos)
{
	// MW-2013-07-01: [[ EnhancedFilter ]] Removed 'usecache' parameter as there's
	//   no reason not to use the cache.
	compiled = MCR_compile(pattern, casesensitive);
	if (compiled == NULL)
	{
		MCeerror->add
		(EE_MATCH_BADPATTERN, line, pos, MCR_geterror());
		return ES_ERROR;
	}
    return ES_NORMAL;
}

Boolean MCRegexMatcher::match(char *s)
{
	return MCR_exec(compiled, s, strlen(s));
}

Exec_stat MCWildcardMatcher::compile(uint2 line, uint2 pos)
{
    // wildcard patterns are not compiled
    return ES_NORMAL;
}

#define OPEN_BRACKET '['
#define CLOSE_BRACKET ']'

/* static */ Boolean MCWildcardMatcher::match(char *s, char *p, Boolean casesensitive)
{
	uint1 scc, c;
    
	while (*s)
	{
		scc = *s++;
		c = *p++;
		switch (c)
		{
            case OPEN_BRACKET:
			{
				Boolean ok = False;
				int lc = -1;
				int notflag = 0;
                
				if (*p == '!' )
				{
					notflag = 1;
					p++;
				}
				while (*p)
				{
					c = *p++;
					if (c == CLOSE_BRACKET && lc >= 0)
						return ok ? match(s, p, casesensitive) : 0;
					else
						if (c == '-' && lc >= 0 && *p != CLOSE_BRACKET)
						{
							c = *p++;
							if (notflag)
							{
								if (lc > scc || scc > c)
									ok = True;
								else
									return False;
							}
							else
							{
								if (lc < scc && scc <= c)
									ok = True;
							}
						}
						else
						{
							if (notflag)
							{
								if (scc != c)
									ok = True;
								else
									return False;
							}
							else
								if (scc == c)
									ok = True;
							lc = c;
						}
				}
			}
                return False;
            case '?':
                break;
            case '*':
                while (*p == '*')
                    p++;
                if (*p == 0)
                    return True;
                --s;
                c = *p;
                while (*s)
                    if ((casesensitive ? c != *s : MCS_tolower(c) != MCS_tolower(*s))
				        && *p != '?' && *p != OPEN_BRACKET)
                        s++;
                    else
                        if (match(s++, p, casesensitive))
                            return True;
                return False;
            case 0:
                return scc == 0;
            default:
                if (casesensitive)
                {
                    if (c != scc)
                        return False;
                }
                else
                    if (MCS_tolower(c) != MCS_tolower(scc))
                        return False;
                break;
		}
	}
	while (*p == '*')
		p++;
	return *p == 0;
}

Boolean MCWildcardMatcher::match(char *s)
{
	char *p = pattern;
	return match(s, p, casesensitive);
}

MCFilter::~MCFilter()
{
	delete container;
	delete target;
	delete it;
	delete source;
	delete pattern;
}

// JS-2013-07-01: [[ EnhancedFilter ]] Replacement for filterlines which takes a delimiter and
//   pattern matching class.
char *MCFilter::filterdelimited(char *sstring, char delimiter, MCPatternMatcher *matcher)
{
	bool t_success;
	t_success = true;
	
	uint32_t t_length;
	t_length = MCCStringLength(sstring);
	
	uint4 offset = 0;
	char *dstring;
	dstring = nil;
	if (t_success)
		t_success = MCMemoryAllocate(t_length + 1, dstring);
	
	// OK-2010-01-11: Bug 7649 - Filter command was incorrectly removing empty lines.
	// Now does:
	// 1. Remove terminal delimiter from list
	// 2. Do the filtering (now using strchr instead of strtok to fix the original bug)
	// 3. If the filtered list is non-empty and had a terminal delimiter, put a return after it.

	// Duplicate input string because the algorithm needs to change it.
	char *t_string;
	t_string = nil;
	
	if (t_success)
		t_success = MCCStringClone(sstring, t_string);

	if (!t_success)
	{
		// IM-2013-07-26: [[ Bug 10774 ]] return nil if memory allocation fails
		MCMemoryDeallocate(dstring);
		MCCStringFree(t_string);
		
		return nil;
	}
	
	// Keep a copy of the original pointer so it can be freed
	char *t_original_string;
	t_original_string = t_string;

	// MW-2010-10-05: [[ Bug 9034 ]] If t_string is of zero length, then the next couple
	//   of lines will cause problems so return empty in this case.
	if (t_length == 0)
	{
		free(t_original_string);
		*dstring = '\0';
		return dstring;
	}

	// Record whether or not the string was terminated with a trailing delimiter,
	// if it was, then remove this trailing delimiter.
	bool t_was_terminated;
	t_was_terminated = (t_string[t_length - 1] == delimiter);
	if (t_was_terminated)
		t_string[t_length - 1] = '\0';

	for(;;)
	{
		char *t_return;
		t_return = strchr(t_string, delimiter);

		if (t_return != nil)
			*t_return = '\0';

		char *chunk;
		chunk = t_string;
		if (matcher->match(chunk) != discardmatches)
		{
			if (offset)
				dstring[offset++] = delimiter;

			// MW-2010-10-18: [[ Bug 7864 ]] This should be a 32-bit integer - removing 65535 char limit.
			uint32_t length = strlen(chunk);
			memcpy(&dstring[offset], chunk, length);
			offset += length;
		}

		if (t_return == nil)
			break;

		t_string = t_return + 1;
	}

	if (offset != 0 && t_was_terminated)
		dstring[offset++] = delimiter;


	free(t_original_string);

	dstring[offset] = '\0';
	return dstring;
}

// JS-2013-07-01: [[ EnhancedFilter ]] Rewritten to support new filter syntax.
Parse_stat MCFilter::parse(MCScriptPoint &sp)
{
	// Syntax :
	//   filter [ ( lines | items ) of ] <container_or_exp>
	//          ( with | without | [ not ] matching )
	//          [ { wildcard | regex } [ pattern ] ] <pattern>
	//          [ into <container> ]
	//
	Parse_errors t_error;
	t_error = PE_UNDEFINED;
    
	initpoint(sp);

	// Parse the chunk type (if present)
	if (t_error == PE_UNDEFINED)
	{
		// First check for 'lines' or 'items'.
		if (sp.skip_token(SP_FACTOR, TT_CLASS, CT_LINE) == PS_NORMAL)
			chunktype = CT_LINE;
		else if (sp.skip_token(SP_FACTOR, TT_CLASS, CT_ITEM) == PS_NORMAL)
			chunktype = CT_ITEM;
		// If we parsed a chunk then ensure there's an 'of'
		if (chunktype != CT_UNDEFINED && sp.skip_token(SP_FACTOR, TT_OF) != PS_NORMAL)
			t_error = PE_FILTER_BADDEST;
	}

	// If there was no error and no chunk type then default to line
	if (t_error == PE_UNDEFINED && chunktype == CT_UNDEFINED)
		chunktype = CT_LINE;

	// Next parse the source container or expression
	if (t_error == PE_UNDEFINED)
	{
		MCerrorlock++;
		MCScriptPoint tsp(sp);
		container = new MCChunk(True);
		if (container->parse(sp, False) != PS_NORMAL)
		{
			sp = tsp;
			MCerrorlock--;
			delete container;
			container = NULL;
			if (sp.parseexp(True, True, &source) != PS_NORMAL)
			{
				t_error = PE_FILTER_BADDEST;
			}
		}
		else
			MCerrorlock--;
	}

	// Now look for the filter mode
	if (t_error == PE_UNDEFINED)
	{
		if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
			discardmatches = False;
		else if (sp.skip_token(SP_SUGAR, TT_PREP, PT_WITHOUT) == PS_NORMAL)
			discardmatches = True;
		else if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_MATCHING) == PS_NORMAL)
			discardmatches = False;
		else if (sp.skip_token(SP_FACTOR, TT_UNOP, O_NOT) == PS_NORMAL
				 && sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_MATCHING) == PS_NORMAL)
			discardmatches = True;
		else
			t_error = PE_FILTER_NOWITH;
	}

	// Now look for the optional pattern match mode
	if (t_error == PE_UNDEFINED)
	{
		if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_REGEX) == PS_NORMAL)
			matchmode = MA_REGEX;
		else if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_WILDCARD) == PS_NORMAL)
			matchmode = MA_WILDCARD;
		// Skip the optional pattern keyword
		sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_PATTERN);
	}

	// Now parse the pattern expression
	if (t_error == PE_UNDEFINED && sp.parseexp(False, True, &pattern) != PS_NORMAL)
		t_error = PE_FILTER_BADEXP;

	// Finally check for the (optional) 'into' clause
	if (t_error == PE_UNDEFINED)
	{
		if (sp.skip_token(SP_FACTOR, TT_PREP, PT_INTO) == PS_NORMAL)
		{
			target = new MCChunk(True);
			if (target->parse(sp, False) != PS_NORMAL)
				t_error = PE_FILTER_BADDEST;
        }
        else if (container == NULL)
			getit(sp, it);
	}

	// If we encountered an error, add it to the parse error stack and fail.
	if (t_error != PE_UNDEFINED)
	{
		MCperror->add(t_error, sp);
		return PS_ERROR;
	}

	// Success!
	return PS_NORMAL;
}

// JS-2013-07-01: [[ EnhancedFilter ]] Rewritten to support new syntax.
Exec_stat MCFilter::exec(MCExecPoint &ep)
{
#ifdef /* MCFilter */ LEGACY_EXEC
	Exec_stat stat;

	// Evaluate the container or source expression
	if (container != NULL)
		stat = container->eval(ep);
	else
		stat = source->eval(ep);
	if (stat != ES_NORMAL)
	{
		MCeerror->add(EE_FILTER_CANTGET, line, pos);
		return ES_ERROR;
	}
	char *sptr = ep.getsvalue().clone();

	// Evaluate the pattern expression
	if (pattern->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_FILTER_CANTGETPATTERN, line, pos);
		delete sptr;
		return ES_ERROR;
	}
	
	// MW-2013-07-01: [[ EnhancedFilter ]] Use the ep directly as the matcher
	//   classes copy the pattern string.
	// Create the pattern matcher
	MCPatternMatcher *matcher;
	if (matchmode == MA_REGEX)
        matcher = new MCRegexMatcher(ep.getcstring(), ep.getcasesensitive());
    else
		matcher = new MCWildcardMatcher(ep.getcstring(), ep.getcasesensitive());
	stat = matcher->compile(line, pos);
	if (stat != ES_NORMAL)
	{
		delete sptr;
		delete matcher;
		return stat;
	}

	// Determine the delimiter
	char delimiter;
	if (chunktype == CT_LINE)
		delimiter = ep.getlinedel();
	else
		delimiter = ep.getitemdel();

	// Filter the data
	char *dptr = filterdelimited(sptr, delimiter, matcher);
	delete sptr;
	delete matcher;
	
	// IM-2013-07-26: [[ Bug 10774 ]] if filterlines returns nil throw a "no memory" error
	if (dptr == nil)
	{
		MCeerror->add(EE_NO_MEMORY, line, pos);
		return ES_ERROR;
	}

	ep.copysvalue(dptr, strlen(dptr));
	delete dptr;

	// Now put the filtered data into the correct container
	if (it != NULL)
		stat = it->set(ep);
	else if (target != NULL)
		stat = target->set(ep, PT_INTO);
	else
		stat = container->set(ep, PT_INTO);
	if (stat != ES_NORMAL)
	{
		MCeerror->add(EE_FILTER_CANTSET, line, pos);
		return ES_ERROR;
	}

	// Success!
	return ES_NORMAL;
#endif /* MCFilter */
}

////////////////////////////////////////////////////////////////////////////////

MCImport::~MCImport()
{
	delete fname;
	delete mname;
	delete dname;
	delete container;
	delete size;
}

// import snapshot [ from rectangle <expr> [of window <window>] [of display <display>] ]
Parse_stat MCImport::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_IMPORT_NOTYPE, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_EXPORT, te) != PS_NORMAL)
	{
		MCperror->add(PE_IMPORT_BADTYPE, sp);
		return PS_ERROR;
	}
	format = (Export_format)te->which;
	if (format == EX_SNAPSHOT)
	{
		if (sp.skip_token(SP_FACTOR, TT_FROM) == PS_NORMAL)
		{
			bool t_has_rectangle;
		
			if (sp.skip_token(SP_FACTOR, TT_PROPERTY, P_RECTANGLE) == PS_NORMAL)
			{
				if (sp.parseexp(False, True, &fname) != PS_NORMAL)
				{
					MCperror->add(PE_IMPORT_BADFILENAME, sp);
					return PS_ERROR;
				}
			
				t_has_rectangle = true;
			}
			else
				t_has_rectangle = false;

			if (!t_has_rectangle || sp.skip_token(SP_FACTOR, TT_OF) == PS_NORMAL)
			{
				Symbol_type type;
				const LT *te = NULL;

				// MW-2006-03-24: Bug 3442 - crash in specific case due to not checking result of sp.lookup
				if (sp.next(type) == PS_NORMAL && sp.lookup(SP_FACTOR, te) == PS_NORMAL && te -> type == TT_CHUNK && te -> which == CT_STACK)
				{
					if (sp.parseexp(False, True, &mname) != PS_NORMAL)
					{
						MCperror->add(PE_IMPORT_BADFILENAME, sp);
						return PS_ERROR;
					}

					sp.skip_token(SP_FACTOR, TT_OF);
					if (sp.skip_token(SP_EXPORT, TT_UNDEFINED, EX_DISPLAY) == PS_NORMAL)
						if (sp.parseexp(False, True, &dname) != PS_NORMAL)
						{
							MCperror->add(PE_IMPORT_BADFILENAME, sp);
							return PS_ERROR;
						}
				}
				else
				{
					sp . backup();
					container = new MCChunk(False);
					if (container -> parse(sp, False) != PS_NORMAL)
					{
						MCperror -> add(PE_IMPORT_BADFILENAME, sp);
						return PS_ERROR;
					}

					bool t_need_effects;
					if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
						t_need_effects = true, with_effects = true;
				else if (sp . skip_token(SP_SUGAR, TT_PREP, PT_WITHOUT) == PS_NORMAL)
						t_need_effects = true, with_effects = false;
					else
						t_need_effects = false;

					if (t_need_effects &&
						sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_EFFECTS) != PS_NORMAL)
					{
						MCperror -> add(PE_IMPORT_BADFILENAME, sp);
						return PS_ERROR;
					}
					
					if (sp . skip_token(SP_FACTOR, TT_PREP, PT_AT) == PS_NORMAL)
					{
						if (sp . skip_token(SP_FACTOR, TT_PROPERTY, P_SIZE) != PS_NORMAL ||
							sp . parseexp(False, True, &size) != PS_NORMAL)
						{
							MCperror -> add(PE_IMPORT_BADFILENAME, sp);
							return PS_ERROR;
						}
					}
				}
			}
		}
		return PS_NORMAL;
	}
	if (sp.skip_token(SP_FACTOR, TT_FROM) != PS_NORMAL)
	{
		MCperror->add(PE_IMPORT_NOFROM, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_OPEN, TT_UNDEFINED) != PS_NORMAL)
	{
		MCperror->add(PE_IMPORT_NOFILE, sp);
		return PS_ERROR;
	}
	if (sp.parseexp(False, True, &fname) != PS_NORMAL)
	{
		MCperror->add(PE_IMPORT_BADFILENAME, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		if (sp.skip_token(SP_EXPORT, TT_UNDEFINED) != PS_NORMAL)
		{
			MCperror->add(PE_IMPORT_NOMASK, sp);
			return PS_ERROR;
		}
		if (sp.parseexp(False, True, &mname) != PS_NORMAL)
		{
			MCperror->add(PE_IMPORT_BADMASKNAME, sp);
			return PS_ERROR;
		}
	}
	if (sp.skip_token(SP_FACTOR, TT_IN) == PS_NORMAL
	        || sp.skip_token(SP_FACTOR, TT_PREP) == PS_NORMAL)
	{
		container = new MCChunk(False);
		if (container->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_CREATE_BADBGORCARD, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

Exec_stat MCImport::exec(MCExecPoint &ep)
{
#ifdef /* MCImport */ LEGACY_EXEC
	if (format == EX_SNAPSHOT)
	{
		if ((container == NULL) && (MCsecuremode & MC_SECUREMODE_PRIVACY))
		{
			MCeerror->add(EE_PRIVACY_NOPERM, line, pos);
			return ES_ERROR;
		}
	}
	else
	{
		if (MCsecuremode & MC_SECUREMODE_DISK)
		{
			MCeerror->add(EE_DISK_NOPERM, line, pos);
			return ES_ERROR;
		}
	}
	if (format != EX_STACK && MCdefaultstackptr->islocked())
	{
		MCeerror->add(EE_IMPORT_LOCKED, line, pos);
		return ES_ERROR;
	}

	char *ifile = NULL;
	if (fname != NULL)
	{
		if (fname->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_IMPORT_BADNAME, line, pos);
			return ES_ERROR;
		}
		ifile = ep.getsvalue().clone();
	}
	char *mfile = NULL;
	if (mname != NULL)
	{
		if (mname->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_IMPORT_BADNAME, line, pos);
			delete ifile;
			return ES_ERROR;
		}
		mfile = ep.getsvalue().clone();
	}

	// MW-2013-05-20: [[ Bug 10897 ]] Object snapshot returns a premultipled
	//   bitmap, which needs to be processed before compression. This flag
	//   indicates to do this processing later on in the method.
	bool t_needs_unpremultiply;
	t_needs_unpremultiply = false;
	if (format == EX_SNAPSHOT)
	{
		// IM-2013-08-01: [[ ResIndependence ]] Resolution scale for snapshots - unless specified should produce point-scale image
		MCGFloat t_image_scale;
		t_image_scale = 1.0;
		
		char *disp = NULL;
		if (dname != NULL)
		{
			if (dname->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_IMPORT_BADNAME, line, pos);
				return ES_ERROR;
			}
			disp = ep.getsvalue().clone();
		}

		MCRectangle r;
		r.x = r.y = -32768;
		r.width = r.height = 0;
		if (ifile != NULL)
		{
			int2 i1, i2, i3, i4;
			if (!MCU_stoi2x4(ifile, i1, i2, i3, i4))
			{
				MCeerror->add(EE_IMPORT_BADNAME, line, pos);
				delete ifile;
				delete mfile;
				delete disp;
				return ES_ERROR;
			}
			r.x = i1;
			r.y = i2;
			r.width = MCU_max(i3 - i1, 0);
			r.height = MCU_max(i4 - i2, 0);
			delete ifile;
		}

		uint4 w = 0;
		if (mfile != NULL)
		{
			if (!MCU_stoui4(mfile, w))
			{
				delete mfile;
				delete disp;
				MCeerror->add(EE_IMPORT_BADNAME, line, pos);
				return ES_ERROR;
			}
			delete mfile;
		}
		
		MCPoint t_wanted_size;
		if (size != NULL)
		{
			if (size -> eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_EXPORT_NOSELECTED, line, pos);
				return ES_ERROR;
			}
			if (!MCU_stoi2x2(ep . getsvalue(), t_wanted_size . x, t_wanted_size . y))
			{
				MCeerror->add(EE_EXPORT_NOSELECTED, line, pos);
				return ES_ERROR;
			}
		}
		
		MCImageBitmap *t_bitmap = nil;
		if (container != NULL)
		{
			MCObject *parent = NULL;
			uint4 parid;
			if (container->getobj(ep, parent, parid, True) != ES_NORMAL)
			{
				MCeerror -> add(EE_IMPORT_BADNAME, line, pos);
				return ES_ERROR;
			}
		
			t_bitmap = parent -> snapshot(fname == NULL ? nil : &r, size == NULL ? nil : &t_wanted_size, t_image_scale, with_effects);
			// OK-2007-04-24: If the import rect doesn't intersect with the object, MCobject::snapshot
			// may return null. In this case, return an error.
			if (t_bitmap == NULL)
			{
				MCeerror ->add(EE_IMPORT_EMPTYRECT, line, pos);
				return ES_ERROR;
			}
			
			// MW-2013-05-20: [[ Bug 10897 ]] The 'snapshot' command produces a premultiplied bitmap
			//   so mark it to be unpremultiplied for later on.
			t_needs_unpremultiply = true;
		}
		else
		{
			t_bitmap = MCscreen->snapshot(r, t_image_scale, w, disp);

			delete disp;
		}

		MCImage *iptr = nil;
		if (t_bitmap != nil)
		{
			// MW-2013-05-20: [[ Bug 10897 ]] Make sure we unpremultiply if needed.
			if (t_needs_unpremultiply)
				MCImageBitmapUnpremultiply(t_bitmap);
			MCImageBitmapCheckTransparency(t_bitmap);

			/* UNCHECKED */ iptr = (MCImage *)MCtemplateimage->clone(False, OP_NONE, false);
			// IM-2013-08-01: [[ ResIndependence ]] pass image scale when setting bitmap
			if (t_bitmap != nil)
				iptr->setbitmap(t_bitmap, t_image_scale, true);
			MCImageFreeBitmap(t_bitmap);
		}
	
		if (iptr != NULL)
			iptr->attach(OP_CENTER, false);

		return ES_NORMAL;
	}
	
	MCU_watchcursor(ep.getobj()->getstack(), True);
	
	Exec_stat stat = ES_NORMAL;
	IO_handle stream;
	if ((stream = MCS_open(ifile, IO_READ_MODE, True, False, 0)) == NULL)
	{
		MCeerror->add(EE_IMPORT_CANTOPEN, line, pos, ifile);
		delete ifile;
		
		// MW-2007-12-17: [[ Bug 266 ]] The watch cursor must be reset before we
		//   return back to the caller.
		MCU_unwatchcursor(ep.getobj()->getstack(), True);
		
		return ES_ERROR;
	}
	switch (format)
	{
	case EX_AUDIO_CLIP:
		{
			MCAudioClip *aptr = new MCAudioClip;
			if (!aptr->import(ifile, stream))
			{
				MCeerror->add(EE_IMPORT_CANTREAD, line, pos, ifile);
				delete aptr;
				stat = ES_ERROR;
				break;
			}
			MCdefaultstackptr->appendaclip(aptr);
		}
		break;
	case EX_VIDEO_CLIP:
		{
			MCVideoClip *vptr = new MCVideoClip;
			if (!vptr->import(ifile, stream))
			{
				MCeerror->add(EE_IMPORT_CANTREAD, line, pos, ifile);
				delete vptr;
				stat = ES_ERROR;
				break;
			}
			MCdefaultstackptr->appendvclip(vptr);
		}
		break;
	case EX_EPS:
		{
			MCEPS *eptr = new MCEPS;
			if (!eptr->import(ifile, stream))
			{
				MCeerror->add(EE_IMPORT_CANTREAD, line, pos, ifile);
				delete eptr;
				stat = ES_ERROR;
				break;
			}
			eptr->attach(OP_CENTER, false);
		}
		break;
	case EX_STACK:
		{
			MCStack *sptr;
			if (hc_import(strclone(ifile), stream, sptr) != IO_NORMAL)
			{
				MCS_close(stream);
				MCeerror->add(EE_IMPORT_CANTREAD, line, pos, ifile);
				stat = ES_ERROR;
			}
			sptr->open();
		}
		break;
	default:
		{
			MCObject *parent = NULL;
			if (container != NULL)
			{
				uint4 parid;
				if (container->getobj(ep, parent, parid, True) != ES_NORMAL
				        || parent->gettype() != CT_GROUP && parent->gettype() != CT_CARD)
				{
					MCeerror->add(EE_CREATE_BADBGORCARD, line, pos);
					stat = ES_ERROR;
					break;
				}
			}
			IO_handle mstream = NULL;
			if (mfile != NULL)
			{
				if ((mstream = MCS_open(mfile, IO_READ_MODE, True, False, 0)) == NULL)
				{
					MCeerror->add(EE_IMPORT_CANTOPEN, line, pos, mfile);
					delete mfile;
					stat = ES_ERROR;
					break;
				}
				delete mfile;
			}
			MCtemplateimage->setparent(parent);
			MCImage *iptr = (MCImage *)MCtemplateimage->clone(False, OP_NONE, false);
			MCtemplateimage->setparent(NULL);
			iptr->setflag(True, F_I_ALWAYS_BUFFER);
			if (iptr->import(ifile, stream, mstream) != IO_NORMAL)
			{
				if (mstream != NULL)
					MCS_close(mstream);
				delete iptr;
				MCeerror->add(EE_IMPORT_CANTREAD, line, pos, ifile);
				stat = ES_ERROR;
				break;
			}
			if (mstream != NULL)
				MCS_close(mstream);
			iptr->attach(OP_CENTER, false);
		}
		break;
	}
	delete ifile;
	MCS_close(stream);
	
	// MW-2007-12-17: [[ Bug 266 ]] The watch cursor must be reset before we
	//   return back to the caller.
	MCU_unwatchcursor(ep.getobj()->getstack(), True);
	
	return stat;
#endif /* MCImport */
}

typedef struct
{
	const char *token;
	int4 which;
}
ST;

static ST signal_table[] = {
#if defined(TARGET_PLATFORM_LINUX)
                               {"abrt", SIGABRT}, {"alrm", SIGALRM}, {"bus", SIGBUS}, {"chld", SIGCHLD},
                               {"cld", SIGCLD}, {"cont", SIGCONT}, {"emt", SIGBOGUS}, {"fpe", SIGFPE},
                               {"hup", SIGHUP}, {"ill", SIGILL}, {"info", SIGBOGUS}, {"int", SIGINT},
                               {"io", SIGIO}, {"iot", SIGIOT}, {"kill", SIGKILL}, {"lwp", SIGBOGUS},
                               {"phone", SIGBOGUS}, {"pipe", SIGPIPE}, {"poll", SIGPOLL}, {"prof", SIGPROF},
                               {"pwr", SIGPWR}, {"quit", SIGQUIT}, {"segv", SIGSEGV}, {"stop", SIGSTOP},
                               {"sys", SIGSYS}, {"term", SIGTERM}, {"trap", SIGTRAP}, {"tstp", SIGTSTP},
                               {"ttin", SIGTTIN}, {"ttou", SIGTTOU}, {"urg", SIGURG}, {"usr1", SIGUSR1},
                               {"usr2", SIGUSR2}, {"vtalrm", SIGVTALRM}, {"waiting", SIGBOGUS},
                               {"winch", SIGWINCH}, {"xcpu", SIGXCPU}, {"xfsz", SIGXFSZ}
#else
                               {"xfsz", 1}
#endif
                           };

int4 MCKill::lookup(const MCString &s)
{
	uint2 size = ELEMENTS(signal_table);
	while(size--)
		if (s == signal_table[size].token)
			return signal_table[size].which;
	return SIGTERM;
}

MCKill::~MCKill()
{
	delete sig;
	delete pname;
}

Parse_stat MCKill::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.skip_token(SP_OPEN, TT_UNDEFINED, OA_PROCESS) != PS_NORMAL)
	{
		if (sp.parseexp(False, True, &sig) != PS_NORMAL)
		{
			MCperror->add
			(PE_KILL_NOPROCESS, sp);
			return PS_ERROR;
		}
		if (sp.skip_token(SP_OPEN, TT_UNDEFINED, OA_PROCESS) != PS_NORMAL)
		{
			MCperror->add
			(PE_KILL_NOPROCESS, sp);
			return PS_ERROR;
		}
	}
	if (sp.parseexp(False, True, &pname) != PS_NORMAL)
	{
		MCperror->add
		(PE_KILL_BADNAME, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCKill::exec(MCExecPoint &ep)
{
#ifdef /* MCKill */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_PROCESS)
	{
		MCeerror->add
		(EE_PROCESS_NOPERM, line, pos);
		return ES_ERROR;
	}
	int4 number = SIGTERM;
	if (sig != NULL)
	{
		if (sig->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_KILL_BADNUMBER, line, pos);
			return ES_ERROR;
		}
		if (ep.getint4(number, line, pos, EE_KILL_BADNUMBER) == ES_NORMAL)
			number = MCU_abs(number);
		else
			number = lookup(ep.getsvalue());
	}
	if (pname->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_KILL_BADNAME, line, pos);
		return ES_ERROR;
	}
	char *name = ep.getsvalue().clone();
	uint2 index;
	if (IO_findprocess(name, index))
	{
		if (number == SIGTERM || number == SIGKILL)
			MCS_closeprocess(index);
		uint2 count = SHELL_COUNT;
		MCS_kill(MCprocesses[index].pid, number);
		if (number == SIGTERM || number == SIGKILL)
		{
			while (--count)
			{
				if (MCprocesses[index].pid == 0)
					break;
				if (MCscreen->wait(SHELL_INTERVAL, False, False))
				{
					MCeerror->add
					(EE_SHELL_ABORT, 0, 0);
					return ES_ERROR;
				}
			}
			if (!count)
			{
				MCresult->sets("process didn't die");
				MCprocesses[index].pid = 0;
			}
			IO_cleanprocesses();
		}
	}
	else
	{
		uint4 pid;
		if (MCU_stoui4(name, pid) && pid != 0 && pid != MCS_getpid())
			MCS_kill(pid, number);
	}
	delete name;
	return ES_NORMAL;
#endif /* MCKill */
}

MCOpen::~MCOpen()
{
	delete fname;
	delete message;
	delete go;
	delete destination;
	delete certificate;
}

Parse_stat MCOpen::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.skip_token(SP_ACCEPT, TT_UNDEFINED, AC_SECURE) == PS_NORMAL)
		secure = True;
	else if (sp.skip_token(SP_ACCEPT, TT_UNDEFINED, AC_DATAGRAM) == PS_NORMAL)
		datagram = True;
	else if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_ELEVATED) == PS_NORMAL)
		elevated = True;
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_OPEN_NOTYPE, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_OPEN, te) != PS_NORMAL)
	{
		sp.backup();
		MCerrorlock++;
		go = new MCGo;
		if (go->parse(sp) != PS_NORMAL)
		{
			MCerrorlock--;
			MCperror->add
			(PE_OPEN_BADTYPE, sp);
			return PS_ERROR;
		}
		MCerrorlock--;
		return PS_NORMAL;
	}
	arg = (Open_argument)te->which;
	if (arg == OA_PRINTING)
	{
		if (sp.skip_token(SP_FACTOR, TT_TO, PT_TO) == PS_NORMAL)
		{
			// open printing to <dst> <expr> [ with options <expr> ]
			if (sp . next(type) != PS_NORMAL)
			{
				MCperror -> add(PE_OPENPRINTING_NODST, sp);
				return PS_ERROR;
			}

			destination = sp . gettoken() . clone();

			if (sp . parseexp(False, True, &fname) != PS_NORMAL)
			{
				MCperror -> add(PE_OPENPRINTING_NOFILENAME, sp);
				return PS_ERROR;
			}

			if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
			{
				if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_OPTIONS) != PS_NORMAL ||
					sp . parseexp(False, True, &options) != PS_NORMAL)
				{
					MCperror -> add(PE_OPENPRINTING_BADOPTIONS, sp);
					return PS_ERROR;
				}
			}

			return PS_NORMAL;

		}
		if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL
		        && sp.skip_token(SP_LOCK, TT_UNDEFINED, LC_ERRORS) == PS_NORMAL)
		{
			dialog = True;
			if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AS) == PS_NORMAL
			        && sp.skip_token(SP_ASK, TT_UNDEFINED, AT_SHEET) == PS_NORMAL)
				sheet = True;
		}
		return PS_NORMAL;
	}
	sp.skip_token(SP_FACTOR, TT_TO, PT_TO);
	if (sp.parseexp(False, True, &fname) != PS_NORMAL)
	{
		MCperror->add
		(PE_OPEN_BADNAME, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_FOR) == PS_NORMAL)
	{
		if (sp.next(type) != PS_NORMAL)
		{
			MCperror->add
			(PE_OPEN_NOMODE, sp);
			return PS_ERROR;
		}
		if (sp.lookup(SP_MODE, te) != PS_NORMAL)
		{
			MCperror->add
			(PE_OPEN_BADMODE, sp);
			return PS_ERROR;
		}
		if (te->which == OM_BINARY || te->which == OM_TEXT)
		{
			textmode = te->which == OM_TEXT;
			if (sp.next(type) != PS_NORMAL)
			{
				MCperror->add
				(PE_OPEN_NOMODE, sp);
				return PS_ERROR;
			}
			if (sp.lookup(SP_MODE, te) != PS_NORMAL)
			{
				MCperror->add
				(PE_OPEN_BADMODE, sp);
				return PS_ERROR;
			}
		}
		mode = (Open_mode)te->which;
	}
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		sp.skip_token(SP_SUGAR, TT_CHUNK, CT_UNDEFINED); // message
		if (sp.parseexp(False, True, &message) != PS_NORMAL)
		{
			MCperror->add
			(PE_OPEN_BADMESSAGE, sp);
			return PS_ERROR;
		}
	}
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
		if (sp.skip_token(SP_SSL, TT_UNDEFINED, SSL_VERIFICATION) != PS_NORMAL)
		{
			MCperror->add
			(PE_OPEN_BADMESSAGE, sp);
		}

	if (sp.skip_token(SP_SUGAR, TT_PREP, PT_WITHOUT) == PS_NORMAL)
		if (sp.skip_token(SP_SSL, TT_UNDEFINED, SSL_VERIFICATION) == PS_NORMAL)
			secureverify = False;
		else
			MCperror->add
			(PE_OPEN_BADMESSAGE, sp);
	return PS_NORMAL;
}

Exec_stat MCOpen::exec(MCExecPoint &ep)
{
#ifdef /* MCOpen */ LEGACY_EXEC
	if (go != NULL)
		return go->exec(ep);

	MCresult->clear(False);
	if (arg == OA_PRINTING)
	{
		if (MCsecuremode & MC_SECUREMODE_PRINT)
		{
			MCeerror->add(EE_PRINT_NOPERM, line, pos);
			return ES_ERROR;
		}

		// Handle custom printer formt
		if (destination != NULL)
		{
			Exec_stat t_stat;
			t_stat = ES_NORMAL;

			if (t_stat == ES_NORMAL &&
				fname -> eval(ep) != ES_NORMAL)
			{
				MCeerror -> add(EE_OPEN_BADNAME, line, pos);
				t_stat = ES_ERROR;
			}

			char *t_filename;
			t_filename = nil;
			if (t_stat == ES_NORMAL)
				t_filename = ep . getsvalue() . clone();

			if (t_stat == ES_NORMAL &&
				options != nil && options -> eval(ep) != ES_NORMAL)
			{
				MCeerror -> add(EE_OPEN_BADOPTIONS, line, pos);
				t_stat = ES_ERROR;
			}

			extern Exec_stat MCCustomPrinterCreate(const char *, const char *, MCVariableValue *, MCPrinter*&);
			if (t_stat == ES_NORMAL)
				t_stat = MCCustomPrinterCreate(destination, t_filename, ep . getformat() == VF_ARRAY ? ep . getarray() : nil, MCprinter);
			
			if (t_stat == ES_NORMAL)
				MCprinter -> Open(false);
			
			delete t_filename;

			return t_stat;
		}

		// If 'with dialog' was specified do an 'answer printer' here. Note that
		// we exit if 'cancel' is returned for backwards compatibility. This form
		// the open printing command is, however, deprecated.
		if (dialog)
		{
			const char *t_result;
			t_result = MCprinter -> ChoosePrinter(sheet == True);
			if (t_result != NULL)
			{
				MCresult -> sets(t_result);
				return ES_NORMAL;
			}
		}
		
		MCprinter -> Open(false);

		return ES_NORMAL;
	}
	if (fname->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_OPEN_BADNAME, line, pos);
		return ES_ERROR;
	}
	char *name = ep.getsvalue().clone();
	uint2 index;
	IO_handle istream = NULL;
	IO_handle ostream = NULL;
	switch (arg)
	{
	case OA_DRIVER:
	case OA_FILE:
		if (MCsecuremode & MC_SECUREMODE_DISK)
		{
			delete name;
			MCeerror->add(EE_DISK_NOPERM, line, pos);
			return ES_ERROR;
		}
		IO_closefile(name);
		switch (mode)
		{
		case OM_APPEND:
			ostream = MCS_open(name, IO_APPEND_MODE, False, arg == OA_DRIVER, 0);
			break;
		case OM_NEITHER:
			break;
		case OM_READ:
			istream = MCS_open(name, IO_READ_MODE, True, arg == OA_DRIVER, 0);
			break;
		case OM_WRITE:
			ostream = MCS_open(name, IO_WRITE_MODE, False, arg == OA_DRIVER, 0);
			break;
		case OM_UPDATE:
			istream = ostream = MCS_open(name, IO_UPDATE_MODE, False,
			                             arg == OA_DRIVER, 0);
			break;
		default:
			break;
		}
		if (istream == NULL && ostream == NULL)
		{
			MCresult->sets("can't open that file");
			delete name;
			return ES_NORMAL;
		}
		MCU_realloc((char **)&MCfiles, MCnfiles, MCnfiles + 1, sizeof(Streamnode));
		MCfiles[MCnfiles].name = name;
		MCfiles[MCnfiles].mode = mode;
		MCfiles[MCnfiles].textmode = textmode;
		MCfiles[MCnfiles].ihandle = istream;
		MCfiles[MCnfiles++].ohandle = ostream;
		break;
	case OA_PROCESS:
		if (MCsecuremode & MC_SECUREMODE_PROCESS)
		{
			delete name;
			MCeerror->add(EE_PROCESS_NOPERM, line, pos);
			return ES_ERROR;
		}
		if (IO_findprocess(name, index))
		{
			MCresult->sets("process is already open");
			delete name;
			return ES_NORMAL;
		}
		MCS_startprocess(name, NULL, mode, elevated);
		if (IO_findprocess(name, index))
			MCprocesses[index].textmode = textmode;
		break;
	case OA_SOCKET:
		if ((MCsecuremode & MC_SECUREMODE_NETWORK) && !MCModeCanAccessDomain(name))
		{
			delete name;
			MCeerror->add(EE_NETWORK_NOPERM, line, pos);
			return ES_ERROR;
		}
		if (IO_findsocket(name, index))
		{
			MCresult->sets("socket is already open");
			delete name;
			return ES_NORMAL;
		}
		else
		{
			MCAutoNameRef t_message_name;
			if (message != NULL)
			{
				if (message->eval(ep) != ES_NORMAL)
				{
					MCeerror->add(EE_OPEN_BADMESSAGE, line, pos);
					return ES_ERROR;
				}
				/* UNCHECKED */ ep . copyasnameref(t_message_name);
			}
			// MW-2012-10-26: [[ Bug 10062 ]] Make sure we clear the result.
			MCresult -> clear(True);
			MCSocket *s = MCS_open_socket(name, datagram, ep.getobj(), t_message_name, secure, secureverify, NULL);
			if (s != NULL)
			{
				MCU_realloc((char **)&MCsockets, MCnsockets, MCnsockets + 1, sizeof(MCSocket *));
				MCsockets[MCnsockets++] = s;
			}
			else
				delete name;
		}
		break;
	default:
		break;
	}
	return ES_NORMAL;
#endif /* MCOpen */
}

MCRead::~MCRead()
{
	delete fname;
	delete maxwait;
	delete stop;
	delete it;
	delete at;
}

IO_stat MCRead::readfor(IO_handle stream, int4 pindex, File_unit unit,
                        uint4 count, MCExecPoint &ep, real8 duration)
{
	uint4 size;
	uint4 bsize;

	switch (unit)
	{
	case FU_INT1:
	case FU_UINT1:
		size = count;
		bsize = count * I1L;
		break;
	case FU_INT2:
	case FU_UINT2:
		size = count * 2;
		bsize = count * I2L;
		break;
	case FU_INT4:
	case FU_UINT4:
	case FU_REAL4:
		size = count * 4;
		bsize = count * R4L;
		break;
	case FU_INT8:
	case FU_UINT8:
		size = count * 4;
		bsize = count * R4L;
		break;
	case FU_REAL8:
		size = count * 8;
		bsize = count * R8L;
		break;
	default:
		size = bsize = count;
		break;
	}
	char *tptr = ep.getbuffer(bsize + 1);
	ep.clear();
	char *dptr;
	if (bsize == size)
		dptr = tptr;
	else
		dptr = new char[size];
	uint4 tsize = 0;
	IO_stat stat;
	do
	{
		uint4 rsize = size - tsize;
		uint4 fullsize = rsize;
		stat = MCS_read(&dptr[tsize], sizeof(char), rsize, stream);
		tsize += rsize;
		if (rsize < fullsize)
		{
			// MW-2010-10-25: [[ Bug 4022 ]] If we are reading from a process and we didn't
			//   get as much data as we wanted then do a sweep.
			if (pindex != -1)
				MCS_checkprocesses();

			if (((stat == IO_ERROR || stat == IO_EOF)
			        && (pindex == -1 || MCprocesses[pindex].pid == 0)))
			{
				stat = IO_EOF;
				break;
			}
			duration -= READ_INTERVAL;
			if (duration < 0.0)
			{
				stat = IO_TIMEOUT;
				break;
			}
			else
			{
				MCU_play();
				// MH-2007-05-18 [[Bug 4021]]: read from process times out too soon.
				// Originally the arguments to wait were READ_INTERVAL, False, True
				if (MCscreen->wait(READ_INTERVAL, False, False))
				{
					if (bsize != size)
						delete dptr;
					MCeerror->add(EE_READ_ABORT, line, pos);
					return IO_ERROR;
				}
			}
		}
	}
	while (tsize < size);

	switch (unit)
	{
	case FU_INT1:
	{
			int1 *i1ptr = (int1 *)dptr;
			for (uint4 i = 0 ; i < count ; i++)
				ep.concatint(i1ptr[i], EC_COMMA, i == 0);
		}
		break;
	case FU_INT2:
		{
			int2 *i2ptr = (int2 *)dptr;
			for (uint4 i = 0 ; i < count ; i++)
				ep.concatint(i2ptr[i], EC_COMMA, i == 0);
		}
		break;
	case FU_INT4:
		{
			int4 *i4ptr = (int4 *)dptr;
			for (uint4 i = 0 ; i < count ; i++)
				ep.concatint(i4ptr[i], EC_COMMA, i == 0);
		}
		break;
	case FU_INT8:
		{
			int4 *i8ptr = (int4 *)dptr;
			for (uint4 i = 1 ; i < count ; i += 2)
				ep.concatint(i8ptr[i], EC_COMMA, i == 0);
		}
		break;
	case FU_REAL4:
		{
			real4 *r4ptr = (real4 *)dptr;
			for (uint4 i = 0 ; i < count ; i++)
				ep.concatreal(r4ptr[i], EC_COMMA, i == 0);
		}
		break;
	case FU_REAL8:
		{
			real8 *r8ptr = (real8 *)dptr;
			for (uint4 i = 0 ; i < count ; i++)
				ep.concatreal(r8ptr[i], EC_COMMA, i == 0);
		}
		break;
	case FU_UINT1:
		{
			uint1 *u1ptr = (uint1 *)dptr;
			for (uint4 i = 0 ; i < count ; i++)
				ep.concatuint(u1ptr[i], EC_COMMA, i == 0);
		}
		break;
	case FU_UINT2:
		{
			uint2 *u2ptr = (uint2 *)dptr;
			for (uint4 i = 0 ; i < count ; i++)
				ep.concatuint(u2ptr[i], EC_COMMA, i == 0);
		}
		break;
	case FU_UINT4:
		{
			uint4 *u4ptr = (uint4 *)dptr;
			for (uint4 i = 0 ; i < count ; i++)
				ep.concatuint(u4ptr[i], EC_COMMA, i == 0);
		}
		break;
	case FU_UINT8:
		{
			uint4 *u8ptr = (uint4 *)dptr;
			for (uint4 i = 1 ; i < count ; i += 2)
				ep.concatuint(u8ptr[i], EC_COMMA, i == 0);
		}
		break;
	default:
		ep.setlength(tsize);
		break;
	}
	if (bsize != size)
		delete dptr;
	return stat;
}

IO_stat MCRead::readuntil(IO_handle stream, int4 pindex, uint4 count,
                          const char *sptr, MCExecPoint &ep,
                          Boolean words, real8 duration)
{
	char *dptr = ep.getbuffer(BUFSIZ);
	uint4 tsize = ep.getbuffersize();
	uint4 fullsize;
	if (sptr[0] == '\004')
		fullsize = BUFSIZ;
	else
		fullsize = 1;
	uint4 endcount = strlen(sptr) - 1;

	IO_stat stat;
	uint4 size = 0;
	Boolean doingspace = True;
	while (True)
	{
		uint4 rsize = fullsize;
		if (size + rsize > tsize)
		{
			MCU_realloc((char **)&dptr, tsize, tsize + BUFSIZ, sizeof(char));
			tsize += BUFSIZ;
		}
		stat = MCS_read(&dptr[size], sizeof(char), rsize, stream);
		size += rsize;
		if (rsize < fullsize)
		{
			// MW-2010-10-25: [[ Bug 4022 ]] If we are reading from a process and we didn't
			//   get as much data as we wanted then do a sweep.
			if (pindex != -1)
				MCS_checkprocesses();

			if (sptr[0] == '\0' || ((stat == IO_ERROR || stat == IO_EOF) && (pindex == -1 || MCprocesses[pindex].pid == 0)))
			{
				stat = IO_EOF;
				break;
			}
			duration -= READ_INTERVAL;
			if (duration <= 0)
			{
				stat = IO_TIMEOUT;
				break;
			}
			else
				if (MCscreen->wait(READ_INTERVAL, False, True))
				{
					MCeerror->add
					(EE_READ_ABORT, line, pos);
					return IO_ERROR;
				}
		}
		else
		{
			if (words)
			{
				if (doingspace)
				{
					if (!isspace((uint1)dptr[size - 1]))
						doingspace = False;
				}
				else
					if (isspace((uint1)dptr[size - 1]))
						if (--count == 0)
							break;
						else
							doingspace = True;
			}
			else
			{
				if (sptr[0] && sptr[0] != '\004')
				{
					uint4 i = endcount;
					uint4 j = size - 1;
					while (i && j && dptr[j] == sptr[i])
					{
						i--;
						j--;
					}
					if (i == 0 && (dptr[j] == sptr[0]
					               || sptr[0] == '\n' && dptr[j] == '\r'))
					{
						// MW-2008-08-15: [[ Bug 6580 ]] This clause looks ahead for CR LF sequences
						//   if we have just encountered CR. However, it was previousy using MCS_seek_cur
						//   to retreat, which *doesn't* work for process streams.
						if (sptr[0] == '\n' && endcount == 0
						        && dptr[j] == '\r')
						{
							uint1 term;
							uint4 nread = 1;
							if (MCS_read(&term, sizeof(char), nread, stream) == IO_NORMAL)
								if (term != '\n')
									MCS_putback(term, stream);
								else
									dptr[j] = '\n';
						}
						if (--count == 0)
							break;
					}
				}
			}
		}
	}
	ep.setbuffer(dptr, tsize);
	ep.setlength(size);
	return stat;
}

IO_stat MCRead::readuntil_binary(IO_handle stream, int4 pindex, uint4 count, const MCString &sptr, MCExecPoint &ep,Boolean words, real8 duration)
{
	char *dptr = ep.getbuffer(BUFSIZ);
	uint4 tsize = ep.getbuffersize();
	uint4 fullsize;
	if (sptr . getlength() == 0)
		fullsize = BUFSIZ;
	else
		fullsize = 1;

	uint4 endcount = sptr . getlength() - 1;
	

	IO_stat stat;
	uint4 size = 0;
	Boolean doingspace = True;
	while (True)
	{
		uint4 rsize = fullsize;
		if (size + rsize > tsize)
		{
			MCU_realloc((char **)&dptr, tsize, tsize + BUFSIZ, sizeof(char));
			tsize += BUFSIZ;
		}
		stat = MCS_read(&dptr[size], sizeof(char), rsize, stream);
		size += rsize;
		if (rsize < fullsize)
		{
			// MW-2010-10-25: [[ Bug 4022 ]] If we are reading from a process and we didn't
			//   get as much data as we wanted then do a sweep.
			if (pindex != -1)
				MCS_checkprocesses();

			if (sptr . getlength() == 0
			        || ((stat == IO_ERROR || stat == IO_EOF)
			            && (pindex == -1 || MCprocesses[pindex].pid == 0)))
			{
				stat = IO_EOF;
				break;
			}
			duration -= READ_INTERVAL;
			if (duration <= 0)
			{
				stat = IO_TIMEOUT;
				break;
			}
			else
				if (MCscreen->wait(READ_INTERVAL, False, True))
				{
					MCeerror->add
					(EE_READ_ABORT, line, pos);
					return IO_ERROR;
				}
		}
		else
		{
			if (words)
			{
				if (doingspace)
				{
					if (!isspace((uint1)dptr[size - 1]))
						doingspace = False;
				}
				else
					if (isspace((uint1)dptr[size - 1]))
						if (--count == 0)
							break;
						else
							doingspace = True;
			}
			else
			{
				if (sptr . getlength() != 0)
				{
					uint4 i = endcount;
					uint4 j = size - 1;
					while (i && j && dptr[j] == sptr . getstring()[i])
					{
						i--;
						j--;
					}
					if (i == 0 && (dptr[j] == sptr . getstring()[0]))
					{
						if (--count == 0)
							break;
					}
				}
			}
		}
	}
	ep.setbuffer(dptr, tsize);
	ep.setlength(size);
	return stat;
}

Parse_stat MCRead::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);

	if (sp.skip_token(SP_FACTOR, TT_FROM) != PS_NORMAL)
	{
		MCperror->add
		(PE_READ_NOFROM, sp);
		return PS_ERROR;
	}
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_READ_NOTYPE, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_OPEN, te) != PS_NORMAL)
	{
		MCperror->add
		(PE_READ_BADTYPE, sp);
		return PS_ERROR;
	}
	arg = (Open_argument)te->which;
	if (te->which != OA_STDIN)
	{
		if (sp.parseexp(False, True, &fname) != PS_NORMAL)
		{
			MCperror->add
			(PE_READ_BADNAME, sp);
			return PS_ERROR;
		}
	}
	if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AT) == PS_NORMAL)
		if (sp.parseexp(False, True, &at) != PS_NORMAL)
		{
			MCperror->add
			(PE_READ_BADAT, sp);
			return PS_ERROR;
		}
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_READ_NOCOND, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_REPEAT, te) != PS_NORMAL)
	{
		MCperror->add
		(PE_READ_NOTCOND, sp);
		return PS_ERROR;
	}
	if (te->which == RF_WITH)
	{
		sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_MESSAGE);
		if (sp.parseexp(False, True, &at) != PS_NORMAL)
		{
			MCperror->add
			(PE_READ_BADMESS, sp);
			return PS_ERROR;
		}
		cond = RF_WITH;
		return PS_NORMAL;
	}
	cond = (Repeat_form)te->which;
	if (sp.skip_token(SP_FACTOR, TT_UNOP, O_NOT) == PS_NORMAL)
		sp.skip_token(SP_SUGAR, TT_CHUNK, CT_UNDEFINED);
	else
		if (sp.parseexp(False, True, &stop) != PS_NORMAL)
		{
			MCperror->add
			(PE_READ_BADCOND, sp);
			return PS_ERROR;
		}
	if (cond == RF_FOR)
		if (sp.next(type) == PS_NORMAL)
		{
			if (sp.lookup(SP_UNIT, te) != PS_NORMAL)
				sp.backup();
			else
				unit = (File_unit)te->which;
		}
	if (gettime(sp, &maxwait, timeunits) != PS_NORMAL)
	{
		MCperror->add
		(PE_WAIT_BADCOND, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL
	        && sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_MESSAGE) == PS_NORMAL)
		if (sp.parseexp(False, True, &at) != PS_NORMAL)
		{
			MCperror->add
			(PE_READ_BADMESS, sp);
			return PS_ERROR;
		}
	getit(sp, it);
	return PS_NORMAL;
}

Exec_stat MCRead::exec(MCExecPoint &ep)
{
#ifdef /* MCRead */ LEGACY_EXEC
	IO_handle stream = NULL;
	uint2 index;
	int4 pindex = -1;
	IO_stat stat = IO_NORMAL;
	real8 duration = MAXUINT4;
	Boolean textmode = False;
	if (maxwait != NULL)
	{
		if (maxwait->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_READ_BADEXP, line, pos);
			return ES_ERROR;
		}
		if (ep.getreal8(duration, line, pos, EE_READ_NAN) != ES_NORMAL)
			return ES_ERROR;
		switch (timeunits)
		{
		case F_MILLISECS:
			duration /= 1000.0;
			break;
		case F_TICKS:
			duration /= 60.0;
			break;
		default:
			break;
		}
	}
	MCresult->clear(False);
	if (arg == OA_STDIN)
#ifndef _SERVER
		if (!MCnoui && MCS_isatty(0))
		{
			MCresult->sets("eof");
			it->clear();
			return ES_NORMAL;
		}
		else
#endif
			stream = IO_stdin;
	else
	{
		if (fname->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_OPEN_BADNAME, line, pos);
			return ES_ERROR;
		}
		MCAutoPointer<char> name;
		name = ep.getsvalue().clone();
		switch (arg)
		{
		case OA_DRIVER:
		case OA_FILE:
			if (!IO_findfile(*name, index) || MCfiles[index].mode == OM_APPEND
			        || MCfiles[index].mode == OM_WRITE)
			{
				MCresult->sets("file is not open for read");
				return ES_NORMAL;
			}
			stream = MCfiles[index].ihandle;
			textmode = MCfiles[index].textmode;
			if (at != NULL)
			{
				if (at->eval(ep) != ES_NORMAL)
				{
					MCeerror->add(EE_READ_BADAT, line, pos);
					return ES_ERROR;
				}
				if (ep.getsvalue().getstring()[0] == '\004' || ep.getsvalue() == "eof")
					stat = MCS_seek_end(stream, 0);
				else
				{
					double n;
					if (ep.getreal8(n, line, pos, EE_READ_BADAT) != ES_NORMAL)
						stat = IO_ERROR;
					else
						if (n < 0)
							stat = MCS_seek_end(stream, (int64_t)n);
						else
							stat = MCS_seek_set(stream, (int64_t)n - 1);
				}
				if (stat != IO_NORMAL)
				{
					MCresult->sets("error seeking in file");
					return ES_NORMAL;
				}
			}
			else
				if (stream->flags & IO_WRITTEN)
				{
					stream->flags &= ~IO_WRITTEN;
					MCS_sync(stream);
				}
			break;
		case OA_PROCESS:
			if (!IO_findprocess(*name, index) || MCprocesses[index].mode == OM_APPEND
			        || MCprocesses[index].mode == OM_WRITE
			        || MCprocesses[index].mode == OM_NEITHER)
			{
				MCresult->sets("process is not open for read");
				// MH-2007-05-18 [[Bug 4020]]: A read from an unopened process would throw an error, which is inconsistent with general behaviour through "the result".
				return ES_NORMAL;
			}
			stream = MCprocesses[index].ihandle;
			MCshellfd = stream->getfd();
			textmode = MCprocesses[index].textmode;
			pindex = index;
			break;
		case OA_SOCKET:
			if (IO_findsocket(*name, index))
			{
				MCAutoNameRef t_message_name;
				if (MCsockets[index] -> datagram && at == nil)
				{
					MCeerror -> add(EE_READ_NOTVALIDFORDATAGRAM, line, pos);
					return ES_ERROR;
				}
				
				if (at != NULL)
				{
					if (at->eval(ep) != ES_NORMAL)
					{
						MCeerror->add(EE_WRITE_BADEXP, line, pos);
						return ES_ERROR;
					}
					/* UNCHECKED */ ep . copyasnameref(t_message_name);
				}
				if (cond == RF_FOR)
				{
					if (stop->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
					{
						MCeerror->add(EE_READ_NONUMBER, line, pos);
						return ES_ERROR;
					}
					uint4 size = ep.getuint4();
					char *until = NULL;
					switch (unit)
					{
					case FU_ITEM:
						until = strclone(",");
						break;
					case FU_LINE:
						until = strclone("\n");
						break;
					case FU_WORD:
						until = strclone(" ");
						break;
					default:
						break;
					}
					// MW-2012-10-26: [[ Bug 10062 ]] Make sure we clear the result.
					MCresult->clear(False);
					MCS_read_socket(MCsockets[index], ep, size, until, t_message_name);
				}
				else
				{
					char *sptr = NULL;
					if (stop != NULL)
					{
						if (stop->eval(ep) != ES_NORMAL)
						{
							MCeerror->add(EE_READ_NOCHARACTER, line, pos);
							return ES_ERROR;
						}
						sptr = ep.getsvalue().clone();
					}
					// MW-2012-10-26: [[ Bug 10062 ]] Make sure we clear the result.
					MCresult->clear(False);
					MCS_read_socket(MCsockets[index], ep, 0, sptr, t_message_name);
				}
				if (t_message_name == NULL)
					it->set(ep);
			}
			else
				MCresult->sets("socket is not open");
			return ES_NORMAL;
		default:
			break;
		}
	}
	switch (cond)
	{
	case RF_FOR:
		{
			if (stop->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
			{
				MCeerror->add
				(EE_READ_NONUMBER, line, pos);
				MCshellfd = -1;
				return ES_ERROR;
			}
			uint4 size = ep.getuint4();
			switch (unit)
			{
			case FU_LINE:
				stat = readuntil(stream, pindex, size, "\n", ep, False, duration);
				break;
			case FU_ITEM:
				stat = readuntil(stream, pindex, size, ",", ep, False, duration);
				break;
			case FU_WORD:
				stat = readuntil(stream, pindex, size, " ", ep, True, duration);
				break;
			default:
				stat = readfor(stream, pindex, unit, size, ep, duration);
				break;
			}
		}
		break;
	case RF_UNTIL:
		{
			if (stop->eval(ep) != ES_NORMAL)
			{
				MCeerror->add
				(EE_READ_NOCHARACTER, line, pos);
				MCshellfd = -1;
				return ES_ERROR;
			}

			char *sptr = ep.getsvalue().clone();
			

			if ((arg == OA_DRIVER || arg == OA_FILE) && *sptr == '\004')
			{
				IO_read_to_eof(stream, ep);
				stat = IO_EOF;
			}
			else
			{
				// MW-2009-11-03: [[ Bug 8402 ]] Use a different stream array, depending on what
				//   type of stream we are reading from.
				bool t_is_text;
				if (arg == OA_FILE)
					t_is_text = MCfiles[index] . textmode != 0;
				else if (arg == OA_PROCESS)
					t_is_text = MCprocesses[index] . textmode != 0;
				else
					t_is_text = true;
				if (!t_is_text)
				{
					MCString t_string(sptr, ep.getsvalue() . getlength());
					stat = readuntil_binary(stream, pindex, 1, t_string, ep, False, duration);
				}
				else
					stat = readuntil(stream, pindex, 1, sptr, ep, False, duration);
			}
			delete sptr;
		}
		break;
	default:
		MCeerror->add
		(EE_READ_BADCOND, line, pos);
		MCshellfd = -1;
		return ES_ERROR;
	}
	MCshellfd = -1;
	switch (stat)
	{
	case IO_ERROR:
		MCresult->sets("error reading file");
		break;
	case IO_EOF:
		MCresult->sets("eof");
		break;
	case IO_TIMEOUT:
		MCresult->sets("timed out");
		break;
	default:
		MCresult->clear(False);
		break;
	}
	if (textmode)
		ep.texttobinary();
	it->set(ep);

#if !defined _WIN32 && !defined _MACOSX
	if (arg == OA_FILE || arg == OA_DRIVER)
		MCS_sync(stream);
#endif

	return ES_NORMAL;
#endif /* MCRead */
}

MCSeek::~MCSeek()
{
	delete fname;
	delete where;
}

Parse_stat MCSeek::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);

	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_SEEK_NOMODE, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_FACTOR, te) != PS_NORMAL
	        || (te->type != TT_TO && te->type != TT_FROM))
	{
		MCperror->add
		(PE_SEEK_BADMODE, sp);
		return PS_ERROR;
	}
	mode = (Preposition_type)te->which;
	if (sp.parseexp(False, True, &where) != PS_NORMAL)
	{
		MCperror->add
		(PE_SEEK_BADWHERE, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_IN) != PS_NORMAL)
	{
		MCperror->add
		(PE_SEEK_NOIN, sp);
		return PS_ERROR;
	}
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_SEEK_NOTYPE, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_OPEN, te) != PS_NORMAL || te->which != OA_FILE)
	{
		MCperror->add
		(PE_SEEK_BADTYPE, sp);
		return PS_ERROR;
	}
	if (sp.parseexp(False, True, &fname) != PS_NORMAL)
	{
		MCperror->add
		(PE_SEEK_BADNAME, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCSeek::exec(MCExecPoint &ep)
{
#ifdef /* MCSeek */ LEGACY_EXEC
	if (fname->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_SEEK_BADNAME, line, pos);
		return ES_ERROR;
	}
	const char *name = ep.getcstring();
	uint2 index;
	if (!IO_findfile(name, index)
	        || MCfiles[index].ihandle == NULL && MCfiles[index].ohandle == NULL)
	{
		MCeerror->add(EE_SEEK_NOFILE, line, pos, name);
		return ES_ERROR;
	}
	IO_handle stream = MCfiles[index].ihandle;
	if (stream == NULL)
		stream = MCfiles[index].ohandle;
	IO_stat stat = IO_ERROR;
	if (where->eval(ep) == ES_NORMAL)
	{
		if (ep.getsvalue().getstring()[0] == '\004' || ep.getsvalue() == "eof")
			stat = MCS_seek_end(stream, 0);
		else
		{
			double n;
			if (ep.getreal8(n, line, pos, EE_SEEK_BADWHERE) != ES_NORMAL)
				stat = IO_ERROR;
			else
				if (mode == PT_TO)
					if (n < 0)
						stat = MCS_seek_end(stream, (int64_t)n);
					else
						stat = MCS_seek_set(stream, (int64_t)n);
				else
					stat = MCS_seek_cur(stream, (int64_t)n);
		}
		stream->flags |= IO_SEEKED;
	}
	if (stat != IO_NORMAL)
	{
		MCeerror->add
		(EE_SEEK_BADWHERE, line, pos, ep.getsvalue());
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCSeek */
}

MCWrite::~MCWrite()
{
	delete source;
	delete fname;
	delete at;
}

Parse_stat MCWrite::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add
		(PE_WRITE_BADEXP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_TO) != PS_NORMAL)
	{
		MCperror->add
		(PE_WRITE_NOTO, sp);
		return PS_ERROR;
	}
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_WRITE_NOTYPE, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_OPEN, te) != PS_NORMAL)
	{
		MCperror->add
		(PE_WRITE_BADTYPE, sp);
		return PS_ERROR;
	}
	arg = (Open_argument)te->which;
	if (te->which != OA_STDERR && te->which != OA_STDOUT)
	{
		if (sp.parseexp(False, True, &fname) != PS_NORMAL)
		{
			MCperror->add
			(PE_WRITE_BADNAME, sp);
			return PS_ERROR;
		}
	}
	if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AT) == PS_NORMAL
	        || (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL
	            && sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_MESSAGE) == PS_NORMAL))
	{
		if (sp.parseexp(False, True, &at) != PS_NORMAL)
		{
			MCperror->add
			(PE_WRITE_BADAT, sp);
			return PS_ERROR;
		}
	}
	if (sp.skip_token(SP_FACTOR, TT_PREP) != PS_NORMAL)
		return PS_NORMAL;
	if (sp.next(type) != PS_NORMAL)
		return PS_NORMAL;
	if (sp.lookup(SP_UNIT, te) != PS_NORMAL)
	{
		sp.backup();
		return PS_NORMAL;
	}
	unit = (File_unit)te->which;
	return PS_NORMAL;
}

Exec_stat MCWrite::exec(MCExecPoint &ep)
{
#ifdef /* MCWrite */ LEGACY_EXEC
	uint2 index;
	IO_handle stream = NULL;
	IO_stat stat = IO_NORMAL;
	Boolean textmode = False;

	if (arg == OA_STDERR)
		stream = IO_stderr;
	else
		if (arg == OA_STDOUT)
			stream = IO_stdout;
		else
		{
			if (fname->eval(ep) != ES_NORMAL)
			{
				MCeerror->add
				(EE_WRITE_BADEXP, line, pos);
				return ES_ERROR;
			}
			char *name = ep.getsvalue().clone();
			switch (arg)
			{
			case OA_DRIVER:
			case OA_FILE:
				if (!IO_findfile(name, index)
				        || MCfiles[index].mode == OM_NEITHER
				        || MCfiles[index].mode == OM_READ)
				{
					MCresult->sets("file is not open for write");
					delete name;
					return ES_NORMAL;
				}
				stream = MCfiles[index].ohandle;
				textmode = MCfiles[index].textmode;
				if (at != NULL)
				{
					if (at->eval(ep) != ES_NORMAL)
					{
						MCeerror->add
						(EE_WRITE_BADEXP, line, pos);
						return ES_ERROR;
					}
					if (ep.getsvalue().getstring()[0] == '\004'
					        || ep.getsvalue() == "eof")
						stat = MCS_seek_end(stream, 0);
					else
					{
						double n;
						if (ep.getreal8(n, line, pos, EE_WRITE_BADEXP) != ES_NORMAL)
							stat = IO_ERROR;
						else
							if (n < 0)
								stat = MCS_seek_end(stream, (int64_t)n);
							else
								stat = MCS_seek_set(stream, (int64_t)n);
					}
					if (stat != IO_NORMAL)
					{
						MCresult->sets("error seeking in file");
						return ES_NORMAL;
					}
				}
				break;
			case OA_PROCESS:
				if (!IO_findprocess(name, index)
				        || MCprocesses[index].mode == OM_NEITHER
				        || MCprocesses[index].mode == OM_READ)
				{
					MCresult->sets("process is not open for write");
					delete name;
					return ES_NORMAL;
				}
				stream = MCprocesses[index].ohandle;
				textmode = MCprocesses[index].textmode;
				break;
			case OA_SOCKET:
				if (IO_findsocket(name, index))
				{
					MCAutoNameRef t_message_name;
					if (at != NULL)
					{
						if (at->eval(ep) != ES_NORMAL)
						{
							delete name;
							MCeerror->add(EE_WRITE_BADEXP, line, pos);
							return ES_ERROR;
						}
						/* UNCHECKED */ ep . copyasnameref(t_message_name);
					}
					if (source->eval(ep) != ES_NORMAL)
					{
						delete name;
						MCeerror->add(EE_WRITE_BADEXP, line, pos);
						return ES_ERROR;
					}
					MCresult->clear(False);
					MCS_write_socket(ep.getsvalue(), MCsockets[index], ep.getobj(), t_message_name);
				}
				else
					MCresult->sets("socket is not open");
				delete name;
				return ES_NORMAL;
			default:
				break;
			}
			delete name;
		}
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_WRITE_BADEXP, line, pos);
		return ES_ERROR;
	}
	MCresult->clear(False);
	if (textmode)
		ep.binarytotext();

	uint4 offset;
	Boolean haseof = False;
	// MW-2004-11-17: EOD should only happen when writing to processes in text-mode
	if (arg == OA_PROCESS && textmode && MCU_offset("\004", ep.getsvalue(), offset, True))
	{
		ep.substring(0, offset);
		haseof = True;
	}

	switch (unit)
	{
	case FU_CHARACTER:
	case FU_ITEM:
	case FU_LINE:
	case FU_WORD:
		stat = MCS_write(ep.getsvalue().getstring(), sizeof(char),
		                 ep.getsvalue().getlength(), stream);
		break;
	default:
		{
			uint4 l = ep.getsvalue().getlength();
			const char *sptr = ep.getsvalue().getstring();
			while (l)
			{
				const char *startptr = sptr;
				if (!MCU_strchr(sptr, l, ','))
				{
					sptr += l;
					l = 0;
				}
				MCString s(startptr, sptr - startptr);
				real8 n;
				if (!MCU_stor8(s, n))
				{
					MCeerror->add
					(EE_FUNCTION_NAN, 0, 0, s);
					stat = IO_ERROR;
					break;
				}
				sptr++;
				switch (unit)
				{
				case FU_INT1:
					stat = IO_write_int1((int1)n, stream);
					break;
				case FU_INT2:
					stat = IO_write_int2((int2)n, stream);
					break;
				case FU_INT4:
					stat = IO_write_int4((int4)n, stream);
					break;
				case FU_INT8:
					if (n < 0)
						stat = IO_write_int4(-1, stream);
					else
						stat = IO_write_int4(0, stream);
					if (stat == IO_NORMAL)
						stat = IO_write_int4((int4)n, stream);
					break;
				case FU_REAL4:
					stat = IO_write_real4((real4)n, stream);
					break;
				case FU_REAL8:
					stat = IO_write_real8(n, stream);
					break;
				case FU_UINT1:
					stat = IO_write_uint1((uint1)n, stream);
					break;
				case FU_UINT2:
					stat = IO_write_uint2((uint2)n, stream);
					break;
				case FU_UINT4:
					stat = IO_write_uint4((uint4)n, stream);
					break;
				case FU_UINT8:
					stat = IO_write_uint4(0, stream);
					if (stat == IO_NORMAL)
						stat = IO_write_uint4((uint4)n, stream);
					break;
				default:
					break;
				}
			}
			if (stat != IO_NORMAL)
				break;
		}
		break;
	}

	if (haseof)
	{
		MCS_close(MCprocesses[index].ohandle);
		MCprocesses[index].ohandle = NULL;
		if (MCprocesses[index].mode == OM_UPDATE)
			MCprocesses[index].mode = OM_READ;
		else
			MCprocesses[index].mode = OM_NEITHER;
	}
	if (stat != IO_NORMAL)
	{
		MCresult->sets("error writing file");
		return ES_NORMAL;
	}
	if (arg == OA_DRIVER || arg == OA_FILE)
		stream->flags |= IO_WRITTEN;
#if !defined _WIN32 && !defined _MACOSX
	if (!haseof)
		MCS_flush(stream);
#endif

	MCresult->clear(False);
	return ES_NORMAL;
#endif /* MCWrite */
}
