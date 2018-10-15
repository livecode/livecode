---
version: 9.0.0-dp-7
---
# LiveCode Builder Standard Library
## Foreign function interface

* The machine types Bool, SIntSize, UIntSize, SIntPtr, UIntPtr, SInt8, UInt8,
  SInt16, UInt16, SInt32, UInt32, SInt64 and UInt64 have been added. These all
  map to their corresponding foreign counterparts.

* The C types CBool, CChar, CSChar, CUChar, CSShort, CUShort, CSInt,
  CUInt, CSLong, CULong, CSLongLong and CULongLong have been added.
  These all map to their corresponding C counterparts.

* The Java integer primtive types JBoolean, JByte, JShort, JInt, JLong,
  JFloat and JDouble have been added. These map to int8_t,
  int16_t, int32_t, int64_t, float and double.
