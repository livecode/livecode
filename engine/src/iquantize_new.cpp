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

#include "parsedef.h"
#include "globals.h"

#include "uidc.h"

#include "imagebitmap.h"
#include "iquantization.h"

#define MC_COLORCHANNELS (3)

typedef struct
{
	union
	{
		uint8_t channel[4];
		uint32_t pixel;
	};
	uint32_t count;
} MCWeightedPixel;

class MCWeightedPixelBox
{
public:
	MCWeightedPixelBox(MCWeightedPixel *p_pixels, uint32_t p_first, uint32_t p_last);
	void shrink_box();
	void find_volume();
	uint32_t nth_element(uint32_t p_channel, uint32_t p_midpoint);
	uint32_t split_axis(uint32_t p_channel, uint32_t p_left, uint32_t p_right, uint32_t p_pivot_value);

	uint32_t population;
	uint32_t pre_count;

	MCWeightedPixel *pixels;
	uint32_t first, last;
	uint8_t channel_min[MC_COLORCHANNELS];
	uint8_t channel_max[MC_COLORCHANNELS];
	uint8_t longest_dim;
	uint8_t longest_axis;
	uint32_t volume;
};

MCWeightedPixelBox::MCWeightedPixelBox(MCWeightedPixel *p_pixels, uint32_t p_first, uint32_t p_last)
{
	pixels = p_pixels;
	first = p_first;
	last = p_last;
	pre_count = 0;
	shrink_box();
	find_volume();
}

void MCWeightedPixelBox::shrink_box()
{
	population = 0;
	for (uint32_t c=0; c<MC_COLORCHANNELS; c++)
		channel_min[c] = channel_max[c] = pixels[first].channel[c];
	for (uint32_t i=first; i<=last; i++)
	{
		population += pixels[i].count;
		for (uint32_t c=0; c<MC_COLORCHANNELS; c++)
		{
			channel_min[c] = MCMin((uint32_t)channel_min[c], (uint32_t)pixels[i].channel[c]);
			channel_max[c] = MCMax((uint32_t)channel_max[c], (uint32_t)pixels[i].channel[c]);
		}
	}
}

void MCWeightedPixelBox::find_volume()
{
	longest_dim = 0;
	longest_axis = 0;
	uint32_t t_diff;
	volume = 1;
	for (uint32_t i=0; i<MC_COLORCHANNELS; i++)
	{
		t_diff = channel_max[i] - channel_min[i];
		volume *= (t_diff + 1);
		if (t_diff > longest_dim)
		{
			longest_dim = t_diff;
			longest_axis = i;
		}
	}
}

static inline void swap_pixels(MCWeightedPixel *p_pixels, uint32_t p_a, uint32_t p_b)
{
	MCWeightedPixel t_swap;
	t_swap = p_pixels[p_a];
	p_pixels[p_a] = p_pixels[p_b];
	p_pixels[p_b] = t_swap;
}

static uint32_t pivot_pixels_by_value(MCWeightedPixel *p_pixels, uint32_t p_channel, uint32_t p_left, uint32_t p_right, uint32_t p_pivot_value, uint32_t &r_weight)
{
	uint32_t t_store_index = p_left;

	r_weight = 0;

	for (uint32_t i=p_left; i<=p_right; i++)
		if (p_pixels[i].channel[p_channel] <= p_pivot_value)
		{
			r_weight += p_pixels[i].count;
			swap_pixels(p_pixels, i, t_store_index++);
		}

	return t_store_index;
}

static uint32_t pivot_pixels(MCWeightedPixel *p_pixels, uint32_t p_channel, uint32_t p_left, uint32_t p_right, uint32_t &r_weight)
{
	uint32_t t_pivot_value = p_pixels[p_right].channel[p_channel];
	uint32_t t_store_index;
	t_store_index = pivot_pixels_by_value(p_pixels, p_channel, p_left, p_right - 1, t_pivot_value, r_weight);

	swap_pixels(p_pixels, t_store_index, p_right); // Move pivot to its final place
	return t_store_index;
}

