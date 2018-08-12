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

#ifndef __MC_EXEC_CONTEXT__
#define __MC_EXEC_CONTEXT__

////////////////////////////////////////////////////////////////////////////////

class MCExecContext
{
public:
	MCExecContext(void);
	MCExecContext(MCExecContext& other);
	
	///////////
	
	bool HasError(void)
	{
        return (m_stat == ES_ERROR || m_stat == ES_NOT_FOUND || m_stat == ES_NOT_HANDLED);
	}
	
	void IgnoreLastError()
	{
		m_stat = ES_NORMAL;
	}
	
    void SetIsReturnHandler()
    {
        m_stat = ES_RETURN_HANDLER;
    }
	
	void Throw(void)
	{
		m_stat = ES_ERROR;
	}
	
	void SetLine(uint2 line);
	uint2 GetLine(void);
	
	Exec_stat Catch(uint2 line, uint2 pos);
	
	void LegacyThrow(Exec_errors error, MCValueRef hint = nil);
	void LegacyThrow(Exec_errors error, uint32_t hint);
	
	void Unimplemented(void)
	{
		abort();
	}
	
	//////////
	
	bool GetCaseSensitive(void) const
	{
		return m_case_sensitive;
	}
	
	bool GetConvertOctals(void) const
	{
		return m_convert_octals;
	}
	
	bool GetWholeMatches(void) const
	{
		return m_whole_matches;
	}
	
	bool GetUseUnicode(void) const
	{
		return m_use_unicode;
	}
	
	bool GetUseSystemDate(void) const
	{
		return m_use_system_date;
	}
	
	char_t GetLineDelimiter(void) const
	{
		return m_line_delimiter;
	}
	
	char_t GetItemDelimiter(void) const
	{
		return m_item_delimiter;
	}
	
	char_t GetColumnDelimiter(void) const
	{
		return m_column_delimiter;
	}
	
	char_t GetRowDelimiter(void) const
	{
		return m_row_delimiter;
	}
	
	uint2 GetCutOff(void) const
	{
		return m_cutoff;
	}
	
	uinteger_t GetNumberFormatWidth() const
	{
		return m_nffw;
	}
	
	uinteger_t GetNumberFormatTrailing() const
	{
		return m_nftrailing;	
	}
	
	uinteger_t GetNumberFormatForce() const
	{
		return m_nfforce;
	}
	
	//////////
	
	void SetNumberFormat(uint2 p_fw, uint2 p_trailing, uint2 p_force)
    {
		m_nffw = p_fw;
		m_nftrailing = p_trailing;
		m_nfforce = p_force;
    }
	
	void SetCaseSensitive(bool p_value)
	{
		m_case_sensitive = p_value;
	}
	
	void SetConvertOctals(bool p_value)
	{
		m_convert_octals = p_value;
	}
	
	void SetWholeMatches(bool p_value)
	{
		m_whole_matches = p_value;
	}
	
	void SetUseUnicode(bool p_value)
	{
		m_use_unicode = p_value;
	}
	
	void SetUseSystemDate(bool p_value)
	{
		m_use_system_date = p_value;
	}
	
	void SetCutOff(uint2 p_value)
	{
		m_cutoff = p_value;
	}
	
	void SetLineDelimiter(char_t p_value)
	{
		m_line_delimiter = p_value;
	}
	
	void SetItemDelimiter(char_t p_value)
	{
		m_item_delimiter = p_value;
	}
	
	void SetColumnDelimiter(char_t p_value)
	{
		m_column_delimiter = p_value;
	}
	
	void SetRowDelimiter(char_t p_value)
	{
		m_row_delimiter = p_value;
    }
	
    //////////
	
	// Convert the given valueref to a string. If the type is not convertable
	// to a string, the empty string is returned.
	// This method should be used in cases where a string is required and
	// we want to silently convert non-stringables to empty.
	bool ForceToString(MCValueRef value, MCStringRef& r_string);
	bool ForceToBoolean(MCValueRef value, MCBooleanRef& r_boolean);
	
