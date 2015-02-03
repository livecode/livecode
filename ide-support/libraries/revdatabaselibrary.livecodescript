script "revdatabaselibrary"

on revLoadLibrary
	if the target is not me then
		pass "revLoadLibrary"
	end if	
   insert the script of me into front
end revLoadLibrary

on revUnloadLibrary
	if the target is not me then
		pass "revUnloadLibrary"
	end if	
   remove the script of me from front
end revUnloadLibrary

####################################################################################################
# Original comment by J.S
----- Database object functions using existing revdb functions ------
----- These new functions modify the existing revdb functions to
----- operate on query objects as opposed to cursors. Other objects link to query objects
-----to display, and update the query. I Plan to implement queries as stack level objects
-----in a future release you can see the planned syntax above each function. When I do this
-----These functions will simply be replaced with the actual transcript code.

# Database query objects are unplaced backgrounds stored on user's stacks. Most of the 
# database connection and query information is stored as custom properties of the backgrounds.

# Database connection info string format:
#
# dbtype,<type eg "mysql">
# dbhost,<host eg "localhost" or "10.2.1.245">
# dbdatabase,<database, the name of the database>
# dbport,<port, the port number, may be empty>
# dbuser,<user, username to log into database. May be empty>
# dbpassword,<password, password to log into database. May be empty>
# dbssl,<ssl, whether to use ssl or not, true or false>
#
# Note the the connection info string should always be sorted before being used to look up a connection.

####################################################################################################

# An array of database connections for query objects. Each key is the base64Encode of the connection details for the connection
# and each value is the integer connection id. Also stores database connection names, with keys <connectioninfo>,"Name".
global gREVDatabaseConnection

# OK-2007-10-24 : This code was duplicated throughout the library so I abstracted it into the following function,
# which is also now used in the new query builder to help assign names to database connections.

# Parameters
#   pObject : reference to the query object to act on
# Returns
#   A sorted string containing all the connection details for pObect, in the format of a "Database connection info string".
#   (See comment at top of script for details).
function revQueryConnectionInfo pObject
  local tObject
  put revQueryLongName(pObject) into tObject
  
  local tInfo
  put "dbtype",tolower(the cREVDatabaseQuery["dbtype"] of tObject) & return & \
      "dbhost",the cREVDatabaseQuery["dbhost"] of tObject & return & \
      "dbdatabase",the cREVDatabaseQuery["dbdatabase"] of tObject & return & \
      "dbport",the cREVDatabaseQuery["dbport"] of tObject & return & \
      "dbuser",the cREVDatabaseQuery["dbuser"] of tObject & return & \
      "dbpassword",the cREVDatabaseQuery["dbpassword"] of tObject & return & \
      "dbssl",the cREVDatabaseQuery["dbssl"] of tObject into tInfo
  sort tInfo
  
  return tInfo
end revQueryConnectionInfo

# Parameters
#   pObject : reference to the query object to act on
# Returns
#   The database connection id (integer) of the connection associated with pObject, or empty if the connection fails
# Description
#   Retrieves the database connection details from pObject and first checks if the query is already connected
#   if so then returns the connection id. 
#   If the query is is not connected then the function attempts to connect. If this succeeds then the connection id is returned.
#   If the attempted connection fails, then the message "revDatabaseError" is sent to the query object and empty is returned.
function revCheckConnection pObject
  local tResult
  put revQueryLongName(pObject) into pObject
  
  local tInfo
  put revQueryConnectionInfo(pObject) into tInfo
  
  local tIndex
  put base64Encode(tInfo) into tIndex
  put gREVDatabaseConnection[tIndex] into tResult
  if (tResult is a number) and (tResult is not among the items of revdb_connections()) then
    -- if the connection is disconnected while still having items in the connection cache
    split tInfo by return and comma
    
    local tConnectResult
    if tInfo["dbtype"] is "valentina" then
      put revdb_connect(tInfo["dbtype"], tInfo["dbhost"], tInfo["dbdatabase"], tInfo["dbuser"], tInfo["dbpassword"], 3 * 1024, tInfo["dbuser"], tInfo["dbpassword"]) into tConnectResult
    else
      put revdb_connect(tInfo["dbtype"], tInfo["dbhost"], tInfo["dbdatabase"], tInfo["dbuser"], tInfo["dbpassword"], tInfo["dbport"], tInfo["dbssl"]) into tConnectResult
    end if
    
    if tConnectResult is a number then
      put tConnectResult into gREVDatabaseConnection[tIndex]
      put tConnectResult into tResult
    else
      set the cREVDatabaseQuery["sqlresult"] of pObject to tConnectResult
      revHandleQueryError pObject, "Connect", pObject
      put empty into tResult
    end if
  end if
  return tResult
end revCheckConnection

# Parameters
#   pDB : A connection info string, see comments at start of script for details
# Returns
#   true if pDB represents a known database connection for a query object, false otherwise
# Description
#   Known database connections are stored in the global array gREVDatabaseConnection.
function revIsConnection pDB
  local tIndex
  put base64Encode(pDB) into tIndex
  
  local tValue
  put gREVDatabaseConnection[tIndex] into tValue
  if tValue is a number then
    return true
  else
    return false
  end if
end revIsConnection

# Parameters
#   pObject : reference to the query object to act on
#   pNewName : the name of a database connection
# Description
#   First checks if the query object pObject contains valid connection details. If not, does nothing.
#   Retrieves the connection details from the query object and checks that there is a known database connection associated with the query.
#   Gets the list of names that the connection has (some of these may be from other query objects), finds the name that the current
#   query object refers to the connection by, and replaces it with pNewName in the global array gREVDatabaseConnection.
on revChangeConnection pObject, pNewName
  put revQueryLongName(pObject) into pObject
  if revValidConnection(pObject) then
    local tInfo
    put revQueryConnectionInfo(pObject) into tInfo
    
    # OK-2007-10-23 : This looks like a bug because revIsConnection base64Encodes tInfo again...
    # This line should be moved when there is time to test it doesn't break stuff
    local tIndex
    put base64Encode(tInfo) into tIndex
    -- checks if there is a database connection and changes the name
    if revIsConnection(tInfo) then
      
      local tName
      put the cREVDatabaseQuery["name"] of pObject into tName
      
      local tNameList
      put gREVDatabaseConnection[tIndex,"Name"] into tNameList
      
      local tNumber
      put itemOffset(tName,tNameList) into tNumber
      if tNumber is not 0 then
        put pNewName into item tNumber of tNameList
        put tNameList into gREVDatabaseConnection[tIndex,"Name"]
      end if
    end if
  end if
end revChangeConnection

on revAddConnection pObject, pID
  put revQueryLongName(pObject) into pObject
  
  local tInfo
  put revQueryConnectionInfo(pObject) into tInfo
  
  local tIndex
  put base64Encode(tInfo) into tIndex
  
  local tName
  put the cREVDatabaseQuery["name"] of pObject into tName
  if pID is not empty then
    put pID into gREVDatabaseConnection[tIndex]
    put 1 into gREVDatabaseConnection[tIndex,"Number"]
    put tName into gREVDatabaseConnection[tIndex,"Name"]
    
    local tConnectionID
    put pID into tConnectionID
  else
    put gREVDatabaseConnection[tIndex] into tConnectionID
    
    local tNumber
    put gREVDatabaseConnection[tIndex,"Number"] into tNumber
    put tNumber + 1 into tNumber
    
    local tCurrentName
    put gREVDatabaseConnection[tIndex,"Name"] into tCurrentName
    put tNumber into gREVDatabaseConnection[tIndex,"Number"]
    put tName into item tNumber of gREVDatabaseConnection[tIndex,"Name"]
  end if
end revAddConnection

on revRemoveConnection pObject
  put revQueryLongName(pObject) into pObject
  
  local tInfo
  put revQueryConnectionInfo(pObject) into tInfo
  
  local tIndex
  put base64Encode(tInfo) into tIndex
  
  local tName
  put the cREVDatabaseQuery["name"] of pObject into tName
  if tIndex is not empty then
    
    local tNumber
    put gREVDatabaseConnection[tIndex,"Number"] into tNumber
    put tNumber - 1 into tNumber
    if tNumber is 0 then
      
      local tConnectionId
      put gREVDatabaseConnection[tIndex] into tConnectionId
      
      local tResult
      put revdb_disconnect(tConnectionId) after tResult
      put empty into gREVDatabaseConnection[tIndex]
      put empty into gREVDatabaseConnection[tIndex,"Number"]
      put empty into gREVDatabaseConnection[tIndex,"Name"]
      delete variable gREVDatabaseConnection[tIndex]
      delete variable gREVDatabaseConnection[tIndex,"Number"]
      delete variable gREVDatabaseConnection[tIndex,"Name"]
    else
      put tNumber into gREVDatabaseConnection[tIndex,"Number"]
      
      local tCurrentName
      put gREVDatabaseConnection[tIndex,"Name"] into tCurrentName
      
      local tItemNumber
      put itemOffset(tName,tCurrentName) into tItemNumber
      delete item tItemNumber of gREVDatabaseConnection[tIndex,"Name"]
    end if
  end if
end revRemoveConnection

function revGetDatabaseConnection pStack
  local tResult
  
  local tStackObject
  if pStack is not empty then 
    put "stack" && quote & pStack & quote into tStackObject
  else
    if char 2 to 4 of word 2 of the defaultStack is "rev" then
      put the long name of the topStack into tStackObject
    else 
      put the long name of the defaultStack into tStackObject
    end if
  end if
  repeat with i = 1 to the number of backgrounds of tStackObject
    local tObject
    put the long id of background i into tObject
    put the short name of tObject into tObject
    put revQueryLongName(tObject) into tObject
    if (tObject is not empty) and (revValidConnection(tObject)) then
      
      local tInfo
      put revQueryConnectionInfo(tObject) into tInfo
      
      if revIsConnection(tInfo) then
        
        local tIndex
        put base64Encode(tInfo) into tIndex
        
        local tNames
        put gREVDatabaseConnection[tIndex,"Name"] into tNames
        
        local tNumber
        put gREVDatabaseConnection[tIndex,"Number"] into tNumber
        put "Query Names:" & tab & tNames & tab & cr after tResult
        put "Number of Connections:" & tab & tNumber & tab & cr after tResult
        put "Connection ID:" & tab & revCheckConnection(tObject) & tab & cr after tResult
        put "Type:" & tab & tolower(the cREVDatabaseQuery["dbtype"] of tObject) & tab & cr after tResult
        put "Host:" & tab & the cREVDatabaseQuery["dbhost"] of tObject & tab & cr after tResult
        put "Database:" & tab & the cREVDatabaseQuery["dbdatabase"] of tObject & tab & cr after tResult
        put "Port:" & tab & the cREVDatabaseQuery["dbport"] of tObject & tab & cr after tResult
        put "User:" & tab & the cREVDatabaseQuery["dbuser"] of tObject & tab & cr after tResult
        put "Password:" & tab & the cREVDatabaseQuery["dbpassword"] of tObject & tab & cr after tResult
        put "SSL:" & tab & the cREVDatabaseQuery["dbssl"] of tObject & tab & cr after tResult
      end if
    end if
    put cr after tResult
  end repeat
  if the last char of tResult is cr then delete the last char of tResult
  return tResult
