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
import android.app.*;
import android.util.Log;
import android.content.res.AssetFileDescriptor;
import android.database.Cursor;
import android.database.MatrixCursor;

public class AttachmentProvider
{        
    public static final String NAME = "_display_name";
    public static final String MIME_TYPE = "_mime_type";
    public static final String SIZE = "_size";
    public static final String TEMPORARY = "_tmp";
    public static final String FILE = "_file";
    
    private HashMap<String, ArrayList<String> > m_infos = null;
    private android.content.Context m_context;    
    
    public AttachmentProvider(android.content.Context p_context)
    {
        if (m_infos == null)
            m_infos = new HashMap<String, ArrayList<String> >();
        
        m_context = p_context;
    }
     
    private boolean isAPK(Uri p_uri)
    {
        return p_uri . toString().contains(m_context . getPackageCodePath());
    }
    
    private String getFilePath(Uri p_uri)
    {
        if (isAPK(p_uri))
            return p_uri . getLastPathSegment();
        else
            return p_uri.getPath().substring(1, p_uri.getPath().length());
    }
    
    // Open a file
    public ParcelFileDescriptor doOpenFile(Uri uri)
        throws FileNotFoundException
    {
        // Check whether we are asked for a file from the APK
        String t_filepath = getFilePath(uri);
        String t_path = uri.getPath();
                
        ParcelFileDescriptor pfd;                
        try
        {
            boolean t_apk = isAPK(uri);
            if (t_apk)
            {
                AssetFileDescriptor afd = m_context.getAssets().openFd(t_filepath);
                pfd = afd.getParcelFileDescriptor();
            }
            else
            {
                File t_file = new File(t_filepath);
                pfd = ParcelFileDescriptor.open(t_file, ParcelFileDescriptor.MODE_READ_ONLY);                
            }
            
            return pfd;
        }
        catch(IOException e)
        {
            Log.i("revandroid", "Failed to open attachment: " + e.getMessage());
            throw new FileNotFoundException("unable to find file " + t_filepath);
        }
    }
        
    // Fetch the information stored for the asked email
    public Cursor doQuery(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder)
    {
          if (projection . length < 2 ||
                !projection[0].equals(this.NAME) ||
                !projection[1].equals(this.SIZE))
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
        
        return t_cursor;
    }
    
    // The insert method update the saved values for the email
    public Uri doInsert (Uri uri, ContentValues p_values)
    {        
        String t_path = uri.getPath();
//        Log.i("revandroid", "insert attachment at path: " + t_path);
        
        if (m_infos.containsKey(t_path))
        {
//            Log.i("revandroid", String.format("contains %s", t_path));
            return uri;
        }
        
        if (p_values . containsKey(this.NAME) && p_values . containsKey(this.TEMPORARY) && p_values . containsKey(this.MIME_TYPE))
        {
            ArrayList<String> t_infos = new ArrayList<String>(2);
                        
            // get the file descriptor to have the correct size.
            ParcelFileDescriptor t_file;
            try
            {
                t_file = doOpenFile(uri);
                
                t_infos . add(p_values . getAsString(this.NAME));
                t_infos . add(Long.toString(t_file . getStatSize()));
                t_infos . add(p_values . getAsString(this.MIME_TYPE));
                t_infos . add(p_values . getAsBoolean(this.TEMPORARY).toString());
                
//                Log.i("revandroid", String.format("insert (%s, %d, %s, %s)",
//                                                  p_values . getAsString(this.NAME),
//                                                  t_file . getStatSize(),
//                                                  p_values . getAsString(this.MIME_TYPE),
//                                                  p_values . getAsBoolean(this.TEMPORARY).toString()));
                
                t_file . close();
                
                m_infos . put(t_path, t_infos);
//                m_references . put(t_path, 1);
            }
            catch (FileNotFoundException e)
            {
                Log.d("revandroid", e.getMessage());
                return uri;
            }
            catch (IOException e)
            {
                Log.d("revandroid", e.getMessage());
                return uri;
            }
        }
        
        return uri;
    }
    
    public int doDelete (Uri uri, String selection, String[] selectionArgs)
    {
        return 0;
    }
    
    public String doGetType (Uri uri)
    {
        String t_path = uri.getPath();
        
        if (m_infos . containsKey(t_path))
            return m_infos . get(t_path) . get(2);
        else
            return null;
    }
}
