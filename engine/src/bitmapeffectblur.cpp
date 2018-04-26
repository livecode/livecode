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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "packed.h"

#include "bitmapeffectblur.h"

////////////////////////////////////////////////////////////////////////////////

// This is the MCBitmapEffectBlur opaque type definition. Its implemented as a
// C++ interface.
//
struct MCBitmapEffectBlur
{
	// We make the destructor virtual, although cleanup should happen in Finalize.
	virtual ~MCBitmapEffectBlur(void) {}
	
	// MP-2013-02-05: [[ x64 ]] Change strides to be signed to avoid problems with
	//   ptr arithmetic and promotions in 64-bit.
	// This method is called by BlurBegin after its setup the common state.
	virtual bool Initialize(const MCBitmapEffectBlurParameters& params, const MCRectangle& input_rect, const MCRectangle& output_rect, uint32_t *src_pixels, int32_t src_stride) = 0;

	// This method is called by BlurContinue to produce the next scanline.
	virtual void Process(uint8_t *mask) = 0;

	// This method is called by BlurEnd to finish a blur.
	virtual void Finalize(void) = 0;
};

////////////////////////////////////////////////////////////////////////////////

// The factory method is defined at the bottom of the file, after the various
// blur implementations :o)
static bool MCBitmapEffectBlurFactory(MCBitmapEffectFilter type, MCBitmapEffectBlur*& r_blur);

// MP-2013-02-05: [[ x64 ]] Change strides to be signed to avoid problems with
//   ptr arithmetic and promotions in 64-bit.
bool MCBitmapEffectBlurBegin(const MCBitmapEffectBlurParameters& p_params, const MCRectangle& p_input_rect, const MCRectangle& p_output_rect, uint32_t *p_src_pixels, int32_t p_src_stride, MCBitmapEffectBlurRef& r_blur)
{
	MCBitmapEffectBlur *t_blur;
	if (!MCBitmapEffectBlurFactory(p_params.filter, t_blur))
		return false;

	if (!t_blur -> Initialize(p_params, p_input_rect, p_output_rect, p_src_pixels, p_src_stride))
	{
		delete t_blur;
		return false;
	}

	r_blur = t_blur;

	return true;
}

void MCBitmapEffectBlurContinue(MCBitmapEffectBlurRef blur, uint8_t *mask)
{
	blur -> Process(mask);
}

void MCBitmapEffectBlurEnd(MCBitmapEffectBlurRef blur)
{
	blur -> Finalize();

	delete blur;
}

////////////////////////////////////////////////////////////////////////////////

// This is the (original) naive gaussian blur implementation. Note that the 
// state is indirected through a separate structure because it was refactored
// from differently structured code. This is largely irrelevant though, since
// we want to replace it with a faster version based on the fact the Gaussian
// convoluton filter is separable :o)

struct MCBitmapEffectGaussianBlurState
{
	int32_t radius;
	uint32_t *kernel;
	
	// MP-2013-02-05: [[ x64 ]] Change strides to be signed to avoid problems with
	//   ptr arithmetic and promotions in 64-bit.
	uint32_t *pixels;
	int32_t stride;

	int32_t width;
	int32_t height;

	int32_t left, top, right, bottom;

	int32_t y;
};

struct MCBitmapEffectGaussianBlur: public MCBitmapEffectBlur
{
	// MP-2013-02-05: [[ x64 ]] Change strides to be signed to avoid problems with
	//   ptr arithmetic and promotions in 64-bit.
	bool Initialize(const MCBitmapEffectBlurParameters& params, const MCRectangle& input_rect, const MCRectangle& output_rect, uint32_t *src_pixels, int32_t src_stride);
	void Process(uint8_t *mask);
	void Finalize(void);

	MCBitmapEffectGaussianBlurState state;
};

static inline float _gaussian_value(int32_t i, float r)
{
	float x;
	x = 3 * (i + 1) / r;
	return (float)exp(-x * x / 2.);
}