end revGetDatabaseConnection

on revConnectQuery pObject
   put revQueryLongName(pObject) into pObject
   
   local tInfo
   put revQueryConnectionInfo(pObject) into tInfo
   
   if revIsConnection(tInfo) is false then
      split tInfo by return and comma
      if tInfo["dbtype"] is "valentina" then
         if the platform is "Win32" then
            replace "/" with "\" in tInfo["dbdatabase"]
         end if
         if the platform is "MacOS" and char 1 of the systemVersion is not "1" then
            put revMacFromUnixPath(tInfo["dbdatabase"]) into tInfo["dbdatabase"]
         end if
         
         local tResult
         put revdb_connect(tInfo["dbtype"],tInfo["dbhost"],tInfo["dbdatabase"],tInfo["dbuser"],tInfo["dbpassword"],3*1024,tInfo["dbuser"],tInfo["dbpassword"]) into tResult
      else
         # OK-2008-07-03 : Bug 6333 : Connection to MySQL / PostgreSQL was failing with non-default ports.
         # If the database is of a type where ports can be specified and a port has been specified, then pass this
         # to the revOpenDatabase function after the hostname parameter.
         if (tInfo["dbtype"] is "MySQL" or tInfo["dbtype"] is "PostgreSQL") and (tInfo["dbport"] is not empty) then
            put revOpenDatabase(tInfo["dbtype"], (tInfo["dbhost"] & ":" & tInfo["dbport"]), tInfo["dbdatabase"], tInfo["dbuser"], tInfo["dbpassword"],tInfo["dbssl"]) into tResult
         else
            put revOpenDatabase(tInfo["dbtype"], tInfo["dbhost"], tInfo["dbdatabase"], tInfo["dbuser"], tInfo["dbpassword"],tInfo["dbssl"]) into tResult
         end if
      end if
      if tResult is a number then
         revAddConnection pObject,tResult
      else
         set the cREVDatabaseQuery["sqlresult"] of pObject to tResult
         revHandleQueryError pObject, "Connect", pObject
      end if
   else
      revAddConnection pObject
   end if   
   return tResult
end revConnectQuery


#engine syntax: disconnect query
on revDisconnectQuery pObject
  put revQueryLongName(pObject) into pObject
  if revCursorOfQuery(pObject) is not empty then
    revCloseCursorOfQuery pObject
  end if
  revRemoveConnection pObject
end revDisconnectQuery

#engine syntax: none connection dies when query is deleted
# OK-2008-03-05 : Bug 5974. Added the pDontUpdateView parameter. See comment above revDBQueryRefreshSQL.
on revCloseCursorOfQuery pObject, pDontUpdateView
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put the cREVDatabaseQuery["cursorid"] of pObject into tCursorId
   if tCursorId is "cached" then
      set the customproperties["cRevDatabaseQueryCache"] of pObject to empty
      set the cRevDatabaseQuery["recordcount"] of pObject to 0
      set the cRevDatabaseQuery["columnnames"] of pObject to empty
      set the cRevDatabaseQuery["columnlengths"] of pObject to empty
      set the cRevDatabaseQuery["columncount"] of pObject to empty
      set the cRevDatabaseQuery["columntypes"] of pObject to empty
   else if cursorVerify(pObject, tCursorId) is not empty then
      get revdb_closecursor(tCursorId)
   end if
   set the cREVDatabaseQuery["cursorid"] of pObject to empty
   
   if pDontUpdateView is not "true" then
      revDisplayQuery pObject -- clear linked objects
   end if
end revCloseCursorOfQuery

on revAddQueryObjectLink pObject,pDataLinkedObject
  revDBDisplayLinkedObject pDataLinkedObject
end revAddQueryObjectLink

# OK-2007-10-24 : Modified library to support generation of SQL queries on demand from a set
# of properties.

# Parameters
#   pObject : reference to the query object to act on
# Returns
#   The sql command for the specified query object. This may be dynamically generated from other properties of the object
#   but not in all cases.
private function revQuerySqlCommand pObject
  local tObject
  put revQueryLongName(pObject) into tObject
  
  # If there is a hardcoded sql query set on the object (i.e. old behavior) then just return this
  local tAdvancedQuery
  put the cREVDatabaseQuery["sqlquery"] of tObject into tAdvancedQuery
  if tAdvancedQuery is not empty then
    return tAdvancedQuery
  end if
  
  # Otherwise generate a query from the object's properties
  local tTable, tColumns, tFilters, tOrder
  put revQueryFormatIdentifier(revGetQueryProperty(tObject, "sqlTable")) into tTable
  put revGetQueryProperty(tObject, "sqlColumns") into tColumns
  put revGetQueryProperty(tObject, "sqlFilters") into tFilters
  put revGetQueryProperty(tObject, "sqlOrder") into tOrder
  
  # Default to all columns if none specified
  if tColumns is empty then
    put "*" into tColumns
  end if
  
  local tSql
  put "SELECT " & tColumns & " FROM " & tTable into tSql
  
  if tFilters is not empty then
    put " WHERE " & revQueryProcessFilters(tFilters, pObject) after tSql
  end if
  
  if tOrder is not empty then
    put " ORDER BY " & revQueryProcessOrder(tOrder, pObject) after tSQL
  end if
  
  return tSQL
end revQuerySqlCommand

# Parameters
#   pColumns : return delimited list of column names
# Returns
#   A comma delimited list of formatted column names suitable for use in an sql query
private function revQueryProcessColumns pColumns
  if pColumns is empty then
    return "*"
  end if
  
  local tProcessedColumns
  repeat for each line tColumn in pColumns
    put revQueryFormatIdentifier(tColumn) & comma after tProcessedColumns
  end repeat
  delete the last char of tProcessedColumns
  
  return tProcessedColumns
end revQueryProcessColumns

# Parameters
#   pFilters : return delimited list of urlEncoded filter details, format described below
#   pObject  : reference to the query object to act on
# Returns
#   A partial sql statement that links ao
private function revQueryProcessFilters pFilters, pObject
  local tSql
  
  local tColumn, tOperator, tValue
  repeat for each line tFilter in pFilters
    get urlDecode(tFilter)
    put line 2 of it into tColumn
    put line 3 of it into tOperator
    put line 4 of it into tValue
    
    local tFilterApplication
    put revGetQueryProperty(pObject, "sqlFilterApplication") into tFilterApplication
    
    put revQueryFormatIdentifier(tColumn) && tOperator && revQueryFormatFilterValue(tValue) into tSql
    if tFilterApplication is "all" then
      put " AND " after tSql
    else
      put " OR " after tSql
    end if
  end repeat
  delete the last word of tSql
  
  return tSql
end revQueryProcessFilters

# Parameters
#   pOrder : a two line string. Line 1 is the name of the column to order by. Line 2 is either "DESC" and "ASC"
#            and represents the direction to order in.
private function revQueryProcessOrder pOrder
  local tColumn, tDirection
  put line 1 of pOrder into tColumn
  put line 2 of pOrder into tDirection
  
  local tSql
  put revQueryFormatIdentifier(tColumn) && toUpper(tDirection) into tSql
  return tSql
end revQueryProcessOrder

# Parameters
#   pValue : the filter value to format
# Returns
#   The formatted filter value ready for use in an sql statement.
private function revQueryFormatFilterValue pValue
  return pValue
end revQueryFormatFilterValue

# Parameters
#   pIdentifier : an sql identifier e.g. column or table name
# Returns
#   The formatted version of the identifier suitable for use in an SQL query
private function revQueryFormatIdentifier pIdentifier
  return "`" & pIdentifier & "`"
end revQueryFormatIdentifier

# OK-2008-03-05 : Added extra parameter to fix bug 5974
# The bug was caused when this command was called by revDBDisplayLinkedObject,
# and the cursor was non-emtpy. This resulted in revCloseCursorOfQuery being called
# which after closing the query, refreshed the view, causing revDBDisplayLinkedObject
# to be triggered again, and thus entering an infinite loop. I added the extra parameter
# to avoid changing the semantics of revCloseCursorOfQuery.
on revDBQueryRefreshSQL pObject, pDontUpdateView
   put revQueryLongName(pObject) into pObject
   if revCursorOfQuery(pObject) is not empty then
      revCloseCursorOfQuery pObject, pDontUpdateView
   end if
   
   local tQuery
   --put the cREVDatabaseQuery["sqlquery"] of pObject into tQuery
   put revQuerySqlCommand(pObject) into tQuery
   
   local tConnection
   put revConnectionOfQuery(pObject) into tConnection
   
   local tResult, tNoConnection
   if tQuery is empty or tConnection is empty then 
      put "empty" into tResult
      put true into tNoConnection
   end if
   if (word 1 of tQuery is among the items of "select,show,describe") and (tNoConnection is not true) then
      # OK-2008-01-28 : Separate function for blobs no longer required
      --    if the cRevDatabaseQuery["displaybinarydata"] of pObject is true then
      --      put revdb_queryblob(tConnection, tQuery) into tResult
      --    else
      
      put revdb_query(tConnection, tQuery) into tResult
      
      --    end if
      if tResult is not a number then
         set the cREVDatabaseQuery["cursorid"] of pObject to empty
         set the cREVDatabaseQuery["sqlresult"] of pObject to tResult
         revHandleQueryError pObject, "ExecuteQuery", pObject
      else
         local tCursorId
         put tResult into tCursorId
         set the cREVDatabaseQuery["cursorid"] of pObject to tCursorId
      end if
      if the cRevDatabaseQuery["cacheresults"] of pObject is true then
         RevDBQueryCacheSQLResults pObject
         set the cREVDatabaseQuery["cursorid"] of pObject to "cached"
         
         if cursorVerify(pObject, tCursorId) is not empty then
            get revdb_closecursor(tCursorId)
         end if
      end if
   else
      put "empty" into tResult
   end if
   if tResult is not a number then
      set the cREVDatabaseQuery["sqlresult"] of pObject to tResult
   end if
   
   
   return tResult
