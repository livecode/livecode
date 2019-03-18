/* Copyright (C) 2018 LiveCode Ltd.
 
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
import java.lang.*;
import java.nio.channels.FileChannel;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.text.*;
import android.os.ParcelFileDescriptor;
import android.net.Uri;
import android.app.*;
import android.util.Log;
import android.content.res.AssetFileDescriptor;
import android.database.Cursor;
import android.database.MatrixCursor;

public class FileProvider
{
	public static final String NAME = "_display_name";
	public static final String MIME_TYPE = "_mime_type";
	public static final String SIZE = "_size";
	public static final String TEMPORARY = "_tmp";
	public static final String FILE = "_file";

	private static final String PRIVATE_FILEMODE = "_private_filemode";

	// list of fields available via query - public fields only
	private static final String[] COLS = {NAME, MIME_TYPE, SIZE, TEMPORARY, FILE};

	private HashMap<String, HashMap<String, String> > m_infos = null;
	private android.content.Context m_context;
	private String m_content_uri_base;

	private FileProvider(android.content.Context p_context)
	{
		m_infos = new HashMap<String, HashMap<String, String> >();
		m_context = p_context;
		m_content_uri_base = "content://" + p_context.getPackageName() + ".fileprovider/share";
	}

	private static FileProvider s_provider = null;
	public static FileProvider getProvider(android.content.Context p_context)
	{
		if (s_provider == null)
			s_provider = new FileProvider(p_context);
		return s_provider;
	}

	private boolean isAPK(String p_path)
	{
		return p_path.startsWith(m_context . getPackageCodePath());
	}

	private String getAPKPath(String p_file_path)
	{
		int p_index = m_context.getPackageCodePath().length();
		if (p_file_path.charAt(p_index) == '/')
			p_index++;
		return p_file_path.substring(p_index);
	}

	private String getFilePath(String p_path)
	{
		if (isAPK(p_path))
			return getAPKPath(p_path);
		else
			return p_path;
	}

	private String getUriStringForFilePath(String p_path)
	{
        // If p_path begins with slash, then the resulting URI will be like e.g. :
        
        // content://<app_id>.fileprovider//storage/emulated/0/Android/data/<app_id>/cache/..
        //
        // This is not a problem in general, but in _some_ devices (e.g. Huawei nova e3) this URI is
        // normalized to:
        //
        // content://<app_id>.fileprovider/storage/emulated/0/Android/data/<app_id>/cache/..
        //
        // This results in the URI not being found. So use .substring(1) to remove the first slash, if any
        if (p_path.charAt(0) == '/')
            p_path = p_path.substring(1);
		return m_content_uri_base + "/" + p_path;
	}

	private boolean validField(String p_field)
	{
		for (final String p_col : COLS)
			if (p_col.equals(p_field))
				return true;
		return false;
	}

	private boolean validQuery(String[] projection)
	{
		boolean t_valid = true;
		for (int i = 0; i < projection . length && t_valid; ++i)
		{
			if (validField(projection[i]))
				continue;

			Log.i("revandroid", "incorrect projection: " + projection[i]);
			t_valid = false;
		}

		return t_valid;
	}

	// Open a file
	public ParcelFileDescriptor doOpenFile(Uri uri)
		throws FileNotFoundException
	{
		// Log.i("revandroid", "doOpenFile(" + uri.toString());
		String t_uri_string = uri.toString();

		ParcelFileDescriptor pfd;
		try
		{
			if (!m_infos.containsKey(t_uri_string))
				throw new IOException();

			String t_file_path;
			t_file_path = m_infos.get(t_uri_string).get(this.FILE);

			int t_mode;
			t_mode = Integer.valueOf(m_infos.get(t_uri_string).get(this.PRIVATE_FILEMODE));

			File t_file = new File(t_file_path);
			pfd = ParcelFileDescriptor.open(t_file, t_mode);

			return pfd;
		}
		catch(IOException e)
		{
			Log.i("revandroid", "Failed to open content file: " + e.getMessage());
			throw new FileNotFoundException("unable to find file " + t_uri_string);
		}
	}

	// Fetch the information stored for the asked file
	public Cursor doQuery(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder)
	{
		// SN-2014-05-15 [[ Bug 11895 ]] mobileComposeMail missing attachment in Android (Android Mail)
		// Some versions of Email ask for the name and the type in two different queries, not a single one...
		if (!validQuery(projection))
		{
			Log.i("revandroid", "Unexpected projection:");
			for (int i = 0; i < projection . length; ++i)
				Log.i("revandroid", String.format("\t\tprojection[%d]: %s", i, projection[i]));

			return null;
		}

		String t_uri_string = uri.toString();
		HashMap<String, String> t_infos = m_infos.get(t_uri_string);

		String t_values[] = new String[projection.length];
		for (int i = 0; i < projection.length; ++i)
			t_values[i] = t_infos.get(projection[i]);
		// Log.i("revandroid", "query for " + uri.toString());
		// for (int i = 0; i < t_values . length; ++i)
		// 	Log.i("revandroid", "\t" + t_values[i]);

		MatrixCursor t_cursor = new MatrixCursor(projection);
		t_cursor . addRow(t_values);

		return t_cursor;
	}

	public Uri addPath(String p_name, String p_path, String p_mime_type, boolean p_temporary, int p_filemode)
	{
		try
		{
			String t_file_path;
			Long t_size;
			// get the file descriptor to have the correct size.
			if (isAPK(p_path))
			{
				// SN-2014-05-09 [[ Bug 12418 ]]
				// Since AssetFileDescriptor.getParcelFileDescriptor() returns a parcel file descriptor for the whole asset file,
				// and not only the one targeted when, we can only read the bytes from the location, it's impossible to get a file descriptor
				// Thus, creating a copy of the file in the application cache folder is inevitable.

				AssetFileDescriptor afd = m_context.getAssets().openFd(getAPKPath(p_path));
				t_size = afd.getLength();

				FileInputStream fis = afd.createInputStream();
				byte[] t_bytes = new byte[t_size.intValue()];

				fis.read(t_bytes, 0, t_size.intValue());

				// Copy the file data at the temporary destination - same way it's done by Email
				File t_tempfile;
				t_tempfile = File.createTempFile("eml", p_name, m_context . getCacheDir());

				FileOutputStream t_out = new FileOutputStream(t_tempfile);
				t_out.write(t_bytes);
				t_out.close();

				t_file_path = t_tempfile.getPath();
				afd.close();

				// Mark this file as temporary
				p_temporary = true;
			}
			else
			{
				File t_file = new File(p_path);
				t_size = t_file.length();
				t_file_path = p_path;
			}

			HashMap<String, String> t_infos = new HashMap<String, String>();

			t_infos.put(this.NAME, p_name);
			t_infos.put(this.SIZE, Long.toString(t_size));
			t_infos.put(this.MIME_TYPE, p_mime_type);
			t_infos.put(this.TEMPORARY, Boolean.toString(p_temporary));
			t_infos.put(this.FILE, t_file_path);
			t_infos.put(this.PRIVATE_FILEMODE, Integer.toString(p_filemode));

			m_infos.put(getUriStringForFilePath(p_path), t_infos);
		}
		catch (FileNotFoundException e)
		{
			Log.d("revandroid", String.format("File not found (%s)", e.getMessage()));
			return null;
		}
		catch (IOException e)
		{
			Log.d("revandroid", String.format("IOException (&s)", e.getMessage()));
			return null;
		}

		Uri t_uri = Uri.parse(getUriStringForFilePath(p_path));
		return t_uri;
	}

	public void removePath(String p_path)
	{
		String t_uri_string = getUriStringForFilePath(p_path);
		if (!m_infos.containsKey(t_uri_string))
			return;

		HashMap<String, String> t_info = m_infos.get(t_uri_string);
		m_infos.remove(t_uri_string);

		boolean t_temporary = Boolean.valueOf(t_info.get(this.TEMPORARY));

		if (t_temporary)
		{
			// Delete temporary file
			File t_file = new File(t_info.get(this.FILE));
			t_file.delete();
		}
	}

	// Don't allow external queries to insert file entries
	public Uri doInsert (Uri uri, ContentValues p_values)
	{
		throw new UnsupportedOperationException("No external inserts");
	}

	// Don't allow external queries to update file entries
	public int doUpdate (Uri uri, ContentValues values, String selection, String[] selectionArgs)
	{
		throw new UnsupportedOperationException("No external updates");
	}

	// Don't allow external queries to delete file entries
	public int doDelete (Uri uri, String selection, String[] selectionArgs)
	{
		throw new UnsupportedOperationException("No external deletions");
	}

	public String doGetType (Uri uri)
	{
		String t_uri_string = uri.toString();

		if (m_infos.containsKey(t_uri_string))
			return m_infos.get(t_uri_string).get(this.MIME_TYPE);
		else
			return null;
	}
}