static inline float _gaussian_function(int32_t i, float r)
{
	return (float) (_gaussian_value(i, r) * 3 / (sqrt(2 * M_PI) * r));
}

static float *MCBitmapEffectComputeGaussianKernel(uint4 r)
{
	uint4 t_width;
	t_width = r * 2 + 1;

	float *lk;
	lk = new (nothrow) float[t_width];
	if (lk == NULL)
		return NULL;

	for(uint4 i = 0; i < r + 1; i++)
	{
		float v;
		v = _gaussian_value(i, float(r));
		lk[i] = v;
		lk[t_width - 1 - i] = v;
	}
	return lk;
}

static uint32_t *MCBitmapEffectComputeBlurKernel(uint4 r)
{
	uint4 t_width;
	t_width = r * 2 + 1;

	float *lk;
	lk = MCBitmapEffectComputeGaussianKernel(r);
	
	float *k, t_sum;
	k = new (nothrow) float[t_width * t_width];
	t_sum = 0.0;
	for(uint4 i = 0; i < t_width; i++)
		for(uint4 j = 0; j < t_width; j++)
		{
			float v;
			v = lk[i] * lk[j];
			k[i * t_width + j] = 1.0f - v;
			t_sum += 1.0f - v;
		}

	uint32_t *ik;
	ik = new (nothrow) uint32_t[t_width * t_width];
	for(uint4 i = 0; i < t_width; i++)
		for(uint4 j = 0; j < t_width; j++)
			ik[i * t_width + j] = (uint32_t)(k[i * t_width + j] * 0x1000000 / t_sum);
	
	delete[] lk;
	delete[] k;
	
	return ik;
}

// MP-2013-02-05: [[ x64 ]] Change strides to be signed to avoid problems with
//   ptr arithmetic and promotions in 64-bit.
bool MCBitmapEffectGaussianBlur::Initialize(const MCBitmapEffectBlurParameters& params, const MCRectangle& input_rect, const MCRectangle& output_rect, uint32_t *src_pixels, int32_t src_stride)
{
	state . radius = params . radius;
	if (params . radius != 0)
		state . kernel = MCBitmapEffectComputeBlurKernel(params . radius);
	else
		state . kernel = 0;

	state . width = output_rect . width;
	state . height = output_rect . height;
	state . left = input_rect . x - output_rect . x;
	state . top = input_rect . y - output_rect . y;
	state . right = state . left + input_rect . width;
	state . bottom = state . top + input_rect . height;
	state . pixels = src_pixels;
	state . stride = src_stride;
	state . y = 0;

	return true;
}

void MCBitmapEffectGaussianBlur::Process(uint8_t *mask)
{
	if (state . kernel != NULL)
	{
		int32_t t_top, t_bottom;
		t_top = MCU_max(-state . radius, state . top - state . y);
		t_bottom = MCU_min(state . radius, state . bottom - state . y - 1);

		int32_t t_vcount;
		t_vcount = t_bottom - t_top + 1;


		uint32_t *t_kernel;
		t_kernel = state . kernel + (state . radius + t_top) * (state . radius * 2 + 1) + state . radius;

		uint32_t *t_pixels;
		t_pixels = state . pixels + state . stride * t_top;

		for(int32_t x = 0; x < state . width; x++)
		{
			int32_t t_left, t_right;
			t_left = MCU_max(-state . radius, state . left - x);
			t_right = MCU_min(state . radius, state . right - x - 1);

			int32_t t_hcount;
			t_hcount = t_right - t_left;

			uint32_t *t_kernel_ptr;
			t_kernel_ptr = t_kernel + t_left;

			uint32_t *t_pixel_ptr;
			t_pixel_ptr = t_pixels + x + t_left;

			uint32_t t_alpha;
			t_alpha = 0;

			for(int32_t k = t_vcount; k > 0; k--)
			{
				for(int32_t j = t_hcount; j >= 0; j--)
					t_alpha += t_kernel_ptr[j] * (t_pixel_ptr[j] >> 24);

				t_pixel_ptr += state . stride;
				t_kernel_ptr += state . radius * 2 + 1;
			}

			mask[x] = t_alpha >> 24;
		}
	}
	else
	{
		if (state . y >= state . top && state . y < state . bottom)
		{
			for(int32_t x = 0; x < state . left; x++)
				mask[x] = 0;
			for(int32_t x = state . left; x < MCU_min(state . width, state . right); x++)
				mask[x] = state . pixels[x] >> 24;
			for(int32_t x = state . right; x < state . width; x++)
				mask[x] = 0;
		}
	}

	state . y += 1;
	state . pixels += state . stride;
}

