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

#include "uidc.h"

#include "globals.h"

#include "exec.h"
#include "mblsyntax.h"
#include "mblcontact.h"

#include "mbliphone.h"
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
            // SN-201-04-28: [[ Bug 15124 ]] The value must be retained.
			r_name = MCValueRetain(*s_label_map[i].name);
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
			// PM-2015-12-09: [[ Bug 16156 ]] Prevent crash caused by underretain
			r_name = MCValueRetain(*s_key_map[i].name);
			return true;
		}
	}
	
	return false;
}

static bool name_to_key(MCNameRef p_name, CFStringRef &r_key)
{
	for (uindex_t i = 0; i < ELEMENTS(s_key_map); i++)
	{
		if (MCNameIsEqualToCaseless(*s_key_map[i].name, p_name))
		{
			r_key = s_key_map[i].key;
			return true;
		}
	}
	
	return false;
}

bool MCCFDictionaryToArray(CFDictionaryRef p_dict, MCArrayRef &r_array)
{
	bool t_success = true;
	
	CFStringRef *t_dict_keys = nil;
	CFStringRef *t_dict_values = nil;
	uindex_t t_dict_size = 0;
	
	MCAutoArrayRef t_prop_array;
	t_success = MCArrayCreateMutable(&t_prop_array);
	
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
			MCNewAutoNameRef t_key_name;
			if (key_to_name(t_dict_keys[i], &t_key_name))
			{
				MCAutoStringRef t_string;
				MCStringCreateWithCFStringRef((CFStringRef)t_dict_values[i], &t_string);
				t_success = MCArrayStoreValue(*t_prop_array, false, *t_key_name, *t_string);
			}
		}
	}
	
	if (t_success)
		r_array = MCValueRetain(*t_prop_array);
	
	MCMemoryDeleteArray(t_dict_keys);
	MCMemoryDeleteArray(t_dict_values);
	
	return t_success;
}

