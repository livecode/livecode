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

#if !defined(_SERVER)
static bool s_ssl_loaded = false;

extern "C" int initialise_weak_link_crypto(void);
extern "C" int initialise_weak_link_ssl(void);
bool load_ssl_library()
{
	if (s_ssl_loaded)
		return true;
    
	s_ssl_loaded = initialise_weak_link_crypto() && initialise_weak_link_ssl();
    
	return s_ssl_loaded;
}
#elif defined(_SERVER)
bool load_ssl_library()
{
	return true;
}
#endif
