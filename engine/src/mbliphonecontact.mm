/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "uidc.h"
#include "execpt.h"
#include "globals.h"

#include "exec.h"
#include "mblsyntax.h"
#include "mblcontact.h"

#include "mbliphoneapp.h"

#import <UIKit/UIKit.h>
#import <MessageUI/MessageUI.h>
#import <AddressBook/AddressBook.h>
#import <AddressBookUI/AddressBookUI.h>

////////////////////////////////////////////////////////////////////////////////

UIView *MCIPhoneGetView(void);
UIViewController *MCIPhoneGetViewController(void);

////////////////////////////////////////////////////////////////////////////////

typedef struct _ios_key_map
{
	CFStringRef key;
	MCNameRef *name;
} ios_key_map_t;
typedef struct _ios_label_map
{
	CFStringRef label;
	MCNameRef *name;
} ios_label_map_t;

typedef struct _ios_property_map
{
	const ABPropertyID *property;
	MCNameRef *name;
	bool has_labels;
	bool has_keys;
} ios_property_map_t;


static ios_key_map_t s_key_map[] = {
	{kABPersonAddressStreetKey, &MCN_street},
	{kABPersonAddressCityKey, &MCN_city},
	{kABPersonAddressStateKey, &MCN_state},
	{kABPersonAddressZIPKey, &MCN_zip},
	{kABPersonAddressCountryKey, &MCN_country},
	{kABPersonAddressCountryCodeKey, &MCN_countrycode},
};

static ios_label_map_t s_label_map[] = {
	{kABHomeLabel, &MCN_home},
	{kABWorkLabel, &MCN_work},
	{kABOtherLabel, &MCN_other},
	{kABPersonPhoneMobileLabel, &MCN_mobile},
	{kABPersonPhoneIPhoneLabel, &MCN_iphone},
	{kABPersonPhoneMainLabel, &MCN_main},
	{kABPersonPhoneHomeFAXLabel, &MCN_homefax},
	{kABPersonPhoneWorkFAXLabel, &MCN_workfax},
	{kABPersonPhonePagerLabel, &MCN_pager},
#ifdef __IPHONE_5_0
	{CFSTR("_$!<OtherFAX>!$_"), &MCN_otherfax}, // kABPersonPhoneOtherFAXLabel
#endif
};

static ios_property_map_t s_property_map[] = {
	{&kABPersonFirstNameProperty, &MCN_firstname, false, false},
	{&kABPersonLastNameProperty, &MCN_lastname, false, false},
	{&kABPersonMiddleNameProperty, &MCN_middlename, false, false},
	{&kABPersonPrefixProperty, &MCN_prefix, false, false},
	{&kABPersonSuffixProperty, &MCN_suffix, false, false},
	{&kABPersonNicknameProperty, &MCN_nickname, false, false},
	{&kABPersonFirstNamePhoneticProperty, &MCN_firstnamephonetic, false, false},
	{&kABPersonLastNamePhoneticProperty, &MCN_lastnamephonetic, false, false},
	{&kABPersonMiddleNamePhoneticProperty, &MCN_middlenamephonetic, false, false},
	{&kABPersonOrganizationProperty, &MCN_organization, false, false},
	{&kABPersonJobTitleProperty, &MCN_jobtitle, false, false},
	{&kABPersonDepartmentProperty, &MCN_department, false, false},
	{&kABPersonNoteProperty, &MCN_note, false, false},
	
	{&kABPersonEmailProperty, &MCN_email, true, false},
	{&kABPersonPhoneProperty, &MCN_phone, true, false},
	{&kABPersonAddressProperty, &MCN_address, true, true},
};

static bool label_to_name(CFStringRef p_label, MCNameRef &r_name)
{
	for (uindex_t i = 0; i < ELEMENTS(s_label_map); i++)
	{
		if (CFStringCompare(s_label_map[i].label, p_label, 0) == kCFCompareEqualTo)
		{
			r_name = *s_label_map[i].name;
			return true;
		}
	}
	
	return false;
}

static bool key_to_name(CFStringRef p_key, MCNameRef &r_name)
{
	for (uindex_t i = 0; i < ELEMENTS(s_key_map); i++)
	{
		if (CFStringCompare(s_key_map[i].key, p_key, 0) == kCFCompareEqualTo)
		{
			r_name = *s_key_map[i].name;
			return true;
		}
	}
	
	return false;
}