bool MCCFDictionaryFromArray(MCArrayRef p_array, CFDictionaryRef& r_dict)
{
	bool t_success = true;

	CFMutableDictionaryRef t_dict = nil;

	if (t_success)
		t_success = nil != (t_dict = CFDictionaryCreateMutable(kCFAllocatorDefault, MCArrayGetCount(p_array), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
	
	MCValueRef t_entry;
	MCNameRef t_key_name;
	uintptr_t t_index;

	while (t_success && MCArrayIterate(p_array, t_index, t_key_name, t_entry))
	{
		CFStringRef t_key;
		if (name_to_key(t_key_name, t_key))
		{
			NSString *t_value = nil;
			if (t_success)
				t_success = nil != (t_value = MCStringConvertToAutoreleasedNSString((MCStringRef)t_entry));
			
			if (t_success)
				CFDictionaryAddValue(t_dict, t_key, t_value);
		}
	}
	
	if (t_success)
		r_dict = t_dict;
	else if (t_dict != nil)
		CFRelease(t_dict);
	
	return t_success;
}

bool MCCreatePersonData(ABRecordRef p_person, MCArrayRef& r_contact)
{
	bool t_success = true;
	
	MCAutoArrayRef t_contact;
	t_success = MCArrayCreateMutable(&t_contact);
	
	for (uindex_t i = 0; t_success && i < ELEMENTS(s_property_map); i++)
	{
		CFTypeRef t_prop_value;
		t_prop_value = ABRecordCopyValue(p_person, *s_property_map[i].property);
		if (t_prop_value != nil)
		{
			if (!s_property_map[i].has_labels)
			{
				MCAutoStringRef t_value;
				MCStringCreateWithCFStringRef((CFStringRef)t_prop_value, &t_value);
				t_success = MCContactAddProperty(*t_contact, *s_property_map[i].name, *t_value);
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
					MCNewAutoNameRef t_label_name;
					
					t_label = ABMultiValueCopyLabelAtIndex(t_values, j);
					
					if (label_to_name(t_label, &t_label_name))
					{
						CFTypeRef t_multi_value;
						t_multi_value = ABMultiValueCopyValueAtIndex(t_values, j);
						
						// Currently we're only dealing with string values
						if (t_proptype == kABStringPropertyType)
						{
							MCAutoStringRef t_value;
							MCStringCreateWithCFStringRef((CFStringRef)t_multi_value, &t_value);
							t_success = MCContactAddPropertyWithLabel(*t_contact, *s_property_map[i].name, *t_label_name, *t_value);
						}
						else if (t_proptype == kABDictionaryPropertyType)
						{
							// construct an array containing the keys/values of the CFDictionaryRef and add it to our contact array
							MCAutoArrayRef t_prop_array;
							t_success = MCCFDictionaryToArray((CFDictionaryRef)t_multi_value, &t_prop_array) && 
								MCContactAddPropertyWithLabel(*t_contact, *s_property_map[i].name, *t_label_name, *t_prop_array);
						}
						
						CFRelease(t_multi_value);
					}
				}
			}
			
			CFRelease(t_prop_value);
		}
	}
	
	if (t_success)
		r_contact = MCValueRetain(*t_contact);

	return t_success;
}

bool MCCreatePerson(MCArrayRef p_contact, ABRecordRef &r_person)
{
	bool t_success = true;
	ABRecordRef t_person = nil;
	t_success = nil != (t_person = ABPersonCreate());
	
	for (uindex_t i = 0; t_success && i < ELEMENTS(s_property_map); i++)
	{
		MCValueRef t_value;
		if (MCArrayFetchValue(p_contact, false, *s_property_map[i].name, t_value))
		{
			if (!s_property_map[i].has_labels)
			{
                // PM-2015-05-25: [[ Bug 15403 ]] Convert the valueref to a stringref
                MCExecContext ctxt(nil,nil,nil);
                MCAutoStringRef t_value_string;
                ctxt.ConvertToString(t_value, &t_value_string);
				if (MCStringGetLength(*t_value_string) > 0)
				{
					t_success = ABRecordSetValue(t_person, *s_property_map[i].property,
									 MCStringConvertToAutoreleasedNSString(*t_value_string),
									 nil);
				}
			}
			else if (MCValueIsArray(t_value))
			{
				ABMutableMultiValueRef t_multi_value = nil;
				
				CFTypeID t_multi_type = s_property_map[i].has_keys ? kABDictionaryPropertyType : kABStringPropertyType;
				t_success = nil != (t_multi_value = ABMultiValueCreateMutable(t_multi_type));
				for (uindex_t j = 0; t_success && j < ELEMENTS(s_label_map); j++)
				{
					MCValueRef t_element;
					if (MCArrayFetchValue((MCArrayRef)t_value, false, *s_label_map[j].name, t_element))
					{
						if (MCValueIsArray(t_element))
						{
							uindex_t t_index = 1;
							MCValueRef t_index_value;
                            
                            // PM-2015-05-21: [[ Bug 14792 ]] t_success should not become false if MCArrayFetchValueAtIndex fails
							while ((MCArrayFetchValueAtIndex((MCArrayRef)t_element, t_index++, t_index_value)))
							{
								if (!s_property_map[i].has_keys)
								{
                                    // PM-2015-05-25: [[ Bug 15403 ]] Convert the valueref to a stringref
                                    MCExecContext ctxt(nil,nil,nil);
                                    MCAutoStringRef t_index_value_string;
                                    /* UNCHECKED */ ctxt.ConvertToString(t_index_value, &t_index_value_string);
									if (MCStringGetLength(*t_index_value_string) > 0)
									{
										t_success = ABMultiValueAddValueAndLabel(t_multi_value,
																				 MCStringConvertToAutoreleasedNSString(*t_index_value_string),
																				 s_label_map[j].label,
																				 nil);
									}
								}
								else if (MCValueIsArray(t_index_value))
								{
									CFDictionaryRef t_dict = nil;
									t_success = MCCFDictionaryFromArray((MCArrayRef)t_index_value, t_dict) &&
									ABMultiValueAddValueAndLabel(t_multi_value, t_dict, s_label_map[j].label, nil);
									if (t_dict != nil)
										CFRelease(t_dict);
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
    
	if (t_success)
		r_person = t_person;
	else if (t_person != nil)
		CFRelease(t_person);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static void requestAuthorization(ABAddressBookRef &x_address_book)
{
#ifdef __IPHONE_6_0
    ABAuthorizationStatus t_status = ABAddressBookGetAuthorizationStatus();
    
    // ABAddressBookGetAuthorizationStatus() returns kABAuthorizationStatusNotDetermined *only* the first time the app is installed. The user will only be prompted the first time access is requested; any subsequent calls will use the existing permissions.
    if (t_status == kABAuthorizationStatusNotDetermined)
    {
        __block bool t_blocking;
        t_blocking = true;
        ABAddressBookRequestAccessWithCompletion(x_address_book, ^(bool granted, CFErrorRef error) {
            MCIPhoneRunBlockOnMainFiber(^(void){
                
                t_blocking = false;
            });
        });
        
        while (t_blocking)
            MCscreen -> wait(1.0, False, True);
    }
#endif
}

bool MCContactAddContact(MCArrayRef p_contact, int32_t& r_chosen)
{
    bool t_success = true;
	
    ABAddressBookRef t_address_book = nil;
    
    // PM-2014-10-08: [[ Bug 13621 ]] ABAddressBookCreate is deprecated in iOS 6. Use ABAddressBookCreateWithOptions instead
    if (MCmajorosversion < 600)
    {
        // Fetch the address book
        t_address_book = ABAddressBookCreate();
    }
    else
    {
#ifdef __IPHONE_6_0
        // The ABAddressBookRef created with ABAddressBookCreateWithOptions will initially not have access to contact data. The app must then call ABAddressBookRequestAccessWithCompletion to request this access.
        t_address_book = ABAddressBookCreateWithOptions(NULL, NULL);
        requestAuthorization(t_address_book);
#endif
    }

	if (t_success)
		t_success = (nil != t_address_book);
	
	ABRecordRef t_contact = nil;
	if (t_success)
		t_success = MCCreatePerson(p_contact, t_contact);
	
	
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
	
    ABAddressBookRef t_address_book = nil;
    
    // PM-2014-10-08: [[ Bug 13621 ]] ABAddressBookCreate is deprecated in iOS 6. Use ABAddressBookCreateWithOptions instead
    if (MCmajorosversion < 600)
    {
        // Fetch the address book
        t_address_book = ABAddressBookCreate();
    }
    else
    {
#ifdef __IPHONE_6_0
        // The ABAddressBookRef created with ABAddressBookCreateWithOptions will initially not have access to contact data. The app must then call ABAddressBookRequestAccessWithCompletion to request this access.
        t_address_book = ABAddressBookCreateWithOptions(NULL, NULL);
        requestAuthorization(t_address_book);
#endif
    }
    
    ABRecordRef t_contact = ABAddressBookGetPersonWithRecordID (t_address_book, p_person_id);
	
	t_success = t_contact != nil && t_address_book != nil;
	if (t_success)
		t_success = ABAddressBookRemoveRecord (t_address_book, t_contact, nil) &&
			ABAddressBookSave(t_address_book, nil);
	
	if (t_address_book != nil)
		CFRelease(t_address_book);
	
	return t_success;
}

bool MCContactFindContact(MCStringRef p_person_name, MCStringRef &r_chosen)
{
	bool t_success = true;
	
	if (p_person_name == nil)
	{
		r_chosen = nil;
		return true;
	}
    
    ABAddressBookRef t_address_book = nil;
    
    // PM-2014-10-08: [[ Bug 13621 ]] ABAddressBookCreate is deprecated in iOS 6. Use ABAddressBookCreateWithOptions instead
    if (MCmajorosversion < 600)
    {
        // Fetch the address book
        t_address_book = ABAddressBookCreate();
    }
    else
    {
#ifdef __IPHONE_6_0
        // The ABAddressBookRef created with ABAddressBookCreateWithOptions will initially not have access to contact data. The app must then call ABAddressBookRequestAccessWithCompletion to request this access.
        t_address_book = ABAddressBookCreateWithOptions(NULL, NULL);
        requestAuthorization(t_address_book);
#endif
    }
    
	t_success = t_address_book != nil;
	
	CFStringRef t_person_name = nil;
    NSArray *t_people = nil;
	NSMutableString *t_chosen = nil;
	
    if (t_success)
		t_success = MCStringConvertToCFStringRef(p_person_name, t_person_name);

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
            [t_chosen appendFormat:@",%d", ABRecordGetRecordID((ABRecordRef)[t_people objectAtIndex:i])];
			}
		}
	}

    // AL-2015-05-14: [[ Bug 15370 ]] Crash when matching contact not found
    if (t_success && t_chosen != nil)
		t_success = MCStringCreateWithCFStringRef((CFStringRef)t_chosen, r_chosen);
	
    if (t_people != nil)
		[t_people release];
	if (t_person_name != nil)
		CFRelease(t_person_name);
	if (t_address_book != nil)
		CFRelease(t_address_book);
	
	return t_success; 
}

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCIPhoneContactDelegate : NSObject
{
	bool m_running, m_finished, m_success;
	UINavigationController *m_navigation;
	ABRecordID m_selected_person;
}

- (id)init;
- (void)dealloc;

@end

@implementation com_runrev_livecode_MCIPhoneContactDelegate

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

@interface com_runrev_livecode_MCIPhonePickContactDelegate : com_runrev_livecode_MCIPhoneContactDelegate <ABPeoplePickerNavigationControllerDelegate>
{
    ABPeoplePickerNavigationController *m_pick_contact;
}

- (id)init;
- (void)dealloc;

@end

@implementation com_runrev_livecode_MCIPhonePickContactDelegate

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
									  [NSNumber numberWithInt:kABPersonBirthdayProperty], nil, nil];
		
		m_pick_contact.displayedProperties = t_displayed_items;
		// Show the picker
		m_running = true;
		
        if (MCmajorosversion >= 500)
        {
            [MCIPhoneGetViewController() presentViewController:m_pick_contact animated:YES completion:nil];
        }
        else
            [MCIPhoneGetViewController() presentModalViewController:m_pick_contact animated:YES];
	}
}

-(bool)showPickContact: (int32_t&) r_chosen
{
#ifdef __IPHONE_8_0
    // PM-2014-11-10: [[ Bug 13979 ]] On iOS 8, we need to request authorization to be able to get a record identifier
    // The ABAddressBookRef created with ABAddressBookCreateWithOptions will initially not have access to contact data. The app must then call ABAddressBookRequestAccessWithCompletion to request this access.
    if (MCmajorosversion >= 800)
    {
        ABAddressBookRef t_address_book = ABAddressBookCreateWithOptions(NULL, NULL);
        requestAuthorization(t_address_book);
    }
#endif

	MCIPhoneCallSelectorOnMainFiber(self, @selector(doShowPickContact));
	
    while (m_running)
		MCscreen -> wait(1.0, False, True);
	
    // PM-2014-10-10: [[ Bug 13639 ]] On iOS 8, the ABPeoplePickerNavigationController is dismissed in peoplePickerNavigationController:didSelectPerson. If [self dismissController] is called, then the completion block of dismissViewControllerAnimated:completion:^(){} in doDismissController is never called. So m_finish never becomes true and the app freezes
    if (MCmajorosversion < 800)
        [self dismissController];
	
    // Return the result
    if (m_selected_person == kABRecordInvalidID)
        r_chosen = 0;
    else
        r_chosen = m_selected_person;
	
	return m_success;
}

// PM-2014-10-10: [[ Bug 13639 ]] In iOS 8, this is the replacement for peoplePickerNavigationController:shouldContinueAfterSelectingPerson
// Called after a person has been selected by the user. It seems that it is also dismissing the ABPeoplePickerNavigationController (m_pick_contact), so we should not call [self dismissController] in showPickContact.
- (void)peoplePickerNavigationController:(ABPeoplePickerNavigationController*)peoplePicker didSelectPerson:(ABRecordRef)person;
{
    if (person != NULL)
        m_selected_person = ABRecordGetRecordID(person);
    m_running = false;
    return;
}


- (void)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker didSelectPerson:(ABRecordRef)person property:(ABPropertyID)property identifier:(ABMultiValueIdentifier)identifier
{
    if (person != NULL)
        m_selected_person = ABRecordGetRecordID(person);
    m_running = false;
    return;
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

@interface com_runrev_livecode_MCIPhoneShowContactDelegate : com_runrev_livecode_MCIPhoneContactDelegate <ABPersonViewControllerDelegate>
{
	ABPersonViewController *m_view_contact;
}

- (id)init;
- (void)dealloc;

@end

@implementation com_runrev_livecode_MCIPhoneShowContactDelegate

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
    
    // ABAddressBookCreate is deprecated in iOS 6. Use ABAddressBookCreateWithOptions instead
    if (MCmajorosversion < 600)
    {
        // Fetch the address book
        t_address_book = ABAddressBookCreate();
    }
    else
    {
#ifdef __IPHONE_6_0
        // The ABAddressBookRef created with ABAddressBookCreateWithOptions will initially not have access to contact data. The app must then call ABAddressBookRequestAccessWithCompletion to request this access.
        t_address_book = ABAddressBookCreateWithOptions(NULL, NULL);
        requestAuthorization(t_address_book);
#endif
    }
    
    m_success = t_address_book != nil;

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
		
        // PM-2014-12-10: [[ Bug 13168 ]] Add a Cancel button to allow dismissing mobileShowContact
        if (m_success)
            m_success = nil != (m_view_contact.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(handleGetContactCancel)]);
        
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

// PM-2014-12-10: [[ Bug 13168 ]] If Cancel button is pressed return to the app
-(void) handleGetContactCancel
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

@interface com_runrev_livecode_MCIPhoneCreateContactDelegate : com_runrev_livecode_MCIPhoneContactDelegate <ABNewPersonViewControllerDelegate>
{
	ABNewPersonViewController *m_get_contact;
}

- (id)init;
- (void)dealloc;

@end

@implementation com_runrev_livecode_MCIPhoneCreateContactDelegate

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

@interface com_runrev_livecode_MCIPhoneUpdateContactDelegate : com_runrev_livecode_MCIPhoneContactDelegate <ABUnknownPersonViewControllerDelegate>
{
	ABUnknownPersonViewController *m_update_contact;
}

- (id)init;
- (void)dealloc;

@end

@implementation com_runrev_livecode_MCIPhoneUpdateContactDelegate

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
			   withTitle: (MCStringRef)p_title withMessage: (MCStringRef)p_message withAlternateName: (MCStringRef)p_alternate_name
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
            t_title = MCStringConvertToAutoreleasedNSString(p_title == nil ? kMCEmptyString : p_title);
            t_message = MCStringConvertToAutoreleasedNSString(p_message == nil ? kMCEmptyString : p_message);
			t_alternate_name = MCStringConvertToAutoreleasedNSString(p_alternate_name == nil ? kMCEmptyString : p_alternate_name);
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
        
        // PM-2014-12-10: [[ Bug 13169 ]] Add a Cancel button to allow dismissing mobileUpdateContact
        if (m_success)
            m_success = nil != (m_update_contact.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(handleUpdateContactCancel)]);
        
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

// PM-2014-12-10: [[ Bug 13169 ]] If Cancel button is pressed return to the app
-(void) handleUpdateContactCancel
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
	
    com_runrev_livecode_MCIPhonePickContactDelegate *t_pick_contact = nil;
	t_success = nil != (t_pick_contact = [[com_runrev_livecode_MCIPhonePickContactDelegate alloc] init]);
	
	if (t_success)
		t_success = [t_pick_contact showPickContact: r_result];
	
	if (t_pick_contact != nil)
		[t_pick_contact release];
	
    return t_success;
}

bool MCSystemShowContact(int32_t p_contact_id, int32_t& r_result) // ABPersonViewController
{
    bool t_success = true;
	
    com_runrev_livecode_MCIPhoneShowContactDelegate *t_view_contact = nil;
	t_success = nil != (t_view_contact = [[com_runrev_livecode_MCIPhoneShowContactDelegate alloc] init]);
	
	if (t_success)
		t_success = [t_view_contact showViewContact:p_contact_id withResult: r_result];
	
	if (t_view_contact != nil)
		[t_view_contact release];
	
    return t_success;
}

bool MCSystemCreateContact(int32_t& r_result) // ABNewPersonViewController
{
    bool t_success = true;
	
    com_runrev_livecode_MCIPhoneCreateContactDelegate *t_create_contact = nil;
	t_success = nil != (t_create_contact = [[com_runrev_livecode_MCIPhoneCreateContactDelegate alloc] init]);
	
	if (t_success)
		t_success = [t_create_contact showCreateContact:r_result];
	
	if (t_create_contact != nil)
		[t_create_contact release];

    return t_success;
}

bool MCSystemUpdateContact(MCArrayRef p_contact, MCStringRef p_title, MCStringRef p_message, MCStringRef p_alternate_name,
						   int32_t &r_result)
{
	bool t_success = true;

	ABRecordRef t_contact = nil;
	t_success = MCCreatePerson(p_contact, t_contact);

	com_runrev_livecode_MCIPhoneUpdateContactDelegate *t_update_contact = nil;
	if (t_success)
		t_success = nil != (t_update_contact = [[com_runrev_livecode_MCIPhoneUpdateContactDelegate alloc] init]);
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

bool MCSystemGetContactData(int32_t p_contact_id, MCArrayRef &r_contact_data)
{
	bool t_success = true;
	
    ABAddressBookRef t_address_book = nil;
    
    // PM-2014-10-08: [[ Bug 13621 ]] ABAddressBookCreate is deprecated in iOS 6. Use ABAddressBookCreateWithOptions instead
    if (MCmajorosversion < 600)
    {
        // Fetch the address book
        t_address_book = ABAddressBookCreate();
    }
    else
    {
#ifdef __IPHONE_6_0
        // The ABAddressBookRef created with ABAddressBookCreateWithOptions will initially not have access to contact data. The app must then call ABAddressBookRequestAccessWithCompletion to request this access.
        t_address_book = ABAddressBookCreateWithOptions(NULL, NULL);
        requestAuthorization(t_address_book);
#endif
    }

	t_success = (nil != t_address_book);
	
    ABRecordRef t_person = nil;
	if (t_success)
		t_success = nil != (t_person = ABAddressBookGetPersonWithRecordID (t_address_book, p_contact_id));
	
	if (t_success)
		t_success = MCCreatePersonData(t_person, r_contact_data);
	
	if (t_address_book != nil)
		CFRelease(t_address_book);
	
	return t_success; 
}

bool MCSystemRemoveContact(int32_t p_contact_id)
{
	return MCContactDeleteContact(p_contact_id);
}

bool MCSystemAddContact(MCArrayRef p_contact, int32_t& r_result)
{
	return MCContactAddContact(p_contact, r_result);
}

bool MCSystemFindContact(MCStringRef p_contact_name, MCStringRef& r_result)
{
	return MCContactFindContact(p_contact_name, r_result);
}
