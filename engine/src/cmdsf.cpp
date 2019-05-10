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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcio.h"

#include "exec.h"

#include "dispatch.h"
#include "handler.h"
#include "scriptpt.h"

#include "cmds.h"
#include "chunk.h"
#include "mcerror.h"
#include "object.h"
#include "mccontrol.h"
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
#include "variable.h"

#include "exec-interface.h"

#include "resolution.h"
#include "regex.h"

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
			stack = new (nothrow) MCChunk(False);
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

void MCClose::exec_ctxt(MCExecContext &ctxt)
{
	if (arg == OA_OBJECT)
    {
		if (stack == NULL)
			MCInterfaceExecCloseDefaultStack(ctxt);
		else
		{
			MCObject *optr;
            uint4 parid;

            if (!stack->getobj(ctxt, optr, parid, True)
                    || optr->gettype() != CT_STACK)
			{
                ctxt . LegacyThrow(EE_CLOSE_NOOBJ);
                return;
			}
			MCInterfaceExecCloseStack(ctxt, (MCStack *)optr);
		}
	}
	else if (arg == OA_PRINTING)
		MCPrintingExecClosePrinting(ctxt);
	else
    {
        MCNewAutoNameRef t_name;
        if (!ctxt . EvalExprAsNameRef(fname, EE_CLOSE_BADNAME, &t_name))
            return;

		switch (arg)	
		{	
		case OA_DRIVER:
			MCFilesExecCloseDriver(ctxt, *t_name);
			break;	
		case OA_FILE:
			MCFilesExecCloseFile(ctxt, *t_name);
			break;	
		case OA_PROCESS:	
			MCFilesExecCloseProcess(ctxt, *t_name);
			break;
		case OA_SOCKET:
			MCNetworkExecCloseSocket(ctxt, *t_name);
			break;
		default:
			break;
		}
    }
}

MCEncryptionOp::~MCEncryptionOp()
{
	delete ciphername;
	delete source;
	delete keystr;
	delete keylen;
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
	return PS_NORMAL;

}

void MCEncryptionOp::exec_ctxt(MCExecContext &ctxt)
{
    ctxt . SetTheResultToEmpty();

	if (is_rsa)
	{
        MCAutoStringRef t_key;
        if (!ctxt . EvalExprAsStringRef(rsa_key, EE_OPEN_BADNAME, &t_key))
            return;

		MCAutoStringRef t_passphrase;
        if (!ctxt . EvalOptionalExprAsStringRef(rsa_passphrase, kMCEmptyString, EE_OPEN_BADNAME, &t_passphrase))
            return;

        MCAutoStringRef t_data;
        if (!ctxt . EvalExprAsStringRef(source, EE_OPEN_BADNAME, &t_data))
            return;

		if (isdecrypt)
			MCSecurityExecRsaDecrypt(ctxt, *t_data, rsa_keytype != RSAKEY_PRIVKEY, *t_key, *t_passphrase);
		else
			MCSecurityExecRsaEncrypt(ctxt, *t_data, rsa_keytype != RSAKEY_PRIVKEY, *t_key, *t_passphrase);
	}
	else
	{
        MCNewAutoNameRef t_cipher;
        if (!ctxt . EvalExprAsNameRef(ciphername, EE_OPEN_BADNAME, &t_cipher))
            return;

        MCAutoStringRef t_key;
        if (!ctxt . EvalExprAsStringRef(keystr, EE_OPEN_BADNAME, &t_key))
            return;
		
        uinteger_t keybits;
        if (!ctxt . EvalOptionalExprAsUInt(keylen, 0, EE_OPEN_BADNAME, keybits)
                || keybits > 65535) // Ensure that keybits is an uint2
            return;

		MCAutoStringRef t_iv;
		MCAutoStringRef t_salt;
        if (salt != nil && !ctxt . EvalExprAsStringRef(salt, EE_OPEN_BADNAME, &t_salt))
            return;

        if (iv != nil && !ctxt . EvalExprAsStringRef(iv, EE_OPEN_BADNAME, &t_iv))
            return;

        MCAutoStringRef t_data;
        if (!ctxt . EvalExprAsStringRef(source, EE_READ_BADAT, &t_data))
            return;

		if (ispassword)
		{
			if (isdecrypt)
				MCSecurityExecBlockDecryptWithPassword(ctxt, *t_data, *t_cipher, *t_key, *t_salt, *t_iv, keybits);
			else
				MCSecurityExecBlockEncryptWithPassword(ctxt, *t_data, *t_cipher, *t_key, *t_salt, *t_iv, keybits);
		}
		else
		{
			if (isdecrypt)
				MCSecurityExecBlockDecryptWithKey(ctxt, *t_data, *t_cipher, *t_key, *t_iv, keybits);
			else
				MCSecurityExecBlockEncryptWithKey(ctxt, *t_data, *t_cipher, *t_key, *t_iv, keybits);
		}
    }
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
    // MERG-2014-07-11: metadata array
    delete metadata;
}

