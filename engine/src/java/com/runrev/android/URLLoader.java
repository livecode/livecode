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

import java.net.*;
import javax.net.ssl.*;
import java.security.*;
import java.security.cert.*;
import java.io.*;

import android.util.*;

// url download/upload class

abstract class URLLoader implements Runnable
{
	private URL m_url;
	private URLConnection m_connection;
	private int m_id;
	
	boolean m_is_http;

	private byte[] m_upload_data;
	
	private byte[] m_bytes;
	private int m_byte_count;
	
	private static final int BUFFER_SIZE = 65536; 
	private byte[] m_buffer;
	private int m_buffer_frontier;
	
	private String m_err_string;
	private int m_content_length;
	
	public URLLoader(int p_id, String p_url) throws URISyntaxException, MalformedURLException, IOException
	{
		Log.i("revandroid", "URLLoader " + p_url);
		m_id = p_id;
		
		URI t_uri = new URI(p_url);
			
		m_url = t_uri.toURL();
		
		m_connection = m_url.openConnection();

		m_is_http = m_url.getProtocol().equals("http") || m_url.getProtocol().equals("https");

		if (m_is_http)
		{
			HttpURLConnection t_http_connection = (HttpURLConnection)m_connection;
			t_http_connection.setFollowRedirects(true);
		}
		
		// MM-2012-01-13: Added support for HTTP basic authentication.
		if (m_is_http)
		{
			String t_user_info = m_url.getUserInfo();
			if (t_user_info != null)
				m_connection.setRequestProperty("Authorization", "Basic " + Base64.encodeToString(t_user_info.getBytes(), Base64.DEFAULT));
		}		
	}
	
	public void setTimeout(int p_timeout)
	{
		m_connection.setConnectTimeout(p_timeout);
		m_connection.setReadTimeout(p_timeout);
	}
	
	// MW-2013-10-02: [[ MobileSSLVerify ]] Configure whether to verify SSL connections.
	public void setSSLVerification(boolean p_verify_ssl)
	{
		// If we aren't HTTP do nothing.
		if (!m_is_http)
			return;
		
		// If we aren't HTTPS do nothing.
		if (!(m_connection instanceof HttpsURLConnection))
			return;
		
		// If we are to verify SSL leave things as they are.
		if (p_verify_ssl)
			return;
		
		// Install a dummy TrustManager which accepts all certs.
		try
		{
			SSLContext t_context;
			t_context = SSLContext.getInstance("TLS");
			t_context . init(null,
							 new X509TrustManager[] {
								 new X509TrustManager() {
									 public X509Certificate[] getAcceptedIssuers() { return null; }
									 public void checkClientTrusted(X509Certificate[] certs, String auth) {}
									 public void checkServerTrusted(X509Certificate[] certs, String auth) {} }
							 },
							 null);
			((HttpsURLConnection)m_connection) . setSSLSocketFactory(t_context . getSocketFactory());
		}
		catch(NoSuchAlgorithmException e)
		{
		}
		catch(KeyManagementException e)
		{
		}
		
		// Install a dummy hostname verified which accepts all hosts.
		((HttpsURLConnection)m_connection) . setHostnameVerifier(new HostnameVerifier() {
			public boolean verify(String hostname, SSLSession session)
			{
				return true;
			}
		});
	}
	
	public void setMethod(String p_method)
	{
		if (p_method.equals("POST"))
		{
			m_connection.setDoInput(true);
			m_connection.setDoOutput(true);
		}
        else if (p_method.equals("PUT"))
        {
            m_connection.setDoInput(false);
            m_connection.setDoOutput(true);
        }
		if (m_is_http)
		{
			try
			{
				((HttpURLConnection)m_connection).setRequestMethod(p_method);
			}
			catch (ProtocolException e)
			{
				return;
			}
		}
	}
	
	public void setHeaders(String p_headers)
	{
		if (m_is_http)
		{
			if (p_headers != null && p_headers.length() > 0)
			{
				for (String t_header : p_headers.split("\n"))
				{
					int t_index = t_header.indexOf(':');
					if (t_index != -1)
					{
						String t_key = t_header.substring(0, t_index).trim();
						String t_value = t_header.substring(t_index + 1).trim();
						m_connection.setRequestProperty(t_key, t_value);
					}
				}
			}
		}
	}
	
	public void setUploadData(byte[] p_upload_data)
	{
		if (p_upload_data != null && p_upload_data.length > 0)
		{
			m_upload_data = p_upload_data;
		}
	}
	
