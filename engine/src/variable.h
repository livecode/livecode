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

#define VAR_PAD 16
#define VAR_MASK 0xFFFFFFF0
#define VAR_APPEND_MAX (MAXUINT2 * 4)

///////////////////////////////////////////////////////////////////////////////
//
// The MCVariableArray class represents LiveCode's 'hash' value.
// It is intended to be used by another class that controls construction and
// destruction (notice it has no constructor/destructor). The reason behind
// this is that we want to be able to put it in 'union' constructs and have
// another class control its discrimination.

#define TABLE_SIZE 8
#define EXTENT_NONNUM 0
#define EXTENT_RECALC 1
#define EXTENT_ALLOCEVAL 2
#define ROW_DIM  0
#define COL_DIM 1

// MW-2014-08-06: [[ Bug 13113 ]] Change extents to signed ints so negative indicies work.
typedef struct
{
	int32_t minimum;
	int32_t maximum;
}
arrayextent;

struct MCHashentry;

class MCVariableArray
{
	MCHashentry **table;
	uint32_t tablesize;
	uint32_t nfilled;
	uint32_t keysize;
	uint8_t dimensions;
	arrayextent *extents;

public:
    // Initialize the array to empty
    void clear(void);
    
	// Initialize the hash to the given size
	bool presethash(uint4 p_size);

	// Increase the size of the hash to the next power of two
	bool resizehash(uint32_t p_new_size = 0);

	// Free the array's memory
	void freehash(void);

	//

	// Return the number of entrys in the array
	uint32_t getnfilled(void) const;

	//
	// Copy any keys from v not present in this
	// PRECONDITION: this is initialized
    // MERG-2013-08-26: [[ RecursiveArrayOp ]] Support nested arrays in union and intersect
	Exec_stat unionarray(MCVariableArray& v, bool p_recursive);

	// Remove any keys in this not present in v
	// PRECONDITION: this is initialized
	// MERG-2013-08-26: [[ RecursiveArrayOp ]] Support nested arrays in union and intersect
    Exec_stat intersectarray(MCVariableArray& v, bool p_recursive);

	// Set the value of the variable to the transpose of the value contained in v.
	// PRECONDITION: this is uninitialized
	Exec_stat transpose(MCVariableArray& v);

	// Return True if the variable is a dense numerically indexed array of some dimension.
	Boolean isnumeric(void);

	// Return True if the variable is a sequence...
	///  ... which is a dense numerically indexed array of some dimension.
	Boolean issequence(void);

	// Lookup the given key in the array potentially adding it if it is not present
	// PRECONDITION: this is initialized
	MCHashentry *lookuphash(const MCString &, Boolean cs, Boolean add);
	
	// Lookup the given index in the sequence.
	// PRECONDITION: this is initialized and issequence() == True
	MCHashentry *lookupindex(uint32_t p_index, Boolean add);

	// Lookup the given key and remove it if it is present
	// PRECONDITION: this is initialized
	void removehash(const MCString&, Boolean cs);
	
	// Remove the given hashentry directly.
	// PRECONDITION: this is initialized and p_hash is a direct child
	void removehash(MCHashentry *p_entry);
	
	// Steal the given variable's hashtable
	// PRECONDITION: this is uninitialized
	void taketable(MCVariableArray *v);

	// Copy the given variable's hashtable
	// PRECONDITION: this is uninitialized
	bool copytable(const MCVariableArray &v);

	// Return a list of keys of the array (used by xcommands)
	// PRECONDITION: this is initialized
	void getkeys(char **keylist, uint4 kcount);

	// Iterate over the keys in the array
	// PRECONDITION: this is initialized
	MCHashentry *getnextkey(uint4& l, MCHashentry *e) const;

	// Alternative form of iteration - uses the entry's hash element to compute
	// the next entry.
	// PRECONDITION: this is initialized
	MCHashentry *getnextkey(MCHashentry *e) const;

	// Set the array to the result of splitting the given value.
	// PRECONDITION: this is uninitialized
	void split(const MCString& s, char e, char k);