end revDBQueryRefreshSQL

on revDBQueryCacheSQLResults pObject
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put revCursorOfQuery(pObject) into tCursorId
   
   # OK-2007-10-24 : Added to prevent infinite loop in the case of invalid cursor id
   if cursorVerify(pObject, tCursorId) is empty then
      exit revDBQueryCacheSQLResults
   end if
   
   get revDB_MoveFirst(tCursorId)
   
   local tRecordCount
   put 0 into tRecordCount
   
   local tColumnNames
   put revdb_columnnames(tCursorId) into tColumnNames
   
   local tColumnCount
   local tDataArray
   repeat while revDB_IsEOF(tCursorId) is not true
      add 1 to tRecordCount
      put 0 into tColumnCount
      repeat for each item tColumnName in tColumnNames
         add 1 to tColumnCount
         
         local tColumnData
         if revDB_ColumnIsNull(tCursorId, tColumnCount) is true then
            put "NULL" into tColumnData
         else
            put revDB_ColumnByNumber(tCursorId, tColumnCount) into tColumnData
         end if
         put tColumnData into tDataArray[tRecordCount, tColumnName]
      end repeat
      get revDB_MoveNext(tCursorId)
   end repeat
   get revDB_MoveFirst(tCursorId)
   
   set the customproperties["CRevDatabaseQueryCache"] of pObject to tDataArray
   set the cRevDatabaseQuery["currentrecord"] of pObject to 1
   set the cRevDatabaseQuery["recordcount"] of pObject to tRecordCount
   set the cRevDatabaseQuery["columnnames"] of pObject to tColumnNames
   set the cRevDatabaseQuery["columncount"] of pObject to revdb_columncount(tCursorId)
   set the cRevDatabaseQuery["columnlengths"] of pObject to revdb_columnlengths(tCursorId)
   set the cRevDatabaseQuery["columntypes"] of pObject to revdb_columntypes(tCursorId)
end revDBQueryCacheSQLResults

on revRefreshQuery pObject
  put revQueryLongName(pObject) into pObject
  revDBQueryRefreshSQL pObject
  revDisplayQuery pObject
end revRefreshQuery

on revDisplayQuery pObject
  put revQueryLongName(pObject) into pObject
  
  local tDefault
  put the short name of the defaultStack into tDefault
  set the defaultStack to revTargetStack(pObject)
  repeat with i = 1 to the number of controls
    
    local tDataLinkObject
    put the long id of control i into tDataLinkObject
    if (the cREVGeneral["database"] of tDataLinkObject is true) then
      
      local tWhichQueryControl
      put the cREVDatabase["queryobject"] of tDataLinkObject into tWhichQueryControl
      put word 2 of tWhichQueryControl into tWhichQueryControl
      if the first char of tWhichQueryControl is quote then delete the first char of tWhichQueryControl
      if the last char of tWhichQueryControl is quote then delete the last char of tWhichQueryControl
      if (tWhichQueryControl) is (the short name of pObject) then
        revDBDisplayLinkedObject tDataLinkObject
      end if
      put the cREVDatabase["menuqueryobject"] of tDataLinkObject into tWhichQueryControl
      if tWhichQueryControl is empty then next repeat
      put word 2 of tWhichQueryControl into tWhichQueryControl
      if the first char of tWhichQueryControl is quote then delete the first char of tWhichQueryControl
      if the last char of tWhichQueryControl is quote then delete the last char of tWhichQueryControl
      if (tWhichQueryControl) is (the short name of pObject) then
        revDBUpdateMenuText tDataLinkObject
      end if
    end if
  end repeat
   
  # OK-2008-02-14 : Added message to allow user stack's to perform actions in response to query refresh.
  lock screen
  try
    send "queryRecordChanged" && revQueryShortName(pObject) to this card
  catch someError
    -- do nothing if there is no handler
  end try
  unlock screen
   
   
  set the defaultStack to tDefault
end revDisplayQuery

on revChangeQueryObjectName pObject,pNewName
  put revQueryLongName(pObject) into pObject
  revChangeConnection pObject,pNewName
  
  local tDefault
  put the short name of the defaultStack into tDefault
  set the defaultStack to revTargetStack(pObject)
  repeat with i = 1 to the number of controls
    
    local tDataLinkObject
    put the long id of control i into tDataLinkObject
    if (the cREVGeneral["database"] of tDataLinkObject is true) then
      local tWhichQueryControl
      put the cREVDatabase["queryobject"] of tDataLinkObject into tWhichQueryControl
      put word 2 of tWhichQueryControl into tWhichQueryControl
      if the first char of tWhichQueryControl is quote then delete the first char of tWhichQueryControl
      if the last char of tWhichQueryControl is quote then delete the last char of tWhichQueryControl
      if (tWhichQueryControl) is (the short name of pObject) then
        revDBChangeQueryObjectName tDataLinkObject, pNewName
      end if
    end if
  end repeat
  set the defaultStack to tDefault
end revChangeQueryObjectName

on revDBChangeQueryObjectName pDataLinkObject, pNewName
  if the cREVDatabase["queryobject"] of pDataLinkObject is not empty then
    local tObject
    put the cREVDatabase["queryobject"] of pDataLinkObject into tObject
  else
    exit revDBChangeQueryObjectName
  end if
  put revQueryLongName(tObject) into tObject
  -- just want to replace the name of the background not the stack if it has the same name
  put quote & pNewName & quote into pNewName
  put pNewName into word 2 of tObject
  set the cREVDatabase["queryobject"] of pDataLinkObject to tObject
end revDBChangeQueryObjectName

command revDBDisplayLinkedObject pDataLinkObject
  if the cREVDatabase["queryobject"] of pDataLinkObject is empty then
    exit revDBDisplayLinkedObject
  end if
  
  local tObject
  put the cREVDatabase["queryobject"] of pDataLinkObject into tObject
  
  local tWhichColumn
  put the cREVDatabase["linkcolumn"] of pDataLinkObject into tWhichColumn
  if tWhichColumn is empty then
    if the cREVDatabase["showRecordNumber"] of pDataLinkObject is true then
      revDBDisplayRecordNumber pDataLinkObject, tObject
    end if
     
    exit revDBDisplayLinkedObject
  end if
   
  # OK-2008-01-28 : Unfortunately the below change caused problems when using a table field with only a single column displaying
  # so instead of checking the number of columns, we need to check whether or not the control is a table field.
  if the cREVGeneral["table"] of pDataLinkObject is true then
     
    # TM-2007-10-26 : Changed from ">=" due to unwanted table behaviour in single-line fields
    --if tWhichColumn is "show all" or the number of items of tWhichColumn > 1 then
    revDBQueryDisplayTable tObject, pDataLinkObject
    exit revDBDisplayLinkedObject
  end if
   
  local tData
  if tWhichColumn is a number then
    revDBQueryColumnByNumber tObject, tWhichColumn, tData
  else
    revDBQueryColumnByName tObject, tWhichColumn, tData
  end if
  
  switch word 1 of pDataLinkObject
  case "field"
    switch the cREVDatabase["saveoptions"]  of pDataLinkObject
    case "htmltext"
      set the htmlText of pDataLinkObject to tData
      break
    case "rtftext"
      set the rtfText of pDataLinkObject to tData
      break
    case "unicodetext"
      set the unicodeText of pDataLinkObject to tData
      break
    default
      set the text of pDataLinkObject to tData
      break
    end switch
    break
  case "image"
    set the text of pDataLinkObject to tData
    break
  case "button"
    switch style of pDataLinkObject
    case "checkbox"
      local tIsCheckMarked
      put false into tIsCheckMarked
      if tData is true or tData is 1 then put true into tIsCheckMarked
      set the hilite of pDataLinkObject to tIsCheckMarked
      break
    case "menu"
      switch the menuMode of pDataLinkObject
      case "combobox"
        set the label of pDataLinkObject to tData
        break
      case "option"
        set the wholeMatches to true
        switch the cREVDatabase["saveoptions"] of pDataLinkObject
        case "itemtext"
          put lineOffset(tData,the text of pDataLinkObject) into tData
          break
        case "keyvalue"
          local tMenuQueryObject
          put the cREVDatabase["menuqueryobject"] of pDataLinkObject into tMenuQueryObject
          if tMenuQueryObject is not empty then
            local tKeysValues
            put revGetKeysValuesOfQuery(tMenuQueryObject) into tKeysValues
            put lineOffset(tData, tKeysValues) into tData
          end if
          break
        default
          break
        end switch
        set the wholeMatches to false
        if tData is a number then set the menuhistory of pDataLinkObject to tData
        break
      default
        if tData is a number then set the menuhistory of pDataLinkObject to tData
        break
      end switch
      break
    end switch
    break
  case "group"
    if the cREVDatabase["saveoptions"] of pDataLinkObject is "number"
    then set the hilitedButton of pDataLinkObject to tData
    else set the hilitedButtonName of pDataLinkObject to tData
    break
  end switch
end revDBDisplayLinkedObject

private command revDBDisplayRecordNumber pObject, pQuery
  local tRecordNumber
  put revCurrentRecordOfQuery(pQuery) into tRecordNumber
  
  switch word 1 of pObject
  case "field"
    set the text of pObject to tRecordNumber
    break
  end switch
end revDBDisplayRecordNumber