	public void run()
	{
		// Allocate the buffer
		m_buffer = new byte[BUFFER_SIZE];
		m_buffer_frontier = 0;
		
		// fetch url
		try
		{
			if (m_upload_data != null && m_is_http)
				((HttpURLConnection)m_connection).setFixedLengthStreamingMode(m_upload_data.length);

			m_connection.connect();
			// notify engine of start
			onURLStart();

			if (m_upload_data != null)
			{
                int t_uploaded_bytes = 0;
				OutputStream t_url_output = m_connection.getOutputStream();
                while (t_uploaded_bytes < m_upload_data.length)
                {
                    int t_to_write = Math.min(128 * 1024, m_upload_data.length - t_uploaded_bytes);
                    t_url_output.write(m_upload_data, t_uploaded_bytes, t_to_write);
                    t_uploaded_bytes += t_to_write;
                    onURLUploadData(t_uploaded_bytes);
                }
			}

            boolean t_http_status_error = false;
            int t_http_status_code = 0;
            
			// if http(s), check response code
			if (m_is_http)
			{
				t_http_status_code = ((HttpURLConnection)m_connection).getResponseCode();
				if (t_http_status_code >= 400)
				{
                    t_http_status_error = true;
					m_err_string = String.valueOf(t_http_status_code) + " " + ((HttpURLConnection)m_connection).getResponseMessage();
				}
			}
            
            if (m_connection.getDoInput())
            {
                m_content_length = m_connection.getContentLength();

                // notify engine of connect
                onURLConnect();
                
                InputStream t_url_input;
                if (t_http_status_error)
                    t_url_input = ((HttpURLConnection)m_connection).getErrorStream();
                else
                    t_url_input = m_connection.getInputStream();
                
                // This is the next point in the buffer to read data into. It starts at
                // zero.
                int t_next_buffer_frontier;
                t_next_buffer_frontier = 0;
                
                for(;;)
                {
                    // Try to fill the remaining space in the buffer with data from the
                    // fetch. Notice we use 'next buffer frontier' rather than the instance
                    // variable as m_buffer_frontier must only be accessed when synced.
                    int t_bytes_read;
                    t_bytes_read = t_url_input . read(m_buffer, t_next_buffer_frontier, BUFFER_SIZE - t_next_buffer_frontier);
                    if (t_bytes_read == -1)
                        break;
                    
                    // Access to the buffer frontier index is synchronized, as it is used from
                    // both this thread and the main dispatching thread.
                    synchronized(this)
                    {
                        if (m_buffer_frontier != 0)
                        {
                            // The buffer contains data already and a notification has been made
                            // but not processed. So just update the frontier.
                            m_buffer_frontier += t_bytes_read;
                        }
                        else
                        {
                            // There is no pending processing of data, so copy down the data we
                            // have just read, mark as dirty and post a notification.
                            System.arraycopy(m_buffer, t_next_buffer_frontier, m_buffer, 0, t_bytes_read);
                            m_buffer_frontier = t_bytes_read;
                            onURLReceiveData();
                        }
                        
                        // At this point we wait if the buffer is full.
                        if (m_buffer_frontier == BUFFER_SIZE)
                            wait();
                        
                        // Now take the current value of the frontier as the next one to use
                        t_next_buffer_frontier = m_buffer_frontier;
                    }
                }
                
                t_url_input.close();

                // Notify engine of completed download
                if (t_http_status_error)
					onURLError();
                else
                    onURLComplete();
            }
		}
		catch (SocketTimeoutException e)
		{
			m_err_string = "timeout";
			onURLError();
		}
		catch (IOException e)
		{
			m_err_string = e.toString();
			onURLError();
		}
		catch (Exception e)
		{
			// This is IllegalMonitorException and InterruptedException...
		}
	}
	
	public synchronized byte[] popBytes()
	{
		// Copy the dirty portion of the buffer to return.
		byte[] t_bytes;
		t_bytes = new byte[m_buffer_frontier];
		System.arraycopy(m_buffer, 0, t_bytes, 0, m_buffer_frontier);

		// Notify the loader thread to wake up if needed.
		if (m_buffer_frontier == BUFFER_SIZE)
			notify();
		
		// Reset the frontier to 0
		m_buffer_frontier = 0;
		
		// Return the bytes
		return t_bytes;
	}
	
	public int getID() { return m_id; }	
	public int getContentLength() { return m_content_length; }
	public byte[] getBytes() { return m_bytes; }
	public int getByteCount() { return m_byte_count; }
	public String getErrorString() { return m_err_string; }
	
	public abstract void onURLStart();
	public abstract void onURLConnect();
	public abstract void onURLUploadData(int p_sent);
	public abstract void onURLReceiveData();
	public abstract void onURLComplete();
	public abstract void onURLError();
}