Parse_stat MCExport::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te = NULL;

	initpoint(sp);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_EXPORT_NOTYPE, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_EXPORT, te) == PS_NORMAL)
    {
		sformat = (Export_format)te->which;
        // MERG-2014-07-17: Bugfix because export JPEG etc was failing to set the format
        format = sformat;
    }
	else
	{
		sp.backup();
		image = new (nothrow) MCChunk(False);
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
				Symbol_type t_type;
				const LT *t_te = NULL;

				// MW-2006-05-04: Bug 3506 - crash in specific case due to not checking result of sp.lookup
				// MW-2007-09-11: [[ Bug 5242 ]] - the alternate of this if used to fail if te == NULL, this
				//   can happen though if we are looking at a variable chunk.
				if (sp.next(t_type) == PS_NORMAL && sp.lookup(SP_FACTOR, t_te) == PS_NORMAL && t_te -> type == TT_CHUNK && Chunk_term(t_te -> which) == CT_STACK)
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
				else if (t_te == NULL || t_te -> type != TT_TO)
				{
					sp . backup();
					image = new (nothrow) MCChunk(False);
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
                        // MERG-2014-07-11: [[ ImageMetadata ]] Allow metadata without having to specify effects
                        if (with_effects && sp . skip_token(SP_FACTOR, TT_PROPERTY, P_METADATA) == PS_NORMAL)
                        {
                            sp . backup();
                            sp . backup();
                        }
                        else
                        {
                            MCperror -> add(PE_IMPORT_BADFILENAME, sp);
                            return PS_ERROR;
                        }
					}
				}
			}
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
    
    // MERG-2014-07-11: [[ ImageMetadata ]] metadata array
    bool t_is_image;
    t_is_image = false;
    if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL || sp . skip_token(SP_FACTOR, TT_BINOP, O_AND) == PS_NORMAL )
    {
        if (sp . skip_token(SP_FACTOR, TT_PROPERTY, P_METADATA) != PS_NORMAL ||
            sp . parseexp(False, True, &metadata) != PS_NORMAL)
        {
            MCperror -> add(PE_IMPORT_BADFILENAME, sp);
            return PS_ERROR;
        }
        t_is_image = true;
    }
    
	if (sp.skip_token(SP_FACTOR, TT_TO) != PS_NORMAL)
	{
		MCperror->add(PE_EXPORT_NOTO, sp);
		return PS_ERROR;
	}

    if (!t_is_image &&
        sp.skip_token(SP_VALIDATION, TT_UNDEFINED, IV_ARRAY) == PS_NORMAL)
    {
        dest = new (nothrow) MCChunk(True);
        if (dest->parse(sp, False) != PS_NORMAL)
        {
            MCperror->add(PE_EXPORT_NOARRAY, sp);
            return PS_ERROR;
        }
        sformat = EX_OBJECT;
    }
    else
    {
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
            dest = new (nothrow) MCChunk(True);
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
    }
    
	return PS_NORMAL;
}