on revDBUpdateMenuText pDataLinkObject
  local tObject
  put the cREVDatabase["menuqueryobject"] of pDataLinkObject into tObject
  if tObject is not empty then
    local tResult
    put empty into tResult
    put revQueryLongName(tObject) into tObject
    
    local tConnectionId
    put revConnectionOfQuery(tObject) into tConnectionId
    if tConnectionId is empty then exit revDBUpdateMenuText
    
    -- first determine the column numbers to display
    local tColumnToDisplay
    put the cREVDatabase["menulinkcolumn"] of pDataLinkObject into tColumnToDisplay
    
    local tColumnNames
    put revColumnNamesOfQuery(tObject) into tColumnNames
    
    local tItemNumber
    put itemOffset(tColumnToDisplay, tColumnNames) into tItemNumber
    
    local tRecordCount
    put revRecordCountOfQuery(tObject) into tRecordCount
    local columndata
    if tRecordCount <> 0 then
      local tOldCurrentRecord
      put revCurrentRecordOfQuery(tObject) into tOldCurrentRecord
      get revDBQueryMoveFirst(tObject)
      repeat tRecordCount times
        if revDBQueryColumnIsNull(tObject, tItemNumber) is true then
          put "NULL" & return after tResult
        else
          revDBQueryColumnByNumber tObject, tItemNumber, columndata
          put columndata & return after tResult
        end if
        get revDBQueryMoveNext(tObject)
      end repeat
      delete char -1 of tResult
      revDBQueryGoToRecord tObject, tOldCurrentRecord
    end if
    set the text of pDataLinkObject to tResult
  end if
end revDBUpdateMenuText

#engine syntax: commit query xxx
on revCommitQuery pObject
  put revQueryLongName(pObject) into pObject
  
  local tConnectionId
  put revConnectionOfQuery(pObject) into tConnectionId
  
  local tResult
  put revdb_commit(tConnectionId) into tResult
  return tResult
end revCommitQuery

on revExecuteWithQuery pObject,pSQL
  put revQueryLongName(pObject) into pObject
  
  local tConnectionId
  put revConnectionOfQuery(pObject) into tConnectionId
  return revdb_execute(tConnectionId, pSQL)
end revExecuteWithQuery

#engine syntax: rollback query xxx
function revRollBackQuery pObject
  put revQueryLongName(pObject) into pObject
  
  local tConnectionId
  put revConnectionOfQuery(pObject) into tConnectionId
  
  local tResult
  put revdb_rollback(tConnectionId) into tResult
  return tResult
end revRollBackQuery

# OK-2007-08-02 : Changed semantics and added pRecordNumber parameter to fix bug where updating
# any row apart from the first would not work.
# Parameters
#   pObject : reference to the query object to operate on
#   pRecordNumber : the 0-based index of the record number retrieve the list of key values for. Same as the line number in the query object
# Returns
#   A return-delimited list of the values of each of the primary keys for the row specified by pRecordNumber.
# Comments
#   If pRecordNumber is empty then the primary key values for the first record are returned (to be consistent with
#   previous behavior).
function revGetKeysValuesOfQuery pObject, pRecordNumber
   put revQueryLongName(pObject) into pObject
   
   local tPrimaryKeyStrings
   put the cREVDatabaseQuery["primarykey"] of pObject into tPrimaryKeyStrings
   
   local tOldRecordNumber
   put revCurrentRecordOfQuery(pObject) into tOldRecordNumber
   
   # First move down until we get to the correct record (and hope this doesn't break anything)
   get revDBQueryMoveFirst(pObject)
   if pRecordNumber is not empty and pRecordNumber > 0 then
      # OK-2008-07-07 : Bug 6679 - As Record numbers are now 0 based, we don't have to subtract one anymore
      --repeat pRecordNumber - 1 times
      repeat pRecordNumber times
         get revDBQueryMoveNext(pObject)
      end repeat
   end if
   
   local tKeyValues
   repeat for each line tPrimaryKey in tPrimaryKeyStrings
      local tPrimaryKeyValue
      revDBQueryColumnByName pObject, tPrimaryKey, tPrimaryKeyValue
      put tPrimaryKeyValue & return after tKeyValues
   end repeat
   delete the last char of tKeyValues
   
   # OK-2008-07-07 : Bug 6679 - This function should not change the current record, so we have to restore it to its previous
   # value before returning our result.
   get revDBQueryMoveFirst(pObject)
   if tOldRecordNumber is not empty and tOldRecordNumber > 0 then
      repeat tOldRecordNumber times
         get revDBQueryMoveNext(pObject)
      end repeat
   end if
   
   return tKeyValues
end revGetKeysValuesOfQuery

on revUpdateQueryObject pDataLinkedObject
   if there is not a pDataLinkedObject then 
      exit revUpdateQueryObject
   end if
   
   if the cREVGeneral["database"] of pDataLinkedObject is not true or the cREVDatabase["updateafterediting"] of pDataLinkedObject is not true then
      exit revUpdateQueryObject
   end if
   
   local tObject
   put the cREVDatabase["queryobject"] of pDataLinkedObject into tObject
   put revQueryLongName(tObject) into tObject
   
   local tColumnName
   put the cREVDatabase["linkcolumn"] of pDataLinkedObject into tColumnName
   
   # OK-2007-07-03: This mistake would have caused this command to always fail. I'm not sure what the 
   # intended behavior was, but it seems like tColumnName might be what was meant, thus I added what 
   # I think is the correct if statement below.
   
   --    if tcolumn is empty or tcolumn is "show all" then
   --      exit revUpdateQueryObject
   --    end if
   --    
   
   if tColumnName is empty or tColumnName is "show all" then
      exit revUpdateQueryObject
   end if
   
   switch word 1 of pDataLinkedObject
      case "button"
         local tValue
         switch the style of pDataLinkedObject
            case "menu"
               switch the menuMode of pDataLinkedObject
                  case "combobox"
                     put the label of pDataLinkedObject into tValue
                  break
                  case "option"
                     put the menuhistory of pDataLinkedObject into tValue
                     switch the cREVDatabase["saveoptions"] of pDataLinkedObject
                        case "itemtext"
                           put line tValue of the text of pDataLinkedObject into tValue
                        break
                        case "keyvalue"
                           local tMenuQueryObject
                           put the cREVDatabase["menuqueryobject"] of pDataLinkedObject into tMenuQueryObject
                           if tMenuQueryObject is not empty then
                              local tKeysValues
                              put revGetKeysValuesOfQuery(tMenuQueryObject) into tKeysValues
                              put line tvalue of tKeysValues into tValue
                           end if
                        break
                        default
                        break
                     end switch # the cREVDatabase["saveoptions"] of pDataLinkedObject
                  default
                     put the menuHistory of pDataLinkedObject into tValue
                  break
               end switch # the menuMode of pDataLinkedObject
            case "checkbox"
               put the hilite of pDataLinkedObject into tValue
            break
         end switch # the style of pDataLinkedObject
      break
      case "field"
         switch the cREVDatabase["saveoptions"] of pDataLinkedObject
            case "htmltext"
               put the htmltext of pDataLinkedObject into tValue
            break
            case "rtftext"
               put the rtftext of pDataLinkedObject into tValue
            break
            case "unicodetext"
               put the unicodetext of pDataLinkedObject into tValue
            break
            default
               put the text of pDataLinkedObject into tValue
            break
         end switch # the cREVDatabase["saveoptions"] of pDataLinkedObject
      break
      case "group"
         if the cREVDatabase["saveoptions"] of pDataLinkedObject is "number"
         then put the hilitedButton of pDataLinkedObject into tValue
         else put the hilitedButtonName of pDataLinkedObject into tValue
      break
      default
         put the text of pDataLinkedObject into tValue
      break
   end switch # word 1 of pDataLinkedObject
   
   # OK-2008-07-04 : Bug 6679 : "Cell editing" would not previously work with anything but the first record,
   # to fix this, we obtain the current record number, and pass it to revGetKeysValuesOfQuery.
   local tCurrentRecord
   put revCurrentRecordOfQuery(tObject) into tCurrentRecord
   
   local tKeyValues
   put revGetKeysValuesOfQuery(tObject, tCurrentRecord) into tKeyValues
   
   local tDataType
   if word 1 of pDataLinkedObject is "image" then
      put "binary" into tDataType
   else
      put "text" into tDataType
   end if
   
   revWriteCellData tObject, tColumnName, tValue, tKeyValues, tDataType
end revUpdateQueryObject

on revUpdateColumnOfQuery pObject,pColumnName,pValue
  put revQueryLongName(pObject) into pObject
  local tKeyValues
  put revGetKeysValuesOfQuery(pObject) into tKeyValues
  revWriteCellData pObject, pColumnName, pValue, tKeyValues, "text"
end revUpdateColumnOfQuery

#engine syntax: update query xxx to force an update
on revUpdateQuery pObject
  put revQueryLongName(pObject) into pObject
  if pObject is empty then
    exit revUpdateQuery
  end if
  
  local tDefault
  put the short name of the defaultStack into tDefault
  set the defaultStack to revTargetStack(pObject)
  repeat with i = 1 to the number of controls
    
    local tDataLinkObject
    put the long id of control i into tDataLinkObject
    if (the cREVGeneral["database"] of tDataLinkObject is true) then
      
      local tWhichQueryControl
      put the cREVDatabase["queryobject"] of tDataLinkObject into tWhichQueryControl
      put word 2 of tWhichQueryControl into tWhichQueryControl
      if the first char of tWhichQueryControl is quote then delete the first char of tWhichQueryControl
      if the last char of tWhichQueryControl is quote then delete the last char of tWhichQueryControl
      if (tWhichQueryControl) is (the short name of pObject) then
         # OK-2008-07-07 : This appears to be wrong and is throwing an error, copied the code from 2.8.1 in to replace it.
         --tWhichQueryControl tDataLinkObject
         revUpdateQueryObject tDataLinkObject
      end if
    end if
  end repeat
  set the defaultStack to tDefault
end revUpdateQuery

function revSetQueryString pObject, pColumnName
  -- generates part of the query string for updating the columns
  -- pColumnName and pColumnVal are a line separated list of columns
  -- and their associated values
  put revQueryLongName(pObject) into pObject
  local tColumnCount
  put the number of lines in pColumnName into tColumnCount
  
  local tResult
  repeat with i = 1 to tColumnCount
    local tPlaceholder
    put line i of pColumnName into tPlaceholder
    put tPlaceholder & " = :" & i after tResult
    put " , " after tResult
  end repeat
  delete last word of tResult
  return tResult
end revSetQueryString

function revKeyQueryString pObject, pKey
  -- generates part of the query string for the primary keys
  -- pKey is a line separated list of different key values used to
  -- form the primary key
  put revQueryLongName(pObject) into pObject
  
  local tPrimaryKey
  put the cREVDatabaseQuery["primarykey"] of pObject into tPrimaryKey
  
  local tKeyCount
  put the number of lines in tPrimaryKey into tKeyCount
  
  local tResult
  repeat with i = 1 to tKeyCount
    local tPlaceholder
    put line i of tPrimaryKey into tPlaceholder
    
    local tKeyValue
    put line i of pKey into tKeyValue
    put tPlaceholder & " = '" & tKeyValue & "'" after tResult
    put " AND " after tResult
  end repeat
  delete last word of tResult
  return tResult
