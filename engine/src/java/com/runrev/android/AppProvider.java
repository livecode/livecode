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
import android.content.*;
import android.net.Uri;
import android.app.*;
import android.database.*;
import java.io.*;
import android.util.Log;
import android.os.ParcelFileDescriptor;
import com.runrev.android.AttachmentProvider;

public class AppProvider extends ContentProvider
{
    public static final String URI = "content://com.runv.android.attachmentprovider";
    public static final String AUTHORITY = "com.runv.android.attachmentprovider";
    
    private com.runrev.android.AttachmentProvider m_attachments;
    
    @Override
    public boolean onCreate()
    {
        m_attachments = new AttachmentProvider(getContext());
        return true;
    }
    
    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode)
    throws FileNotFoundException
    {
        Log.i("revandroid", uri.toString());
        return m_attachments.doOpenFile(uri);
    }
    
    @Override
    public Cursor query (Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder)
    {
        Log.i("revandroid", "query: " + uri.toString());
        return m_attachments.doQuery(uri, projection, selection, selectionArgs, sortOrder);
    }
    
    @Override
    public Uri insert (Uri uri, ContentValues p_values)
    {
        return m_attachments.doInsert(uri, p_values);
    }
    
    @Override
    public int update (Uri uri, ContentValues values, String selection, String[] selectionArgs)
    {
        return 0;
    }
    
    @Override
    public int delete (Uri uri, String selection, String[] selectionArgs)
    {
        return m_attachments.doDelete(uri, selection, selectionArgs);
    }
    
    @Override
    public String getType (Uri uri)
    {
        return m_attachments.doGetType(uri);
    }
    
}
