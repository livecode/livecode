
#include <foundation.h>
#include <foundation-auto.h>

void MCBinaryExecPutBytesBefore(MCDataRef p_source, MCDataRef& x_target)
{
    MCAutoDataRef t_data;
    MCBinaryEvalConcatenateBytes(p_source, x_target, &t_data);
    
    MCValueAssign(x_target, *t_data);
}

void MCBinaryExecPutBytesAfter(MCDataRef p_source, MCDataRef& x_target)
{
    MCAutoDataRef t_data;
    MCBinaryEvalConcatenateBytes(x_target, p_source, &t_data);
    
    MCValueAssign(x_target, *t_data);
}

void MCBinaryEvalConcatenateBytes(MCDataRef p_left, MCDataRef p_right, MCDataRef& r_output)
{
    MCAutoDataRef t_data;
    if (!MCDataMutableCopy(p_left, &t_data))
        return;
    
    if (!MCDataAppend(*t_data, p_right))
        return;
    
    if (!MCDataCopy(*t_data, r_output))
        return;
}

void MCBinaryEvalNumberOfBytesIn(MCDataRef p_source, uindex_t& r_output)
{
    r_output = MCDataGetLength(p_source);
}

void MCBinaryEvalIsAmongTheBytesOf(MCDataRef p_needle, MCDataRef p_target, bool p_is_not, bool& r_output)
{
    bool t_found = MCDataContains(p_target, p_needle);
    
    if (p_is_not)
        t_found = !t_found;
    
    r_output = t_found;
}

void MCBinaryEvalOffsetOfBytesIn(MCDataRef p_needle, MCDataRef p_target, uindex_t& r_output)
{
    r_output = MCDataFirstIndexOf(p_target, p_needle, 0);
}

void MCBinaryEvalOffsetOfBytesAfterIndexIn(MCDataRef p_needle, MCDataRef p_target, uindex_t p_after, uindex_t& r_output)
{
    r_output = MCDataFirstIndexOf(p_target, p_needle, p_after);
}

void MCBinaryFetchByteOf(uindex_t p_index, MCDataRef p_target, MCDataRef& r_output)
{
    if (!MCDataCreateWithBytes(MCDataGetBytePtr(p_target) + (p_index - 1), 1, r_output))
        return;
}

void MCBinaryStoreByteOf(MCDataRef p_value, uindex_t p_index, MCDataRef& x_target)
{
    
}

void MCBinaryFetchByteRangeOf(uindex_t p_start, uindex_t p_finish, MCDataRef p_target, MCDataRef& r_output)
{
    if (!MCDataCreateWithBytes(MCDataGetBytePtr(p_target) + (p_start - 1), p_finish - p_start, r_output))
        return;
}

void MCBinaryStoreByteRangeOf(MCDataRef p_value, uindex_t p_start, uindex_t p_finish, MCDataRef& x_target)
{
    
}