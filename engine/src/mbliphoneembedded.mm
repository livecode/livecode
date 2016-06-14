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
#include "mcio.h"

#include "param.h"
#include "uidc.h"

#include "osspec.h"
#include "globals.h"
#include "stack.h"
#include "card.h"
#include "eventqueue.h"

#include "mbliphoneview.h"

#include <stdarg.h>
#include <objc/runtime.h>

////////////////////

extern "C" UIView *LiveCodeGetView(void);

extern MCExecPoint *MCEPptr;

////////////////////

typedef uint8_t ObjcType;
enum
{
	kObjcTypeVoid,
	kObjcTypeChar,
	kObjcTypeInt,
	kObjcTypeShort,
	kObjcTypeLong,
	kObjcTypeLongLong,
	kObjcTypeUnsignedChar,
	kObjcTypeUnsignedInt,
	kObjcTypeUnsignedShort,
	kObjcTypeUnsignedLong,
	kObjcTypeUnsignedLongLong,
	kObjcTypeFloat,
	kObjcTypeDouble,
	kObjcTypeBool,
	kObjcTypeCString,
	kObjcTypeObject,
	kObjcTypeNumber,
	kObjcTypeString,
	kObjcTypeData,
	kObjcTypeArray,
	kObjcTypeDictionary,
};

union ObjcValue
{
	char c;
	int i;
	short s;
	long l;
	long long q;
	unsigned char C;
	unsigned int I;
	unsigned short S;
	unsigned long L;
	unsigned long long Q;
	float f;
	double d;
	bool B;
	const char *cstring; // *
	id object; // @
	NSNumber *number; // #N
	NSString *string; // #S
	NSData *data; // #D
	NSArray *array; // #A
	NSDictionary *dictionary; // #T
};

struct DelegateMethod
{
	char *message;
	MCNameRef message_name;
	ObjcType *native_args;
	uindex_t native_arg_count;
ObjcType native_return_type;
	SEL selector;
};

static uindex_t native_signature_measure(const char *p_signature)
{
	if (*p_signature == '\0')
		return 0;
	
	uindex_t t_count;
	t_count = 0;
	for(uindex_t i = 0; p_signature[i] != '\0'; i++)
	{
		if (p_signature[i] == '#')
			i++;
		t_count++;
	}
	
	return t_count;
}

static bool native_signature_parse(const char *p_signature, ObjcType*& r_native_args, uindex_t& r_native_arg_count, ObjcType& r_native_return_type)
{
	bool t_success;
	t_success = true;

	uindex_t t_count;
	t_count = native_signature_measure(p_signature);
	
	ObjcType *t_args;
	t_args = (ObjcType*)malloc(t_count);
	
	for(uindex_t i = 0, j = 0; p_signature[i] != '\0'; i++)
	{
		ObjcType t_type;
		if (p_signature[i] == '#')
		{
			i++;
			switch(p_signature[i])
			{
			case 'N': t_type = kObjcTypeNumber; break;
			case 'S': t_type = kObjcTypeString; break;
			case 'D': t_type = kObjcTypeData; break;
			case 'A': t_type = kObjcTypeArray; break;
			case 'T': t_type = kObjcTypeDictionary; break;
			default:
				t_success = false;
				break;
			}
		}
		else
		{
			switch(p_signature[i])
			{
			case 'v': t_type = kObjcTypeVoid; break;
			case 'c': t_type = kObjcTypeChar; break;
			case 'i': t_type = kObjcTypeInt; break;
			case 's': t_type = kObjcTypeShort; break;
			case 'l': t_type = kObjcTypeLong; break;
			case 'q': t_type = kObjcTypeLongLong; break;
			case 'C': t_type = kObjcTypeUnsignedChar; break;
			case 'I': t_type = kObjcTypeUnsignedInt; break;
			case 'S': t_type = kObjcTypeUnsignedShort; break;
			case 'L': t_type = kObjcTypeUnsignedLong; break;
			case 'Q': t_type = kObjcTypeUnsignedLongLong; break;
			case 'f': t_type = kObjcTypeFloat; break;
			case 'd': t_type = kObjcTypeDouble; break;
			case 'B': t_type = kObjcTypeBool; break;
			case '*': t_type = kObjcTypeCString; break;
			case '@': t_type = kObjcTypeObject; break;
			default:
				t_success = false;
				break;
			}
		}
		
		t_args[j++] = t_type;
	}

	if (t_success)
	{
		r_native_return_type = t_args[0];
		memmove(t_args, t_args + 1, t_count - 1);
		r_native_args = t_args;
		r_native_arg_count = t_count - 1;
	}
	else
		free(t_args);

	return t_success;
}

