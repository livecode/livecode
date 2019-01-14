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

#ifndef	__MC_VARIABLE__
#define	__MC_VARIABLE__

#ifndef __MC_EXPRESSION__
#include "express.h"
#endif

#define kMCEncodedValueTypeUndefined 1
#define kMCEncodedValueTypeEmpty 2
#define kMCEncodedValueTypeString 3
#define kMCEncodedValueTypeNumber 4
#define kMCEncodedValueTypeLegacyArray 5
#define kMCEncodedValueTypeArray 6

typedef enum
{
    kMCVariableSetInto,
    kMCVariableSetAfter,
    kMCVariableSetBefore
} MCVariableSettingStyle;

class MCVariable
{
protected:
	MCNewAutoNameRef name;
	MCExecValue value;
	MCVariable *next = nullptr;

	bool is_msg = false;
	bool is_env = false;
	bool is_global = false;

	// MW-2011-08-28: [[ SERVER ]] Some variables must only be computed when they
	//   are first requested - in particular $_POST and $_POST_RAW. To support this
	//   a variable can be marked as 'deferred' which causes a special varref to
	//   be constructed when a reference to the variable is parsed. This bit is
	//   unset as soon as the value has been computed once. If this bit is set then
	//   it means the variable is actually an instance of MCDeferredVariable.
	bool is_deferred = false;

	// If set, this means that the variable has been parsed as an 'unquoted-
	// literal'. Such variables get cleared when referenced as an l-value.
	bool is_uql = false;

	// The correct way to create variables is with the static 'create' methods
	// which can catch a failure.
    MCVariable() = default;
    
    // Returns true if the existing value of the variable is can become or remain
    // data when the operation is complete, without loss of information.
    bool can_become_data(MCExecContext& ctxt, MCSpan<MCNameRef> p_path);
    
    // Modify the content of the variable - append or prepend (nested key).
    bool modify(MCExecContext& ctxt, MCValueRef p_value, MCSpan<MCNameRef> p_path, MCVariableSettingStyle p_setting);
    // Modify the variable by appending/prepending the value given (nested key).
    bool modify_ctxt(MCExecContext& ctxt, MCExecValue p_value, MCSpan<MCNameRef> p_path, MCVariableSettingStyle p_setting);
    
    bool modify(MCExecContext& ctxt, MCValueRef p_value, MCVariableSettingStyle p_setting);
    bool modify_ctxt(MCExecContext& ctxt, MCExecValue p_value, MCVariableSettingStyle p_setting);
    
    bool modify_string(MCExecContext& ctxt, MCStringRef p_value, MCSpan<MCNameRef> p_path, MCVariableSettingStyle p_setting);
    // Modify the content of the variable - append or prepend (nested key). Target must already be data.
    bool modify_data(MCExecContext& ctxt, MCDataRef p_data, MCSpan<MCNameRef> p_path, MCVariableSettingStyle p_setting);
public:
	
	// Destructor
	~MCVariable(void);

	/////////
    
    // SN-2014-04-11 [[ FasterVariables ]] Now able to handle prepending a string to a variable.
    // A style of setting allows us to set the variable or to modify it by appending/prepending
    // '::append' has been renamed '::modify' to take in consideration this new ability

	// Set the content of the variable (nested key) to the given value.
	bool setvalueref(MCSpan<MCNameRef> path, bool case_sensitive, MCValueRef value);
	// Return the content of the variable (nested key). This does not copy the value.
	MCValueRef getvalueref(MCSpan<MCNameRef> path, bool case_sensitive);
	// Make an immutable copy of the content of the variable (nested key).
	bool copyasvalueref(MCSpan<MCNameRef> path, bool case_sensitive, MCValueRef& r_value);