void MCBitmapEffectGaussianBlur::Finalize(void)
{
	delete[] state . kernel;
}

////////////////////////////////////////////////////////////////////////////////

// This is an implementation of the gaussian blur convolution filter,
// using the fact that such a filter has the 'separable' property.
//
// A naive version of this algorithm will require an intermediate buffer the full
// size of the output_rect. However, a cleverer version could make do (I believe)
// with one of size output_rect . width * (2 * radius + 1).

struct MCBitmapEffectFastGaussianBlur: public MCBitmapEffectBlur
{
	// MP-2013-02-05: [[ x64 ]] Change strides to be signed to avoid problems with
	//   ptr arithmetic and promotions in 64-bit.
	bool Initialize(const MCBitmapEffectBlurParameters& params, const MCRectangle& input_rect, const MCRectangle& output_rect, uint32_t *src_pixels, int32_t src_stride);
	void Process(uint8_t *mask);
	void Finalize(void);

	int32_t radius;
	uint32_t *kernel;
	
	// MP-2013-02-05: [[ x64 ]] Change strides to be signed to avoid problems with
	//   ptr arithmetic and promotions in 64-bit.
	uint32_t *pixels;
	int32_t stride;

	int32_t width;
	int32_t height;

	int32_t left, top, right, bottom;

	int32_t y;
	
	// MP-2013-02-05: [[ x64 ]] Change strides to be signed to avoid problems with
	//   ptr arithmetic and promotions in 64-bit.
	uint32_t *buffer;
	int32_t buffer_stride;
	uint32_t buffer_height;
	uint32_t buffer_nextrow;
};

// MP-2013-02-05: [[ x64 ]] Change strides to be signed to avoid problems with
//   ptr arithmetic and promotions in 64-bit.
bool MCBitmapEffectFastGaussianBlur::Initialize(const MCBitmapEffectBlurParameters& params, const MCRectangle& input_rect, const MCRectangle& output_rect, uint32_t *src_pixels, int32_t src_stride)
{
	radius = params . radius;

	uint32_t t_width;
	t_width = params.radius * 2 + 1;

	uint32_t t_spread_radius;
	t_spread_radius = radius * (255 - params.spread) / 255;
	if (params . radius != 0)
	{
		float *lk = MCBitmapEffectComputeGaussianKernel(t_spread_radius);
		kernel = new (nothrow) uint32_t[t_width];
		float t_sum = 0;
		for (uint32_t i=0; i<(t_spread_radius * 2 + 1); i++)
		{
			lk[i] = 1.0f - lk[i];
			t_sum += lk[i];
		}
		for (uint32_t i=0; i<t_spread_radius; i++)
		{
			kernel[i] = (uint32_t) (lk[i] * 0x10000 / t_sum);
			kernel[t_width - i - 1] = kernel[i];
		}
		for (uint32_t i=t_spread_radius; i<t_width - t_spread_radius; i++)
			kernel[i] = (uint32_t) (lk[t_spread_radius] * 0x10000 / t_sum);

		// MW-2009-08-24: Memory leak :o)
		delete lk;
	}
	else
		kernel = 0;

	width = output_rect . width;
	height = output_rect . height;
	left = input_rect . x - output_rect . x;
	top = input_rect . y - output_rect . y;
	right = left + input_rect . width;
	bottom = top + input_rect . height;
	stride = src_stride;
	y = 0;

	buffer_height = t_width;
	buffer_nextrow = top;
	buffer = new (nothrow) uint32_t[buffer_height * output_rect.width];
	buffer_stride = output_rect.width;

	if (radius > 0)
		pixels = src_pixels + stride * buffer_nextrow;
	else
		pixels = src_pixels;
	return true;
}