static const char *native_signature_to_objc(ObjcType p_type)
{
	const char *t_tag;
	switch(p_type)
	{
	case kObjcTypeVoid:
		t_tag = "v";
		break;
	case kObjcTypeChar:
		t_tag = "c";
		break;
	case kObjcTypeInt:
		t_tag = "i";
		break;
	case kObjcTypeShort:
		t_tag = "s";
		break;
	case kObjcTypeLong:
		t_tag = "l";
		break;
	case kObjcTypeLongLong:
		t_tag = "q";
		break;
	case kObjcTypeUnsignedChar:
		t_tag = "C";
		break;
	case kObjcTypeUnsignedInt:
		t_tag = "I";
		break;
	case kObjcTypeUnsignedShort:
		t_tag = "S";
		break;
	case kObjcTypeUnsignedLong:
		t_tag = "L";
		break;
	case kObjcTypeUnsignedLongLong:
		t_tag = "Q";
		break;
	case kObjcTypeFloat:
		t_tag = "f";
		break;
	case kObjcTypeDouble:
		t_tag = "d";
		break;
	case kObjcTypeBool:
		t_tag = "B";
		break;
	case kObjcTypeCString:
		t_tag = "*";
		break;
	case kObjcTypeObject:
		t_tag = "@";
		break;
	case kObjcTypeNumber:
		t_tag = "@";
		break;
	case kObjcTypeString:
		t_tag = "@";
		break;
	case kObjcTypeData:
		t_tag = "@";
		break;
	case kObjcTypeArray:
		t_tag = "@";
		break;
	case kObjcTypeDictionary:
		t_tag = "@";
		break;
	default:
	t_tag = "v";
		break;
	}
	return t_tag;
}

static bool param_to_objc_value(MCExecPoint& ep, MCParameter *p_param, ObjcType p_as_type, ObjcValue& r_objc_value)
{
	p_param -> eval_argument(ep);
	
	switch(p_as_type)
	{
	case kObjcTypeVoid:
		break;
	case kObjcTypeChar:
		ep.ton();
		r_objc_value . c = ep . getint4();
		break;
	case kObjcTypeInt:
		ep.ton();
		r_objc_value . i = ep . getint4();
		break;
	case kObjcTypeShort:
		ep.ton();
		r_objc_value . s = ep . getint4();
		break;
	case kObjcTypeLong:
		ep.ton();
		r_objc_value . l = ep . getint4();
		break;
	case kObjcTypeLongLong:
		ep.ton();
		r_objc_value . q = ep . getint8();
		break;
	case kObjcTypeUnsignedChar:
		ep.ton();
		r_objc_value . c = ep . getuint4();
		break;
	case kObjcTypeUnsignedInt:
		ep.ton();
		r_objc_value . i = ep . getuint4();
		break;
	case kObjcTypeUnsignedShort:
		ep.ton();
		r_objc_value . s = ep . getuint4();
		break;
	case kObjcTypeUnsignedLong:
		ep.ton();
		r_objc_value . l = ep . getuint4();
		break;
	case kObjcTypeUnsignedLongLong:
		ep.ton();
		r_objc_value . q = ep . getuint8();
		break;
	case kObjcTypeFloat:
		ep.ton();
		r_objc_value . f = ep . getnvalue();
		break;
	case kObjcTypeDouble:
		ep.ton();
		r_objc_value . d = ep . getnvalue();
		break;
	case kObjcTypeBool:
		r_objc_value . B = ep . getsvalue() == MCtruemcstring;
		break;
	case kObjcTypeCString:
		r_objc_value . cstring = [[NSString stringWithCString: ep . getcstring() encoding: NSMacOSRomanStringEncoding] cStringUsingEncoding: NSMacOSRomanStringEncoding];
		break;
	case kObjcTypeObject:
		r_objc_value . object = nil;
		break;
	case kObjcTypeNumber:
		ep . ton();
		r_objc_value . number = [NSNumber numberWithDouble: ep . getnvalue()];
		break;
	case kObjcTypeString:
		r_objc_value . string = [NSString stringWithCString: ep . getcstring() encoding: NSMacOSRomanStringEncoding];
		break;
	case kObjcTypeData:
		r_objc_value . data = [NSData dataWithBytes: ep . getsvalue() . getstring() length: ep . getsvalue() . getlength()];
		break;
	case kObjcTypeArray:
		r_objc_value . array = nil;
		break;
	case kObjcTypeDictionary:
		r_objc_value . dictionary = nil;
		break;
	}
	
	return true;
}