	// These attempt to convert the value as specified, returning 'true' if successeful.
	// These will raise an appropriate error if the conversion fails and
	// strict mode is on. If strict mode is off (the default) then default
	// values will be returned for things that can't be converted.
	//   (boolean - false, string - empty, number/integer/real - 0, array -
	//    empty array).
	bool ConvertToBoolean(MCValueRef value, MCBooleanRef& r_boolean);
	bool ConvertToString(MCValueRef value, MCStringRef& r_string);
    bool ConvertToData(MCValueRef p_value, MCDataRef& r_data);
    bool ConvertToName(MCValueRef p_value, MCNameRef& r_data);
	bool ConvertToNumber(MCValueRef value, MCNumberRef& r_number);
	bool ConvertToArray(MCValueRef value, MCArrayRef& r_array);
    
    bool ConvertToBool(MCValueRef value, bool& r_bool);
	bool ConvertToInteger(MCValueRef value, integer_t& r_integer);
	bool ConvertToUnsignedInteger(MCValueRef value, uinteger_t& r_integer);
	bool ConvertToReal(MCValueRef value, real64_t& r_real);
	bool ConvertToChar(MCValueRef value, char_t& r_real);
	bool ConvertToLegacyPoint(MCValueRef value, MCPoint& r_point);
	bool ConvertToLegacyRectangle(MCValueRef value, MCRectangle& r_rectangle);
	bool ConvertToLegacyColor(MCValueRef value, MCColor& r_color);
    
    bool ConvertToMutableString(MCValueRef p_value, MCStringRef &r_string);
    bool ConvertToNumberOrArray(MCExecValue &x_value);
    
	// These attempt to convert the given value as specified. If conversion
	// was successful then 'r_converted' is set to true, else 'false'. If
	// an error occurs (such as out-of-memory), false is returned.
	bool TryToConvertToBoolean(MCValueRef value, bool& r_converted, MCBooleanRef& r_boolean);
	bool TryToConvertToString(MCValueRef value, bool& r_converted, MCStringRef& r_string);
	bool TryToConvertToNumber(MCValueRef value, bool& r_converted, MCNumberRef& r_number);
	bool TryToConvertToInteger(MCValueRef value, bool& r_converted, integer_t& r_integer);
	bool TryToConvertToUnsignedInteger(MCValueRef value, bool& r_converted, uinteger_t& r_integer);
	bool TryToConvertToReal(MCValueRef value, bool& r_converted, real64_t& r_real);
	bool TryToConvertToArray(MCValueRef value, bool& r_converted, MCArrayRef& r_array);
	bool TryToConvertToLegacyPoint(MCValueRef value, bool& r_converted, MCPoint& r_point);
	bool TryToConvertToLegacyRectangle(MCValueRef value, bool& r_converted, MCRectangle& r_rectangle);
	bool TryToConvertToLegacyColor(MCValueRef value, bool& r_converted, MCColor& r_color);
	
	//////////
	
	bool CopyElementAsBoolean(MCArrayRef, MCNameRef key, bool case_sensitive, MCBooleanRef &r_boolean);
	bool CopyElementAsString(MCArrayRef, MCNameRef key, bool case_sensitive, MCStringRef &r_string);
	bool CopyElementAsNumber(MCArrayRef, MCNameRef key, bool case_sensitive, MCNumberRef &r_number);
	bool CopyElementAsInteger(MCArrayRef, MCNameRef key, bool case_sensitive, integer_t &r_integer);
	bool CopyElementAsUnsignedInteger(MCArrayRef, MCNameRef key, bool case_sensitive, uinteger_t &r_integer);
	bool CopyElementAsReal(MCArrayRef, MCNameRef key, bool case_sensitive, real64_t &r_real);
	bool CopyElementAsArray(MCArrayRef, MCNameRef key, bool case_sensitive, MCArrayRef &r_array);
	
	bool CopyElementAsStringArray(MCArrayRef, MCNameRef key, bool case_sensitive, MCArrayRef &r_string_array);
	bool CopyElementAsFilepath(MCArrayRef, MCNameRef key, bool case_sensitive, MCStringRef &r_path);
	bool CopyElementAsFilepathArray(MCArrayRef, MCNameRef key, bool case_sensitive, MCArrayRef &r_path_array);
	
	//////////
	
	bool CopyOptElementAsBoolean(MCArrayRef, MCNameRef key, bool case_sensitive, MCBooleanRef &r_boolean);
	bool CopyOptElementAsString(MCArrayRef, MCNameRef key, bool case_sensitive, MCStringRef &r_string);
	bool CopyOptElementAsStringArray(MCArrayRef, MCNameRef key, bool case_sensitive, MCArrayRef &r_string_array);
	bool CopyOptElementAsFilepath(MCArrayRef, MCNameRef key, bool case_sensitive, MCStringRef &r_path);
	bool CopyOptElementAsFilepathArray(MCArrayRef, MCNameRef key, bool case_sensitive, MCArrayRef &r_path_array);
	
	
	//////////
	
