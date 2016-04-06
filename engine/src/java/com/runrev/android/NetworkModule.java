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
import java.net.*;
import java.util.*;

public class NetworkModule
{
    protected Engine m_engine;
    protected int m_url_timeout;
	
	// MW-2013-10-02: [[ MobileSSLVerify ]] Determines whether SSL connections
	//   should be verified.
	protected boolean m_url_ssl_verify;
    
    public NetworkModule(Engine p_engine)
    {
        m_engine = p_engine;
        
        m_url_timeout = 10 * 1000;
		
		m_url_ssl_verify = true;
    }
    
    public void setURLTimeout(int p_timeout)
    {
        m_url_timeout = p_timeout * 1000;
    }
    
	// MW-2013-10-02: [[ MobileSSLVerify ]] Enables or disables SSL verification.
	public void setURLSSLVerification(boolean p_enabled)
	{
		m_url_ssl_verify = p_enabled;
	}
    
    public boolean loadURL(int p_id, String p_url, String p_headers)
	{
		try
		{
			URLLoader t_loader = createURLLoader(p_id, p_url);
			t_loader.setHeaders(p_headers);
			t_loader.setTimeout(m_url_timeout);
			// MW-2013-10-02: [[ MobileSSLVerify ]] Configure the loaders ssl verification.
			t_loader.setSSLVerification(m_url_ssl_verify);
			
			new Thread(t_loader).start();
		}
		catch (MalformedURLException e)
		{
			return false;
		}
		catch (URISyntaxException e)
		{
			return false;
		}
		catch (IOException e)
		{
			return false;
		}
		
		return true;
	}

	public boolean postURL(int p_id, String p_url, String p_headers, byte[] p_post_data)
	{
		try
		{
			URLLoader t_loader = createURLLoader(p_id, p_url);
            t_loader.setHeaders("Content-Type: application/x-www-form-urlencoded");
			t_loader.setHeaders(p_headers);
			t_loader.setMethod("POST");
			t_loader.setUploadData(p_post_data);
			t_loader.setTimeout(m_url_timeout);
			// MW-2013-10-02: [[ MobileSSLVerify ]] Configure the loaders ssl verification.
			t_loader.setSSLVerification(m_url_ssl_verify);
			
			new Thread(t_loader).start();
		}
		catch (MalformedURLException e)
		{
			return false;
		}
		catch (URISyntaxException e)
		{
			return false;
		}
		catch (IOException e)
		{
			return false;
		}
		
		return true;
	}
	
	public boolean putURL(int p_id, String p_url, String p_headers, byte[] p_send_data)
	{
        try
        {
            URLLoader t_loader = createURLLoader(p_id, p_url);
            t_loader.setHeaders(p_headers);
            t_loader.setMethod("PUT");
            t_loader.setUploadData(p_send_data);
            t_loader.setTimeout(m_url_timeout);
			// MW-2013-10-02: [[ MobileSSLVerify ]] Configure the loaders ssl verification.
			t_loader.setSSLVerification(m_url_ssl_verify);
            
            new Thread(t_loader).start();
        }
        catch (MalformedURLException e)
        {
            return false;
        }
        catch (URISyntaxException e)
        {
            return false;
        }
        catch (IOException e)
        {
            return false;
        }
        
        return true;
	}
    
	protected URLLoader createURLLoader(int p_id, String p_url) throws URISyntaxException, MalformedURLException, IOException
	{
		return new URLLoader(p_id, p_url)
		{
			public void onURLStart()
			{
				final int t_id = getID();
				m_engine.post(new Runnable() {
					public void run() {
						m_engine.onUrlDidStart(t_id);
					}
				});
			}
			
			public void onURLConnect()
			{
				final int t_id = getID();
				final int t_content_length = getContentLength();
				m_engine.post(new Runnable() {
					public void run() {
						m_engine.onUrlDidConnect(t_id, t_content_length);
					}
				});
			}
			
			public void onURLUploadData(int p_sent)
			{
				final URLLoader t_loader = this;
				final int t_id = getID();
				final int t_sent = p_sent;
				m_engine.post(new Runnable() {
					public void run() {
						m_engine.onUrlDidSendData(t_id, t_sent);
					}
				});
			}
			public void onURLReceiveData()
			{
				final URLLoader t_loader = this;
				final int t_id = getID();		
				m_engine.post(new Runnable() {
					public void run() {
						byte[] t_bytes;
						t_bytes = t_loader . popBytes();
						m_engine.onUrlDidReceiveData(t_id, t_bytes, t_bytes . length);
					}
				});
                
			}
			
			public void onURLComplete()
			{
				final int t_id = getID();
				m_engine.post(new Runnable() {
					public void run() {
						m_engine.onUrlDidFinish(t_id);
					}
				});
                
			}
			
			public void onURLError()
			{
				final int t_id = getID();
				final String t_err_msg = getErrorString();
				m_engine.post(new Runnable() {
					public void run() {
						m_engine.onUrlError(t_id, t_err_msg);
					}
				});
                
			}
		};
	}

    public String getNetworkInterfaces()
    {
        StringBuffer t_sb = new StringBuffer();

        try
        {
            Enumeration<NetworkInterface> t_interface_enum = NetworkInterface.getNetworkInterfaces();
            while (t_interface_enum.hasMoreElements())
            {
                NetworkInterface t_interface = t_interface_enum.nextElement();
                Enumeration<InetAddress> t_addr_enum = t_interface.getInetAddresses();
                while (t_addr_enum.hasMoreElements())
                {
                    InetAddress t_addr = t_addr_enum.nextElement();
                    if (t_addr instanceof Inet4Address)
                    {
                        if (t_sb.length() > 0)
                        t_sb.append("\n");
                        t_sb.append(t_addr.getHostAddress());
                    }
                }
            }
        }
        catch (SocketException e)
        {
            return null;
        }

        return t_sb.toString();
    }
}