static void objc_value_to_livecode(MCExecPoint& ep, ObjcType p_type, const ObjcValue& p_value)
{
	switch(p_type)
	{
	case kObjcTypeVoid:
		break;
	case kObjcTypeChar:
		ep . setint(p_value . c);
		break;
	case kObjcTypeInt:
		ep . setint(p_value . i);
		break;
	case kObjcTypeShort:
		ep . setint(p_value . s);
		break;
	case kObjcTypeLong:
		ep . setint(p_value . l);
		break;
	case kObjcTypeLongLong:
		ep . setint64(p_value . q);
		break;
	case kObjcTypeUnsignedChar:
		ep . setuint(p_value . C);
		break;
	case kObjcTypeUnsignedInt:
		ep . setuint(p_value . I);
		break;
	case kObjcTypeUnsignedShort:
		ep . setuint(p_value . S);
		break;
	case kObjcTypeUnsignedLong:
		ep . setuint(p_value . L);
		break;
	case kObjcTypeUnsignedLongLong:
		ep . setuint64(p_value . Q);
		break;
	case kObjcTypeFloat:
		ep . setnvalue(p_value . f);
		break;
	case kObjcTypeDouble:
		ep . setnvalue(p_value . d);
		break;
	case kObjcTypeBool:
		ep . setboolean(p_value . B);
		break;
	case kObjcTypeCString:
		ep . copysvalue(p_value . cstring);
		break;
	case kObjcTypeObject:
		ep . copysvalue([[p_value . object description] cStringUsingEncoding: NSMacOSRomanStringEncoding]);
		break;
	case kObjcTypeNumber:
		ep . setnvalue([p_value . number doubleValue]);
		break;
	case kObjcTypeString:
		ep . copysvalue([p_value . string cStringUsingEncoding: NSMacOSRomanStringEncoding]);
		break;
	case kObjcTypeData:
		ep . copysvalue((const char *)[p_value . data bytes], [p_value . data length]);
		break;
	case kObjcTypeArray:
		ep . clear();
		break;
	case kObjcTypeDictionary:
		ep . clear();
		break;
	default:
		break;
	}
}

class MCEmbeddedPostEvent: public MCCustomEvent
{
public:
	MCEmbeddedPostEvent(MCObject *p_target, MCNameRef p_message, void (^p_completion)(id), MCParameter *p_arguments)
	{
		MCNameClone(p_message, m_message);
		m_completion = Block_copy(p_completion);
		m_arguments = p_arguments;
		m_target = p_target -> gethandle();
	}
	