void MCBitmapEffectFastGaussianBlur::Process(uint8_t *mask)
{
	if (kernel != NULL)
	{
		if (y >= (top - radius) && (y < bottom + radius))
		{
			int32_t t_top, t_bottom;
			t_top = MCU_max(-radius, top - y); // min offset from kernel midpoint
			t_bottom = MCU_min(bottom - y - 1, radius); // max offset from kernel midpoint

			int32_t t_vcount;
			t_vcount = t_bottom - t_top + 1;


			uint32_t *t_kernel;
			t_kernel = kernel + radius; // point to kernel midpoint

			// calculate any new buffer rows needed for this one
			for (int32_t t_y = buffer_nextrow; t_y <= t_bottom + y; t_y++)
			{
				uint32_t *t_mask = buffer + (buffer_stride * ((t_y + buffer_height) % buffer_height));

				for(int32_t x = 0; x < width; x++)
				{
					int32_t t_left, t_right;
					t_left = MCU_max(-radius, left - x);
					t_right = MCU_min(right - x - 1, radius);

					int32_t t_hcount;
					t_hcount = t_right - t_left;

					uint32_t *t_kernel_ptr;
					t_kernel_ptr = t_kernel + t_left;

					uint32_t *t_pixel_ptr;
					t_pixel_ptr = pixels + x + t_left;

					uint32_t t_alpha;
					t_alpha = 0;

					for(int32_t j = t_hcount; j >= 0; j--)
					{
						uint32_t t_weighted = t_kernel_ptr[j] * (t_pixel_ptr[j] >> 24);
						if ((0xFFFFFFFF - t_alpha) > t_weighted)
							t_alpha += t_weighted;
						else
							t_alpha = 0xFFFFFFFF;
					}

					t_mask[x] = t_alpha ;
				}
				pixels += stride;
			}
			buffer_nextrow = t_bottom + y + 1;

			uint32_t *t_buffer_end;
			t_buffer_end = buffer + (buffer_stride * (buffer_height - 1));

			uint32_t *t_buffer;
			t_buffer = buffer + (buffer_stride * ((t_top + y + buffer_height) % buffer_height));

			for(int32_t x = 0; x < width; x++)
			{

				uint32_t *t_kernel_ptr;
				t_kernel_ptr = t_kernel + t_top;

				uint32_t t_alpha;
				t_alpha = 0;

				uint32_t *t_buffer_ptr;
				t_buffer_ptr = t_buffer;

                for (int32_t k = 0; k < t_vcount; k++)
				{
					uint32_t t_weighted = t_kernel_ptr[k] * (t_buffer_ptr[x] >> 16);
					if ((0xFFFFFFFF - t_alpha) > t_weighted)
						t_alpha += t_weighted;
					else
						t_alpha = 0xFFFFFFFF;
					if (t_buffer_ptr < t_buffer_end)
						t_buffer_ptr += buffer_stride;
					else
						t_buffer_ptr = buffer;
				}

				if (t_alpha < 0x1000000)
					mask[x] = t_alpha >> 16;
				else
					mask[x] = 0xFF;
			}
		}
		else
		{
			for (int32_t x=0; x < width; x++)
				mask[x] = 0;
		}
	}
	else
	{
		if (y >= top && y < bottom)
		{
			int32_t x;
			x = 0;

			int32_t t_left, t_right;
			t_left = MCU_min(width, left);
			t_right = MCU_min(width, right);

			for(; x < t_left; x++)
				mask[x] = 0;
			for(; x < t_right; x++)
				mask[x] = pixels[x] >> 24;
			for(; x < width; x++)
				mask[x] = 0;
		}
		else
			for(int32_t x = 0; x < width; x++)
				mask[x] = 0;
		pixels += stride;
	}

	y += 1;
}

