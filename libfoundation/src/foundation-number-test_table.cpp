#include <vector>

using namespace std;

////////

#define INT_IS_32
#define RESTRICT_BITS

///////

#ifdef INT_IS_32

#define MC_INT_MIN INT32_MIN
#define MC_INT_MAX UINT32_MAX
#define MC_INT_BITS 32

#else

#define MC_INT_MIN INT64_MIN
#define MC_INT_MAX UINT64_MAX
#define MC_INT_BITS 64

#endif

static int s_bit_positions[] =
{
    0, 1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 63
};

struct BinaryEntry
{
    __int128_t x, y;
    struct
    {
        __int128_t add;
        __int128_t sub;
        __int128_t mul;
        __int128_t div;
        __int128_t mod;
        __int128_t wrap;
    };
};

static vector<BinaryEntry> s_entries;

template<typename T> static inline T sgn(T a)
{
    return a < 0 ? -1 : (a > 0 ? 1 : 0);
}

template<typename T> static inline T abs(T a)
{
    return a >= 0 ? a : -a;
}

template<typename T> static inline T __flooring_integral_mod(T x, T y)
{
    return sgn(y) * (abs(x) % abs(y));
}

static bool IsOutOfRange(__int128_t x)
{
    if (x < MC_INT_MIN || x > MC_INT_MAX)
        return true;
    
    return false;
}

static bool IsBitOutOfRange(int x)
{
#ifdef RESTRICT_BITS
    if (x < 0)
        x = -x;
    
    for(int i = 0; i < sizeof(s_bit_positions) / sizeof(s_bit_positions[0]); i++)
        if (s_bit_positions[i] == x)
            return false;
    
    return true;
#else
    return false;
#endif
}

static bool IsDuplicateBinaryEntry(__int128_t x, __int128_t y)
{
    for(int i = 0; i < s_entries . size(); i++)
        if (s_entries[i] . x == x && s_entries[i] . y == y)
            return true;
    
    return false;
}

static void AddBinaryEntry(__int128_t x, __int128_t y)
{
    if (IsOutOfRange(x))
        return;
    
    if (IsOutOfRange(y))
        return;
    
    if (IsDuplicateBinaryEntry(x, y))
        return;
    
    BinaryEntry e;
    e . x = x;
    e . y = y;
    e . add = x + y;
    e . sub = x - y;
    e . mul = x * y;
    if (y != 0)
    {
        e . div = x / y;
        e . mod = __flooring_integral_mod(x, y);
        e . wrap = __flooring_integral_mod(x - 1, y) + 1;
    }
    else
    {
        e . div = 0;
        e . mod = 0;
        e . wrap = 0;
    }
    
    s_entries . push_back(e);
}

static void EmitValue(FILE *stream, __int128_t x)
{
    if (IsOutOfRange(x))
        fprintf(stream, "");
    else if (x < 0)
        fprintf(stream, "%lld", (int64_t)x);
    else if (x >= 0)
        fprintf(stream, "%llu", (uint64_t)x);
}

int main(int argc, char **argv)
{
    fprintf(stderr, "Generating tables for:\n");
    fprintf(stderr, "  %d bit integers\n", MC_INT_BITS);
    fprintf(stderr, "  %lld minimum value\n", (int64_t)MC_INT_MIN);
    fprintf(stderr, "  %llu maximum value\n", (uint64_t)MC_INT_MAX);
    
    for(int xb = -(MC_INT_BITS - 1); xb < MC_INT_BITS; xb++)
    {
        if (IsBitOutOfRange(xb))
            continue;
        
        for(int yb = -(MC_INT_BITS - 1); yb < MC_INT_BITS; yb++)
        {
            if (IsBitOutOfRange(yb))
                continue;
            
            __int128_t one;
            one = 1;
            
            __int128_t x, y;
            if (xb < 0)
                x = -(one << (-xb));
            else
                x = one << xb;
            
            if (yb < 0)
                y = -(one << (-yb));
            else
                y = one << yb;
                
            AddBinaryEntry(x, y);
            AddBinaryEntry(x + 1, y);
            AddBinaryEntry(x - 1, y);
            
            AddBinaryEntry(x, y + 1);
            AddBinaryEntry(x + 1, y + 1);
            AddBinaryEntry(x - 1, y + 1);
            
            AddBinaryEntry(x, y - 1);
            AddBinaryEntry(x + 1, y - 1);
            AddBinaryEntry(x - 1, y - 1);
        }
    }
    
    fprintf(stderr, "Generating %ld entries\n", s_entries . size());
    
#if 0
    fprintf(stdout, "[\\\n");
    for(int i = 0; i < s_entries . size(); i++)
    {
        fprintf(stdout, i == 0 ? "[ " : ",\\\n[ ");
        EmitValue(stdout, s_entries[i] . x);
        fprintf(stdout, ", ");
        EmitValue(stdout, s_entries[i] . y);
        fprintf(stdout, ", [ ");
        EmitValue(stdout, s_entries[i] . add);
        fprintf(stdout, ", ");
        EmitValue(stdout, s_entries[i] . sub);
        fprintf(stdout, ", ");
        EmitValue(stdout, s_entries[i] . mul);
        fprintf(stdout, ", ");
        if (s_entries[i] . y != 0)
            EmitValue(stdout, s_entries[i] . div);
        else
            fprintf(stdout, "\"dbz\"");
        fprintf(stdout, ", ");
        EmitValue(stdout, s_entries[i] . mod);
        fprintf(stdout, ", ");
        EmitValue(stdout, s_entries[i] . wrap);
        fprintf(stdout, " ] ]");
    }
    fprintf(stdout, "\\\n]\n");
#else
    for(int i = 0; i < s_entries . size(); i++)
    {
        if (i != 0)
            fprintf(stdout, "\n");
        EmitValue(stdout, s_entries[i] . x);
        fprintf(stdout, ",");
        EmitValue(stdout, s_entries[i] . y);
        fprintf(stdout, ",");
        EmitValue(stdout, s_entries[i] . add);
        fprintf(stdout, ",");
        EmitValue(stdout, s_entries[i] . sub);
        fprintf(stdout, ",");
        EmitValue(stdout, s_entries[i] . mul);
        fprintf(stdout, ",");
        if (s_entries[i] . y != 0)
            EmitValue(stdout, s_entries[i] . div);
        else
            fprintf(stdout, "");
        fprintf(stdout, ",");
        if (s_entries[i] . y != 0)
            EmitValue(stdout, s_entries[i] . mod);
        else
            fprintf(stdout, "");
        fprintf(stdout, ",");
        if (s_entries[i] . y != 0)
            EmitValue(stdout, s_entries[i] . wrap);
        else
            fprintf(stdout, "");
    }
#endif
    
    
    return 0;
}