	void Destroy(void)
	{
		MCNameDelete(m_message);
		[m_completion release];		
		while(m_arguments != nil)
		{
			MCParameter *t_param;
			t_param = m_arguments;
			m_arguments = m_arguments -> getnext();
			delete t_param;
		}
		m_target -> Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		if (!m_target -> Exists())
			return;
			
		m_target -> Get() -> message(m_message, m_arguments);
		
		if (m_completion != nil)
		{
            MCAutoValueRef t_value;
            MCresult->copyasvalueref(&t_value);
			
			id t_result;
            if (MCValueIsEmpty(*t_value))
				t_result = nil;

            else if (MCValueGetTypeCode(*t_value) == kMCValueTypeCodeArray)
			{
				t_result = nil;
			}
            else if (MCValueGetTypeCode(*t_value) == kMCValueTypeCodeNumber)
                t_result = [NSNumber numberWithDouble: MCNumberFetchAsReal((MCNumberRef)*t_value)];
            else if (MCValueGetTypeCode((*t_value) == kMCValueTypeCodeString))
                t_result = [NSString stringWithMCStringRef: (MCStringRef)*t_value];
            else if (MCValueGetTypeCode((*t_value) == kMCValueTypeCodeName))
                t_result = [NSString stringWithMCNameRef: (MCNameRef)*t_value)];
            else if (MCValueGeTypeCode((*t_value) == kMCValueTypeCodeData))
            {
                MCAutoStringRef t_string;
                MCStringDecode((MCDataRef)*t_value, kMCStringEncodingNative, false, &t_string);
                t_result = [NSString stringWithMCStringRef: *t_string];
            }
				
			m_completion(t_result);
		}
	}

private:
	MCObjectHandle *m_target;
	MCNameRef m_message;
	void (^m_completion)(id);
	MCParameter *m_arguments;
};

static void post_message_to_engine(MCObject *p_target, NSString *p_message, void (^p_completion)(id), va_list p_arguments)
{
	MCAutoNameRef t_message;
	t_message . CreateWithCString([p_message cStringUsingEncoding: NSMacOSRomanStringEncoding]);
	
	MCParameter *t_parameters, *t_last_parameter;
	t_parameters = nil;
	t_last_parameter = nil;

	if (p_arguments != nil)
	{
		for(;;)
		{
			id t_arg;
			t_arg = va_arg(p_arguments, id);
			if (t_arg == nil)
				break;

			MCParameter *t_new_parameter;
			t_new_parameter = new MCParameter;
			if (t_last_parameter != nil)
				t_last_parameter -> setnext(t_new_parameter);
			else
				t_parameters = t_new_parameter;

			MCExecPoint ep;
			[t_arg com_runrev_livecode_convertToLiveCode: &ep];
			t_new_parameter -> set_argument(ep);

			t_last_parameter = t_new_parameter;
		}
	}

	MCEmbeddedPostEvent *t_event;
	t_event = new MCEmbeddedPostEvent(p_target, t_message, p_completion, t_parameters);
	MCEventQueuePostCustom(t_event);
}

////////////////////

@interface NSObject (com_runrev_livecode_MCIPhoneValueConversion)

- (void)com_runrev_livecode_convertToLiveCode: (MCExecPoint *)ep;

@end

@implementation NSObject (com_runrev_livecode_MCIPhoneValueConversion)

- (void)com_runrev_livecode_convertToLiveCode: (MCExecPoint *)ep
{
	ep -> copysvalue([[self description] cStringUsingEncoding: NSMacOSRomanStringEncoding]);
}

@end

@implementation NSString (com_runrev_livecode_MCIPhoneValueConversion)

- (void)com_runrev_livecode_convertToLiveCode: (MCExecPoint *)ep
{
	ep -> copysvalue([self cStringUsingEncoding: NSMacOSRomanStringEncoding]);
}

@end

@implementation NSData (com_runrev_livecode_MCIPhoneValueConversion)

- (void)com_runrev_livecode_convertToLiveCode: (MCExecPoint *)ep
{
	ep -> copysvalue((const char *)[self bytes], [self length]);
}

@end

@implementation NSNumber (com_runrev_livecode_MCIPhoneValueConversion)

- (void)com_runrev_livecode_convertToLiveCode: (MCExecPoint *)ep
{
	if (strcmp([self objCType], @encode(BOOL)) == 0)
		ep -> setboolean([self boolValue]);
	else
		ep -> setnvalue([self doubleValue]);
}

