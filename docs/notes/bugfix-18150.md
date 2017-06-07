# The ability to use date format strings has been added to the date function and convert command

Use the date function with a format string parameter to obtain a date string in a format that is
not currently supported. For example:

```
put date("%Y%m%dT%H%M%S%z") into tDate -- 20170507T205054+1000
```

Use the convert function with a format string to convert from, to or between formats that are
not currently supported with syntax. For example:

```    
put "20170507T205054+1000" into tDate
convert tDate from "%Y%m%dT%H%M%S%z" to dateItems
-- tDate is now 2017,5,7,20,50,54,1
```