    // Evaluate the contents of the variable (nested key) into the ep.
	// Evalue the contents of the variable (nested key) into r_value.
    bool eval(MCExecContext& ctxt, MCSpan<MCNameRef> p_path, MCValueRef &r_value);
    // Copy the contents of the valueref into the variable (nested key).
    bool set(MCExecContext& ctxt, MCValueRef p_value, MCSpan<MCNameRef> p_path, MCVariableSettingStyle p_setting = kMCVariableSetInto);
    // Remove the content (nested key) of the variable.
    bool remove(MCExecContext& ctxt, MCSpan<MCNameRef> p_path);
    
    // Evaluate the contents of the variable (nested key) into the ep.
	// Evalue the contents of the variable (nested key) into r_value.
    bool eval_ctxt(MCExecContext& ctxt, MCSpan<MCNameRef> p_path, MCExecValue &r_value);
    // Give the exec value to the variable (nested key).
    bool give_value(MCExecContext& ctxt, MCExecValue p_value, MCSpan<MCNameRef> p_path, MCVariableSettingStyle p_setting = kMCVariableSetInto);
	
    bool setvalueref(MCValueRef value);
	MCValueRef getvalueref(void);
	bool copyasvalueref(MCValueRef& r_value);
    bool copyasexecvalue(MCExecValue &r_value);
	// Return the content of the variable as an exec value. This does not copy the value.
	MCExecValue getexecvalue(void);

    bool eval(MCExecContext& ctxt, MCValueRef& r_value);
    bool set(MCExecContext& ctxt, MCValueRef p_value, MCVariableSettingStyle p_setting = kMCVariableSetInto);
    
    // SN-2014-04-11 [[ FasterVariable ]]
    // Replace the content of the internal string according to the range given to avoid unnecessary copy
	bool replace(MCExecContext& ctxt, MCValueRef p_replacement, MCRange p_range);
    bool replace(MCExecContext& ctxt, MCValueRef p_replacement, MCRange p_range, MCSpan<MCNameRef> p_path);
    
    bool replace_string(MCExecContext& ctxt, MCStringRef p_replacement, MCRange p_range, MCSpan<MCNameRef> p_path);
    bool replace_data(MCExecContext& ctxt, MCDataRef p_replacement, MCRange p_range, MCSpan<MCNameRef> p_path);
    
	bool deleterange(MCExecContext& ctxt, MCRange p_range);
    bool deleterange(MCExecContext& ctxt, MCRange p_range, MCSpan<MCNameRef> p_path);
    
    bool eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
    bool give_value(MCExecContext& ctxt, MCExecValue p_value, MCVariableSettingStyle p_setting = kMCVariableSetInto);
    
    bool remove(MCExecContext& ctxt);
    
	// Converts the value in the variable to an array of strings.
    bool converttoarrayofstrings(MCExecContext& ctxt);
	// Converts the value to a (mutable) string.
    bool converttomutablestring(MCExecContext& ctxt);
    bool converttomutabledata(MCExecContext& ctxt);

    // Converts the value to a (mutable) array.
	bool converttomutablearray(void);

	/////////

	// Return whether the variable has been assigned a value. i.e. whether the
	// value is 'UNDEFINED'.
	Boolean isclear(void) const;
	
	// Return whether the variable has been 'deleted'. At the moment we use the
	// 'UNDEFINED' state to determine this, but this is not strictly correct.
	// TODO: Make 'freed' a state of the variable.
	Boolean isfree(void) const;
	
	// Returns True if the variable currently holds an array.
	Boolean isarray(void) const;

	// Returns True if the variable is currently empty. A variable is considered
	// empty if it has never been assigned a value, or is the empty string.
	Boolean isempty(void) const;

	// Returns true if the variable is (currently) a UQL.
	bool isuql(void) const;

	//

	// Set the current value to undefined. If 'dodel' is True then any buffer
	// should be freed also.
	// N.B. p_delete_buffer is currently used as a hint that the variable may
	//      go from being undefined to a copied string at some point in the
	//      near future.
	//
	void clear(Boolean p_delete_buffer = False);

	// Clear the variable's value but only if it is an unquoted literal (i.e. its
	// name is the same pointer as its string value).
	void clearuql(void);
	void doclearuql(void);

	//