static uint32_t nth_vbox_element(MCWeightedPixel *p_pixels, uint32_t p_channel, uint32_t p_start, uint32_t p_end, uint32_t p_midpoint)
{
	uint32_t t_pivot_point, t_start, t_end;
	t_pivot_point = p_midpoint;
	t_start = p_start;
	t_end = p_end;
	uint32_t t_weight = 0;
	uint32_t t_new_weight = p_midpoint;
	uint32_t t_target_weight;

	do
	{
		 t_target_weight = t_new_weight;
		if (t_start == t_end)
		{
			t_pivot_point = t_start;
			t_weight = 0;
		}
		else
			t_pivot_point = pivot_pixels(p_pixels, p_channel, t_start, t_end, t_weight);
		if (t_weight > t_target_weight)
		{
			while (t_weight > t_target_weight && p_pixels[t_pivot_point].channel[p_channel] == p_pixels[t_pivot_point - 1].channel[p_channel])
			{
				t_pivot_point--;
				t_weight -= p_pixels[t_pivot_point].count;
			}
			t_end = t_pivot_point - 1;
			t_new_weight = t_target_weight;
		}
		else if (t_weight + p_pixels[t_pivot_point].count <= t_target_weight)
		{
			while (t_weight + p_pixels[t_pivot_point].count <= t_target_weight && p_pixels[t_pivot_point].channel[p_channel] == p_pixels[t_pivot_point + 1].channel[p_channel])
			{
				t_weight += p_pixels[t_pivot_point].count;
				t_pivot_point++;
			}
			t_start = t_pivot_point + 1;
			t_new_weight = t_target_weight - (t_weight + p_pixels[t_pivot_point].count);
		}
	}
	while (t_weight > t_target_weight || t_weight + p_pixels[t_pivot_point].count <= t_target_weight);

	return t_pivot_point;
}

uint32_t MCWeightedPixelBox::nth_element(uint32_t p_channel, uint32_t p_midpoint)
{
	return nth_vbox_element(pixels, p_channel,first, last, p_midpoint);
}

uint32_t MCWeightedPixelBox::split_axis(uint32_t p_channel, uint32_t p_left, uint32_t p_right, uint32_t p_pivot_value)
{
	uint32_t t_weight;
	return pivot_pixels_by_value(pixels, p_channel, p_left, p_right, p_pivot_value, t_weight);
}

static int32_t vbox_compare_population(MCWeightedPixelBox *, MCWeightedPixelBox *) ATTRIBUTE_UNUSED;
static int32_t vbox_compare_longest_dimension(MCWeightedPixelBox *, MCWeightedPixelBox *) ATTRIBUTE_UNUSED;
static int32_t vbox_compare_volume(MCWeightedPixelBox *, MCWeightedPixelBox *) ATTRIBUTE_UNUSED;
static int32_t vbox_compare_volume_population_product(MCWeightedPixelBox *, MCWeightedPixelBox *) ATTRIBUTE_UNUSED;
static int32_t vbox_compare_null(MCWeightedPixelBox *, MCWeightedPixelBox *) ATTRIBUTE_UNUSED;

static int32_t vbox_compare_population(MCWeightedPixelBox *p_a, MCWeightedPixelBox *p_b)
{
	return p_a->population - p_b->population;
}

static int32_t vbox_compare_longest_dimension(MCWeightedPixelBox *p_a, MCWeightedPixelBox *p_b)
{
	return p_a->longest_dim - p_b->longest_dim;
}

static int32_t vbox_compare_volume(MCWeightedPixelBox *p_a, MCWeightedPixelBox *p_b)
{
	return p_a->volume - p_b->volume;
}

static int32_t vbox_compare_volume_population_product(MCWeightedPixelBox *p_a, MCWeightedPixelBox *p_b)
{
	uint32_t t_a_pixel_weight, t_b_pixel_weight;
	uint32_t t_max_pop = MCU_max(p_a->population, p_b->population);
	t_a_pixel_weight = 255 * p_a->population / t_max_pop;
	t_b_pixel_weight = 255 * p_b->population / t_max_pop;
	return (int32_t)((p_a->volume * t_a_pixel_weight) >> 8) - (int32_t)((p_b->volume * t_b_pixel_weight) >> 8);
}

static int32_t vbox_compare_null(MCWeightedPixelBox *p_a, MCWeightedPixelBox *p_b)
{
	return 0;
}