void MCExport::exec_ctxt(MCExecContext &ctxt)
{
    if (sformat == EX_OBJECT)
    {
        if (image == nil)
        {
            ctxt . LegacyThrow(EE_EXPORT_NOTANIMAGE);
            return;
        }
        
        MCObject *optr = NULL;
        //get image from chunk
        uint4 parid;
        if (!image->getobj(ctxt, optr, parid, True))
        {
            ctxt . LegacyThrow(EE_EXPORT_NOSELECTED);
            return;
        }
        
        MCAutoArrayRef t_array;
        MCInterfaceExecExportObjectToArray(ctxt, optr, &t_array);
        if (ctxt . HasError())
            return;
        
        dest->set(ctxt, PT_INTO, *t_array);
        if (ctxt . HasError())
        {
            ctxt . LegacyThrow(EE_EXPORT_CANTWRITE);
            return;
        }
        
        return;
    }
    
    MCAutoDataRef t_return_data;
    MCAutoStringRef t_filename;
    if (!ctxt . EvalOptionalExprAsNullableStringRef(fname, EE_EXPORT_BADNAME, &t_filename))
        return;

	MCAutoStringRef t_mask_filename;
    if (!ctxt . EvalOptionalExprAsNullableStringRef(mname, EE_EXPORT_BADNAME, &t_mask_filename))
        return;
    
    // MERG-2014-07-11: metadata array
    MCAutoArrayRef t_metadata_array;
    MCImageMetadata t_metadata;
    if (!ctxt . EvalOptionalExprAsArrayRef(metadata, kMCEmptyArray , EE_EXPORT_NOSELECTED, &t_metadata_array))
        return;

    MCImageParseMetadata(ctxt, *t_metadata_array, t_metadata);

	MCObject *optr = NULL;
	if (image != NULL)
	{
		//get image from chunk
		uint4 parid;
        if (!image->getobj(ctxt, optr, parid, True))
        {
            ctxt . LegacyThrow(EE_EXPORT_NOSELECTED);
            return;
        }
	}

	MCInterfaceImagePaletteSettings t_settings;
	MCInterfaceImagePaletteSettings *t_settings_ptr = nil;

    switch (palette_type)
    {
    case kMCImagePaletteTypeCustom:
        {
        MCColor *t_colors;
        uindex_t t_count;
        MCAutoStringRef t_input;
        if (!ctxt . EvalExprAsStringRef(palette_color_list, EE_EXPORT_BADPALETTE, &t_input))
            return;

        if (MCStringIsEmpty(*t_input)
                || !MCImageParseColourList(*t_input, t_count, t_colors))
        {
            ctxt . LegacyThrow(EE_EXPORT_BADPALETTE);
            return;
        }

        MCInterfaceMakeCustomImagePaletteSettings(ctxt, t_colors, t_count, t_settings);
        t_settings_ptr = &t_settings;
        }
        break;
    case kMCImagePaletteTypeOptimal:
        {
        integer_t *t_count_ptr = nil;
        integer_t t_count;
        if (!ctxt . EvalOptionalExprAsInt(palette_color_count, 0, EE_EXPORT_BADPALETTESIZE, t_count))
            return;

        if (t_count != 0)
            t_count_ptr = &t_count;

        MCInterfaceMakeOptimalImagePaletteSettings(ctxt, t_count_ptr, t_settings);
        t_settings_ptr = &t_settings;
        }
        break;
    case kMCImagePaletteTypeWebSafe:
        MCInterfaceMakeWebSafeImagePaletteSettings(ctxt, t_settings);
        t_settings_ptr = &t_settings;
        break;
    case kMCImagePaletteTypeEmpty:
        t_settings . type = kMCImagePaletteTypeEmpty;
        break;
    }

    // AL-2014-10-27: [[ Bug 13804 ]] Make sure execution stops here if there was an error creating palette settings
    if (ctxt . HasError())
        return;
    
    bool t_success;
    t_success = true;
    if (sformat == EX_SNAPSHOT)
    {
        MCRectangle *t_rect_ptr;
        MCRectangle t_rect;

        t_rect_ptr = &t_rect;

        t_success = ctxt . EvalOptionalExprAsRectangle(exsrect, nil, EE_IMPORT_BADNAME, t_rect_ptr);
            
        MCPoint t_size;
        MCPoint *t_size_ptr = nil;
            
        t_size_ptr = &t_size;
        
        if (t_success)
            t_success = ctxt . EvalOptionalExprAsPoint(size, nil, EE_EXPORT_NOSELECTED, t_size_ptr);
       
        if (t_success)
        {
            if (exsstack != NULL)
            {
                MCAutoStringRef t_stack_name, t_display;
                
                t_success = ctxt . EvalExprAsStringRef(exsstack, EE_EXPORT_NOSELECTED, &t_stack_name);
                
                if (t_success)
                    t_success = ctxt . EvalOptionalExprAsNullableStringRef(exsdisplay, EE_EXPORT_NOSELECTED, &t_display);
                
                if (t_success)
                {
					if (*t_filename == nil)
						MCInterfaceExecExportSnapshotOfStack(ctxt, *t_stack_name, *t_display, t_rect_ptr, t_size_ptr, format, t_settings_ptr, &t_metadata, &t_return_data);
					else
						MCInterfaceExecExportSnapshotOfStackToFile(ctxt, *t_stack_name, *t_display, t_rect_ptr, t_size_ptr, format, t_settings_ptr, &t_metadata, *t_filename, *t_mask_filename);
				}
			}
            else if (optr != NULL)
            {
                
                if (*t_filename == nil)
                    MCInterfaceExecExportSnapshotOfObject(ctxt, optr, t_rect_ptr, with_effects, t_size_ptr, format, t_settings_ptr, &t_metadata, &t_return_data);
                else
                    MCInterfaceExecExportSnapshotOfObjectToFile(ctxt, optr, t_rect_ptr, with_effects, t_size_ptr, format, t_settings_ptr, &t_metadata, *t_filename, *t_mask_filename);
            }
            else
            {
                if (*t_filename == nil)
                    MCInterfaceExecExportSnapshotOfScreen(ctxt, t_rect_ptr, t_size_ptr, format, t_settings_ptr, &t_metadata, &t_return_data);
                else
                    MCInterfaceExecExportSnapshotOfScreenToFile(ctxt, t_rect_ptr, t_size_ptr, format, t_settings_ptr, &t_metadata, *t_filename, *t_mask_filename);
            }
        }
	}
	else
	{
		if (*t_filename == nil)
			MCInterfaceExecExportImage(ctxt, (MCImage *)optr, format, t_settings_ptr, &t_metadata, &t_return_data);
		else
			MCInterfaceExecExportImageToFile(ctxt, (MCImage *)optr, format, t_settings_ptr, &t_metadata, *t_filename, *t_mask_filename);
	}
    
    MCInterfaceImagePaletteSettingsFree(ctxt, t_settings);

	if (*t_return_data != nil)
	{
        dest->set(ctxt, PT_INTO, *t_return_data);

        if (ctxt . HasError())
		{
            ctxt . LegacyThrow(EE_EXPORT_CANTWRITE);
            return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

MCFilter::~MCFilter()
{
	delete container;
	delete target;
	delete source;
	delete pattern;
}

// JS-2013-07-01: [[ EnhancedFilter ]] Rewritten to support new filter syntax.
Parse_stat MCFilter::parse(MCScriptPoint &sp)
{
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
        else if (sp.skip_token(SP_FACTOR, TT_FUNCTION, F_KEYS) == PS_NORMAL)
            chunktype = CT_KEY;
        else if (sp.skip_token(SP_FACTOR, TT_CLASS, CT_ELEMENT) == PS_NORMAL)
            chunktype = CT_ELEMENT;
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
		container = new (nothrow) MCChunk(True);
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
        else if (sp.skip_token(SP_MARK, TT_UNDEFINED, MC_WHERE) == PS_NORMAL)
        {
            discardmatches = False;
            matchmode = MA_EXPRESSION;
        }
		else
			t_error = PE_FILTER_NOWITH;
	}

	// Now look for the optional pattern match mode
	if (t_error == PE_UNDEFINED && matchmode == MA_UNDEFINED)
	{
		if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_REGEX) == PS_NORMAL)
			matchmode = MA_REGEX;
        else if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_WILDCARD) == PS_NORMAL)
			matchmode = MA_WILDCARD;
        // Skip the optional pattern keyword
		sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_PATTERN);
	}
    
    if (matchmode == MA_UNDEFINED)
        matchmode = MA_WILDCARD;

	// Now parse the pattern expression
	if (t_error == PE_UNDEFINED && sp.parseexp(False, True, &pattern) != PS_NORMAL)
		t_error = PE_FILTER_BADEXP;

	// Finally check for the (optional) 'into' clause
	if (t_error == PE_UNDEFINED)
	{
		if (sp.skip_token(SP_FACTOR, TT_PREP, PT_INTO) == PS_NORMAL)
		{
			target = new (nothrow) MCChunk(True);
			if (target->parse(sp, False) != PS_NORMAL)
				t_error = PE_FILTER_BADDEST;
        }
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
void MCFilter::exec_ctxt(MCExecContext &ctxt)
{
    MCAutoValueRef t_source;
    MCAutoStringRef t_pattern;
    
    if (container != NULL)
    {
        if (!ctxt . EvalExprAsValueRef(container, EE_FILTER_CANTGET, &t_source))
            return;
    }
    else
    {
        if (!ctxt . EvalExprAsValueRef(source, EE_FILTER_CANTGET, &t_source))
            return;
    }
    
    if (matchmode != MA_EXPRESSION)
    {
        if (!ctxt . EvalExprAsStringRef(pattern, EE_FILTER_CANTGETPATTERN, &t_pattern))
            return;
    }
        
    MCAutoStringRef t_source_string;
    MCAutoArrayRef t_source_array;
    
    if (chunktype == CT_LINE || chunktype == CT_ITEM)
    {
        if (!ctxt . ConvertToString(*t_source, &t_source_string))
        {
            ctxt . LegacyThrow(EE_FILTER_CANTGET);
            return;
        }
    }
    else
    {
        if (!ctxt . ConvertToArray(*t_source, &t_source_array))
        {
            ctxt . LegacyThrow(EE_FILTER_CANTGET);
            return;
        }
    }
    
    bool stat;
    
    if (chunktype == CT_LINE || chunktype == CT_ITEM)
    {
        MCAutoStringRef t_output_string;
        
        if (matchmode == MA_REGEX)
            MCStringsExecFilterRegex(ctxt, *t_source_string, *t_pattern, discardmatches == True, chunktype == CT_LINE, &t_output_string);
        else if (matchmode == MA_EXPRESSION)
            MCStringsExecFilterExpression(ctxt, *t_source_string, pattern, discardmatches == True, chunktype == CT_LINE, &t_output_string);
        else
            MCStringsExecFilterWildcard(ctxt, *t_source_string, *t_pattern, discardmatches == True, chunktype == CT_LINE, &t_output_string);
        
        if (*t_output_string != nil)
        {
            if (target != nil)
                stat = target -> set(ctxt, PT_INTO, *t_output_string);
            else if (container != nil)
                stat = container -> set(ctxt, PT_INTO, *t_output_string);
            else
            {
                ctxt . SetItToValue(*t_output_string);
                stat = true;
            }
                
            if (!stat)
            {
                ctxt . LegacyThrow(EE_FILTER_CANTSET);
                return;
            }
        }
        else if (target == nil && container == nil)
        {
            ctxt . SetItToEmpty();
        }
    }
    else
    {
        MCAutoArrayRef t_output_array;
        
        if (matchmode == MA_REGEX)
            MCArraysExecFilterRegex(ctxt, *t_source_array, *t_pattern, discardmatches == True, chunktype == CT_KEY, &t_output_array);
        else if (matchmode == MA_EXPRESSION)
            MCArraysExecFilterExpression(ctxt, *t_source_array, pattern, discardmatches == True, chunktype == CT_KEY, &t_output_array);
        else
            MCArraysExecFilterWildcard(ctxt, *t_source_array, *t_pattern, discardmatches == True, chunktype == CT_KEY, &t_output_array);
        
        if (*t_output_array != nil)
        {
            if (target != nil)
                stat = target -> set(ctxt, PT_INTO, *t_output_array);
            else if (container != nil)
                stat = container -> set(ctxt, PT_INTO, *t_output_array);
            else
            {
                ctxt . SetItToValue(*t_output_array);
                stat = true;
            }
            
            if (!stat)
            {
                ctxt . LegacyThrow(EE_FILTER_CANTSET);
                return;
            }
        }
        else if (target == nil && container == nil)
        {
            ctxt . SetItToEmpty();
        }

    }
    
    if (ctxt . HasError())
        ctxt . LegacyThrow(EE_FILTER_CANTSET);
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
	const LT *te = NULL;

	initpoint(sp);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_IMPORT_NOTYPE, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_EXPORT, te) != PS_NORMAL)
	{
        if (sp.lookup(SP_FACTOR, te) != PS_NORMAL ||
            te -> type != TT_CHUNK ||
            te -> which != CT_WIDGET)
        {
            MCperror->add(PE_IMPORT_BADTYPE, sp);
            return PS_ERROR;
        }
        format = EX_OBJECT;
	}
    else
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
				Symbol_type t_type;
				const LT *t_te = NULL;

				// MW-2006-03-24: Bug 3442 - crash in specific case due to not checking result of sp.lookup
				if (sp.next(t_type) == PS_NORMAL && sp.lookup(SP_FACTOR, t_te) == PS_NORMAL && t_te -> type == TT_CHUNK && t_te -> which == CT_STACK)
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
					container = new (nothrow) MCChunk(False);
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
				}
			}
		}
		
		// MW-2014-02-20: [[ Bug 11811 ]] Add the 'at size' clause to screen snapshot.
		if (sp . skip_token(SP_FACTOR, TT_PREP, PT_AT) == PS_NORMAL)
		{
			if (sp . skip_token(SP_FACTOR, TT_PROPERTY, P_SIZE) != PS_NORMAL ||
				sp . parseexp(False, True, &size) != PS_NORMAL)
			{
				MCperror -> add(PE_IMPORT_BADFILENAME, sp);
				return PS_ERROR;
			}
		}
		return PS_NORMAL;
	}
    
	if (sp.skip_token(SP_FACTOR, TT_FROM) != PS_NORMAL)
	{
		MCperror->add(PE_IMPORT_NOFROM, sp);
		return PS_ERROR;
	}
    if (sp.skip_token(SP_VALIDATION, TT_UNDEFINED, IV_ARRAY) == PS_NORMAL)
    {
        if (sp.parseexp(False, True, &fname) != PS_NORMAL)
        {
            MCperror -> add(PE_IMPORT_NOARRAY, sp);
            return PS_ERROR;
        }
    }
    else
    {
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
    }
	if (sp.skip_token(SP_FACTOR, TT_IN) == PS_NORMAL
	        || sp.skip_token(SP_FACTOR, TT_PREP) == PS_NORMAL)
	{
		container = new (nothrow) MCChunk(False);
		if (container->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_CREATE_BADBGORCARD, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

void MCImport::exec_ctxt(MCExecContext &ctxt)
{
	if (format == EX_SNAPSHOT)
    {
        MCRectangle t_rectangle;
        MCRectangle *t_rect_ptr;

        t_rect_ptr = &t_rectangle;

        if (!ctxt . EvalOptionalExprAsRectangle(fname, nil, EE_IMPORT_BADNAME, t_rect_ptr))
            return;

        MCPoint t_size;
        MCPoint *t_size_ptr = &t_size;
        
        if (!ctxt . EvalOptionalExprAsPoint(size, nil, EE_IMPORT_NOSELECTED, t_size_ptr))
            return;
        
		if (container != NULL)
		{
			MCObject *t_parent = nil;
			uint4 parid;

            if (!container->getobj(ctxt, t_parent, parid, True))
			{
                ctxt . LegacyThrow(EE_IMPORT_BADNAME);
                return;
			}

			MCInterfaceExecImportSnapshotOfObject(ctxt, t_parent, t_rect_ptr, with_effects, t_size_ptr);
		}
        else if (mname != NULL)
        {
            MCAutoStringRef t_stack;
            if (!ctxt . EvalExprAsStringRef(mname, EE_IMPORT_BADNAME, &t_stack))
                return;

			MCAutoStringRef t_display;
            if (!ctxt . EvalOptionalExprAsNullableStringRef(dname, EE_IMPORT_BADNAME, &t_display))
                return;
			// MW-2014-02-20: [[ Bug 11811 ]] Pass the wanted size to the snapshot method.
			MCInterfaceExecImportSnapshotOfStack(ctxt, *t_stack, *t_display, t_rect_ptr, t_size_ptr);
		}
		else
            // MW-2014-02-20: [[ Bug 11811 ]] Pass the wanted size to the snapshot method.
			MCInterfaceExecImportSnapshotOfScreen(ctxt, t_rect_ptr, t_size_ptr);
	}
	else if (format == EX_OBJECT)
    {
        MCAutoArrayRef t_array;
        if (!ctxt . EvalExprAsArrayRef(fname, EE_IMPORT_BADARRAY, &t_array))
            return;
        
        MCObject *parent = NULL;
        if (container != NULL)
        {
            uint4 parid;
            if (!container->getobj(ctxt, parent, parid, True)
                || (parent->gettype() != CT_GROUP && parent->gettype() != CT_CARD))
            {
                ctxt . LegacyThrow(EE_CREATE_BADBGORCARD);
                return;
            }
        }
        
        MCInterfaceExecImportObjectFromArray(ctxt, *t_array, parent);
    }
    else
	{
		MCAutoStringRef t_filename;
        if (!ctxt . EvalOptionalExprAsNullableStringRef(fname, EE_IMPORT_BADNAME, &t_filename))
            return;

		switch (format)
		{
            case EX_AUDIO_CLIP:
                MCInterfaceExecImportAudioClip(ctxt, *t_filename);
                break;
            case EX_VIDEO_CLIP:
                MCInterfaceExecImportVideoClip(ctxt, *t_filename);
                break;
            case EX_EPS:
                MCLegacyExecImportEps(ctxt, *t_filename);
                break;
            case EX_STACK:
                MCLegacyExecImportHypercardStack(ctxt, *t_filename);
                break;
            default:
			{
				MCObject *parent = NULL;
				if (container != NULL)
				{
                    uint4 parid;
                    if (!container->getobj(ctxt, parent, parid, True)
                            || (parent->gettype() != CT_GROUP && parent->gettype() != CT_CARD))
					{
                        ctxt . LegacyThrow(EE_CREATE_BADBGORCARD);
                        return;
					}
				}
				MCAutoStringRef t_mask_filename;

                if (!ctxt . EvalOptionalExprAsNullableStringRef(mname, EE_IMPORT_BADNAME, &t_mask_filename))
                    return;

				MCInterfaceExecImportImage(ctxt, *t_filename, *t_mask_filename, parent);
			}
                break;
		}
    }
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

void MCKill::exec_ctxt(MCExecContext& ctxt)
{
    MCAutoStringRef t_signal;
    if (!ctxt . EvalOptionalExprAsNullableStringRef(sig, EE_KILL_BADNUMBER, &t_signal))
        return;
    
    MCAutoStringRef t_process;
    if (!ctxt . EvalExprAsStringRef(pname, EE_KILL_BADNAME, &t_process))
        return;
    
    MCFilesExecKillProcess(ctxt, *t_process, *t_signal);
}

MCOpen::~MCOpen()
{
	delete fname;
	delete message;
	delete go;
    delete encoding;
	MCValueRelease(destination);
	delete certificate;
	delete verifyhostname;
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
		go = new (nothrow) MCGo;
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

            destination = MCValueRetain(sp . gettoken_stringref());

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
    if (arg == OA_SOCKET && sp.skip_token(SP_FACTOR, TT_FROM, PT_FROM) == PS_NORMAL)
    {
        if (sp.parseexp(False, True, &(&fromaddress)) != PS_NORMAL)
        {
            MCperror->add(PE_OPEN_NOFROM, sp);
            return PS_ERROR;
        }
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

        // SN-2014-02-13: text encoding option added to the 'open' function
        // SN-2014-05-06 [[ Bug 12360 ]] 'open' doesn't take expressions as encoding type
        // Now check for the encoding as an expression instead of a token        
        if (sp.lookup(SP_MODE, te) != PS_NORMAL)
        {
            // Look for an encoding at first
            if (sp.backup() != PS_NORMAL || sp.parseexp(True, True, &encoding) != PS_NORMAL)
            {
                MCperror -> add(PE_OPEN_BADENCODING, sp);
                return PS_ERROR;
            }
            
            if (sp.next(type) != PS_NORMAL)
            {
                MCperror -> add(PE_OPEN_BADMODE, sp);
                return PS_ERROR;
            }
        }
        
        if (sp.lookup(SP_MODE, te) != PS_NORMAL)
        {
            MCperror -> add(PE_OPEN_BADMODE, sp);
            return PS_ERROR;
        }
        
        if (encoding != NULL && te->which == OM_BINARY)
        {
            MCperror->add(PE_OPEN_BADBINARYENCODING, sp);
            return PS_ERROR;
        }

		if (te->which == OM_BINARY || te->which == OM_TEXT)
        {
            // AL-2014-05-27: [[ Bug 12493 ]] Make sure binary open is done as binary
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
	{
		if (sp.skip_token(SP_SSL, TT_UNDEFINED, SSL_VERIFICATION) != PS_NORMAL)
		{
			MCperror->add
			(PE_OPEN_BADMESSAGE, sp);
		}
		
		// MM-2014-06-13: [[ Bug 12567 ]] Added new "with verification for <host>" variant.
		if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_FOR) == PS_NORMAL)
		{
			if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_HOST) != PS_NORMAL)
			{
				MCperror -> add(PE_OPEN_NOHOST, sp);
				return PS_ERROR;
			}			
			if (sp . parseexp(False, True, &verifyhostname) != PS_NORMAL)
			{
				MCperror -> add(PE_OPEN_BADHOST, sp);
				return PS_ERROR;
			}
		}	
	}

	if (sp.skip_token(SP_SUGAR, TT_PREP, PT_WITHOUT) == PS_NORMAL)
	{
		if (sp.skip_token(SP_SSL, TT_UNDEFINED, SSL_VERIFICATION) == PS_NORMAL)
			secureverify = False;
		else
			MCperror->add (PE_OPEN_BADMESSAGE, sp);
	}
	return PS_NORMAL;
}

void MCOpen::exec_ctxt(MCExecContext &ctxt)
{
	if (go != NULL)
        return go->exec_ctxt(ctxt);

    ctxt . SetTheResultToEmpty();

	if (arg == OA_PRINTING)
	{
		// Handle custom printer formt
		if (destination != NULL)
		{
            MCAutoStringRef t_filename;
            MCAutoArrayRef t_options;
            if (!ctxt . EvalExprAsStringRef(fname, EE_OPEN_BADNAME, &t_filename))
                return;

            if (!ctxt . EvalOptionalExprAsNullableArrayRef(options, EE_OPEN_BADOPTIONS, &t_options))
                return;

			MCPrintingExecOpenPrintingToDestination(ctxt, destination, *t_filename, *t_options);
		}
		else if (dialog)
		{
			// If 'with dialog' was specified do an 'answer printer' here. Note that
			// we exit if 'cancel' is returned for backwards compatibility. This form
			// the open printing command is, however, deprecated.
			MCPrintingExecOpenPrintingWithDialog(ctxt, sheet == True);
		}
		else
			MCPrintingExecOpenPrinting(ctxt);
	}
	else
	{
        MCNewAutoNameRef t_name;
        MCNewAutoNameRef t_message_name;
        MCAutoStringRef t_encoding_as_string;
        Encoding_type t_encoding;

        if (!ctxt . EvalExprAsNameRef(fname, EE_OPEN_BADNAME, &t_name))
            return;
        
        if (encoding != NULL)
        {
            extern bool MCStringsEvalTextEncoding(MCStringRef p_encoding, MCStringEncoding &r_encoding);
            
            if (!ctxt . EvalExprAsStringRef(encoding, EE_OPEN_BADENCODING, &t_encoding_as_string))
                return;
            
            if (MCStringIsEqualToCString(*t_encoding_as_string, "binary", kMCStringOptionCompareCaseless))
                t_encoding = EN_BINARY;
            else
            {
                MCStringEncoding t_string_encoding;
                if (!MCStringsEvalTextEncoding(*t_encoding_as_string, (MCStringEncoding&)t_string_encoding))
                {
                    ctxt . LegacyThrow(EE_OPEN_BADENCODING);
                    return;
                }
                                
                switch (t_string_encoding)
                {
                    case kMCStringEncodingUTF8:
                        t_encoding = EN_UTF8;
                        break;
                        
                    case kMCStringEncodingUTF16LE:
                        t_encoding = EN_UTF16LE;
                        break;
                        
                    case kMCStringEncodingUTF16BE:
                        t_encoding = EN_UTF16BE;
                        break;
                        
                    case kMCStringEncodingUTF16:
                        t_encoding = EN_UTF16;
                        break;
                        
                    case kMCStringEncodingNative:
                        t_encoding = EN_NATIVE;
                        break;
                    
                    case kMCStringEncodingUTF32:
                        t_encoding = EN_UTF32;
                        break;
                        
                    case kMCStringEncodingUTF32LE:
                        t_encoding = EN_UTF32LE;
                        break;
                        
                    case kMCStringEncodingUTF32BE:
                        t_encoding = EN_UTF32BE;
                        break;
                        
                    default:
                        // ASCII
                        ctxt . LegacyThrow(EE_OPEN_UNSUPPORTED_ENCODING);
                        return;
                }        
            }
        }
        else if (!textmode)
            t_encoding = EN_BINARY;
        else
            t_encoding = EN_BOM_BASED;

		switch (arg)
		{
        case OA_DRIVER:
            MCFilesExecOpenDriver(ctxt, *t_name, mode, t_encoding);
			break;
        case OA_FILE:
            MCFilesExecOpenFile(ctxt, *t_name, mode, t_encoding);
			break;
		case OA_PROCESS:
			if (elevated)
                MCFilesExecOpenElevatedProcess(ctxt, *t_name, mode, t_encoding);
			else
                MCFilesExecOpenProcess(ctxt, *t_name, mode, t_encoding);
			break;
		case OA_SOCKET:
        {                
            if (!ctxt . EvalOptionalExprAsNullableNameRef(message, EE_OPEN_BADMESSAGE, &t_message_name))
                return;
            
            MCNewAutoNameRef t_from_address;
            if (!ctxt . EvalOptionalExprAsNameRef(fromaddress.Get(), kMCEmptyName, EE_OPEN_BADFROMADDRESS, &t_from_address))
                return;

            // MM-2014-06-13: [[ Bug 12567 ]] Added support for specifying an end host name to verify against.
            MCNewAutoNameRef t_end_hostname;
            if (!ctxt . EvalOptionalExprAsNameRef(verifyhostname, kMCEmptyName, EE_OPEN_BADHOST, &t_end_hostname))
                return;

			if (datagram)
				MCNetworkExecOpenDatagramSocket(ctxt, *t_name, *t_from_address, *t_message_name, *t_end_hostname);
			else if (secure)
				MCNetworkExecOpenSecureSocket(ctxt, *t_name, *t_from_address, *t_message_name, *t_end_hostname, secureverify);
			else
				MCNetworkExecOpenSocket(ctxt, *t_name, *t_from_address, *t_message_name, *t_end_hostname);
			break;
        }
		default:
			break;
		}
    }
}

MCRead::~MCRead()
{
	delete fname;
	delete maxwait;
	delete stop;
	delete at;
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
		if (arg == OA_SOCKET)
			cond = RF_UNTIL;
		else
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
	return PS_NORMAL;
}

void MCRead::exec_ctxt(MCExecContext& ctxt)
{
    real8 t_max_wait;
    if (!ctxt . EvalOptionalExprAsDouble(maxwait, MAXUINT4, EE_READ_BADEXP, t_max_wait))
        return;

    ctxt . SetTheResultToEmpty();
    MCNewAutoNameRef t_message;
    MCNewAutoNameRef t_source;
	bool t_is_end = false;
	int64_t t_at = 0;
    
	if (arg != OA_STDIN)
	{
        if (!ctxt . EvalExprAsNameRef(fname, EE_OPEN_BADNAME, &t_source))
            return;
        switch (arg)
		{
            case OA_DRIVER:
            case OA_FILE:
                if (at != NULL)
                {
                    MCAutoStringRef t_temp;
                    if (!ctxt . EvalExprAsStringRef(at, EE_READ_BADAT, &t_temp))
                        return;
                    
                    if (MCStringGetNativeCharAtIndex(*t_temp, 0) == '\004' || MCStringIsEqualToCString(*t_temp, "eof", kMCCompareCaseless))
                    {
                        t_is_end = true;
                        t_at = 0;
                    }
                    else
                    {
                        double n;
                        if (!MCStringToDouble(*t_temp, n))
                        {
                            ctxt . SetTheResultToCString("error seeking in file");
                            return;
                        }
                        else
                        {
                            if (n < 0)
                            {
                                t_is_end = true;
                                t_at = (int64_t)n;
                            }
                            else
                            {
                                t_is_end = false;
                                t_at = (int64_t)n - 1;
                            }
                        }
                    }
                }
                break;
            case OA_SOCKET:
                if (!ctxt . EvalOptionalExprAsNullableNameRef(at, EE_WRITE_BADEXP, &t_message))
                    return;
                break;
            default:
                break;
		}
	}
    switch (cond)
	{
        case RF_FOR:
        {
            uint4 size;
            if (!ctxt . EvalExprAsUInt(stop, EE_READ_NONUMBER, size))
            {
                MCshellfd = -1;
                return;
            }
            
            switch (arg)
            {
                case OA_FILE:
                case OA_DRIVER:
                    if (at != NULL)
                    {
                        if (t_is_end)
                            MCFilesExecReadFromFileOrDriverAtEndFor(ctxt, arg == OA_DRIVER, *t_source, t_at, size, unit, t_max_wait, timeunits);
                        else
                            MCFilesExecReadFromFileOrDriverAtFor(ctxt, arg == OA_DRIVER, *t_source, t_at, size, unit, t_max_wait, timeunits);
                    }
                    else
                        MCFilesExecReadFromFileOrDriverFor(ctxt, arg == OA_DRIVER, *t_source, size, unit, t_max_wait, timeunits);
                    break;
                case OA_PROCESS:
                    MCFilesExecReadFromProcessFor(ctxt, *t_source, size, unit, t_max_wait, timeunits);
                    break;
                case OA_SOCKET:
                    MCNetworkExecReadFromSocketFor(ctxt, *t_source, size, unit, *t_message);
                    break;
                case OA_STDIN:
                    MCFilesExecReadFromStdinFor(ctxt, size, unit, t_max_wait, timeunits);
                    break;
                default:
                    break;
            }
            break;
        }
            
        case RF_UNTIL:
        {
            MCAutoStringRef t_sentinel;
            if (!ctxt . EvalOptionalExprAsNullableStringRef(stop, EE_READ_NOCHARACTER, &t_sentinel))
            {
                MCshellfd = -1;
                return;
            }
        
            switch (arg)
            {
                case OA_FILE:
                case OA_DRIVER:
                    if (at != NULL)
                    {
                        if (t_is_end)
                            MCFilesExecReadFromFileOrDriverAtEndUntil(ctxt, arg == OA_DRIVER, *t_source, t_at, *t_sentinel, t_max_wait, timeunits);
                        else
                            MCFilesExecReadFromFileOrDriverAtUntil(ctxt, arg == OA_DRIVER, *t_source, t_at, *t_sentinel, t_max_wait, timeunits);
                    }
                    else
                        MCFilesExecReadFromFileOrDriverUntil(ctxt, arg == OA_DRIVER, *t_source, *t_sentinel, t_max_wait, timeunits);
                    break;
                case OA_PROCESS:
                    MCFilesExecReadFromProcessUntil(ctxt, *t_source, *t_sentinel, t_max_wait, timeunits);
                    break;
                case OA_SOCKET:
                    MCNetworkExecReadFromSocketUntil(ctxt, *t_source, *t_sentinel, *t_message);
                    break;
                case OA_STDIN:
                    MCFilesExecReadFromStdinUntil(ctxt, *t_sentinel, t_max_wait, timeunits);
                    break;
                default:
                    break;
            }
        }
            break;
		default:
        ctxt . LegacyThrow(EE_READ_BADCOND);
		MCshellfd = -1;
		return;
	}
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

void MCSeek::exec_ctxt(MCExecContext& ctxt)
{
    MCNewAutoNameRef t_file;
    if (!ctxt . EvalExprAsNameRef(fname, EE_SEEK_BADNAME, &t_file))
        return;
    
    MCAutoStringRef t_temp;
    if (ctxt . EvalExprAsStringRef(where, EE_UNDEFINED, &t_temp))
    {
        if (MCStringGetNativeCharAtIndex(*t_temp, 0) == '\004' || MCStringIsEqualToCString(*t_temp, "eof", kMCCompareCaseless))
            MCFilesExecSeekToEofInFile(ctxt, *t_file);
        else
        {
            double n;
            if (!MCStringToDouble(*t_temp, n))
            {
                ctxt . LegacyThrow(EE_SEEK_BADWHERE);
				return;
            }
            if (mode == PT_TO)
				MCFilesExecSeekAbsoluteInFile(ctxt,(int64_t)n, *t_file);
			else
				MCFilesExecSeekRelativeInFile(ctxt, (int64_t)n, *t_file);
        }
    }
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

void MCWrite::exec_ctxt(MCExecContext& ctxt)
{
    ctxt . SetTheResultToEmpty();
	MCAutoStringRef t_data;
    if (!ctxt . EvalExprAsStringRef(source, EE_WRITE_BADEXP, &t_data))
        return;
	
    if (arg == OA_STDERR)
		MCFilesExecWriteToStderr(ctxt, *t_data, unit);
	else
		if (arg == OA_STDOUT)
			MCFilesExecWriteToStdout(ctxt, *t_data, unit);
		else
		{
			MCNewAutoNameRef t_target;
            if (!ctxt . EvalExprAsNameRef(fname, EE_WRITE_BADEXP, &t_target))
                return;
            switch (arg)
			{
                case OA_DRIVER:
                case OA_FILE:
                    if (at != NULL)
                    {
                        MCAutoStringRef t_temp;
                        if (!ctxt . EvalExprAsStringRef(at, EE_WRITE_BADEXP, &t_temp))
                            return;
                        if (MCStringGetNativeCharAtIndex(*t_temp, 0) == '\004' || MCStringIsEqualToCString(*t_temp, "eof", kMCCompareCaseless))
                            MCFilesExecWriteToFileOrDriverAtEnd(ctxt, *t_target, *t_data, unit);
                        else
                        {
                            double n;
                            if (!MCStringToDouble(*t_temp, n))
                            {
                                ctxt . SetTheResultToCString("error seeking in file");
                                return;
                            }
                            MCFilesExecWriteToFileOrDriverAt(ctxt, *t_target, *t_data, unit, (int64_t)n);
                        }
                    }
                    else
                        MCFilesExecWriteToFileOrDriver(ctxt, *t_target, *t_data, unit);
                    break;
                case OA_PROCESS:
                    MCFilesExecWriteToProcess(ctxt, *t_target, *t_data, unit);
                    break;
                case OA_SOCKET:
				{
                    MCNewAutoNameRef t_message;
                    if (!ctxt . EvalOptionalExprAsNullableNameRef(at, EE_WRITE_BADEXP, &t_message))
                        return;
                    MCNetworkExecWriteToSocket(ctxt, *t_target, *t_data, *t_message);
				}
                    break;
                default:
                    break;
			}
		}
}

////////////////////////////////////////////////////////////////////////////////

// MM-2014-02-12: [[ SecureSocket ]]
// MM-2014-06-13: [[ Bug 12567 ]] Added new "with verification for <host>" variant.
//  New secure socket command, used to ensure all future communications over the given socket are encrypted.
//
//  After securing:
//    All pending and future reads from the socket will assumed to be encrypted.
//    All pending writes will continue unencrypted. All future writes will be encrypted.
//
//  Unless specified, the connection will be verified.
//
//  Syntax:
//    secure socket <socket>
//    secure socket <socket> with verification
//    secure socket <socket> without verification
//    secure socket <socket> with verification for host <host>
//
MCSecure::~MCSecure()
{
	delete m_sock_name;
	delete m_verify_host_name;
}

Parse_stat MCSecure::parse(MCScriptPoint &sp)
{	
	initpoint(sp);
	
	Symbol_type type;
	if (sp . next(type) != PS_NORMAL)
	{
		MCperror -> add(PE_SECURE_NOSOCKET, sp);
		return PS_ERROR;
	}
	
	const LT *te;
	if (sp . lookup(SP_OPEN, te) != PS_NORMAL)
	{
		MCperror -> add(PE_SECURE_NOSOCKET, sp);
		return PS_ERROR;
	}
	
	Open_argument t_open_arg;
	t_open_arg = (Open_argument) te -> which;
	if (t_open_arg != OA_SOCKET)
	{
		MCperror -> add(PE_SECURE_NOSOCKET, sp);
		return PS_ERROR;
	}
	
	if (sp . parseexp(False, True, &m_sock_name) != PS_NORMAL)
	{
		MCperror -> add(PE_SECURE_BADNAME, sp);
		return PS_ERROR;
	}
	
	if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		if (sp . skip_token(SP_SSL, TT_UNDEFINED, SSL_VERIFICATION) != PS_NORMAL)
		{
			MCperror -> add(PE_SECURE_BADMESSAGE, sp);
			return PS_ERROR;
		}
		
		secureverify = True;
			
		// MM-2014-06-13: [[ Bug 12567 ]] Added new "with verification for <host>" variant.
		if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_FOR) == PS_NORMAL)
		{
			if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_HOST) != PS_NORMAL)
			{
				MCperror -> add(PE_SECURE_NOHOST, sp);
				return PS_ERROR;
			}			
			if (sp . parseexp(False, True, &m_verify_host_name) != PS_NORMAL)
			{
				MCperror -> add(PE_SECURE_BADHOST, sp);
				return PS_ERROR;
			}
		}	
	}
	
	if (sp . skip_token(SP_SUGAR, TT_PREP, PT_WITHOUT) == PS_NORMAL)
	{
		if (sp . skip_token(SP_SSL, TT_UNDEFINED, SSL_VERIFICATION) == PS_NORMAL)
			secureverify = False;
		else
		{
			MCperror -> add(PE_SECURE_BADMESSAGE, sp);
			return PS_ERROR;
		}
	}
	
	return PS_NORMAL;
}

void MCSecure::exec_ctxt(MCExecContext& ctxt)
{
    
    MCNewAutoNameRef t_name;
    if (!ctxt . EvalExprAsNameRef(m_sock_name, EE_SECURE_BADNAME, &t_name))
        return;
	
	// MM-2014-06-13: [[ Bug 12567 ]] Added passing through the host name to verify against.
    // SN-2015-05-05: [[ Bug 15314 ]] The host name should be initialised.
	MCNewAutoNameRef t_host_name;
    if (!ctxt . EvalOptionalExprAsNameRef(m_verify_host_name, kMCEmptyName, EE_SECURE_BADHOST, &t_host_name))
        return;

    MCSecurityExecSecureSocket(ctxt, *t_name, secureverify == True, *t_host_name);
}