	// Set the value of the variable to the string 's' - this does *not* copy
	// the value, so ownership remains with the caller.
	// If the string is empty, then the variable is set to the UNDEFINED state.
	//
	// NB: This should only be used for *constant* strings.
	//
	Exec_stat sets(const MCString &s);

	// Copy the given string into the variable
	void copysvalue(const MCString &s);

	// Set the value of the variable to the given real number.
	void setnvalue(real8 n);

	// Copy the given string into the variable by taking ownership of the buffer.
	// This is used in places where a potentially large string has been manipulated
	// transiently.
	void grab(char *p_buffer, uint4 p_length);

	// Apply any changes to the value to the 'special' part of the var.
	// i.e. if it is an environment variable or the msg variable
    // (ep is just used for local context vars)
    void synchronize(MCExecContext& ctxt, bool p_notify = false);

	void setnext(MCVariable *n)
	{
		next = n;
	}

	MCVariable *getnext()
	{
		return next;
	}

	//////////

	MCNameRef getname(void)
	{
		return *name;
	}

	bool hasname(MCNameRef other_name)
	{
		return MCNameIsEqualToCaseless(*name, other_name);
	}

	//////////

	// Mark the variable as being the message box
	void setmsg(void) { is_msg = true; }

	// Mark the variable as being a unquoted literal
	void setuql(void) { is_uql = true; }

	// Returns true if the variable is deferred
	bool isdeferred(void) { return is_deferred; }
	
	// Returns true if the var doesn't need synching.
	bool isplain(void) { return !is_msg && !is_env; }

	// Returns a new MCVarref of the appropriate type for this var
	MCVarref *newvarref(void);

	//////////

	// Look for a global variable with the given name. If no such variable
	// exists, nil is returned.
	static MCVariable *lookupglobal(MCNameRef name);
	static MCVariable *lookupglobal_cstring(const char *name);

	// Ensure that a global variable with the given name exists. If the variable
	// does not exist it is created.
	/* CAN FAIL */ static bool ensureglobal(MCNameRef name, MCVariable*& r_var);

	/* CAN FAIL */ static bool create(MCVariable*& r_var);
	/* CAN FAIL */ static bool createwithname(MCNameRef name, MCVariable*& r_var);

	/* CAN FAIL */ static bool createcopy(MCVariable& other, MCVariable*& r_var);

    ///////////
    // Does what MCVariableValue equivalent was doing
    bool encode(void *&r_buffer, uindex_t& r_size);
    bool decode(void *p_buffer, uindex_t p_size);
};

///////////////////////////////////////////////////////////////////////////////

class MCContainer
{
public:
    enum
    {
        /* A short path length of 6 has been chosen as it means an MCContainer
         * occupies 64 bytes on 64-bit and 48 bytes on 32-bit. */
        kShortPathLength = 6,
        
        /* The extension size to use for a long path, should be a power-of-two. */
        kLongPathSegmentLength = 8,
    };
    
    MCContainer() = default;
    
    MCContainer(MCVariable *var) : m_path_length(0), m_variable(var) {}

    ~MCContainer(void);

	//

    bool remove(MCExecContext& ctxt);
    
    bool eval(MCExecContext& ctxt, MCValueRef& r_value);
    bool eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
    
    bool set(MCExecContext& ctxt, MCValueRef p_value, MCVariableSettingStyle p_setting = kMCVariableSetInto);
    bool give_value(MCExecContext& ctxt, MCExecValue p_value, MCVariableSettingStyle p_setting = kMCVariableSetInto);
    
	bool replace(MCExecContext& ctxt, MCValueRef p_replacement, MCRange p_range);
	bool deleterange(MCExecContext& ctxt, MCRange p_range);

	bool clear(void);
	bool set_real(double real);

	bool set_valueref(MCValueRef value);
    MCValueRef get_valueref(void);

    MCSpan<MCNameRef> getpath();

