#ifndef __MC_SSERIALIZE_LNX__
#define __MC_SSERIALIZE_LNX__

#ifndef __MC_CORE__
#include "core.h"
#endif

////////////////////////////////////////////////////////////////////////////////

struct MCLinuxPageSetup
{
	int32_t paper_width;
	int32_t paper_height;
	int32_t left_margin, top_margin, right_margin, bottom_margin;
	uint32_t orientation;
};

bool MCLinuxPageSetupEncode(const MCLinuxPageSetup& setup, void*& r_data, uint32_t& r_data_size);
bool MCLinuxPageSetupDecode(const void *data, uint32_t data_size, MCLinuxPageSetup& setup);

////////////////////////////////////////////////////////////////////////////////

struct MCLinuxPrintSetup
{
};

bool MCLinuxPrintSetupEncode(const MCLinuxPrintSetup& setup, void*& r_data, uint32_t& r_data_size);
bool MCLinuxPrintSetupDecode(const void *data, uint32_t data_size, MCLinuxPrintSetup& setup);

////////////////////////////////////////////////////////////////////////////////

#endif
