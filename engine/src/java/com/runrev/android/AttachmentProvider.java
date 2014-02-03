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
    
    public static final String NAME = "_display_name";
    public static final String MIME_TYPE = "_mime_type";
    public static final String SIZE = "_size";
    
    private HashMap<String, ArrayList<String> > m_infos = null;
    
    
    @Override
    public boolean onCreate()
    {
        if (m_infos == null)
            m_infos = new HashMap<String, ArrayList<String> >();
        return true;
    }
    
    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode)
        throws FileNotFoundException
    {
//        Log.i("revandroid", "openFile " + uri.toString());
        if (!uri.toString().contains(this.URI))
            throw new FileNotFoundException("invalid URI given");
        
        // Check whether we are asked for a file from the APK
        if (uri.toString().contains(getContext().getPackageCodePath()))
        {
            String t_file_path = uri.getLastPathSegment();
//            Log.i("revandroid", "open APK file " + t_file_path);
            
            try
            {
                AssetFileDescriptor afd = getContext().getAssets().openFd(t_file_path);
                return afd.getParcelFileDescriptor();
            }
            catch(IOException e)
            {
//                Log.i("revandroid", "Failed to open: " + e.getMessage());
                throw new FileNotFoundException("unable to find file " + t_file_path);
            }
        }
        else
        {
            String t_path = uri.getPath().substring(1, uri.getPath().length());
            
//            Log.i("revandroid", "open file " + t_path);
            
            try
            {
                ParcelFileDescriptor pfd = ParcelFileDescriptor.open(new File(t_path), ParcelFileDescriptor.MODE_READ_ONLY);
                return pfd;
            }
            catch(Exception e)
            {
//                Log.i("revandroid", "Failed to open: " + e.getMessage());
                throw new FileNotFoundException("unable to find file " + t_path);
            }
        }
    }
        
    // Fetch the information stored for the asked email
    @Override
    public Cursor query (Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder)
    {
        if (!uri.toString().contains(this.URI) ||
                projection . length < 2 ||
                projection[0] != this.NAME ||
                projection[1] != this.SIZE)
            return null;
        
        String t_path = uri.getPath();
        ArrayList<String> t_infos = m_infos.get(t_path);
        
        String t_values[] = new String[2];
        t_values[0] = t_infos . get(0);
        t_values[1] = t_infos . get(1);
        
//        Log.i("revandroid", "query(" + uri.toString() + "): " +
//              t_values[0] + ", " +
//              t_values[1]);
        
        MatrixCursor t_cursor = new MatrixCursor(projection);
        t_cursor . addRow(t_values);
        
        m_infos . remove(t_path);
        
        return t_cursor;
    }
    
    // The insert method update the saved values for the email
    @Override
    public Uri insert (Uri uri, ContentValues p_values)
    {        
//        Log.i("revandroid", "AttachmentProvider::insert " + uri.toString());
        if (!uri.toString().contains(this.URI))
            return uri;
        
        String t_path = uri.getPath();
        if (m_infos.containsKey(t_path))
            return uri;
        
        if (p_values . containsKey(this.NAME) && p_values . containsKey(this.SIZE) && p_values . containsKey(this.MIME_TYPE))
        {
            ArrayList<String> t_infos = new ArrayList<String>(2);
            
//            Log.i("revandroid", "insert (" +
//                  p_values . getAsString(this.NAME) + "," +
//                  p_values . getAsLong(this.SIZE) + "," +
//                  p_values . getAsString(this.MIME_TYPE) + ")");
            t_infos . add(p_values . getAsString(this.NAME));
            t_infos . add(Long.toString(p_values . getAsLong(this.SIZE)));
            t_infos . add(p_values . getAsString(this.MIME_TYPE));
            
            m_infos . put(t_path, t_infos);
        }
        
        return uri;
    }
    
    @Override
    public int update (Uri uri, ContentValues values, String selection, String[] selectionArgs)
    {
//        Log.i("revandroid", "AttachmentProvider::update " + uri.toString());
        return 0;
    }
    
    @Override
    public int delete (Uri uri, String selection, String[] selectionArgs)
    {
//        Log.i("revandroid", "AttachmentProvider::delete " + uri.toString());
        return 0;
    }
    
    @Override
    public String getType (Uri uri)
    {
        String t_path = uri.getPath();
        if (m_infos . containsKey(t_path))
        {
//            Log.i("revandroid", "getType(" + uri.toString() + ") returns " + m_infos . get(t_path) . get(2));
            return m_infos . get(t_path) . get(2);
        }
        else
        {
//            Log.i("revandroid", "getType(" + uri.toString() + ") returns null");
            return null;
        }
    }
}