end revKeyQueryString

on revWriteCellData pObject, pColumnName, pColumnVal, pKey, pDataType
   -- updates the database using the primary key value(s) where the columns
   -- name and values are specified.
   put revQueryLongName(pObject) into pObject
   
   local tConnectionId
   put revConnectionOfQuery(pObject) into tConnectionId
   if tConnectionId is not a number then
      revConnectQuery pObject
      put revConnectionOfQuery(pObject) into tConnectionId
   end if
   if tConnectionId is empty then 
      revHandleQueryError pObject, "Connect", empty
      exit revWriteCellData
   end if
   
   local tPrimaryKeyTest
   put the cREVDatabaseQuery["primarykey"] of pObject into tPrimaryKeyTest
   if tPrimaryKeyTest is empty then
      revHandleQueryError pObject, "Nokey", pObject
      exit revWriteCellData
   end if
   if the cREVDatabaseQuery["defaulttablename"] of pObject is not empty then
      
      local tCurrentTable
      put the cREVDatabaseQuery["defaulttablename"] of pObject into tCurrentTable
   else
      put the cREVDatabaseQuery["tablename"] of pObject into tCurrentTable
   end if
   if tCurrentTable is empty then
      revHandleQueryError pObject, "NoTable", pObject
      exit revWriteCellData
   end if
   
   local tPrimaryKey
   put revKeyQueryString(pObject,pKey) into tPrimaryKey
   
   local tUpdatedColumns
   put revSetQueryString(pObject,pColumnName) into tUpdatedColumns
   
   local tQuery
   put "UPDATE " & tCurrentTable & " SET " & tUpdatedColumns & " WHERE " & tPrimaryKey into tQuery
   
   local tDatabaseResult
   if pDataType is "binary" then
      put revdb_execute(tConnectionId,tQuery,"*b"&"pColumnVal") into tDatabaseResult
   else
      put revdb_execute(tConnectionId,tQuery,"pColumnVal") into tDatabaseResult
   end if
   
   if tDatabaseResult is not a number then
      revHandleQueryError pObject, "WriteData", tQuery
   else
      if the cRevDatabaseQuery["cursorid"] of pObject is "cached" then
         local tCurrentRecord
         put the cRevDatabaseQuery["currentrecord"] of pObject into tCurrentRecord
         set the cRevDatabaseQueryCache[tCurrentRecord,pColumnname]of pObject to pColumnval
      else
         # OK-2008-07-07 : Bug 6679 - Remember the old current record and preserve this after updating query
         local tOldRecordNumber
         put revCurrentRecordOfQuery(pObject) into tOldRecordNumber
         
         # OK-2008-03-19: Bug 6125. If the table was sucessfully updated, we
         # need to re-execute the query and obtain a new cursor so that
         # when the linked controls are next updated, they contain the new value.
         # This is only done if we are not caching the record set.
         revDBQueryRefreshSQL pObject, true
         
         if tOldRecordNumber is not empty and tOldRecordNumber > 0 then
            repeat tOldRecordNumber times
               get revDBQueryMoveNext(pObject)
            end repeat
         end if
         
      end if
   end if
end revWriteCellData

on revWriteImageData pDataLinkedObject, pImage
  set the text of pDataLinkedObject to URL ("binfile:" & pImage)
  revUpdateQueryObject pDataLinkedObject
end revWriteImageData

-- **Bookmark Entry** accessors
--accessors
function revConnectionErr pObject
  put revQueryLongName(pObject) into pObject
  local tConnectionId
  put the cREVDatabase["connectionid"] of pObject into tConnectionId
  
  local tResult
  put revdb_connectionerr(tConnectionId) into tResult
  return tResult
end revConnectionErr

on revSetCurrentTableOfQuery pObject, pTable
  put revQueryLongName(pObject) into pObject
  set the cREVDatabaseQuery["tablename"] of pObject to pTable
end revSetCurrentTableOfQuery

function revUseBinaryDataOfQuery pObject
  put revQueryLongName(pObject) into pObject
  
  local tResult
  put the cREVDatabaseQuery["displaybinarydata"] of pObject into tResult
  return tResult
end revUseBinaryDataOfQuery

#engine: the currentrecord of query xxx
function revCurrentRecordOfQuery pObject
   put revQueryLongName(pObject) into pObject
   local tCursorId
   put revCursorOfQuery(pObject) into tCursorId
   if tCursorId is empty then
      return empty
   end if
   
   if tCursorId is "cached" then
      local tResult
      put the cRevDatabaseQuery["currentrecord"] of pObject into tResult
   else
      if cursorVerify(pObject, tCursorId) is not empty then
         put revdb_currentrecord(tCursorId) into tResult
      end if
   end if
   return tResult
end revCurrentRecordOfQuery

#engine: the cursorerror of query xxx
function revCursorErrorOfQuery pObject
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put revCursorOfQuery(pObject) into tCursorId
   
   local tResult
   if tCursorId is empty then
      return empty
   end if
   if tCursorId is "cached" then
      put empty into tResult
   else 
      if cursorVerify(pObject, tCursorId) is not empty then
         put revdb_cursorerr(tCursorId) into tResult
      end if
   end if
   return tResult
end revCursorErrorOfQuery
  

#engine syntax: the databasetype of query xxx
function revDatabaseTypeOfQuery pObject
  put revQueryLongName(pObject) into pObject
  
  local tConnectionid
  put revConnectionOfQuery(pObject) into tConnectionid
  if tConnectionid is empty then return empty
  
  local tResult
  put revdb_dbtype(tConnectionid) into tResult
  return tResult
end revDatabaseTypeOfQuery


# Parameters
#   pObject : reference to the query object to act on
#   pSQL : the sql query to set
# Description
#   Sets the SQL statement that the specified query object should use to refresh itself, then refreshes the query.
on revSetSQLOfQuery pObject, pSQL
  put revQueryLongName(pObject) into pObject
  set the cRevDatabaseQuery["sqlquery"] of pObject to pSQL
  revRefreshQuery pObject
end revSetSQLOfQuery

# Parameters
#   pObject : reference to the query object to act on
# Description
#   Returns the SQL statement that the specified query object uses to refresh itself
function revGetSQLOfQuery pObject
  local tObject
  put revQueryLongName(pObject) into tObject
  return revQuerySQLCommand(tObject)
end revGetSQLOfQuery

# OK-2007-10-19 : Added functions to allow arbitrary query properties to be set on query objects
# These are used by the new query builder. Note that these functions do not automatically refresh the query

# Documented properties:

# "consistent" - A query object is consistent if it is in a state where it would be expected to work.
#  Reasons for being inconsistent include being edited by the query builder or having an incorrect
#  query entered by the user, or an error was thrown when trying to execute the query. Note that
#  an empty value means "true" in this case for compatibility with older stacks.

# Parameters
#   pObject : reference to the query object to act on
#   pTag : name of the property to set
#   pValue : value to set
# Description
#   Sets the cREVDatabaseQuery[pTag] of the query object pObject to pValue
command revSetQueryProperty pObject, pTag, pValue
  local tObject
  put revQueryLongName(pObject) into tObject
  set the cRevDatabaseQuery[pTag] of tObject to pValue
end revSetQueryProperty

# Parameters
#   pObject : reference to the query object to act on
#   pTag : name of the property to set
# Returns
#   The current value of the property
# Description
#   Returns the cREVDatabaseQuery[pTag] of the query object pObject
function revGetQueryProperty pObject, pTag
  local tObject
  put revQueryLongName(pObject) into tObject
    return the cRevDatabaseQuery[pTag] of tObject
end revGetQueryProperty

# OK-2007-10-19 : Added function that returns SQL for a query object generated from a set of custom properties

# Parameters
#   pObject : reference to the query object to act on
# Returns
#   The SQL query for the query object.
# Description
#   This function does not currently do any error checking...
#   If no columns have been set, the default is to display all columns, so a query of the form
#   SELECT * FROM is generated. If insufficient information is available to generate a valid query
#   then empty is returned. This will happen in particular if no table has been selected.
function revGenerateQuerySQL pObject
  local tObject
  put revQueryLongName(pObject) into tObject
  
  local tColumns
  put revGetQueryProperty(tObject, "sqlColumns") into tColumns
  replace return with comma in tColumns
  if tColumns is empty then
    put "*" into tColumns
  end if
  
  local tTable
  put revGetQueryProperty(tObject, "sqlTable") into tTable
  if tTable is empty then
    return empty
  end if
  
  # Basic query, eg SELECT <columns> FROM <table>
  local tSQL
  put "SELECT " & tColumns & " FROM " & revGetQueryProperty(tObject, "sqlTable") into tSQL
  
  # Filters and ordering, these are of course optional
  local tFilters
  put revGetQueryProperty(tObject, "sqlFilters") into tFilters
  
  # Decode the filters and construct the appropriate SQL. Each filter is urlEncoded and contains 4 lines
  # Line 1 : The filter id (an integer used internally by the query builder)
  # Line 2 : The name of the column to apply the filter to
  # Line 3 : The operator to use
  # Line 4 : The value that the column should be compared to (this may of course be another column name)
  local tColumn, tOperator, tValue, tFilterSQL
  repeat for each line tFilter in tFilters
    get urlDecode(tFilter)
    put line 2 of it into tColumn
    put line 3 of it into tOperator
    put line 4 of it into tValue
    put revQueryFormatIdentifier(tColumn) && tOperator && tValue after tFilterSQL
    if revGetQueryProperty(tObject, "sqlFilterApplication") is "all" then
      put " AND " after tFilterSQL
    else 
      put " OR " after tFilterSQL
    end if
  end repeat
  delete the last word of tFilterSQL
  
  if tFilterSQL is not empty then
    put " WHERE " & tFilterSQL after tSQL
  end if
  
  # Order, just a single column at the moment...
  local tOrderColumn, tOrderDirection
  get revGetQueryProperty(tObject, "sqlOrder")
  put line 1 of it into tOrderColumn
  put line 2 of it into tOrderDirection
  
  local tOrderSQL
  if tOrderColumn is not empty then
    put " ORDER BY " & tOrderColumn after tSQL
    if tOrderDirection is "descending" then
      put "DESC" after tSQL
    else
      put "ASC" after tSQL
    end if
  end if
   
  return tSQL
