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
    
    private boolean validQuery(String[] projection)
    {
        boolean t_valid = true;
        for (int i = 0; i < projection . length && t_valid; ++i)
        {
            if (projection[i].equals(this.NAME)
                    || projection[i].equals(this.MIME_TYPE)
                    || projection[i].equals(this.SIZE)
                    || projection[i].equals(this.TEMPORARY)
                    || projection[i].equals(this.FILE))
                continue;
            
            Log.i("revandroid", "incorrect projection: " + projection[i]);
            t_valid = false;
        }
        
        return t_valid;
    }
    
    private int projectionToInfoIndex(String projection)
    {
        if (projection.equals(this.NAME))
            return 0;
        else if (projection.equals(this.MIME_TYPE))
            return 1;
        else if (projection.equals(this.SIZE))
            return 2;
        else if (projection.equals(this.TEMPORARY))
            return 3;
        else if (projection.equals(this.FILE))
            return 4;
        
        return -1;
    }
    
    // Open a file
    public ParcelFileDescriptor doOpenFile(Uri uri)
        throws FileNotFoundException
    {
//        Log.i("revandroid", "doOpenFile(" + uri.toString());
        // Check whether we are asked for a file from the APK
        String t_filepath = getFilePath(uri);
        String t_path = uri.getPath();
                
        ParcelFileDescriptor pfd;            
        try
        {
            String t_accessible_filepath;
            if (isAPK(uri))
            {                
                // SN-2014-05-09 [[ Bug 12418 ]]
                // We should already have stored the cached file path in case
                // the file is in the APK
                if (m_infos.containsKey(t_path))
                    t_accessible_filepath = m_infos.get(t_path).get(4);
                else
                    throw new IOException();
            }
            else
                t_accessible_filepath = t_filepath;
                
            File t_file = new File(t_accessible_filepath);
            pfd = ParcelFileDescriptor.open(t_file, ParcelFileDescriptor.MODE_READ_ONLY);                
            
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
        // SN-2014-05-15 [[ Bug 11895 ]] mobileComposeMail missing attachment in Android (Android Mail)
        // Some versions of Email ask for the name and the type in two different queries, not a single one...
        if (!validQuery(projection))
        {
            Log.i("revandroid", "Unexpected projection:");            
            for (int i = 0; i < projection . length; ++i)
                Log.i("revandroid", String.format("\t\tprojection[%d]: %s", i, projection[i]));
            
            return null;
        }
        
        String t_path = uri.getPath();
        ArrayList<String> t_infos = m_infos.get(t_path);
        
        String t_values[] = new String[projection.length];        
        for (int i = 0; i < projection.length; ++i)
            t_values[i] = t_infos.get(projectionToInfoIndex(projection[i]));
//        Log.i("revandroid", "query for " + uri.toString());
//        
//        for (int i = 0; i < t_values . length; ++i)
//            Log.i("revandroid", "\t" + t_values[i]);
        
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
            ArrayList<String> t_infos = new ArrayList<String>(5);
                        
            try
            {
                // get the file descriptor to have the correct size.
                Long t_size;
                String t_cached_file;
                if (isAPK(uri))
                {
                    // SN-2014-05-09 [[ Bug 12418 ]]
                    // Since AssetFileDescriptor.getParcelFileDescriptor() returns a parcel file descriptor for the whole asset file,
                    // and not only the one targeted when, we can only read the bytes from the location, it's impossible to get a file descriptor
                    // Thus, creating a copy of the file in the application cache folder is inevitable.
                    
                    AssetFileDescriptor afd = m_context.getAssets().openFd(getFilePath(uri));
                    t_size = afd.getLength();
                    
                    FileInputStream fis = afd.createInputStream();
                    byte[] t_bytes = new byte[t_size.intValue()];
                    
                    fis.read(t_bytes, 0, t_size.intValue());
                    
                    // Copy the file data at the temporary destination - same way it's done by Email
                    File t_tempfile;
                    t_tempfile = File.createTempFile("eml", p_values . getAsString(this.NAME), m_context . getCacheDir());
                    
                    FileOutputStream t_out = new FileOutputStream(t_tempfile);
                    t_out.write(t_bytes);
                    t_out.close();
                    
                    t_cached_file = t_tempfile.getPath();                    
                    afd.close();
                }
                else
                {
                    ParcelFileDescriptor pfd = doOpenFile(uri);
                    t_size = pfd.getStatSize();
                    pfd . close();
                    t_cached_file = "";
                }
                
                t_infos . add(p_values . getAsString(this.NAME));
                t_infos . add(Long.toString(t_size));
                t_infos . add(p_values . getAsString(this.MIME_TYPE));
                t_infos . add(p_values . getAsBoolean(this.TEMPORARY).toString());
                t_infos . add(t_cached_file);
                
//                Log.i("revandroid", String.format("insert (\n\tname: %s, \n\tsize: %d, \n\tMIME: %s, \n\ttemp: %s, \n\tCache: %s)",
//                                                  p_values . getAsString(this.NAME),
//                                                  t_size,
//                                                  p_values . getAsString(this.MIME_TYPE),
//                                                  p_values . getAsBoolean(this.TEMPORARY).toString(),
//                                                  t_cached_file));
                
                m_infos . put(t_path, t_infos);
            }
            catch (FileNotFoundException e)
            {
                Log.d("revandroid", String.format("File not found (%s)", e.getMessage()));
                return uri;
            }
            catch (IOException e)
            {
                Log.d("revandroid", String.format("IOException (&s)", e.getMessage()));
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