bool MCImageMedianCutQuantization(MCWeightedPixel *p_pixels, uint32_t p_pixel_count, uint32_t p_colour_count, MCWeightedPixel *&r_colourmap)
{
	bool t_success = true;

	// create first box (containing all pixels)
	MCWeightedPixelBox *t_box = new (nothrow) MCWeightedPixelBox(p_pixels, 0, p_pixel_count - 1);

	// create queue containing the box
	MCPriorityQueue<MCWeightedPixelBox*> *t_queue = new (nothrow) MCPriorityQueue<MCWeightedPixelBox*>(vbox_compare_volume_population_product);
	MCPriorityQueue<MCWeightedPixelBox*> *t_finished_queue = new (nothrow) MCPriorityQueue<MCWeightedPixelBox*>(vbox_compare_null);
	if (t_box->volume == 1)
		t_finished_queue->insert_sorted(t_box);
	else
		t_queue->insert_sorted(t_box);
	uint32_t t_count = 1;

	uint32_t f = 0;
	while (t_queue->count() > 0 && t_count < p_colour_count)
	{
		if (f && t_count >= f)
		{
			MCPriorityQueue<MCWeightedPixelBox*> *t_new_queue = new (nothrow) MCPriorityQueue<MCWeightedPixelBox*>(vbox_compare_volume_population_product);
			while (t_queue->count() > 0)
				t_new_queue->insert_sorted(t_queue->pop_first());
			delete t_queue;
			t_queue = t_new_queue;
			f = 0;
		}
		MCWeightedPixelBox *t_box1, *t_box2;
		// pop first box from queue
		t_box = t_queue->pop_first();

		// get the axis in which its dimension is greatest
		// split the box along this axis around the median pixel in the box along that axis
		//uint32_t t_median = (t_box->last + t_box->first) / 2;
		//uint32_t t_median = (t_box->population) / 2;
		uint32_t t_median = ((t_box->pre_count * 2 + t_box->population - 1) / 2) - t_box->pre_count;

		uint32_t t_median_index = t_box->nth_element(t_box->longest_axis, t_median);

		uint32_t t_left_diff, t_right_diff;
		t_left_diff = p_pixels[t_median_index].channel[t_box->longest_axis] - t_box->channel_min[t_box->longest_axis];
		t_right_diff = t_box->channel_max[t_box->longest_axis] - p_pixels[t_median_index].channel[t_box->longest_axis];

		if (t_left_diff > t_right_diff)
		{
			uint32_t t_midpoint_value = ( p_pixels[t_median_index].channel[t_box->longest_axis] + t_box->channel_min[t_box->longest_axis] ) / 2;
			uint32_t t_midpoint_index = t_box->split_axis(t_box->longest_axis, t_box->first, t_median_index, t_midpoint_value) - 1;
			t_box1 = new (nothrow) MCWeightedPixelBox(p_pixels, t_box->first, t_midpoint_index);
			t_box2 = new (nothrow) MCWeightedPixelBox(p_pixels, t_midpoint_index + 1, t_box->last);
            //MCLog("split axis %d by value %d into %d+%d (population %d, first %d)", t_box->longest_axis, t_midpoint_value, t_box1->population, t_box2->population, t_box->population, t_box->first);
		}
		else
		{
			uint32_t t_midpoint_value = ( p_pixels[t_median_index].channel[t_box->longest_axis] + t_box->channel_max[t_box->longest_axis] ) / 2;
			uint32_t t_midpoint_index = t_box->split_axis(t_box->longest_axis, t_median_index, t_box->last, t_midpoint_value) - 1;
			t_box1 = new (nothrow) MCWeightedPixelBox(p_pixels, t_box->first, t_midpoint_index);
			t_box2 = new (nothrow) MCWeightedPixelBox(p_pixels, t_midpoint_index + 1, t_box->last);
            //MCLog("split axis %d by value %d into %d+%d (population %d, first %d)", t_box->longest_axis, t_midpoint_value, t_box1->population, t_box2->population, t_box->population, t_box->first);
		}

		t_box1->pre_count = t_box->pre_count;
		t_box2->pre_count = t_box1->pre_count + t_box1->population;

		// add the two boxes to the queue in reverse order of the size of their largest dimension
		if (t_box1->volume > 1)
			t_queue->insert_sorted(t_box1);
		else
			t_finished_queue->insert_sorted(t_box1);
		if (t_box2->volume > 1)
			t_queue->insert_sorted(t_box2);
		else
			t_finished_queue->insert_sorted(t_box2);
		t_count++;

		delete t_box;
	}
	// generate representative pixel for each box by calculating the mean
	MCWeightedPixel *t_colourmap;
	if (t_success)
		t_success = MCMemoryNewArray<MCWeightedPixel>(p_colour_count, t_colourmap);
	if (t_success)
	{
		uint32_t t_cmap_index = 0;
		while (t_queue->count() > 0)
			t_finished_queue->insert_sorted(t_queue->pop_first());

		while (t_finished_queue->count() > 0)
		{
			t_box = t_finished_queue->pop_first();
			uint32_t t_sum[MC_COLORCHANNELS] = {0, 0, 0};
			for (uint32_t i=t_box->first; i<=t_box->last; i++)
				for (uint32_t c=0; c<MC_COLORCHANNELS; c++)
					t_sum[c] += p_pixels[i].channel[c] * p_pixels[i].count;
			for (uint32_t c=0; c<MC_COLORCHANNELS; c++)
				t_colourmap[t_cmap_index].channel[c] = (t_sum[c] + t_box->population / 2) / t_box->population;
			t_cmap_index++;
			delete t_box;
		}
		r_colourmap = t_colourmap;
	}
	delete t_queue;
	delete t_finished_queue;
	return t_success;
}