@end

/*@implementation NSArray (com_runrev_livecode_MCIPhoneValueConversion)

- (void)com_runrev_livecode_convertToLiveCode: (MCExecPoint *)ep
{
	
}

@end*/

//////////

@interface com_runrev_livecode_MCIPhoneEmbeddedView : com_runrev_livecode_MCIPhoneRootView
{
	bool m_running : 1;
	
	bool m_activate_keyboard_pending : 1;
	
	CGRect m_view_bounds;
	
	id m_delegate;
	bool m_delegate_methods_changed : 1;
	DelegateMethod *m_delegate_methods;
	uindex_t m_delegate_method_count;
}

//////////

- (id)init;
- (void)dealloc;

- (void)startup;
- (void)shutdown;

//////////

- (void)setViewBounds: (CGRect)frame;
- (CGRect)viewBounds;

//////////

- (void)setDelegate:(id)delegate;
- (id)delegate;

- (BOOL)mapMessage:(const char *)message withSignature:(const char *)signature toSelector:(SEL)selector;
- (BOOL)handle:(MCNameRef)message arguments:(MCParameter *)arguments;

//////////

- (void)post:(NSString *)message;
- (void)post:(NSString *)message completion:(void (^)(id))completion;
- (void)post:(NSString *)message arguments:(id)firstArg, ...;
- (void)post:(NSString *)message completion:(void (^)(id))completion arguments:(id)firstArg, ...;
- (void)postV:(NSString *)message completion:(void (^)(id))completion arguments:(va_list)args;

- (BOOL)send:(NSString *)message;
- (BOOL)send:(NSString *)message result:(id *)result;
- (BOOL)send:(NSString *)message result:(id *)result arguments:(id)firstArg, ...;
- (BOOL)send:(NSString *)message completion:(void (^)(id))completion;
- (BOOL)send:(NSString *)message completion:(void (^)(id))completion arguments:(id)firstArg, ...;
- (BOOL)sendV:(NSString *)message completion:(void (^)(id))completion arguments:(va_list)args;

///////////

- (void)activateKeyboard;
- (void)deactivateKeyboard;

- (void)startPreparing;
- (void)startExecuting;

//////////

@end

@implementation com_runrev_livecode_MCIPhoneEmbeddedView

////////////////////

- (id)init
{
	self = [super init];
	if (self == nil)
		return nil;
		
	m_running = false;
	m_activate_keyboard_pending = false;
	m_view_bounds = CGRectZero;
	
	m_delegate = nil;
	m_delegate_methods_changed = false;
	m_delegate_method_count = 0;
	m_delegate_methods = nil;
	
	[self setClipsToBounds: YES];
	
	return self;
}

- (void)dealloc
{
	for(uindex_t i = 0; i < m_delegate_method_count; i++)
	{
		free(m_delegate_methods[i] . message);
		MCNameDelete(m_delegate_methods[i] . message_name);
		free(m_delegate_methods[i] . native_args);
	}
	MCMemoryDeleteArray(m_delegate_methods);

	[super dealloc];
}

////////////////////

- (void)setDelegate:(id)p_new_delegate
{
	m_delegate = p_new_delegate;
}

- (id)delegate
{
	return m_delegate;
}

////////////////////