void MCBitmapEffectFastGaussianBlur::Finalize(void)
{
	delete [] kernel;
	delete [] buffer;
}

////////////////////////////////////////////////////////////////////////////////

// This is an implementation of a 'box' blur. The algorithm is applied 'passes'
// times.  3 passes should give a close approximation to gaussian blur.
//
// This version of the algorithm uses buffers for each pass totalling
// approximately output_rect . width * (2 * radius + passes)

struct MCBitmapEffectBoxBlurPassInfo
{
	uint32_t *buffer;

	int32_t left, top, right, bottom;

	int32_t radius;
	int32_t window;

	int32_t width;
	int32_t stride;
	uint32_t height;
	uint32_t buffer_height;

	int32_t buffer_nextrow;
	int32_t buffer_needrow;
};

struct MCBitmapEffectBoxBlur: public MCBitmapEffectBlur
{
	// MP-2013-02-05: [[ x64 ]] Change strides to be signed to avoid problems with
	//   ptr arithmetic and promotions in 64-bit.
	bool Initialize(const MCBitmapEffectBlurParameters& params, const MCRectangle& input_rect, const MCRectangle& output_rect, uint32_t *src_pixels, int32_t src_stride);
	void Process(uint8_t *mask);
	void Finalize(void);

	////

	void CalculateRows(uint32_t p_pass);

	MCBitmapEffectBoxBlur(uint32_t passes)
	{
		m_passes = MCU_max(passes, 1u);
	}

	uint32_t m_passes;

	int32_t y;

	int32_t width;
	int32_t height;

	int32_t top, bottom, left, right;
	
	// MP-2013-02-05: [[ x64 ]] Change strides to be signed to avoid problems with
	//   ptr arithmetic and promotions in 64-bit.
	uint32_t *pixels;
	int32_t stride;

	MCBitmapEffectBoxBlurPassInfo *pass_info;

	uint32_t *row_buffer;

	uint32_t spread;
};

