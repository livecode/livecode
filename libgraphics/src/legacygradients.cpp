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

#include "graphics.h"
#include "graphics-internal.h"

////////////////////////////////////////////////////////////////////////////////

#if __BIG_ENDIAN__
#define iman_ 1
#else
#define iman_ 0
#endif

typedef long int32;

#ifdef _LINUX
//IM - fast double -> int code seems to be broken on linux
inline int32 fast_rint(double val) {
	return (int32)(val + 0.5);
}

inline int32 fast_floor(double val) {
	return (int32)val;
}
#else

/* Fast version of (int)rint()
 Works for -2147483648.5 .. 2147483647.49975574019
 Requires IEEE floating point.
 */
inline int32 fast_rint(double val) {
	val = val + 68719476736.0*65536.0*1.5;
	return ((int32*)&val)[iman_];
}

/* Fast version of (int)floor()
 Requires IEEE floating point.
 Rounds numbers greater than n.9999923668 to n+1 rather than n,
 this could be fixed by changing the FP rounding mode and using
 the fast_rint() code.
 Works for -32728 to 32727.99999236688
 The alternative that uses long-long works for -2147483648 to 
 2147483647.999923688
 */
inline int32 fast_floor(double val) {
	val = val + (68719476736.0*1.5);
#if LARGER_FAST_FLOOR
	return (int32)((*(long long *)&val)>>16);
#else
	return (((int32*)&val)[iman_]>>16);
#endif
}
#endif

#define STOP_DIFF_PRECISION 24
#define STOP_DIFF_MULT ((1 << STOP_DIFF_PRECISION) * (uint32_t)255)
#define STOP_INT_PRECISION 16
#define STOP_INT_MAX ((1 << STOP_INT_PRECISION) - 1)
#define STOP_INT_MIRROR_MAX ((2 << STOP_INT_PRECISION) - 1)
#define GRADIENT_ROUND_EPSILON (float)0.000005

#define GRADIENT_AA_SCALE (2)

#define FP_2PI ((int32_t)(2 * M_PI * (1<<8)))
#define FP_INV_2PI ((STOP_INT_MAX << 8) / FP_2PI)

////////////////////////////////////////////////////////////////////////////////

struct MCGradientFillStop
{
	uint32_t offset;
	uint32_t color;
	uint32_t hw_color;
	uint32_t difference;
};

////////////////////////////////////////////////////////////////////////////////

#ifdef __VISUALC__
#define PACKED_INLINE __forceinline
#else
#define PACKED_INLINE inline
#endif

// r_i = (x_i * a) / 255
PACKED_INLINE uint32_t packed_scale_bounded(uint32_t x, uint8_t a)
{
	uint32_t u, v;
	
	u = ((x & 0xff00ff) * a) + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
	
	v = (((x >> 8) & 0xff00ff) * a) + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
	
	return u | v;
}

// r_i = (x_i * a + y_i * b) / 255
PACKED_INLINE uint32_t packed_bilinear_bounded(uint32_t x, uint8_t a, uint32_t y, uint8_t b)
{
	uint32_t u, v;
	
	u = (x & 0xff00ff) * a + (y & 0xff00ff) * b + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
	
	v = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
	
	return u | v;
}