- (BOOL)mapMessage:(const char *)p_message withSignature:(const char *)p_signature toSelector:(SEL)p_selector
{
	// If there is no delegate we cannot map.
	if (m_delegate == nil)
	{
		NSLog(@"{LCEmbedded} mapMessage called with no delegate");
		return NO;
	}
	
	ObjcType *t_native_args;
	uindex_t t_native_arg_count;
	ObjcType t_native_return_type;
	if (!native_signature_parse(p_signature, t_native_args, t_native_arg_count, t_native_return_type))
	{
		NSLog(@"{LCEmbedded} mapMessage called with invalid signature");
		return NO;
	}
	
	// First check to see if the delegate accepts the selector.
	NSMethodSignature *t_signature;
	t_signature = [m_delegate methodSignatureForSelector: p_selector];
	if (t_signature == nil)
	{
		NSLog(@"{LCEmbedded} mapMessage called for non-existant delegate selector");
		return NO;
	}

	// Check that the number of args matches.
	if ([t_signature numberOfArguments] != t_native_arg_count + 2)
	{
		NSLog(@"{LCEmbedded} mapMessage signature has wrong number of args");
		return NO;
	}
	
	// Check that all the args match.
	for(uindex_t i = 0; i < t_native_arg_count; i++)
		if (strcmp([t_signature getArgumentTypeAtIndex: i + 2], native_signature_to_objc(t_native_args[i])) != 0)
		{
			NSLog(@"{LCEmbedded} mapMessage signature argument %d type mismatch", i);
			return NO;
		}
	
	// Check that it matches the method result signature.
	if (strcmp([t_signature methodReturnType], native_signature_to_objc(t_native_return_type)) != 0)
	{
		NSLog(@"{LCEmbedded} mapMessage signature return type mismatch");
		return NO;
	}
	
	// Check to see if there's already an entry for it, and if so update.
	for(uindex_t i = 0; i < m_delegate_method_count; i++)
		if (strcasecmp(p_message, m_delegate_methods[i] . message) == 0)
		{
			free(m_delegate_methods[i] . native_args);
			m_delegate_methods[i] . native_args = t_native_args;
			m_delegate_methods[i] . native_arg_count = t_native_arg_count;
			m_delegate_methods[i] . native_return_type = t_native_return_type;
		m_delegate_methods[i] . selector = p_selector;
			return YES;
		}

	// Append the name to the end of the array.
	MCMemoryResizeArray(m_delegate_method_count + 1, m_delegate_methods, m_delegate_method_count);
	m_delegate_methods[m_delegate_method_count - 1] . message = strdup(p_message);
	m_delegate_methods[m_delegate_method_count - 1] . native_args = t_native_args;
	m_delegate_methods[m_delegate_method_count - 1] . native_arg_count = t_native_arg_count;
	m_delegate_methods[m_delegate_method_count - 1] . native_return_type = t_native_return_type;
	m_delegate_methods[m_delegate_method_count - 1] . selector = p_selector;
	
	// Mark the list as changed (will cause it to be sorted in future).
	m_delegate_methods_changed = true;
	
	return YES;
}

- (BOOL)handle:(MCNameRef)p_message arguments:(MCParameter *)p_parameters
{
	// If there is no delegate, there is nothing to do.
	if (m_delegate == nil)
		return NO;

	// If the methods have changed then update.
	if (m_delegate_methods_changed)
	{
		for(uindex_t i = 0; i < m_delegate_method_count; i++)
			if (m_delegate_methods[i] . message_name == nil)
				MCNameCreateWithCString(m_delegate_methods[i] . message, m_delegate_methods[i] . message_name);

		m_delegate_methods_changed = false;
	}

	// Lookup which selector to map to. For now, just do a linear search, should
	// replace with bsearch later...
	SEL t_selector;
	ObjcType *t_native_args;
	uindex_t t_native_arg_count;
	ObjcType t_native_return_type;
	t_native_args = nil;
	t_native_arg_count = 0;
	t_native_return_type = kObjcTypeVoid;
	t_selector = nil;
	for(uindex_t i = 0; i < m_delegate_method_count; i++)
		if (MCNameIsEqualTo(p_message, m_delegate_methods[i] . message_name, kMCCompareCaseless))
		{
			t_selector = m_delegate_methods[i] . selector;
			t_native_args = m_delegate_methods[i] . native_args;
			t_native_arg_count = m_delegate_methods[i] . native_arg_count;
			t_native_return_type = m_delegate_methods[i] . native_return_type;
			break;
		}

	// If the selector is nil, we are done.
	if (t_selector == nil)
		return NO;

	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
		
	// Now lookup the selector in the delegate.
	NSMethodSignature *t_signature;
	t_signature = [m_delegate methodSignatureForSelector: t_selector];
	if (t_signature == NULL)
		return NO;
	
	// Now create an invocation object, and build it.
	NSInvocation *t_invocation;
	t_invocation = [NSInvocation invocationWithMethodSignature: t_signature];
	[t_invocation setSelector: t_selector];
	[t_invocation setTarget: m_delegate];
	
	// Create an ep from the current context.
	MCExecPoint ep(nil, nil, nil);
	for(uindex_t i = 2; i < [t_signature numberOfArguments]; i++)
	{
		// Convert the parameter value to obj-c, or empty if failure.
		ObjcValue t_value;
		if (p_parameters == nil ||
			!param_to_objc_value(ep, p_parameters, t_native_args[i - 2], t_value))
			memset(&t_value, 0, sizeof(ObjcValue));
			
		// Copy the value into the invocation.
		[t_invocation setArgument: &t_value atIndex: i];
		
		// Advance to the next parameter.
		p_parameters = p_parameters -> getnext();
	}
	
	// Invoke the message send.
	[t_invocation invoke];
	
	// Now process the return type.
	if (t_native_return_type == kObjcTypeVoid)
		MCresult -> clear();
	else
	{
		ObjcValue t_value;
		[t_invocation getReturnValue: &t_value];
		objc_value_to_livecode(ep, t_native_return_type, t_value);
		MCresult -> store(ep, True);
	}
	
	[t_pool release];

	return YES;
}

