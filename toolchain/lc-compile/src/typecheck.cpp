/* Copyright (C) 2016 LiveCode Ltd.
 
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

#include "foundation.h"
#include "foundation-auto.h"
#include "libscript/script.h"
#include "libscript/script-auto.h"


// Forward declarations
typedef class ExpressionOpaque* ExpressionRef;
typedef class ExpressionListOpaque* ExpressionListRef;
typedef class InvokeListOpaque* InvokeListRef;
typedef class InvokeInfo* InvokeInfoRef;
typedef class InvokeMethodListOpaque* InvokeMethodListRef;
typedef class InvokeSignatureOpaque* InvokeSignatureRef;
typedef class ParameterOpaque* ParameterRef;
typedef class ParameterListOpaque* ParameterListRef;
typedef class SignatureOpaque* SignatureRef;
typedef class TypeOpaque* TypeRef;


// Functions imported from Gentle
extern "C" void ExpressionListGetHead(ExpressionListRef, ExpressionRef*);
extern "C"  int ExpressionListGetTail(ExpressionListRef, ExpressionListRef*);
extern "C" void InvokeListGetInfo(InvokeListRef, InvokeInfoRef*);
extern "C"  int InvokeListGetRest(InvokeListRef, InvokeListRef*);
extern "C" void InvokeInfoGetInvokeMethodList(InvokeInfoRef, InvokeMethodListRef*);
extern "C" void InvokeInfoGetModuleName(InvokeInfoRef, const char**);
extern "C"  int InvokeMethodListGetNext(InvokeMethodListRef, InvokeMethodListRef*);
extern "C" void InvokeMethodListGetSignature(InvokeMethodListRef, InvokeSignatureRef*);
extern "C" void InvokeMethodListGetName(InvokeMethodListRef, const char**);
extern "C"  int InvokeMethodListIsIterator(InvokeMethodListRef);
extern "C" void InvokeSignatureGetIndex(InvokeSignatureRef, long*);
extern "C"  int InvokeSignatureGetNext(InvokeSignatureRef, InvokeSignatureRef*);
extern "C"  int IsInvokeModuleAllowed(InvokeInfoRef);
extern "C" void NameForType(TypeRef, const char**);
extern "C" void ParameterGetNameId(ParameterRef, const char**);
extern "C" void ParameterGetType(ParameterRef, TypeRef*);
extern "C" void ParameterListGetHead(ParameterListRef, ParameterRef*);
extern "C"  int ParameterListGetTail(ParameterListRef, ParameterListRef*);
extern "C"  int TypesAreBridgeable(TypeRef, TypeRef);
extern "C"  int TypesAreCompatible(TypeRef, TypeRef);
extern "C" void TypeOfExpression(ExpressionRef, TypeRef*);
extern "C"  int SignatureForInvokeMethod(const char*, const char*, SignatureRef*);
extern "C" void SignatureGetParameterList(SignatureRef, ParameterListRef*);

// Functions implemented in other C/C++ modules
extern "C" void _ErrorV(long, const char*, ...);

// Functions exported to Gentle
extern "C"  int AreAnyInvokesCandidates(InvokeListRef, ExpressionListRef);
extern "C" void Error_NoInvokeCandidates(long, MCStringRef);
extern "C" void ExplainInvokeRejections(InvokeListRef, ExpressionListRef, MCStringRef*);
extern "C"  int IsInvokeCandidate(InvokeInfoRef, ExpressionListRef);
extern "C"  int IsInvokeMethodCandidate(const char*, const char*, InvokeSignatureRef, ExpressionListRef);



// Core of the invoke method checking
static bool IsInvokeMethodCandidate(const char* p_module_name, const char* p_method_name, bool p_is_iterator, InvokeSignatureRef p_invoke_sig, ExpressionListRef p_expr_list, MCStringRef* r_rejection_reason, int* r_cost)
{
    // Initialise the rejection reason to empty, if it has been requested
    if (r_rejection_reason != nil)
        *r_rejection_reason = nil;
    
    // Get the signature for this invoke method
    SignatureRef t_method_sig;
    if (!SignatureForInvokeMethod(p_module_name, p_method_name, &t_method_sig))
    {
        // Failed to get the signature. This isn't a fatal error (yet) because
        // this seems to fail fairly often... I think the problem is that the
        // "standard" modules are not appearing in the list.
        fprintf(stderr, "Internal compiler warning: could not find signature for method %s in module %s\n", p_method_name, p_module_name);
        return true;
    }
    
    // Extract the parameter list from the signature
    ParameterListRef t_param_list;
    SignatureGetParameterList(t_method_sig, &t_param_list);
    
    // Count the number of incoming arguments
    int t_arg_count = -1;
    ExpressionListRef t_expr_iter = p_expr_list;
    do
    {
        t_arg_count++;
    }
    while (ExpressionListGetTail(t_expr_iter, &t_expr_iter));
    
    // Count the number of parameters in this signature. Note that we count the
    // number in the invoke signature, not the method signature, as some of the
    // parameters may be implicit (e.g. for iterator syntax).
    int t_param_count = -1;
    InvokeSignatureRef t_invokesig_iter = p_invoke_sig;
    do
    {
        t_param_count++;
    }
    while (InvokeSignatureGetNext(t_invokesig_iter, &t_invokesig_iter));
    
    // If the number of arguments and parameters do not match, reject this method
    if (t_arg_count != t_param_count)
    {
        // Create the rejection reason, if required
        if (r_rejection_reason != nil)
            MCStringFormat(*r_rejection_reason, "expected %d arguments, have %d", t_param_count, t_arg_count);
        
        return false;
    }
    
    // Loop over the arguments and parameters and check that the types match.
    // Calculate the cost of the method (in terms of number of bridges needed)
    // as we go.
    int t_cost = 0;
    t_invokesig_iter = p_invoke_sig;
    t_expr_iter = p_expr_list;
    bool t_reject = false;
    for (int i = 0; i < t_param_count; i++)
    {
        // Get the index of this parameter
        long t_index, t_index_counter;
        ParameterListRef t_param_iter = t_param_list;
        InvokeSignatureGetIndex(t_invokesig_iter, &t_index);
        
        // Invoke iterators have an implicit first parameter
        if (p_is_iterator)
            t_index++;
        
        // The parameters of the invoke signature are not necessarily in order
        // so we need to find the parameter given by the index in the signature.
        t_index_counter = t_index;
        while (t_index_counter-- > 0)
        {
            if (!ParameterListGetTail(t_param_iter, &t_param_iter))
            {
                // This shouldn't happen!
                fprintf(stderr, "Internal compiler error: [%s:%d in %s] parameter in invoke signature with invalid index\n",
                        __FILE__, __LINE__, __func__);
                exit(1);
            }
        }
        
        // Parameter has been found
        ParameterRef t_param = nil;
        ParameterListGetHead(t_param_iter, &t_param);
        
        // Get the types for both the expression and parameter
        TypeRef t_param_type;
        TypeRef t_expr_type;
        ExpressionRef t_expr;
        ExpressionListGetHead(t_expr_iter, &t_expr);
        TypeOfExpression(t_expr, &t_expr_type);
        ParameterGetType(t_param, &t_param_type);
        
        // Do the types match without bridging?
        if (TypesAreCompatible(t_expr_type, t_param_type))
        {
            // Okay, no cost for compatible types
        }
        // Or can the types be bridged?
        else if (TypesAreBridgeable(t_expr_type, t_param_type))
        {
            // Increase the cost due to needing bridging
            t_cost++;
        }
        else
        {
            // Types are not compatible
            if (r_rejection_reason != nil)
            {
                MCAutoStringRef t_reason;
                const char* t_expr_type_name;
                const char* t_param_type_name;
                NameForType(t_expr_type, &t_expr_type_name);
                NameForType(t_param_type, &t_param_type_name);
                MCStringFormat(&t_reason, "parameter %d: types %s and %s are incompatible", t_index, t_expr_type_name, t_param_type_name);
                
                // Append the reason to the end of the message.
                if (*r_rejection_reason == nil)
                    MCStringMutableCopy(*t_reason, *r_rejection_reason);
                else
                    MCStringAppendFormat(*r_rejection_reason, ", %@", *t_reason);
                
                // This method needs to be rejected
                t_reject = true;
            }
            else
            {
                // We don't stop processing this method immediately if a
                // rejection reason is required in order that the rejection
                // message be as comprehensive as possible
                return false;
            }
        }
        
        // Move on to the next parameter
        InvokeSignatureGetNext(t_invokesig_iter, &t_invokesig_iter);
        ExpressionListGetTail(t_expr_iter, &t_expr_iter);
    }
    
    // Matching complete. If a rejection reason was given, make it non-mutable
    if (r_rejection_reason != nil && *r_rejection_reason != nil && MCStringIsMutable(*r_rejection_reason))
        MCStringCopyAndRelease(*r_rejection_reason, *r_rejection_reason);
    
    // Return the cost, if requested
    if (r_cost != nil)
        *r_cost = t_cost;
    
    // Return the match status
    return !t_reject;
}

// Core of the invoke candidate checking
static bool IsInvokeCandidate(InvokeInfoRef p_invoke_info, ExpressionListRef p_expr_list, InvokeInfoRef* r_invoke_info, MCStringRef* r_rejection_reason)
{
    // Initialise the rejection reason to empty, if it has been requested
    if (r_rejection_reason != nil)
        *r_rejection_reason = nil;
    
    // Get the name of the module
    const char* t_module_name;
    InvokeInfoGetModuleName(p_invoke_info, &t_module_name);
    
    // Is this module available for invokes? This isn't immediately an error if
    // collecting rejection reasons as we'd like to provide more comprehensive
    // error messages.
    bool t_module_allowed = IsInvokeModuleAllowed(p_invoke_info);
    if (!t_module_allowed && r_rejection_reason == nil)
            return false;
    
    // Number of methods within this invoke that are candidates
    int t_candidate_method_count = 0;
    
    // List of methods that were accepted
    MCAutoArray<InvokeMethodListRef> t_candidates;
    
    // Get the method list for this invoke info
    InvokeMethodListRef t_method_list;
    InvokeInfoGetInvokeMethodList(p_invoke_info, &t_method_list);
    
    // Iterate through the methods
    InvokeMethodListRef t_methodlist_iter;
    t_methodlist_iter = t_method_list;
    do
    {
        // Get the name associated with this method
        const char* t_method_name;
        InvokeMethodListGetName(t_methodlist_iter, &t_method_name);
        
        // Get the invoke signature
        InvokeSignatureRef t_invoke_sig;
        InvokeMethodListGetSignature(t_methodlist_iter, &t_invoke_sig);
        
        // Is this method group an iterator?
        bool t_is_iterator = InvokeMethodListIsIterator(t_methodlist_iter);
        
        // If collecting rejection reasons, make some space for one
        MCAutoStringRef t_rejection_reason;
        MCStringRef* t_reject_ptr = (r_rejection_reason != nil) ? &(&t_rejection_reason) : nil;
        
        // We don't care about cost at this stage
        int* t_cost = nil;
        
        // We check the candidate signature if we've not rejected the module OR
        // if we've rejected the module but want rejection reasons.
        
        // Is this method a candidate?
        bool t_is_candidate = IsInvokeMethodCandidate(t_module_name, t_method_name, t_is_iterator, t_invoke_sig, p_expr_list, t_reject_ptr, t_cost);
        
        if (t_module_allowed && t_is_candidate)
        {
            // Found a candidate method
            t_candidate_method_count++;
            t_candidates.Push(t_methodlist_iter);
        }
        else
        {
            // Not a candidate. If collecting reasons, append to the list
            if (r_rejection_reason != nil)
            {
                MCAutoStringRef t_reason;
                if (t_is_candidate)
                    MCStringFormat(&t_reason, "rejected candidate method %s.%s: module %s has not been imported", t_module_name, t_method_name, t_module_name);
                else
                    MCStringFormat(&t_reason, "rejected candidate method %s.%s: %@", t_module_name, t_method_name, *t_rejection_reason);
                
                if (*r_rejection_reason == nil)
                    MCStringMutableCopy(*t_reason, *r_rejection_reason);
                else
                    MCStringAppendFormat(*r_rejection_reason, "\n%@", *t_reason);
            }
        }
    }
    while (InvokeMethodListGetNext(t_methodlist_iter, &t_methodlist_iter));
    
    // If the rejection reason is still mutable, make it immutable
    if (r_rejection_reason != nil && *r_rejection_reason != nil && MCStringIsMutable(*r_rejection_reason))
        MCStringCopyAndRelease(*r_rejection_reason, *r_rejection_reason);
    
    // Return whether any matches were found
    return t_candidate_method_count > 0;
}

static bool AreAnyInvokesCandidates(InvokeListRef p_invoke_list, ExpressionListRef p_expr_list, MCStringRef* r_rejection_reason)
{
    // Initialise the rejection reason to empty, if it has been requested
    if (r_rejection_reason != nil)
        *r_rejection_reason = nil;
    
    // Whether any candidates have been found
    bool t_have_candidates = false;
    
    // Iterate through the available invocations
    InvokeListRef t_invokelist_iter = p_invoke_list;
    do
    {
        // Get the info for this invoke
        InvokeInfoRef t_invoke;
        InvokeListGetInfo(t_invokelist_iter, &t_invoke);
        
        // If collecting rejection reasons, make some space for one
        MCAutoStringRef t_rejection_reason;
        MCStringRef* t_reject_ptr = (r_rejection_reason != nil) ? &(&t_rejection_reason) : nil;
        
        // Are there any candidate methods within this invoke?
        bool t_candidate = IsInvokeCandidate(t_invoke, p_expr_list, nil, t_reject_ptr);
        if (t_candidate)
        {
            // (At least) one candidate has been found
            t_have_candidates = true;
        }
        else
        {
            // Not a candidate. If collecting reasons, append to the list
            if (r_rejection_reason != nil)
            {
                if (*r_rejection_reason == nil)
                    MCStringMutableCopy(*t_rejection_reason, *r_rejection_reason);
                else
                    MCStringAppendFormat(*r_rejection_reason, "\n%@", *t_rejection_reason);
            }
        }
    }
    while (InvokeListGetRest(t_invokelist_iter, &t_invokelist_iter));
    
    // If the rejection reason is still mutable, make it immutable
    if (r_rejection_reason != nil && *r_rejection_reason != nil && MCStringIsMutable(*r_rejection_reason))
        MCStringCopyAndRelease(*r_rejection_reason, *r_rejection_reason);
    
    // Return whether any candidates were found
    return t_have_candidates;
}


int IsInvokeCandidate(InvokeInfoRef p_invoke, ExpressionListRef p_expr_list)
{
    return IsInvokeCandidate(p_invoke, p_expr_list, nil, nil);
}

int AreAnyInvokesCandidates(InvokeListRef p_invoke_list, ExpressionListRef p_expr_list)
{
    return AreAnyInvokesCandidates(p_invoke_list, p_expr_list, nil);
}

void ExplainInvokeRejections(InvokeListRef p_invoke_list, ExpressionListRef p_expr_list, MCStringRef* r_reasons)
{
    AreAnyInvokesCandidates(p_invoke_list, p_expr_list, r_reasons);
}

void Error_NoInvokeCandidates(long p_position, MCStringRef p_reasons)
{
    _ErrorV(p_position, "could not resolve invoke:\n%s", MCStringGetCString(p_reasons));
}