PACKED_INLINE uint32_t _combine(uint32_t u, uint32_t v)
{
	u += 0x800080;
	v += 0x800080;
	return (((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff) + (((v + ((v >> 8) & 0xff00ff))) & 0xff00ff00);
}

PACKED_INLINE uint32_t _multiply_low(uint32_t x, uint32_t y)
{
	return ((x & 0xff) * (y & 0xff)) | ((x & 0xff0000) * ((y >> 16) & 0xff));
}

PACKED_INLINE uint32_t _multiply_high(uint32_t x, uint32_t y)
{
	x = x >> 8;
	return ((x & 0xff) * ((y >> 8) & 0xff)) | ((x & 0xff0000) * (y >> 24));
}

// r_i = x_i * y_i / 255;
PACKED_INLINE uint32_t packed_multiply_bounded(uint32_t x, uint32_t y)
{
	return _combine(_multiply_low(x, y), _multiply_high(x, y));
}

// r_i = (x_i + y_i) / 2
PACKED_INLINE uint32_t packed_avg(uint32_t x, uint32_t y)
{
	uint32_t u, v;
	u = (((x & 0xff00ff) + (y & 0xff00ff)) >> 1) & 0xff00ff;
	v = ((((x >> 8) & 0xff00ff) + ((y >> 8) & 0xff00ff)) << 7) & 0xff00ff00;
	
	return u | v;
}

////////////////////////////////////////////////////////////////////////////////

class MCGGeneralizedGradientShader : public SkShader
{
public:
    MCGGeneralizedGradientShader(MCGGeneralizedGradientRef p_gradient)
    {
        m_gradient = MCGRetain(p_gradient);
    }
    
    ~MCGGeneralizedGradientShader()
    {
        MCGRelease(m_gradient);
    }
    
    class Context : public SkShader::Context
    {
    public:
        Context(const MCGGeneralizedGradientShader& p_shader, const ContextRec& p_rec)
            : INHERITED(p_shader, p_rec)
        {
            /* Matrix is context transform * gradient transform */
            MCGAffineTransform t_rec_transform;
            MCGAffineTransformFromSkMatrix(*p_rec.fMatrix, t_rec_transform);
            
            MCGAffineTransform t_transform =
                    MCGAffineTransformConcat(t_rec_transform, p_shader.m_gradient->m_transform);
            
            int32_t vx = (int32_t) t_transform . a;
            int32_t vy = (int32_t) t_transform . b;
            int32_t wx = (int32_t) t_transform . c;
            int32_t wy = (int32_t) t_transform . d;
            
            int32_t d = vy * wx - vx *wy;
            
            MCGRampRef t_ramp = p_shader.m_gradient->m_ramp;
            if (!MCMemoryNewArray(t_ramp->GetLength(), m_ramp))
            {
                return;
            }
            
            m_ramp_length = t_ramp->GetLength();
            
            uint32_t i;
            for (i = 0; i < m_ramp_length; i++)
            {
                m_ramp[i].offset = (uint32_t) (t_ramp->GetStops()[i] * STOP_INT_MAX);
                m_ramp[i].color = t_ramp->GetColors()[i];
                
                if (i != 0)
                {
                    // MM-2013-11-20: [[ Bug 11479 ]] Make sure we don't divide by zero.
                    if (m_ramp[i].offset != m_ramp[i - 1].offset)
                    {
                        m_ramp[i - 1].difference = (uint32_t)(STOP_DIFF_MULT / (m_ramp[i] . offset - m_ramp[i - 1] . offset));
                    }
                    else
                    {
                        m_ramp[i - 1].difference = (uint32_t) (STOP_DIFF_MULT / STOP_INT_MAX);
                    }
                }
                // AL-2014-07-21: [[ Bug 12867 ]] Ensure RBGA values are always packed in native format
                m_ramp[i].hw_color = MCGPixelToNative(kMCGPixelFormatBGRA, m_ramp[i] . color);
            }
            
            // MW-2013-10-26: [[ Bug 11315 ]] Index shuold be i - 1 (otherwise memory overrun occurs!).
            m_ramp[i - 1].difference = (uint32_t) (STOP_DIFF_MULT / STOP_INT_MAX);
            
            m_bilinear = p_shader.m_gradient->m_filter != kMCGImageFilterNone;
            m_origin_x = t_transform.tx;
            m_origin_y = t_transform.ty;
            m_mirror = p_shader.m_gradient->m_mirror;
            m_repeats = p_shader.m_gradient->m_repeats;
            m_wrap = p_shader.m_gradient->m_wrap;
            
            if (d != 0)
            {
                m_x_coef_a = STOP_INT_MAX * -wy / d;
                m_x_coef_b = STOP_INT_MAX * wx / d;
                m_x_inc = (uint32_t) (STOP_INT_MAX * (int64_t)(m_origin_x * wy - m_origin_y * wx) / d);
                m_y_coef_a = STOP_INT_MAX * vy / d;
                m_y_coef_b = STOP_INT_MAX * -vx / d;
                m_y_inc = (uint32_t) (STOP_INT_MAX * -(int64_t)(m_origin_x * vy - m_origin_y * vx) / d);
            }
            else
            {
                m_x_coef_a = 0;
                m_x_coef_b = 0;
                m_x_inc = 0;
                m_y_coef_a = 0;
                m_y_coef_b = 0;
                m_y_inc = 0;
            }
            
            if (!m_bilinear)
            {
                m_x_inc += (m_x_coef_a + m_x_coef_b) >> 1;
                m_y_inc += (m_y_coef_a + m_y_coef_b) >> 1;
                switch(p_shader.m_gradient->m_function)
                {
                    case kMCGGradientFunctionLinear:
                        m_combine = fill_combine<kMCGGradientFunctionLinear>;
                        break;
                    case kMCGGradientFunctionRadial:
                        m_combine = fill_combine<kMCGGradientFunctionRadial>;
                        break;
                    case kMCGGradientFunctionSweep:
                        m_combine = fill_combine<kMCGGradientFunctionSweep>;
                        break;
                    case kMCGLegacyGradientDiamond:
                        m_combine = fill_combine<kMCGLegacyGradientDiamond>;
                        break;
                    case kMCGLegacyGradientSpiral:
                        m_combine = fill_combine<kMCGLegacyGradientSpiral>;
                        break;
                    case kMCGLegacyGradientXY:
                        m_combine = fill_combine<kMCGLegacyGradientXY>;
                        break;
                    case kMCGLegacyGradientSqrtXY:
                        m_combine = fill_combine<kMCGLegacyGradientSqrtXY>;
                        break;
                }
            }
            else
            {
                m_x_inc += (m_x_coef_a + m_x_coef_b) >> 2;
                m_y_inc += (m_y_coef_a + m_y_coef_b) >> 2;
                switch(p_shader.m_gradient->m_function)
                {
                    case kMCGGradientFunctionLinear:
                        m_combine = bilinear_fill_combine<kMCGGradientFunctionLinear>;
                        break;
                    case kMCGGradientFunctionRadial:
                        m_combine = bilinear_fill_combine<kMCGGradientFunctionRadial>;
                        break;
                    case kMCGGradientFunctionSweep:
                        m_combine = bilinear_fill_combine<kMCGGradientFunctionSweep>;
                        break;
                    case kMCGLegacyGradientDiamond:
                        m_combine = bilinear_fill_combine<kMCGLegacyGradientDiamond>;
                        break;
                    case kMCGLegacyGradientSpiral:
                        m_combine = bilinear_fill_combine<kMCGLegacyGradientSpiral>;
                        break;
                    case kMCGLegacyGradientXY:
                        m_combine = bilinear_fill_combine<kMCGLegacyGradientXY>;
                        break;
                    case kMCGLegacyGradientSqrtXY:
                        m_combine = bilinear_fill_combine<kMCGLegacyGradientSqrtXY>;
                        break;
                }
            }
            
            m_y = 0;
            m_buffer = nullptr;
            m_buffer_width = 0;
        }
        
        ~Context()
        {
            MCMemoryDeleteArray(m_ramp);
        }
        
        virtual void shadeSpan(int x, int y, SkPMColor dstC[], int count) override
        {
            if (m_combine == nullptr)
            {
                return;
            }
            
            int32_t t_dy = y - m_y;
            m_x_inc += m_x_coef_b * t_dy;
            m_y_inc += m_y_coef_b * t_dy;
            m_y = y;
            memset(dstC, 0x00, count * sizeof(SkPMColor));
            m_bits = dstC - x;
            
            if (m_bilinear)
            {
                if ((int)m_buffer_width < count * GRADIENT_AA_SCALE)
                {
                    uindex_t t_size = m_buffer_width * GRADIENT_AA_SCALE * GRADIENT_AA_SCALE;
                    if (!MCMemoryResizeArray(count * GRADIENT_AA_SCALE * GRADIENT_AA_SCALE, m_buffer, t_size))
                    {
                        return;
                    }
                    m_buffer_width = count * GRADIENT_AA_SCALE;
                }
            }
            
            m_combine(this, x, x + count);
        }
        
        template<MCGGradientFunction x_type>
        static inline int32_t compute_index(int32_t p_x, int32_t p_y, bool p_mirror, uint32_t p_repeat, bool p_wrap)
        {
            int32_t t_index;
            switch(x_type)
            {
                case kMCGGradientFunctionLinear:
                    t_index = p_x;
                    break;
                case kMCGGradientFunctionSweep:
                {
                    int32_t t_angle = fast_rint((atan2((double)p_y, p_x) * (1<<8)));
                    if (t_angle < 0)
                        t_angle += FP_2PI;
                    t_index = (t_angle * FP_INV_2PI) >> 8;
                }
                    break;
                case kMCGGradientFunctionRadial:
                {
                    double t_dist = ((double)(p_x)*p_x + (double)(p_y)*p_y);
                    t_index = !p_wrap && t_dist > ((double)STOP_INT_MAX * STOP_INT_MAX) ? STOP_INT_MAX + 1 : fast_rint(sqrt(t_dist));
                }
                    break;
                case kMCGLegacyGradientDiamond:
                    t_index = MCMax(MCAbs(p_x), MCAbs(p_y));
                    break;
                case kMCGLegacyGradientSpiral:
                {
                    int32_t t_angle = fast_rint((atan2((double)p_y, p_x) * (1<<8)));
                    double t_dist = sqrt((double)(p_x)*p_x + (double)(p_y)*p_y);
                    t_index = fast_rint(t_dist);
                    if (t_angle > 0)
                        t_angle -= FP_2PI;
                    t_index -= (t_angle * FP_INV_2PI) >> 8;
                    t_index %= STOP_INT_MAX;
                }
                    break;
                case kMCGLegacyGradientXY:
                {
                    uint32_t t_x = MCAbs(p_x);  uint32_t t_y = MCAbs(p_y);
                    t_index = (int32_t) ((int64_t)t_x * t_y / STOP_INT_MAX);
                }
                    break;
                case kMCGLegacyGradientSqrtXY:
                {
                    double t_x = MCAbs(p_x);  double t_y = MCAbs(p_y);
                    t_index = fast_rint(sqrt(t_x * t_y));
                }
                    break;
            }
            
            if (p_mirror)
            {
                if (p_wrap)
                {
                    if (p_repeat > 1)
                        t_index = (t_index * p_repeat);
                    t_index &= STOP_INT_MIRROR_MAX;
                    if (t_index > STOP_INT_MAX)
                    {
                        t_index = STOP_INT_MAX - (t_index & STOP_INT_MAX);
                    }
                }
                else
                {
                    if (t_index >= STOP_INT_MAX)
                    {
                        if ((p_repeat & 1) == 0)
                            t_index = -t_index;
                    }
                    else if (p_repeat > 1 && t_index > 0)
                    {
                        t_index = (t_index * p_repeat);
                        t_index &= STOP_INT_MIRROR_MAX;
                        if (t_index > STOP_INT_MAX)
                        {
                            t_index = STOP_INT_MAX - (t_index & STOP_INT_MAX);
                        }
                    }
                }
            }
            else
            {
                if (p_wrap)
                    t_index &= STOP_INT_MAX;
                if (p_repeat > 1 && t_index > 0 && t_index < STOP_INT_MAX)
                {
                    t_index = (t_index * p_repeat);
                    t_index &= 0xFFFF;
                }
            }
            return t_index;
        }

        template<MCGGradientFunction x_type>
        static void fill_combine(Context *self, int32_t fx, int32_t tx)
        {
            uint32_t *d;
            uint32_t s;
            
            d = self->m_bits;
            
            int32_t t_index;
            int32_t t_x = self->m_x_inc + self->m_x_coef_a * ((int32_t)fx);
            int32_t t_y = self->m_y_inc + self->m_y_coef_a * ((int32_t)fx);
            
            int32_t t_min = (int32_t)self->m_ramp[0].offset;
            int32_t t_max = (int32_t)self->m_ramp[self->m_ramp_length - 1].offset;
            
            if (fx == tx) return;
            
            bool t_mirror = self->m_mirror;
            uint32_t t_repeat = self->m_repeats;
            bool t_wrap = self->m_wrap;
            
            uint32_t t_stop_pos = 0;
            
            t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
            while (fx < tx)
            {
                if (t_index <= t_min)
                {
                    s = self->m_ramp[0].hw_color;
                    while (t_index <= t_min)
                    {
                        uint32_t sa = packed_scale_bounded(s | 0xFF000000, (s >> 24));
                        d[fx] = packed_scale_bounded(d[fx], 255 - (sa >> 24)) + sa;
                        fx += 1;
                        if (fx == tx)
                            return;
                        t_x += self->m_x_coef_a;
                        t_y += self->m_y_coef_a;
                        t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
                    }
                }
                
                if (t_index >= t_max)
                {
                    s = self->m_ramp[self->m_ramp_length - 1].hw_color;
                    while (t_index >= t_max)
                    {
                        uint32_t sa = packed_scale_bounded(s | 0xFF000000, (s >> 24));
                        d[fx] = packed_scale_bounded(d[fx], 255 - (sa >> 24)) + sa;
                        fx += 1;
                        if (fx == tx)
                            return;
                        t_x += self->m_x_coef_a;
                        t_y += self->m_y_coef_a;
                        t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
                    }
                }
                
                while (t_index >= t_min && t_index <= t_max)
                {
                    MCGradientFillStop *t_current_stop = &self->m_ramp[t_stop_pos];
                    int32_t t_current_offset = t_current_stop->offset;
                    int32_t t_current_difference = t_current_stop->difference;
                    uint32_t t_current_color = t_current_stop->hw_color;
                    MCGradientFillStop *t_next_stop = &self->m_ramp[t_stop_pos+1];
                    int32_t t_next_offset = t_next_stop->offset;
                    uint32_t t_next_color = t_next_stop->hw_color;
                    
                    while (t_next_offset >= t_index && t_current_offset <= t_index)
                    {
                        uint8_t b = ((t_index - t_current_offset) * t_current_difference) >> STOP_DIFF_PRECISION ;
                        uint8_t a = 255 - b;
                        
                        s = packed_bilinear_bounded(t_current_color, a, t_next_color, b);
                        uint32_t sa = packed_scale_bounded(s | 0xFF000000, (s >> 24));
                        d[fx] = packed_scale_bounded(d[fx], 255 - (sa >> 24)) + sa;
                        fx += 1;
                        if (fx == tx)
                            return;
                        t_x += self->m_x_coef_a;
                        t_y += self->m_y_coef_a;
                        t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
                    }
                    if (t_current_offset > t_index && t_stop_pos > 0)
                        t_stop_pos -= 1;
                    else if (t_next_offset < t_index && t_stop_pos < (self->m_ramp_length - 1))
                        t_stop_pos += 1;
                }
            }
        }
        
        template<MCGGradientFunction x_type>
        static void blend_row(Context *self, int32_t fx, int32_t tx, uint32_t *p_buff)
        {
            uint32_t s;
            
            int32_t t_index;
            int32_t t_x = self->m_x_inc + self->m_x_coef_a * ((int32_t)fx);
            int32_t t_y = self->m_y_inc + self->m_y_coef_a * ((int32_t)fx);
            
            int32_t t_min = (int32_t)self->m_ramp[0].offset;
            int32_t t_max = (int32_t)self->m_ramp[self->m_ramp_length - 1].offset;
            
            uint32_t t_stop_pos = 0;
            
            bool t_mirror = self->m_mirror;
            uint32_t t_repeat = self->m_repeats;
            bool t_wrap = self->m_wrap;
            
            t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
            while (fx < tx)
            {
                if (t_index <= t_min)
                {
                    s = self->m_ramp[0].hw_color;
                    while (t_index <= t_min)
                    {
                        *p_buff = s;
                        fx += 1;
                        if (fx == tx)
                            return;
                        p_buff++;
                        t_x += self->m_x_coef_a;
                        t_y += self->m_y_coef_a;
                        t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
                    }
                }
                
                if (t_index >= t_max)
                {
                    s = self->m_ramp[self->m_ramp_length - 1].hw_color;
                    while (t_index >= t_max)
                    {
                        *p_buff = s;
                        fx += 1;
                        if (fx == tx)
                            return;
                        p_buff++;
                        t_x += self->m_x_coef_a;
                        t_y += self->m_y_coef_a;
                        t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
                    }
                }
                
                while (t_index >= t_min && t_index <= t_max)
                {
                    MCGradientFillStop *t_current_stop = &self->m_ramp[t_stop_pos];
                    int32_t t_current_offset = t_current_stop->offset;
                    int32_t t_current_difference = t_current_stop->difference;
                    uint32_t t_current_color = t_current_stop->hw_color;
                    MCGradientFillStop *t_next_stop = &self->m_ramp[t_stop_pos+1];
                    int32_t t_next_offset = t_next_stop->offset;
                    uint32_t t_next_color = t_next_stop->hw_color;
                    
                    while (t_next_offset >= t_index && t_current_offset <= t_index)
                    {
                        uint8_t b = ((t_index - t_current_offset) * t_current_difference) >> STOP_DIFF_PRECISION ;
                        uint8_t a = 255 - b;
                        
                        s = packed_bilinear_bounded(t_current_color, a, t_next_color, b);
                        *p_buff = s;
                        fx += 1;
                        if (fx == tx)
                            return;
                        p_buff++;
                        t_x += self->m_x_coef_a;
                        t_y += self->m_y_coef_a;
                        t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
                    }
                    if (t_current_offset > t_index && t_stop_pos > 0)
                        t_stop_pos -= 1;
                    else if (t_next_offset < t_index && t_stop_pos < (self->m_ramp_length - 1))
                        t_stop_pos += 1;
                }
            }
        }
        
        template<MCGGradientFunction x_type>
        static void bilinear_fill_combine(Context *self, int32_t fx, int32_t tx)
        {
            uint32_t *d;
            uint32_t s;
            
            d = self -> m_bits;
            
            if (fx == tx) return;
            
            uint32_t *t_buffer = self->m_buffer;
            uint32_t t_bufflen = self->m_buffer_width;
            
            int32_t x_a, x_b, x_inc;
            int32_t y_a, y_b, y_inc;
            x_a = self->m_x_coef_a; x_b = self->m_x_coef_b; x_inc = self->m_x_inc;
            y_a = self->m_y_coef_a; y_b = self->m_y_coef_b; y_inc = self->m_y_inc;
            self->m_x_coef_a /= GRADIENT_AA_SCALE; self->m_x_coef_b /= GRADIENT_AA_SCALE;
            self->m_y_coef_a /= GRADIENT_AA_SCALE; self->m_y_coef_b /= GRADIENT_AA_SCALE;
            
            uint32_t dx = (tx - fx) * GRADIENT_AA_SCALE;
            uint32_t t_fx = fx * GRADIENT_AA_SCALE;
            for (int i = 0; i < GRADIENT_AA_SCALE; i++)
            {
                blend_row<x_type>(self, t_fx, t_fx + dx, t_buffer);
                t_buffer += t_bufflen;
                self->m_x_inc += self->m_x_coef_b;
                self->m_y_inc += self->m_y_coef_b;
            }
            
            self->m_x_coef_a = x_a; self->m_x_coef_b = x_b; self->m_x_inc = x_inc;
            self->m_y_coef_a = y_a; self->m_y_coef_b = y_b; self->m_y_inc = y_inc;
            
            uint32_t i = 0;
            for (; fx < tx; fx++)
            {
                uint32_t u = (self->m_buffer[i*2] & 0xFF00FF) + (self->m_buffer[i*2 + 1] & 0xFF00FF) + \
                (self->m_buffer[t_bufflen + i*2] & 0xFF00FF) + (self->m_buffer[t_bufflen + i*2 + 1] & 0xFF00FF);
                uint32_t v = ((self->m_buffer[i*2] >> 8) & 0xFF00FF) + ((self->m_buffer[i*2 + 1] >> 8) & 0xFF00FF) + \
                ((self->m_buffer[t_bufflen + i*2] >> 8) & 0xFF00FF) + ((self->m_buffer[t_bufflen + i*2 + 1] >> 8) & 0xFF00FF);
                u = (u >> 2) & 0xFF00FF;
                v = (v << 6) & 0xFF00FF00;
                
                i++;
                
                s = u | v;
                uint8_t alpha = (s >> 24);
                d[fx] = packed_bilinear_bounded(d[fx], 255 - alpha, s | 0xFF000000, alpha);
            }
        }
        
    private:
        MCGradientFillStop *m_ramp = nullptr;
        uint32_t *m_bits;
        void (*m_combine)(Context* self, int32_t fx, int32_t tx) = nullptr;
        size_t m_ramp_length = 0;
        uindex_t m_buffer_width;
        uint32_t *m_buffer;
        int32_t m_origin_x, m_origin_y;
        uint32_t m_repeats;
        int32_t m_x_coef_a, m_x_coef_b;
        int32_t m_y_coef_a, m_y_coef_b;
        int32_t m_x_inc, m_y_inc;
        int32_t m_y;
        bool m_mirror : 1;
        bool m_wrap : 1;
        bool m_bilinear : 1;
        typedef SkShader::Context INHERITED;
    };
    
    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(MCGGeneralizedGradientShader)
    
protected:
    size_t onContextSize(const ContextRec& p_rec) const override
    {
        return sizeof(MCGGeneralizedGradientShader::Context);
    }
    
    Context* onCreateContext(const ContextRec& p_rec, void* p_storage) const override
    {
        return new (p_storage) Context(*this, p_rec);
    }
    
private:
    MCGGeneralizedGradientRef m_gradient;
    
    typedef SkShader::Context INHERITED;
};

#ifndef SK_IGNORE_TO_STRING
void MCGGeneralizedGradientShader::toString(SkString* p_str) const
{
    p_str->append("MCGGeneralizedGradientShader: ()");
}
#endif

sk_sp<SkFlattenable> MCGGeneralizedGradientShader::CreateProc(SkReadBuffer& buffer)
{
    return NULL;
}

bool MCGGeneralizedGradientShaderApply(MCGGeneralizedGradientRef p_gradient, SkPaint& r_paint)
{
    sk_sp<SkShader> t_shader(new MCGGeneralizedGradientShader(p_gradient));
    if (t_shader == nullptr)
    {
        return false;
    }
    
    r_paint.setShader(t_shader);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