// MP-2013-02-05: [[ x64 ]] Change strides to be signed to avoid problems with
//   ptr arithmetic and promotions in 64-bit.
bool MCBitmapEffectBoxBlur::Initialize(const MCBitmapEffectBlurParameters& params, const MCRectangle& input_rect, const MCRectangle& output_rect, uint32_t *src_pixels, int32_t src_stride)
{
	width = output_rect . width;
	height = output_rect . height;

	left = input_rect . x - output_rect . x;
	top = input_rect . y - output_rect . y;
	right = left + input_rect . width;
	bottom = top + input_rect . height;

	stride = src_stride;

	int32_t t_radius;
	t_radius = params.radius;

	if (width == 0 || height == 0)
		t_radius = 0;

	// use the gaussian function (slightly tweaked with *MAGIC NUMBERS*)
	// to get the scale factor for the spread value
	spread = (uint32_t)(256 / _gaussian_value(t_radius * params.spread / 160, (float)t_radius));

	y = 0;

	int32_t t_buff_left, t_buff_top, t_buff_right, t_buff_bottom;
	t_buff_top = -t_radius;
	t_buff_bottom = height + t_radius;

	t_buff_left = -t_radius;
	t_buff_right = width + t_radius;

	int32_t t_left, t_top, t_right, t_bottom;
	t_top = top;
	t_bottom = bottom;
	t_left = left;
	t_right = right;

	// test for input + blur radius overlapping with output
	if (MCU_min(right, t_buff_right) > MCU_max(left, t_buff_left)
		&& MCU_min(bottom, t_buff_bottom) > MCU_max(top, t_buff_top))
		;// valid, yay!
	else
		m_passes = 0;

	// skip zero-radius passes
	if (t_radius == 0)
		m_passes = 0;

	pass_info = new (nothrow) MCBitmapEffectBoxBlurPassInfo[m_passes];

	int32_t t_maxwidth = 0;
	for (uint32_t i=0; i<m_passes; i++)
	{
		MCBitmapEffectBoxBlurPassInfo *t_pass;
		t_pass = &pass_info[i];

		t_pass->radius = (t_radius + (m_passes - i - 1)) / (m_passes - i);
		t_radius -= t_pass->radius;

		t_pass->window = t_pass->radius * 2 + 1;
		t_pass->left = MCU_max(t_left, t_buff_left);
		t_pass->top = MCU_max(t_top, t_buff_top);
		t_pass->right = MCU_min(t_right, t_buff_right);
		t_pass->bottom = MCU_min(t_bottom, t_buff_bottom);

		assert(t_pass->right >= t_pass->left && t_pass->bottom > t_pass->top);

		// calculate how much buffer space each pass needs

		// this alg requires an extra zero-ed row at the top
		// and column to the left
		t_pass->width = t_pass->right - t_pass->left;
		t_pass->stride = t_pass->width + 1;
		t_pass->height = t_pass->bottom - t_pass->top;
		t_pass->buffer_height = t_pass->height + 1;
		t_pass->buffer = new (nothrow) uint32_t[(t_pass->stride) * t_pass->buffer_height];
		memset(pass_info[i].buffer, 0, (t_pass->stride) * t_pass->buffer_height * sizeof(uint32_t));

		t_pass->buffer_nextrow = t_pass->top;
		t_pass->buffer_needrow = t_pass->buffer_nextrow - 1;

		t_maxwidth = MCU_max(t_pass->width, t_maxwidth);

		t_left -= t_pass->radius;
		t_right += t_pass->radius;
		t_top -= t_pass->radius;
		t_bottom += t_pass->radius;

		t_buff_left += t_pass->radius;
		t_buff_top += t_pass->radius;
		t_buff_right -= t_pass->radius;
		t_buff_bottom -= t_pass->radius;
	}

	row_buffer = new (nothrow) uint32_t[t_maxwidth];

	if (m_passes > 0)
		pixels = src_pixels + stride * pass_info[0].top;
	else
		pixels = src_pixels;

	return true;
}