	// Set the array to the result of splitting the given value column-wise
	// PRECONDITION: this is uninitialized
	void split_column(const MCString& s, char r, char c);

	// Set the array to the result of splitting the given value as a set.
	// PRECONDITION: this is uninitialized
	void split_as_set(const MCString& s, char e);
	
	// Load the keys of the array from the given stream
	// PRECONDITION: this is uninitialized
	IO_stat loadkeys(IO_handle stream, bool p_merge);

	// Save the keys of the array to the given stream
	// PRECONDITION: this is initialized
	IO_stat savekeys(IO_handle stream);

	// Set the properties of the given object to those listed in this array
	// PRECONDITION: this is initialized
	Exec_stat setprops(uint4 parid, MCObject *optr);

	// Returns true if the array has arrays as sub-keys
	// PRECONDITION: this is initialized
	bool isnested(void);

	// Fill the given buffer with an unordered list of the hashentries
	void listelements(MCHashentry** elements);
	
	// Returns the serialized length of the array (in total)
	uint4 measure(bool p_only_nested);

	IO_stat save(MCObjectOutputStream& p_stream, bool p_only_nested);
	IO_stat load(MCObjectInputStream& p_stream, bool p_merge);

private:
	// Compute the hash value of the given string
	uint4 computehash(const MCString &);

	// Update the extents on the array appropriately using the given key
	void extentfromkey(char *skey);

	// Recalculate the extents
	void calcextents(void);

	// Get the extent of the given dimension
    // MW-2014-05-28: [[ Bug 12479 ]] Make sure extents use 32-bit ints.
	uint32_t getextent(uint1 tdimension) const;

	// Return True if the extents and number of keys do not match up
	Boolean ismissingelement(void) const;
};

inline uint32_t MCVariableArray::getnfilled(void) const
{
	return nfilled;
}

///////////////////////////////////////////////////////////////////////////////
//
// The MCVariableValue class represents a value that can be stored in a 
// LiveCode variable, or container (such as a hash).
//
// A value has one of 5 states:
//   - UNDEFINED: no value has been set, this is evaluated as *both* 0.0 and
//       the empty string
//   - STRING: only a string value has been set
//   - NUMBER: only a numeric value has been set
//   - BOTH: the string value has been converted to a number or vice-versa
//   - ARRAY: an array value is set
//
// Note that arrays are always non-empty, if an array has all its keys deleted
// it reverts to the empty string.
//
// If the value is both or a string, then strnum . svalue . string is always
// non-NULL (even it is the constant "" empty string).
//
// The value class uses a buffer when in string or both mode. This buffer is
// used for fast appending and modification of the string.
//
// The following implicit conversions take place:
//   - undefined -> 0.0
//   - undefined -> ""
//   - "" -> 0.0 (when used in numeric context - not for comparison)
//   - array -> empty
//   - array -> 0.0
//   - empty -> empty array
//

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
	MCNameRef name;
	MCExecValue value;
	MCVariable *next;

	bool is_msg : 1;
	bool is_env : 1;
	bool is_global : 1;

	// MW-2011-08-28: [[ SERVER ]] Some variables must only be computed when they
	//   are first requested - in particular $_POST and $_POST_RAW. To support this
	//   a variable can be marked as 'deferred' which causes a special varref to
	//   be constructed when a reference to the variable is parsed. This bit is
	//   unset as soon as the value has been computed once. If this bit is set then
	//   it means the variable is actually an instance of MCDeferredVariable.
	bool is_deferred : 1;

	// If set, this means that the variable has been parsed as an 'unquoted-
	// literal'. Such variables get cleared when referenced as an l-value.
	bool is_uql : 1;

	// The correct way to create variables is with the static 'create' methods
	// which can catch a failure.
	MCVariable(void) {}
	MCVariable(const MCVariable& other) {}
    
    // Returns true if the existing value of the variable is can become or remain
    // data when the operation is complete, without loss of information.
    bool can_become_data(MCExecContext& ctxt, MCNameRef *p_path, uindex_t p_length);
    
    // Modify the content of the variable - append or prepend (nested key).
    bool modify(MCExecContext& ctxt, MCValueRef p_value, MCNameRef *p_path, uindex_t p_length, MCVariableSettingStyle p_setting);
    // Modify the variable by appending/prepending the value given (nested key).
    bool modify_ctxt(MCExecContext& ctxt, MCExecValue p_value, MCNameRef *p_path, uindex_t p_length, MCVariableSettingStyle p_setting);
    
    bool modify(MCExecContext& ctxt, MCValueRef p_value, MCVariableSettingStyle p_setting);
    bool modify_ctxt(MCExecContext& ctxt, MCExecValue p_value, MCVariableSettingStyle p_setting);
    
    bool modify_string(MCExecContext& ctxt, MCStringRef p_value, MCNameRef *p_path, uindex_t p_length, MCVariableSettingStyle p_setting);
    // Modify the content of the variable - append or prepend (nested key). Target must already be data.
    bool modify_data(MCExecContext& ctxt, MCDataRef p_data, MCNameRef *p_path, uindex_t p_length, MCVariableSettingStyle p_setting);