int32_t pixel_cmp(const void *a, const void *b)
{
	return ((MCWeightedPixel*)a)->pixel - ((MCWeightedPixel*)b)->pixel;
}

bool MCImageGenerateOptimalPaletteWithWeightedPixels(MCImageBitmap *p_bitmap, uint32_t p_palette_size, MCColor *&r_colours)
{
	bool t_success = true;
	uint32_t t_pixel_count = 0;
	MCWeightedPixel *t_pixels = NULL;

	t_success = MCMemoryNewArray<MCWeightedPixel>(p_bitmap->width * p_bitmap->height, t_pixels);
	MCWeightedPixel *t_colourmap = NULL;

	if (t_success)
	{
		uint8_t *t_src_ptr = (uint8_t*)p_bitmap->data;
		MCColor c;
		for (uint32_t y=0; y<p_bitmap->height; y++)
		{
			uint32_t *t_src_row = (uint32_t*)t_src_ptr;
			for (uint32_t x=0; x<p_bitmap->width; x++)
			{
				if ((*t_src_row >> 24) > 0)
				{
					MCColorSetPixel(c, *t_src_row);
					t_pixels[t_pixel_count].channel[0] = c.red >> 8;
					t_pixels[t_pixel_count].channel[1] = c.green >> 8;
					t_pixels[t_pixel_count].channel[2] = c.blue >> 8;
					t_pixel_count++;
				}
				t_src_row++;
			}
			t_src_ptr += p_bitmap->stride;
		}
		
		if (t_pixel_count > 1)
		{
			qsort(t_pixels, t_pixel_count, sizeof(MCWeightedPixel), pixel_cmp);
			t_pixels[0].count = 1;
			uint32_t t_last_pixel = t_pixels[0].pixel;
			uint32_t t_last_index = 0;
			for (uint32_t i=1; i<t_pixel_count; i++)
			{
				if (t_pixels[i].pixel == t_last_pixel)
					t_pixels[t_last_index].count++;
				else
				{
					t_last_index++;
					t_last_pixel = t_pixels[i].pixel;
					t_pixels[t_last_index].pixel = t_last_pixel;
					t_pixels[t_last_index].count = 1;
				}
			}
			t_pixel_count = t_last_index + 1;
		}
		if (t_pixel_count <= p_palette_size)
		{
			t_colourmap = t_pixels;
		}
		else
		{
			t_success = MCImageMedianCutQuantization(t_pixels, t_pixel_count, p_palette_size, t_colourmap);
			MCMemoryDeleteArray(t_pixels);
		}
	}

	if (t_success && r_colours == NULL)
		t_success = MCMemoryNewArray<MCColor>(p_palette_size, r_colours);

	if (t_success)
	{
		for (uint32_t i=0; i<p_palette_size; i++)
		{
			r_colours[i].red = t_colourmap[i].channel[0] << 8 | t_colourmap[i].channel[0];
			r_colours[i].green = t_colourmap[i].channel[1] << 8 | t_colourmap[i].channel[1];
			r_colours[i].blue = t_colourmap[i].channel[2] << 8 | t_colourmap[i].channel[2];
		}
	}
	if (t_colourmap != NULL)
		MCMemoryDeleteArray(t_colourmap);

	return t_success;
}