void MCBitmapEffectBoxBlur::CalculateRows(uint32_t p_pass)
{
	MCBitmapEffectBoxBlurPassInfo *t_pass = &pass_info[p_pass];
	MCBitmapEffectBoxBlurPassInfo *t_prev_pass;

	if (p_pass > 0)
		t_prev_pass = &pass_info[p_pass - 1];

	uint32_t *t_sum_buffer_row, *t_sum_buffer_prev_row, *t_sum_buffer_final_row;
	t_sum_buffer_final_row = t_pass->buffer + (t_pass->stride * (t_pass->buffer_height - 1));
	int32_t t_nextbufferrow = t_pass->buffer_nextrow - 1;
	while (t_nextbufferrow < 0)
		t_nextbufferrow += t_pass->buffer_height;
	t_sum_buffer_prev_row = t_pass->buffer + (t_pass->stride * (t_nextbufferrow % t_pass->buffer_height));

	uint32_t *t_row_buffer;

	t_row_buffer = row_buffer;

	for (int32_t t_y = t_pass->buffer_nextrow; t_y <= t_pass->buffer_needrow; t_y++)
	{
		// get pointer for the position in our buffer where the new
		// sum totals will go, and also the position in the buffer of the previous
		// line

		if (p_pass != 0)
		{
			// fill the temporary row buffer by calculating the blurred pixel
			// values using the sum buffer from the previous pass

			int32_t t_top, t_bottom;
			t_top = MCU_max(-t_prev_pass->radius, t_prev_pass->top - t_y); // min offset from kernel midpoint
			t_bottom = MCU_min(t_prev_pass->bottom - t_y - 1, t_prev_pass->radius); // max offset from kernel midpoint

			t_prev_pass->buffer_needrow = t_bottom + t_y ;

			// calculate the required sum buffer rows for the previous pass
			CalculateRows(p_pass - 1);

			uint32_t *t_buff_top, *t_buff_bottom;
            t_nextbufferrow = t_y + t_top - 1;
			while (t_nextbufferrow < 0)
				t_nextbufferrow += t_prev_pass->buffer_height;
			t_buff_top = t_prev_pass->buffer + (t_prev_pass->stride * (t_nextbufferrow % t_prev_pass->buffer_height));
			t_nextbufferrow = t_y + t_bottom;
			while (t_nextbufferrow < 0)
				t_nextbufferrow += t_prev_pass->buffer_height;
			t_buff_bottom = t_prev_pass->buffer + (t_prev_pass->stride * (t_nextbufferrow % t_prev_pass->buffer_height));

			int32_t t_rel_left, t_rel_right;
			t_rel_left = t_prev_pass->left - t_pass->left;
			t_rel_right = t_prev_pass->right - t_pass->left;

			t_buff_top -= t_rel_left;
			t_buff_bottom -= t_rel_left;

			t_buff_top += 1;
			t_buff_bottom += 1;

			int32_t t_area;
			t_area = t_prev_pass->window * t_prev_pass->window;
			for(int32_t x = 0; x < t_pass->width; x++)
			{
				// calculate total = p(x-r, y-r) + p(x+r, y+r) - p(x+r, y-r) - p(x-r, y+r)
				int32_t t_left, t_right;
				t_left = MCU_max(-t_prev_pass->radius, t_rel_left - x);
				t_left -= 1;
				t_right = MCU_min(t_rel_right - x - 1, t_prev_pass->radius);
				t_row_buffer[x] = (t_buff_top[x + t_left] + t_buff_bottom[x + t_right] - t_buff_top[x + t_right] - t_buff_bottom[x + t_left]) / t_area;
			}
		}
		else
		{
			// copy alpha component to byte buffer
			uint32_t *t_pixel_ptr;
			t_pixel_ptr = pixels + t_pass->left;

			pixels += stride;
			for(int32_t x=0; x<t_pass->width; x++)
				t_row_buffer[x] = t_pixel_ptr[x] >> 24;
		}

		if (t_sum_buffer_prev_row < t_sum_buffer_final_row)
			t_sum_buffer_row = t_sum_buffer_prev_row + t_pass->stride;
		else
			t_sum_buffer_row = t_pass->buffer;

		// compute cumulative sum for this row using previous sum buffer row and
		// the temporary row buffer
		int32_t t_alpha = 0;
		for(int32_t x = 0; x < t_pass->width; x++)
		{
			t_alpha += t_row_buffer[x];
			t_alpha += t_sum_buffer_prev_row[x+1];

			t_alpha -= t_sum_buffer_prev_row[x - 1 + 1];
			t_sum_buffer_row[x+1] = t_alpha;
		}
		t_sum_buffer_prev_row = t_sum_buffer_row;
	}
	t_pass->buffer_nextrow = t_pass->buffer_needrow + 1;
}