////////////////////

- (void)post:(NSString *)p_message
{
	[self post: p_message completion: nil arguments: nil];
}

- (void)post:(NSString *)p_message completion:(void (^)(id))p_completion
{
	[self post: p_message completion: p_completion arguments: nil];
}

- (void)post:(NSString *)p_message arguments:(id)p_first_arg, ...
{
	va_list t_args;
	va_start(t_args, p_first_arg);
	[self postV: p_message completion: nil arguments: t_args];
	va_end(t_args);
}

- (void)post:(NSString *)p_message completion:(void (^)(id))p_completion arguments:(id)p_first_arg, ...
{
	va_list t_args;
	va_start(t_args, p_first_arg);
	if (p_first_arg != nil)
		[self postV: p_message completion: p_completion arguments: t_args];
	else
		[self postV: p_message completion: p_completion arguments: nil];
	va_end(t_args);
}

- (void)postV:(NSString *)p_message completion:(void (^)(id))p_completion arguments:(va_list)p_args
{
	post_message_to_engine([self currentStack] -> getcard(), p_message, p_completion, p_args);
}

////////////////////

- (BOOL)send:(NSString *)message
{
	return NO;
}

- (BOOL)send:(NSString *)message result:(id *)result
{
	return NO;
}

- (BOOL)send:(NSString *)message result:(id *)result arguments:(id)firstArg, ...
{
	return NO;
}

- (BOOL)send:(NSString *)message completion:(void (^)(id))completion
{
	return NO;
}

- (BOOL)send:(NSString *)message completion:(void (^)(id))completion arguments:(id)firstArg, ...
{
	return NO;
}

- (BOOL)sendV:(NSString *)message completion:(void (^)(id))completion arguments:(va_list)args
{
	return NO;
}

////////////////////

- (void)startup
{
	m_running = true;
	
	MCIPhoneHandleDidBecomeActive();
	
	[self performSelector: @selector(startPreparing) withObject: nil afterDelay: 0.0];
}

- (void)shutdown
{
	MCIPhoneHandleWillTerminate();
}

- (void)startPreparing
{
	[self setViewBounds: m_view_bounds];
		
	MCIPhoneHandleDidStartPreparing();

	[self performSelector: @selector(startExecuting) withObject: nil afterDelay: 0.0];
}

- (void)startExecuting
{
	if (m_activate_keyboard_pending)
		[self activateKeyboard];

	MCIPhoneHandleDidStartExecuting();
}

////////////////////

- (void)setViewBounds: (CGRect)p_bounds
{
	m_view_bounds = p_bounds;
	
	if (m_running)
		[self setFrame: p_bounds];
	
	CGRect t_frame;
	t_frame = [self frame];
}