	bool FormatBool(bool p_bool, MCStringRef& r_output);
	bool FormatReal(real64_t p_real, MCStringRef& r_output);
	bool FormatUnsignedInteger(uinteger_t p_integer, MCStringRef& r_output);
	bool FormatInteger(integer_t p_integer, MCStringRef& r_output);
	bool FormatLegacyPoint(MCPoint value, MCStringRef& r_value);
	bool FormatLegacyRectangle(MCRectangle value, MCStringRef& r_value);
	bool FormatLegacyColor(MCColor value, MCStringRef& r_value);
	
	
	//////////
	
	// This method evaluates the given expression returning the result in 'result'.
	// If an error is raised during the course of evaluation, 'false' is returned.
	// Note: This method throws any errors that occur.
	bool EvaluateExpression(MCExpression *expr, MCValueRef& r_result);
    
    // These methods try to evaluate / set, as many times as the debug context dictates,
    // only throwing an error if they ultimately fail.
    bool TryToEvaluateExpression(MCExpression *p_expr, uint2 line, uint2 pos, Exec_errors p_error, MCValueRef& r_result);
    bool TryToEvaluateExpressionAsNonStrictBool(MCExpression * p_expr, uint2 line, uint2 pos, Exec_errors p_error, bool& r_result);
    bool TryToSetVariable(MCVarref *p_var, uint2 line, uint2 pos, Exec_errors p_error, MCValueRef p_value);
    
	//////////
	
	// Note: This method throws any errors that occur.
	bool EnsurePrintingIsAllowed(void);
	bool EnsureDiskAccessIsAllowed(void);
	bool EnsureProcessIsAllowed(void);
	bool EnsureNetworkAccessIsAllowed(void);
	bool EnsurePrivacyIsAllowed(void);
	
	//////////
	
	void SetItToEmpty(void);
	void SetItToValue(MCValueRef p_value);
	
	//////////
	
    MCHandler *GetHandler(void)
    {
		return m_handler;
    }
	
	void SetHandler(MCHandler *p_handler)
	{
		m_handler = p_handler;
	}
	
	MCHandlerlist *GetHandlerList()
	{
		return m_handler_list;
	}
	
	void SetHandlerList(MCHandlerlist *p_list)
	{
		m_handler_list = p_list;
	}
    
	MCObject *GetObject(void)
	{
		return m_object;
	}
	
	void SetObject(MCObject *p_object)
	{
		m_object = p_object;
	}
	
	void SetParentScript(MCParentScriptUse *p_parent_script)
	{
		m_parent_script = p_parent_script;
	}
	
    MCParentScriptUse *GetParentScript(void)
	{
		return m_parent_script;
	}
    
    
    // MM-2011-02-16: Added ability to get handle of current object
    MCObjectHandle GetObjectHandle(void);
	void SetTheResultToEmpty(void);
	void SetTheResultToValue(MCValueRef p_value);
	void SetTheResultToStaticCString(const char *p_cstring);
    void SetTheResultToNumber(real64_t p_value);
    void GiveCStringToResult(char *p_cstring);
    void SetTheResultToCString(const char *p_string);
    void SetTheResultToBool(bool p_bool);
    
	//////////
	
	bool EvalExprAsValueRef(MCExpression *expr, Exec_errors error, MCValueRef& r_value);
	bool EvalOptionalExprAsValueRef(MCExpression *expr, MCValueRef default_value, Exec_errors error, MCValueRef& r_value);
	
    bool EvalExprAsBooleanRef(MCExpression *expr, Exec_errors error, MCBooleanRef& r_value);
	bool EvalOptionalExprAsBooleanRef(MCExpression *expr, MCBooleanRef default_value, Exec_errors error, MCBooleanRef& r_value);
    
    bool EvalExprAsStringRef(MCExpression *expr, Exec_errors error, MCStringRef& r_value);
    bool EvalOptionalExprAsStringRef(MCExpression *expr, MCStringRef default_value, Exec_errors error, MCStringRef& r_value);
    bool EvalOptionalExprAsNullableStringRef(MCExpression *p_expr, Exec_errors p_error, MCStringRef& r_value);
    bool EvalExprAsMutableStringRef(MCExpression *p_expr, Exec_errors p_error, MCStringRef& r_mutable_string);
    
