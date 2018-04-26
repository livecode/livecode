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

#ifndef __MC_IQUANTIZATION_H__
#define __MC_IQUANTIZATION_H__


template <class T>
class MCPriorityQueue
{
	typedef int32_t (*MCPQComparator)(T p_a, T p_b);

	class MCPQNode
	{
	public:
		MCPQNode(T p_item, MCPQNode *p_next)
		{
			item = p_item;
			next = p_next;
		}

		T item;
		MCPQNode *next;
	};
public:
	MCPriorityQueue(MCPQComparator p_compare)
	{
		item_count = 0;
		head = NULL;
		compare = p_compare;
	}

	T pop_first()
	{
		T t_item;
		if (head)
		{
			MCPQNode *t_head = head;
			t_item = head->item;
			head = head->next;
			delete t_head;
		}
		else
			t_item = NULL;
		item_count--;
		return t_item;
	}

	void insert_sorted(T p_item)
	{
		MCPQNode *t_current, *t_prev;
		t_current = head;
		t_prev = NULL;
		while (t_current && compare(t_current->item, p_item) > 0)
		{
			t_prev = t_current;
			t_current = t_current->next;
		}

		if (t_prev == NULL)
			head = new (nothrow) MCPQNode(p_item, t_current);
		else
			t_prev->next = new (nothrow) MCPQNode(p_item, t_current);

		item_count++;
	}

	uint32_t count()
	{
		return item_count;
	}

private:
	uint32_t item_count;
	MCPQNode *head;
	MCPQComparator compare;
};

extern bool MCImageParseColourList(MCStringRef p_input, uint32_t &r_ncolours, MCColor *&r_colours);

extern bool MCImageGenerateOptimalPaletteWithWeightedPixels(MCImageBitmap *p_bitmap, uint32_t p_palette_size, MCColor *&r_colours);
extern bool MCImageGenerateWebsafePalette(uint32_t &r_palette_size, MCColor *&r_colours);

#endif