end revGenerateQuerySQL


function revRecordCountOfQuery pObject
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put revCursorOfQuery(pObject) into tCursorId
   if tCursorId is empty then return empty
   if tCursorId is "cached" then
      local tResult
      put the cRevDatabaseQuery["recordcount"] of pObject into tResult
   else
      if cursorVerify(pObject, tCursorId) is not empty then
         put revdb_recordcount(tCursorId) into tResult
      end if
   end if
   return tResult
end revRecordCountOfQuery
  

#engine syntax: the columncount of query xxx
function revColumnCountOfQuery pObject
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put revCursorOfQuery(pObject) into tCursorId
   if tCursorId is empty then return empty
   if tCursorId is "cached" then
      local tResult
      put the cRevDatabaseQuery["columncount"] of pObject into tResult
   else 
      if cursorVerify(pObject, tCursorId) is not empty then
         put revdb_columncount(tCursorId) into tResult
      end if
   end if
   return tResult
end revColumnCountOfQuery

# Parameters
#   pObject : reference to the query object to act on
# Returns
#   A comma-delimited list of the available column names of the query, or empty if a connection cannot be established
function revColumnNamesOfQuery pObject
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put revCursorOfQuery(pObject) into tCursorId
   if tCursorId is empty then return empty
   
   local tResult
   if tCursorId is "cached" then
      put the cREVDatabaseQuery["columnnames"] of pObject into tResult
   else
      if cursorVerify(pObject, tCursorId) is not empty then
         put revdb_columnnames(tCursorId) into tResult
         if (tResult is "Tables_in_test") and (revDatabaseTypeOfQuery(pObject) is "mysql") then
            revRefreshQuery pObject
            put revCursorOfQuery(pObject) into tCursorId
            put revdb_columnnames(tCursorId) into tResult
         end if
      end if
   end if
   if item 1 of tResult is "revdberr" then put empty into tResult
   return tResult
end revColumnNamesOfQuery

# OK-2007-10-30 : Added as this seemed to be missing.
# Parameters
#   pObject : reference to the query object to act on
# Returns
#   A comma-delimited list of column names
# Description
#   This function returns the columns that are available in the current table of the query, as apposed to 
#   revColumnNamesOfQuery which returns the columns which are actually being displayed. If the query is 
#   "advanced", then we technically do no know the table name and therefore we just return the same value as
#   revColumnNamesOfQuery. Returns empty if an error occurs.
function revAvailableColumnsOfQuery pObject
  local tTable
  put revGetQueryProperty(pObject, "sqlTable") into tTable
  if tTable is empty then
    return revColumnNamesOfQuery(pObject)
  end if
  
  local tConnection
  put revConnectionOfQuery(pObject) into tConnection
  
  local tQuery
  put "SELECT * FROM " & tTable & " WHERE 0" into tQuery
  
  local tCursor
  put revQueryDatabase(tConnection, tQuery) into tCursor
  
  local tColumns
  put revDatabaseColumnNames(tCursor) into tColumns
  
  revCloseCursor tCursor
  
  if item 1 of tColumns is "revdberr" then
    return empty
  else
    return tColumns
  end if
end revAvailableColumnsOfQuery

#engine syntax: the columnlengths of query xxx
function revColumnLengthsOfQuery pObject
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put revCursorOfQuery(pObject) into tCursorId
   if tCursorId is empty then return empty
   if tCursorId is "cached" then
      local tResult
      put the cRevDatabaseQuery["columnlengths"] of pObject into tResult
   else
      if cursorVerify(pObject, tCursorId) is not empty then
         put revdb_columnlengths(tCursorId) into tResult
      end if
   end if
   return tResult
end revColumnLengthsOfQuery

#engine syntax: the columntypes of query xxx
function revColumnTypesOfQuery pObject
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put revCursorOfQuery(pObject) into tCursorId
   if tCursorId is empty then return empty
   if tCursorId is "cached" then
      local tResult
      put the cRevDatabaseQuery["columntypes"] of pObject into tResult
   else
      if cursorVerify(pObject, tCursorId) is not empty then
         put revdb_columntypes(tCursorId) into tResult
      end if
   end if 
   return tResult
end revColumnTypesOfQuery

# OK-2008-06-19 : Bug 6564 - An error will now by thrown by revDB if invalid cursor is passed to it
# ensure this does not happen.
# Parameters
#   pObject : the "query long name" of a query object
#   pCursorId : the proposed id of a cursor
# Returns
#   If the cursor is valid, returns pCursorId, else returns empty
private function cursorVerify pObject, pCursorId
   local tConnection
   put revConnectionOfQuery(pObject) into tConnection
   if tConnection is empty then
      return empty
   end if
   
   if pCursorId is among the items of revDatabaseCursors(tConnection) then
      return  pCursorId
   else
      return empty
   end if
end cursorVerify
  

#engine syntax: the cursor of query
function revCursorOfQuery pObject
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put the cREVDatabaseQuery["cursorid"] of pObject into tCursorId
   if tCursorId is not empty then
      if tCursorId is not "cached" then
         # OK-2008-06-19 : Bug 6564 - An error will now by thrown by revDB if invalid cursor is passed to it
         # ensure this does not happen.
         if cursorVerify(pObject, tCursorId) is not empty then
            local tResult
            put revdb_cursorerr(tCursorId) into tResult
            if item 1 of tResult is "revdberr" then return empty
         end if
      end if
   end if
   return tCursorId
end revCursorOfQuery

#engine syntax: the connection of query

# Parameters
#   pObject : reference to the query object to act on
# Returns
#   The integer connection id of the database connection associated with pObject,
#   or empty if the query is not connected.
function revConnectionOfQuery pObject
  put revQueryLongName(pObject) into pObject
  
  local tConnectionId
  put revCheckConnection(pObject) into tConnectionId
  --put the cREVDatabaseQuery["connectionid"] of pObject into connectionid
  if tConnectionId is empty or tConnectionId is not among the items of revdb_connections() \
      then return empty
  return tConnectionId
end revConnectionOfQuery

# Parameters
#   pObject : reference to the query object to act on
# Returns
#   A return-delimited list of table names, or empty if a connection cannot be established
function revTableNamesOfQuery pObject
  put revQueryLongName(pObject) into pObject
  local tConnectionId
  put revConnectionOfQuery(pObject) into tConnectionId
  
  local tResult
  if tConnectionId is empty then
    put empty into tResult
  else
     
    local tQuery
    put the cREVDatabaseQuery["defaulttablequery"] of pObject into tQuery
    if tQuery is not empty then
      put revdb_querylist(tab,return,tConnectionId,tQuery) into tResult
    else
      local tType
      put revdb_dbtype(tConnectionId) into tType
      switch item 1 of tType
      case "mysql"
      case "oracle"
      case "valentina"
      case "postgresql"
      case "frontbase"
      case "sqlite"
        put revdb_tablenames(tConnectionId) into tResult
        break
      case "odbc"
        switch item 2 of tType
        case "mysql"
          put revdb_querylist(tab,return,tConnectionId,"SHOW tables;") into tResult
          break
        case "oracle"
          put revdb_querylist(tab,return,tConnectionId,"SELECT * from tab;") into tResult
          break
        case "postgresql"
          put revdb_querylist(tab,return,tConnectionId,"SELECT tablename FROM pg_tables WHERE tablename NOT LIKE 'pg%';") into tResult
          break
        case "access"
          put revdb_querylist(tab,return,tConnectionId,"SELECT name FROM MSysObjects WHERE type IN (1,6) AND name NOT LIKE 'MSys%';") into tResult
          break
        default
          put revdb_tablenames(tConnectionId) into tResult
          break
        end switch
        break
      end switch
    end if
  end if
  return tresult
end revTableNamesOfQuery

on revDBQueryGoToRecord  pObject,trecordnum
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put revCursorOfQuery(pObject) into tCursorId
   if tCursorId is empty then return empty
   if tCursorId is "cached" then 
      if trecordnum > 0 and trecordnum <= the cRevDatabaseQuery["recordcount"] of pObject then
         set the cREVDatabaseQuery["currentrecord"] of pObject to trecordnum
      end if
   else
      if cursorVerify(pObject, tCursorId) is not empty then
         switch revDatabaseTypeOfQuery(pObject)
            case "mysql"
            case "valentina"
            case "postgresql"
               get revdb_movefirst(tCursorId)
               subtract 1 from trecordnum
               repeat trecordnum
                  get revdb_movenext(tCursorId)
               end repeat
               break
               
               # OK-2010-02-15: Bug 7897 - revGoToRecordOfQuery not working with SQLite.
            case "sqlite"
               revMoveToRecord tCursorId, trecordnum
               break
         end switch
      end if
   end if
end revDBQueryGoToRecord

function revDBQueryMoveNext pObject
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put the cREVDatabaseQuery["cursorid"] of pObject into tCursorId
   if tCursorId is "cached" then
      if revDBQueryIsEOF(pObject) is false then
         set the cREVDatabaseQuery["currentrecord"] of pObject to the cREVDatabaseQuery["currentrecord"] of pObject + 1
      end if
   else
      if cursorVerify(pObject, tCursorId) is not empty then
         get revdb_movenext(tCursorId)
      end if
   end if
end revDBQueryMoveNext


function revDBQueryColumnIsNull pObject,pColumnNum
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put the cREVDatabaseQuery["cursorid"] of pObject into tCursorId
   if tCursorId is "cached" then
      local tdata
      revDBQueryColumnByNumber pObject,pColumnNum,tdata
      return tdata is "NULL"
   else 
      if cursorVerify(pObject, tCursorId) is not empty then
         return revdb_columnisnull(tCursorId,pColumnNum)
      end if
   end if
end revDBQueryColumnIsNull



function revDBQueryIsBOF pObject
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put the cREVDatabaseQuery["cursorid"] of pObject into tCursorId
   if tCursorId is "cached" then
      return the cREVDatabaseQuery["currentrecord"] of pObject is 1
   else 
      if cursorVerify(pObject, tCursorId) is not empty then
         return revdb_isbof(tCursorId)
      end if
   end if
end revDBQueryIsBOF

