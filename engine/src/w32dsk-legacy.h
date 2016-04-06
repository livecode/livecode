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

extern unsigned char MCS_langidtocharset(unsigned short langid);
extern unsigned short MCS_charsettolangid(unsigned char charset);

extern void MCS_unicodetomultibyte(const char *s, unsigned int len, char *d,
                            unsigned int destbufferlength, unsigned int &destlen,
                            unsigned char charset);
extern void MCS_multibytetounicode(const char *s, unsigned int len, char *d,
                            unsigned int destbufferlength, unsigned int &destlen,
                            unsigned char charset);
