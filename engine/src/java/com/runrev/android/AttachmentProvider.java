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
    public static final String TEMPORARY = "_tmp";
    public static final String FILE = "_file";
    
    private HashMap<String, ArrayList<String> > m_infos = null;
    private HashMap<String, Integer> m_references = null;
    
    private void release(String p_filename, String p_reference_key, boolean p_is_apk)
    {
//        Log.i("revandroid", String.format("releasing stored path '%s'", p_filename));
        int t_refs = m_references . get(p_reference_key);
        File t_file = new File(p_filename);
        
        if (t_refs == 1)
        {
            // Check whether the file is a temporary file
//            if (!p_is_apk && m_infos . get(p_reference_key).get(3).equals("true"))
//            {
//                // The temporary file should be deleted, but it's actually impossible to allow
//                // this step with the Email application, since it sends the Activity result,
//                // open the file to check whether it exists, and reopen it before sending the
//                // the email - so there is no way to keep an open file descriptor up to the sending
//                // of the email.
//                t_file . delete();
//                
//                Log.i("revandroid", String.format("%s is deleted", p_filename));
//            }
//            else
//                Log.i("revandroid", String.format("%s isn't deleted - not a temporary file", p_filename));
            
            // Remove entry from the hashtables
            m_references.remove(p_reference_key);
            m_infos.remove(p_reference_key);
        }
        else
        {
            m_references . put (p_reference_key, t_refs - 1);
        }
    }
    
    private boolean isAPK(Uri p_uri)
    {
        return p_uri . toString().contains(getContext().getPackageCodePath());
    }
    
    private String getFilePath(Uri p_uri)
    {
        if (isAPK(p_uri))
            return p_uri . getLastPathSegment();
        else
            return p_uri.getPath().substring(1, p_uri.getPath().length());
    }
    
    // Open a file
    private ParcelFileDescriptor doOpenFile(Uri uri, boolean p_cleanup_file)
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
                AssetFileDescriptor afd = getContext().getAssets().openFd(t_filepath);
                pfd = afd.getParcelFileDescriptor();
            }
            else
            {
                File t_file = new File(t_filepath);
                pfd = ParcelFileDescriptor.open(t_file, ParcelFileDescriptor.MODE_READ_ONLY);                
            }
            
            if (p_cleanup_file)
                release(t_filepath, t_path, t_apk);
            
            return pfd;
        }
        catch(IOException e)
        {
            Log.i("revandroid", "Failed to open attachment: " + e.getMessage());
            throw new FileNotFoundException("unable to find file " + t_filepath);
        }
    }
    
    @Override
    public boolean onCreate()
    {
        if (m_infos == null)
            m_infos = new HashMap<String, ArrayList<String> >();
        
        if (m_references == null)
            m_references = new HashMap<String, Integer>();
        
        return true;
    }
    
    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode)
        throws FileNotFoundException
    {
//        Log.i("revandroid", "openFile " + uri.toString());
        if (!uri.toString().contains(this.URI))
            throw new FileNotFoundException("invalid URI given");
        
        try
        {
            boolean t_release = m_references . containsKey(uri.getPath());
            
            return doOpenFile(uri, t_release);
        }
        catch (FileNotFoundException e)
        {
            throw e;
        }
    }
        
    // Fetch the information stored for the asked email
    @Override
    public Cursor query (Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder)
    {
        if (!uri.toString().contains(this.URI) ||
                projection . length < 2 ||
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
    @Override
    public Uri insert (Uri uri, ContentValues p_values)
    {
        if (!uri.toString().contains(this.URI))
            return uri;
        
        String t_path = uri.getPath();
                
        if (m_infos.containsKey(t_path))
        {
//            Log.i("revandroid", String.format("contains %s", t_path));
//            int t_ref = m_references.get(t_path);
//            m_references.put(t_path, t_ref + 1);
            return uri;
        }
        
        if (p_values . containsKey(this.NAME) && p_values . containsKey(this.TEMPORARY) && p_values . containsKey(this.MIME_TYPE))
        {
            ArrayList<String> t_infos = new ArrayList<String>(2);
                        
            // get the file descriptor to have the correct size.
            ParcelFileDescriptor t_file;
            try
            {
                t_file = doOpenFile(uri, false);
                
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
    
    @Override
    public int update (Uri uri, ContentValues values, String selection, String[] selectionArgs)
    {
//        Log.i("revandroid", "AttachmentProvider::update " + uri.toString());
        return 0;
    }
    
    @Override
    public int delete (Uri uri, String selection, String[] selectionArgs)
    {        
        if (selection == this.FILE)
        {
            String t_key =  uri.getPath();
            String t_filepath = getFilePath(uri);
            
            if (m_references . containsKey(t_key))
                m_references . put(t_key, m_references.get(t_key) + 1);
            else
                m_references . put(t_key, 1);
            
            return 1;
        }
        else
            return 0;
    }
    
    @Override
    public String getType (Uri uri)
    {
        String t_path = uri.getPath();
        
        if (m_infos . containsKey(t_path))
            return m_infos . get(t_path) . get(2);
        else
            return null;
    }
}