    MCVariable *getvar()
    {
        return m_variable;
    }
    
private:
    /* If m_path_length <= 6 then m_short_path contains the path, otherwise
     * m_long_path (a dynamically allocated vector) will contain it. */
    uindex_t m_path_length = 0;
    union
    {
        MCNameRef m_short_path[kShortPathLength];
        struct
        {
            uindex_t m_long_path_capacity;
            MCNameRef *m_long_path;
        };
    };
	MCVariable *m_variable = nullptr;
    
    friend class MCVarref;
    friend class MCContainerBuilder;
};

///////////////////////////////////////////////////////////////////////////////

class MCVarref : public MCExpression
{
protected:
	MCVariable *ref = nullptr;
	MCHandler *handler = nullptr;
	union
	{
		MCExpression *exp = nullptr;
		MCExpression **exps;
	};
	uint16_t index = 0;
	uint8_t dimensions = 0;
	bool isparam = false;

	// MW-2008-10-28: [[ ParentScripts ]] This boolean flag is True if this
	//   varref refers to a script local.
	bool isscriptlocal = false;
	
	// MW-2012-03-15: [[ Bug ]] This boolean flag is true if this varref is
	//   a plain var and doesn't require synching.
	bool isplain = false;

public:
    MCVarref(MCVariable *var)
        : ref(var),
          isplain(ref->isplain())
    {}

	// MW-2008-10-28: [[ ParentScripts ]] A new constructor to handle the case
	//   of a script local.
    MCVarref(MCVariable *var, uint2 i)
        : ref(var),
          index(i),
          isscriptlocal(true),
          isplain(true)
    {}

    MCVarref(MCHandler *hptr, uint2 i, Boolean param)
        : handler(hptr),
          index(i),
          isparam(param),
          isplain(true)
    {}

    virtual ~MCVarref();
    
    void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
    virtual bool evalcontainer(MCExecContext &ctxt, MCContainer& r_container);

	virtual MCVarref *getrootvarref(void);

	Boolean getisscriptlocal() { return isscriptlocal; };

    bool set(MCExecContext& ctxt, MCValueRef p_value, MCVariableSettingStyle p_setting = kMCVariableSetInto);
    bool give_value(MCExecContext& ctxt, MCExecValue p_value, MCVariableSettingStyle p_setting = kMCVariableSetInto);
    bool replace(MCExecContext& ctxt, MCValueRef p_replacement, MCRange p_range);
    bool deleterange(MCExecContext& ctxt, MCRange p_range);
	Parse_stat parsearray(MCScriptPoint &);
	Exec_stat sets(const MCString &s);
	void clear();
	void clearuql();
    bool dofree(MCExecContext& ctxt);
    
	bool getisplain(void) const { return isplain; }
	
private:
    MCVariable *fetchvar(MCExecContext& ctxt);
    MCContainer *fetchcontainer(MCExecContext& ctxt);
    
    bool resolve(MCExecContext& ctxt, MCContainer& r_container);

    MCSpan<MCNameRef> getpath(MCExecContext& ctxt);
};

///////////////////////////////////////////////////////////////////////////////

// This callback is invoked by a deferred variable when its value is needed.
typedef bool (*MCDeferredVariableComputeCallback)(void *context, MCVariable *variable);

class MCDeferredVariable: public MCVariable
{
protected:
	MCDeferredVariableComputeCallback m_callback;
	void *m_context;

public:
	static bool createwithname(MCNameRef p_name, MCDeferredVariableComputeCallback callback, void *context, MCVariable*& r_var);

    bool compute(void);
};

// A 'deferred' varref works identically to a normal varref except that it
// ensures that the value of the target variable has been computed before
// evaluation.
class MCDeferredVarref: public MCVarref
{
public:
	MCDeferredVarref(MCVariable *var)
		: MCVarref(var)
	{
	}

	// Override all methods that require the value of the variable. These
	// just ensure 'compute' is called on the MCDeferredVar before the
	// super-class methods with the same name are invoked.
    virtual void eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
    virtual bool evalcontainer(MCExecContext& ctxt, MCContainer& r_container);
};

///////////////////////////////////////////////////////////////////////////////

#endif
