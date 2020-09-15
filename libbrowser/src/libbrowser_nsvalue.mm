/* Copyright (C) 2019 LiveCode Ltd.
 
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

#import <Foundation/Foundation.h>

#include <core.h>

#include "libbrowser_internal.h"
#include "libbrowser_nsvalue.h"

bool MCNSObjectToBrowserValue(id p_obj, MCBrowserValue &r_value, MCCustomNSObjectConverter p_converter)
{
	if ([p_obj isKindOfClass: [NSString class]])
		return MCBrowserValueSetUTF8String(r_value, [(NSString*)p_obj cStringUsingEncoding: NSUTF8StringEncoding]);
	else if ([p_obj isKindOfClass: [NSNumber class]])
		return MCNSNumberToBrowserValue((NSNumber*)p_obj, r_value);
	else if ([p_obj isKindOfClass: [NSDictionary class]])
		return MCNSDictionaryToBrowserValue((NSDictionary*)p_obj, r_value);
	else if ([p_obj isKindOfClass: [NSArray class]])
		return MCNSArrayToBrowserValue((NSArray*)p_obj, r_value);
	
	if (p_converter != nil)
		return p_converter(p_obj, r_value);
	
	return false;
}

bool MCNSDictionaryToBrowserDictionary(NSDictionary *p_dictionary, MCBrowserDictionaryRef &r_dict, MCCustomNSObjectConverter p_converter)
{
	__block bool t_success;
	t_success = true;
	
	__block MCBrowserDictionaryRef t_dict;
	t_dict = nil;
	
	if (t_success)
		t_success = MCBrowserDictionaryCreate(t_dict, [p_dictionary count]);
	
	if (t_success)
		[p_dictionary enumerateKeysAndObjectsUsingBlock: ^(id p_key, id p_obj, BOOL *r_stop) {
			MCBrowserValue t_value;
			MCBrowserMemoryClear(&t_value, sizeof(MCBrowserValue));
			
			t_success = [p_key isKindOfClass: [NSString class]];
			
			if (t_success)
				t_success = MCNSObjectToBrowserValue(p_obj, t_value, p_converter);
			
			if (t_success)
				t_success = MCBrowserDictionarySetValue(t_dict, [(NSString*)p_key cStringUsingEncoding: NSUTF8StringEncoding], t_value);
			
			if (!t_success)
				*r_stop = YES;
			
			MCBrowserValueClear(t_value);
		}];
	
	if (t_success)
		r_dict = t_dict;
	else
		MCBrowserDictionaryRelease(t_dict);
	
	return t_success;
}

bool MCNSArrayToBrowserList(NSArray *p_array, MCBrowserListRef &r_list, MCCustomNSObjectConverter p_converter)
{
	__block bool t_success;
	t_success = true;
	
	__block MCBrowserListRef t_list;
	t_list = nil;
	
	if (t_success)
		t_success = MCBrowserListCreate(t_list, [p_array count]);
	
	if (t_success)
		[p_array enumerateObjectsUsingBlock: ^(id p_obj, NSUInteger p_index, BOOL *r_stop) {
			MCBrowserValue t_value;
			MCBrowserMemoryClear(&t_value, sizeof(MCBrowserValue));
			
			t_success = MCNSObjectToBrowserValue(p_obj, t_value, p_converter);
			
			if (t_success)
				t_success = MCBrowserListSetValue(t_list, p_index, t_value);
			
			if (!t_success)
				*r_stop = YES;
			
			MCBrowserValueClear(t_value);
		}];
	
	if (t_success)
		r_list = t_list;
	else
		MCBrowserListRelease(t_list);
	
	return t_success;
}

bool MCNSNumberToBrowserValue(NSNumber *p_number, MCBrowserValue &r_value)
{
	if (p_number == [NSNumber numberWithBool:YES])
		return MCBrowserValueSetBoolean(r_value, true);
	else if (p_number == [NSNumber numberWithBool:NO])
		return MCBrowserValueSetBoolean(r_value, false);
	else if (MCCStringEqual([p_number objCType], @encode(int)))
		return MCBrowserValueSetInteger(r_value, [p_number intValue]);
	else
		return MCBrowserValueSetDouble(r_value, [p_number doubleValue]);
}

//////////

bool MCNSDictionaryToBrowserValue(NSDictionary *p_dictionary, MCBrowserValue &r_value, MCCustomNSObjectConverter p_converter)
{
	bool t_success;
	t_success = true;
	
	MCBrowserDictionaryRef t_dict;
	t_dict = nil;

	t_success = MCNSDictionaryToBrowserDictionary(p_dictionary, t_dict, p_converter);
	
	if (t_success)
		t_success = MCBrowserValueSetDictionary(r_value, t_dict);
	
	MCBrowserDictionaryRelease(t_dict);
	
	return t_success;
}

bool MCNSArrayToBrowserValue(NSArray *p_array, MCBrowserValue &r_value, MCCustomNSObjectConverter p_converter)
{
	bool t_success;
	t_success = true;
	
	MCBrowserListRef t_list;
	t_list = nil;
	
	t_success = MCNSArrayToBrowserList(p_array, t_list, p_converter);
	
	if (t_success)
		t_success = MCBrowserValueSetList(r_value, t_list);
	
	MCBrowserListRelease(t_list);
	
	return t_success;
}