static bool name_to_key(MCNameRef p_name, CFStringRef &r_key)
{
	for (uindex_t i = 0; i < ELEMENTS(s_key_map); i++)
	{
		if (MCNameIsEqualTo(*s_key_map[i].name, p_name, kMCCompareCaseless))
		{
			r_key = s_key_map[i].key;
			return true;
		}
	}
	
	return false;
}

bool MCCFDictionaryToArray(MCExecPoint& ep, CFDictionaryRef p_dict, MCVariableValue *&r_array)
{
	bool t_success = true;
	
	CFStringRef *t_dict_keys = nil;
	CFStringRef *t_dict_values = nil;
	uindex_t t_dict_size = 0;
	
	MCVariableValue *t_prop_array = nil;
	
	t_success = nil != (t_prop_array = new MCVariableValue());
	
	if (t_success)
	{
		t_dict_size = CFDictionaryGetCount(p_dict);
		t_success = MCMemoryNewArray(t_dict_size, t_dict_keys) &&
		MCMemoryNewArray(t_dict_size, t_dict_values);
	}
	
	if (t_success)
	{
		CFDictionaryGetKeysAndValues(p_dict, (const void **)t_dict_keys, (const void**)t_dict_values);
		for (uindex_t i = 0; t_success && i < t_dict_size; i++)
		{
			MCNameRef t_key_name;
			MCVariableValue *t_array_entry;
			if (key_to_name(t_dict_keys[i], t_key_name))
			{
				t_success = t_prop_array->lookup_element(ep, MCNameGetOldString(t_key_name), t_array_entry) == ES_NORMAL;
				if (t_success)
				{
					const char *t_value = [(NSString*)t_dict_values[i] cStringUsingEncoding: NSMacOSRomanStringEncoding];
					t_success = t_array_entry->assign_string(MCString(t_value));
				}
			}
		}
	}
	
	if (t_success)
		r_array = t_prop_array;
	
	MCMemoryDeleteArray(t_dict_keys);
	MCMemoryDeleteArray(t_dict_values);
	
	return t_success;
}