public:
	
	// Destructor
	~MCVariable(void);

	/////////
    
    // SN-2014-04-11 [[ FasterVariables ]] Now able to handle prepending a string to a variable.
    // A style of setting allows us to set the variable or to modify it by appending/prepending
    // '::append' has been renamed '::modify' to take in consideration this new ability

	// Set the content of the variable (nested key) to the given value.
	bool setvalueref(MCNameRef *path, uindex_t length, bool case_sensitive, MCValueRef value);
	// Return the content of the variable (nested key). This does not copy the value.
	MCValueRef getvalueref(MCNameRef *path, uindex_t length, bool case_sensitive);
	// Make an immutable copy of the content of the variable (nested key).
	bool copyasvalueref(MCNameRef *path, uindex_t length, bool case_sensitive, MCValueRef& r_value);

    // Evaluate the contents of the variable (nested key) into the ep.
	// Evalue the contents of the variable (nested key) into r_value.
    bool eval(MCExecContext& ctxt, MCNameRef *p_path, uindex_t p_length, MCValueRef &r_value);
    // Copy the contents of the valueref into the variable (nested key).
    bool set(MCExecContext& ctxt, MCValueRef p_value, MCNameRef *p_path, uindex_t p_length, MCVariableSettingStyle p_setting = kMCVariableSetInto);
    // Remove the content (nested key) of the variable.
    bool remove(MCExecContext& ctxt, MCNameRef *p_path, uindex_t p_length);
    
    // Evaluate the contents of the variable (nested key) into the ep.
	// Evalue the contents of the variable (nested key) into r_value.
    bool eval_ctxt(MCExecContext& ctxt, MCNameRef *p_path, uindex_t p_length, MCExecValue &r_value);
    // Give the exec value to the variable (nested key).
    bool give_value(MCExecContext& ctxt, MCExecValue p_value, MCNameRef *p_path, uindex_t p_length, MCVariableSettingStyle p_setting = kMCVariableSetInto);
	
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
    bool replace(MCExecContext& ctxt, MCValueRef p_replacement, MCRange p_range, MCNameRef *p_path, uindex_t p_length);
    
    bool replace_string(MCExecContext& ctxt, MCStringRef p_replacement, MCRange p_range, MCNameRef *p_path, uindex_t p_length);
    bool replace_data(MCExecContext& ctxt, MCDataRef p_replacement, MCRange p_range, MCNameRef *p_path, uindex_t p_length);
    
	bool deleterange(MCExecContext& ctxt, MCRange p_range);
    bool deleterange(MCExecContext& ctxt, MCRange p_range, MCNameRef *p_path, uindex_t p_length);
    
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
		return name;
	}

	bool hasname(MCNameRef other_name)
	{
		return MCNameIsEqualTo(name, other_name, kMCCompareCaseless);
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
	~MCContainer(void);

	//

    
    bool eval(MCExecContext& ctxt, MCValueRef& r_value);
	bool remove(MCExecContext& ctxt);
    bool set(MCExecContext& ctxt, MCValueRef p_value, MCVariableSettingStyle p_setting = kMCVariableSetInto);
    
    bool eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
    bool give_value(MCExecContext& ctxt, MCExecValue p_value, MCVariableSettingStyle p_setting = kMCVariableSetInto);
    
	bool replace(MCExecContext& ctxt, MCValueRef p_replacement, MCRange p_range);
	bool deleterange(MCExecContext& ctxt, MCRange p_range);

	bool clear(void);
	bool set_real(double real);

	bool set_valueref(MCValueRef value);
    MCValueRef get_valueref(void);
    
    void getpath(MCNameRef*& r_path, uindex_t& r_length)
    {
        r_path = m_path;
        r_length = m_length;
    }

	static bool createwithvariable(MCVariable *var, MCContainer*& r_container);
	static bool createwithpath(MCVariable *var, MCNameRef *path, uindex_t length, MCContainer*& r_container);
    static bool copywithpath(MCContainer *p_container, MCNameRef *p_path, uindex_t p_length, MCContainer*& r_container);
    
    MCVariable *getvar()
    {
        return m_variable;
    }
    
private:
	MCVariable *m_variable;
	MCNameRef *m_path;
	uindex_t m_length;
	bool m_case_sensitive;
};