void MCBitmapEffectBoxBlur::Process(uint8_t *mask)
{
	if (m_passes > 0)
	{
		uint32_t t_pass_index;
		t_pass_index = m_passes - 1;

		MCBitmapEffectBoxBlurPassInfo *t_pass;
		t_pass = &pass_info[t_pass_index];

		if (y >= (t_pass->top - t_pass->radius)
			&& y < (t_pass->bottom + t_pass->radius))
		{
			int32_t t_top, t_bottom;
			t_top = MCU_max(-t_pass->radius, t_pass->top - y); // min offset from kernel midpoint
			t_bottom = MCU_min(t_pass->bottom - y - 1, t_pass->radius); // max offset from kernel midpoint

			t_pass->buffer_needrow = t_bottom + y ;

			CalculateRows(t_pass_index);

			uint32_t *t_buff_top, *t_buff_bottom;
			int32_t t_nextbufferrow = y + t_top - 1;
			while (t_nextbufferrow < 0)
				t_nextbufferrow += t_pass->buffer_height;
			t_buff_top = t_pass->buffer + (t_pass->stride * (t_nextbufferrow % t_pass->buffer_height));
			t_nextbufferrow = y + t_bottom;
			while (t_nextbufferrow < 0)
				t_nextbufferrow += t_pass->buffer_height;
			t_buff_bottom = t_pass->buffer + (t_pass->stride * (t_nextbufferrow % t_pass->buffer_height));

			assert(t_buff_top[0] == 0 && t_buff_bottom[0] == 0);
			t_buff_top -= t_pass->left;
			t_buff_bottom -= t_pass->left;
			
			t_buff_top += 1;
			t_buff_bottom += 1;

			int32_t t_area;
			t_area = t_pass->window * t_pass->window;

			int32_t x;
			x = 0;

			int32_t t_sleft, t_sright;
			t_sleft = t_pass->left - t_pass->radius;
			t_sright = MCU_min(width, t_pass->right + t_pass->radius + 1);

			// MW-2012-04-05: [[ Bug 10146 ]] It is possible for spread to be 0, if it is
			//   then the mask is all 0xff.
			// calculate minimum sum for which sum * spread / area
			// is greater than the maximum value
			int32_t t_max_val;
			if (spread != 0)
				t_max_val = 0x100 * t_area * 256 / spread;
			else
				t_max_val = 0;

			for(; x < t_sleft; x++)
				mask[x] = 0;
			for(; x < t_sright; x++)
			{
				// calculate total = p(x-r, y-r) + p(x+r, y+r) - p(x+r, y-r) - p(x-r, y+r)
				int32_t t_left, t_right;
				t_left = MCU_max(-t_pass->radius, t_pass->left - x);
				t_left -= 1;
				t_right = MCU_min(t_pass->right - x - 1, t_pass->radius);

				int32_t t_sum;
				t_sum = t_buff_top[x + t_left] + t_buff_bottom[x + t_right] - t_buff_top[x + t_right] - t_buff_bottom[x + t_left];
				if (t_sum <= t_max_val)
					mask[x] = t_sum * spread / (t_area * 256);
				else
					mask[x] = 0xFF;
			}
			for(; x < width; x++)
				mask[x] = 0;
		}
		else
		{
			for (int32_t x=0; x < width; x++)
				mask[x] = 0;
		}
	}
	else
	{
		if (y >= top && y < bottom)
		{
			int32_t x;
			x = 0;

			int32_t t_left, t_right;
			t_left = MCU_min(width, left);
			t_right = MCU_min(width, right);

			for(; x < t_left; x++)
				mask[x] = 0;
			for(; x < t_right; x++)
				mask[x] = pixels[x] >> 24;
			for(; x < width; x++)
				mask[x] = 0;
		}
		else
		{
			for (int32_t x=0; x < width; x++)
				mask[x] = 0;
		}
		pixels += stride;
	}

	y += 1;
}

void MCBitmapEffectBoxBlur::Finalize(void)
{
	for (uint32_t i=0; i<m_passes; i++)
		delete [] pass_info[i].buffer;
	delete [] row_buffer;
	delete [] pass_info;
}

////////////////////////////////////////////////////////////////////////////////

static bool MCBitmapEffectBlurFactory(MCBitmapEffectFilter p_type, MCBitmapEffectBlur*& r_blur)
{
	switch(p_type)
	{
	case kMCBitmapEffectFilterFastGaussian:
		r_blur = new (nothrow) MCBitmapEffectFastGaussianBlur;
		return true;

	case kMCBitmapEffectFilterOnePassBox:
		r_blur = new (nothrow) MCBitmapEffectBoxBlur(1);
		return true;

	case kMCBitmapEffectFilterTwoPassBox:
		r_blur = new (nothrow) MCBitmapEffectBoxBlur(2);
		return true;

	case kMCBitmapEffectFilterThreePassBox:
		r_blur = new (nothrow) MCBitmapEffectBoxBlur(3);
		return true;

	default:
		break;
	}

	return false;
}