    bool EvalExprAsNameRef(MCExpression *expr, Exec_errors error, MCNameRef& r_value);
	bool EvalOptionalExprAsNameRef(MCExpression *expr, MCNameRef default_value, Exec_errors error, MCNameRef& r_value);
    bool EvalOptionalExprAsNullableNameRef(MCExpression *p_expr, Exec_errors p_error, MCNameRef& r_value);
    
    bool EvalExprAsDataRef(MCExpression *expr, Exec_errors error, MCDataRef& r_value);
	bool EvalOptionalExprAsDataRef(MCExpression *expr, MCDataRef default_value, Exec_errors error, MCDataRef& r_value);
    bool EvalOptionalExprAsNullableDataRef(MCExpression *p_expr, Exec_errors p_error, MCDataRef& r_value);
    
    bool EvalExprAsArrayRef(MCExpression *expr, Exec_errors error, MCArrayRef& r_value);
	bool EvalOptionalExprAsArrayRef(MCExpression *expr, MCArrayRef default_value, Exec_errors error, MCArrayRef& r_value);
    bool EvalOptionalExprAsNullableArrayRef(MCExpression *p_expr, Exec_errors p_error, MCArrayRef& r_value);
    
    bool EvalExprAsNumberRef(MCExpression *expr, Exec_errors error, MCNumberRef& r_value);
	bool EvalOptionalExprAsNumberRef(MCExpression *expr, MCNumberRef default_value, Exec_errors error, MCNumberRef& r_value);
    
	bool EvalExprAsUInt(MCExpression *expr, Exec_errors error, uinteger_t& r_uint);
	bool EvalOptionalExprAsUInt(MCExpression *expr, uinteger_t default_value, Exec_errors error, uinteger_t& r_uint);
    
    bool EvalExprAsInt(MCExpression *expr, Exec_errors error, integer_t& r_int);
	bool EvalOptionalExprAsInt(MCExpression *expr, integer_t default_value, Exec_errors error, integer_t& r_int);
    
    bool EvalExprAsBool(MCExpression *expr, Exec_errors error, bool& r_bool);
	bool EvalOptionalExprAsBool(MCExpression *expr, bool default_value, Exec_errors error, bool& r_bool);
	
    bool EvalExprAsNonStrictBool(MCExpression *expr, Exec_errors error, bool& r_bool);
    
    bool EvalExprAsDouble(MCExpression *expr, Exec_errors error, double& r_double);
	bool EvalOptionalExprAsDouble(MCExpression *expr, double default_value, Exec_errors error, double& r_double);
    
    bool EvalExprAsChar(MCExpression *expr, Exec_errors error, char_t& r_char);
	bool EvalOptionalExprAsChar(MCExpression *expr, char_t default_value, Exec_errors error, char_t& r_char);
    
    bool EvalExprAsPoint(MCExpression *expr, Exec_errors error, MCPoint& r_point);
    bool EvalOptionalExprAsPoint(MCExpression *expr, MCPoint *default_value, Exec_errors error, MCPoint *&r_point);
    
    bool EvalExprAsColor(MCExpression *expr, Exec_errors error, MCColor& r_color);
    bool EvalOptionalExprAsColor(MCExpression *expr, MCColor* default_value, Exec_errors error, MCColor*& r_color);
	
	bool EvalExprAsRectangle(MCExpression *expr, Exec_errors error, MCRectangle& r_rectangle);
    bool EvalOptionalExprAsRectangle(MCExpression *expr, MCRectangle *default_value, Exec_errors error, MCRectangle *&r_rectangle);
    	
private:
	MCObject *m_object;
	MCParentScriptUse *m_parent_script;
	MCHandlerlist *m_handler_list;
	MCHandler *m_handler;
	uint16_t m_line;
	
	uint16_t m_nffw, m_nftrailing, m_nfforce;
	uint16_t m_cutoff;
	
	bool m_convert_octals : 1;
	bool m_case_sensitive : 1;
	bool m_whole_matches : 1;
	bool m_use_system_date : 1;
	bool m_use_unicode : 1;
	
	char m_item_delimiter;
	char m_column_delimiter;
	char m_line_delimiter;
	char m_row_delimiter;
	
	Exec_stat m_stat;
};

////////////////////////////////////////////////////////////////////////////////

#endif
