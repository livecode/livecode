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



////////////////////////////////////////////////////////////////////////////////

static const char *s_orientation_names[] =
{
	"unknown", "portrait", "portrait upside down", "landscape right", "landscape left", "face up", "face down", nil
};

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleDeviceOrientation(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil)
	MCExecContext ctxt(ep);

	intenum_t t_orientation;
	MCOrientationGetDeviceOrientation(ctxt, t_orientation);

	ctxt . SetTheResultToStaticCString(s_orientation_names[(int)t_orientation]);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleOrientation(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil)
	MCExecContext ctxt(ep);

	intenum_t t_orientation;
	MCOrientationGetOrientation(ctxt, t_orientation);

	ctxt . SetTheResultToStaticCString(s_orientation_names[(int)r_orientation]);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleAllowedOrientations(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil)
	MCExecContext ctxt(ep);

	intset_t t_orientations;
	MCOrientationGetAllowedOrientations(ctxt, t_orientations);

	bool t_success;
	t_success = true;
	
	MCAutoListRef t_orientations;

	if (t_success)
		t_success = MCListCreateMutable(EC_COMMA, &t_orientations);

	for (uint32_t j = 0; s_orientation_names[j] != nil; j++)
	{
		if (r_orientations & (1 << j)) != 0)
		{		
			MCAutoStringRef t_orientation;
			if (t_success)
				t_success = MCStringFormat(&t_orientation, "%s", s_orientation_names[j]);

			if (t_success)
				t_success = MCListAppend(*t_orientations, *t_orientation);
		}
	}

	MCAutoStringRef t_result;
	if (t_success)
		t_success = MCListCopyAsString(*t_orientations, &t_result);

	if (t_success)
	{
		ctxt . SetTheResultToValue(*t_result);
		return;
	}
	else 
		ctxt . Throw();

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleSetAllowedOrientations(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil)
	MCExecContext ctxt(ep);

	MCAutoStringRef t_orientations;
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		/* UNCHECKED */ ep . copyasstringref(&t_orientations);
	}

	char **t_orientations_array;
	uint32_t t_orientations_count;
	t_orientations_array = nil;
	t_orientations_count = 0;
	if (t_success)
		t_success = MCCStringSplit(MCStringGetCString(t_orientations), ',', t_orientations_array, t_orientations_count);
	
	intset_t_t t_orientations_set;
	t_orientations_set = 0;
	if (t_success)
		for(uint32_t i = 0; i < t_orientations_count; i++)
			for(uint32_t j = 0; get_orientation_name(j) != nil; j++)
				if (MCCStringEqualCaseless(t_orientations_array[i], s_orientation_names[j]))
					t_orientations_set |= (1 << j);
	
	for(uint32_t i = 0; i < t_orientations_count; i++)
		MCCStringFree(t_orientations_array[i]);
	MCMemoryDeleteArray(t_orientations_array);

	MCOrientationSetAllowedOrientations(ctxt, t_orientations_set);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleOrientationLocked(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil)
	MCExecContext ctxt(ep);

	bool t_locked;
	MCOrientationGetOrientationLocked(ctxt, t_locked);

	ctxt . SetTheResultToValue(t_locked);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleLockOrientation(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil)
	MCExecContext ctxt(ep);

	MCOrientationLockOrientation(ctxt);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleUnlockOrientation(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil)
	MCExecContext ctxt(ep);

	MCOrientationUnlockOrientation(ctxt);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleRevMail(void *context, MCParameter *p_parameters)
{
	MCAutoStringRef t_address, t_cc_address, t_subject, t_message_body;

	MCExecPoint ep(nil, nil, nil);
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		/* UNCHECKED */ ep . copyasstringref(&t_address);
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		/* UNCHECKED */ ep . copyasstringref(&t_cc_address);
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		/* UNCHECKED */ ep . copyasstringref(&t_subject);
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		/* UNCHECKED */ ep . copyasstringref(&t_message_body);
		p_parameters = p_parameters -> getnext();
	}
	
	MCExecContext ctxt(ep);

	MCMailExecSendEmail(t_address, t_cc_address, t_subject, t_message_body);
	
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ctxt . Catch(line, pos)
}

Exec_stat MCHandleComposeMail(MCMailType p_type, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_to, t_cc, t_bcc, t_subject, t_body;
	MCAutoArrayRef t_attachments;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &t_subject, &t_to, &t_cc, &t_bcc, &t_body, &t_attachments);

	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	if (t_success)
		MCMailExecComposeMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments, p_type);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ctxt . Catch(line, pos);
}

Exec_stat MCHandleComposePlainMail(void *context, MCParameter *p_parameters)
{
	return MCHandleComposeMail(kMCMailTypePlain, p_parameters);
}

Exec_stat MCHandleComposeUnicodeMail(void *context, MCParameter *p_parameters)
{
	return MCHandleComposeMail(kMCMailTypeUnicode, p_parameters);
}

Exec_stat MCHandleComposeHtmlMail(void *context, MCParameter *p_parameters)
{
	return MCHandleComposeMail(kMCMailTypeHtml, p_parameters);
}

Exec_stat MCHandleCanSendMail(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	bool t_can_send;

	MCMailGetCanSendMail(ctxt, t_can_send);

	ctxt . SetTheResultToValue(t_can_send);
}