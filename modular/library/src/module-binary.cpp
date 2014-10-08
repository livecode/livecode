
#include "foundation.h"

void MCBinaryExecPutBytesBefore(MCHandlerContext& ctxt, MCDataRef p_source, MCDataRef& x_target)
{
    MCAutoDataRef t_data;
    if (!MCDataMutableCopy(x_target, &t_data))
        return;
    
    if (!MCDataPrepend(*t_data, p_source))
        return;
    
    if (!MCDataCopy(*t_data, x_target))
        return;
}

void MCBinaryExecPutBytesAfter(MCHandlerContext& ctxt, MCDataRef p_source, MCDataRef& x_target)
{
    MCAutoDataRef t_data;
    if (!MCDataMutableCopy(x_target, &t_data))
        return;
    
    if (!MCDataAppend(*t_data, p_source))
        return;
    
    if (!MCDataCopy(*t_data, x_target))
        return;
}

void MCBinaryEvalConcatenateBytes(MCHandlerContext& ctxt, MCDataRef p_left, MCDataRef p_right, MCDataRef& r_output)
{
    MCAutoDataRef t_data;
    if (!MCDataMutableCopy(p_left, &t_data))
        return;
    
    if (!MCDataAppend(*t_data, p_right))
        return;
    
    if (!MCDataCopy(*t_data, r_output))
        return;
}

void MCBinaryEvalNumberOfBytesIn(MCHandlerContext& ctxt, MCDataRef p_source, uindex_t& r_output)
{
    r_output = MCDataGetLength(p_source);
}

void MCBinaryEvalIsAmongTheBytesOf(MCHandlerContext& ctxt, MCDataRef p_needle, MCDataRef p_target, bool p_is_not, bool& r_output)
{
    bool t_found = MCDataContains(p_target, p_needle);
    
    if (p_is_not)
        t_found = !t_found;
    
    r_output = t_found;
}

void MCBinaryEvalOffsetOfBytesIn(MCHandlerContext& ctxt, MCDataRef p_needle, MCDataRef p_target, uindex_t& r_output)
{
    r_output = MCDataFirstIndexOf(p_target, p_needle, 0);
}

void MCBinaryEvalOffsetOfBytesAfterIndexIn(MCHandlerContext& ctxt, MCDataRef p_needle, MCDataRef p_target, uindex_t p_after, uindex_t& r_output)
{
    r_output = MCDataFirstIndexOf(p_target, p_needle, p_after);
}

void MCBinaryFetchByteOf(MCHandlerContext& ctxt, uindex_t p_index, MCDataRef p_target, MCDataRef& r_output)
{
    
}

void MCBinaryStoreByteOf(MCHandlerContext& ctxt, MCDataRef p_value, uindex_t p_index, MCDataRef& x_target)
{
    
}

void MCBinaryFetchByteRangeOf(MCHandlerContext& ctxt, uindex_t p_start, uindex_t p_finish, MCDataRef p_target, MCDataRef& r_output)
{
    
}

void MCBinaryStoreByteRangeOf(MCHandlerContext& ctxt, MCDataRef p_value, uindex_t p_start, uindex_t p_finish, MCDataRef& x_target)
{
    
}