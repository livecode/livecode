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
import java.lang.*;
import java.nio.channels.FileChannel;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.text.*;
import android.os.ParcelFileDescriptor;
import android.net.Uri;
import android.util.Log;
import android.content.res.AssetFileDescriptor;

import android.database.*;


public class AttachmentProvider extends ContentProvider
{    
    public static final String URI = "content://com.runrev.android.attachmentprovider";
    public static final String AUTHORITY = "com.runrev.android.attachmentprovider";
    
    
    @Override
    public boolean onCreate()
    {
        return true;
    }
    
    @Override
    public AssetFileDescriptor openAssetFile(Uri uri, String mode)
        throws FileNotFoundException
    {
        // Check whether we are really asked for a file from the APK
        if (!uri.toString().contains(getContext().getPackageCodePath()))
            throw new FileNotFoundException("Unable to open non-asset file");
        
        String t_file_path = uri.getLastPathSegment();
        
        try
        {
            AssetFileDescriptor afd = getContext().getAssets().openFd(t_file_path);
            return afd;
        }
        catch(IOException e)
        {
            Log.i("revandroid", "Failed to open: " + e.getMessage());
            throw new FileNotFoundException("unable to find file " + t_file_path);
        }
    }
        
    // Fetch the URI stored for the ask email
    @Override
    public Cursor query (Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder)
    {
        return null;
    }
    
    // The insert method update the saved values for the email
    @Override
    public Uri insert (Uri uri, ContentValues p_values)
    {
        // Does nothing
        return uri;
    }
    
    @Override
    public int update (Uri uri, ContentValues values, String selection, String[] selectionArgs)
    {
        // Does nothing
        return 0;
    }
    
    @Override
    public int delete (Uri uri, String selection, String[] selectionArgs)
    {
        // Does nothing
        return 0;
    }
    
    @Override
    public String getType (Uri uri)
    {
        // Does nothing
        return null;
    }
}
