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
	private ArrayList<File> m_temp_files;
	
	private String m_mime_type;
	
	public Email(String address, String cc, String bcc, String subject, String message_body, boolean is_html)
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
		if (m_attachment_uris == null)
			m_attachment_uris = new ArrayList<Uri>();
		
		m_attachment_uris.add(uri);
		m_mime_type = combineMimeTypes(m_mime_type, mime_type);
		return true;
	}
	
	public boolean addAttachment(String path, String mime_type, String name)
	{
		return addAttachment(Uri.fromFile(new File(path)), mime_type, name);
	}
	
	public boolean addAttachment(byte[] data, String mime_type, String name)
	{
		try
		{
			File t_tempfile;
			t_tempfile = File.createTempFile("eml", name);
			
			FileOutputStream t_out = new FileOutputStream(t_tempfile);
			t_out.write(data);
			t_out.close();
			
			if (m_temp_files == null)
				m_temp_files = new ArrayList<File>();
			
			m_temp_files.add(t_tempfile);
			
			return addAttachment(Uri.fromFile(t_tempfile), mime_type, name);
		}
		catch (Exception e)
		{
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
		return t_mail_intent;
	}
	
	public void cleanupTempFiles()
	{
		if (m_temp_files == null)
			return;
		
		try
		{
			for (File t_tempfile : m_temp_files)
			{
				t_tempfile.delete();
			}
		}
		catch (Exception e)
		{
		}
	}
}