- (CGRect)viewBounds
{
	return m_view_bounds;
}

//////////

- (void)activateKeyboard
{
	if (!m_running)
	{
		m_activate_keyboard_pending = true;
		return;
	}
	
	[super activateKeyboard];

	m_activate_keyboard_pending = false;
}

- (void)deactivateKeyboard
{
	if (!m_running)
	{
		m_activate_keyboard_pending = false;
		return;
	}
	
	[super deactivateKeyboard];
	
	m_activate_keyboard_pending = false;
}

////////////////////

@end

static com_runrev_livecode_MCIPhoneEmbeddedView *s_view = nil;

UIView *LiveCodeGetView(void)
{
	if (s_view == nil)
		s_view = [[com_runrev_livecode_MCIPhoneEmbeddedView alloc] init];
	return s_view;
}

//////////

UIView *MCIPhoneGetView(void)
{
	return [s_view mainView];
}

UIView *MCIPhoneGetRootView(void)
{
	return s_view;
}

UIView *MCIPhoneGetDisplayView(void)
{
	return [s_view displayView];
}

UIViewController *MCIPhoneGetViewController(void)
{
	UIResponder *t_responder;
	t_responder = s_view;
	while(![t_responder isKindOfClass: [UIViewController class]])
	{
		t_responder = [t_responder nextResponder];
		if (t_responder == nil)
			break;
	}
	
	return (UIViewController *)t_responder;
}

MCIPhoneApplication *MCIPhoneGetApplication(void)
{
	return nil;
}

//////////

CGRect MCIPhoneGetViewBounds(void)
{
	return [s_view viewBounds];
}

void MCIPhoneSetViewBounds(CGRect p_rect)
{
	if (s_view == nil)
		return;
		
	[s_view setViewBounds: p_rect];
}

CGRect MCIPhoneGetScreenBounds(void)
{
	return [s_view viewBounds];
}

//////////

UIKeyboardType MCIPhoneGetKeyboardType(void)
{
	return [[s_view textView] keyboardType];
}

UIReturnKeyType MCIPhoneGetReturnKeyType(void)
{
	return [[s_view textView] returnKeyType];
}

void MCIPhoneSetKeyboardType(UIKeyboardType p_type)
{
	[[s_view textView] setKeyboardType: p_type];
}

void MCIPhoneSetReturnKeyType(UIReturnKeyType p_type)
{
	[[s_view textView] setReturnKeyType: p_type];
}

void MCIPhoneActivateKeyboard(void)
{
	[s_view activateKeyboard];
}

void MCIPhoneDeactivateKeyboard(void)
{
	[s_view deactivateKeyboard];
}

//////////

void MCIPhoneSwitchViewToUIKit(void)
{
	[s_view switchToDisplayClass: [MCIPhoneUIKitDisplayView class]];
}

void MCIPhoneSwitchViewToOpenGL(void)
{
	[s_view switchToDisplayClass: [MCIPhoneOpenGLDisplayView class]];
}

void MCIPhoneConfigureContentScale(int p_scale)
{
	UIView *t_display_view;
	t_display_view = [s_view displayView];
	if ([t_display_view isMemberOfClass: [MCIPhoneOpenGLDisplayView class]])
		[t_display_view setContentScaleFactor: p_scale];
}

bool MCIPhoneIsEmbedded(void)
{
	return true;
}

//////////

UIInterfaceOrientation MCIPhoneGetOrientation(void)
{
	return [MCIPhoneGetViewController() interfaceOrientation];
}

//////////

Exec_stat MCIPhoneHandleMessage(MCNameRef p_message, MCParameter *p_parameters)
{
	__block bool t_handled;
	t_handled = false;
	MCIPhoneRunBlockOnMainFiber(^(void) {
		t_handled = [s_view handle: p_message arguments: p_parameters];
	});
	
	if (t_handled)
		return ES_NORMAL;
	
	return ES_NOT_HANDLED;
}

//////////

void add_simulator_redirect(const char *p_redirect_def)
{
}

//////////
