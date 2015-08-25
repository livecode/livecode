#ifndef __MC_SSERIALIZE_W32_H__
#define __MC_SSERIALIZE_W32_H__

#include <windows.h>

extern bool deserialize_data_to_hglobal(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, HGLOBAL &r_hglobal);
extern bool serialize_printdlg_data(char *&r_buffer, uint32_t &r_size, PRINTDLGEXA &p_data);
extern bool deserialize_printdlg_data(const char *p_buffer, uint32_t p_size, PRINTDLGEXA *&x_data);
extern bool serialize_pagedlg_data(char *&r_buffer, uint32_t &r_size, PAGESETUPDLGA &p_data);
extern bool deserialize_pagedlg_data(const char *p_buffer, uint32_t p_size, PAGESETUPDLGA *&x_data);

#endif