///////////////////////////////////////////////////////////////////////////////

class MCVarref : public MCExpression
{
protected:
	MCVariable *ref;
	MCHandler *handler;
	union
	{
		MCExpression *exp;
		MCExpression **exps;
	};
	unsigned index : 16;
	unsigned dimensions : 8;
	Boolean isparam : 1;

	// MW-2008-10-28: [[ ParentScripts ]] This boolean flag is True if this
	//   varref refers to a script local.
	Boolean isscriptlocal : 1;
	
	// MW-2012-03-15: [[ Bug ]] This boolean flag is true if this varref is
	//   a plain var and doesn't require synching.
	bool isplain : 1;

public:
	MCVarref(MCVariable *var)
	{
		ref = var;
		exp = NULL;
		dimensions = 0;
		index = 0;
		isparam = False;
		isscriptlocal = False;
		handler = NULL;
		isplain = var -> isplain();
	}

	// MW-2008-10-28: [[ ParentScripts ]] A new constructor to handle the case
	//   of a script local.
	MCVarref(MCVariable *var, uint2 i)
	{
		ref = var;
		exp = NULL;
		dimensions = 0;
		index = i;
		isparam = False;
		isscriptlocal = True;
		handler = NULL;
		isplain = true;
	}

	MCVarref(MCHandler *hptr, uint2 i, Boolean param)
	{
		index = i;
		isparam = param;
		handler = hptr;
		ref = NULL;
		exp = NULL;
		dimensions = 0;
		isscriptlocal = False;
		isplain = true;
	}
    virtual ~MCVarref();
    
    bool needsContainer(void) const {return dimensions != 0;}
    
    void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
    bool evalcontainer(MCExecContext &ctxt, MCContainer*& r_container);
    virtual MCVariable *evalvar(MCExecContext& ctxt);
	
	virtual void compile(MCSyntaxFactoryRef);
	virtual void compile_in(MCSyntaxFactoryRef);
	virtual void compile_out(MCSyntaxFactoryRef);
	virtual void compile_inout(MCSyntaxFactoryRef);

	//virtual MCVariable *getvar();
	virtual MCVarref *getrootvarref(void);

	// This method returns true if p_other refers to the same root
	// variable as self.
	bool rootmatches(MCVarref *p_other) const;

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
    
    bool resolve(MCExecContext& ctxt, MCContainer*& r_container);
    
    void getpath(MCExecContext& ctxt, MCNameRef*& r_path, uindex_t& r_length);
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
    virtual bool evalcontainer(MCExecContext& ctxt, MCContainer*& r_container);
    virtual MCVariable *evalvar(MCExecContext& ctxt);
};

///////////////////////////////////////////////////////////////////////////////

#endif