bool MCCFDictionaryFromArray(MCExecPoint& p_ep, MCVariableValue *p_array, CFDictionaryRef& r_dict)
{
	if (!p_array->is_array())
		return false;
	
	bool t_success = true;
	
	MCVariableArray *t_array = p_array->get_array();

	CFMutableDictionaryRef t_dict = nil;
	t_success = nil != (t_dict = CFDictionaryCreateMutable(kCFAllocatorDefault, t_array->getnfilled(), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
	
	MCHashentry *t_hashentry = nil;
	uindex_t t_index = 0;
	
	MCExecPoint ep(p_ep);
	
	while (t_success && nil != (t_hashentry = t_array->getnextelement(t_index, t_hashentry, False, ep)))
	{
		CFStringRef t_key;
		t_hashentry->value.fetch(ep);
		
		MCNameRef t_key_name = nil;
		t_success = MCNameCreateWithCString(t_hashentry->string, t_key_name);
		
		if (name_to_key(t_key_name, t_key))
		{
			NSString *t_value = nil;
			if (t_success)
				t_success = nil != (t_value = [NSString stringWithCString:ep.getcstring() encoding: NSMacOSRomanStringEncoding]);
			
			if (t_success)
				CFDictionaryAddValue(t_dict, t_key, t_value);
		}
		
		MCNameDelete(t_key_name);
	}
	
	if (t_success)
		r_dict = t_dict;
	else if (t_dict != nil)
		CFRelease(t_dict);
	
	return t_success;
}

bool MCCreatePersonData(MCExecPoint& ep, ABRecordRef p_person, MCVariableValue *&r_contact)
{
	MCVariableValue *t_contact = nil;
	bool t_success = true;
	
	t_contact = new MCVariableValue();
	t_success = t_contact != nil;
	
	for (uindex_t i = 0; t_success && i < ELEMENTS(s_property_map); i++)
	{
		CFTypeRef t_prop_value;
		t_prop_value = ABRecordCopyValue(p_person, *s_property_map[i].property);
		if (t_prop_value != nil)
		{
			if (!s_property_map[i].has_labels)
			{
				const char *t_value = [(NSString*)t_prop_value cStringUsingEncoding: NSMacOSRomanStringEncoding];
				t_success = MCContactAddProperty(ep, t_contact, *s_property_map[i].name, MCString(t_value));
			}
			else
			{
				// if the property has a label, it's a multivalue prop and can have multiple entries of the same type
				ABMultiValueRef t_values = t_prop_value;
				ABPropertyType t_proptype = ABMultiValueGetPropertyType(t_values);
				uindex_t t_value_count = ABMultiValueGetCount(t_values);

				for (uindex_t j = 0; j < t_value_count; j++)
				{
					CFStringRef t_label;
					MCNameRef t_label_name;
					
					t_label = ABMultiValueCopyLabelAtIndex(t_values, j);
					
					if (label_to_name(t_label, t_label_name))
					{
						CFTypeRef t_multi_value;
						t_multi_value = ABMultiValueCopyValueAtIndex(t_values, j);
						
						// Currently we're only dealing with string values
						if (t_proptype == kABStringPropertyType)
						{
							const char *t_value = [(NSString*)t_multi_value cStringUsingEncoding: NSMacOSRomanStringEncoding];
							t_success = MCContactAddPropertyWithLabel(ep, t_contact, *s_property_map[i].name, t_label_name, MCString(t_value));
						}
						else if (t_proptype == kABDictionaryPropertyType)
						{
							// construct an array containing the keys/values of the CFDictionaryRef and add it to our contact array
							MCVariableValue *t_prop_array = nil;
							t_success = MCCFDictionaryToArray(ep, (CFDictionaryRef)t_multi_value, t_prop_array) && 
								MCContactAddPropertyWithLabel(ep, t_contact, *s_property_map[i].name, t_label_name, t_prop_array);
							
							delete t_prop_array;
						}
						
						CFRelease(t_multi_value);
					}
				}
			}
			
			CFRelease(t_prop_value);
		}
	}
	
	if (t_success)
		r_contact = t_contact;
	else
		delete t_contact;
	
	return t_success;
}

bool MCCreatePerson(MCExecPoint &p_ep, MCVariableValue *p_contact, ABRecordRef &r_person)
{
	MCExecPoint ep(p_ep);
	bool t_success = true;
	ABRecordRef t_person = nil;
	t_success = nil != (t_person = ABPersonCreate());
	
	for (uindex_t i = 0; t_success && i < ELEMENTS(s_property_map); i++)
	{
		if (p_contact->fetch_element_if_exists(ep, MCNameGetOldString(*s_property_map[i].name), false))
		{
			if (!s_property_map[i].has_labels)
			{
				MCString t_value = ep.getsvalue0();
				if (t_value.getlength() > 0)
				{
					t_success = ABRecordSetValue(t_person, *s_property_map[i].property,
									 [NSString stringWithCString:t_value.getstring() encoding:NSMacOSRomanStringEncoding],
									 nil);
				}
			}
			else
			{
				MCVariableValue *t_prop_array = ep.getarray();
				if (t_prop_array != nil)
				{
					ABMutableMultiValueRef t_multi_value = nil;
					
					CFTypeID t_multi_type = s_property_map[i].has_keys ? kABDictionaryPropertyType : kABStringPropertyType;
					t_success = nil != (t_multi_value = ABMultiValueCreateMutable(t_multi_type));
					for (uindex_t j = 0; t_success && j < ELEMENTS(s_label_map); j++)
					{
						if (t_prop_array->fetch_element_if_exists(ep, MCNameGetOldString(*s_label_map[j].name), false))
					{
							MCVariableValue *t_indexed_array = ep.getarray();
							if (t_indexed_array != nil)
							{
								MCVariableValue *t_index_value;
								uindex_t t_index = 1;
								
								while (t_success = (ES_NORMAL == t_indexed_array->lookup_index(ep, t_index++, false, t_index_value)))
								{
									if (t_index_value == nil)
										break;

									t_index_value->fetch(ep);
									if (!s_property_map[i].has_keys)
									{
										MCString t_value = ep.getsvalue0();
										if (t_value.getlength() > 0)
										{
											t_success = ABMultiValueAddValueAndLabel(t_multi_value,
																					 [NSString stringWithCString:t_value.getstring() encoding:NSMacOSRomanStringEncoding],
																					 s_label_map[j].label,
																					 nil);
										}
									}
									else
									{
										MCVariableValue *t_label_array = ep.getarray();
										if (t_label_array != nil)
										{
											CFDictionaryRef t_dict = nil;
											t_success = MCCFDictionaryFromArray(ep, t_label_array, t_dict) &&
											ABMultiValueAddValueAndLabel(t_multi_value, t_dict, s_label_map[j].label, nil);
											if (t_dict != nil)
												CFRelease(t_dict);
										}
									}
								}
							}
						}
					}
					
					if (t_success && ABMultiValueGetCount(t_multi_value) > 0)
						t_success = ABRecordSetValue(t_person, *s_property_map[i].property, t_multi_value, nil);
				}
			}
		}
	}
	
	if (t_success)
		r_person = t_person;
	else if (t_person != nil)
		CFRelease(t_person);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCContactAddContact(MCVariableValue *p_contact, int32_t& r_chosen)
{
    bool t_success = true;
	MCExecPoint ep(nil, nil, nil);
	
    ABAddressBookRef t_address_book = nil;
	if (t_success)
		t_success = nil != (t_address_book = ABAddressBookCreate());
	
	ABRecordRef t_contact = nil;
	if (t_success)
		t_success = MCCreatePerson(ep, p_contact, t_contact);
	
	
    // try to add new record in the address book
    if (t_success)
    {
		// save changes made in address book
        t_success = ABAddressBookAddRecord (t_address_book, t_contact, nil) &&
			ABAddressBookSave(t_address_book, nil);
    }
	ABRecordID t_id;
    // Return the result
	if (t_success)
		t_success = kABRecordInvalidID != (t_id = ABRecordGetRecordID(t_contact));

	if (t_success)
		r_chosen = t_id;
	
	if (t_contact != nil)
	CFRelease(t_contact);
	if (t_address_book != nil)
		CFRelease(t_address_book);
	
	return t_success;
}

bool MCContactDeleteContact(int32_t p_person_id)
{
	bool t_success = true;
	
    ABAddressBookRef t_address_book = ABAddressBookCreate();
    ABRecordRef t_contact = ABAddressBookGetPersonWithRecordID (t_address_book, p_person_id);
	
	t_success = t_contact != nil && t_address_book != nil;
	if (t_success)
		t_success = ABAddressBookRemoveRecord (t_address_book, t_contact, nil) &&
			ABAddressBookSave(t_address_book, nil);
	
	if (t_address_book != nil)
		CFRelease(t_address_book);
	
	return t_success;
}

bool MCContactFindContact(const char* p_person_name, char *&r_chosen)
{
	bool t_success = true;
	
	if (p_person_name == nil)
	{
		r_chosen = nil;
		return true;
	}
	
    // Fetch the address book
    ABAddressBookRef t_address_book = ABAddressBookCreate();
	t_success = t_address_book != nil;
	
	CFStringRef t_person_name = nil;
    NSArray *t_people = nil;
	NSMutableString *t_chosen = nil;
	
    if (t_success)
		t_success = nil != (t_person_name = CFStringCreateWithCString(NULL, p_person_name, kCFStringEncodingMacRoman));

	if (t_success)
	{
		t_people = (NSArray *)ABAddressBookCopyPeopleWithName(t_address_book, t_person_name);
		if ((t_people != NULL) && [t_people count] > 0)
		{
			t_chosen = [NSMutableString stringWithFormat:@"%d", ABRecordGetRecordID((ABRecordRef)[t_people objectAtIndex:0])];
						t_success = nil != t_chosen;
			// set the label item
			for (int i = 1; t_success && i < [t_people count]; i++)
			{
				/* UNCHECKED */ [t_chosen appendFormat:@",%d", ABRecordGetRecordID((ABRecordRef)[t_people objectAtIndex:i])];
			}
		}
	}

	if (t_success)
		t_success = MCCStringClone([t_chosen cStringUsingEncoding:NSMacOSRomanStringEncoding], r_chosen);
	
    if (t_people != nil)
		[t_people release];
	if (t_person_name != nil)
		CFRelease(t_person_name);
	if (t_address_book != nil)
		CFRelease(t_address_book);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

@interface MCIPhoneContactDelegate : NSObject
{
	bool m_running, m_finished, m_success;
	UINavigationController *m_navigation;
	ABRecordID m_selected_person;
}

- (id)init;
- (void)dealloc;

@end

@implementation MCIPhoneContactDelegate

- (id)init
{
	if (self = [super init])
	{
		// Is a view delegate running.
		m_running = true;
		
		// Has the view delegate finished.
		m_finished = true;
		
		m_navigation = nil;

		// Returned values.
		m_selected_person = kABRecordInvalidID;
	}
    
	return self;
}

- (void)dealloc
{
	if (m_navigation != nil)
		[m_navigation release];
	[super dealloc];
}

- (void)doDismissController
{
	if (MCmajorosversion >= 500)
		[MCIPhoneGetViewController() dismissViewControllerAnimated:YES completion:^(){m_finished = true;}];
	else
        [MCIPhoneGetViewController() dismissModalViewControllerAnimated:YES];
}

- (void)dismissController
{
    // HSC-2012-05-14: [[ BZ 10213 ]] Delayed continuation until view disappeared. 
    if (MCmajorosversion >= 500)
    {
        m_finished = false;
		MCIPhoneCallSelectorOnMainFiber(self, @selector(doDismissController));
        while (!m_finished)
            MCscreen -> wait(1.0, False, True);
    }
    else
	{
		MCIPhoneCallSelectorOnMainFiber(self, @selector(doDismissController));
		while([self retainCount] > 2)
			MCscreen -> wait(0.01, False, True);
	}
}

@end


////////////////////////////////////////////////////////////////////////////////

@interface MCIPhonePickContactDelegate : MCIPhoneContactDelegate <ABPeoplePickerNavigationControllerDelegate>
{
    ABPeoplePickerNavigationController *m_pick_contact;
}

- (id)init;
- (void)dealloc;

@end

@implementation MCIPhonePickContactDelegate

- (id)init
{
	if (self = [super init])
	{
		// The view delegates.
		m_pick_contact = nil;
	}
    return self;
}

- (void)dealloc
{
	if (m_pick_contact != nil)
		[m_pick_contact release];
    [super dealloc];
}


-(void)doShowPickContact
{
	m_success = true;
	
    if (m_pick_contact == nil)
        m_success = nil != (m_pick_contact = [[ABPeoplePickerNavigationController alloc] init]);
	
    if (m_success)
	{
		m_pick_contact.peoplePickerDelegate = self;
		NSArray *t_displayed_items = [NSArray arrayWithObjects:[NSNumber numberWithInt:kABPersonPhoneProperty],
									  [NSNumber numberWithInt:kABPersonEmailProperty],
									  [NSNumber numberWithInt:kABPersonBirthdayProperty], nil];
		
		m_pick_contact.displayedProperties = t_displayed_items;
		// Show the picker
		m_running = true;
		
		[MCIPhoneGetViewController() presentModalViewController:m_pick_contact animated:YES];
	}
}

-(bool)showPickContact: (int32_t&) r_chosen
{
	MCIPhoneCallSelectorOnMainFiber(self, @selector(doShowPickContact));
	
    while (m_running)
		MCscreen -> wait(1.0, False, True);
	
	[self dismissController];
	
    // Return the result
    if (m_selected_person == kABRecordInvalidID)
        r_chosen = 0;
    else
        r_chosen = m_selected_person;
	
	return m_success;
}

- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker shouldContinueAfterSelectingPerson:(ABRecordRef)person
{
    if (person != NULL)
        m_selected_person = ABRecordGetRecordID(person);
	m_running = false;
    return NO;
}

// Does not allow users to perform default actions such as dialing a phone number, when they select a person property.
- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker shouldContinueAfterSelectingPerson:(ABRecordRef)person 
                                property:(ABPropertyID)property identifier:(ABMultiValueIdentifier)identifier
{
    if (person != NULL)
        m_selected_person = ABRecordGetRecordID(person);
	m_running = false;
    return NO;
}

// Dismisses the people picker and shows the application when users tap Cancel. 
- (void)peoplePickerNavigationControllerDidCancel:(ABPeoplePickerNavigationController *)peoplePicker;
{
	m_running = false;
}

@end

////////////////////////////////////////////////////////////////////////////////

@interface MCIPhoneShowContactDelegate : MCIPhoneContactDelegate <ABPersonViewControllerDelegate>
{
	ABPersonViewController *m_view_contact;
}

- (id)init;
- (void)dealloc;

@end

@implementation MCIPhoneShowContactDelegate

- (id)init
{
	if (self = [super init])
	{
		// The view delegates.
		m_view_contact = nil;
	}
	return self;
}

- (void)dealloc
{
	if (m_view_contact != nil)
		[m_view_contact release];
    [super dealloc];
}

-(void)doShowViewContact: (NSNumber *)personId
{
	int32_t t_person_id;
	t_person_id = [personId intValue];
	
    ABAddressBookRef t_address_book = nil;
	m_success = nil != (t_address_book = ABAddressBookCreate());

    ABRecordRef t_person = ABAddressBookGetPersonWithRecordID (t_address_book, t_person_id);
	if (t_person != nil)
    {
        m_selected_person = t_person_id;
        if (m_view_contact == nil)
			m_success = nil != (m_view_contact = [[ABPersonViewController alloc] init]);
		
		if (m_success)
		{
			m_view_contact.personViewDelegate = self;
			m_view_contact.displayedPerson = t_person;
			m_view_contact.allowsEditing = NO;
			m_success = nil != (m_navigation = [[UINavigationController alloc] initWithRootViewController: m_view_contact]);
		}
		
		if (m_success)
		{
			[m_navigation setToolbarHidden: NO];
			[MCIPhoneGetViewController() presentModalViewController:m_navigation animated:YES];
		
			UIBarButtonItem *t_done_button = [[UIBarButtonItem alloc] initWithTitle:@"Done" style:UIBarButtonItemStyleDone target:self action:@selector(handleGetContactDone)];
			NSArray *t_items = [NSArray arrayWithObject: t_done_button];
			[t_done_button release];
			[m_navigation.toolbar setItems: t_items animated:YES];
		}
    }
	
	if (t_person == nil || !m_success)
		m_running = false;
	
	if (t_address_book != nil)
		CFRelease(t_address_book);
}

-(bool)showViewContact: (int32_t) p_person_id withResult: (int32_t&) r_chosen
{
	MCIPhoneCallSelectorOnMainFiberWithObject(self, @selector(doShowViewContact:), [NSNumber numberWithInt: p_person_id]);
	
	if (m_running)
	{
		while (m_running)
			MCscreen -> wait(1.0, False, True);
		
		[self dismissController];
	}
	
    // Return the result
    if (m_selected_person == kABRecordInvalidID)
        r_chosen = 0;
    else
        r_chosen = m_selected_person;
	
	return m_success;
}

-(void) handleGetContactDone
{
	m_running = false;
}

// Does not allow users to perform default actions such as dialing a phone number, when they select a contact property.
- (BOOL)personViewController:(ABPersonViewController *)personViewController shouldPerformDefaultActionForPerson:(ABRecordRef)person 
                    property:(ABPropertyID)property identifier:(ABMultiValueIdentifier)identifierForValue
{
    if (person != NULL)
        m_selected_person = ABRecordGetRecordID(person);
    return NO;
}

@end

////////////////////////////////////////////////////////////////////////////////

@interface MCIPhoneCreateContactDelegate : MCIPhoneContactDelegate <ABNewPersonViewControllerDelegate>
{
	ABNewPersonViewController *m_get_contact;
}

- (id)init;
- (void)dealloc;

@end

@implementation MCIPhoneCreateContactDelegate

- (id)init
{
	if (self = [super init])
	{
		// The view delegates.
		m_get_contact = nil;
	}
	return self;
}

- (void)dealloc
{
	if (m_get_contact != nil)
		[m_get_contact release];
    [super dealloc];
}

-(void)doShowCreateContact
{
	m_success = true;
	
	m_success = nil != (m_get_contact = [[ABNewPersonViewController alloc] init]);
	
	if (m_success)
	{
		m_get_contact.newPersonViewDelegate = self;    
		// Check out documentation. We need to launch this with a navigationController.
		if (m_navigation == nil)
			m_success = nil != (m_navigation = [[UINavigationController alloc] initWithRootViewController:m_get_contact]);
	}
	
	if (m_success)
		[MCIPhoneGetViewController() presentModalViewController:m_navigation animated:YES];
	else
		m_running = false;
}

-(bool)showCreateContact: (int32_t&) r_chosen
{
	MCIPhoneCallSelectorOnMainFiber(self, @selector(doShowCreateContact));
	
    while (m_running)
		MCscreen -> wait(1.0, False, True);
    
	[self dismissController];
	
    // Return the result
    if (m_selected_person == kABRecordInvalidID)
        r_chosen = 0;
    else
        r_chosen = m_selected_person;
	
	return m_success;
}

// Dismisses the new-person view controller. 
- (void)newPersonViewController:(ABNewPersonViewController *)newPersonViewController didCompleteWithNewPerson:(ABRecordRef)person
{
    if (person != NULL)
        m_selected_person = ABRecordGetRecordID(person);
	m_running = false;
}

@end

////////////////////////////////////////////////////////////////////////////////

@interface MCIPhoneUpdateContactDelegate : MCIPhoneContactDelegate <ABUnknownPersonViewControllerDelegate>
{
	ABUnknownPersonViewController *m_update_contact;
}

- (id)init;
- (void)dealloc;

@end

@implementation MCIPhoneUpdateContactDelegate

- (id)init
{
	if (self = [super init])
	{
		// The view delegates.
		m_update_contact = nil;
	}
	return self;
}

- (void)dealloc
{
	if (m_update_contact != nil)
		[m_update_contact release];
    [super dealloc];
}

-(bool)showUpdateContact: (ABRecordRef) p_contact
			   withTitle: (const char*)p_title withMessage: (const char*)p_message withAlternateName: (const char*)p_alternate_name
			  withResult: (int32_t&) r_chosen
{
	m_success = true;
	
	MCIPhoneRunBlockOnMainFiber(^(void) {
		if (m_update_contact == nil)
			m_success = nil != (m_update_contact = [[ABUnknownPersonViewController alloc] init]);
		
		NSString *t_title = nil;
		NSString *t_message = nil;
		NSString *t_alternate_name = nil;
		
		if (m_success)
		{
			t_title = [NSString stringWithCString: p_title == nil ? "" : p_title encoding:NSMacOSRomanStringEncoding];
			t_message = [NSString stringWithCString: p_message == nil ? "" : p_message encoding:NSMacOSRomanStringEncoding];
			t_alternate_name = [NSString stringWithCString: p_alternate_name == nil ? "" : p_alternate_name encoding:NSMacOSRomanStringEncoding];
			m_success = (t_title != nil) && (t_message != nil) && (t_alternate_name != nil);
		}
		
		if (m_success)
		{
			m_update_contact.unknownPersonViewDelegate = self;
			m_update_contact.displayedPerson = p_contact;
			m_update_contact.allowsAddingToAddressBook = YES;
			m_update_contact.allowsActions = YES;
			m_update_contact.title = t_title;
			m_update_contact.message = t_message;
			m_update_contact.alternateName = t_alternate_name;
		}
		
		if (m_success)
			m_success = nil != (m_navigation = [[UINavigationController alloc] initWithRootViewController:m_update_contact]);
		
		UIBarButtonItem *t_done_button = nil;
		NSArray *t_items = nil;
		
		if (m_success)
			m_success = nil != (t_done_button = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(handleUpdateContactDone)]);
		if (m_success)
			m_success = nil != (t_items = [NSArray arrayWithObject: t_done_button]);
		
		if (m_success)
		{
			[m_navigation setToolbarHidden: NO];
			[MCIPhoneGetViewController() presentModalViewController:m_navigation animated:YES];
			[m_navigation.toolbar setItems: t_items animated: YES];
		}
		else
			m_running = false;
		
		if (t_done_button != nil)
			[t_done_button release];
		
	});
	
    while (m_running)
		MCscreen -> wait(1.0, False, True);
	
	[self dismissController];
	
    // Return the result
    if (m_selected_person == kABRecordInvalidID)
        r_chosen = 0;
    else
        r_chosen = m_selected_person;
	
	return m_success;
}

-(void) handleUpdateContactDone
{
	m_running = false;
}

// Dismisses the picker when users are done creating a contact or adding the displayed person properties to an existing contact. 
- (void)unknownPersonViewController:(ABUnknownPersonViewController *)unknownPersonView didResolveToPerson:(ABRecordRef)person
{
    if (person != NULL)
        m_selected_person = ABRecordGetRecordID(person);
}

// Does not allow users to perform default actions such as emailing a contact, when they select a contact property.
- (BOOL)unknownPersonViewController:(ABUnknownPersonViewController *)personViewController shouldPerformDefaultActionForPerson:(ABRecordRef)person property:(ABPropertyID)property identifier:(ABMultiValueIdentifier)identifier
{
    return YES;
}

@end

////////////////////////////////////////////////////////////////////////////////

bool MCSystemPickContact(int32_t& r_result) // ABPeoplePickerNavigationController
{
    bool t_success = true;
	
    MCIPhonePickContactDelegate *t_pick_contact = nil;
	t_success = nil != (t_pick_contact = [[MCIPhonePickContactDelegate alloc] init]);
	
	if (t_success)
		t_success = [t_pick_contact showPickContact: r_result];
	
	if (t_pick_contact != nil)
		[t_pick_contact release];
	
    return t_success;
}

bool MCSystemShowContact(int32_t p_contact_id, int32_t& r_result) // ABPersonViewController
{
    bool t_success = true;
	
    MCIPhoneShowContactDelegate *t_view_contact = nil;
	t_success = nil != (t_view_contact = [[MCIPhoneShowContactDelegate alloc] init]);
	
	if (t_success)
		t_success = [t_view_contact showViewContact:p_contact_id withResult: r_result];
	
	if (t_view_contact != nil)
		[t_view_contact release];
	
    return t_success;
}

bool MCSystemCreateContact(int32_t& r_result) // ABNewPersonViewController
{
    bool t_success = true;
	
    MCIPhoneCreateContactDelegate *t_create_contact = nil;
	t_success = nil != (t_create_contact = [[MCIPhoneCreateContactDelegate alloc] init]);
	
	if (t_success)
		t_success = [t_create_contact showCreateContact:r_result];
	
	if (t_create_contact != nil)
		[t_create_contact release];

    return t_success;
}

bool MCSystemUpdateContact(MCVariableValue *p_contact, const char *p_title, const char *p_message, const char *p_alternate_name,
						   int32_t &r_result)
{
	bool t_success = true;
	
	MCExecPoint ep(nil, nil, nil);
	ABRecordRef t_contact = nil;
	t_success = MCCreatePerson(ep, p_contact, t_contact);

	MCIPhoneUpdateContactDelegate *t_update_contact = nil;
	if (t_success)
		t_success = nil != (t_update_contact = [[MCIPhoneUpdateContactDelegate alloc] init]);
	if (t_success)
		t_success = [t_update_contact showUpdateContact:t_contact
											  withTitle:p_title withMessage:p_message withAlternateName:p_alternate_name
											 withResult:r_result];
	if (t_update_contact != nil)
		[t_update_contact release];
	if (t_contact != nil)
		CFRelease(t_contact);
	
	return t_success;
}

bool MCSystemGetContactData(MCExecContext &r_ctxt, int32_t p_contact_id, MCVariableValue *&r_contact_data)
{
	bool t_success = true;
	
    ABAddressBookRef t_address_book = nil;
	t_success = nil != (t_address_book = ABAddressBookCreate());
	
    ABRecordRef t_person = nil;
	if (t_success)
		t_success = nil != (t_person = ABAddressBookGetPersonWithRecordID (t_address_book, p_contact_id));
	
	if (t_success)
		t_success = MCCreatePersonData(r_ctxt.GetEP(), t_person, r_contact_data);
	
	if (t_address_book != nil)
		CFRelease(t_address_book);
	
	return t_success;
}

bool MCSystemRemoveContact(int32_t p_contact_id)
{
	return MCContactDeleteContact(p_contact_id);
}

bool MCSystemAddContact(MCVariableValue *p_contact, int32_t& r_result)
{
	return MCContactAddContact(p_contact, r_result);
}

bool MCSystemFindContact(const char* p_contact_name, char *& r_result)
{
	return MCContactFindContact(p_contact_name, r_result);
}