function revDBQueryIsEOF pObject
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put the cREVDatabaseQuery["cursorid"] of pObject into tCursorId
   if tCursorId is "cached" then
      return the cREVDatabaseQuery["currentrecord"] of pObject is \
      the cREVDatabaseQuery["recordcount"] of pObject
   else
      if cursorVerify(pObject, tCursorId) is not empty then
         return revdb_iseof(tCursorId)
      end if
   end if
end revDBQueryIsEOF

function revDBQueryMoveFirst pObject
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put the cREVDatabaseQuery["cursorid"] of pObject into tCursorId
   if tCursorId is "cached" then
      set the cREVDatabaseQuery["currentrecord"] of pObject to 1
   else
      if cursorVerify(pObject, tCursorId) is not empty then
         return revdb_movefirst(tCursorId)
      end if
   end if
end revDBQueryMoveFirst

function revDBQueryMoveLast pObject
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put the cREVDatabaseQuery["cursorid"] of pObject into tCursorId
   if tCursorId is "cached" then
      set the cREVDatabaseQuery["currentrecord"] of pObject to \
      the cREVDatabaseQuery["recordcount"] of pObject
   else
      if cursorVerify(pObject, tCursorId) is not empty then
         return revdb_movelast(tCursorId)
      end if
   end if
end revDBQueryMoveLast

function revDBQueryMovePrev pObject
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put the cREVDatabaseQuery["cursorid"] of pObject into tCursorId
   if tCursorId is "cached" then
      if revDBQueryIsBOF(pObject) is false then
         set the cREVDatabaseQuery["currentrecord"] of pObject to \
         the cREVDatabaseQuery["currentrecord"] of pObject - 1
      end if
   else
      if cursorVerify(pObject, tCursorId) is not empty then
         return revdb_moveprev(tCursorId)
      end if
   end if
end revDBQueryMovePrev

#engine: put column yy of record xx of query zzz
on revDBQueryColumnByName pObject, pWhichcolumn, @pHolder
   -- The holder parameter is necessary for storing binary data.
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put the cREVDatabaseQuery["cursorid"] of pObject into tCursorId
   
   if tCursorId is "cached" then
      local tCurrentRecord
      put the cREVDatabaseQuery["currentrecord"] of pObject into tCurrentRecord
      put the cREVDatabaseQueryCache[tcurrentrecord,pWhichcolumn]of pObject into pHolder
   else
      if cursorVerify(pObject, tCursorId) is not empty then
         if revDatabaseTypeOfQuery(pObject) is "Valentina" then
            put revdb_columnbyname(tCursorId,pWhichcolumn) into pHolder
         else
            get revdb_columnbyname(tCursorId,pWhichcolumn,"pHolder")
         end if
      end if
      if item 1 of pHolder is "revdberr" then put empty into pHolder
   end if
end revDBQueryColumnByName

on revDBQueryColumnByNumber pObject, pWhichcolumn, @pHolder
   put revQueryLongName(pObject) into pObject
   
   local tCursorId
   put the cREVDatabaseQuery["cursorid"] of pObject into tCursorId
   
   if tCursorId is "cached" then
      local tCurrentRecord
      put the cREVDatabaseQuery["currentrecord"] of pObject into tCurrentRecord
      
      local tColumnName
      put item pWhichcolumn of the cRevDatabaseQuery["columnnames"] of pObject into tColumnName
      put the cREVDatabaseQueryCache[tcurrentrecord,tcolumnname]of pObject into pHolder
   else
      local tHolder
      if cursorVerify(pObject, tCursorId) is not empty then
         if revDatabaseTypeOfQuery(pObject) is "Valentina" then
            put revdb_columnbynumber(tCursorId,pWhichcolumn) into tHolder
         else
            get revdb_columnbynumber(tCursorId,pWhichcolumn,"tHolder")
         end if
      end if
      
      if item 1 of tHolder is "revdberr" then 
         put empty into tHolder
      end if
      
      # OK-2007-08-03 : It seems this part was forgotten
      put tHolder into pHolder
   end if
end revDBQueryColumnByNumber

#engine: go to first record of query xxx
on revGoFirstRecordOfQuery pObject
  put revQueryLongName(pObject) into pObject
  get revDBQueryMoveFirst(pObject)
  revDisplayQuery pObject
end revGoFirstRecordOfQuery

#engine: go to record y of query xxx
on revGoToRecordOfQuery pObject,pNumber
  put revQueryLongName(pObject) into pObject
  revDBQueryGoToRecord pObject,pNumber
  revDisplayQuery pObject
end revGoToRecordOfQuery

#engine: go to last record of query xxx
on revGoLastRecordOfQuery pObject
  put revQueryLongName(pObject) into pObject
  get revDBQueryMoveLast(pObject)
  revDisplayQuery pObject
end revGoLastRecordOfQuery

#engine: go to next record of query xxx
on revGoNextRecordOfQuery pObject
  put revQueryLongName(pObject) into pObject
  get revDBQueryMoveNext(pObject)
  revDisplayQuery pObject
end revGoNextRecordOfQuery

#engine: go to previous record of query xxx
on revGoPrevRecordOfQuery pObject
  put revQueryLongName(pObject) into pObject
  get revDBQueryMovePrev(pObject)
  revDisplayQuery pObject
end revGoPrevRecordOfQuery

# Parameters
#   pStack : reference to the stack to list objects in
# Returns
#   A return delimited list of references to query objects on the stack (long names). Returns empty if none are found
# Description
#  pStack added OK-2007-10-23. If pStack is empty then the defaultStack is used (if not an IDE stack). Failing this, the topstack
function revQueryObjects pStack
  local tObject
  
  if pStack is not empty then
    put pStack into tObject
  else
    if char 2 to 4 of word 2 of the defaultStack is "rev" then
      put the long name of the topStack into tObject
    else
      put the long name of the defaultStack into tObject
    end if
  end if
  
  local tObjectList
  repeat with i = 1 to number of backgrounds of tObject
    if the cREVGeneral["databasequeryobject"] of background i of tobject is true then
      put the long name of background i of tobject & return after tObjectList
    end if
  end repeat
  delete last char of tObjectList
  sort tObjectList
  return tObjectList
end revQueryObjects

command revDBQueryDisplayTable pObject, pDataLinkedObject
  local tResult
  put empty into tResult
  put revQueryLongName(pObject) into pObject
  
  local tConnectionId
  put revConnectionOfQuery(pObject) into tConnectionId
  if tConnectionId is empty then 
    exit revDBQueryDisplayTable
  end if
  
  -- first determine the column numbers to display
  local tColumnsToDisplay
  put the cREVDatabase["linkcolumn"] of pDataLinkedObject into tColumnsToDisplay
  
  if tColumnsToDisplay is not "Show all" then
    local tColumnNames
    put revColumnNamesOfQuery(pObject) into tColumnNames
    
    # OK-2007-08-03 : Fixed problem where it was not possible to change the order in which columns are displayed
    # Also restructed loop for greater speed.
    local tColumnNumbers
    repeat for each item tColumnName in tColumnsToDisplay
      put itemOffset(tColumnName, tColumnNames) & comma after tColumnNumbers
    end repeat
    delete the last char of tColumnNumbers
    
  end if
   
  local tRecordCount
  put revRecordCountOfQuery(pObject) into tRecordCount
  
  local columndata
  if tRecordCount <> 0 then
    local tOldCurrentRecord
    put revCurrentRecordOfQuery(pObject) into tOldCurrentRecord
    get revDBQueryMoveFirst(pObject)
    if tColumnsToDisplay is "Show all" then
      local tColumnCount
      put revColumnCountOfQuery(pObject) into tColumnCount
      
      repeat tRecordCount times
        repeat with i = 1 to tColumnCount
          if revDBQueryColumnIsNull(pObject,i) is true then
            put "NULL" & tab after tResult
          else
            revDBQueryColumnByNumber pObject, i, columndata
            put columndata & tab after tResult
          end if
        end repeat
        put return after tResult
        get revDBQueryMoveNext(pObject)
      end repeat
    else
      repeat tRecordCount times
        repeat for each item i in tColumnNumbers
          if revDBQueryColumnIsNull(pObject,i) is true then
            put "NULL" & tab after tResult
          else
            revDBQueryColumnByNumber pObject, i, columndata
            put columndata & tab after tResult
          end if
        end repeat
        put return after tResult
        get revDBQueryMoveNext(pObject)
      end repeat
    end if
    delete char -1 of tResult
    
    if tOldCurrentRecord > 0 then 
      revDBQueryGoToRecord pObject, tOldCurrentRecord
    end if
  end if
  
  -- now that we have the data, display it in the table field
  set the cREVDatabase["returnedview"] of pDataLinkedObject to tResult
  
  if the cREVGeneral["table"] of pDataLinkedObject is true then
    set the cREVTable["currentview"] of pDataLinkedObject to the cREVDatabase["returnedview"] of pDataLinkedObject
    
    revDisplayFormattedData pDataLinkedObject
  else
    set the text of pDataLinkedObject to tResult
  end if
end revDBQueryDisplayTable

on revUpdateDatabase pDataLinkedObject, pXCell, pYCell, pValue
  if (the cREVGeneral["database"] of pDataLinkedObject is not true) or (the cREVDatabase["updateafterediting"] of pDataLinkedObject is not true) then
    exit revUpdateDatabase
  end if
   
  local tObject
  put the cREVDataBase["queryobject"] of pDataLinkedObject into tObject
  put revQueryLongName(tObject) into tObject
   
  local tColumnsToDisplay
  put the cREVDatabase["linkcolumn"] of pDataLinkedObject into tColumnsToDisplay
  if tColumnsToDisplay is "Show all" then
     
    local tColumnNames
    put revColumnNamesOfQuery(tObject) into tColumnNames
    replace comma with return in tColumnNames
     
    local tColumnName
    put line pXCell of tColumnNames into tColumnName
  else
    replace comma with return in tColumnsToDisplay
    put line pXCell of tColumnsToDisplay into tColumnName
  end if
   
  local tKeysValuesList
  put revGetKeysValuesOfQuery(tObject, pYCell) into tKeysValuesList
   
  local tKeyValues
  put tKeysValuesList into tKeyValues
  
  revWriteCellData tObject, tColumnName, pValue, tKeyValues, "text"
end revUpdateDatabase

