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

#ifndef X_PASTEBOARD_H
#define X_PASTEBOARD_H


class MCGdkPasteboard : public MCPasteboard
{
public:
    
    MCGdkPasteboard(GdkAtom p_atom, MCGdkTransferStore *p_store);
    virtual ~MCGdkPasteboard() {}
    
    void Retain();
    void Release();
    bool Query(MCTransferType* &r_types, size_t &r_type_count);
    bool Fetch(MCTransferType p_type, MCDataRef &r_data);
    bool Fetch_MIME(MCMIMEtype *p_type, MCDataRef &r_data);
    
    void SetWindows(GdkWindow *p_source_window, GdkWindow *p_target_window);
    
private:
    
    GdkAtom m_atom;
    GdkWindow *m_source_window;
    GdkWindow *m_target_window;
    guint32 m_lock_time;
    
    unsigned int m_references;
    
    unsigned int m_type_count;
    MCTransferType *m_types;
    
    bool m_valid;
    
    MCGdkTransferStore *m_transfer_store;
};


#endif 
