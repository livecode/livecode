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

package com.runrev.android;

import java.io.*;
import java.util.*;
import android.content.*;
import android.net.*;
import android.text.*;
import android.util.*;

class Email
{
	private String[] m_email;
	private String[] m_cc;
	private String[] m_bcc;
	private String m_subject;
	private Spanned m_message;
	
	private ArrayList<Uri> m_attachment_uris;
	
	private String m_mime_type;
    private String m_provider_uri;
	
	public Email(String address, String cc, String bcc, String subject, String message_body, String p_provider_authority, boolean is_html)
	{
		if (address != null && address.length() > 0)
			m_email = address.trim().split(" *, *");
		if (cc != null && cc.length() > 0)
			m_cc = cc.trim().split(" *, *");
		if (bcc != null && bcc.length() > 0)
			m_bcc = bcc.trim().split(" *, *");
		if (subject != null && subject.length() > 0)
			m_subject = subject;

		if (message_body != null && message_body.length() > 0)
		{
			if (is_html)
			{
				m_message = Html.fromHtml(message_body);
				m_mime_type = "text/html";
			}
			else
			{
				m_message = new SpannedString(message_body);
				m_mime_type = "text/plain";
			}
		}
        
        m_provider_uri = "content://" + p_provider_authority;
	}
	
	private static String getMimeCategory(String mime_type)
	{
		if (mime_type == null)
			return "*/*";
		
		return mime_type.substring(0, mime_type.lastIndexOf('/'));
	}
	
	private String combineMimeTypes(String type_a, String type_b)
	{
		if (type_a == null)
			return type_b;
		if (type_b == null)
			return type_a;
		if (type_a.equals(type_b))
			return type_a;
		
		String cat_a = getMimeCategory(type_a);
		String cat_b = getMimeCategory(type_b);
		
		if (cat_a.equals(cat_b))
			return cat_a + "/*";
		else
            // IM 2012-01-20 change the mimetype for multiple types to */* as this seems to be more compatible with email apps
			return "*/*"; //"multipart/mixed";
	}
	
	private boolean addAttachment(Uri uri, String mime_type, String name)
	{
//        Log.i("revandroid", "addAttachment: " + uri.toString());
		if (m_attachment_uris == null)
			m_attachment_uris = new ArrayList<Uri>();
                
        m_attachment_uris.add(uri);
        
		m_mime_type = combineMimeTypes(m_mime_type, mime_type);
		return true;
	}
    
    private ContentValues createAttachmentValues(String p_name, String p_mime_type, boolean p_temporary)
    {        
        ContentValues t_values = new ContentValues(2);
        t_values . put(AttachmentProvider.NAME, p_name);
        t_values . put(AttachmentProvider.MIME_TYPE, p_mime_type);
        t_values . put(AttachmentProvider.TEMPORARY, p_temporary);
        
        return t_values;
    }
	
	public boolean addAttachment(Context p_context, String path, String mime_type, String name)
	{
//        Log.i("revandroid", String.format("addAttachment string attachment %s", m_provider_uri + "/" + path));
        File t_file = new File(path);
        
        // SN-2014-02-03: [[ Bug 11069 ]] Generate a URI leading to the AttachmentProvider
        Uri t_uri = Uri.parse(m_provider_uri + "/" + path);
        ContentValues t_values = createAttachmentValues(name, mime_type, false);
                
        p_context . getContentResolver() . insert(t_uri, t_values);
        
		return addAttachment(t_uri, mime_type, name);
	}
	
	public boolean addAttachment(Context p_context, byte[] data, String mime_type, String name)
	{
		try
		{
			File t_tempfile;
            // SN-2014-02-04: [[ Bug 11069 ]] Temporary files are created in the cache directory
            t_tempfile = File.createTempFile("eml", name, p_context . getCacheDir());
			
			FileOutputStream t_out = new FileOutputStream(t_tempfile);
			t_out.write(data);
			t_out.close();
						            
            // SN-2014-02-03: [[ Bug 11069 ]] Generate a URI leading to AttachmentProvider
            Uri t_uri = Uri.parse(m_provider_uri + "/" + t_tempfile.getPath());
                        
            ContentValues t_values = createAttachmentValues(name, mime_type, true);
            p_context . getContentResolver() . insert(t_uri, t_values);
            			
			return addAttachment(t_uri, mime_type, name);
		}
		catch (Exception e)
		{
//            Log.i("revandroid", "creatTempFile exception: " + e.getMessage());
			return false;
		}
	}
	
	public Intent createIntent()
	{
		Intent t_mail_intent = new Intent(Intent.ACTION_SEND_MULTIPLE);
		if (m_email != null)
		{
			t_mail_intent.putExtra(Intent.EXTRA_EMAIL, m_email);
		}
		if (m_cc != null)
		{
			t_mail_intent.putExtra(Intent.EXTRA_CC, m_cc);
		}
		if (m_bcc != null)
		{
			t_mail_intent.putExtra(Intent.EXTRA_BCC, m_bcc);
		}
		if (m_subject != null)
		{
			t_mail_intent.putExtra(Intent.EXTRA_SUBJECT, m_subject);
		}
		
		if (m_message != null)
		{
			t_mail_intent.putExtra(Intent.EXTRA_TEXT, m_message);
		}
		
		if (m_attachment_uris != null)
		{
			t_mail_intent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, m_attachment_uris);
		}
		if (m_mime_type != null)
			t_mail_intent.setType(m_mime_type);
		else
			t_mail_intent.setType("text/plain");
        
        // SN-2014-01-28: [[ Bug 11069 ]] mobileComposeMail attachment missing in Android
        // This permission of reading must be added to the Intent to allow it to read the file
        t_mail_intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        
		return t_mail_intent;
	}
	
	public void cleanupAttachments(ContentResolver p_resolver)
	{		
		try
		{
            for (Uri t_uri : m_attachment_uris)
            {
                p_resolver . delete(t_uri, AttachmentProvider.FILE, null);
            }
		}
		catch (Exception e)
		{
		}
	}
}