# Parameters
#   pObject : reference to a query object to act on
# Returns
#   true if pObject contains valid connection settings, i.e. all the correct properties are set in order to
#   make a connection to that database type. Does not check that the connection will actually work, just that the
#   required values are non-empty.
function revValidConnection pObject
  local tResult
  put false into tResult
  put revQueryLongName(pObject) into pObject
  if (the cREVDatabaseQuery["dbtype"] of pObject is not empty) then
    switch (the cREVDatabaseQuery["dbtype"] of pObject)
    case "Valentina"
      if (the cREVDatabaseQuery["dbdatabase"] of pObject is not empty) then
        put true into tResult
      end if
      break
    case "ODBC"
      if (the cREVDatabaseQuery["dbhost"] of pObject is not empty) then
        put true into tResult
      end if
      break
    case "MySQL"
    case "Postgresql"
    case "Oracle"
      if (the cREVDatabaseQuery["dbdatabase"] of pObject is not empty) and (the cREVDatabaseQuery["dbhost"] of pObject is not empty) then
        put true into tResult
      end if
      break
    end switch
  end if
  return tResult
end revValidConnection

# Parameters
#   pObject : the long id of a database query object
# Returns
#   Empty if pObject does not exist, the short name of pObject otherwise
function revQueryShortName pObject
  if there is no pObject then
    return empty
  end if
  return the short name of pObject
end revQueryShortName


# OK-2007-10-24 : Added ability to specify a stack other than the default stack.
# Parameters
#   pName : the name of a query object. May be either a short or long name
#   pStack : long id of the stack that the object is contained in. Optional.
# Returns
#   The long name, in the form of a reference to the query object that pName refers to. Returns empty if the object cannot be found.
#   If a stack is specified in the pStack parameter then the query is assumed to be in this stack, otherwise it is assumed to be in 
#   the default stack.
function revQueryLongName pName, pStack
  if pName is empty then
    return empty
  end if
  
  -- quick exit if it is a fully qualified query name
  local tExistsFlag
  if (word 1 of pName is among the items of "background,bkgnd,bg") and (word -3 to -2 of pName is "of stack") then
    -- just in case it's not a valid background object reference
    try
      put (there is a pName) into tExistsFlag
    catch tError
      put false into tExistsFlag
    end try
    if tExistsFlag and the cREVGeneral["databasequeryobject"] of pName
    then return pName
  end if
  
  -- otherwise make sure we have a correct query name
  if (word 1 of pName is among the items of "background,bkgnd,bg") then
    put word 2 of pName into pName
    if the first char of pName is quote then delete the first char of pName
    if the last char of pName is quote then delete the last char of pName
  end if
  
  local tQueryLongNames
  put revQueryObjects(pStack) into tQueryLongNames
  
  local tQueryShortNames
  repeat for each line i in tQueryLongNames
    put revQueryShortName(i) & cr after tQueryShortNames
  end repeat
  delete last char of tQueryShortNames
  set the wholeMatches to true
  
  local tNumber
  put lineOffset(pName, tQueryShortNames) into tNumber
  
  local tWhichLongName
  put line tNumber of tQueryLongNames into tWhichLongName
  return tWhichLongName
end revQueryLongName

# OK-2008-01-25

# Stores a mapping from query object long name to a flag specifying whether the object is in an error state or not.
local sErrorStates

# Parameters
#   pObject : reference to a query object to act on
# Returns
#   true or false
# Description
#   Returns the transient error state of the query. A query enters an error state when an exception is thrown
#   by the database library for the query. The state can be reset using revResetQueryError. The error state is 
#   implemented by a virtual setProp and getProp in the query object script.
function revGetQueryError pObject
  local tObject
  put revQueryLongName(pObject) into tObject
  
  local tState
  put sErrorStates[tObject] into tState
  if tState is empty then
    put "false" into tState
  end if
  
  return tState
end revGetQueryError

# Parameters
#   pObject : reference to a query object to act on
# Description
#   Resets a query object's transient error state. Use this to allow a query to start working again even though it has thrown an error
command revUnsetQueryError pObject
  local tObject
  put revQueryLongName(pObject) into tObject
   
  put false into sErrorStates[tObject]
end revUnsetQueryError

# Parameters
#   pObject : reference to a query object to act on
# Description
#   Puts a query into an error state.
command revSetQueryError pObject
  local tObject
  put revQueryLongName(pObject) into tObject
  
  put true into sErrorStates[tObject]
end revSetQueryError

# Parameters
#   pObject : reference to a query object to act on
# Description
#   Called whenever an error is detected in a query. If the query is already known to be in an inconsistent state, or has an error
#   set, then does nothing. Otherwise, sets the query's error state to true and sends a "revDatabaseError" message to the object.
private command revHandleQueryError pObject, pType, pError
  local tObject
  put revQueryLongName(pObject) into tObject
  
  if revGetQueryError(tObject) or revGetQueryProperty(tObject, "consistent") is "false" then
    exit revHandleQueryError
  end if
  
  revSetQueryError tObject
  send "revDatabaseError pType, pError, tObject" to tObject
end revHandleQueryError

# OK-2008-07-07 : Bug 6679 - This command is called when the current record is about to be changed in response to a mouseUp
# action on a database linked button. It ensures that any changes made in the currently focused field are updated into the database
# before the action is performed. We can't rely on closeField to do this because the linked button may not take focus away from the field.
private command updateFromFocusedObject
   local tFocusedObject
   put the long id of the focusedObject into tFocusedObject
   if the cREVGeneral["database"] of tFocusedObject is true and the cREVDatabase["updateafterediting"] of tFocusedObject is true then
      revUpdateQueryObject tFocusedObject
   end if 
end updateFromFocusedObject

on mouseUp
   local tWhichObject
   put the long id of target into tWhichObject
   if word 1 of tWhichObject is "button" and the cREVGeneral["database"] of tWhichObject is true then
      
      local tObject
      put the cREVDatabase["queryobject"] of tWhichObject into tObject
      put revQueryLongName(tObject) into tObject
      if exists(tObject) and the cREVGeneral["databasequeryobject"] of tObject is true then
         switch the style of tWhichObject
            case "menu"
            case "checkbox"
               revUpdateQueryObject tWhichObject
            break
            default
               switch the cREVDatabase["clickaction"] of tWhichObject
                  case "Refresh Current Record"
                     answer "not implemented yet"
                  break
                     
                     # OK-2008-07-07 : Bug 6679 - Any actions which move away from the previous record or otherwise refresh the controls
                     # should make sure the database is updated according to the previous values of the controls first. 
                  case "Move To First Record"
                     updateFromFocusedObject
                     revGoFirstRecordOfQuery tObject
                  break
                  case "Move To Last Record"
                     updateFromFocusedObject
                     revGoLastRecordOfQuery tObject
                  break
                  case "Move To Next Record"
                     updateFromFocusedObject
                     revGoNextRecordOfQuery tObject
                  break
                  case "Move To Previous Record"
                     updateFromFocusedObject
                     revGoPrevRecordOfQuery tObject
                  break
                  case "Refresh SQL"
                     updateFromFocusedObject
                     revRefreshQuery tObject
                  break
                  case "Delete Record"
                     answer "not implemented yet"
                  break
                  case "Update Record"
                     revUpdateQuery tObject
                  break
                  case "Insert Record"
                     answer "not implemented yet"
                  break
               end switch
            break
         end switch
      end if
   else if word 1 of tWhichObject is "group" and the cREVGeneral["database"] of tWhichObject is true then
      revUpdateQueryObject tWhichObject
   end if
   pass mouseup
end mouseUp
  

on closeCard
  repeat with i = 1 to the number of backgrounds
    
    local tObject
    put the long id of background i into tObject
    if there is not a tObject then next repeat
    
    if the cRevGeneral["databasequeryobject"] of tObject is true and the cREVDatabaseQuery["closewhen"] of tObject is "card closes" then
      # OK-2007-10-11 : Removed these two lines as they seem to be redundant
      -- put revQueryShortName(tObject) into tObject
      -- put revQueryLongName(tObject) into tObject
      revDisconnectQuery tObject
    end if
  end repeat
  pass closeCard
end closeCard

on closeStack
  repeat with i = 1 to the number of backgrounds
    
    local tObject
    put the long id of background i into tObject
    if there is not a tObject then next repeat
    
    # OK-2008-03-28 : Bug 6245. Ensure that revUnsetQueryError is not called unless the object is a database query object,
    # as this will significantly slow down stacks with large numbers of backgrounds.
    if the cRevGeneral["databasequeryobject"] of tObject is true then
      # OK-2008-01-28 : Remove error states from any query objects when the stack is closed
      revUnsetQueryError tObject
       
      if the cREVDatabaseQuery["closewhen"] of tObject is "stack closes" then
        # OK-2007-10-11 : Removed this line as it seemed to be redundant
        -- put revQueryShortName(tObject) into tObject
        put revQueryLongName(tObject) into tObject
        revDisconnectQuery tObject
      end if
    end if
  end repeat
  pass closeStack
end closeStack

on preOpenCard
  repeat with i = 1 to the number of backgrounds
    
    local tObject
    put the long id of background i into tObject
    
    if there is not a tObject then
      next repeat
    end if
    if the cRevGeneral["databasequeryobject"] of tObject is true and the cREVDatabaseQuery["openwhen"] of tObject is "card opens" then
      # OK-2007-10-11 : Removed these two lines as they seem to be redundant
      -- put revQueryShortName(tObject) into tObject
      -- put revQueryLongName(tObject) into tObject
      revConnectQuery tObject
      revRefreshQuery tObject
    end if
  end repeat
  pass preOpenCard
end preOpenCard

on preOpenStack
  repeat with i = 1 to the number of backgrounds
    
    local tObject
    put the long id of background i into tObject
    if there is not a tObject then next repeat
    if the cRevGeneral["databasequeryobject"] of tObject is true and the cREVDatabaseQuery["openwhen"] of tObject is "stack opens" then
      # OK-2007-10-11 : Removed these two lines as they seem to be redundant
      -- put revQueryShortName(tObject) into tObject
      -- put revQueryLongName(tObject) into tObject
      revConnectQuery tObject
      revRefreshQuery tObject
    end if
  end repeat
  pass preOpenStack
end preOpenStack

on closeField
  local tDataLinkedObject
  put the long id of the target into tDataLinkedObject
  revUpdateQueryObject tDataLinkedObject
  pass closeField
end closeField

private command log pMessage
  --put pMessage & return after msg
end log


