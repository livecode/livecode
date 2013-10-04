#line 1 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
















#line 1 "c:\\github\\livecode-runrev\\engine\\src\\prefix.h"


















#line 1 "c:\\github\\livecode-runrev\\engine\\src\\globdefs.h"



















































#line 53 "c:\\github\\livecode-runrev\\engine\\src\\globdefs.h"









#line 63 "c:\\github\\livecode-runrev\\engine\\src\\globdefs.h"





#line 69 "c:\\github\\livecode-runrev\\engine\\src\\globdefs.h"

#line 1 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"






























































































#line 96 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"













#line 110 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"

#line 112 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"



































#line 148 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"














#line 163 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"








#line 1 "c:\\github\\livecode-runrev\\engine\\src\\typedefs.h"

















































typedef unsigned char   uint1;
typedef          char   int1;
typedef unsigned short  uint2;
typedef          short  int2;
typedef unsigned int    uint4;
typedef          int    int4;
typedef float           real4;
typedef double          real8;

























typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;



	
		typedef unsigned long long int uint64_t;
	

#line 97 "c:\\github\\livecode-runrev\\engine\\src\\typedefs.h"
#line 98 "c:\\github\\livecode-runrev\\engine\\src\\typedefs.h"

	
		typedef signed long long int int64_t;
	

#line 104 "c:\\github\\livecode-runrev\\engine\\src\\typedefs.h"
#line 105 "c:\\github\\livecode-runrev\\engine\\src\\typedefs.h"

typedef float float32_t;
typedef double float64_t;

typedef float real32_t;
typedef double real64_t;

typedef uint32_t uindex_t;
typedef int32_t index_t;





























	
	


		typedef uint32_t uintptr_t;
	#line 149 "c:\\github\\livecode-runrev\\engine\\src\\typedefs.h"
#line 150 "c:\\github\\livecode-runrev\\engine\\src\\typedefs.h"


	
	


		typedef int32_t intptr_t;
	#line 158 "c:\\github\\livecode-runrev\\engine\\src\\typedefs.h"
#line 159 "c:\\github\\livecode-runrev\\engine\\src\\typedefs.h"





#line 165 "c:\\github\\livecode-runrev\\engine\\src\\typedefs.h"



#line 169 "c:\\github\\livecode-runrev\\engine\\src\\typedefs.h"



typedef unsigned char Boolean;



#line 177 "c:\\github\\livecode-runrev\\engine\\src\\typedefs.h"





#line 183 "c:\\github\\livecode-runrev\\engine\\src\\typedefs.h"
#line 172 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"






class MCString;

#line 1 "c:\\github\\livecode-runrev\\engine\\src\\name.h"





















typedef struct MCName *MCNameRef;

typedef uint32_t MCCompareOptions;

enum
{
	kMCCompareExact,
	kMCCompareCaseless
};

bool MCNameCreateWithStaticCString(const char *cstring, MCNameRef& r_name);

bool MCNameCreateWithCString(const char *cstring, MCNameRef& r_name);
bool MCNameCreateWithOldString(const MCString& string, MCNameRef& r_name);
bool MCNameCreateWithChars(const char *chars, uindex_t char_count, MCNameRef& r_name);

void MCNameDelete(MCNameRef name);

bool MCNameClone(MCNameRef name, MCNameRef& r_new_name);

uintptr_t MCNameGetCaselessSearchKey(MCNameRef name);

const char *MCNameGetCString(MCNameRef name);
MCString MCNameGetOldString(MCNameRef name);

char MCNameGetCharAtIndex(MCNameRef name, uindex_t at);

bool MCNameIsEmpty(MCNameRef name);

bool MCNameIsEqualTo(MCNameRef left, MCNameRef right, MCCompareOptions options);
bool MCNameIsEqualToCString(MCNameRef left, const char *cstring, MCCompareOptions options);
bool MCNameIsEqualToOldString(MCNameRef left, const MCString& string, MCCompareOptions options);

MCNameRef MCNameLookupWithCString(const char *cstring, MCCompareOptions options);
MCNameRef MCNameLookupWithOldString(const MCString& string, MCCompareOptions options);

bool MCNameInitialize(void);
void MCNameFinalize(void);

extern MCNameRef kMCEmptyName;



class MCAutoNameRef
{
public:
	MCAutoNameRef(void)
	{
		m_name = 0;
	}

	~MCAutoNameRef(void)
	{
		MCNameDelete(m_name);
	}

	bool CreateWithCString(const char *p_name)
	{
		return MCNameCreateWithCString(p_name, m_name);
	}

	bool CreateWithOldString(const MCString& p_name)
	{
		return MCNameCreateWithOldString(p_name, m_name);
	}

	bool Clone(MCNameRef p_name)
	{
		return MCNameClone(p_name, m_name);
	}

	MCNameRef Take(void)
	{
		MCNameRef t_name;
		t_name = m_name;
		m_name = 0;
		return t_name;
	}

	operator MCNameRef& (void)
	{
		return m_name;
	}

	operator MCNameRef (void) const
	{
		return m_name;
	}

private:
	MCNameRef m_name;
};



#line 118 "c:\\github\\livecode-runrev\\engine\\src\\name.h"
#line 181 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\rawarray.h"





















template<class T> class MCRawArray
{
public:
	MCRawArray(void);
	~MCRawArray(void);

	uindex_t Count(void) const;
	void Compact(void);
	void Clear(void);

	T operator [] (uindex_t index) const;
	T& operator [] (uindex_t index);

	bool Append(T value);
	bool AppendSeq(const T* values, uindex_t value_count);
	bool Prepend(T value);
	bool PrependSeq(const T* values, uindex_t value_count);
	bool Replace(uindex_t at, T value);
	bool ReplaceSeq(uindex_t at, uindex_t count, const T* values, uindex_t value_count);
	bool Insert(uindex_t at, T value);
	bool InsertSeq(uindex_t at, const T* values, uindex_t value_count);
	void Remove(uindex_t at);
	void RemoveSeq(uindex_t at, uindex_t count);

private:
	T *m_elements;
	uindex_t m_count;
};

template<class T> inline MCRawArray<T>::MCRawArray(void)
{
	m_elements = 0;
	m_count = 0;
}

template<class T> inline MCRawArray<T>::~MCRawArray(void)
{
	MCMemoryDeleteArray(m_elements);
}

template<class T> inline uindex_t MCRawArray<T>::Count(void) const
{
	return m_count;
}

template<class T> inline T MCRawArray<T>::operator [] (uindex_t p_index) const
{
	return m_elements[p_index];
}

template<class T> inline T& MCRawArray<T>::operator [] (uindex_t p_index)
{
	return m_elements[p_index];
}

template<class T> inline bool MCRawArray<T>::Append(T p_value)
{
	if (!MCMemoryResizeArray(m_count + 1, m_elements, m_count))
		return false;
	m_elements[m_count - 1] = p_value;
	return true;
}

template<class T> inline bool MCRawArray<T>::Insert(uindex_t p_at, T p_value)
{
	if (!MCMemoryResizeArray(m_count + 1, m_elements, m_count))
		return false;
	MCMemoryMove(m_elements + p_at + 1, m_elements + p_at, sizeof(T) * (m_count - p_at - 1));
	m_elements[p_at] = p_value;
	return true;
}

template<class T> inline void MCRawArray<T>::Remove(uindex_t p_at)
{
	MCMemoryMove(m_elements + p_at, m_elements + p_at + 1, sizeof(T) * (m_count - p_at - 1));
	m_count -= 1;
}



#line 103 "c:\\github\\livecode-runrev\\engine\\src\\rawarray.h"
#line 182 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"










typedef struct __MCSysModuleHandle *MCSysModuleHandle;
typedef struct __MCSysBitmapHandle *MCSysBitmapHandle;
typedef struct __MCSysWindowHandle *MCSysWindowHandle;
typedef struct __MCSysFontHandle *MCSysFontHandle;
typedef struct __MCSysContextHandle *MCSysContextHandle;

typedef void *MCColorTransformRef;
typedef struct MCCursor *MCCursorRef;
















#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdarg.h"















#pragma once
#line 18 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdarg.h"






#line 25 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdarg.h"


#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"













#pragma once
#line 16 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"






#line 23 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"









#pragma pack(push,8)
#line 34 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"


extern "C" {
#line 38 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"







#line 46 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"
#line 47 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"





typedef __w64 unsigned int   uintptr_t;
#line 54 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"

#line 56 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"





typedef char *  va_list;
#line 63 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"

#line 65 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"





#line 71 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"











#line 83 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"


#line 86 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"













#line 100 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"












































#line 145 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"


}
#line 149 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"


#pragma pack(pop)
#line 153 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"

#line 155 "c:\\program files\\microsoft visual studio 8\\vc\\include\\vadefs.h"
#line 28 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdarg.h"





#line 34 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdarg.h"
#line 217 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"
#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\errno.h"
















#pragma once
#line 19 "c:\\program files\\microsoft visual studio 8\\vc\\include\\errno.h"




#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"














 





#line 22 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 23 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"














#line 38 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"



#line 42 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"











    

#line 56 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 58 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\sal.h"

























































































































































































































#pragma once






#line 226 "c:\\program files\\microsoft visual studio 8\\vc\\include\\sal.h"
extern "C" {




#line 232 "c:\\program files\\microsoft visual studio 8\\vc\\include\\sal.h"



























































































































































































































#line 452 "c:\\program files\\microsoft visual studio 8\\vc\\include\\sal.h"
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
#line 486 "c:\\program files\\microsoft visual studio 8\\vc\\include\\sal.h"



































































































































































































































    
    
#line 716 "c:\\program files\\microsoft visual studio 8\\vc\\include\\sal.h"






#line 723 "c:\\program files\\microsoft visual studio 8\\vc\\include\\sal.h"
#line 724 "c:\\program files\\microsoft visual studio 8\\vc\\include\\sal.h"


}
#line 728 "c:\\program files\\microsoft visual studio 8\\vc\\include\\sal.h"



#line 60 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"























































































#pragma pack(push,8)
#line 149 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"




extern "C" {
#line 155 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"





#line 161 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"




#line 166 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"








#line 175 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"






#line 182 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 184 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 185 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"






#line 192 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 194 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 195 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"













#line 209 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 210 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"





#line 216 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"







#line 224 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 226 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 228 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"







#line 236 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 237 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"




#line 242 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 244 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 245 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"




#line 250 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 252 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 253 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"


 

#line 258 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
  
 #line 260 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 261 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"










#line 272 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 273 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"






#line 280 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 281 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

















#line 299 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"




#line 304 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"





#line 310 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"







#line 318 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 319 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"







#line 327 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 328 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"





#line 334 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"


#line 337 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 339 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 340 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 341 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"











#line 353 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 355 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 356 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 357 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"












#line 370 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 371 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"







#line 379 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 381 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 382 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"



 
  
 



#line 392 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 393 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"


 
  
  
 



#line 403 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 404 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"


 
  
   
  

#line 412 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
 



#line 417 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 418 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"







#line 426 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 427 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"





typedef __w64 unsigned int   size_t;
#line 434 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 436 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"



typedef size_t rsize_t;

#line 442 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 443 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"





typedef __w64 int            intptr_t;
#line 450 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 452 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"














typedef __w64 int            ptrdiff_t;
#line 468 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 470 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"







typedef unsigned short wint_t;
typedef unsigned short wctype_t;

#line 481 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"


















#line 500 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 501 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"





__declspec(deprecated("This name was supported during some Whidbey pre-releases. Instead, use the standard name errno_t.")) typedef int errcode;


#line 510 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

typedef int errno_t;
#line 513 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"


typedef __w64 long __time32_t;   

#line 518 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"



typedef __int64 __time64_t;     
#line 523 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 525 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"





typedef __time64_t time_t;      
#line 532 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 534 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"







#line 542 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 543 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"




#line 548 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 550 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 551 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"




#line 556 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 558 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 559 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"






#line 566 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 567 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"




#line 572 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"



#line 576 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"











#line 588 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"




#line 593 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"




#line 598 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 600 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 601 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"







#line 609 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"


































































































































































#line 772 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 773 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"







































































































































































































































































































































































































































































































































































































































































































































































































































































#line 1613 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"






















































































#line 1700 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"
#line 1701 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

struct threadlocaleinfostruct;
struct threadmbcinfostruct;
typedef struct threadlocaleinfostruct * pthreadlocinfo;
typedef struct threadmbcinfostruct * pthreadmbcinfo;
struct __lc_time_data;

typedef struct localeinfo_struct
{
    pthreadlocinfo locinfo;
    pthreadmbcinfo mbcinfo;
} _locale_tstruct, *_locale_t;


typedef struct tagLC_ID {
        unsigned short wLanguage;
        unsigned short wCountry;
        unsigned short wCodePage;
} LC_ID, *LPLC_ID;

#line 1722 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"


typedef struct threadlocaleinfostruct {
        int refcount;
        unsigned int lc_codepage;
        unsigned int lc_collate_cp;
        unsigned long lc_handle[6]; 
        LC_ID lc_id[6];
        struct {
            char *locale;
            wchar_t *wlocale;
            int *refcount;
            int *wrefcount;
        } lc_category[6];
        int lc_clike;
        int mb_cur_max;
        int * lconv_intl_refcount;
        int * lconv_num_refcount;
        int * lconv_mon_refcount;
        struct lconv * lconv;
        int * ctype1_refcount;
        unsigned short * ctype1;
        const unsigned short * pctype;
        const unsigned char * pclmap;
        const unsigned char * pcumap;
        struct __lc_time_data * lc_time_curr;
} threadlocinfo;

#line 1751 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"


}
#line 1755 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"



#line 1759 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 1761 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"



#line 1765 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 1767 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"



#line 1771 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 1773 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"






#line 1780 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"



#line 1784 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"


#pragma pack(pop)
#line 1788 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 1790 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"

#line 24 "c:\\program files\\microsoft visual studio 8\\vc\\include\\errno.h"


extern "C" {
#line 28 "c:\\program files\\microsoft visual studio 8\\vc\\include\\errno.h"





 extern int * __cdecl _errno(void);


errno_t __cdecl _set_errno(     int _Value);
errno_t __cdecl _get_errno(     int * _Value);
#line 39 "c:\\program files\\microsoft visual studio 8\\vc\\include\\errno.h"
















































#line 88 "c:\\program files\\microsoft visual studio 8\\vc\\include\\errno.h"
#line 89 "c:\\program files\\microsoft visual studio 8\\vc\\include\\errno.h"








}
#line 99 "c:\\program files\\microsoft visual studio 8\\vc\\include\\errno.h"

#line 101 "c:\\program files\\microsoft visual studio 8\\vc\\include\\errno.h"
#line 218 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"
#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"














#pragma once
#line 17 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"




#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"














 















































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 22 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"


extern "C" {
#line 26 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"



#line 30 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"










#line 41 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"
#line 42 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"

 const unsigned short * __cdecl __pctype_func(void);

 extern const unsigned short *_pctype;


#line 49 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"
#line 50 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"
#line 51 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"





 extern const unsigned short _wctype[];
#line 58 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"

 const wctype_t * __cdecl __pwctype_func(void);

 extern const wctype_t *_pwctype;


#line 65 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"
#line 66 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"
#line 67 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"


#line 70 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"










                                













  int __cdecl _isctype(     int _C,      int _Type);
  int __cdecl _isctype_l(     int _C,      int _Type,        _locale_t _Locale);
   int __cdecl isalpha(     int _C);
  int __cdecl _isalpha_l(     int _C,        _locale_t _Locale);
   int __cdecl isupper(     int _C);
  int __cdecl _isupper_l(     int _C,        _locale_t _Locale);
   int __cdecl islower(     int _C);
  int __cdecl _islower_l(     int _C,        _locale_t _Locale);
   int __cdecl isdigit(     int _C);
  int __cdecl _isdigit_l(     int _C,        _locale_t _Locale);
  int __cdecl isxdigit(     int _C);
  int __cdecl _isxdigit_l(     int _C,        _locale_t _Locale);
   int __cdecl isspace(     int _C);
  int __cdecl _isspace_l(     int _C,        _locale_t _Locale);
  int __cdecl ispunct(     int _C);
  int __cdecl _ispunct_l(     int _C,        _locale_t _Locale);
   int __cdecl isalnum(     int _C);
  int __cdecl _isalnum_l(     int _C,        _locale_t _Locale);
  int __cdecl isprint(     int _C);
  int __cdecl _isprint_l(     int _C,        _locale_t _Locale);
  int __cdecl isgraph(     int _C);
  int __cdecl _isgraph_l(     int _C,        _locale_t _Locale);
  int __cdecl iscntrl(     int _C);
  int __cdecl _iscntrl_l(     int _C,        _locale_t _Locale);
   int __cdecl toupper(     int _C);
   int __cdecl tolower(     int _C);
   int __cdecl _tolower(     int _C);
  int __cdecl _tolower_l(     int _C,        _locale_t _Locale);
   int __cdecl _toupper(     int _C);
  int __cdecl _toupper_l(     int _C,        _locale_t _Locale);
  int __cdecl __isascii(     int _C);
  int __cdecl __toascii(     int _C);
  int __cdecl __iscsymf(     int _C);
  int __cdecl __iscsym(     int _C);

#line 130 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"







  int __cdecl iswalpha(     wint_t _C);
  int __cdecl _iswalpha_l(     wint_t _C,        _locale_t _Locale);
  int __cdecl iswupper(     wint_t _C);
  int __cdecl _iswupper_l(     wint_t _C,        _locale_t _Locale);
  int __cdecl iswlower(     wint_t _C);
  int __cdecl _iswlower_l(     wint_t _C,        _locale_t _Locale);
  int __cdecl iswdigit(     wint_t _C);
  int __cdecl _iswdigit_l(     wint_t _C,        _locale_t _Locale);
  int __cdecl iswxdigit(     wint_t _C);
  int __cdecl _iswxdigit_l(     wint_t _C,        _locale_t _Locale);
  int __cdecl iswspace(     wint_t _C);
  int __cdecl _iswspace_l(     wint_t _C,        _locale_t _Locale);
  int __cdecl iswpunct(     wint_t _C);
  int __cdecl _iswpunct_l(     wint_t _C,        _locale_t _Locale);
  int __cdecl iswalnum(     wint_t _C);
  int __cdecl _iswalnum_l(     wint_t _C,        _locale_t _Locale);
  int __cdecl iswprint(     wint_t _C);
  int __cdecl _iswprint_l(     wint_t _C,        _locale_t _Locale);
  int __cdecl iswgraph(     wint_t _C);
  int __cdecl _iswgraph_l(     wint_t _C,        _locale_t _Locale);
  int __cdecl iswcntrl(     wint_t _C);
  int __cdecl _iswcntrl_l(     wint_t _C,        _locale_t _Locale);
  int __cdecl iswascii(     wint_t _C);
  int __cdecl isleadbyte(     int _C);
  int __cdecl _isleadbyte_l(     int _C,        _locale_t _Locale);

  wint_t __cdecl towupper(     wint_t _C);
  wint_t __cdecl _towupper_l(     wint_t _C,        _locale_t _Locale);
  wint_t __cdecl towlower(     wint_t _C);
  wint_t __cdecl _towlower_l(     wint_t _C,        _locale_t _Locale); 
  int __cdecl iswctype(     wint_t _C,      wctype_t _Type);
  int __cdecl _iswctype_l(     wint_t _C,      wctype_t _Type,        _locale_t _Locale);

  int __cdecl __iswcsymf(     wint_t _C);
  int __cdecl _iswcsymf_l(     wint_t _C,        _locale_t _Locale);
  int __cdecl __iswcsym(     wint_t _C);
  int __cdecl _iswcsym_l(     wint_t _C,        _locale_t _Locale);

__declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "iswctype" "instead. See online help for details."))  int __cdecl is_wctype(     wint_t _C,      wctype_t _Type);


#line 179 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"















#line 195 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"


 extern int __mb_cur_max;


#line 201 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"
 int __cdecl ___mb_cur_max_func(void);
 int __cdecl ___mb_cur_max_l_func(_locale_t);
#line 204 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"





 int __cdecl _chvalidator(     int _Ch,      int _Mask);



#line 214 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"











































#line 258 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"
#line 259 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"


int __cdecl _chvalidator_l(       _locale_t,      int _Ch,      int _Mask);



#line 266 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"


















































#line 317 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"


























#line 344 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"

#line 346 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"













#line 360 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"
















#line 377 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"

#line 379 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"


}
#line 383 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"

#line 385 "c:\\program files\\microsoft visual studio 8\\vc\\include\\ctype.h"
#line 219 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"
#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"















#pragma once
#line 18 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"




#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"














 















































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 23 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"


extern "C" {
#line 27 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"




#line 32 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
















 void *  __cdecl _memccpy(        void * _Dst,      const void * _Src,      int _Val,      size_t _MaxCount);
  const void *  __cdecl memchr(          const void * _Buf ,      int _Val,      size_t _MaxCount);
  int     __cdecl _memicmp(         const void * _Buf1,          const void * _Buf2,      size_t _Size);
  int     __cdecl _memicmp_l(         const void * _Buf1,          const void * _Buf2,      size_t _Size,        _locale_t _Locale);
         int     __cdecl memcmp(         const void * _Buf1,          const void * _Buf2,      size_t _Size);
         void *  __cdecl memcpy(         void * _Dst,          const void * _Src,      size_t _Size);

 errno_t  __cdecl memcpy_s(         void * _Dst,      rsize_t _DstSize,          const void * _Src,      rsize_t _MaxCount);
#line 57 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
        void *  __cdecl memset(         void * _Dst,      int _Val,      size_t _Size);



  void * __cdecl memccpy(       void * _Dst,          const void * _Src,      int _Val,      size_t _Size);
   int __cdecl memicmp(         const void * _Buf1,          const void * _Buf2,      size_t _Size);
#line 64 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"

#line 66 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"

         char *  __cdecl _strset(         char * _Str,      int _Val);
  errno_t __cdecl _strset_s(           char * _Dst,      size_t _DstSize,      int _Value);

  errno_t __cdecl strcpy_s(       char * _Dst,      rsize_t _DstSize,        const char * _Src);
#line 72 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl strcpy_s(       char (&_Dest)[_Size],        const char * _Source) { return strcpy_s(_Dest, _Size, _Source); } }
  char * __cdecl strcpy( char *_Dest,  const char * _Source);

  errno_t __cdecl strcat_s(           char * _Dst,      rsize_t _DstSize,        const char * _Src);
#line 77 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl strcat_s(           char (&_Dest)[_Size],        const char * _Source) { return strcat_s(_Dest, _Size, _Source); } }
  char * __cdecl strcat( char *_Dest,  const char * _Source);
         int     __cdecl strcmp(       const char * _Str1,        const char * _Str2);
         size_t  __cdecl strlen(        const char * _Str);
  size_t  __cdecl strnlen(          const char * _Str,      size_t _MaxCount);

static __inline  size_t  __cdecl strnlen_s(          const char * _Str,      size_t _MaxCount)
{
    return strnlen(_Str, _MaxCount);
}
#line 88 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"

  errno_t __cdecl memmove_s(         void * _Dst,      rsize_t _DstSize,          const void * _Src,      rsize_t _MaxCount);
#line 91 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"



#line 95 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
  void *  __cdecl memmove(         void * _Dst,          const void * _Src,      size_t _Size);
#line 97 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"




#line 102 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"

  char *  __cdecl _strdup(         const char * _Src);



#line 108 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"

  const char *  __cdecl strchr(       const char * _Str,      int _Val);
  int     __cdecl _stricmp(        const char * _Str1,         const char * _Str2);
  int     __cdecl _strcmpi(        const char * _Str1,         const char * _Str2);
  int     __cdecl _stricmp_l(        const char * _Str1,         const char * _Str2,        _locale_t _Locale);
  int     __cdecl strcoll(        const char * _Str1,         const  char * _Str2);
  int     __cdecl _strcoll_l(        const char * _Str1,         const char * _Str2,        _locale_t _Locale);
  int     __cdecl _stricoll(        const char * _Str1,         const char * _Str2);
  int     __cdecl _stricoll_l(        const char * _Str1,         const char * _Str2,        _locale_t _Locale);
  int     __cdecl _strncoll  (       const char * _Str1,        const char * _Str2,      size_t _MaxCount);
  int     __cdecl _strncoll_l(       const char * _Str1,        const char * _Str2,      size_t _MaxCount,        _locale_t _Locale);
  int     __cdecl _strnicoll (       const char * _Str1,        const char * _Str2,      size_t _MaxCount);
  int     __cdecl _strnicoll_l(       const char * _Str1,        const char * _Str2,      size_t _MaxCount,        _locale_t _Locale);
  size_t  __cdecl strcspn(        const char * _Str,         const char * _Control);
   char *  __cdecl _strerror(         const char * _ErrMsg);
  errno_t __cdecl _strerror_s(       char * _Buf,      size_t _SizeInBytes,          const char * _ErrMsg);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _strerror_s(     char (&_Buffer)[_Size],          const char * _ErrorMessage) { return _strerror_s(_Buffer, _Size, _ErrorMessage); } }
   char *  __cdecl strerror(     int);

  errno_t __cdecl strerror_s(       char * _Buf,      size_t _SizeInBytes,      int _ErrNum);
#line 129 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl strerror_s(     char (&_Buffer)[_Size],      int _ErrorMessage) { return strerror_s(_Buffer, _Size, _ErrorMessage); } }
  errno_t __cdecl _strlwr_s(           char * _Str,      size_t _Size);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _strlwr_s(           char (&_String)[_Size]) { return _strlwr_s(_String, _Size); } }
  char * __cdecl _strlwr( char *_String);
  errno_t __cdecl _strlwr_s_l(           char * _Str,      size_t _Size,        _locale_t _Locale);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _strlwr_s_l(           char (&_String)[_Size],        _locale_t _Locale) { return _strlwr_s_l(_String, _Size, _Locale); } }
  char * __cdecl _strlwr_l(         char *_String,        _locale_t _Locale);

  errno_t __cdecl strncat_s(           char * _Dst,      rsize_t _DstSize,        const char * _Src,      rsize_t _MaxCount);
#line 139 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl strncat_s(           char (&_Dest)[_Size],        const char * _Source,      size_t _Count) { return strncat_s(_Dest, _Size, _Source, _Count); } }
#pragma warning(push)
#pragma warning(disable:4609 6059)

  char * __cdecl strncat(           char *_Dest,        const char * _Source,      size_t _Count);
#pragma warning(pop)


#line 148 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
  int     __cdecl strncmp(       const char * _Str1,        const char * _Str2,      size_t _MaxCount);
#line 150 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
  int     __cdecl _strnicmp(       const char * _Str1,        const char * _Str2,      size_t _MaxCount);
  int     __cdecl _strnicmp_l(       const char * _Str1,        const char * _Str2,      size_t _MaxCount,        _locale_t _Locale);

  errno_t __cdecl strncpy_s(       char * _Dst,      rsize_t _DstSize,          const char * _Src,      rsize_t _MaxCount);
#line 155 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl strncpy_s(     char (&_Dest)[_Size],        const char * _Source,      size_t _Count) { return strncpy_s(_Dest, _Size, _Source, _Count); } }
  char * __cdecl strncpy(     char *_Dest,        const char * _Source,      size_t _Count);
  char *  __cdecl _strnset(         char * _Str,      int _Val,      size_t _MaxCount);
  errno_t __cdecl _strnset_s(           char * _Str,      size_t _Size,      int _Val,      size_t _MaxCount);
  const char *  __cdecl strpbrk(       const char * _Str,        const char * _Control);
  const char *  __cdecl strrchr(       const char * _Str,      int _Ch);
 char *  __cdecl _strrev(         char * _Str);
  size_t  __cdecl strspn(       const char * _Str,        const char * _Control);
  const char *  __cdecl strstr(       const char * _Str,        const char * _SubStr);
   char *  __cdecl strtok(           char * _Str,        const char * _Delim);

  char *  __cdecl strtok_s(           char * _Str,        const char * _Delim,                        char ** _Context);
#line 168 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
  errno_t __cdecl _strupr_s(           char * _Str,      size_t _Size);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _strupr_s(           char (&_String)[_Size]) { return _strupr_s(_String, _Size); } }
  char * __cdecl _strupr( char *_String);
  errno_t __cdecl _strupr_s_l(           char * _Str,      size_t _Size, _locale_t _Locale);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _strupr_s_l(           char (&_String)[_Size], _locale_t _Locale) { return _strupr_s_l(_String, _Size, _Locale); } }
  char * __cdecl _strupr_l(         char *_String,        _locale_t _Locale);
  size_t  __cdecl strxfrm (       char * _Dst,        const char * _Src,      size_t _MaxCount);
  size_t  __cdecl _strxfrm_l(       char * _Dst,        const char * _Src,      size_t _MaxCount,        _locale_t _Locale);


extern "C++" {


inline  char * __cdecl strchr(       char * _Str,      int _Ch)
	{ return (char*)strchr((const char*)_Str, _Ch); }
inline  char * __cdecl strpbrk(       char * _Str,        const char * _Control)
	{ return (char*)strpbrk((const char*)_Str, _Control); }
inline  char * __cdecl strrchr(       char * _Str,      int _Ch)
	{ return (char*)strrchr((const char*)_Str, _Ch); }
inline  char * __cdecl strstr(       char * _Str,        const char * _SubStr)
	{ return (char*)strstr((const char*)_Str, _SubStr); }
#line 190 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"


inline  void * __cdecl memchr(         void * _Pv,      int _C,      size_t _N)
	{ return (void*)memchr((const void*)_Pv, _C, _N); }
#line 195 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
}
#line 197 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"






#line 204 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"

   char * __cdecl strdup(         const char * _Src);



#line 210 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"


   int __cdecl strcmpi(       const char * _Str1,        const char * _Str2);
   int __cdecl stricmp(       const char * _Str1,        const char * _Str2);
  char * __cdecl strlwr(         char * _Str);
   int __cdecl strnicmp(       const char * _Str1,        const char * _Str,      size_t _MaxCount);
  char * __cdecl strnset(         char * _Str,      int _Val,      size_t _MaxCount);
  char * __cdecl strrev(         char * _Str);
         char * __cdecl strset(         char * _Str,      int _Val);
  char * __cdecl strupr(         char * _Str);

#line 222 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"









#line 232 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"

  wchar_t * __cdecl _wcsdup(       const wchar_t * _Str);



#line 238 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"


  errno_t __cdecl wcscat_s(           wchar_t * _Dst,      rsize_t _DstSize, const wchar_t * _Src);
#line 242 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl wcscat_s(           wchar_t (&_Dest)[_Size],        const wchar_t * _Source) { return wcscat_s(_Dest, _Size, _Source); } }
  wchar_t * __cdecl wcscat( wchar_t *_Dest,  const wchar_t * _Source);
  const wchar_t * __cdecl wcschr(       const wchar_t * _Str, wchar_t _Ch);
  int __cdecl wcscmp(       const wchar_t * _Str1,        const wchar_t * _Str2);

  errno_t __cdecl wcscpy_s(       wchar_t * _Dst,      rsize_t _DstSize,        const wchar_t * _Src);
#line 249 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl wcscpy_s(     wchar_t (&_Dest)[_Size],        const wchar_t * _Source) { return wcscpy_s(_Dest, _Size, _Source); } }
  wchar_t * __cdecl wcscpy( wchar_t *_Dest,  const wchar_t * _Source);
  size_t __cdecl wcscspn(       const wchar_t * _Str,        const wchar_t * _Control);
  size_t __cdecl wcslen(       const wchar_t * _Str);
  size_t __cdecl wcsnlen(         const wchar_t * _Src,      size_t _MaxCount);

static __inline  size_t __cdecl wcsnlen_s(         const wchar_t * _Src,      size_t _MaxCount)
{
    return wcsnlen(_Src, _MaxCount);
}
#line 260 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"

  errno_t __cdecl wcsncat_s(           wchar_t * _Dst,      rsize_t _DstSize,        const wchar_t * _Src,      rsize_t _MaxCount);
#line 263 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl wcsncat_s(           wchar_t (&_Dest)[_Size],        const wchar_t * _Source,      size_t _Count) { return wcsncat_s(_Dest, _Size, _Source, _Count); } }
  wchar_t * __cdecl wcsncat(     wchar_t *_Dest,        const wchar_t * _Source,      size_t _Count);
  int __cdecl wcsncmp(       const wchar_t * _Str1,        const wchar_t * _Str2,      size_t _MaxCount);

  errno_t __cdecl wcsncpy_s(       wchar_t * _Dst,      rsize_t _DstSize,        const wchar_t * _Src,      rsize_t _MaxCount);
#line 269 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl wcsncpy_s(     wchar_t (&_Dest)[_Size],        const wchar_t * _Source,      size_t _Count) { return wcsncpy_s(_Dest, _Size, _Source, _Count); } }
  wchar_t * __cdecl wcsncpy(     wchar_t *_Dest,        const wchar_t * _Source,      size_t _Count);
  const wchar_t * __cdecl wcspbrk(       const wchar_t * _Str,        const wchar_t * _Control);
  const wchar_t * __cdecl wcsrchr(       const wchar_t * _Str,      wchar_t _Ch);
  size_t __cdecl wcsspn(       const wchar_t * _Str,        const wchar_t * _Control);
  const wchar_t * __cdecl wcsstr(       const wchar_t * _Str,        const wchar_t * _SubStr);
   wchar_t * __cdecl wcstok(           wchar_t * _Str,        const wchar_t * _Delim);
  wchar_t * __cdecl wcstok_s(           wchar_t * _Str,        const wchar_t * _Delim,                        wchar_t ** _Context);
   wchar_t * __cdecl _wcserror(     int _ErrNum);
  errno_t __cdecl _wcserror_s(         wchar_t * _Buf,      size_t _SizeInWords,      int _ErrNum);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wcserror_s(     wchar_t (&_Buffer)[_Size],      int _Error) { return _wcserror_s(_Buffer, _Size, _Error); } }
   wchar_t * __cdecl __wcserror(         const wchar_t * _Str);
  errno_t __cdecl __wcserror_s(         wchar_t * _Buffer,      size_t _SizeInWords,        const wchar_t * _ErrMsg);
extern "C++" { template <size_t _Size> inline errno_t __cdecl __wcserror_s(     wchar_t (&_Buffer)[_Size],        const wchar_t * _ErrorMessage) { return __wcserror_s(_Buffer, _Size, _ErrorMessage); } }

  int __cdecl _wcsicmp(       const wchar_t * _Str1,        const wchar_t * _Str2);
  int __cdecl _wcsicmp_l(       const wchar_t * _Str1,        const wchar_t * _Str2,        _locale_t _Locale);
  int __cdecl _wcsnicmp(       const wchar_t * _Str1,        const wchar_t * _Str2,      size_t _MaxCount);
  int __cdecl _wcsnicmp_l(       const wchar_t * _Str1,        const wchar_t * _Str2,      size_t _MaxCount,        _locale_t _Locale);
  wchar_t * __cdecl _wcsnset(         wchar_t * _Str,        wchar_t _Val,      size_t _MaxCount);
  errno_t __cdecl _wcsnset_s(           wchar_t * _Dst,      size_t _DstSizeInWords, wchar_t _Val,      size_t _MaxCount);
 wchar_t * __cdecl _wcsrev(         wchar_t * _Str);
  wchar_t * __cdecl _wcsset(         wchar_t * _Str, wchar_t _Val);
  errno_t __cdecl _wcsset_s(           wchar_t * _Str,      size_t _SizeInWords, wchar_t _Val);

  errno_t __cdecl _wcslwr_s(           wchar_t * _Str,      size_t _SizeInWords);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wcslwr_s(           wchar_t (&_String)[_Size]) { return _wcslwr_s(_String, _Size); } }
  wchar_t * __cdecl _wcslwr( wchar_t *_String);
  errno_t __cdecl _wcslwr_s_l(           wchar_t * _Str,      size_t _SizeInWords,        _locale_t _Locale);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wcslwr_s_l(           wchar_t (&_String)[_Size],        _locale_t _Locale) { return _wcslwr_s_l(_String, _Size, _Locale); } }
  wchar_t * __cdecl _wcslwr_l(         wchar_t *_String,        _locale_t _Locale);
  errno_t __cdecl _wcsupr_s(           wchar_t * _Str,      size_t _Size);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wcsupr_s(           wchar_t (&_String)[_Size]) { return _wcsupr_s(_String, _Size); } }
  wchar_t * __cdecl _wcsupr( wchar_t *_String);
  errno_t __cdecl _wcsupr_s_l(           wchar_t * _Str,      size_t _Size,        _locale_t _Locale);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wcsupr_s_l(           wchar_t (&_String)[_Size],        _locale_t _Locale) { return _wcsupr_s_l(_String, _Size, _Locale); } }
  wchar_t * __cdecl _wcsupr_l(         wchar_t *_String,        _locale_t _Locale);
  size_t __cdecl wcsxfrm(       wchar_t * _Dst,        const wchar_t * _Src,      size_t _MaxCount);
  size_t __cdecl _wcsxfrm_l(       wchar_t * _Dst,        const wchar_t *_Src,      size_t _MaxCount,        _locale_t _Locale);
  int __cdecl wcscoll(       const wchar_t * _Str1,        const wchar_t * _Str2);
  int __cdecl _wcscoll_l(       const wchar_t * _Str1,        const wchar_t * _Str2,        _locale_t _Locale);
  int __cdecl _wcsicoll(       const wchar_t * _Str1,        const wchar_t * _Str2);
  int __cdecl _wcsicoll_l(       const wchar_t * _Str1,        const wchar_t *_Str2,        _locale_t _Locale);
  int __cdecl _wcsncoll(       const wchar_t * _Str1,        const wchar_t * _Str2,      size_t _MaxCount);
  int __cdecl _wcsncoll_l(       const wchar_t * _Str1,        const wchar_t * _Str2,      size_t _MaxCount,        _locale_t _Locale);
  int __cdecl _wcsnicoll(       const wchar_t * _Str1,        const wchar_t * _Str2,      size_t _MaxCount);
  int __cdecl _wcsnicoll_l(       const wchar_t * _Str1,        const wchar_t * _Str2,      size_t _MaxCount,        _locale_t _Locale);




extern "C++" {
inline  wchar_t * __cdecl wcschr(       wchar_t *_Str, wchar_t _Ch)
        {return ((wchar_t *)wcschr((const wchar_t *)_Str, _Ch)); }
inline  wchar_t * __cdecl wcspbrk(       wchar_t *_Str,        const wchar_t *_Control)
        {return ((wchar_t *)wcspbrk((const wchar_t *)_Str, _Control)); }
inline  wchar_t * __cdecl wcsrchr(       wchar_t *_Str,      wchar_t _Ch)
        {return ((wchar_t *)wcsrchr((const wchar_t *)_Str, _Ch)); }
inline  wchar_t * __cdecl wcsstr(       wchar_t *_Str,        const wchar_t *_SubStr)
        {return ((wchar_t *)wcsstr((const wchar_t *)_Str, _SubStr)); }
}
#line 331 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
#line 332 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"






#line 339 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"

   wchar_t * __cdecl wcsdup(       const wchar_t * _Str);



#line 345 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"





   int __cdecl wcsicmp(       const wchar_t * _Str1,        const wchar_t * _Str2);
   int __cdecl wcsnicmp(       const wchar_t * _Str1,        const wchar_t * _Str2,      size_t _MaxCount);
  wchar_t * __cdecl wcsnset(         wchar_t * _Str,        wchar_t _Val,      size_t _MaxCount);
  wchar_t * __cdecl wcsrev(         wchar_t * _Str);
  wchar_t * __cdecl wcsset(         wchar_t * _Str, wchar_t _Val);
  wchar_t * __cdecl wcslwr(         wchar_t * _Str);
  wchar_t * __cdecl wcsupr(         wchar_t * _Str);
   int __cdecl wcsicoll(       const wchar_t * _Str1,        const wchar_t * _Str2);

#line 360 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"


#line 363 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"



}
#line 368 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"

#line 370 "c:\\program files\\microsoft visual studio 8\\vc\\include\\string.h"
#line 220 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"
#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
















#pragma once
#line 19 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"




#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"














 















































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 24 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\limits.h"















#pragma once
#line 18 "c:\\program files\\microsoft visual studio 8\\vc\\include\\limits.h"

#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"














 















































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 20 "c:\\program files\\microsoft visual studio 8\\vc\\include\\limits.h"
















#line 37 "c:\\program files\\microsoft visual studio 8\\vc\\include\\limits.h"



















#line 57 "c:\\program files\\microsoft visual studio 8\\vc\\include\\limits.h"





#line 63 "c:\\program files\\microsoft visual studio 8\\vc\\include\\limits.h"





#line 69 "c:\\program files\\microsoft visual studio 8\\vc\\include\\limits.h"








#line 78 "c:\\program files\\microsoft visual studio 8\\vc\\include\\limits.h"








#line 87 "c:\\program files\\microsoft visual studio 8\\vc\\include\\limits.h"






#line 94 "c:\\program files\\microsoft visual studio 8\\vc\\include\\limits.h"
#line 95 "c:\\program files\\microsoft visual studio 8\\vc\\include\\limits.h"







#line 103 "c:\\program files\\microsoft visual studio 8\\vc\\include\\limits.h"
#line 104 "c:\\program files\\microsoft visual studio 8\\vc\\include\\limits.h"
































#line 137 "c:\\program files\\microsoft visual studio 8\\vc\\include\\limits.h"
#line 25 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"






#pragma pack(push,8)
#line 33 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"


extern "C" {
#line 37 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"



















typedef int (__cdecl * _onexit_t)(void);



#line 61 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"



#line 65 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"




#line 70 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"


#line 73 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"






typedef struct _div_t {
        int quot;
        int rem;
} div_t;

typedef struct _ldiv_t {
        long quot;
        long rem;
} ldiv_t;


#line 91 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"










#pragma pack(4)
typedef struct {
    unsigned char ld[10];
} _LDOUBLE;
#pragma pack()













#line 120 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"

typedef struct {
        double x;
} _CRT_DOUBLE;

typedef struct {
    float f;
} _CRT_FLOAT;





typedef struct {
        


        long double x;
} _LONGDOUBLE;



#pragma pack(4)
typedef struct {
    unsigned char ld12[12];
} _LDBL12;
#pragma pack()


#line 150 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"





























































typedef void (__cdecl *_purecall_handler)(void); 


 _purecall_handler __cdecl _set_purecall_handler(       _purecall_handler _Handler);
 _purecall_handler __cdecl _get_purecall_handler();
#line 217 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"


extern "C++"
{




#line 226 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"







#line 234 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
}
#line 236 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"



typedef void (__cdecl *_invalid_parameter_handler)(const wchar_t *, const wchar_t *, const wchar_t *, unsigned int, uintptr_t); 


 _invalid_parameter_handler __cdecl _set_invalid_parameter_handler(       _invalid_parameter_handler _Handler);
 _invalid_parameter_handler __cdecl _get_invalid_parameter_handler(void);
#line 245 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"


extern "C++"
{




#line 254 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"






#line 261 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
}
#line 263 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"











 unsigned long * __cdecl __doserrno(void);


errno_t __cdecl _set_doserrno(     unsigned long _Value);
errno_t __cdecl _get_doserrno(     unsigned long * _Value);


  char ** __cdecl __sys_errlist(void);


  int * __cdecl __sys_nerr(void);













#line 299 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"


 extern int __argc;          
 extern char ** __argv;      
 extern wchar_t ** __wargv;  







#line 312 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"





 extern char ** _environ;    
 extern wchar_t ** _wenviron;    
#line 320 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"

  extern char * _pgmptr;      
  extern wchar_t * _wpgmptr;  














#line 338 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"

errno_t __cdecl _get_pgmptr(       char ** _Value);
errno_t __cdecl _get_wpgmptr(       wchar_t ** _Value);



  extern int _fmode;          



#line 349 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"

 errno_t __cdecl _set_fmode(     int _Mode);
 errno_t __cdecl _get_fmode(     int * _PMode);



#pragma warning(push)
#pragma warning(disable:4141)


 __declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "GetVersionEx" "instead. See online help for details."))	 extern unsigned int _osplatform;
 __declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "GetVersionEx" "instead. See online help for details."))			 extern unsigned int _osver;
 __declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "GetVersionEx" "instead. See online help for details."))		 extern unsigned int _winver;
 __declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "GetVersionEx" "instead. See online help for details."))		 extern unsigned int _winmajor;
 __declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "GetVersionEx" "instead. See online help for details."))		 extern unsigned int _winminor;














#line 379 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"

#pragma warning(pop)

__declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "GetVersionEx" "instead. See online help for details.")) errno_t __cdecl _get_osplatform(     unsigned int * _Value);
__declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "GetVersionEx" "instead. See online help for details.")) errno_t __cdecl _get_osver(     unsigned int * _Value);
__declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "GetVersionEx" "instead. See online help for details.")) errno_t __cdecl _get_winver(     unsigned int * _Value);
__declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "GetVersionEx" "instead. See online help for details.")) errno_t __cdecl _get_winmajor(     unsigned int * _Value);
__declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "GetVersionEx" "instead. See online help for details.")) errno_t __cdecl _get_winminor(     unsigned int * _Value);





#line 393 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
extern "C++"
{
template <typename _CountofType, size_t _SizeOfArray>
char (*__countof_helper( _CountofType (&_Array)[_SizeOfArray]))[_SizeOfArray];

}
#line 400 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
#line 401 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"





 __declspec(noreturn) void __cdecl exit(     int _Code);
 __declspec(noreturn) void __cdecl _exit(     int _Code);
 void __cdecl abort(void);
#line 410 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"

 unsigned int __cdecl _set_abort_behavior(     unsigned int _Flags,      unsigned int _Mask);



        int     __cdecl abs(     int _X);
        long    __cdecl labs(     long _X);
#line 418 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"


        __int64    __cdecl _abs64(__int64);
#line 422 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"










#line 433 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"












#line 446 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
        int    __cdecl atexit(void (__cdecl *)(void));
#line 448 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"


 double  __cdecl atof(       const char *_String);
 double  __cdecl _atof_l(       const char *_String,        _locale_t _Locale);
#line 453 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
   int    __cdecl atoi(       const char *_Str);
  int    __cdecl _atoi_l(       const char *_Str,        _locale_t _Locale);
  long   __cdecl atol(       const char *_Str);
  long   __cdecl _atol_l(       const char *_Str,        _locale_t _Locale);



  void * __cdecl bsearch_s(     const void * _Key,        const void * _Base, 
             rsize_t _NumOfElements,      rsize_t _SizeOfElements,
             int (__cdecl * _PtFuncCompare)(void *, const void *, const void *), void * _Context);
#line 464 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
  void * __cdecl bsearch(     const void * _Key,        const void * _Base, 
             size_t _NumOfElements,      size_t _SizeOfElements,
             int (__cdecl * _PtFuncCompare)(const void *, const void *));


 void __cdecl qsort_s(       void * _Base, 
             rsize_t _NumOfElements,      rsize_t _SizeOfElements,
             int (__cdecl * _PtFuncCompare)(void *, const void *, const void *), void *_Context);
#line 473 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
 void __cdecl qsort(       void * _Base, 
	     size_t _NumOfElements,      size_t _SizeOfElements, 
             int (__cdecl * _PtFuncCompare)(const void *, const void *));
#line 477 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
         unsigned short __cdecl _byteswap_ushort(     unsigned short _Short);
         unsigned long  __cdecl _byteswap_ulong (     unsigned long _Long);

         unsigned __int64 __cdecl _byteswap_uint64(unsigned __int64 _Int64);
#line 482 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
  div_t  __cdecl div(     int _Numerator,      int _Denominator);
   char * __cdecl getenv(       const char * _VarName);

  errno_t __cdecl getenv_s(     size_t * _ReturnSize,        char * _DstBuf,      rsize_t _DstSize,        const char * _VarName);
#line 487 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl getenv_s(     size_t * _ReturnSize,      char (&_Dest)[_Size],        const char * _VarName) { return getenv_s(_ReturnSize, _Dest, _Size, _VarName); } }
  errno_t __cdecl _dupenv_s(                     char **_PBuffer,        size_t * _PBufferSizeInBytes,        const char * _VarName);
  errno_t __cdecl _itoa_s(     int _Value,        char * _DstBuf,      size_t _Size,      int _Radix);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _itoa_s(     int _Value,      char (&_Dest)[_Size],      int _Radix) { return _itoa_s(_Value, _Dest, _Size, _Radix); } }
  char * __cdecl _itoa( int _Value,  char *_Dest,  int _Radix);

  errno_t __cdecl _i64toa_s(     __int64 _Val,        char * _DstBuf,      size_t _Size,      int _Radix);
  char * __cdecl _i64toa(     __int64 _Val,      char * _DstBuf,      int _Radix);
  errno_t __cdecl _ui64toa_s(     unsigned __int64 _Val,        char * _DstBuf,      size_t _Size,      int _Radix);
  char * __cdecl _ui64toa(     unsigned __int64 _Val,      char * _DstBuf,      int _Radix);
  __int64 __cdecl _atoi64(       const char * _String);
  __int64 __cdecl _atoi64_l(       const char * _String,        _locale_t _Locale);
  __int64 __cdecl _strtoi64(       const char * _String,                    char ** _EndPtr,      int _Radix);
  __int64 __cdecl _strtoi64_l(       const char * _String,                    char ** _EndPtr,      int _Radix,        _locale_t _Locale);
  unsigned __int64 __cdecl _strtoui64(       const char * _String,                    char ** _EndPtr,      int _Radix);
  unsigned __int64 __cdecl _strtoui64_l(       const char * _String,                    char ** _EndPtr,      int  _Radix,        _locale_t _Locale);
#line 504 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
  ldiv_t __cdecl ldiv(     long _Numerator,      long _Denominator);

extern "C++"
{
    inline ldiv_t  div(     long _A1,      long _A2)
    {
        return ldiv(_A1, _A2);
    }
}
#line 514 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
  errno_t __cdecl _ltoa_s(     long _Val,        char * _DstBuf,      size_t _Size,      int _Radix);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _ltoa_s(     long _Value,      char (&_Dest)[_Size],      int _Radix) { return _ltoa_s(_Value, _Dest, _Size, _Radix); } }
  char * __cdecl _ltoa( long _Value,  char *_Dest,  int _Radix);
  int    __cdecl mblen(           const char * _Ch,      size_t _MaxCount);
  int    __cdecl _mblen_l(           const char * _Ch,      size_t _MaxCount,        _locale_t _Locale);
  size_t __cdecl _mbstrlen(       const char * _Str);
  size_t __cdecl _mbstrlen_l(       const char *_Str,        _locale_t _Locale);
  size_t __cdecl _mbstrnlen(         const char *_Str,      size_t _MaxCount);
  size_t __cdecl _mbstrnlen_l(         const char *_Str,      size_t _MaxCount,        _locale_t _Locale);
 int    __cdecl mbtowc(     wchar_t * _DstCh,            const char * _SrcCh,      size_t _SrcSizeInBytes);
 int    __cdecl _mbtowc_l(     wchar_t * _DstCh,            const char * _SrcCh,      size_t _SrcSizeInBytes,        _locale_t _Locale);
  errno_t __cdecl mbstowcs_s(       size_t * _PtNumOfCharConverted,          wchar_t * _DstBuf,      size_t _SizeInWords,          const char * _SrcBuf,      size_t _MaxCount );
extern "C++" { template <size_t _Size> inline errno_t __cdecl mbstowcs_s(       size_t * _PtNumOfCharConverted,        wchar_t (&_Dest)[_Size],        const char * _Source,      size_t _MaxCount) { return mbstowcs_s(_PtNumOfCharConverted, _Dest, _Size, _Source, _MaxCount); } }
  size_t __cdecl mbstowcs( wchar_t *_Dest,  const char * _Source,  size_t _MaxCount);

  errno_t __cdecl _mbstowcs_s_l(       size_t * _PtNumOfCharConverted,          wchar_t * _DstBuf,      size_t _SizeInWords,          const char * _SrcBuf,      size_t _MaxCount,        _locale_t _Locale);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _mbstowcs_s_l(       size_t * _PtNumOfCharConverted,      wchar_t (&_Dest)[_Size],        const char * _Source,      size_t _MaxCount,        _locale_t _Locale) { return _mbstowcs_s_l(_PtNumOfCharConverted, _Dest, _Size, _Source, _MaxCount, _Locale); } }
  size_t __cdecl _mbstowcs_l(       wchar_t *_Dest,        const char * _Source,      size_t _MaxCount,        _locale_t _Locale);

  int    __cdecl rand(void);


#line 537 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"

  int    __cdecl _set_error_mode(     int _Mode);

 void   __cdecl srand(     unsigned int _Seed);
  double __cdecl strtod(       const char * _Str,                    char ** _EndPtr);
  double __cdecl _strtod_l(       const char * _Str,                    char ** _EndPtr,        _locale_t _Locale);
  long   __cdecl strtol(       const char * _Str,                    char ** _EndPtr,      int _Radix );
  long   __cdecl _strtol_l(       const char *_Str,                    char **_EndPtr,      int _Radix,        _locale_t _Locale);
  unsigned long __cdecl strtoul(       const char * _Str,                    char ** _EndPtr,      int _Radix);
  unsigned long __cdecl _strtoul_l(const char * _Str,                    char **_EndPtr,      int _Radix,        _locale_t _Locale);


 int __cdecl system(         const char * _Command);
#line 551 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
  errno_t __cdecl _ultoa_s(     unsigned long _Val,        char * _DstBuf,      size_t _Size,      int _Radix);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _ultoa_s(     unsigned long _Value,      char (&_Dest)[_Size],      int _Radix) { return _ultoa_s(_Value, _Dest, _Size, _Radix); } }
  char * __cdecl _ultoa( unsigned long _Value,  char *_Dest,  int _Radix);
  int    __cdecl wctomb(         char * _MbCh,        wchar_t _WCh);
  int    __cdecl _wctomb_l(       char * _MbCh,        wchar_t _WCh,        _locale_t _Locale);

  errno_t __cdecl wctomb_s(       int * _SizeConverted,          char * _MbCh,      rsize_t _SizeInBytes,        wchar_t _WCh);
#line 559 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
  errno_t __cdecl _wctomb_s_l(       int * _SizeConverted,          char * _MbCh,      size_t _SizeInBytes,        wchar_t _WCh,        _locale_t _Locale);
  errno_t __cdecl wcstombs_s(       size_t * _PtNumOfCharConverted,          char * _Dst,      size_t _DstSizeInBytes,        const wchar_t * _Src,      size_t _MaxCountInBytes);
extern "C++" { template <size_t _Size> inline errno_t __cdecl wcstombs_s(       size_t * _PtNumOfCharConverted,        char (&_Dest)[_Size],        const wchar_t * _Source,      size_t _MaxCount) { return wcstombs_s(_PtNumOfCharConverted, _Dest, _Size, _Source, _MaxCount); } }
  size_t __cdecl wcstombs( char *_Dest,  const wchar_t * _Source,  size_t _MaxCount);
  errno_t __cdecl _wcstombs_s_l(       size_t * _PtNumOfCharConverted,          char * _Dst,      size_t _DstSizeInBytes,        const wchar_t * _Src,      size_t _MaxCountInBytes,        _locale_t _Locale);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wcstombs_s_l(       size_t * _PtNumOfCharConverted,        char (&_Dest)[_Size],        const wchar_t * _Source,      size_t _MaxCount,        _locale_t _Locale) { return _wcstombs_s_l(_PtNumOfCharConverted, _Dest, _Size, _Source, _MaxCount, _Locale); } }
  size_t __cdecl _wcstombs_l(       char *_Dest,        const wchar_t * _Source,      size_t _MaxCount,        _locale_t _Locale);






















#line 589 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"



   __declspec(noalias) __declspec(restrict)         void * __cdecl calloc(     size_t _NumOfElements,      size_t _SizeOfElements);
                     __declspec(noalias)                                                                             void   __cdecl free(       void * _Memory);
   __declspec(noalias) __declspec(restrict)                                   void * __cdecl malloc(     size_t _Size);
                     __declspec(noalias) __declspec(restrict)                                void * __cdecl realloc(       void * _Memory,      size_t _NewSize);
                     __declspec(noalias) __declspec(restrict)                            void * __cdecl _recalloc(       void * _Memory,      size_t _Count,      size_t _Size);
                     __declspec(noalias)                                                                             void   __cdecl _aligned_free(       void * _Memory);
                     __declspec(noalias) __declspec(restrict)                                   void * __cdecl _aligned_malloc(     size_t _Size,      size_t _Alignment);
                     __declspec(noalias) __declspec(restrict)                                   void * __cdecl _aligned_offset_malloc(     size_t _Size,      size_t _Alignment,      size_t _Offset);
                     __declspec(noalias) __declspec(restrict)                                   void * __cdecl _aligned_realloc(       void * _Memory,      size_t _Size,      size_t _Alignment);
                     __declspec(noalias) __declspec(restrict)                            void * __cdecl _aligned_recalloc(       void * _Memory,      size_t _Count,      size_t _Size,      size_t _Alignment);
                     __declspec(noalias) __declspec(restrict)                                   void * __cdecl _aligned_offset_realloc(       void * _Memory,      size_t _Size,      size_t _Alignment,      size_t _Offset);
                     __declspec(noalias) __declspec(restrict)                            void * __cdecl _aligned_offset_recalloc(       void * _Memory,      size_t _Count,      size_t _Size,      size_t _Alignment,      size_t _Offset);
#line 605 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"





  errno_t __cdecl _itow_s (     int _Val,        wchar_t * _DstBuf,      size_t _SizeInWords,      int _Radix);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _itow_s(     int _Value,      wchar_t (&_Dest)[_Size],      int _Radix) { return _itow_s(_Value, _Dest, _Size, _Radix); } }
  wchar_t * __cdecl _itow( int _Value,  wchar_t *_Dest,  int _Radix);
  errno_t __cdecl _ltow_s (     long _Val,        wchar_t * _DstBuf,      size_t _SizeInWords,      int _Radix);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _ltow_s(     long _Value,      wchar_t (&_Dest)[_Size],      int _Radix) { return _ltow_s(_Value, _Dest, _Size, _Radix); } }
  wchar_t * __cdecl _ltow( long _Value,  wchar_t *_Dest,  int _Radix);
  errno_t __cdecl _ultow_s (     unsigned long _Val,        wchar_t * _DstBuf,      size_t _SizeInWords,      int _Radix);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _ultow_s(     unsigned long _Value,      wchar_t (&_Dest)[_Size],      int _Radix) { return _ultow_s(_Value, _Dest, _Size, _Radix); } }
  wchar_t * __cdecl _ultow( unsigned long _Value,  wchar_t *_Dest,  int _Radix);
  double __cdecl wcstod(       const wchar_t * _Str,                    wchar_t ** _EndPtr);
  double __cdecl _wcstod_l(       const wchar_t *_Str,                    wchar_t ** _EndPtr,        _locale_t _Locale);
  long   __cdecl wcstol(       const wchar_t *_Str,                    wchar_t ** _EndPtr, int _Radix);
  long   __cdecl _wcstol_l(       const wchar_t *_Str,                    wchar_t **_EndPtr, int _Radix,        _locale_t _Locale);
  unsigned long __cdecl wcstoul(       const wchar_t *_Str,                    wchar_t ** _EndPtr, int _Radix);
  unsigned long __cdecl _wcstoul_l(       const wchar_t *_Str,                    wchar_t **_EndPtr, int _Radix,        _locale_t _Locale);
   wchar_t * __cdecl _wgetenv(       const wchar_t * _VarName);
  errno_t __cdecl _wgetenv_s(     size_t * _ReturnSize,        wchar_t * _DstBuf,      size_t _DstSizeInWords,        const wchar_t * _VarName);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wgetenv_s(     size_t * _ReturnSize,      wchar_t (&_Dest)[_Size],        const wchar_t * _VarName) { return _wgetenv_s(_ReturnSize, _Dest, _Size, _VarName); } }
  errno_t __cdecl _wdupenv_s(                     wchar_t **_Buffer,        size_t *_BufferSizeInWords,        const wchar_t *_VarName);


 int __cdecl _wsystem(         const wchar_t * _Command);
#line 633 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
  double __cdecl _wtof(       const wchar_t *_Str);
  double __cdecl _wtof_l(       const wchar_t *_Str,        _locale_t _Locale);
  int __cdecl _wtoi(       const wchar_t *_Str);
  int __cdecl _wtoi_l(       const wchar_t *_Str,        _locale_t _Locale);
  long __cdecl _wtol(       const wchar_t *_Str);
  long __cdecl _wtol_l(       const wchar_t *_Str,        _locale_t _Locale);


  errno_t __cdecl _i64tow_s(     __int64 _Val,        wchar_t * _DstBuf,      size_t _SizeInWords,      int _Radix);
  wchar_t * __cdecl _i64tow(     __int64 _Val,      wchar_t * _DstBuf,      int _Radix);
  errno_t __cdecl _ui64tow_s(     unsigned __int64 _Val,        wchar_t * _DstBuf,      size_t _SizeInWords,      int _Radix);
  wchar_t * __cdecl _ui64tow(     unsigned __int64 _Val,      wchar_t * _DstBuf,      int _Radix);
  __int64   __cdecl _wtoi64(       const wchar_t *_Str);
  __int64   __cdecl _wtoi64_l(       const wchar_t *_Str,        _locale_t _Locale);
  __int64   __cdecl _wcstoi64(       const wchar_t * _Str,                    wchar_t ** _EndPtr,      int _Radix);
  __int64   __cdecl _wcstoi64_l(       const wchar_t * _Str,                    wchar_t ** _EndPtr,      int _Radix,        _locale_t _Locale);
  unsigned __int64  __cdecl _wcstoui64(       const wchar_t * _Str,                    wchar_t ** _EndPtr,      int _Radix);
  unsigned __int64  __cdecl _wcstoui64_l(       const wchar_t *_Str ,                    wchar_t ** _EndPtr,      int _Radix,        _locale_t _Locale);
#line 652 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"


#line 655 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"












#line 668 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"

  char * __cdecl _fullpath(         char * _FullPath,        const char * _Path,      size_t _SizeInBytes);



#line 674 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"

  errno_t __cdecl _ecvt_s(       char * _DstBuf,      size_t _Size,      double _Val,      int _NumOfDights,      int * _PtDec,      int * _PtSign);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _ecvt_s(     char (&_Dest)[_Size],      double _Value,      int _NumOfDigits,      int * _PtDec,      int * _PtSign) { return _ecvt_s(_Dest, _Size, _Value, _NumOfDigits, _PtDec, _PtSign); } }
   char * __cdecl _ecvt(     double _Val,      int _NumOfDigits,      int * _PtDec,      int * _PtSign);
  errno_t __cdecl _fcvt_s(       char * _DstBuf,      size_t _Size,      double _Val,      int _NumOfDec,      int * _PtDec,      int * _PtSign);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _fcvt_s(     char (&_Dest)[_Size],      double _Value,      int _NumOfDigits,      int * _PtDec,      int * _PtSign) { return _fcvt_s(_Dest, _Size, _Value, _NumOfDigits, _PtDec, _PtSign); } }
   char * __cdecl _fcvt(     double _Val,      int _NumOfDec,      int * _PtDec,      int * _PtSign);
 errno_t __cdecl _gcvt_s(       char * _DstBuf,      size_t _Size,      double _Val,      int _NumOfDigits);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _gcvt_s(     char (&_Dest)[_Size],      double _Value,      int _NumOfDigits) { return _gcvt_s(_Dest, _Size, _Value, _NumOfDigits); } }
  char * __cdecl _gcvt(     double _Val,      int _NumOfDigits,      char * _DstBuf);

  int __cdecl _atodbl(     _CRT_DOUBLE * _Result,        char * _Str);
  int __cdecl _atoldbl(     _LDOUBLE * _Result,        char * _Str);
  int __cdecl _atoflt(     _CRT_FLOAT * _Result,        char * _Str);
  int __cdecl _atodbl_l(     _CRT_DOUBLE * _Result,        char * _Str,        _locale_t _Locale);
 int __cdecl _atoldbl_l(     _LDOUBLE * _Result,        char * _Str,        _locale_t _Locale);
 int __cdecl _atoflt_l(     _CRT_FLOAT * _Result,        char * _Str,        _locale_t _Locale);
         unsigned long __cdecl _lrotl(     unsigned long _Val,      int _Shift);
         unsigned long __cdecl _lrotr(     unsigned long _Val,      int _Shift);
  errno_t   __cdecl _makepath_s(       char * _PathResult,        size_t _Size,          const char * _Drive,          const char * _Dir,          const char * _Filename,
                 const char * _Ext);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _makepath_s(     char (&_Path)[_Size],          const char * _Drive,          const char * _Dir,          const char * _Filename,          const char * _Ext) { return _makepath_s(_Path, _Size, _Drive, _Dir, _Filename, _Ext); } }
  void __cdecl _makepath( char *_Path,  const char * _Drive,  const char * _Dir,  const char * _Filename,  const char * _Ext);












#line 710 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"












#line 723 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
        _onexit_t __cdecl _onexit(       _onexit_t _Func);
#line 725 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
        


 void __cdecl perror(         const char * _ErrMsg);
#line 730 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
  int    __cdecl _putenv(       const char * _EnvString);
  errno_t __cdecl _putenv_s(       const char * _Name,        const char * _Value);
         unsigned int __cdecl _rotl(     unsigned int _Val,      int _Shift);

         unsigned __int64 __cdecl _rotl64(     unsigned __int64 _Val,      int _Shift);
#line 736 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
         unsigned int __cdecl _rotr(     unsigned int _Val,      int _Shift);

         unsigned __int64 __cdecl _rotr64(     unsigned __int64 _Val,      int _Shift);
#line 740 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
 errno_t __cdecl _searchenv_s(       const char * _Filename,        const char * _EnvVar,        char * _ResultPath,      size_t _SizeInBytes);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _searchenv_s(       const char * _Filename,        const char * _EnvVar,      char (&_ResultPath)[_Size]) { return _searchenv_s(_Filename, _EnvVar, _ResultPath, _Size); } }
  void __cdecl _searchenv( const char * _Filename,  const char * _EnvVar,  char *_ResultPath);

  void   __cdecl _splitpath(       const char * _FullPath,        char * _Drive,        char * _Dir,        char * _Filename,        char * _Ext);
  errno_t  __cdecl _splitpath_s(       const char * _FullPath, 
		         char * _Drive,      size_t _DriveSize, 
		         char * _Dir,      size_t _DirSize, 
		         char * _Filename,      size_t _FilenameSize, 
		         char * _Ext,      size_t _ExtSize);
extern "C++" { template <size_t _DriveSize, size_t _DirSize, size_t _NameSize, size_t _ExtSize> inline errno_t __cdecl _splitpath_s(     const char *_Dest,        char (&_Drive)[_DriveSize],        char (&_Dir)[_DirSize],        char (&_Name)[_NameSize],        char (&_Ext)[_ExtSize]) { return _splitpath_s(_Dest, _Drive, _DriveSize, _Dir, _DirSize, _Name, _NameSize, _Ext, _ExtSize); } }

 void   __cdecl _swab(           char * _Buf1,            char * _Buf2, int _SizeInBytes);








#line 762 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"

  wchar_t * __cdecl _wfullpath(         wchar_t * _FullPath,        const wchar_t * _Path,      size_t _SizeInWords);



#line 768 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"

  errno_t __cdecl _wmakepath_s(       wchar_t * _PathResult,        size_t _SizeInWords,          const wchar_t * _Drive,          const wchar_t * _Dir,          const wchar_t * _Filename,
                 const wchar_t * _Ext);        
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wmakepath_s(     wchar_t (&_ResultPath)[_Size],          const wchar_t * _Drive,          const wchar_t * _Dir,          const wchar_t * _Filename,          const wchar_t * _Ext) { return _wmakepath_s(_ResultPath, _Size, _Drive, _Dir, _Filename, _Ext); } }
  void __cdecl _wmakepath( wchar_t *_ResultPath,  const wchar_t * _Drive,  const wchar_t * _Dir,  const wchar_t * _Filename,  const wchar_t * _Ext);


 void __cdecl _wperror(         const wchar_t * _ErrMsg);
#line 777 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
  int    __cdecl _wputenv(       const wchar_t * _EnvString);
  errno_t __cdecl _wputenv_s(       const wchar_t * _Name,        const wchar_t * _Value);
 errno_t __cdecl _wsearchenv_s(       const wchar_t * _Filename,        const wchar_t * _EnvVar,        wchar_t * _ResultPath,      size_t _SizeInWords);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wsearchenv_s(       const wchar_t * _Filename,        const wchar_t * _EnvVar,      wchar_t (&_ResultPath)[_Size]) { return _wsearchenv_s(_Filename, _EnvVar, _ResultPath, _Size); } }
  void __cdecl _wsearchenv( const wchar_t * _Filename,  const wchar_t * _EnvVar,  wchar_t *_ResultPath);
  void   __cdecl _wsplitpath(       const wchar_t * _FullPath,        wchar_t * _Drive,        wchar_t * _Dir,        wchar_t * _Filename,        wchar_t * _Ext);
 errno_t __cdecl _wsplitpath_s(       const wchar_t * _FullPath, 
		         wchar_t * _Drive,      size_t _DriveSizeInWords, 
		         wchar_t * _Dir,      size_t _DirSizeInWords, 
		         wchar_t * _Filename,      size_t _FilenameSizeInWords, 
		         wchar_t * _Ext,      size_t _ExtSizeInWords);
extern "C++" { template <size_t _DriveSize, size_t _DirSize, size_t _NameSize, size_t _ExtSize> inline errno_t __cdecl _wsplitpath_s(     const wchar_t *_Path,        wchar_t (&_Drive)[_DriveSize],        wchar_t (&_Dir)[_DirSize],        wchar_t (&_Name)[_NameSize],        wchar_t (&_Ext)[_ExtSize]) { return _wsplitpath_s(_Path, _Drive, _DriveSize, _Dir, _DirSize, _Name, _NameSize, _Ext, _ExtSize); } }


#line 792 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"


__declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "SetErrorMode" "instead. See online help for details."))  void __cdecl _seterrormode(     int _Mode);
__declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "Beep" "instead. See online help for details."))  void __cdecl _beep(     unsigned _Frequency,      unsigned _Duration);
__declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "Sleep" "instead. See online help for details."))  void __cdecl _sleep(     unsigned long _Duration);

#line 799 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"
















#pragma warning(push)
#pragma warning(disable: 4141)  
 		  char * __cdecl ecvt(     double _Val,      int _NumOfDigits,      int * _PtDec,      int * _PtSign);
 		  char * __cdecl fcvt(     double _Val,      int _NumOfDec,      int * _PtDec,      int * _PtSign);
 		 char * __cdecl gcvt(     double _Val,      int _NumOfDigits,      char * _DstBuf);
 		 char * __cdecl itoa(     int _Val,      char * _DstBuf,      int _Radix);
 		 char * __cdecl ltoa(     long _Val,      char * _DstBuf,      int _Radix);
 									  int    __cdecl putenv(       const char * _EnvString);
										 void   __cdecl swab(           char * _Buf1,           char * _Buf2,      int _SizeInBytes);
 	 char * __cdecl ultoa(     unsigned long _Val,      char * _Dstbuf,      int _Radix);
#pragma warning(pop)
_onexit_t __cdecl onexit(       _onexit_t _Func);

#line 829 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"

#line 831 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"


}

#line 836 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"


#pragma pack(pop)
#line 840 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"

#line 842 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdlib.h"

#line 221 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"
#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"















#pragma once
#line 18 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"




#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"














 















































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 23 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"





#pragma pack(push,8)
#line 30 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"


extern "C" {
#line 34 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"

























struct _iobuf {
        char *_ptr;
        int   _cnt;
        char *_base;
        int   _flag;
        int   _file;
        int   _charbuf;
        int   _bufsiz;
        char *_tmpfname;
        };
typedef struct _iobuf FILE;

#line 72 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"










#line 83 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"













#line 97 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"




















#line 118 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"













 FILE * __cdecl __iob_func(void);
#line 133 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"










#line 144 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"



typedef __int64 fpos_t;




#line 153 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
#line 154 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"


#line 157 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"






#line 164 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
























  int __cdecl _filbuf(     FILE * _File );
  int __cdecl _flsbuf(     int _Ch,      FILE * _File);




  FILE * __cdecl _fsopen(       const char * _Filename,        const char * _Mode,      int _ShFlag);
#line 196 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"

 void __cdecl clearerr(     FILE * _File);
  errno_t __cdecl clearerr_s(     FILE * _File );
  int __cdecl fclose(     FILE * _File);
  int __cdecl _fcloseall(void);




  FILE * __cdecl _fdopen(     int _FileHandle,        const char * _Mode);
#line 207 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"

  int __cdecl feof(     FILE * _File);
  int __cdecl ferror(     FILE * _File);
  int __cdecl fflush(       FILE * _File);
  int __cdecl fgetc(     FILE * _File);
  int __cdecl _fgetchar(void);
  int __cdecl fgetpos(     FILE * _File ,      fpos_t * _Pos);
  char * __cdecl fgets(       char * _Buf,      int _MaxCount,      FILE * _File);




  int __cdecl _fileno(     FILE * _File);
#line 221 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"




#line 226 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"

  char * __cdecl _tempnam(         const char * _DirName,          const char * _FilePrefix);



#line 232 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"

  int __cdecl _flushall(void);
   FILE * __cdecl fopen(       const char * _Filename,        const char * _Mode);

  errno_t __cdecl fopen_s(                  FILE ** _File,        const char * _Filename,        const char * _Mode);
#line 238 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
  int __cdecl fprintf(     FILE * _File,         const char * _Format, ...);
  int __cdecl fprintf_s(     FILE * _File,         const char * _Format, ...);
  int __cdecl fputc(     int _Ch,      FILE * _File);
  int __cdecl _fputchar(     int _Ch);
  int __cdecl fputs(       const char * _Str,      FILE * _File);
  size_t __cdecl fread(     void * _DstBuf,      size_t _ElementSize,      size_t _Count,      FILE * _File);
  size_t __cdecl fread_s(     void * _DstBuf,      size_t _DstSize,      size_t _ElementSize,      size_t _Count,      FILE * _File);
   FILE * __cdecl freopen(       const char * _Filename,        const char * _Mode,      FILE * _File);

  errno_t __cdecl freopen_s(                  FILE ** _File,        const char * _Filename,        const char * _Mode,      FILE * _OldFile);
#line 249 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
   int __cdecl fscanf(     FILE * _File,         const char * _Format, ...);
   int __cdecl _fscanf_l(     FILE * _File,         const char * _Format,        _locale_t _Locale, ...);

  int __cdecl fscanf_s(     FILE * _File,         const char * _Format, ...);
#line 254 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
  int __cdecl _fscanf_s_l(     FILE * _File,         const char * _Format,        _locale_t _Locale, ...);
  int __cdecl fsetpos(     FILE * _File,      const fpos_t * _Pos);
  int __cdecl fseek(     FILE * _File,      long _Offset,      int _Origin);
  long __cdecl ftell(     FILE * _File);

  int __cdecl _fseeki64(     FILE * _File,      __int64 _Offset,      int _Origin);
  __int64 __cdecl _ftelli64(     FILE * _File);

  size_t __cdecl fwrite(       const void * _Str,      size_t _Size,      size_t _Count,      FILE * _File);
  int __cdecl getc(     FILE * _File);
  int __cdecl getchar(void);
  int __cdecl _getmaxstdio(void);

 char * __cdecl gets_s(       char * _Buf,      rsize_t _Size);
#line 269 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
extern "C++" { template <size_t _Size> inline char * __cdecl gets_s(     char (&_Buffer)[_Size]) { return gets_s(_Buffer, _Size); } }
  char * __cdecl gets( char *_Buffer);
 int __cdecl _getw(     FILE * _File);




  int __cdecl _pclose(     FILE * _File);
  FILE * __cdecl _popen(       const char * _Command,        const char * _Mode);
  int __cdecl printf(        const char * _Format, ...);
  int __cdecl printf_s(        const char * _Format, ...);
  int __cdecl putc(     int _Ch,      FILE * _File);
  int __cdecl putchar(     int _Ch);
  int __cdecl puts(       const char * _Str);
  int __cdecl _putw(     int _Word,      FILE * _File);


  int __cdecl remove(       const char * _Filename);
  int __cdecl rename(       const char * _OldFilename,        const char * _NewFilename);
  int __cdecl _unlink(       const char * _Filename);

   int __cdecl unlink(       const char * _Filename);
#line 292 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
#line 293 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
 void __cdecl rewind(     FILE * _File);
  int __cdecl _rmtmp(void);
   int __cdecl scanf(        const char * _Format, ...);
   int __cdecl _scanf_l(        const char * _Format,        _locale_t _Locale, ...);

  int __cdecl scanf_s(        const char * _Format, ...);
#line 300 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
  int __cdecl _scanf_s_l(        const char * _Format,        _locale_t _Locale, ...);
  void __cdecl setbuf(     FILE * _File,              char * _Buffer);
  int __cdecl _setmaxstdio(     int _Max);
  unsigned int __cdecl _set_output_format(     unsigned int _Format);
  unsigned int __cdecl _get_output_format(void);
  int __cdecl setvbuf(     FILE * _File,          char * _Buf,      int _Mode,      size_t _Size);
  int __cdecl _snprintf_s(       char * _DstBuf,      size_t _DstSize,      size_t _MaxCount,         const char * _Format, ...);
extern "C++" { __pragma(warning(push)); __pragma(warning(disable: 4793)); template <size_t _Size> inline int __cdecl _snprintf_s(     char (&_Dest)[_Size],      size_t _Size,         const char * _Format, ...) { va_list _ArgList; ( _ArgList = (va_list)( &reinterpret_cast<const char &>(_Format) ) + ( (sizeof(_Format) + sizeof(int) - 1) & ~(sizeof(int) - 1) ) ); return _vsnprintf_s(_Dest, _Size, _Size, _Format, _ArgList); } __pragma(warning(pop)); }
  int __cdecl sprintf_s(       char * _DstBuf,      size_t _DstSize,         const char * _Format, ...);
extern "C++" { __pragma(warning(push)); __pragma(warning(disable: 4793)); template <size_t _Size> inline int __cdecl sprintf_s(     char (&_Dest)[_Size],         const char * _Format, ...) { va_list _ArgList; ( _ArgList = (va_list)( &reinterpret_cast<const char &>(_Format) ) + ( (sizeof(_Format) + sizeof(int) - 1) & ~(sizeof(int) - 1) ) ); return vsprintf_s(_Dest, _Size, _Format, _ArgList); } __pragma(warning(pop)); }
  int __cdecl _scprintf(        const char * _Format, ...);
   int __cdecl sscanf(       const char * _Src,         const char * _Format, ...);
   int __cdecl _sscanf_l(       const char * _Src,         const char * _Format,        _locale_t _Locale, ...);

  int __cdecl sscanf_s(       const char * _Src,         const char * _Format, ...);
#line 316 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
  int __cdecl _sscanf_s_l(       const char * _Src,         const char * _Format,        _locale_t _Locale, ...);
   int __cdecl _snscanf(         const char * _Src,      size_t _MaxCount,         const char * _Format, ...);
   int __cdecl _snscanf_l(         const char * _Src,      size_t _MaxCount,         const char * _Format,        _locale_t _Locale, ...);
  int __cdecl _snscanf_s(         const char * _Src,      size_t _MaxCount,         const char * _Format, ...);
  int __cdecl _snscanf_s_l(         const char * _Src,      size_t _MaxCount,         const char * _Format,        _locale_t _Locale, ...);
   FILE * __cdecl tmpfile(void);

  errno_t __cdecl tmpfile_s(                FILE ** _File);
  errno_t __cdecl tmpnam_s(       char * _Buf,      rsize_t _Size);
#line 326 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl tmpnam_s(       char (&_Buf)[_Size]) { return tmpnam_s(_Buf, _Size); } }
  char * __cdecl tmpnam( char *_Buffer);
  int __cdecl ungetc(     int _Ch,      FILE * _File);
  int __cdecl vfprintf(     FILE * _File,         const char * _Format, va_list _ArgList);
  int __cdecl vfprintf_s(     FILE * _File,         const char * _Format, va_list _ArgList);
  int __cdecl vprintf(        const char * _Format, va_list _ArgList);
  int __cdecl vprintf_s(        const char * _Format, va_list _ArgList);
   int __cdecl vsnprintf(     char * _DstBuf,      size_t _MaxCount,         const char * _Format, va_list _ArgList);
  int __cdecl vsnprintf_s(       char * _DstBuf,      size_t _DstSize,      size_t _MaxCount,         const char * _Format, va_list _ArgList);
  int __cdecl _vsnprintf_s(       char * _DstBuf,      size_t _DstSize,      size_t _MaxCount,         const char * _Format, va_list _ArgList);
extern "C++" { template <size_t _Size> inline int __cdecl _vsnprintf_s(     char (&_Dest)[_Size],      size_t _Size,         const char * _Format, va_list _Args) { return _vsnprintf_s(_Dest, _Size, _Size, _Format, _Args); } }
#pragma warning(push)
#pragma warning(disable:4793)
  int __cdecl _snprintf(     char *_Dest,      size_t _Count,         const char * _Format, ...);   int __cdecl _vsnprintf(     char *_Dest,      size_t _Count,         const char * _Format, va_list _Args);
#pragma warning(pop)
 int __cdecl vsprintf_s(       char * _DstBuf,      size_t _Size,         const char * _Format, va_list _ArgList);
extern "C++" { template <size_t _Size> inline int __cdecl vsprintf_s(     char (&_Dest)[_Size],         const char * _Format, va_list _Args) { return vsprintf_s(_Dest, _Size, _Format, _Args); } }
#pragma warning(push)
#pragma warning(disable:4793)
  int __cdecl sprintf( char *_Dest,  const char * _Format, ...);   int __cdecl vsprintf( char *_Dest,  const char * _Format, va_list _Args);
#pragma warning(pop)
  int __cdecl _vscprintf(        const char * _Format, va_list _ArgList);
  int __cdecl _snprintf_c(     char * _DstBuf,      size_t _MaxCount,         const char * _Format, ...);
  int __cdecl _vsnprintf_c(     char *_DstBuf,      size_t _MaxCount,         const char * _Format, va_list _ArgList);

  int __cdecl _fprintf_p(     FILE * _File,         const char * _Format, ...);
  int __cdecl _printf_p(        const char * _Format, ...);
  int __cdecl _sprintf_p(       char * _Dst,      size_t _MaxCount,         const char * _Format, ...);
  int __cdecl _vfprintf_p(     FILE * _File,         const char * _Format, va_list _ArgList);
  int __cdecl _vprintf_p(        const char * _Format, va_list _ArgList);
  int __cdecl _vsprintf_p(       char * _Dst,      size_t _MaxCount,         const char * _Format, va_list _ArgList);
  int __cdecl _scprintf_p(        const char * _Format, ...);
  int __cdecl _vscprintf_p(        const char * _Format, va_list _ArgList);
 int __cdecl _set_printf_count_output(     int _Value);
 int __cdecl _get_printf_count_output();

  int __cdecl _printf_l(        const char * _Format,        _locale_t _Locale, ...);
  int __cdecl _printf_p_l(        const char * _Format,        _locale_t _Locale, ...);
  int __cdecl _printf_s_l(        const char * _Format,        _locale_t _Locale, ...);
  int __cdecl _vprintf_l(        const char * _Format,        _locale_t _Locale, va_list _ArgList);
  int __cdecl _vprintf_p_l(        const char * _Format,        _locale_t _Locale, va_list _ArgList);
  int __cdecl _vprintf_s_l(        const char * _Format,        _locale_t _Locale, va_list _ArgList);

  int __cdecl _fprintf_l(     FILE * _File,         const char * _Format,        _locale_t _Locale, ...);
  int __cdecl _fprintf_p_l(     FILE * _File,         const char * _Format,        _locale_t _Locale, ...);
  int __cdecl _fprintf_s_l(     FILE * _File,         const char * _Format,        _locale_t _Locale, ...);
  int __cdecl _vfprintf_l(     FILE * _File,        const char * _Format,        _locale_t _Locale, va_list _ArgList);
  int __cdecl _vfprintf_p_l(     FILE * _File,        const char * _Format,        _locale_t _Locale, va_list _ArgList);
  int __cdecl _vfprintf_s_l(     FILE * _File,        const char * _Format,        _locale_t _Locale, va_list _ArgList);

   int __cdecl _sprintf_l(     char * _DstBuf,         const char * _Format,        _locale_t _Locale, ...);
  int __cdecl _sprintf_p_l(       char * _DstBuf,      size_t _MaxCount,         const char * _Format,        _locale_t _Locale, ...);
  int __cdecl _sprintf_s_l(       char * _DstBuf,      size_t _DstSize,         const char * _Format,        _locale_t _Locale, ...);
   int __cdecl _vsprintf_l(     char * _DstBuf,        const char * _Format,        _locale_t, va_list _ArgList);
  int __cdecl _vsprintf_p_l(       char * _DstBuf,      size_t _MaxCount,         const char* _Format,        _locale_t _Locale,  va_list _ArgList);
  int __cdecl _vsprintf_s_l(       char * _DstBuf,      size_t _DstSize,         const char * _Format,        _locale_t _Locale, va_list _ArgList);

  int __cdecl _scprintf_l(        const char * _Format,        _locale_t _Locale, ...);
  int __cdecl _scprintf_p_l(        const char * _Format,        _locale_t _Locale, ...);
  int __cdecl _vscprintf_l(        const char * _Format,        _locale_t _Locale, va_list _ArgList);
  int __cdecl _vscprintf_p_l(        const char * _Format,        _locale_t _Locale, va_list _ArgList);

   int __cdecl _snprintf_l(     char * _DstBuf,      size_t _MaxCount,         const char * _Format,        _locale_t _Locale, ...);
  int __cdecl _snprintf_c_l(     char * _DstBuf,      size_t _MaxCount,         const char * _Format,        _locale_t _Locale, ...);
  int __cdecl _snprintf_s_l(       char * _DstBuf,      size_t _DstSize,      size_t _MaxCount,         const char * _Format,        _locale_t _Locale, ...);
   int __cdecl _vsnprintf_l(     char * _DstBuf,      size_t _MaxCount,         const char * _Format,        _locale_t _Locale, va_list _ArgList);
  int __cdecl _vsnprintf_c_l(     char * _DstBuf,      size_t _MaxCount, const char *,        _locale_t _Locale, va_list _ArgList);
  int __cdecl _vsnprintf_s_l(       char * _DstBuf,      size_t _DstSize,      size_t _MaxCount,         const char* _Format,       _locale_t _Locale, va_list _ArgList);












  FILE * __cdecl _wfsopen(       const wchar_t * _Filename,        const wchar_t * _Mode,      int _ShFlag);
#line 408 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"

  wint_t __cdecl fgetwc(     FILE * _File);
  wint_t __cdecl _fgetwchar(void);
  wint_t __cdecl fputwc(       wchar_t _Ch,      FILE * _File);
  wint_t __cdecl _fputwchar(       wchar_t _Ch);
  wint_t __cdecl getwc(     FILE * _File);
  wint_t __cdecl getwchar(void);
  wint_t __cdecl putwc(       wchar_t _Ch,      FILE * _File);
  wint_t __cdecl putwchar(       wchar_t _Ch);
  wint_t __cdecl ungetwc(     wint_t _Ch,      FILE * _File);

  wchar_t * __cdecl fgetws(       wchar_t * _Dst,      int _SizeInWords,      FILE * _File);
  int __cdecl fputws(       const wchar_t * _Str,      FILE * _File);
  wchar_t * __cdecl _getws_s(       wchar_t * _Str,      size_t _SizeInWords);
extern "C++" { template <size_t _Size> inline wchar_t * __cdecl _getws_s(     wchar_t (&_String)[_Size]) { return _getws_s(_String, _Size); } }
  wchar_t * __cdecl _getws( wchar_t *_String);
  int __cdecl _putws(       const wchar_t * _Str);

  int __cdecl fwprintf(     FILE * _File,         const wchar_t * _Format, ...);
  int __cdecl fwprintf_s(     FILE * _File,         const wchar_t * _Format, ...);
  int __cdecl wprintf(        const wchar_t * _Format, ...);
  int __cdecl wprintf_s(        const wchar_t * _Format, ...);
  int __cdecl _scwprintf(        const wchar_t * _Format, ...);
  int __cdecl vfwprintf(     FILE * _File,         const wchar_t * _Format, va_list _ArgList);
  int __cdecl vfwprintf_s(     FILE * _File,         const wchar_t * _Format, va_list _ArgList);
  int __cdecl vwprintf(        const wchar_t * _Format, va_list _ArgList);
  int __cdecl vwprintf_s(        const wchar_t * _Format, va_list _ArgList);

 int __cdecl swprintf_s(       wchar_t * _Dst,      size_t _SizeInWords,         const wchar_t * _Format, ...);
extern "C++" { __pragma(warning(push)); __pragma(warning(disable: 4793)); template <size_t _Size> inline int __cdecl swprintf_s(     wchar_t (&_Dest)[_Size],         const wchar_t * _Format, ...) { va_list _ArgList; ( _ArgList = (va_list)( &reinterpret_cast<const char &>(_Format) ) + ( (sizeof(_Format) + sizeof(int) - 1) & ~(sizeof(int) - 1) ) ); return vswprintf_s(_Dest, _Size, _Format, _ArgList); } __pragma(warning(pop)); }
 int __cdecl vswprintf_s(       wchar_t * _Dst,      size_t _SizeInWords,         const wchar_t * _Format, va_list _ArgList);
extern "C++" { template <size_t _Size> inline int __cdecl vswprintf_s(     wchar_t (&_Dest)[_Size],         const wchar_t * _Format, va_list _Args) { return vswprintf_s(_Dest, _Size, _Format, _Args); } }

  int __cdecl _swprintf_c(       wchar_t * _DstBuf,      size_t _SizeInWords,         const wchar_t * _Format, ...);
  int __cdecl _vswprintf_c(       wchar_t * _DstBuf,      size_t _SizeInWords,         const wchar_t * _Format, va_list _ArgList);

  int __cdecl _snwprintf_s(       wchar_t * _DstBuf,      size_t _DstSizeInWords,      size_t _MaxCount,         const wchar_t * _Format, ...);
extern "C++" { __pragma(warning(push)); __pragma(warning(disable: 4793)); template <size_t _Size> inline int __cdecl _snwprintf_s(     wchar_t (&_Dest)[_Size],      size_t _Count,         const wchar_t * _Format, ...) { va_list _ArgList; ( _ArgList = (va_list)( &reinterpret_cast<const char &>(_Format) ) + ( (sizeof(_Format) + sizeof(int) - 1) & ~(sizeof(int) - 1) ) ); return _vsnwprintf_s(_Dest, _Size, _Count, _Format, _ArgList); } __pragma(warning(pop)); }
  int __cdecl _vsnwprintf_s(       wchar_t * _DstBuf,      size_t _DstSizeInWords,      size_t _MaxCount,         const wchar_t * _Format, va_list _ArgList);
extern "C++" { template <size_t _Size> inline int __cdecl _vsnwprintf_s(     wchar_t (&_Dest)[_Size],      size_t _Count,         const wchar_t * _Format, va_list _Args) { return _vsnwprintf_s(_Dest, _Size, _Count, _Format, _Args); } }
#pragma warning(push)
#pragma warning(disable:4793)
  int __cdecl _snwprintf(     wchar_t *_Dest,      size_t _Count,         const wchar_t * _Format, ...);   int __cdecl _vsnwprintf(     wchar_t *_Dest,      size_t _Count,         const wchar_t * _Format, va_list _Args);
#pragma warning(pop)

  int __cdecl _fwprintf_p(     FILE * _File,         const wchar_t * _Format, ...);
  int __cdecl _wprintf_p(        const wchar_t * _Format, ...);
  int __cdecl _vfwprintf_p(     FILE * _File,         const wchar_t * _Format, va_list _ArgList);
  int __cdecl _vwprintf_p(        const wchar_t * _Format, va_list _ArgList);
  int __cdecl _swprintf_p(       wchar_t * _DstBuf,      size_t _MaxCount,         const wchar_t * _Format, ...);
  int __cdecl _vswprintf_p(       wchar_t * _DstBuf,      size_t _MaxCount,         const wchar_t * _Format, va_list _ArgList);
  int __cdecl _scwprintf_p(        const wchar_t * _Format, ...);
  int __cdecl _vscwprintf_p(        const wchar_t * _Format, va_list _ArgList);

  int __cdecl _wprintf_l(        const wchar_t * _Format,        _locale_t _Locale, ...);
  int __cdecl _wprintf_p_l(        const wchar_t * _Format,        _locale_t _Locale, ...);
  int __cdecl _wprintf_s_l(        const wchar_t * _Format,        _locale_t _Locale, ...);
  int __cdecl _vwprintf_l(        const wchar_t * _Format,        _locale_t _Locale, va_list _ArgList);
  int __cdecl _vwprintf_p_l(        const wchar_t * _Format,        _locale_t _Locale, va_list _ArgList);
  int __cdecl _vwprintf_s_l(        const wchar_t * _Format,        _locale_t _Locale, va_list _ArgList);

  int __cdecl _fwprintf_l(     FILE * _File,         const wchar_t * _Format,        _locale_t _Locale, ...);
  int __cdecl _fwprintf_p_l(     FILE * _File,         const wchar_t * _Format,        _locale_t _Locale, ...);
  int __cdecl _fwprintf_s_l(     FILE * _File,         const wchar_t * _Format,        _locale_t _Locale, ...);
  int __cdecl _vfwprintf_l(     FILE * _File,         const wchar_t * _Format,        _locale_t _Locale, va_list _ArgList);
  int __cdecl _vfwprintf_p_l(     FILE * _File,         const wchar_t * _Format,        _locale_t _Locale, va_list _ArgList);
  int __cdecl _vfwprintf_s_l(     FILE * _File,         const wchar_t * _Format,        _locale_t _Locale, va_list _ArgList);

  int __cdecl _swprintf_c_l(       wchar_t * _DstBuf,      size_t _MaxCount,         const wchar_t * _Format,        _locale_t _Locale, ...);
  int __cdecl _swprintf_p_l(       wchar_t * _DstBuf,      size_t _MaxCount,         const wchar_t * _Format,        _locale_t _Locale, ...);
  int __cdecl _swprintf_s_l(       wchar_t * _DstBuf,      size_t _DstSize,         const wchar_t * _Format,        _locale_t _Locale, ...);
  int __cdecl _vswprintf_c_l(       wchar_t * _DstBuf,      size_t _MaxCount,         const wchar_t * _Format,        _locale_t _Locale, va_list _ArgList);
  int __cdecl _vswprintf_p_l(       wchar_t * _DstBuf,      size_t _MaxCount,         const wchar_t * _Format,        _locale_t _Locale, va_list _ArgList);
  int __cdecl _vswprintf_s_l(       wchar_t * _DstBuf,      size_t _DstSize,         const wchar_t * _Format,        _locale_t _Locale, va_list _ArgList);

  int __cdecl _scwprintf_l(        const wchar_t * _Format,        _locale_t _Locale, ...);
  int __cdecl _scwprintf_p_l(        const wchar_t * _Format,        _locale_t _Locale, ...);
  int __cdecl _vscwprintf_p_l(        const wchar_t * _Format,        _locale_t _Locale, va_list _ArgList);

   int __cdecl _snwprintf_l(     wchar_t * _DstBuf,      size_t _MaxCount,         const wchar_t * _Format,        _locale_t _Locale, ...);
  int __cdecl _snwprintf_s_l(       wchar_t * _DstBuf,      size_t _DstSize,      size_t _MaxCount,         const wchar_t * _Format,        _locale_t _Locale, ...);
   int __cdecl _vsnwprintf_l(     wchar_t * _DstBuf,      size_t _MaxCount,         const wchar_t * _Format,        _locale_t _Locale, va_list _ArgList);
  int __cdecl _vsnwprintf_s_l(       wchar_t * _DstBuf,      size_t _DstSize,      size_t _MaxCount,         const wchar_t * _Format,        _locale_t _Locale, va_list _ArgList);










#line 502 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"


#pragma warning(push)
#pragma warning(disable:4141 4996 4793)
 __declspec(deprecated("swprintf has been changed to conform with the ISO C standard, adding an extra character count parameter. To use traditional Microsoft swprintf, set _CRT_NON_CONFORMING_SWPRINTFS."))  int __cdecl _swprintf(     wchar_t *_Dest,         const wchar_t * _Format, ...);  __declspec(deprecated("swprintf has been changed to conform with the ISO C standard, adding an extra character count parameter. To use traditional Microsoft swprintf, set _CRT_NON_CONFORMING_SWPRINTFS."))  int __cdecl _vswprintf(     wchar_t *_Dest,         const wchar_t * _Format, va_list _Args);
 __declspec(deprecated("swprintf has been changed to conform with the ISO C standard, adding an extra character count parameter. To use traditional Microsoft swprintf, set _CRT_NON_CONFORMING_SWPRINTFS."))  int __cdecl __swprintf_l(     wchar_t *_Dest,         const wchar_t * _Format, _locale_t _Plocinfo, ...);  __declspec(deprecated("swprintf has been changed to conform with the ISO C standard, adding an extra character count parameter. To use traditional Microsoft swprintf, set _CRT_NON_CONFORMING_SWPRINTFS."))  int __cdecl __vswprintf_l(     wchar_t *_Dest,         const wchar_t * _Format, _locale_t _Plocinfo, va_list _Args);
#pragma warning(pop)


#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\swprintf.inl"












#pragma once







#line 22 "c:\\program files\\microsoft visual studio 8\\vc\\include\\swprintf.inl"










#line 33 "c:\\program files\\microsoft visual studio 8\\vc\\include\\swprintf.inl"

#pragma warning( push )
#pragma warning( disable : 4793 4412 )
static __inline int swprintf(wchar_t * _String, size_t _Count, const wchar_t * _Format, ...)
{
    va_list _Arglist;
    int _Ret;
    ( _Arglist = (va_list)( &reinterpret_cast<const char &>(_Format) ) + ( (sizeof(_Format) + sizeof(int) - 1) & ~(sizeof(int) - 1) ) );
    _Ret = _vswprintf_c_l(_String, _Count, _Format, 0, _Arglist);
    ( _Arglist = (va_list)0 );
    return _Ret;
}
#pragma warning( pop )

#pragma warning( push )
#pragma warning( disable : 4412 )
static __inline int __cdecl vswprintf(wchar_t * _String, size_t _Count, const wchar_t * _Format, va_list _Ap)
{
    return _vswprintf_c_l(_String, _Count, _Format, 0, _Ap);
}
#pragma warning( pop )


#line 57 "c:\\program files\\microsoft visual studio 8\\vc\\include\\swprintf.inl"

#pragma warning( push )
#pragma warning( disable : 4793 4412 )
static __inline int _swprintf_l(wchar_t * _String, size_t _Count, const wchar_t * _Format, _locale_t _Plocinfo, ...)
{
    va_list _Arglist;
    int _Ret;
    ( _Arglist = (va_list)( &reinterpret_cast<const char &>(_Plocinfo) ) + ( (sizeof(_Plocinfo) + sizeof(int) - 1) & ~(sizeof(int) - 1) ) );
    _Ret = _vswprintf_c_l(_String, _Count, _Format, _Plocinfo, _Arglist);
    ( _Arglist = (va_list)0 );
    return _Ret;
}
#pragma warning( pop )

#pragma warning( push )
#pragma warning( disable : 4412 )
static __inline int __cdecl _vswprintf_l(wchar_t * _String, size_t _Count, const wchar_t * _Format, _locale_t _Plocinfo, va_list _Ap)
{
    return _vswprintf_c_l(_String, _Count, _Format, _Plocinfo, _Ap);
}
#pragma warning( pop )


#pragma warning( push )
#pragma warning( disable : 4996 )

#pragma warning( push )
#pragma warning( disable : 4793 4141 )
extern "C++" __declspec(deprecated("swprintf has been changed to conform with the ISO C standard, adding an extra character count parameter. To use traditional Microsoft swprintf, set _CRT_NON_CONFORMING_SWPRINTFS."))  __inline int swprintf(     wchar_t * _String,         const wchar_t * _Format, ...)
{
    va_list _Arglist;
    ( _Arglist = (va_list)( &reinterpret_cast<const char &>(_Format) ) + ( (sizeof(_Format) + sizeof(int) - 1) & ~(sizeof(int) - 1) ) );
    int _Ret = _vswprintf(_String, _Format, _Arglist);
    ( _Arglist = (va_list)0 );
    return _Ret;
}
#pragma warning( pop )

#pragma warning( push )
#pragma warning( disable : 4141 )
extern "C++" __declspec(deprecated("swprintf has been changed to conform with the ISO C standard, adding an extra character count parameter. To use traditional Microsoft swprintf, set _CRT_NON_CONFORMING_SWPRINTFS."))  __inline int __cdecl vswprintf(     wchar_t * _String,         const wchar_t * _Format, va_list _Ap)
{
    return _vswprintf(_String, _Format, _Ap);
}
#pragma warning( pop )

#pragma warning( push )
#pragma warning( disable : 4793 4141 )
extern "C++" __declspec(deprecated("swprintf has been changed to conform with the ISO C standard, adding an extra character count parameter. To use traditional Microsoft swprintf, set _CRT_NON_CONFORMING_SWPRINTFS."))  __inline int _swprintf_l(     wchar_t * _String,         const wchar_t * _Format, _locale_t _Plocinfo, ...)
{
    va_list _Arglist;
    ( _Arglist = (va_list)( &reinterpret_cast<const char &>(_Plocinfo) ) + ( (sizeof(_Plocinfo) + sizeof(int) - 1) & ~(sizeof(int) - 1) ) );
    int _Ret = __vswprintf_l(_String, _Format, _Plocinfo, _Arglist);
    ( _Arglist = (va_list)0 );
    return _Ret;
}
#pragma warning( pop )

#pragma warning( push )
#pragma warning( disable : 4141 )
extern "C++" __declspec(deprecated("swprintf has been changed to conform with the ISO C standard, adding an extra character count parameter. To use traditional Microsoft swprintf, set _CRT_NON_CONFORMING_SWPRINTFS."))  __inline int __cdecl _vswprintf_l(     wchar_t * _String,         const wchar_t * _Format, _locale_t _Plocinfo, va_list _Ap)
{
    return __vswprintf_l(_String, _Format, _Plocinfo, _Ap);
}
#pragma warning( pop )

#pragma warning( pop )

#line 126 "c:\\program files\\microsoft visual studio 8\\vc\\include\\swprintf.inl"

#line 128 "c:\\program files\\microsoft visual studio 8\\vc\\include\\swprintf.inl"
#line 129 "c:\\program files\\microsoft visual studio 8\\vc\\include\\swprintf.inl"

#line 512 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
#line 513 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"













#line 527 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"

  wchar_t * __cdecl _wtempnam(         const wchar_t * _Directory,          const wchar_t * _FilePrefix);



#line 533 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"

  int __cdecl _vscwprintf(        const wchar_t * _Format, va_list _ArgList);
  int __cdecl _vscwprintf_l(        const wchar_t * _Format,        _locale_t _Locale, va_list _ArgList);
   int __cdecl fwscanf(     FILE * _File,         const wchar_t * _Format, ...);
   int __cdecl _fwscanf_l(     FILE * _File,         const wchar_t * _Format,        _locale_t _Locale, ...);

  int __cdecl fwscanf_s(     FILE * _File,         const wchar_t * _Format, ...);
#line 541 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
  int __cdecl _fwscanf_s_l(     FILE * _File,         const wchar_t * _Format,        _locale_t _Locale, ...);
   int __cdecl swscanf(       const wchar_t * _Src,         const wchar_t * _Format, ...);
   int __cdecl _swscanf_l(       const wchar_t * _Src,         const wchar_t * _Format,        _locale_t _Locale, ...);

  int __cdecl swscanf_s(       const wchar_t *_Src,         const wchar_t * _Format, ...);
#line 547 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
  int __cdecl _swscanf_s_l(       const wchar_t * _Src,         const wchar_t * _Format,        _locale_t _Locale, ...);
   int __cdecl _snwscanf(         const wchar_t * _Src,      size_t _MaxCount,         const wchar_t * _Format, ...);
   int __cdecl _snwscanf_l(         const wchar_t * _Src,      size_t _MaxCount,         const wchar_t * _Format,        _locale_t _Locale, ...);
  int __cdecl _snwscanf_s(         const wchar_t * _Src,      size_t _MaxCount,         const wchar_t * _Format, ...);
  int __cdecl _snwscanf_s_l(         const wchar_t * _Src,      size_t _MaxCount,         const wchar_t * _Format,        _locale_t _Locale, ...);
   int __cdecl wscanf(        const wchar_t * _Format, ...);
   int __cdecl _wscanf_l(        const wchar_t * _Format,        _locale_t _Locale, ...);

  int __cdecl wscanf_s(        const wchar_t * _Format, ...);
#line 557 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
  int __cdecl _wscanf_s_l(        const wchar_t * _Format,        _locale_t _Locale, ...);

  FILE * __cdecl _wfdopen(     int _FileHandle ,        const wchar_t * _Mode);
   FILE * __cdecl _wfopen(       const wchar_t * _Filename,        const wchar_t * _Mode);
  errno_t __cdecl _wfopen_s(                  FILE ** _File,        const wchar_t * _Filename,        const wchar_t * _Mode);
   FILE * __cdecl _wfreopen(       const wchar_t * _Filename,        const wchar_t * _Mode,      FILE * _OldFile);
  errno_t __cdecl _wfreopen_s(                  FILE ** _File,        const wchar_t * _Filename,        const wchar_t * _Mode,      FILE * _OldFile);





  FILE * __cdecl _wpopen(       const wchar_t *_Command,        const wchar_t * _Mode);
  int __cdecl _wremove(       const wchar_t * _Filename);
  errno_t __cdecl _wtmpnam_s(       wchar_t * _DstBuf,      size_t _SizeInWords);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wtmpnam_s(       wchar_t (&_Buffer)[_Size]) { return _wtmpnam_s(_Buffer, _Size); } }
  wchar_t * __cdecl _wtmpnam( wchar_t *_Buffer);

  wint_t __cdecl _fgetwc_nolock(     FILE * _File);
  wint_t __cdecl _fputwc_nolock(       wchar_t _Ch,      FILE * _File);
  wint_t __cdecl _ungetwc_nolock(     wint_t _Ch,      FILE * _File);






#line 585 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"
inline wint_t __cdecl getwchar()
        {return (fgetwc((&__iob_func()[0]))); }   
inline wint_t __cdecl putwchar(wchar_t _C)
        {return (fputwc(_C, (&__iob_func()[1]))); }       
#line 590 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"










#line 601 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"


#line 604 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"


#line 607 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"















#line 623 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"












 void __cdecl _lock_file(     FILE * _File);
 void __cdecl _unlock_file(     FILE * _File);



#line 641 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"


  int __cdecl _fclose_nolock(     FILE * _File);
  int __cdecl _fflush_nolock(       FILE * _File);
  size_t __cdecl _fread_nolock(     void * _DstBuf,      size_t _ElementSize,      size_t _Count,      FILE * _File);
  size_t __cdecl _fread_nolock_s(     void * _DstBuf,      size_t _DstSize,      size_t _ElementSize,      size_t _Count,      FILE * _File);
  int __cdecl _fseek_nolock(     FILE * _File,      long _Offset,      int _Origin);
  long __cdecl _ftell_nolock(     FILE * _File);
  int __cdecl _fseeki64_nolock(     FILE * _File,      __int64 _Offset,      int _Origin);
  __int64 __cdecl _ftelli64_nolock(     FILE * _File);
  size_t __cdecl _fwrite_nolock(       const void * _DstBuf,      size_t _Size,      size_t _Count,      FILE * _File);
  int __cdecl _ungetc_nolock(     int _Ch,      FILE * _File);












#line 666 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"











#line 678 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"

  char * __cdecl tempnam(         const char * _Directory,          const char * _FilePrefix);



#line 684 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"

   int __cdecl fcloseall(void);
   FILE * __cdecl fdopen(     int _FileHandle,         const char * _Format);
   int __cdecl fgetchar(void);
   int __cdecl fileno(     FILE * _File);
   int __cdecl flushall(void);
   int __cdecl fputchar(     int _Ch);
   int __cdecl getw(     FILE * _File);
   int __cdecl putw(     int _Ch,      FILE * _File);
   int __cdecl rmtmp(void);

#line 696 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"


}
#line 700 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"


#pragma pack(pop)
#line 704 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"

#line 706 "c:\\program files\\microsoft visual studio 8\\vc\\include\\stdio.h"

#line 222 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"
#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"
















#pragma once
#line 19 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"




#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"














 















































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 24 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"



#line 28 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"







#pragma pack(push,8)
#line 37 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"


extern "C" {
#line 41 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"








#line 50 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"
















#line 67 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"




























typedef long clock_t;

#line 98 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"





















struct tm {
        int tm_sec;     
        int tm_min;     
        int tm_hour;    
        int tm_mday;    
        int tm_mon;     
        int tm_year;    
        int tm_wday;    
        int tm_yday;    
        int tm_isdst;   
        };

#line 132 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"












   int* __cdecl __daylight(void);



   long* __cdecl __dstbias(void);



   long* __cdecl __timezone(void);



          char ** __cdecl __tzname(void);


 errno_t __cdecl _get_daylight(     int * _Daylight);
 errno_t __cdecl _get_dstbias(     long * _Daylight_savings_bias);
 errno_t __cdecl _get_timezone(     long * _Timezone);
 errno_t __cdecl _get_tzname(     size_t *_ReturnValue,        char *_Buffer,      size_t _SizeInBytes,      int _Index);



   char * __cdecl asctime(     const struct tm * _Tm);

  errno_t __cdecl asctime_s(       char *_Buf,      size_t _SizeInBytes,      const struct tm * _Tm);
#line 170 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl asctime_s(       char (&_Buffer)[_Size],      const struct tm * _Time) { return asctime_s(_Buffer, _Size, _Time); } }

  char * __cdecl _ctime32(     const __time32_t * _Time);
 errno_t __cdecl _ctime32_s(       char *_Buf,      size_t _SizeInBytes,      const __time32_t *_Time);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _ctime32_s(       char (&_Buffer)[_Size],      const __time32_t * _Time) { return _ctime32_s(_Buffer, _Size, _Time); } }

  clock_t __cdecl clock(void);
 double __cdecl _difftime32(     __time32_t _Time1,      __time32_t _Time2);

   struct tm * __cdecl _gmtime32(     const __time32_t * _Time);
  errno_t __cdecl _gmtime32_s(     struct tm *_Tm,      const __time32_t * _Time);

  struct tm * __cdecl _localtime32(     const __time32_t * _Time);
 errno_t __cdecl _localtime32_s(     struct tm *_Tm,      const __time32_t * _Time);

 size_t __cdecl strftime(       char * _Buf,      size_t _SizeInBytes,         const char * _Format,      const struct tm * _Tm);
 size_t __cdecl _strftime_l(     char *_Buf,      size_t _Max_size,         const char * _Format,      const struct tm *_Tm,        _locale_t _Locale);

  errno_t __cdecl _strdate_s(       char *_Buf,      size_t _SizeInBytes);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _strdate_s(       char (&_Buffer)[_Size]) { return _strdate_s(_Buffer, _Size); } }
  char * __cdecl _strdate( char *_Buffer);

  errno_t __cdecl _strtime_s(       char *_Buf ,      size_t _SizeInBytes);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _strtime_s(       char (&_Buffer)[_Size]) { return _strtime_s(_Buffer, _Size); } }
  char * __cdecl _strtime( char *_Buffer);

 __time32_t __cdecl _time32(       __time32_t * _Time);
 __time32_t __cdecl _mktime32(     struct tm * _Tm);
 __time32_t __cdecl _mkgmtime32(     struct tm * _Tm);




 void __cdecl _tzset(void);
#line 205 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"


  double __cdecl _difftime64(     __time64_t _Time1,      __time64_t _Time2);
  char * __cdecl _ctime64(     const __time64_t * _Time);
 errno_t __cdecl _ctime64_s(       char *_Buf,      size_t _SizeInBytes,      const __time64_t * _Time);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _ctime64_s(     char (&_Buffer)[_Size],      const __time64_t * _Time) { return _ctime64_s(_Buffer, _Size, _Time); } }

  struct tm * __cdecl _gmtime64(     const __time64_t * _Time);
 errno_t __cdecl _gmtime64_s(     struct tm *_Tm,      const __time64_t *_Time);

  struct tm * __cdecl _localtime64(     const __time64_t * _Time);
 errno_t __cdecl _localtime64_s(     struct tm *_Tm,      const __time64_t *_Time);

 __time64_t __cdecl _mktime64(     struct tm * _Tm);
 __time64_t __cdecl _mkgmtime64(     struct tm * _Tm);
 __time64_t __cdecl _time64(       __time64_t * _Time);
#line 222 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"


__declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "GetLocalTime" "instead. See online help for details.")) unsigned __cdecl _getsystime(     struct tm * _Tm);
__declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using" "SetLocalTime" "instead. See online help for details.")) unsigned __cdecl _setsystime(     struct tm * _Tm, unsigned _MilliSec);










 
  wchar_t * __cdecl _wasctime(     const struct tm * _Tm);
 errno_t __cdecl _wasctime_s(       wchar_t *_Buf,      size_t _SizeInWords,      const struct tm * _Tm);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wasctime_s(       wchar_t (&_Buffer)[_Size],      const struct tm * _Time) { return _wasctime_s(_Buffer, _Size, _Time); } }

  wchar_t * __cdecl _wctime32(     const __time32_t *_Time);
 errno_t __cdecl _wctime32_s(       wchar_t* _Buf,      size_t _SizeInWords,      const __time32_t * _Time);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wctime32_s(       wchar_t (&_Buffer)[_Size],      const __time32_t * _Time) { return _wctime32_s(_Buffer, _Size, _Time); } }

 size_t __cdecl wcsftime(       wchar_t * _Buf,      size_t _SizeInWords,         const wchar_t * _Format,       const struct tm * _Tm);
 size_t __cdecl _wcsftime_l(       wchar_t * _Buf,      size_t _SizeInWords,         const wchar_t *_Format,      const struct tm *_Tm,        _locale_t _Locale);

 errno_t __cdecl _wstrdate_s(       wchar_t * _Buf,      size_t _SizeInWords);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wstrdate_s(       wchar_t (&_Buffer)[_Size]) { return _wstrdate_s(_Buffer, _Size); } }
  wchar_t * __cdecl _wstrdate( wchar_t *_Buffer);

 errno_t __cdecl _wstrtime_s(       wchar_t * _Buf,      size_t _SizeInWords);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wstrtime_s(       wchar_t (&_Buffer)[_Size]) { return _wstrtime_s(_Buffer, _Size); } }
  wchar_t * __cdecl _wstrtime( wchar_t *_Buffer);


  wchar_t * __cdecl _wctime64(     const __time64_t * _Time);
 errno_t __cdecl _wctime64_s(       wchar_t* _Buf,      size_t _SizeInWords,      const __time64_t *_Time);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wctime64_s(       wchar_t (&_Buffer)[_Size],      const __time64_t * _Time) { return _wctime64_s(_Buffer, _Size, _Time); } }
#line 261 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"


#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\wtime.inl"













#pragma once
#line 16 "c:\\program files\\microsoft visual studio 8\\vc\\include\\wtime.inl"







#line 24 "c:\\program files\\microsoft visual studio 8\\vc\\include\\wtime.inl"





#pragma warning(push)
#pragma warning(disable:4996)















static __inline wchar_t * __cdecl _wctime(const time_t * _Time)
{
#pragma warning( push )
#pragma warning( disable : 4996 )
    return _wctime64(_Time);
#pragma warning( pop )
}

static __inline errno_t __cdecl _wctime_s(wchar_t *_Buffer, size_t _SizeInWords, const time_t * _Time)
{
    return _wctime64_s(_Buffer, _SizeInWords, _Time);
}
#line 59 "c:\\program files\\microsoft visual studio 8\\vc\\include\\wtime.inl"

#pragma warning(pop)

#line 63 "c:\\program files\\microsoft visual studio 8\\vc\\include\\wtime.inl"
#line 64 "c:\\program files\\microsoft visual studio 8\\vc\\include\\wtime.inl"
#line 264 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"
#line 265 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"


#line 268 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"


#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.inl"













#pragma once
#line 16 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.inl"







#line 24 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.inl"




























































static __inline double __cdecl difftime(time_t _Time1, time_t _Time2)
{
    return _difftime64(_Time1,_Time2);
}
 static __inline char * __cdecl ctime(const time_t * _Time)
{
#pragma warning( push )
#pragma warning( disable : 4996 )
    return _ctime64(_Time);
#pragma warning( pop )
}

static __inline errno_t __cdecl ctime_s(char *_Buffer, size_t _SizeInBytes, const time_t * _Time)
{
    return _ctime64_s(_Buffer, _SizeInBytes, _Time);
}
#line 101 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.inl"
 static __inline struct tm * __cdecl gmtime(const time_t * _Time)
{
#pragma warning( push )
#pragma warning( disable : 4996 )
    return _gmtime64(_Time);
#pragma warning( pop )
}

static __inline errno_t __cdecl gmtime_s(struct tm * _Tm, const time_t * _Time)
{
    return _gmtime64_s(_Tm, _Time);
}
#line 114 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.inl"
 static __inline struct tm * __cdecl localtime(const time_t * _Time)
{
#pragma warning( push )
#pragma warning( disable : 4996 )
    return _localtime64(_Time);
#pragma warning( pop )
}
static __inline errno_t __cdecl localtime_s(struct tm * _Tm, const time_t * _Time)
{
    return _localtime64_s(_Tm, _Time);
}
static __inline time_t __cdecl mktime(struct tm * _Tm)
{
    return _mktime64(_Tm);
}
static __inline time_t __cdecl _mkgmtime(struct tm * _Tm)
{
    return _mkgmtime64(_Tm);
}
static __inline time_t __cdecl time(time_t * _Time)
{
    return _time64(_Time);
}
#line 138 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.inl"


#line 141 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.inl"
#line 142 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.inl"
#line 271 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"
#line 272 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"













  extern int daylight;
  extern long timezone;
  extern char * tzname[2];
#line 289 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"

  void __cdecl tzset(void);

#line 293 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"



}
#line 298 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"


#pragma pack(pop)
#line 302 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"

#line 304 "c:\\program files\\microsoft visual studio 8\\vc\\include\\time.h"
#line 223 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"
#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"

















#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"














 















































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 19 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"






#pragma pack(push,8)
#line 27 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"


extern "C" {
#line 31 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"








struct _exception {
        int type;       
        char *name;     
        double arg1;    
        double arg2;    
        double retval;  
        } ;


#line 49 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"







struct _complex {
        double x,y; 
        } ;




#line 64 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"


#line 67 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"
#line 68 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"






















 extern double _HUGE;


#line 94 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"
#line 95 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"












        double  __cdecl acos(     double _X);
        double  __cdecl asin(     double _X);
        double  __cdecl atan(     double _X);
        double  __cdecl atan2(     double _Y,      double _X);

  double __cdecl _copysign (     double _Number,      double _Sign);
  double __cdecl _chgsign (     double _X);

#line 116 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"
        double  __cdecl cos(     double _X);
        double  __cdecl cosh(     double _X);
        double  __cdecl exp(     double _X);
 double  __cdecl fabs(     double _X);
        double  __cdecl fmod(     double _X,      double _Y);
        double  __cdecl log(     double _X);
        double  __cdecl log10(     double _X);
        double  __cdecl pow(     double _X,      double _Y);
        double  __cdecl sin(     double _X);
        double  __cdecl sinh(     double _X);
        double  __cdecl tan(     double _X);
        double  __cdecl tanh(     double _X);
        double  __cdecl sqrt(     double _X);






 double  __cdecl _cabs(     struct _complex _Complex);
 double  __cdecl ceil(     double _X);
 double  __cdecl floor(     double _X);
 double  __cdecl frexp(     double _X,      int * _Y);
 double  __cdecl _hypot(     double _X,      double _Y);
 double  __cdecl _j0(     double _X );
 double  __cdecl _j1(     double _X );
 double  __cdecl _jn(int _X,      double _Y);
 double  __cdecl ldexp(     double _X,      int _Y);




#line 149 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"
        int     __cdecl _matherr(     struct _exception * _Except);
#line 151 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"
#line 152 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"
 double  __cdecl modf(     double _X,      double * _Y);

 double  __cdecl _y0(     double _X);
 double  __cdecl _y1(     double _X);
 double  __cdecl _yn(     int _X,      double _Y);




 int     __cdecl _set_SSE2_enable(     int _Flag);
 float  __cdecl _hypotf(     float _X,      float _Y);

#line 165 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"







































#line 205 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"













































#line 251 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"


































































inline long double acosl(     long double _X)
        {return (acos((double)_X)); }
inline long double asinl(     long double _X)
        {return (asin((double)_X)); }
inline long double atanl(     long double _X)
        {return (atan((double)_X)); }
inline long double atan2l(     long double _X,      long double _Y)
        {return (atan2((double)_X, (double)_Y)); }
inline long double ceill(     long double _X)
        {return (ceil((double)_X)); }
inline long double cosl(     long double _X)
        {return (cos((double)_X)); }
inline long double coshl(     long double _X)
        {return (cosh((double)_X)); }
inline long double expl(     long double _X)
        {return (exp((double)_X)); }
inline long double fabsl(     long double _X)
        {return (fabs((double)_X)); }
inline long double floorl(     long double _X)
        {return (floor((double)_X)); }
inline long double fmodl(     long double _X,      long double _Y)
        {return (fmod((double)_X, (double)_Y)); }
inline long double frexpl(     long double _X,      int *_Y)
        {return (frexp((double)_X, _Y)); }
inline long double ldexpl(     long double _X,      int _Y)
        {return (ldexp((double)_X, _Y)); }
inline long double logl(     long double _X)
        {return (log((double)_X)); }
inline long double log10l(     long double _X)
        {return (log10((double)_X)); }
inline long double modfl(     long double _X,      long double *_Y)
        {double _Di, _Df = modf((double)_X, &_Di);
        *_Y = (long double)_Di;
        return (_Df); }
inline long double powl(     long double _X,      long double _Y)
        {return (pow((double)_X, (double)_Y)); }
inline long double sinl(     long double _X)
        {return (sin((double)_X)); }
inline long double sinhl(     long double _X)
        {return (sinh((double)_X)); }
inline long double sqrtl(     long double _X)
        {return (sqrt((double)_X)); }

inline long double tanl(     long double _X)
        {return (tan((double)_X)); }


#line 365 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"

inline long double tanhl(     long double _X)
        {return (tanh((double)_X)); }

inline long double _chgsignl(     long double _Number)
{
    return _chgsign(static_cast<double>(_Number)); 
}

inline long double _copysignl(     long double _Number,      long double _Sign)
{
    return _copysign(static_cast<double>(_Number), static_cast<double>(_Sign)); 
}

inline float frexpf(     float _X,      int *_Y)
        {return ((float)frexp((double)_X, _Y)); }


inline float fabsf(     float _X)
        {return ((float)fabs((double)_X)); }
inline float ldexpf(     float _X,      int _Y)
        {return ((float)ldexp((double)_X, _Y)); }

inline float acosf(     float _X)
        {return ((float)acos((double)_X)); }
inline float asinf(     float _X)
        {return ((float)asin((double)_X)); }
inline float atanf(     float _X)
        {return ((float)atan((double)_X)); }
inline float atan2f(     float _X,      float _Y)
        {return ((float)atan2((double)_X, (double)_Y)); }
inline float ceilf(     float _X)
        {return ((float)ceil((double)_X)); }
inline float cosf(     float _X)
        {return ((float)cos((double)_X)); }
inline float coshf(     float _X)
        {return ((float)cosh((double)_X)); }
inline float expf(     float _X)
        {return ((float)exp((double)_X)); }
inline float floorf(     float _X)
        {return ((float)floor((double)_X)); }
inline float fmodf(     float _X,      float _Y)
        {return ((float)fmod((double)_X, (double)_Y)); }
inline float logf(     float _X)
        {return ((float)log((double)_X)); }
inline float log10f(     float _X)
        {return ((float)log10((double)_X)); }
inline float modff(     float _X,      float *_Y)
        { double _Di, _Df = modf((double)_X, &_Di);
        *_Y = (float)_Di;
        return ((float)_Df); }
inline float powf(     float _X,      float _Y)
        {return ((float)pow((double)_X, (double)_Y)); }
inline float sinf(     float _X)
        {return ((float)sin((double)_X)); }
inline float sinhf(     float _X)
        {return ((float)sinh((double)_X)); }
inline float sqrtf(     float _X)
        {return ((float)sqrt((double)_X)); }
inline float tanf(     float _X)
        {return ((float)tan((double)_X)); }
inline float tanhf(     float _X)
        {return ((float)tanh((double)_X)); }
#line 429 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"
#line 430 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"
#line 431 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"
#line 432 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"

















 extern double HUGE;


#line 453 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"

  double  __cdecl cabs(     struct _complex _X);
  double  __cdecl hypot(     double _X,      double _Y);
  double  __cdecl j0(     double _X);
  double  __cdecl j1(     double _X);
  double  __cdecl jn(     int _X,      double _Y);
  double  __cdecl y0(     double _X);
  double  __cdecl y1(     double _X);
  double  __cdecl yn(     int _X,      double _Y);

#line 464 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"

#line 466 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"


}

extern "C++" {

template<class _Ty> inline
        _Ty _Pow_int(_Ty _X, int _Y)
        {unsigned int _N;
        if (_Y >= 0)
                _N = (unsigned int)_Y;
        else
                _N = (unsigned int)(-_Y);
        for (_Ty _Z = _Ty(1); ; _X *= _X)
                {if ((_N & 1) != 0)
                        _Z *= _X;
                if ((_N >>= 1) == 0)
                        return (_Y < 0 ? _Ty(1) / _Z : _Z); }}

inline long __cdecl abs(     long _X)
        {return (labs(_X)); }
inline double __cdecl abs(     double _X)
        {return (fabs(_X)); }
inline double __cdecl pow(     double _X,      int _Y)
        {return (_Pow_int(_X, _Y)); }
inline float __cdecl abs(     float _X)
        {return (fabsf(_X)); }
inline float __cdecl acos(     float _X)
        {return (acosf(_X)); }
inline float __cdecl asin(     float _X)
        {return (asinf(_X)); }
inline float __cdecl atan(     float _X)
        {return (atanf(_X)); }
inline float __cdecl atan2(     float _Y,      float _X)
        {return (atan2f(_Y, _X)); }
inline float __cdecl ceil(     float _X)
        {return (ceilf(_X)); }
inline float __cdecl cos(     float _X)
        {return (cosf(_X)); }
inline float __cdecl cosh(     float _X)
        {return (coshf(_X)); }
inline float __cdecl exp(     float _X)
        {return (expf(_X)); }
inline float __cdecl fabs(     float _X)
        {return (fabsf(_X)); }
inline float __cdecl floor(     float _X)
        {return (floorf(_X)); }
inline float __cdecl fmod(     float _X,      float _Y)
        {return (fmodf(_X, _Y)); }
inline float __cdecl frexp(     float _X,      int * _Y)
        {return (frexpf(_X, _Y)); }
inline float __cdecl ldexp(     float _X,      int _Y)
        {return (ldexpf(_X, _Y)); }
inline float __cdecl log(     float _X)
        {return (logf(_X)); }
inline float __cdecl log10(     float _X)
        {return (log10f(_X)); }
inline float __cdecl modf(     float _X,      float * _Y)
        {return (modff(_X, _Y)); }
inline float __cdecl pow(     float _X,      float _Y)
        {return (powf(_X, _Y)); }
inline float __cdecl pow(     float _X,      int _Y)
        {return (_Pow_int(_X, _Y)); }
inline float __cdecl sin(     float _X)
        {return (sinf(_X)); }
inline float __cdecl sinh(     float _X)
        {return (sinhf(_X)); }
inline float __cdecl sqrt(     float _X)
        {return (sqrtf(_X)); }
inline float __cdecl tan(     float _X)
        {return (tanf(_X)); }
inline float __cdecl tanh(     float _X)
        {return (tanhf(_X)); }
inline long double __cdecl abs(     long double _X)
        {return (fabsl(_X)); }
inline long double __cdecl acos(     long double _X)
        {return (acosl(_X)); }
inline long double __cdecl asin(     long double _X)
        {return (asinl(_X)); }
inline long double __cdecl atan(     long double _X)
        {return (atanl(_X)); }
inline long double __cdecl atan2(     long double _Y,      long double _X)
        {return (atan2l(_Y, _X)); }
inline long double __cdecl ceil(     long double _X)
        {return (ceill(_X)); }
inline long double __cdecl cos(     long double _X)
        {return (cosl(_X)); }
inline long double __cdecl cosh(     long double _X)
        {return (coshl(_X)); }
inline long double __cdecl exp(     long double _X)
        {return (expl(_X)); }
inline long double __cdecl fabs(     long double _X)
        {return (fabsl(_X)); }
inline long double __cdecl floor(     long double _X)
        {return (floorl(_X)); }
inline long double __cdecl fmod(     long double _X,      long double _Y)
        {return (fmodl(_X, _Y)); }
inline long double __cdecl frexp(     long double _X,      int * _Y)
        {return (frexpl(_X, _Y)); }
inline long double __cdecl ldexp(     long double _X,      int _Y)
        {return (ldexpl(_X, _Y)); }
inline long double __cdecl log(     long double _X)
        {return (logl(_X)); }
inline long double __cdecl log10(     long double _X)
        {return (log10l(_X)); }
inline long double __cdecl modf(     long double _X,      long double * _Y)
        {return (modfl(_X, _Y)); }
inline long double __cdecl pow(     long double _X,      long double _Y)
        {return (powl(_X, _Y)); }
inline long double __cdecl pow(     long double _X,      int _Y)
        {return (_Pow_int(_X, _Y)); }
inline long double __cdecl sin(     long double _X)
        {return (sinl(_X)); }
inline long double __cdecl sinh(     long double _X)
        {return (sinhl(_X)); }
inline long double __cdecl sqrt(     long double _X)
        {return (sqrtl(_X)); }
inline long double __cdecl tan(     long double _X)
        {return (tanl(_X)); }
inline long double __cdecl tanh(     long double _X)
        {return (tanhl(_X)); }

}
#line 590 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"


#pragma pack(pop)
#line 594 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"

#line 596 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"







































#line 636 "c:\\program files\\microsoft visual studio 8\\vc\\include\\math.h"

#line 224 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"
#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\assert.h"













#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"














 















































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 15 "c:\\program files\\microsoft visual studio 8\\vc\\include\\assert.h"










extern "C" {
#line 27 "c:\\program files\\microsoft visual studio 8\\vc\\include\\assert.h"

 void __cdecl _wassert(       const wchar_t * _Message,        const wchar_t *_File,      unsigned _Line);


}
#line 33 "c:\\program files\\microsoft visual studio 8\\vc\\include\\assert.h"



#line 37 "c:\\program files\\microsoft visual studio 8\\vc\\include\\assert.h"
#line 225 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"




inline uint1 MCS_tolower(uint1 p_char) {return ( ((0)!=0 && (((_locale_t)(0))->locinfo->mb_cur_max) > 1) ? _isctype_l(p_char, (0x1), 0) : _chvalidator_l(0, p_char, 0x1)) ? _tolower_l(p_char, 0) : p_char;}
inline uint1 MCS_toupper(uint1 p_char) {return _toupper_l(p_char, 0);}





















class CDropTarget;




typedef uintptr_t MCSocketHandle;

inline void *operator new(size_t, void *p)
{
	return p;
}

typedef struct __MCWinSysHandle *MCWinSysHandle;
typedef struct __MCWinSysIconHandle *MCWinSysIconHandle;
typedef struct __MCWinSysMetafileHandle *MCWinSysMetafileHandle;
typedef struct __MCWinSysEnhMetafileHandle *MCWinSysEnhMetafileHandle;



#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"













#pragma once
#line 16 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"

#line 1 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdefs.h"














 















































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 18 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"





#pragma pack(push,8)
#line 25 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"












extern "C" {
#line 39 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"

 





typedef void *_HFILE; 


















typedef int (__cdecl * _CRT_REPORT_HOOK)(int, char *, int *);
typedef int (__cdecl * _CRT_REPORT_HOOKW)(int, wchar_t *, int *);



#line 71 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"




#line 76 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"




 





 










typedef int (__cdecl * _CRT_ALLOC_HOOK)(int, void *, size_t, int, long, const unsigned char *, int);


#line 101 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"


#line 104 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"

 





































 














typedef void (__cdecl * _CRT_DUMP_CLIENT)(void *, size_t);


#line 162 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"


#line 165 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"

struct _CrtMemBlockHeader;
typedef struct _CrtMemState
{
        struct _CrtMemBlockHeader * pBlockHeader;
        size_t lCounts[5];
        size_t lSizes[5];
        size_t lHighWaterCount;
        size_t lTotalCount;
} _CrtMemState;


 











#line 190 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"







































































































































 
























 






 extern long _crtAssertBusy;
#line 359 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"


 _CRT_REPORT_HOOK __cdecl _CrtGetReportHook(
    void
    );
#line 365 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"






 _CRT_REPORT_HOOK __cdecl _CrtSetReportHook(
           _CRT_REPORT_HOOK _PFnNewHook
        );

 int __cdecl _CrtSetReportHook2(
             int _Mode,
               _CRT_REPORT_HOOK _PFnNewHook
        );

 int __cdecl _CrtSetReportHookW2(
             int _Mode,
               _CRT_REPORT_HOOKW _PFnNewHook
        );


















#line 403 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"



























#line 431 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"


 int __cdecl _CrtSetReportMode(
             int _ReportType,
               int _ReportMode 
        );

 _HFILE __cdecl _CrtSetReportFile(
             int _ReportType,
               _HFILE _ReportFile 
        );

 int __cdecl _CrtDbgReport(
             int _ReportType,
                 const char * _Filename,
             int _Linenumber,
                 const char * _ModuleName,
                 const char * _Format,
        ...);

 size_t __cdecl _CrtSetDebugFillThreshold(
             size_t _NewDebugFillThreshold
        );



#line 458 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"
 int __cdecl _CrtDbgReportW(
             int _ReportType,
                 const wchar_t * _Filename,
             int _LineNumber,
                 const wchar_t * _ModuleName,
                 const wchar_t * _Format,
        ...);










#line 476 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"



#line 480 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"







#line 488 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"



















#line 508 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"











































































#line 584 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"

 



















































 extern long _crtBreakAlloc;      
#line 639 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"

 long __cdecl _CrtSetBreakAlloc(
             long _BreakAlloc 
        );





      void * __cdecl _malloc_dbg(
             size_t _Size,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

      void * __cdecl _calloc_dbg(
             size_t _NumOfElements,
             size_t _SizeOfElements,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

      void * __cdecl _realloc_dbg(
               void * _Memory,
             size_t _NewSize,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

      void * __cdecl _recalloc_dbg
(
               void * _Memory,
             size_t _NumOfElements,
             size_t _SizeOfElements,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
);

      void * __cdecl _expand_dbg(
               void * _Memory,
             size_t _NewSize,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

 void __cdecl _free_dbg(
               void * _Memory,
             int _BlockType
        );

 size_t __cdecl _msize_dbg (
             void * _Memory,
             int _BlockType
        );

      void * __cdecl _aligned_malloc_dbg(
             size_t _Size,
             size_t _Alignment,
                 const char * _Filename,
             int _LineNumber
        );

      void * __cdecl _aligned_realloc_dbg(
               void * _Memory,
             size_t _Size,
             size_t _Alignment,
                 const char * _Filename,
             int _LineNumber
        );

      void * __cdecl _aligned_recalloc_dbg
(
               void * _Memory,
             size_t _NumOfElements,
             size_t _SizeOfElements,
             size_t _Alignment,
                 const char * _Filename,
             int _LineNumber
);

      void * __cdecl _aligned_offset_malloc_dbg(
             size_t _Size,
             size_t _Alignment,
             size_t _Offset,
                 const char * _Filename,
             int _LineNumber
        );

      void * __cdecl _aligned_offset_realloc_dbg(
               void * _Memory,
             size_t _Size,
             size_t _Alignment,
             size_t _Offset,
                 const char * _Filename,
             int _LineNumber
        );

      void * __cdecl _aligned_offset_recalloc_dbg
(
               void * _Memory,
             size_t _NumOfElements,
             size_t _SizeOfElements,
             size_t _Alignment,
             size_t _Offset,
                 const char * _Filename,
             int _LineNumber
);

 void __cdecl _aligned_free_dbg(
               void * _Memory
        );

         char * __cdecl _strdup_dbg(
                 const char * _Str,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

         wchar_t * __cdecl _wcsdup_dbg(
                 const wchar_t * _Str,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

         char * __cdecl _tempnam_dbg(
                 const char * _DirName,
                 const char * _FilePrefix,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

         wchar_t * __cdecl _wtempnam_dbg(
                 const wchar_t * _DirName,
                 const wchar_t * _FilePrefix,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

         char * __cdecl _fullpath_dbg(
                 char * _FullPath, 
               const char * _Path, 
             size_t _SizeInBytes,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

         wchar_t * __cdecl _wfullpath_dbg(
                 wchar_t * _FullPath, 
               const wchar_t * _Path, 
             size_t _SizeInWords,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

         char * __cdecl _getcwd_dbg(
                 char * _DstBuf,
             int _SizeInBytes,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

         wchar_t * __cdecl _wgetcwd_dbg(
                 wchar_t * _DstBuf,
             int _SizeInWords,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

         char * __cdecl _getdcwd_dbg(
             int _Drive,
                 char * _DstBuf,
             int _SizeInBytes,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

         wchar_t * __cdecl _wgetdcwd_dbg(
             int _Drive,
                 wchar_t * _DstBuf,
             int _SizeInWords,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

        char * __cdecl _getdcwd_lk_dbg(
             int _Drive,
                 char * _DstBuf,
             int _SizeInBytes,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

        wchar_t * __cdecl _wgetdcwd_lk_dbg(
             int _Drive,
                 wchar_t * _DstBuf,
             int _SizeInWords,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

  errno_t __cdecl _dupenv_s_dbg(
                             char ** _PBuffer,
               size_t * _PBufferSizeInBytes,
               const char * _VarName,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );

  errno_t __cdecl _wdupenv_s_dbg(
                             wchar_t ** _PBuffer,
               size_t * _PBufferSizeInWords,
               const wchar_t * _VarName,
             int _BlockType,
                 const char * _Filename,
             int _LineNumber
        );




 





 _CRT_ALLOC_HOOK __cdecl _CrtGetAllocHook
(
    void
);
#line 888 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"






 _CRT_ALLOC_HOOK __cdecl _CrtSetAllocHook
(
           _CRT_ALLOC_HOOK _PfnNewHook
);








#line 907 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"


















#line 926 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"


 











 extern int _crtDbgFlag;
#line 942 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"

 int __cdecl _CrtCheckMemory(
        void
        );

 int __cdecl _CrtSetDbgFlag(
             int _NewFlag
        );

 void __cdecl _CrtDoForAllClientObjects(
             void (__cdecl *_PFn)(void *, void *),
        void * _Context
        );









#line 965 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"

  int __cdecl _CrtIsValidPointer(
               const void * _Ptr,
             unsigned int _Bytes,
             int _ReadWrite
        );

  int __cdecl _CrtIsValidHeapPointer(
               const void * _HeapPtr
        );

 int __cdecl _CrtIsMemoryBlock(
               const void * _Memory,
             unsigned int _Bytes,
               long * _RequestNumber,
               char ** _Filename,
               int * _LineNumber
        );

  int __cdecl _CrtReportBlockType(
               const void * _Memory
        );


 






 _CRT_DUMP_CLIENT __cdecl _CrtGetDumpClient
(
    void
);
#line 1001 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"






 _CRT_DUMP_CLIENT __cdecl _CrtSetDumpClient
(
           _CRT_DUMP_CLIENT _PFnNewDump
);









#line 1021 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"















#line 1037 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"

  void __cdecl _CrtMemCheckpoint(
             _CrtMemState * _State
        );

  int __cdecl _CrtMemDifference(
             _CrtMemState * _State,
             const _CrtMemState * _OldState,
             const _CrtMemState * _NewState
        );

 void __cdecl _CrtMemDumpAllObjectsSince(
               const _CrtMemState * _State
        );

 void __cdecl _CrtMemDumpStatistics(
             const _CrtMemState * _State
        );

 int __cdecl _CrtDumpMemoryLeaks(
        void
        );

 int __cdecl _CrtSetCheckCount(
             int _CheckCount
        );

 int __cdecl _CrtGetCheckCount(
        void
        );

#line 1069 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"


}



extern "C++" {


































 






 
  void * __cdecl operator new[](size_t _Size);

  void * __cdecl operator new(
        size_t _Size,
        int,
        const char *,
        int
        );

  void * __cdecl operator new[](
        size_t _Size,
        int,
        const char *,
        int
        );


void __cdecl operator delete[](void *);

inline void __cdecl operator delete(void * _P, int, const char *, int)
        { ::operator delete(_P); }

inline void __cdecl operator delete[](void * _P, int, const char *, int)
        { ::operator delete[](_P); }
#line 1143 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"











#line 1155 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"

}

#line 1159 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"

#line 1161 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"


#pragma pack(pop)
#line 1165 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"

#line 1167 "c:\\program files\\microsoft visual studio 8\\vc\\include\\crtdbg.h"

#line 272 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"



inline void *operator new(size_t size, const char *fnm, int line) {return _malloc_dbg(size, 1, fnm, line);}
inline void *operator new[](size_t size, const char *fnm, int line) {return _malloc_dbg(size, 1, fnm, line);}








extern void _dbg_MCU_realloc(char **data, uint4 osize, uint4 nsize, uint4 csize, const char *f, int l);


#line 289 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"





struct MCFontStruct
{
	MCSysFontHandle fid;
	uint16_t size;
	int ascent;
	int descent;
	uint1 widths[256];
	uint1 charset;
	Boolean wide;
	Boolean printer;
	Boolean unicode;
};
























































































































































































































#line 523 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"






























#line 554 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"



#line 558 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"










struct MCColor
{
	uint4 pixel;
	uint2 red, green, blue;
	uint1 flags;
	uint1 pad;
};






struct MCSegment
{
	int2 x1, y1, x2, y2;
};

struct MCPoint
{
	int2 x, y;
};

struct MCRectangle
{
	int2 x, y;
	uint2 width, height;
};

struct MCPoint32
{
	int32_t x, y;
};

struct MCRectangle32
{
	int32_t x, y;
	int32_t width, height;
};





struct MCWindowShape
{
	
	int32_t width;
	int32_t height;

	
	bool is_sharp : 1;
	
	
	uint32_t stride;
	char *data;
	
	
	
	void *handle;
};




struct MCBitmap
{
	uint2 width, height;
	uint2 format;
	char *data;
	uint1 byte_order;
	uint1 bitmap_unit;
	uint1 bitmap_bit_order;
	uint1 bitmap_pad;
	uint1 depth;
	uint1 bits_per_pixel;
	uint4 bytes_per_line;
	uint4 red_mask;
	uint4 green_mask;
	uint4 blue_mask;
	MCSysBitmapHandle bm;
};




























#line 679 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"




enum
{
    DC_WINDOW,
    DC_BITMAP,
	DC_BITMAP_WITH_DC
};

typedef struct drawable
{
	uint4 type;
	union
	{
		MCSysWindowHandle window;
		MCSysBitmapHandle pixmap;
	} handle;
}
_Drawable;

struct _ExtendedDrawable: public _Drawable
{
	MCSysContextHandle hdc;
};

typedef  _Drawable *        Window;
typedef  _Drawable *        Pixmap;
typedef  _Drawable *        Drawable;

























#line 735 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"










typedef unsigned long       KeySym;
typedef unsigned long       Atom;























































































































































































































































































































































template<typename T> class MCAutoPointer
{
public:
	MCAutoPointer(void)
	{
		m_ptr = 0;
	}

	~MCAutoPointer(void)
	{
		delete m_ptr;
	}

	T* operator = (T* value)
	{
		delete m_ptr;
		m_ptr = value;
		return value;
	}

	T*& operator & (void)
	{
		MCAssert(m_ptr == 0);
		return m_ptr;
	}

	T* operator -> (void)
	{
		MCAssert(m_ptr != 0);
		return m_ptr;
	}

	T *operator * (void) const
	{
		return m_ptr;
	}

	void Take(T*&r_ptr)
	{
		r_ptr = m_ptr;
		m_ptr = 0;
	}

private:
	T *m_ptr;
};






struct ssl_st;
typedef struct ssl_st SSL;
struct ssl_ctx_st;
typedef struct ssl_ctx_st SSL_CTX;

class MCContext;
typedef class MCContext MCDC;

class MCSharedString;
struct MCPickleContext;

class MCUIDC;
class MCTransferlist;
class MCUndolist;
class MCSellist;
class MCIdlelist;
class MCStacklist;
class MCCardlist;
class MCDispatch;
class MCStack;
class MCTooltip;
class MCAudioClip;
class MCVideoClip;
class MCGroup;
class MCCard;
class MCButton;
class MCGraphic;
class MCEPS;
class MCScrollbar;
class MCPlayer;
class MCImage;
class MCField;
class MCObject;
class MCObjectList;
class MCMagnify;
class MCPrinter;
class MCPrinterDevice;
class MCPrinterSetupDialog;
class MCPageSetupDialog;
class MCSocket;
class MCTheme;
class MCWidget;
class MCScriptEnvironment;
class MCDragData;
class MCClipboardData;
class MCSelectionData;
class MCPasteboard;
class MCFontlist;
struct MCWidgetInfo;
class MCExecPoint;
class MCParameter;
class MCStack;

typedef uint4 MCDragAction;
typedef uint4 MCDragActionSet;

typedef struct _Streamnode Streamnode;
typedef struct _Linkatts Linkatts;

struct MCThemeDrawInfo;

struct MCDateTime;
struct MCDateTimeLocale;

class MPlayer;

class MCObjectInputStream;
class MCObjectOutputStream;
class MCStatement;
class MCScriptPoint;
class MCVarref;

class MCBlock;
struct Ustruct;
class MCControl;
class MCParagraph;
class MCHandlerlist;
class MCHandler;
class MCParentScript;
class MCParentScriptUse;
class MCVariable;
class MCExpression;
struct MCPickleContext;
class MCVariableValue;
class MCVariableArray;

class MCExternal;
class MCExternalHandlerList;

class MCObjptr;

typedef struct __MCRegion *MCRegionRef;
typedef struct MCTileCache *MCTileCacheRef;
class MCStackSurface;

class MCObjectPropertySet;
class MCGo;
class MCVisualEffect;
class MCCRef;
class MCProperty;
class MCChunk;
struct MCObjectRef;

typedef struct MCBitmapEffects *MCBitmapEffectsRef;
class MCCdata;
class MCLine;
struct MCTextBlock;
struct MCTextParagraph;

class MCEffectList;
class MCError;

class MCStyledText;

typedef struct MCFont *MCFontRef;



#line 1261 "c:\\github\\livecode-runrev\\engine\\src\\sysdefs.h"

#line 71 "c:\\github\\livecode-runrev\\engine\\src\\globdefs.h"




#line 1 "c:\\github\\livecode-runrev\\engine\\src\\mcutility.h"





















#line 23 "c:\\github\\livecode-runrev\\engine\\src\\mcutility.h"



#line 27 "c:\\github\\livecode-runrev\\engine\\src\\mcutility.h"



void *memdup(const void *p_src, unsigned int p_src_length);

Boolean strequal(const char *one, const char *two);
Boolean strnequal(const int1 *one, const int1 *two, size_t n);
char *strclone(const char *one);
char *strclone(const char *one, uint4 length);

int4 MCU_strcasecmp(const char *a, const char *b);
int4 MCU_strncasecmp(const char *one, const char *two, size_t n);
bool MCU_strcaseequal(const char *a, const char *b);
Boolean MCU_strchr(const char *&sptr, uint4 &l, char target, Boolean isunicode);



inline char *MCU_empty()
{
	return strclone("");
}

inline void MCU_skip_spaces(const char *&sptr, uint4 &l)
{
	while (l && ( ((0)!=0 && (((_locale_t)(0))->locinfo->mb_cur_max) > 1) ? _isctype_l((uint1)*sptr, (0x8), 0) : _chvalidator_l(0, (uint1)*sptr, 0x8)))
	{
		sptr++;
		l--;
	}
}

inline void MCU_skip_char(const char *&sptr, uint4 &l)
{
	if (l)
	{
		sptr++;
		l--;
	}
}


inline Boolean MCU_comparechar(const char *sptr, char target, Boolean isunicode)
{
	if (isunicode)
	{
		uint2 *uchar = (uint2 *)sptr;
		return *uchar == (uint2)target;
	}
	else
		return *sptr == target;
}

inline uint1 MCU_charsize(Boolean isunicode)
{
	return isunicode ? 2 : 1;
}

inline void MCU_copychar(uint2 source, char *dest, Boolean isunicode)
{
	if (!isunicode)
		*dest = (uint1)source;
	else
	{
		uint2 *d = (uint2 *)dest;
		*d = source;
	}
}

inline void MCU_copychar(const char *source, char *dest, Boolean isunicode)
{
	if (!isunicode)
		*dest = *source;
	else
	{
		uint2 *s = (uint2 *)source;
		uint2 *d = (uint2 *)dest;
		*d = *s;
	}
}



class MCString
{
protected:
	const char *sptr;
	uint4 length;
	
public:
	MCString(void)
	{
			sptr = 0;
			length = 0;
	}
	
	MCString(const char *s, uint4 l)
	{
		sptr = s;
		length = l;
	}
	
	MCString(const char *s);
	
	~MCString(void)
	{
	}
	
	void set(const char *s, uint4 l)
	{
		sptr = s;
		length = l;
	}
	
	void setstring(const char *s)
	{
		sptr = s;
	}
	
	void setlength(uint4 l)
	{
		length = l;
	}
	
	void get(const char *&s, uint4 &l)
	{
		sptr = s;
		length = l;
	}
	
	const char *getstring() const
	{
		return sptr;
	}
	
	uint4 getlength() const
	{
		return length;
	}
	
	char *clone() const;
	
	
	
	
	bool split(char p_char, MCString& r_head, MCString& r_tail);
	
	Boolean operator=(const char *s);

	Boolean operator=(const MCString &s)
	{
		sptr = s.sptr;
		length = s.length;
		return 1;
	}
	
	Boolean operator==(const MCString &two) const;
	Boolean operator!=(const MCString &two) const;
	Boolean equalexactly(const MCString& other) const;
};

struct MCDictionary
{
public:
	MCDictionary(void);
	~MCDictionary(void);

	void Set(uint4 p_id, const MCString& p_value);
	bool Get(uint4 p_id, MCString& r_value);

	bool Unpickle(const void *p_buffer, uint4 p_length);
	void Pickle(void*& r_buffer, uint4& r_length);

private:
	struct Node
	{
		Node *next;
		uint4 key;
		void *buffer;
		uint4 length;
	};

	Node *Find(uint4 p_id);
	uint32_t Checksum(const void *p_data, uint32_t p_length);

	Node *m_nodes;
};

extern MCString MCtruemcstring;
extern MCString MCfalsemcstring;
extern MCString MCnullmcstring;

inline const MCString &MCU_btos(uint4 condition)
{
	return condition ? MCtruemcstring : MCfalsemcstring;
}

inline uint4 MCU_abs(int4 source)
{
	return source > 0 ? source : -source;
}

inline int4 MCU_min(int4 one, int4 two) {return one > two ? two : one;}
inline uint4 MCU_min(uint4 one, uint4 two) {return one > two ? two : one;}

inline int4 MCU_max(int4 one, int4 two) {return one > two ? one : two;}
inline uint4 MCU_max(uint4 one, uint4 two) {return one > two ? one : two;}

inline int4 MCU_clamp(int4 v, int4 lower, int4 upper) {return v < lower ? lower : (v > upper ? upper : v);}

inline real8 MCU_fmin(real8 one, real8 two)
{
	return one > two ? two : one;
}

inline real8 MCU_fmax(real8 one, real8 two)
{
	return one > two ? one : two;
}

extern Boolean MCswapbytes;

inline uint4 swap_uint4(uint4 *dest)
{
	if (MCswapbytes)
	{
		uint1 *tptr = (uint1 *)dest;
		uint1 tmp = tptr[0];
		tptr[0] = tptr[3];
		tptr[3] = tmp;
		tmp = tptr[1];
		tptr[1] = tptr[2];
		tptr[2] = tmp;
	}
	return *dest;
}

inline int4 swap_int4(int4 *dest)
{
	if (MCswapbytes)
	{
		uint1 *tptr = (uint1 *)dest;
		uint1 tmp = tptr[0];
		tptr[0] = tptr[3];
		tptr[3] = tmp;
		tmp = tptr[1];
		tptr[1] = tptr[2];
		tptr[2] = tmp;
	}
	return *dest;
}

inline uint2 swap_uint2(uint2 *dest)
{
	if (MCswapbytes)
	{
		uint1 *tptr = (uint1 *)dest;
		uint1 tmp = tptr[0];
		tptr[0] = tptr[1];
		tptr[1] = tmp;
	}
	return *dest;
}

inline int2 swap_int2(int2 *dest)
{
	if (MCswapbytes)
	{
		uint1 *tptr = (uint1 *)dest;
		uint1 tmp = tptr[0];
		tptr[0] = tptr[1];
		tptr[1] = tmp;
	}
	return *dest;
}

inline uint2 get_uint2(char *source)
{
	uint2 result;
	uint1 *tptr = (uint1 *)&result;
	*tptr++ = *source++;
	*tptr = *source;
	return swap_uint2(&result);
}

inline uint4 get_uint4(char *source)
{
	uint4 result;
	uint1 *tptr = (uint1 *)&result;
	*tptr++ = *source++;
	*tptr++ = *source++;
	*tptr++ = *source++;
	*tptr = *source;
	return swap_uint4(&result);
}

inline int2 swap_int2(int2 x)
{
	if (MCswapbytes)
		return (int2)((x << 8) | ((uint2)x >> 8));
	return x;
}

inline int2 swap_int2(uint2 x)
{
	if (MCswapbytes)
		return (x << 8) | (x >> 8);
	return x;
}

inline char *MC_strchr(const char *s, int c) 
{
	return strchr((char *)s, c);
}

inline Boolean MCU_ispunct(uint1 x)
{
	return ( ((0)!=0 && (((_locale_t)(0))->locinfo->mb_cur_max) > 1) ? _isctype_l(x, (0x10), 0) : _chvalidator_l(0, x, 0x10)) != 0 && x != 39 && x != 212 && x != 213;
}

#line 347 "c:\\github\\livecode-runrev\\engine\\src\\mcutility.h"
#line 76 "c:\\github\\livecode-runrev\\engine\\src\\globdefs.h"
#line 77 "c:\\github\\livecode-runrev\\engine\\src\\globdefs.h"



#line 81 "c:\\github\\livecode-runrev\\engine\\src\\globdefs.h"
#line 20 "c:\\github\\livecode-runrev\\engine\\src\\prefix.h"
#line 21 "c:\\github\\livecode-runrev\\engine\\src\\prefix.h"
#line 18 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"

#line 1 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"






















#line 24 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"



typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;



	


		typedef unsigned long long int uint64_t;
	#line 41 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"
#line 42 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"


	


		typedef long long int int64_t;
	#line 49 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"
#line 50 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"

typedef uint32_t uindex_t;
typedef int32_t index_t;
typedef uint32_t hash_t;
typedef int32_t compare_t;



#line 59 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"








#line 68 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"





















typedef char char_t;


typedef wchar_t unichar_t;
typedef wchar_t *BSTR;


#line 97 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"




#line 102 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"







#line 110 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"


typedef char *va_list;




#line 118 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"



#line 122 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"




extern void __MCAssert(const char *file, uint32_t line, const char *message);


extern void __MCLog(const char *file, uint32_t line, const char *format, ...);


extern void __MCLogWithTrace(const char *file, uint32_t line, const char *format, ...);






#line 140 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"
















#line 157 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"

bool MCThrow(uint32_t error);
bool MCRethrow(uint32_t new_error);





bool MCMemoryAllocate(uindex_t size, void*& r_block);



bool MCMemoryAllocateCopy(const void *p_block, uindex_t p_block_size, void*& r_block);





bool MCMemoryReallocate(void *block, uindex_t new_size, void*& r_new_block);




void MCMemoryDeallocate(void *block);



template<typename T> bool MCMemoryAllocate(uindex_t p_size, T*& r_block)
{
	void *t_block;
	if (MCMemoryAllocate(p_size, t_block))
	{
		r_block = static_cast<T *>(t_block);
		return true;
	}
	return false;
}

template<typename T> bool MCMemoryAllocateCopy(const T *p_block, uindex_t p_block_size, T*& r_block)
{
	void *t_block;
	if (MCMemoryAllocateCopy(p_block, p_block_size, t_block))
	{
		r_block = static_cast<T *>(t_block);
		return true;
	}
	return false;
}

template<typename T> bool MCMemoryReallocate(T *p_block, uindex_t p_new_size, T*& r_new_block)
{
	void *t_new_block;
	if (MCMemoryReallocate(p_block, p_new_size, t_new_block))
	{
		r_new_block = static_cast<T *>(t_new_block);
		return true;
	}
	return false;
}







bool MCMemoryNew(uindex_t size, void*& r_record);


void MCMemoryDelete(void *p_record);







#line 235 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"
#line 236 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"

inline void *operator new (size_t, void *p_block, bool)
{
	return p_block;
}


template<typename T> bool MCMemoryNew(T*& r_record)
{
	void *t_record;
	if (MCMemoryNew(sizeof(T), t_record))
	{
		
		
		
		r_record = new(t_record, true) T;

		return true;
	}
	return false;
}

template<typename T> void MCMemoryDelete(T* p_record)
{
	
	
	p_record -> ~T();

	MCMemoryDelete(static_cast<void *>(p_record));
}





#line 272 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"
#line 273 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"






bool MCMemoryNewArray(uindex_t p_count, uindex_t p_size, void*& r_array);


bool MCMemoryResizeArray(uindex_t p_new_count, uindex_t p_size, void*& x_array, uindex_t& x_count);



void MCMemoryDeleteArray(void *p_array);



template<typename T> inline bool MCMemoryNewArray(uindex_t p_count, T*& r_array)
{
	void *t_array;
	if (MCMemoryNewArray(p_count, sizeof(T), t_array))
		return r_array = static_cast<T *>(t_array), true;
	return false;
}

template<typename T> inline bool MCMemoryResizeArray(uindex_t p_new_count, T*& r_array, uindex_t& x_count)
{
	void *t_array;
	t_array = r_array;
	if (MCMemoryResizeArray(p_new_count, sizeof(T), t_array, x_count))
		return r_array = static_cast<T *>(t_array), true;
	return false;
}



void MCMemoryClear(void *dst, uindex_t size);
void MCMemoryCopy(void *dst, const void *src, uindex_t size);
void MCMemoryMove(void *dst, const void *src, uindex_t size);
bool MCMemoryEqual(const void *left, const void *right, uindex_t size);
compare_t MCMemoryCompare(const void *left, const void *right, uindex_t size);
hash_t MCMemoryHash(const void *src, uindex_t size);










bool MCCStringTokenize(const char *string, char**& r_elements, uint32_t& r_element_count);








bool MCCStringSplit(const char *string, char p_separator, char**& r_elements, uint32_t& r_element_count);
bool MCCStringCombine(const char * const *p_elements, uint32_t p_element_count, char p_separator, char*& r_string);

bool MCCStringFormat(char*& r_string, const char *format, ...);
bool MCCStringFormatV(char*& r_string, const char *format, va_list args);
bool MCCStringAppendFormat(char*& x_string, const char *format, ...);
bool MCCStringAppendFormatV(char*& r_string, const char *format, va_list args);
bool MCCStringClone(const char *s, char*& r_s);
bool MCCStringCloneSubstring(const char *p_string, uint32_t p_length, char*& r_new_string);
bool MCCStringAppend(char*& x_string, const char *string);
void MCCStringFree(char *s);

bool MCCStringArrayClone(const char *const *strings, uint32_t count, char**& r_new_strings);
void MCCStringArrayFree(char **cstrings, uint32_t count);

bool MCCStringIsEmpty(const char *s);
bool MCCStringIsInteger(const char *s);

uint32_t MCCStringLength(const char *s);

bool MCCStringToUnicode(const char *string, unichar_t*& r_unicode_string);

bool MCCStringFromUnicode(const unichar_t* unicode_string, char*& r_string);
bool MCCStringFromUnicodeSubstring(const unichar_t* unicode_string, uint32_t length, char*& r_string);

bool MCCStringToNative(const char *string, char*& r_string);
bool MCCStringFromNative(const char *string, char*& r_string);
bool MCCStringFromNativeSubstring(const char *string, uint32_t length, char*& r_string);




#line 367 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"


bool MCCStringToBSTR(const char *string, BSTR& r_bstr);
bool MCCStringFromBSTR(BSTR bstr, char*& r_cstring);
#line 372 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"

compare_t MCCStringCompare(const char *x, const char *y);

bool MCCStringEqual(const char *x, const char *y);
bool MCCStringEqualCaseless(const char *x, const char *y);
bool MCCStringEqualSubstring(const char *x, const char *y, index_t length);
bool MCCStringEqualSubstringCaseless(const char *x, const char *y, index_t length);

bool MCCStringContains(const char *string, const char *needle);
bool MCCStringContainsCaseless(const char *string, const char *needle);

bool MCCStringBeginsWith(const char *string, const char *prefix);
bool MCCStringBeginsWithCaseless(const char *string, const char *prefix);
bool MCCStringEndsWith(const char *string, const char *suffix);
bool MCCStringEndsWithCaseless(const char *string, const char *suffix);

bool MCCStringToInteger(const char *string, int32_t& r_value);
bool MCCStringToCardinal(const char *string, uint32_t& r_value);

bool MCCStringFirstIndexOf(const char *p_string, char p_search, uint32_t &r_index);
bool MCCStringFirstIndexOf(const char *p_string, const char *p_search, uint32_t &r_index);
bool MCCStringLastIndexOf(const char *p_string, char p_search, uint32_t &r_index);
bool MCCStringLastIndexOf(const char *p_string, const char *p_search, uint32_t &r_index);




class MCAutoCString
{
public:
	MCAutoCString(void)
	{
		m_cstring = 0;
	}

	~MCAutoCString(void)
	{
		MCCStringFree(m_cstring);
	}

	
	bool AssignCString(const char *p_cstring)
	{
		if (m_cstring != 0)
			MCCStringFree(m_cstring);
		return MCCStringClone(p_cstring, m_cstring);
	}

	
	bool AssignNative(const char *p_native)
	{
		if (m_cstring != 0)
			MCCStringFree(m_cstring);
		return MCCStringFromNative(p_native, m_cstring);
	}

	
	bool AssignUnicode(const unichar_t *p_unicode)
	{
		if (m_cstring != 0)
			MCCStringFree(m_cstring);
		return MCCStringFromUnicode(p_unicode, m_cstring);
	}

	
	operator const char *(void) const
	{
		return m_cstring;
	}

private:
	MCAutoCString(const MCAutoCString&) {}

	char *m_cstring;
};



struct MCBinaryEncoder;

bool MCBinaryEncoderCreate(MCBinaryEncoder*& r_encoder);
void MCBinaryEncoderDestroy(MCBinaryEncoder *encoder);

void MCBinaryEncoderBorrow(MCBinaryEncoder *encoder, void*& r_buffer, uint32_t& r_buffer_length);

bool MCBinaryEncoderWriteBytes(MCBinaryEncoder *encoder, const void *data, uint32_t length);
bool MCBinaryEncoderWriteInt32(MCBinaryEncoder *encoder, int32_t p_value);
bool MCBinaryEncoderWriteUInt32(MCBinaryEncoder *encoder, uint32_t p_value);
bool MCBinaryEncoderWriteCBlob(MCBinaryEncoder *encoder, const void *data, uint32_t length);
bool MCBinaryEncoderWriteCString(MCBinaryEncoder *encoder, const char *cstring);








struct MCBinaryDecoder;

bool MCBinaryDecoderCreate(const void *p_buffer, uint32_t p_length, MCBinaryDecoder*& r_decoder);
void MCBinaryDecoderDestroy(MCBinaryDecoder *p_decoder);

bool MCBinaryDecoderReadBytes(MCBinaryDecoder *decoder, void *data, uint32_t count);
bool MCBinaryDecoderReadInt32(MCBinaryDecoder *decoder, int32_t& r_value);
bool MCBinaryDecoderReadUInt32(MCBinaryDecoder *decoder, uint32_t& r_value);
bool MCBinaryDecoderReadCBlob(MCBinaryDecoder *decoder, void*& r_data, uint32_t& r_length);
bool MCBinaryDecoderReadCString(MCBinaryDecoder *self, char *&r_cstring);








inline uint32_t MCMin(uint32_t a, uint32_t b) { return a < b ? a : b; }
inline uint32_t MCMax(uint32_t a, uint32_t b) { return a > b ? a : b; }
inline int32_t MCMin(int32_t a, int32_t b) { return a < b ? a : b; }
inline int32_t MCMax(int32_t a, int32_t b) { return a > b ? a : b; }
inline int64_t MCMin(int64_t a, int64_t b) { return a < b ? a : b; }
inline int64_t MCMax(int64_t a, int64_t b) { return a > b ? a : b; }
inline int64_t MCMin(uint64_t a, uint64_t b) { return a < b ? a : b; }
inline int64_t MCMax(uint64_t a, uint64_t b) { return a > b ? a : b; }
inline double MCMin(double a, double b) { return a < b ? a : b; }
inline double MCMax(double a, double b) { return a > b ? a : b; }
inline float MCMin(float a, float b) { return a < b ? a : b; }
inline float MCMax(float a, float b) { return a > b ? a : b; }
inline uint32_t MCAbs(int32_t a) { return a < 0 ? -a : a; }
inline uint64_t MCAbs(int64_t a) { return a < 0 ? -a : a; }
inline compare_t MCSgn(int32_t a) { return a < 0 ? -1 : (a > 0 ? 1 : 0); }
inline compare_t MCSgn(int64_t a) { return a < 0 ? -1 : (a > 0 ? 1 : 0); }
inline compare_t MCCompare(int32_t a, int32_t b) { return a < b ? -1 : (a > b ? 1 : 0); }
inline compare_t MCCompare(uint32_t a, uint32_t b) { return a < b ? -1 : (a > b ? 1 : 0); }
inline compare_t MCCompare(int64_t a, int64_t b) { return a < b ? -1 : (a > b ? 1 : 0); }
inline compare_t MCCompare(uint64_t a, uint64_t b) { return a < b ? -1 : (a > b ? 1 : 0); }

inline bool MCIsPowerOfTwo(uint32_t x) { return (x & (x - 1)) == 0; }

inline float MCClamp(float value, float min, float max) {return MCMax(min, MCMin(max, value));}



inline uint32_t MCByteSwappedToHost32(uint32_t x)
{

	return ((x >> 24) | ((x >> 8) & 0xff00) | ((x & 0xff00) << 8) | (x << 24));


#line 522 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"
}

inline uint32_t MCByteSwappedFromHost32(uint32_t x)
{

	return ((x >> 24) | ((x >> 8) & 0xff00) | ((x & 0xff00) << 8) | (x << 24));


#line 531 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"
}

inline uint32_t MCSwapInt32HostToNetwork(uint32_t i)
{

	return ((i & 0xff) << 24) | ((i & 0xff00) << 8) | ((i & 0xff0000) >> 8) | ((i & 0xff000000) >> 24);


#line 540 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"
}

inline uint32_t MCSwapInt32NetworkToHost(uint32_t i)
{

	return ((i & 0xff) << 24) | ((i & 0xff00) << 8) | ((i & 0xff0000) >> 8) | ((i & 0xff000000) >> 24);


#line 549 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"
}

inline uint16_t MCSwapInt16HostToNetwork(uint16_t i)
{

	return ((i & 0xff) << 8) | ((i & 0xff00) >> 8);


#line 558 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"
}

inline uint16_t MCSwapInt16NetworkToHost(uint16_t i)
{

	return ((i & 0xff) << 8) | ((i & 0xff00) >> 8);


#line 567 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"
}



void MCListPushBack(void *& x_list, void *element);
void *MCListPopBack(void *&x_list);
void MCListPushFront(void *& x_list, void *element);
void *MCListPopFront(void *&x_list);

void MCListRemove(void *& x_list, void *element);



template<typename T> inline void MCListPushBack(T*& x_list, T *p_element)
{
	void *t_list;
	t_list = x_list;
	MCListPushBack(t_list, p_element);
	x_list = static_cast<T *>(t_list);
}

template<typename T> inline T *MCListPopBack(T*& x_list)
{
	void *t_list, *t_element;
	t_list = x_list;
	t_element = MCListPopBack(t_list);
	x_list = static_cast<T *>(t_list);
	return static_cast<T *>(t_element);
}

template<typename T> inline void MCListPushFront(T*& x_list, T *p_element)
{
	void *t_list;
	t_list = x_list;
	MCListPushFront(t_list, p_element);
	x_list = static_cast<T *>(t_list);
}

template<typename T> inline T *MCListPopFront(T*& x_list)
{
	void *t_list, *t_element;
	t_list = x_list;
	t_element = MCListPopFront(t_list);
	x_list = static_cast<T *>(t_list);
	return static_cast<T *>(t_element);
}

template<typename T> inline void MCListRemove(T*& x_list, T *p_element)
{
	void *t_list;
	t_list = x_list;
	MCListRemove(t_list, p_element);
	x_list = static_cast<T *>(t_list);
}



#line 625 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"
#line 20 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\globdefs.h"
















































































#line 21 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\filedefs.h"





























enum IO_stat {
    IO_NORMAL,
    IO_NONE,
    IO_ERROR,
    IO_EOF,
    IO_TIMEOUT
};

enum Open_mode {
    OM_APPEND,
    OM_NEITHER,
    OM_VCLIP,
    OM_READ,
    OM_WRITE,
    OM_UPDATE,
    OM_TEXT,
    OM_BINARY
};

enum Object_type {
    OT_END,
    OT_HOME,
    OT_NOTHOME,
    OT_STACK,
    OT_CARD,
    OT_GROUP,
    OT_GROUPEND,
    OT_PTR,
    OT_BUTTON,
    OT_BDATA,
    OT_FIELD,
    OT_FDATA,
    OT_PARAGRAPH,
    OT_BLOCK,
    OT_IMAGE,
    OT_SCROLLBAR,
    OT_MAGNIFY,
    OT_COLORS,
    OT_GRAPHIC,
    OT_MCEPS,
    OT_AUDIO_CLIP,
    OT_VIDEO_CLIP,
    OT_ENCRYPT_STACK,
    OT_PLAYER,
    OT_CUSTOM,
	OT_STYLED_TEXT,
	
	OT_PARAGRAPH_EXT,
	
	OT_BLOCK_EXT,
};













struct MCSystemFileHandle;

class IO_header
{
public:

	MCWinSysHandle fhandle;
	char *buffer;
	
	
	size_t len;
	char *ioptr;
	MCWinSysHandle mhandle; 
	int putback;
	
	bool is_pipe;
	IO_header(MCWinSysHandle fp, char *b, uint4 l, uint2 f)
	{
		fhandle = fp;
		ioptr = buffer = b;
		len = l;
		mhandle = 0;
		flags = f;
		putback = -1;
		is_pipe = false;
	}
	int getfd()
	{
		return -1;
	}




































































#line 193 "c:\\github\\livecode-runrev\\engine\\src\\filedefs.h"
	uint2 flags;
};

typedef IO_header * IO_handle;

typedef struct _Streamnode
{
	char *name;
	Open_mode mode;
	IO_handle ihandle;
	IO_handle ohandle;
	int4 pid;

	MCWinSysHandle phandle;  
	MCWinSysHandle thandle;  


#line 211 "c:\\github\\livecode-runrev\\engine\\src\\filedefs.h"
	int4 retcode;
	Boolean textmode;
}
Streamnode;

extern IO_handle IO_stdin;
extern IO_handle IO_stdout;
extern IO_handle IO_stderr;

#line 221 "c:\\github\\livecode-runrev\\engine\\src\\filedefs.h"
#line 22 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\objdefs.h"







































































#line 73 "c:\\github\\livecode-runrev\\engine\\src\\objdefs.h"











































































































































































































































































































































































































































































































typedef struct
{
	uint4 keysym;
	uint1 functions[4];
}
Keytranslations;



typedef struct
{
	uint4 keysym;
	const char *name;
}
Keynames;

typedef struct _Linkatts
{
	MCColor color;
	char *colorname;
	MCColor hilitecolor;
	char *hilitecolorname;
	MCColor visitedcolor;
	char *visitedcolorname;
	Boolean underline;
}
Linkatts;

enum Draw_index {
    DI_FORE,
    DI_BACK,
    DI_HILITE,
    DI_BORDER,
    DI_TOP,
    DI_BOTTOM,
    DI_SHADOW,
    DI_FOCUS
};



















enum Scrollbar_mode {
    SM_CLEARED,
    SM_BEGINNING,
    SM_END,
    SM_LINEDEC,
    SM_LINEINC,
    SM_PAGEDEC,
    SM_PAGEINC
};

enum Object_pos {
    OP_NONE,
    OP_LEFT,
    OP_ALIGN_LEFT,
    OP_CENTER,
    OP_ALIGN_RIGHT,
    OP_RIGHT,
    OP_TOP,
    OP_ALIGN_TOP,
    OP_MIDDLE,
    OP_ALIGN_BOTTOM,
    OP_BOTTOM,
    OP_OFFSET
};

enum Arrow_direction {
    AD_UP,
    AD_DOWN,
    AD_LEFT,
    AD_RIGHT
};

enum Etch {
    ETCH_RAISED,
    ETCH_RAISED_SMALL,
    ETCH_SUNKEN,
    ETCH_SUNKEN_BUTTON,
    ETCH_HILITE,
    ETCH_GROOVE,
    ETCH_RIDGE
};

enum Field_translations {
    FT_UNDEFINED,
    FT_DELBCHAR,
	FT_DELBSUBCHAR = FT_DELBCHAR,
    FT_DELBWORD,
    FT_DELFCHAR,
    FT_DELFWORD,
	FT_DELBOL,
	FT_DELEOL,
	FT_DELBOP,
	FT_DELEOP,
    FT_HELP,
    FT_UNDO,
    FT_CUT,
    FT_CUTLINE,
    FT_COPY,
    FT_PASTE,
    FT_TAB,
    FT_FOCUSFIRST,
    FT_FOCUSLAST,
    FT_FOCUSNEXT,
    FT_FOCUSPREV,
    FT_PARAGRAPH,
	FT_PARAGRAPHAFTER,
    FT_LEFTCHAR,
	FT_BACKCHAR = FT_LEFTCHAR,
    FT_LEFTWORD,
	FT_BACKWORD = FT_LEFTWORD,
	FT_LEFTPARA,
	FT_BACKPARA = FT_LEFTPARA,
    FT_RIGHTCHAR,
	FT_FORWARDCHAR = FT_RIGHTCHAR,
    FT_RIGHTWORD,
	FT_FORWARDWORD = FT_RIGHTWORD,
	FT_RIGHTPARA,
	FT_FORWARDPARA = FT_RIGHTPARA,
    FT_UP,
    FT_DOWN,
    FT_PAGEUP,
    FT_PAGEDOWN,
    FT_HOME,
    FT_END,
    FT_BOL,
    FT_BOS,
    FT_BOP,
    FT_BOF,
    FT_EOL,
    FT_EOS,
    FT_EOP,
    FT_EOF,
    FT_CENTER,
	FT_TRANSPOSE,
    FT_SCROLLDOWN,
    FT_SCROLLUP,
	FT_SCROLLLEFT,
	FT_SCROLLRIGHT,
	FT_SCROLLTOP,
	FT_SCROLLPAGEUP,
	FT_SCROLLBOTTOM,
	FT_SCROLLPAGEDOWN,
    FT_IMEINSERT
};

inline uint4 getstyleint(uint4 flags)
{
	return flags & 0x7;
}

inline uint2 heightfromsize(uint2 size)
{
	return (size * 4 / 3);
}

enum Text_encoding {
    TE_ANSI,
    TE_UNICODE,
    TE_MULTIBYTE
};

enum Lang_charset
{
    LCH_ENGLISH = 0,
    LCH_ROMAN, 
    LCH_JAPANESE,
    LCH_CHINESE,
    LCH_KOREAN,
    LCH_ARABIC,
    LCH_HEBREW,
    LCH_RUSSIAN,
    LCH_TURKISH,
    LCH_BULGARIAN,
    LCH_UKRAINIAN,
    LCH_POLISH,
    LCH_GREEK,
    LCH_SIMPLE_CHINESE,
    LCH_THAI,
	LCH_VIETNAMESE,
	LCH_LITHUANIAN,
    LCH_UNICODE,
    LCH_UTF8,
	LCH_DEFAULT = 255
};

enum MCImagePaletteType
{
	kMCImagePaletteTypeEmpty,
	kMCImagePaletteTypeWebSafe,
	kMCImagePaletteTypeOptimal,
	kMCImagePaletteTypeCustom,
};

typedef struct _MCImagePaletteSettings
{
	MCImagePaletteType type;
	MCColor *colors;
	uint32_t ncolors;
} MCImagePaletteSettings;

#line 782 "c:\\github\\livecode-runrev\\engine\\src\\objdefs.h"
#line 23 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\parsedef.h"
























extern char *MCregexpatterns[];


typedef struct _regexp regexp;
extern regexp *MCregexcache[];

typedef struct _constant
{
	MCString name;
	MCString value;
}
constant;



enum Accept_constants {
    AC_UNDEFINED,
    AC_DATAGRAM,
    AC_SECURE
};

enum Apple_event {
    AE_UNDEFINED,
    AE_AE,
    AE_CLASS,
    AE_DATA,
    AE_ID,
    AE_RETURN,
    AE_RETURN_ID,
    AE_SENDER
};

enum Ask_type {
    AT_UNDEFINED,
    AT_CLEAR,
    AT_COLOR,
    AT_EFFECT,
    AT_ERROR,
    AT_FILE,
    AT_FOLDER,
    AT_INFORMATION,
    AT_PASSWORD,
    AT_PRINTER,
    AT_PROGRAM,
    AT_QUESTION,
    AT_RECORD,
    AT_TITLED,
    AT_WARNING,
    AT_SHEET,
	AT_FILES,
	AT_TYPES,
	AT_FOLDERS,
	
	AT_PAGE,
	AT_SETUP,
	AT_PAGESETUP,
	AT_HINT,
};


enum Chunk_term {
    CT_UNDEFINED,
    CT_START,
    CT_BACKWARD,
    CT_FORWARD,
    CT_FINISH,
    CT_HOME,
	
	
	CT_SERVER_SCRIPT,
    CT_HELP,
    CT_DIRECT,
    CT_RECENT,
    CT_THIS,
    CT_FIRST,
    CT_SECOND,
    CT_THIRD,
    CT_FOURTH,
    CT_FIFTH,
    CT_SIXTH,
    CT_SEVENTH,
    CT_EIGHTH,
    CT_NINTH,
    CT_TENTH,
    CT_LAST,
    CT_NEXT,
    CT_PREV,
    CT_MIDDLE,
    CT_ANY,
    CT_ORDINAL,
    CT_ID,
    CT_EXPRESSION,
    CT_RANGE,
    CT_URL,
    CT_URL_HEADER,
    CT_ALIAS,
	CT_DOCUMENT,
    CT_TOP_LEVEL,
    CT_MODELESS,
    CT_PALETTE,
    CT_MODAL,
    CT_PULLDOWN,
    CT_POPUP,
    CT_OPTION,

    CT_STACK,
    CT_AUDIO_CLIP,
    CT_VIDEO_CLIP,
    CT_BACKGROUND,
    CT_CARD,
    CT_MARKED,
    CT_GROUP,
    CT_LAYER,
    CT_BUTTON,
    CT_MENU,
    CT_SCROLLBAR,
    CT_PLAYER,
    CT_IMAGE,
    CT_GRAPHIC,
    CT_EPS,
    CT_MAGNIFY,
    CT_COLOR_PALETTE,
    CT_FIELD,
	CT_ELEMENT,
    CT_LINE,
    CT_ITEM,
    CT_WORD,
    CT_TOKEN,
    CT_CHARACTER,
    CT_TYPES,
	CT_KEY
};

inline Chunk_term ct_class(Chunk_term src)
{
	if (src == CT_UNDEFINED)
		return src;
	if (src < CT_DIRECT)
		return CT_DIRECT;
	if (src < CT_ORDINAL)
		return CT_ORDINAL;
	if (src == CT_ID)
		return CT_ID;
	if (src == CT_EXPRESSION)
		return CT_EXPRESSION;
	return CT_TYPES;
}

enum Convert_form {
    CF_UNDEFINED,
    CF_SHORT,
    CF_ABBREVIATED,
    CF_LONG,
    CF_INTERNET,
    CF_SECONDS,
    CF_DATEITEMS,
    CF_TIME,
    CF_SHORT_TIME,
    CF_ABBREV_TIME,
    CF_LONG_TIME,
    CF_DATE,
    CF_SHORT_DATE,
    CF_ABBREV_DATE,
    CF_LONG_DATE,
    CF_INTERNET_DATE,
    CF_ENGLISH = 1000,
    CF_SYSTEM = 2000
};


enum Dest_type {
    DT_UNDEFINED,
    DT_ISDEST,
    DT_VARIABLE,
    DT_EXPRESSION,
    DT_ME,
    DT_MENU_OBJECT,
    DT_TARGET,
    DT_BUTTON,
    DT_CARD,
    DT_FIELD,
    DT_GROUP,
    DT_IMAGE,
    DT_GRAPHIC,
    DT_EPS,
    DT_SCROLLBAR,
    DT_AUDIO_CLIP,
    DT_VIDEO_CLIP,
    DT_PLAYER,
    DT_STACK,
    DT_SELECTED,
    DT_ERROR,
    DT_TOP_STACK,
    DT_CLICK_STACK,
    DT_MOUSE_STACK,
    DT_FUNCTION,
	
	
	
	DT_OWNER
};


enum Encryption_constants
{
    ENCRT_BIT,
    ENCRT_IV,
    ENCRT_KEY,
    ENCRT_PASSWORD,
    ENCRT_SALT,
    ENCRT_USING,
    RSA_CERT,
    RSA_PRIVKEY,
    RSA_PUBKEY,
	ENCRT_RSA,
	ENCRT_PUBLIC,
	ENCRT_PRIVATE,
	ENCRT_PASSPHRASE,
};

enum Exec_concat {
    EC_NONE,
    EC_SPACE,
    EC_COMMA,
    EC_NULL,
    EC_RETURN,
	EC_TAB
};


enum Exec_stat {
    ES_ERROR,
    ES_NORMAL,
    ES_NEXT_REPEAT,
    ES_EXIT_REPEAT,
    ES_EXIT_HANDLER,
    ES_EXIT_SWITCH,
    ES_EXIT_ALL,
	ES_RETURN_HANDLER,
    ES_PASS,
    ES_PASS_ALL,
    ES_NOT_HANDLED,
    ES_NOT_FOUND
};

enum Exit_to {
    ET_UNDEFINED,
    ET_ALL,
    ET_REPEAT,
    ET_SWITCH,
    ET_TO
};

enum Export_format {
    EX_UNDEFINED,
    EX_AUDIO_CLIP,
    EX_DISPLAY,
    EX_EPS,
    EX_GIF,
    EX_JPEG,
    EX_PBM,
    EX_PNG,
    EX_SNAPSHOT,
    EX_STACK,
    EX_VIDEO_CLIP,
    EX_XBM,
    EX_XWD,
    EX_AIFF,
    EX_WAVE,
    EX_ULAW,
    EX_MOVIE,
	EX_RAW,
	EX_RAW_ARGB,
	EX_RAW_ABGR,
	EX_RAW_RGBA,
	EX_RAW_BGRA,
	EX_RAW_RGB,
	EX_RAW_BGR,
	EX_RAW_BGRX,
	EX_RAW_GRAY,
	EX_RAW_INDEXED,
	EX_BMP,
};

enum Factor_rank {
    FR_UNDEFINED,
    FR_GROUPING,
    FR_OR,
    FR_AND,
    FR_OR_BITS,
    FR_XOR_BITS,
    FR_AND_BITS,
    FR_EQUAL,
    FR_COMPARISON,
    FR_CONCAT,
    FR_ADDSUB,
    FR_MULDIV,
    FR_POW,
    FR_UNARY,
    FR_VALUE
};

enum File_unit {
    FU_UNDEFINED,
    FU_CHARACTER,
    FU_INT1,
    FU_INT2,
    FU_INT4,
    FU_INT8,
    FU_ITEM,
    FU_LINE,
    FU_REAL4,
    FU_REAL8,
    FU_TOKEN,
    FU_WORD,
    FU_UINT1,
    FU_UINT2,
    FU_UINT4,
    FU_UINT8,
    FU_ELEMENT,
	FU_KEY
};

enum Find_mode {
    FM_UNDEFINED,
    FM_NORMAL,
    FM_WHOLE,
    FM_WORD,
    FM_CHARACTERS,
    FM_STRING
};

enum Flip_dir {
    FL_UNDEFINED,
    FL_HORIZONTAL,
    FL_VERTICAL
};


enum Functions {
    F_UNDEFINED,
    F_ABS,
    F_ACOS,
    F_ALIAS_REFERENCE,
    F_ALTERNATE_LANGUAGES,
    F_ANNUITY,
	
    F_ARI_MEAN,
    F_ASIN,
    F_ATAN,
    F_ATAN2,
	
    F_AVG_DEV,
    F_BACK_SCRIPTS,
    F_BASE_CONVERT,
    F_BASE64_DECODE,
    F_BASE64_ENCODE,
    F_BINARY_DECODE,
    F_BINARY_ENCODE,
    F_BUILD_NUMBER,
    F_CACHED_URLS,
    F_CAPS_LOCK_KEY,
	F_BYTE_TO_NUM,
    F_CHAR_TO_NUM,
    F_CIPHER_NAMES,
    F_CLICK_CHAR,
    F_CLICK_CHAR_CHUNK,
    F_CLICK_CHUNK,
    F_CLICK_FIELD,
    F_CLICK_H,
    F_CLICK_LINE,
    F_CLICK_LOC,
    F_CLICK_STACK,
    F_CLICK_TEXT,
    F_CLICK_V,
    F_CLIPBOARD,
    F_COLOR_NAMES,
    F_COMMAND_KEY,
    F_COMMAND_NAMES,
    F_COMPOUND,
    F_COMPRESS,
    F_CONSTANT_NAMES,
    F_CONTROL_KEY,
    F_COPY_RESOURCE,
    F_COS,
    F_DATE,
    F_DATE_FORMAT,
    F_DECOMPRESS,
    F_DELETE_RESOURCE,
    F_DIRECTORIES,
    F_DISK_SPACE,
    F_DNS_SERVERS,
    F_DRAG_DESTINATION,
    F_DRAG_SOURCE,
    F_DRIVER_NAMES,
    F_DRIVES,
    F_DROP_CHUNK,
    F_ENCRYPT,
    F_ENVIRONMENT,
    F_EXISTS,
    F_EXP,
    F_EXP1,
    F_EXP10,
    F_EXP2,
    F_EXTENTS,
    F_FILES,
    F_FLUSH_EVENTS,
    F_FOCUSED_OBJECT,
    F_FONT_LANGUAGE,
    F_FONT_NAMES,
    F_FONT_SIZES,
    F_FONT_STYLES,
    F_FORMAT,
    F_FOUND_CHUNK,
    F_FOUND_FIELD,
    F_FOUND_LINE,
    F_FOUND_LOC,
    F_FOUND_TEXT,
    F_FRONT_SCRIPTS,
    F_FUNCTION_NAMES,
    F_GET_RESOURCE,
    F_GET_RESOURCES,
	
    F_GEO_MEAN,
    F_GLOBAL_LOC,
    F_GLOBALS,
	
    F_HAR_MEAN,
    F_HAS_MEMORY,
    F_HEAP_SPACE,
    F_HA,
    F_HATON,
    F_HN,
    F_HNTOA,
    F_INTERRUPT,
    F_INTERSECT,
    F_IS_NUMBER,
    F_ISO_TO_MAC,
    F_ITEM_OFFSET,
    F_KEYS,
    F_KEYS_DOWN,
    F_LENGTH,
    F_LICENSED,
    F_LINE_OFFSET,
    F_LN,
    F_LN1,
    F_LOCAL_LOC,
    F_LOCALS,
    F_LOG10,
    F_LOG2,
    F_LONG_FILE_PATH,
    F_MAC_TO_ISO,
    F_MACHINE,
    F_MAIN_STACKS,
    F_MATCH_CHUNK,
    F_MATCH_TEXT,
    F_MATRIX_MULTIPLY,
    F_MAX,
    F_MCI_SEND_STRING,
    F_MD5_DIGEST,
    F_ME,
    F_MEDIAN,
    F_MENU_OBJECT,
    F_MENUS,
    F_MERGE,
    F_MILLISECS,
    F_MIN,
    F_MONTH_NAMES,
    F_MOUSE,
    F_MOUSE_CHAR,
    F_MOUSE_CHAR_CHUNK,
    F_MOUSE_CHUNK,
    F_MOUSE_CLICK,
    F_MOUSE_COLOR,
    F_MOUSE_CONTROL,
    F_MOUSE_H,
    F_MOUSE_LINE,
    F_MOUSE_LOC,
    F_MOUSE_STACK,
    F_MOUSE_TEXT,
    F_MOUSE_V,
    F_MOVIE,
    F_MOVING_CONTROLS,
    F_NUM_TO_CHAR,
	F_NUM_TO_BYTE,
    F_OFFSET,
    F_OPEN_FILES,
    F_OPEN_PROCESSES,
    F_OPEN_PROCESS_IDS,
    F_OPEN_SOCKETS,
    F_OPEN_STACKS,
    F_OPTION_KEY,
	
	
	F_OWNER,
    F_PA,
    F_PARAM,
    F_PARAMS,
    F_PARAM_COUNT,
    F_PENDING_MESSAGES,
    F_PLATFORM,
	
    F_POP_STD_DEV,
	
    F_POP_VARIANCE,
    F_PROCESS_ID,
    F_PROCESSOR,
    F_PROPERTY_NAMES,
    F_QT_EFFECTS,
    F_QT_VERSION,
    F_QUERY_REGISTRY,
    F_RANDOM,
    F_RECORD_COMPRESSION_TYPES,
    F_RECORD_LOUDNESS,
    F_REPLACE_TEXT,
    F_RESULT,
    F_ROUND,
	F_RUNTIME_ENVIRONMENTS, 
    F_SCREEN_COLORS,
    F_SCREEN_DEPTH,
    F_SCREEN_LOC,
    F_SCREEN_NAME,
    F_SCREEN_RECT,
		F_SCREEN_RECTS,
    F_SCREEN_TYPE,
    F_SCREEN_VENDOR,
    F_SCRIPT_LIMITS,
    F_SECONDS,
    F_SELECTED_BUTTON,
    F_SELECTED_CHUNK,
    F_SELECTED_FIELD,
    F_SELECTED_IMAGE,
    F_SELECTED_LINE,
    F_SELECTED_LOC,
    F_SELECTED_TEXT,
    F_SELECTED_OBJECT,
    F_SET_REGISTRY,
    F_SET_RESOURCE,
    F_SHELL,
    F_SHIFT_KEY,
    F_SHORT_FILE_PATH,
    F_SIN,
	
    F_SMP_STD_DEV,
	
    F_SMP_VARIANCE,
    F_SOUND,
    F_SPECIAL_FOLDER_PATH,
    F_SQRT,
    F_STACKS,
    F_STACK_SPACE,
    F_STAT_ROUND,
    F_SUM,
    F_SYS_ERROR,
    F_SYSTEM_VERSION,
    F_TAN,
    F_TARGET,
    F_TEMP_NAME,
    F_TEMPLATE_BUTTON,
    F_TEMPLATE_CARD,
    F_TEMPLATE_FIELD,
    F_TEMPLATE_GROUP,
    F_TEMPLATE_IMAGE,
    F_TEMPLATE_GRAPHIC,
    F_TEMPLATE_EPS,
    F_TEMPLATE_SCROLLBAR,
    F_TEMPLATE_AUDIO_CLIP,
    F_TEMPLATE_VIDEO_CLIP,
    F_TEMPLATE_PLAYER,
    F_TEMPLATE_STACK,
    F_TEXT_HEIGHT_SUM,
    F_TICKS,
    F_TIME,
    F_TO_LOWER,
    F_TO_UPPER,
    F_TOP_STACK,
    F_TRANSPOSE,
    F_TRUNC,
    F_UNI_DECODE,
    F_UNI_ENCODE,
    F_URL_DECODE,
    F_URL_ENCODE,
    F_URL_STATUS,
    F_VALUE,
    F_VARIABLES,
    F_VERSION,
    F_WAIT_DEPTH,
    F_WEEK_DAY_NAMES,
    F_WINDOWS,
    F_WITHIN,
    F_WORD_OFFSET,
    F_DELETE_REGISTRY,
	F_LIST_REGISTRY,
	F_HTTP_PROXY_FOR_URL,
	F_ARRAY_ENCODE,
	F_ARRAY_DECODE,
	F_RANDOM_BYTES,
	F_SHA1_DIGEST,
	
	
	F_CONTROL_AT_LOC,
	F_CONTROL_AT_SCREEN_LOC,
	
	
	F_UUID,
};

enum Handler_type {
    HT_UNDEFINED,
    HT_MESSAGE,
    HT_FUNCTION,
    HT_GETPROP,
    HT_SETPROP,
	
	
	
	HT_BEFORE,
	HT_AFTER,

	HT_PRIVATE
};

enum If_format {
    IF_UNDEFINED,
    IF_ONELINE,
    IF_SINGLE,
    IF_MULTIPLE,
    IF_ELSEMULTIPLE
};

enum If_state {
    IS_UNDEFINED,
    IS_THEN,
    IS_ELSE
};

enum Try_state {
    TS_TRY,
    TS_CATCH,
    TS_FINALLY
};

enum Insert_point {
    IP_BACK,
    IP_FRONT
};

enum Is_type {
    IT_UNDEFINED,
    IT_AMONG,
    IT_NOT_AMONG,
    IT_NORMAL,
    IT_IN,
    IT_NOT,
    IT_NOT_IN,
    IT_WITHIN,
    IT_NOT_WITHIN,
	IT_AMONG_THE_DRAG_DATA,
	IT_NOT_AMONG_THE_DRAG_DATA,
	IT_AMONG_THE_CLIPBOARD_DATA,
	IT_NOT_AMONG_THE_CLIPBOARD_DATA,
};

enum Is_validation {
    IV_UNDEFINED,
    IV_AMONG,
    IV_COLOR,
    IV_DATE,
    IV_INTEGER,
    IV_LOGICAL,
    IV_NUMBER,
    IV_POINT,
    IV_RECT,
	IV_ARRAY,
	
    IV_ASCII,
};

enum Lock_constants {
    LC_UNDEFINED,
    LC_COLORMAP,
    LC_CURSOR,
    LC_ERRORS,
    LC_MENUS,
    LC_MSGS,
    LC_MOVES,
    LC_RECENT,
    LC_SCREEN,
	LC_SCREEN_FOR_EFFECT,
};

enum Mark_constants {
    MC_UNDEFINED,
    MC_ALL,
    MC_BY,
    MC_CARDS,
    MC_FINDING,
    MC_WHERE
};

enum Move_mode {
    MM_UNDEFINED,
    MM_MESSAGES,
    MM_WAITING
};

enum Open_argument {
    OA_UNDEFINED,
    OA_DIRECTORY,
    OA_DRIVER,
    OA_FILE,
    OA_OBJECT,
    OA_PRINTING,
    OA_PROCESS,
    OA_SOCKET,
    OA_STDERR,
    OA_STDIN,
    OA_STDOUT
};

enum Operators {
    O_UNDEFINED,
    O_GROUPING,
    O_NOT,
    O_NOT_BITS,
    O_POW,
    O_THERE,
    O_TIMES,
    O_OVER,
    O_DIV,
    O_MOD,
    O_PLUS,
    O_MINUS,
    O_CONCAT,
    O_CONCAT_SPACE,
    O_LT,
    O_LE,
    O_GE,
    O_GT,
    O_CONTAINS,
    O_NE,
    O_EQ,
    O_IS,
    O_AND_BITS,
    O_XOR_BITS,
    O_OR_BITS,
    O_AND,
    O_OR,
	O_WRAP,
	O_BEGINS_WITH,
	O_ENDS_WITH
};


enum Parse_stat {
    PS_ERROR,
    PS_NORMAL,
    PS_EOL,
    PS_EOF,
    PS_NO_MATCH,
    PS_BREAK
};

enum Pixmap_ids {
    PI_NONE,
    PI_ARROW,
    PI_BRUSH,
    PI_SPRAY,
    PI_ERASER,
    PI_BUCKET,
    PI_BUSY,
    PI_CROSS,
    PI_HAND,
    PI_IBEAM,
    PI_LR,
    PI_PENCIL,
    PI_DROPPER,
    PI_PLUS,
    PI_WATCH,
    PI_HELP,
    PI_BUSY1,
    PI_BUSY2,
    PI_BUSY3,
    PI_BUSY4,
    PI_BUSY5,
    PI_BUSY6,
    PI_BUSY7,
    PI_BUSY8,
    PI_DRAGTEXT,
    PI_DRAGCLONE,
    PI_DRAGREFUSE,
    PI_SIZEV,
    PI_SIZEH,
    PI_NCURSORS,
    PI_BRUSHES = 100,
    PI_PATTERNS = 136,
    PI_END = 300
};

enum Play_params {
    PP_UNDEFINED,
    PP_AUDIO_CLIP,
    PP_BACK,
    PP_FORWARD,
    PP_LOOPING,
    PP_OPTIONS,
    PP_PAUSE,
    PP_PLAYER,
    PP_RESUME,
    PP_STEP,
    PP_STOP,
    PP_TEMPO,
    PP_VIDEO_CLIP,
	PP_VIDEO,
};

enum Record_params {
    RC_BEST,
    RC_BETTER,
    RC_GOOD,
    RC_QUALITY,
    RC_SOUND
};

enum Preposition_type {
    PT_UNDEFINED,
    PT_AFTER,
    PT_AS,
    PT_AT,
    PT_BEFORE,
    PT_FROM,
    PT_IN,
    PT_INTO,
    PT_OF,
    PT_ON,
    PT_RELATIVE,
    PT_TO,
    PT_WITHOUT,
    PT_BY,
    PT_ALIGNED,
	PT_HEADER,
	PT_NEW_HEADER,
	PT_CONTENT,
	PT_MARKUP,
	PT_BINARY,
	PT_COOKIE
};

enum Print_mode {
    PM_UNDEFINED,
    PM_ALL,
    PM_BREAK,
    PM_CARD,
    PM_MARKED,
    PM_SOME,
	PM_ANCHOR,
	PM_LINK,
	PM_BOOKMARK,
	PM_UNICODE_BOOKMARK,
	PM_LINK_ANCHOR,
	PM_LINK_URL,
};

enum Properties {
    
    P_UNDEFINED,
    P_SHORT,
    P_ABBREVIATE,
    P_LONG,
    P_INTERNET,
    P_EFFECTIVE,
    P_ENGLISH,
    P_SYSTEM,
	P_WORKING,
    
    P_CASE_SENSITIVE,
    P_CENTURY_CUTOFF,
    P_CONVERT_OCTALS,
    P_ITEM_DELIMITER,
    P_COLUMN_DELIMITER,
    P_LINE_DELIMITER,
	P_ROW_DELIMITER,
    P_NUMBER_FORMAT,
    P_WHOLE_MATCHES,
    P_USE_SYSTEM_DATE,
    P_USE_UNICODE,
    
    P_CURSOR,
    P_DEFAULT_CURSOR,
    P_DRAG_SPEED,
    P_MOVE_SPEED,
    P_EDIT_BACKGROUND,
    P_DEFAULT_STACK,
    P_STACK_FILES,
    P_LOCK_COLORMAP,
    P_LOCK_CURSOR,
    P_LOCK_ERRORS,
    P_LOCK_MENUS,
    P_LOCK_MESSAGES,
    P_LOCK_MOVES,
    P_LOCK_RECENT,
    P_LOCK_SCREEN,
	
    P_LOCK_UPDATES,
    P_BEEP_LOUDNESS,
    P_BEEP_PITCH,
    P_BEEP_DURATION,
	P_BEEP_SOUND, 
    P_PLAY_LOUDNESS,
    P_PLAY_DESTINATION,
    P_DIRECTORY,
    P_TWELVE_TIME,
    P_PRIVATE_COLORS,
    P_IDLE_RATE,
    P_IDLE_TICKS,
    P_BLINK_RATE,
    P_RECURSION_LIMIT,
    P_REPEAT_RATE,
    P_REPEAT_DELAY,
    P_TYPE_RATE,
    P_SYNC_RATE,
    P_EFFECT_RATE,
    P_DONT_USE_NS,
    P_DONT_USE_QT,
    P_DONT_USE_QT_EFFECTS,
    P_DOUBLE_TIME,
    P_DOUBLE_DELTA,
    P_LONG_WINDOW_TITLES,
    P_BLIND_TYPING,
    P_POWER_KEYS,
    P_NAVIGATION_ARROWS,
    P_TEXT_ARROWS,
    P_EXTEND_KEY,
    P_COLORMAP,
    P_NO_PIXMAPS,
    P_POINTER_FOCUS,
    P_LOW_RESOLUTION_TIMERS,
    P_RAISE_MENUS,
    P_ACTIVATE_PALETTES,
    P_HIDE_PALETTES,
    P_RAISE_PALETTES,
    P_PROPORTIONAL_THUMBS,
    P_SHARED_MEMORY,
    P_VC_SHARED_MEMORY,
    P_VC_PLAYER,
    P_TRACE_ABORT,
    P_TRACE_DELAY,
    P_TRACE_RETURN,
    P_TRACE_STACK,
	P_TRACE_UNTIL,
    P_SHELL_COMMAND,
    
	P_PRINTER_NAMES,

	P_PRINT_COMMAND,
    P_PRINT_FONT_TABLE,
    
	P_PRINT_CARD_BORDERS,
    P_PRINT_GUTTERS,
    P_PRINT_MARGINS,
    P_PRINT_ROTATED,
    P_PRINT_ROWS_FIRST,
    P_PRINT_SCALE,

	P_PRINT_DEVICE_NAME, 
	P_PRINT_DEVICE_SETTINGS,
	P_PRINT_DEVICE_OUTPUT,
	P_PRINT_DEVICE_FEATURES,
	P_PRINT_DEVICE_RECTANGLE,
	
	P_PRINT_PAGE_SIZE,
    P_PRINT_PAGE_ORIENTATION,
	P_PRINT_PAGE_SCALE,
	P_PRINT_PAGE_RECTANGLE,
	
	P_PRINT_JOB_NAME,
	P_PRINT_JOB_COLOR,
	P_PRINT_JOB_COPIES,
	P_PRINT_JOB_COLLATE,
	P_PRINT_JOB_DUPLEX,
	P_PRINT_JOB_RANGES,
	P_PRINT_JOB_PAGE,
	
	P_PRINT_TEXT_ALIGN,
    P_PRINT_TEXT_FONT,
    P_PRINT_TEXT_HEIGHT,
    P_PRINT_TEXT_SIZE,
    P_PRINT_TEXT_STYLE,
	
	P_DIALOG_DATA,

    P_ACCEPT_DROP,
	P_ALLOWABLE_DRAG_ACTIONS,
    P_DRAG_DATA,
	P_DRAG_DELTA,
    P_DRAG_ACTION,
	P_DRAG_IMAGE,
	P_DRAG_IMAGE_OFFSET,
	
	P_CLIPBOARD_DATA,
    P_HC_IMPORT_STAT,
    P_SCRIPT_TEXT_FONT,
    P_SCRIPT_TEXT_SIZE,
    P_LOOK_AND_FEEL,
    P_SCREEN_MOUSE_LOC,
    P_SCREEN_GAMMA,
    P_UMASK,
    P_BUFFER_MODE,
    P_BUFFER_IMAGES,
    P_BACK_DROP,
    P_MULTI_EFFECT,
    P_ALLOW_INTERRUPTS,
    P_EXPLICIT_VARIABLES,
		P_PRESERVE_VARIABLES,
    P_SYSTEM_FS,
    P_SYSTEM_CS,
	P_SYSTEM_PS,
    P_FILE_TYPE,
    P_STACK_FILE_TYPE,
		P_STACK_FILE_VERSION,
    P_SECURE_MODE,
    P_SERIAL_CONTROL_STRING,
    P_TOOL,
    P_EDIT_MENUS,
    P_EDIT_SCRIPTS,
    P_COLOR_WORLD,
    P_PEN_WIDTH,
    P_PEN_HEIGHT,
    P_ALLOW_KEY_IN_FIELD,
    P_REMAP_COLOR,
    P_HIDE_CONSOLE_WINDOWS,
    P_FTP_PROXY,
    P_HTTP_HEADERS,
    P_HTTP_PROXY,
    P_SHOW_INVISIBLES,
    P_SOCKET_TIMEOUT,
    P_RANDOM_SEED,
    P_DEFAULT_MENU_BAR,
    P_ACCENT_COLOR,
    P_JPEG_QUALITY,
    P_LZW_KEY,
    P_PAINT_COMPRESSION,
    P_EMACS_KEY_BINDINGS,
    P_SOUND_CHANNEL,
    P_RELAYER_GROUPED_CONTROLS,
    P_SELECT_GROUPED_CONTROLS,
    P_SELECTION_HANDLE_COLOR,
    P_SELECTION_MODE,
    P_WINDOW_BOUNDING_RECT,
    P_LINK_COLOR,
    P_LINK_HILITE_COLOR,
    P_LINK_VISITED_COLOR,
    P_UNDERLINE_LINKS,
    P_RECORDING,
    P_RECORD_RATE,
    P_RECORD_CHANNELS,
    P_RECORD_SAMPLESIZE,
    P_RECORD_COMPRESSION,
    P_RECORD_FORMAT,
    P_RECORD_INPUT,
    P_BREAK_POINTS,
    P_DEBUG_CONTEXT,
    P_EXECUTION_CONTEXTS,
    P_MESSAGE_MESSAGES,
    P_WATCHED_VARIABLES,
    P_ALLOW_INLINE_INPUT,
    P_SSL_CERTIFICATES,
	P_HIDE_BACKDROP,
	P_QT_IDLE_RATE,
	P_RAISE_WINDOWS,
	P_PROCESS_TYPE,
    P_ERROR_MODE,
	P_ICON_MENU,
	P_STATUS_ICON,
	P_STATUS_ICON_MENU,
	P_STATUS_ICON_TOOLTIP,
	P_OUTPUT_TEXT_ENCODING,
	P_OUTPUT_LINE_ENDINGS,
	P_SESSION_SAVE_PATH,
	P_SESSION_LIFETIME,
	P_SESSION_COOKIE_NAME,
	P_SESSION_ID,

	P_SCRIPT_EXECUTION_ERRORS,
	P_SCRIPT_PARSING_ERRORS,
	P_DEFAULT_NETWORK_INTERFACE,

	
	P_IMAGE_CACHE_LIMIT,
	P_IMAGE_CACHE_USAGE,
	
    
    P_ADDRESS,
    P_STACKS_IN_USE,
	P_NETWORK_INTERFACES,
	
    
    P_NAME,
    P_SHORT_NAME,
    P_ABBREV_NAME,
    P_LONG_NAME,
    P_ID,
    P_SHORT_ID,
    P_ABBREV_ID,
    P_LONG_ID,
    P_ALT_ID,
    P_NUMBER,
    P_SHOW_BORDER,
    P_BOTTOM,
    P_BOTTOM_LEFT,
    P_BOTTOM_RIGHT,
    P_FORE_PIXEL,
    P_BACK_PIXEL,
    P_HILITE_PIXEL,
    P_BORDER_PIXEL,
    P_TOP_PIXEL,
    P_BOTTOM_PIXEL,
    P_SHADOW_PIXEL,
    P_FOCUS_PIXEL,
    P_FORE_COLOR,
    P_BACK_COLOR,
    P_HILITE_COLOR,
    P_BORDER_COLOR,
    P_TOP_COLOR,
    P_BOTTOM_COLOR,
    P_SHADOW_COLOR,
    P_FOCUS_COLOR,
    P_COLORS,
    P_FORE_PATTERN,
    P_BACK_PATTERN,
    P_HILITE_PATTERN,
    P_BORDER_PATTERN,
    P_TOP_PATTERN,
    P_BOTTOM_PATTERN,
    P_SHADOW_PATTERN,
    P_FOCUS_PATTERN,
    P_PATTERNS,
    P_HEIGHT,
    P_LEFT,
    P_LOCATION,
    P_LOCK_LOCATION,
    P_DISABLED,
    P_ENABLED,
    P_OPAQUE,
    P_RECTANGLE,
    P_RIGHT,
    P_SHADOW,
    P_SHADOW_OFFSET,
    P_3D,
    P_TOP,
    P_TOP_LEFT,
    P_TOP_RIGHT,
    P_INVISIBLE,
    P_VISIBLE,
    P_WIDTH,
    P_BORDER_WIDTH,
    P_SELECTED,
    
    P_BRUSH,
    P_BRUSH_COLOR,
    P_BRUSH_PATTERN,
    P_BRUSH_BACK_COLOR,
    P_CENTERED,
    P_ERASER,
    P_FILLED,
    P_GRID,
    P_GRID_SIZE,
    P_LINE_SIZE,
    P_MULTIPLE,
    P_MULTI_SPACE,
    P_PEN_COLOR,
    P_PEN_PATTERN,
    P_PEN_BACK_COLOR,
    P_INK,
    P_POLY_SIDES,
    P_SLICES,
    P_SPRAY,
    P_ROUND_ENDS,
    P_TEXT_ALIGN,
    P_TEXT_FONT,
    P_TEXT_HEIGHT,
    P_TEXT_SIZE,
    P_TEXT_SHIFT,
	
	
	
	P_TEXT_STYLE_ADD,
	P_TEXT_STYLE_REMOVE,
    
	P_REMOTEABLE, 
	P_STACK_URL, 
	P_FULLSCREEN, 
    P_FILE_NAME,
    P_SAVE_COMPRESSED,
    P_USER_LEVEL,
    P_CANT_ABORT,
    P_CANT_PEEK,
    P_CANT_DELETE,
    P_CANT_SELECT,
    P_CANT_MODIFY,
    P_USER_MODIFY,
    P_SCRIPT,
    P_CLOSE_BOX,
    P_DRAGGABLE,
    P_RESIZABLE,
    P_ZOOM_BOX,
    P_MIN_WIDTH,
    P_MIN_HEIGHT,
    P_MAX_WIDTH,
    P_MAX_HEIGHT,
    P_ICONIC,
    P_START_UP_ICONIC,
    P_RECENT_CARDS,
    P_RECENT_NAMES,
    P_MAIN_STACK,
    P_SUBSTACKS,
    P_BACKGROUND_IDS,
    P_BACKGROUND_NAMES,
    P_CARD_IDS,
    P_CARD_NAMES,
    P_EXTERNALS,
    P_EXTERNAL_PACKAGES,
    P_EXTERNAL_COMMANDS,
    P_EXTERNAL_FUNCTIONS,
    P_DESTROY_STACK,
    P_DESTROY_WINDOW,
    P_ALWAYS_BUFFER,
    P_PASSWORD,
    P_KEY,
    P_MODE,
    P_WM_PLACE,
    P_PIXMAP_ID,
    P_WINDOW_ID,
    P_HC_ADDRESSING,
    P_HC_STACK,
    P_DYNAMIC_PATHS,
    P_DECORATIONS,
    P_SIZE,
    P_FREE_SIZE,
	P_OWNER,
    P_SHORT_OWNER,
    P_ABBREV_OWNER,
    P_LONG_OWNER,
    P_CUSTOM,
    P_CUSTOM_VAR,
    P_CUSTOM_PROPERTY_SET,
    P_CUSTOM_PROPERTY_SETS,
    P_PROPERTIES,
    P_TOOL_TIP,
	P_UNICODE_TOOL_TIP,
    P_TOOL_TIP_DELAY,
    P_MENU_BAR,
    P_CHARSET,
    P_FORMAT_FOR_PRINTING,
    P_WINDOW_SHAPE,
    P_METAL,
    P_SYSTEM_WINDOW,
    P_LIVE_RESIZING,
    P_MINIMIZE_BOX,
    P_MAXIMIZE_BOX,
    P_COLLAPSE_BOX,
	P_SCREEN,
	P_REFERRING_STACK, 
	P_UNPLACED_GROUP_IDS, 
	P_IDE_OVERRIDE, 
	P_CURRENT_CARD,
	P_MODIFIED_MARK,
	P_COMPOSITOR_TYPE,
	P_COMPOSITOR_CACHE_LIMIT,
	P_COMPOSITOR_TILE_SIZE,
	P_COMPOSITOR_STATISTICS,
	P_ACCELERATED_RENDERING,
    
    P_TAB_GROUP_BEHAVIOR,
    P_RADIO_BEHAVIOR,
    P_HILITED_BUTTON,
    P_HILITED_BUTTON_ID,
    P_HILITED_BUTTON_NAME,
    P_SHOW_PICT,
    P_BACK_SIZE,
    P_BACKGROUND_BEHAVIOR,
    P_BOUNDING_RECT,
	P_UNICODE_LABEL,
	P_UNBOUNDED_VSCROLL,
	P_UNBOUNDED_HSCROLL,
	P_SHARED_BEHAVIOR,
    
    P_MARKED,
    P_DEFAULT_BUTTON,
    P_GROUP_IDS,
    P_GROUP_NAMES,
	P_SHARED_GROUP_IDS,
	P_SHARED_GROUP_NAMES,
    
    P_DONT_REFRESH,
    P_FRAME_RATE,
    P_CALLBACKS,
    P_CURRENT_TIME,
    P_DURATION,
    P_LOOPING,
    P_PLAY_RATE,
    P_SHOW_BADGE,
    P_SHOW_CONTROLLER,
    P_START_FRAME,
    P_END_FRAME,
    P_START_TIME,
    P_END_TIME,
    P_TIME_SCALE,
    P_PLAY_SELECTION,
    P_SHOW_SELECTION,
    P_PAUSED,
    P_MOVIE_CONTROLLER_ID,
    P_TRACK_COUNT,
    P_TRACKS,
    P_ENABLED_TRACKS,
    P_MEDIA_TYPES,
    P_CURRENT_NODE,
    P_NODES,
    P_ZOOM,
    P_TILT,
    P_PAN,
    P_CONSTRAINTS,
    P_HOT_SPOTS,
    
    P_POSTSCRIPT,
    P_ANGLE,
    P_PROLOG,
    P_RETAIN_IMAGE,
    P_RETAIN_POSTSCRIPT,
    P_SCALE,
    P_SCALE_INDEPENDENTLY,
    P_X_SCALE,
    P_Y_SCALE,
    P_X_OFFSET,
    P_Y_OFFSET,
    P_X_EXTENT,
    P_Y_EXTENT,
    P_CURRENT_PAGE,
    P_PAGE_COUNT,
    
    P_LAYER,
    P_STYLE,
    P_TRAVERSAL_ON,
    P_SHOW_FOCUS_BORDER,
    P_AUTO_ARM,
    P_AUTO_HILITE,
    P_ARM_BORDER,
    P_ARM_FILL,
    P_HILITE_BORDER,
    P_HILITE_FILL,
    P_ARM,
    P_HILITE,
    P_ARMED_ICON,
    P_DISABLED_ICON,
    P_HILITED_ICON,
    P_ICON,
    P_VISITED_ICON,
	P_HOVER_ICON,
    P_SHARED_HILITE,
    P_SHOW_HILITE,
    P_SHOW_ICON,
    P_SHOW_NAME,
    P_MENU_BUTTON,
    P_MENU_HISTORY,
    P_MENU_LINES,
    P_MENU_MODE,
    P_MENU_NAME,
    P_ACCELERATOR_TEXT,
	P_UNICODE_ACCELERATOR_TEXT,
    P_ACCELERATOR_KEY,
    P_ACCELERATOR_MODIFIERS,
    P_MNEMONIC,
    P_MARGINS,
    P_LEFT_MARGIN,
    P_RIGHT_MARGIN,
    P_TOP_MARGIN,
    P_BOTTOM_MARGIN,
    P_DEFAULT,
    P_LABEL,
    P_LABEL_WIDTH,
    P_FAMILY,
    P_VISITED,
    
    P_CHECK_MARK,
    P_COMMAND_CHAR,
    P_MARK_CHAR,
    P_MENU_MESSAGE,
    
    P_MAGNIFY,
    P_HOT_SPOT,
    P_XHOT,
    P_YHOT,
    P_IMAGE_PIXMAP_ID,
    P_MASK_PIXMAP_ID,
    P_ALPHA_DATA,
    P_IMAGE_DATA,
    P_MASK_DATA,
    P_DONT_DITHER,
    P_CURRENT_FRAME,
    P_FRAME_COUNT,
    P_REPEAT_COUNT,
    P_PALINDROME_FRAMES,
    P_CONSTANT_MASK,
    P_BLEND_LEVEL,
    P_RESIZE_QUALITY,
    
    P_DONT_RESIZE,
    P_POINTS,
    P_RELATIVE_POINTS,
    P_START_ANGLE,
    P_ARC_ANGLE,
    P_ROUND_RADIUS,
    P_ARROW_SIZE,
    P_MARKER_DRAWN,
    P_MARKER_OPAQUE,
    P_MARKER_LSIZE,
    P_MARKER_POINTS,
    P_DASHES,
    P_START_ARROW,
    P_END_ARROW,
	P_ANTI_ALIASED,
	P_FILL_RULE, 
	P_EDIT_MODE,
	P_CAP_STYLE,
	P_JOIN_STYLE,
	P_MITER_LIMIT,
	
    P_THUMB_SIZE,
    P_THUMB_POS,
    P_LINE_INC,
    P_PAGE_INC,
    P_ORIENTATION,
    P_START_VALUE,
    P_END_VALUE,
    P_SHOW_VALUE,
    
    P_AUTO_TAB,
    P_DONT_SEARCH,
    P_DONT_WRAP,
    P_FIXED_HEIGHT,
    P_WIDE_MARGINS,
    P_FIRST_INDENT,
    P_LOCK_TEXT,
    P_SHARED_TEXT,
    P_SHOW_LINES,
    P_FORMATTED_LEFT,
    P_FORMATTED_TOP,
    P_FORMATTED_WIDTH,
    P_FORMATTED_HEIGHT,
    P_FORMATTED_RECT,
    P_HSCROLL,
    P_VSCROLL,
    P_HSCROLLBAR,
    P_VSCROLLBAR,
	P_HSCROLLBARID, 
	P_VSCROLLBARID, 
    P_SCROLLBAR_WIDTH,
    P_LIST_BEHAVIOR,
    P_TEXT,
    P_UNICODE_TEXT,
    P_HTML_TEXT,
    P_RTF_TEXT,
	
	P_STYLED_TEXT,
	
	P_FORMATTED_STYLED_TEXT,
	P_PLAIN_TEXT,
	P_UNICODE_PLAIN_TEXT,
    P_FORMATTED_TEXT,
	P_UNICODE_FORMATTED_TEXT,
    P_MULTIPLE_HILITES,
    P_NONCONTIGUOUS_HILITES,
    P_HILITED_LINES,
    P_TAB_STOPS,
    P_TOGGLE_HILITE,
    P_3D_HILITE,
    P_ALLOW_FIELD_REDRAW, 
    P_HGRID,
    P_VGRID,
    P_PAGE_HEIGHTS,
	
    P_PAGE_RANGES,
    P_LINK_TEXT,
    P_IMAGE_SOURCE,
	
	P_METADATA,
	
	P_CHAR_INDEX,
	P_LINE_INDEX,
	P_LIST_STYLE,
	
	P_LIST_DEPTH,
	P_LIST_INDENT,
	
	P_LIST_INDEX,
	P_SPACE_ABOVE,
	P_SPACE_BELOW,
	P_LEFT_INDENT,
	P_RIGHT_INDENT,
	
	P_PADDING,
	
	P_FLAGGED,
	
	
	P_FLAGGED_RANGES,
	
	P_TAB_ALIGN,
	
	
	P_TAB_WIDTHS,
	
	P_ENCODING,
    
    P_SELECTED_COLOR,

    P_REV_LICENSE_LIMITS, 
	P_REV_CRASH_REPORT_SETTINGS, 
	P_REV_AVAILABLE_HANDLERS, 
	P_REV_MESSAGE_BOX_LAST_OBJECT, 
	P_REV_MESSAGE_BOX_REDIRECT, 
	P_REV_LICENSE_INFO, 

	P_REV_RUNTIME_BEHAVIOUR,
	
	P_URL_RESPONSE,
	P_PARENT_SCRIPT,

	P_SECURITY_PERMISSIONS, 
	P_SECURITY_CATEGORIES, 

	P_STACK_LIMIT,
	
	
	P_LAYER_MODE,
	
	
	P_ENGINE_FOLDER,
	P_TEMPORARY_FOLDER,
	P_DOCUMENTS_FOLDER,
	P_DESKTOP_FOLDER,
	P_HOME_FOLDER,

	
	
	P_DEFER_SCREEN_UPDATES,
	
	
	P_REV_OBJECT_LISTENERS, 
	P_REV_PROPERTY_LISTENER_THROTTLE_TIME, 
	
	
	P_ALLOW_DATAGRAM_BROADCASTS,
    
    P_CONTROL_IDS,
    P_CONTROL_NAMES,
	P_CHILD_CONTROL_IDS,
    P_CHILD_CONTROL_NAMES,
	
	
	P_FIRST_ARRAY_PROP,
    P_CUSTOM_KEYS = P_FIRST_ARRAY_PROP,
    P_CUSTOM_PROPERTIES,
	P_REV_AVAILABLE_VARIABLES, 
	P_GRADIENT_FILL,
	P_GRADIENT_STROKE,
	P_BITMAP_EFFECT_DROP_SHADOW,
	P_BITMAP_EFFECT_INNER_SHADOW,
	P_BITMAP_EFFECT_OUTER_GLOW,
	P_BITMAP_EFFECT_INNER_GLOW,
	P_BITMAP_EFFECT_COLOR_OVERLAY,
    P_TEXT_STYLE,
};

enum Look_and_feel {
    LF_UNDEFINED,
    LF_AM,
    LF_MOTIF,
    LF_PM,
    LF_MAC,
    LF_WIN95,
    LF_NATIVEWIN,
    LF_NATIVEMAC,
    LF_NATIVEGTK
};

enum Repeat_form {
    RF_UNDEFINED,
    RF_EACH,
    RF_FOR,
    RF_FOREVER,
    RF_STEP,
    RF_UNTIL,
    RF_WHILE,
    RF_WITH
};

enum Reset_type {
    RT_UNDEFINED,
    RT_CURSORS,
    RT_PAINT,
    RT_PRINTING,
    RT_TEMPLATE_AUDIO_CLIP,
    RT_TEMPLATE_BUTTON,
    RT_TEMPLATE_CARD,
    RT_TEMPLATE_EPS,
    RT_TEMPLATE_FIELD,
    RT_TEMPLATE_GRAPHIC,
    RT_TEMPLATE_GROUP,
    RT_TEMPLATE_IMAGE,
    RT_TEMPLATE_PLAYER,
    RT_TEMPLATE_SCROLLBAR,
    RT_TEMPLATE_STACK,
    RT_TEMPLATE_VIDEO_CLIP,
};


enum RSA_MODE
{
    RSA_DECRYPT,
    RSA_ENCRYPT,
    RSA_SIGN,
    RSA_VERIFY
};

enum RSA_KEYTYPE
{
    RSAKEY_PUBKEY,
    RSAKEY_CERT,
    RSAKEY_PRIVKEY
};

enum Script_point {
    SP_ACCEPT,
    SP_AE,
    SP_ASK,
    SP_COMMAND,
    SP_CONVERT,
    SP_ENCRYPTION,
    SP_EXIT,
    SP_EXPORT,
    SP_FACTOR,
    SP_FIND,
    SP_FLIP,
    SP_GO,
    SP_HANDLER,
    SP_INSERT,
    SP_LOCK,
    SP_MARK,
    SP_MODE,
    SP_MOVE,
    SP_OPEN,
    SP_PLAY,
    SP_RECORD,
    SP_REPEAT,
    SP_RESET,
    SP_SHOW,
    SP_SORT,
    SP_SSL,
    SP_START,
    SP_SUGAR,
    SP_THERE,
    SP_TOOL,
    SP_UNIT,
    SP_VALIDATION,
    SP_VISUAL,
	SP_SERVER
};

enum Show_object {
    SO_UNDEFINED,
    SO_ALL,
    SO_OBJECT,
    SO_BACKGROUND,
    SO_BREAK,
    SO_CARD,
    SO_GROUPS,
    SO_MARKED,
    SO_MENU,
    SO_MESSAGE,
    SO_PALETTE,
    SO_PICTURE,
    SO_TASKBAR,
    SO_TITLEBAR,
    SO_WINDOW
};

enum Sort_type {
    ST_UNDEFINED,
    ST_OF,
    ST_BY,
    ST_LINES,
    ST_ITEMS,
    ST_MARKED,
    ST_CARDS,
    ST_TEXT,
    ST_NUMERIC,
    ST_INTERNATIONAL,
    ST_DATETIME,
    ST_ASCENDING,
    ST_DESCENDING
};


enum SSL_constants {
    SSL_UNDEFINED,
    SSL_CERTIFICATE,
    SSL_VERIFICATION
};


enum Start_constants {
    SC_UNDEFINED,
    SC_DRAG,
    SC_EDITING,
    SC_MOVING,
    SC_PLAYER,
    SC_PLAYING,
    SC_RECORDING,
	SC_SESSION,
    SC_USING,
};

enum Sugar_constants {
	SG_UNDEFINED,
	SG_NOTHING,
	SG_BROWSER,
	SG_STANDARD,
	SG_OPTIMIZED,
	SG_OPTIONS,
	SG_ANCHOR,
	SG_LINK,
	
	
	SG_LISTENER,
	
	SG_ELEVATED,
	SG_BOOKMARK,
	SG_LEVEL,
	SG_EFFECTS,
	SG_UNICODE,
	SG_URL,
	SG_INITIALLY,
	SG_OPEN,
	SG_CLOSED,
	SG_CALLER,
	
	
    SG_STRING,
};

enum Statements {
    S_UNDEFINED,
    S_ACCEPT,
    S_ADD,
    S_ANSWER,
    S_ASK,
    S_BEEP,
    S_BREAK,
    S_BREAKPOINT,
    S_CALL,
    S_CANCEL,
    S_CHOOSE,
    S_CLICK,
    S_CLONE,
    S_CLOSE,
    S_COMBINE,
    S_COMPACT,
    S_CONSTANT,
    S_CONVERT,
    S_COPY,
    S_CREATE,
    S_CROP,
    S_CUT,
    S_DEBUGDO,
    S_DECRYPT,
    S_DEFINE,
    S_DELETE,
    S_DISABLE,
	
	S_DISPATCH,
    S_DIVIDE,
    S_DO,
    S_DOMENU,
    S_DRAG,
    S_DRAWER,
	S_ECHO,
    S_EDIT,
    S_ENABLE,
    S_ENCRYPT,
    S_EXIT,
    S_EXPORT,
    S_FILTER,
    S_FIND,
    S_FLIP,
    S_FOCUS,
    S_GET,
    S_GLOBAL,
    S_GO,
    S_GRAB,
    S_GROUP,
    S_HIDE,
    S_HILITE,
    S_IF,
    S_IMPORT,
	S_INCLUDE,
    S_INSERT,
	S_INTERNAL, 
    S_INTERSECT,
    S_KILL,
    S_LAUNCH,
    S_LIBRARY,
    S_LOCAL,
    S_LOAD,
    S_LOCK,
    S_MARK,
    S_MODAL,
    S_MODELESS,
    S_MOVE,
    S_MULTIPLY,
    S_NEXT,
    S_OPEN,
    S_OPTION,
    S_PALETTE,
    S_PASS,
    S_PASTE,
    S_PLACE,
    S_PLAY,
    S_POP,
    S_POPUP,
    S_POST,
    S_PREPARE,
    S_PRINT,
    S_PULLDOWN,
    S_PUSH,
    S_PUT,
    S_QUIT,
    S_READ,
    S_RECORD,
    S_REDO,
	S_RELAYER,
    S_RELEASE,
    S_REMOVE,
    S_RENAME,
    S_REPEAT,
    S_REPLACE,
    S_REPLY,
    S_REQUEST,
	S_REQUIRE,
    S_RESET,
    S_RETURN,
    S_REVERT,
	S_REV_RELICENSE, 
    S_ROTATE,
    S_SAVE,
    S_SCRIPT_ERROR,
    S_SEEK,
    S_SELECT,
    S_SEND,
    S_SET,
    S_SHEET,
    S_SHOW,
    S_SORT,
    S_SPLIT,
    S_START,
    S_STOP,
    S_SUBTRACT,
    S_SWITCH,
    S_THROW,
    S_TOP_LEVEL,
    S_TRANSPOSE,
    S_TRY,
    S_TYPE,
    S_UNDEFINE,
    S_UNDO,
    S_UNGROUP,
    S_UNHILITE,
    S_UNION,
    S_UNLOAD,
    S_UNLOCK,
    S_UNMARK,
    S_VISUAL,
    S_WAIT,
    S_WRITE
};






typedef uint32_t Symbol_type;
enum {
    ST_ERR,
    ST_EOF,
    ST_EOL,
    ST_SPC,
    ST_COM,
    ST_OP,
    ST_MIN,
    ST_NUM,
    ST_LP,
    ST_RP,
    ST_LB,
    ST_RB,
    ST_SEP,
    ST_SEMI,
    ST_ID,
    ST_ESC,
    ST_LIT,
	
	
	
	
	ST_DATA,
	ST_TAG
};

enum There_mode {
    TM_UNDEFINED,
    TM_DIRECTORY,
    TM_FILE,
    TM_PROCESS,
    TM_URL
};


enum Token_type {
    TT_UNDEFINED,
    TT_EOF,
    TT_NO_MATCH,
    TT_HANDLER,
    TT_STATEMENT,
    TT_CASE,
    TT_CATCH,
    TT_DEFAULT,
    TT_THEN,
    TT_ELSE,
    TT_END,
    TT_IT,
    TT_THE,
    TT_CHUNK,
    TT_FINALLY,
    TT_FUNCTION,
    TT_TOOL,
    TT_UNOP,
    TT_BINOP,
    TT_BIN_OR_UNOP,
    TT_LPAREN,
    TT_RPAREN,
    TT_PROPERTY,
    TT_PREP,
    TT_OF,
    TT_IN,
    TT_TO,
    TT_FROM,
    TT_CLASS,
    TT_VARIABLE,
    TT_VISUAL,
	TT_SERVER
};

enum Tool {
    T_UNDEFINED,
    T_BROWSE,
    T_BRUSH,
    T_BUCKET,
    T_BUTTON,
    T_CURVE,
    T_DROPPER,
    T_ERASER,
    T_FIELD,
    T_GRAPHIC,
    T_HELP,
    T_IMAGE,
    T_LASSO,
    T_LINE,
    T_OVAL,
    T_PENCIL,
    T_POINTER,
    T_POLYGON,
    T_RECTANGLE,
    T_REGULAR_POLYGON,
    T_ROUND_RECT,
    T_SCROLLBAR,
    T_PLAYER,
    T_BROWSER,
    T_SELECT,
    T_SPRAY,
    T_TEXT
};


enum Value_format {
    VF_UNDEFINED,
    VF_STRING,
    VF_NUMBER,
    VF_BOTH,
    VF_ARRAY
};

enum Url_type {
    UT_UNDEFINED,
    UT_FILE,
    UT_BINFILE,
    UT_RESFILE,
    UT_HTTP,
    UT_MAIL,
    UT_FTP
};

enum Visual_effects {
    VE_UNDEFINED,
    VE_EFFECT,
    VE_EXTRA,
    VE_CLOSE,
    VE_OPEN,
    VE_IN,
    VE_OUT,
    VE_DOWN,
    VE_LEFT,
    VE_RIGHT,
    VE_UP,
    VE_BOTTOM,
    VE_CENTER,
    VE_TOP,
    VE_BARN,
    VE_DOOR,
    VE_CHECKERBOARD,
    VE_DISSOLVE,
    VE_IRIS,
    VE_PLAIN,
    VE_PUSH,
    VE_REVEAL,
    VE_SCROLL,
    VE_SHRINK,
    VE_STRETCH,
    VE_VENETIAN,
    VE_BLINDS,
    VE_WIPE,
    VE_ZOOM,
    VE_VERY,
    VE_VSLOW,
    VE_SLOW,
    VE_NORMAL,
    VE_FAST,
    VE_VFAST,
    VE_TO,
    VE_FROM,
    VE_BLACK,
    VE_CARD,
    VE_GRAY,
    VE_INVERSE,
    VE_WHITE,
    VE_WITH,
    VE_SOUND,
    VE_QTEFFECT,
    VE_OPTION,
	VE_CIEFFECT,
	VE_CURL,
	VE_FLIP,
};

enum Server_keywords
{
	SK_HEADER,
	SK_CONTENT,
	SK_MARKUP,
	SK_NEW,
	SK_UNICODE,
	SK_SECURE,
	SK_HTTPONLY,
};

#line 1 "c:\\github\\livecode-runrev\\engine\\src\\parseerrors.h"


























enum Parse_errors
{
	
	PE_UNDEFINED,
	
	
	PE_OBJECT_NAME,
	
	
	PE_OBJECT_NOTLICENSED,
	
	
	PE_ABS_BADPARAM,
	
	
	PE_ACCEPT_BADEXP,
	
	
	PE_ACOS_BADPARAM,
	
	
	PE_ADD_BADDEST,
	
	
	PE_ADD_BADEXP,
	
	
	PE_ADD_NOTO,
	
	
	PE_ALIASREFERENCE_BADPARAM,
	
	
	PE_ANNUITY_BADPARAM,
	
	
	PE_ANSWER_BADQUESTION,
	
	
	PE_ANSWER_BADRESPONSE,
	
	
	PE_ANSWER_BADTITLE,
	
	
	PE_ARRAYOP_BADARRAY,
	
	
	PE_ARRAYOP_NOWITH,
	
	
	PE_ARRAYOP_BADEXP,
	
	
	PE_ARROWKEY_BADEXP,
	
	
	PE_ASIN_BADPARAM,
	
	
	PE_ASK_BADQUESTION,
	
	
	PE_ASK_BADREPLY,
	
	
	PE_ASK_BADTITLE,
	
	
	PE_ATAN2_BADPARAM,
	
	
	PE_ATAN_BADPARAM,
	
	
	PE_AVERAGE_BADPARAM,
	
	
	PE_BASE64DECODE_BADPARAM,
	
	
	PE_BASE64ENCODE_BADPARAM,
	
	
	PE_BASECONVERT_BADPARAM,
	
	
	PE_BINARYD_BADPARAM,
	
	
	PE_BINARYE_BADPARAM,
	
	
	PE_CANCEL_BADEXP,
	
	
	PE_CHARTONUM_BADPARAM,
	
	
	PE_CHOOSE_BADEXP,
	
	
	PE_CHOOSE_NOTOKEN,
	
	
	PE_CHUNK_BADCHUNK,
	
	
	PE_CHUNK_BADDEST,
	
	
	PE_CHUNK_BADEXP,
	
	
	PE_CHUNK_BADORDER,
	
	
	PE_CHUNK_BADPREP,
	
	
	PE_CHUNK_BADRANGE,
	
	
	PE_CHUNK_BADSTACKREF,
	
	
	PE_CHUNK_NOCHUNK,
	
	
	PE_CHUNK_NOENDEXP,
	
	
	PE_CHUNK_NOSTARTEXP,
	
	
	PE_CHUNK_NOVARIABLE,
	
	
	PE_CLICK_BADBUTTONEXP,
	
	
	PE_CLICK_BADLOCATIONEXP,
	
	
	PE_CLICK_NOAT,
	
	
	PE_CLONE_BADCHUNK,
	
	
	PE_CLONE_BADNAME,
	
	
	PE_CLOSE_BADEXP,
	
	
	PE_CLOSE_BADNAME,
	
	
	PE_CLOSE_NOTYPE,
	
	
	PE_COMPACT_BADPARAM,
	
	
	PE_COMPRESS_BADPARAM,
	
	
	PE_COMPOUND_BADPARAM,
	
	
	PE_CONSTANT_BADINIT,
	
	
	PE_CONSTANT_NOINIT,
	
	
	PE_CONVERT_BADAND,
	
	
	PE_CONVERT_NOCONTAINER,
	
	
	PE_CONVERT_NOFORMAT,
	
	
	PE_CONVERT_NOTFORMAT,
	
	
	PE_CONVERT_NOTO,
	
	
	PE_COPY_BADDEST,
	
	
	PE_COS_BADPARAM,
	
	
	PE_CREATE_BADBGORCARD,
	
	
	PE_CREATE_BADFILENAME,
	
	
	PE_CREATE_BADTYPE,
	
	
	PE_CREATE_NOFILENAME,
	
	
	PE_CREATE_NONAME,
	
	
	PE_CREATE_NOTTYPE,
	
	
	PE_CREATE_NOTYPE,
	
	
	PE_CROP_BADIMAGE,
	
	
	PE_CROP_BADRECT,
	
	
	PE_DECOMPRESS_BADPARAM,
	
	
	PE_DEFINE_BADOBJECT,
	
	
	PE_DEFINE_INVALIDNAME,
	
	
	PE_DELETE_BADFILEEXP,
	
	
	PE_DELETE_BADVAREXP,
	
	
	PE_DISABLE_BADCHUNK,
	
	
	PE_DIVIDE_BADDEST,
	
	
	PE_DIVIDE_BADEXP,
	
	
	PE_DIVIDE_NOBY,
	
	
	PE_DO_BADEXP,
	
	
	PE_DO_BADLANG,
	
	
	PE_DOMENU_BADEXP,
	
	
	PE_DRAG_BADBUTTONEXP,
	
	
	PE_DRAG_BADENDLOCEXP,
	
	
	PE_DRAG_BADSTARTLOCEXP,
	
	
	PE_DRAG_NOFROM,
	
	
	PE_DRAG_NOTO,
	
	
	PE_DRIVERNAMES_BADPARAM,
	
	
	PE_DRIVES_BADPARAM,
	
	
	PE_EDIT_NOOF,
	
	
	PE_EDIT_NOSCRIPT,
	
	
	PE_EDIT_NOTARGET,
	
	
	PE_ENCRYPT_BADPARAM,
	
	
	PE_EXIT_BADDEST,
	
	
	PE_EXIT_NODEST,
	
	
	PE_EXP10_BADPARAM,
	
	
	PE_EXP1_BADPARAM,
	
	
	PE_EXP2_BADPARAM,
	
	
	PE_EXPORT_BADFILENAME,
	
	
	PE_EXPORT_BADMASKNAME,
	
	
	PE_EXPORT_BADTYPE,
	
	
	PE_EXPORT_NOFILE,
	
	
	PE_EXPORT_NOMASK,
	
	
	PE_EXPORT_NOTO,
	
	
	PE_EXPORT_NOTYPE,
	
	
	PE_EXPRESSION_BADCHUNK,
	
	
	PE_EXPRESSION_BADFUNCTION,
	
	
	PE_EXPRESSION_BADPROPERTY,
	
	
	PE_EXPRESSION_DOUBLEBINOP,
	
	
	PE_EXPRESSION_NOBINOP,
	
	
	PE_EXPRESSION_NOFACT,
	
	
	PE_EXPRESSION_NOLFACT,
	
	
	PE_EXPRESSION_NORPAR,
	
	
	PE_EXPRESSION_NOTFACT,
	
	
	PE_EXPRESSION_NOTLITERAL,
	
	
	PE_EXPRESSION_WANTRPAR,
	
	
	PE_EXP_BADPARAM,
	
	
	PE_EXTENTS_BADPARAM,
	
	
	PE_FACTOR_BADPARAM,
	
	
	PE_FACTOR_NOOF,
	
	
	PE_FACTOR_NOLPAREN,
	
	
	PE_FACTOR_NORPAREN,
	
	
	PE_FACTOR_NOTSEP,
	
	
	PE_FILTER_BADDEST,
	
	
	PE_FILTER_BADEXP,
	
	
	PE_FILTER_NOWITH,
	
	
	PE_FIND_BADFIELD,
	
	
	PE_FIND_BADSTRING,
	
	
	PE_FIND_NOSTRING,
	
	
	PE_FLIP_BADIMAGE,
	
	
	PE_FLIP_NODIR,
	
	
	PE_FLUSHEVENTS_BADPARAM,
	
	
	PE_FOCUS_BADOBJECT,
	
	
	PE_FONTNAMES_BADPARAM,
	
	
	PE_FONTSIZES_BADPARAM,
	
	
	PE_FONTSTYLES_BADPARAM,
	
	
	PE_FORMAT_BADPARAM,
	
	
	PE_FUNCTION_BADFORM,
	
	
	PE_FUNCTION_BADOBJECT,
	
	
	PE_FUNCTION_BADPARAMS,
	
	
	PE_FUNCTION_CANTMODIFY,
	
	
	PE_GENERAL_CANTMODIFY,
	
	
	PE_GENERAL_NOMODIFY,
	
	
	PE_GET_BADEXP,
	
	
	PE_GLOBAL_BADNAME,
	
	
	PE_GLOBALLOC_BADPOINT,
	
	
	PE_GO_BADCHUNKDEST,
	
	
	PE_GO_BADCHUNKEXP,
	
	
	PE_GO_BADCHUNKORDER,
	
	
	PE_GO_BADCHUNKTYPE,
	
	
	PE_GO_BADDESTEXP,
	
	
	PE_GO_BADDIRECT,
	
	
	PE_GO_BADPREP,
	
	
	PE_GO_BADWINDOW,
	
	
	PE_GO_DUPCHUNK,
	
	
	PE_GO_NODEST,
	
	
	PE_GO_NOID,
	
	
	PE_GO_NOMODE,
	
	
	PE_GRAB_BADCHUNK,
	
	
	PE_HANDLERLIST_BADCHAR,
	
	
	PE_HANDLERLIST_BADEOL,
	
	
	PE_HANDLERLIST_BADHANDLER,
	
	
	PE_HANDLER_BADCOMMAND,
	
	
	PE_HANDLER_BADEND,
	
	
	PE_HANDLER_BADLINE,
	
	
	PE_HANDLER_BADNAME,
	
	
	PE_HANDLER_BADPARAM,
	
	
	PE_HANDLER_BADPARAMEOL,
	
	
	PE_HANDLER_BADVAR,
	
	
	PE_HANDLER_NOCOMMAND,
	
	
	PE_HANDLER_NOEND,
	
	
	PE_HANDLER_NONAME,
	
	
	PE_HANDLER_NOTCOMMAND,
	
	
	PE_HASMEMORY_BADPARAM,
	
	
	PE_HIDE_BADEFFECT,
	
	
	PE_HIDE_BADTARGET,
	
	
	PE_HOSTADDRESS_BADSOCKET,
	
	
	PE_HOSTATON_BADADDRESS,
	
	
	PE_HOSTNAME_BADNAME,
	
	
	PE_HOSTNTOA_BADNAME,
	
	
	PE_IF_BADCONDITION,
	
	
	PE_IF_BADEOL,
	
	
	PE_IF_BADSTATEMENT,
	
	
	PE_IF_BADTYPE,
	
	
	PE_IF_NOTCOMMAND,
	
	
	PE_IF_NOTHEN,
	
	
	PE_IF_WANTEDENDIF,
	
	
	PE_IMPORT_BADFILENAME,
	
	
	PE_IMPORT_BADMASKNAME,
	
	
	PE_IMPORT_BADTYPE,
	
	
	PE_IMPORT_NOFILE,
	
	
	PE_IMPORT_NOFROM,
	
	
	PE_IMPORT_NOMASK,
	
	
	PE_IMPORT_NOTYPE,
	
	
	PE_INSERT_BADOBJECT,
	
	
	PE_INSERT_NOINTO,
	
	
	PE_INSERT_NOPLACE,
	
	
	PE_INSERT_NOSCRIPT,
	
	
	PE_INTERSECT_NOOBJECT,
	
	
	PE_IS_BADAMONGTYPE,
	
	
	PE_IS_BADVALIDTYPE,
	
	
	PE_IS_NORIGHT,
	
	
	PE_IS_NOVALIDTYPE,
	
	
	PE_ISNUMBER_BADPARAM,
	
	
	PE_ISOTOMAC_BADPARAM,
	
	
	PE_KEYS_BADPARAM,
	
	
	PE_KILL_BADNAME,
	
	
	PE_KILL_NOPROCESS,
	
	
	PE_LAUNCH_BADAPPEXP,
	
	
	PE_LENGTH_BADPARAM,
	
	
	PE_LN1_BADPARAM,
	
	
	PE_LN_BADPARAM,
	
	
	PE_LOAD_BADURLEXP,
	
	
	PE_LOAD_BADMESSAGEEXP,
	
	
	PE_LOCAL_BADINIT,
	
	
	PE_LOCAL_BADNAME,
	
	
	PE_LOCAL_SHADOW,
	
	
	PE_LOCALLOC_BADPOINT,
	
	
	PE_LOCK_NOTARGET,
	
	
	PE_LOCK_NOTTARGET,
	
	
	PE_LOG10_BADPARAM,
	
	
	PE_LOG2_BADPARAM,
	
	
	PE_LONGFILEPATH_BADPARAM,
	
	
	PE_MACTOISO_BADPARAM,
	
	
	PE_MARK_BADFIELD,
	
	
	PE_MARK_BADSTRING,
	
	
	PE_MARK_BADWHEREEXP,
	
	
	PE_MARK_NOBYORWHERE,
	
	
	PE_MARK_NOCARDS,
	
	
	PE_MARK_NOFINDING,
	
	
	PE_MARK_NOSTRING,
	
	
	PE_MARK_NOTBYORWHERE,
	
	
	PE_MARK_NOTCARDS,
	
	
	PE_MATCH_BADPARAM,
	
	
	PE_MATRIXMULT_BADPARAM,
	
	
	PE_MAX_BADPARAM,
	
	
	PE_MCISENDSTRING_BADPARAM,
	
	
	PE_MD5DIGEST_BADPARAM,
	
	
	PE_ME_THE,
	
	
	PE_MEDIAN_BADPARAM,
	
	
	PE_MERGE_BADPARAM,
	
	
	PE_MIN_BADPARAM,
	
	
	PE_MOUSE_BADPARAM,
	
	
	PE_MOVE_BADOBJECT,
	
	
	PE_MOVE_BADENDLOCEXP,
	
	
	PE_MOVE_BADSTARTLOCEXP,
	
	
	PE_MOVE_BADWITHOUT,
	
	
	PE_MOVE_NOTO,
	
	
	PE_MULTIPLY_BADDEST,
	
	
	PE_MULTIPLY_BADEXP,
	
	
	PE_MULTIPLY_NOBY,
	
	
	PE_NEXT_NOREPEAT,
	
	
	PE_NEXT_NOTREPEAT,
	
	
	PE_NUMTOCHAR_BADPARAM,
	
	
	PE_OFFSET_BADPARAMS,
	
	
	PE_OPEN_BADMESSAGE,
	
	
	PE_OPEN_BADMODE,
	
	
	PE_OPEN_BADNAME,
	
	
	PE_OPEN_BADTYPE,
	
	
	PE_OPEN_NOMODE,
	
	
	PE_OPEN_NOTYPE,
	
	
	PE_PARAM_BADEXP,
	
	
	PE_PARAM_BADPARAM,
	
	
	PE_PARSE_BADCHAR,
	
	
	PE_PARSE_BADLIT,
	
	
	PE_PASS_NOMESSAGE,
	
	
	PE_PAUSE_BADCLIP,
	
	
	PE_PEERADDRESS_BADSOCKET,
	
	
	PE_PLACE_BADBACKGROUND,
	
	
	PE_PLACE_BADCARD,
	
	
	PE_PLAY_BADCLIP,
	
	
	PE_PLAY_BADLOC,
	
	
	PE_PLAY_BADOPTIONS,
	
	
	PE_PLAY_BADTEMPO,
	
	
	PE_PLAY_BADSTACK,
	
	
	PE_POP_BADCHUNK,
	
	
	PE_POP_BADPREP,
	
	
	PE_POP_NOCARD,
	
	
	PE_POST_BADSOURCEEXP,
	
	
	PE_POST_NOTO,
	
	
	PE_POST_BADDESTEXP,
	
	
	PE_PRINT_BADTARGET,
	
	
	PE_PRINT_BADFROMEXP,
	
	
	PE_PRINT_BADRECTEXP,
	
	
	PE_PRINT_NOTO,
	
	
	PE_PRINT_BADTOEXP,
	
	
	PE_PROPERTY_BADCHUNK,
	
	
	PE_PROPERTY_BADINDEX,
	
	
	PE_PROPERTY_CANTMODIFY,
	
	
	PE_PROPERTY_MISSINGOFORIN,
	
	
	PE_PROPERTY_MISSINGTARGET,
	
	
	PE_PROPERTY_NOPROP,
	
	
	PE_PROPERTY_NOTAPROP,
	
	
	PE_PROPERTY_NOTOF,
	
	
	PE_PROPERTY_NOTOFORIN,
	
	
	PE_PUSH_BADEXP,
	
	
	PE_PUT_BADCHUNK,
	
	
	PE_PUT_BADEXP,
	
	
	PE_PUT_BADPREP,
	
	
	PE_QUERYREGISTRY_BADPARAM,
	
	
	PE_RANDOM_BADPARAM,
	
	
	PE_READ_BADAT,
	
	
	PE_READ_BADCOND,
	
	
	PE_READ_BADMESS,
	
	
	PE_READ_BADNAME,
	
	
	PE_READ_BADTYPE,
	
	
	PE_READ_NOCOND,
	
	
	PE_READ_NOFROM,
	
	
	PE_READ_NOTCOND,
	
	
	PE_READ_NOTYPE,
	
	
	PE_RECORD_BADFILEEXP,
	
	
	PE_REMOVE_BADOBJECT,
	
	
	PE_REMOVE_NOFROM,
	
	
	PE_REMOVE_NOPLACE,
	
	
	PE_RENAME_BADEXP,
	
	
	PE_REPEAT_BADCOMMAND,
	
	
	PE_REPEAT_BADCOND,
	
	
	PE_REPEAT_BADCONDEOL,
	
	
	PE_REPEAT_BADCONDTYPE,
	
	
	PE_REPEAT_BADFORMEOL,
	
	
	PE_REPEAT_BADFORMTYPE,
	
	
	PE_REPEAT_BADSTATEMENT,
	
	
	PE_REPEAT_BADTOKEN,
	
	
	PE_REPEAT_BADWITHENDEXP,
	
	
	PE_REPEAT_BADWITHSTARTEXP,
	
	
	PE_REPEAT_BADWITHSTEPEXP,
	
	
	PE_REPEAT_BADWITHVAR,
	
	
	PE_REPEAT_NODOWNTO,
	
	
	PE_REPEAT_NOEQUALS,
	
	
	PE_REPEAT_NOOF,
	
	
	PE_REPEAT_NOTCOMMAND,
	
	
	PE_REPEAT_NOTEQUALS,
	
	
	PE_REPEAT_NOTWITHTO,
	
	
	PE_REPEAT_NOWITHTO,
	
	
	PE_REPEAT_NOWITHVAR,
	
	
	PE_REPEAT_WANTEDENDREPEAT,
	
	
	PE_REPLACE_BADCONTAINER,
	
	
	PE_REPLACE_BADEXP,
	
	
	PE_REPLACETEXT_BADPARAM,
	
	
	PE_REPLY_BADEXP,
	
	
	PE_REPLY_BADKEYWORD,
	
	
	PE_REQUEST_BADEXP,
	
	
	PE_REQUEST_BADPROGRAM,
	
	
	PE_REQUEST_NOTYPE,
	
	
	PE_REQUEST_NOTTYPE,
	
	
	PE_RESET_NOTYPE,
	
	
	PE_RESOURCES_BADPARAM,
	
	
	PE_RETURN_BADEXP,
	
	
	PE_ROTATE_BADIMAGE,
	
	
	PE_ROTATE_BADANGLE,
	
	
	PE_ROUND_BADPARAM,
	
	
	PE_SAVE_BADEXP,
	
	
	PE_SAVE_BADFILEEXP,
	
	
	PE_SEEK_BADMODE,
	
	
	PE_SEEK_BADNAME,
	
	
	PE_SEEK_BADTYPE,
	
	
	PE_SEEK_BADWHERE,
	
	
	PE_SEEK_NOIN,
	
	
	PE_SEEK_NOMODE,
	
	
	PE_SEEK_NOTYPE,
	
	
	PE_SELECT_NOTARGET,
	
	
	PE_SELECT_NOOF,
	
	
	PE_SELECT_BADTARGET,
	
	
	PE_SELECTEDBUTTON_NOFAMILY,
	
	
	PE_SELECTEDBUTTON_NOOBJECT,
	
	
	PE_SEND_BADEXP,
	
	
	PE_SEND_BADEVENTTYPE,
	
	
	PE_SEND_BADTARGET,
	
	
	PE_SET_BADEXP,
	
	
	PE_SET_NOPROP,
	
	
	PE_SET_NOTHE,
	
	
	PE_SET_NOTO,
	
	
	PE_SETREGISTRY_BADPARAM,
	
	
	PE_SHELL_BADPARAM,
	
	
	PE_SHORTFILEPATH_BADPARAM,
	
	
	PE_SHOW_BADEFFECT,
	
	
	PE_SHOW_BADLOCATION,
	
	
	PE_SHOW_BADTARGET,
	
	
	PE_SIN_BADPARAM,
	
	
	PE_SORT_BADEXPRESSION,
	
	
	PE_SORT_BADTARGET,
	
	
	PE_SORT_NOTARGET,
	
	
	PE_SPECIALFOLDERPATH_BADTYPE,
	
	
	PE_SQRT_BADPARAM,
	
	
	PE_START_BADCHUNK,
	
	
	PE_START_NOTTYPE,
	
	
	PE_START_NOTYPE,
	
	
	PE_STATEMENT_BADCHUNK,
	
	
	PE_STATEMENT_BADINEXP,
	
	
	PE_STATEMENT_BADKEY,
	
	
	PE_STATEMENT_BADPARAM,
	
	
	PE_STATEMENT_BADPARAMS,
	
	
	PE_STATEMENT_BADSEP,
	
	
	PE_STATEMENT_NOKEY,
	
	
	PE_STATEMENT_NOTAND,
	
	
	PE_STATEMENT_NOTSEP,
	
	
	PE_STDDEV_BADPARAM,
	
	
	PE_STOP_BADCHUNK,
	
	
	PE_STOP_NOTTYPE,
	
	
	PE_STOP_NOTYPE,
	
	
	PE_SUBTRACT_BADDEST,
	
	
	PE_SUBTRACT_BADEXP,
	
	
	PE_SUBTRACT_NOFROM,
	
	
	PE_SUBWINDOW_BADEXP,
	
	
	PE_SUM_BADPARAM,
	
	
	PE_SWITCH_BADCASECONDITION,
	
	
	PE_SWITCH_BADCONDITION,
	
	
	PE_SWITCH_BADSTATEMENT,
	
	
	PE_SWITCH_BADTYPE,
	
	
	PE_SWITCH_NOTCOMMAND,
	
	
	PE_SWITCH_WANTEDENDSWITCH,
	
	
	PE_TAN_BADPARAM,
	
	
	PE_THERE_BADFILE,
	
	
	PE_THERE_NOIS,
	
	
	PE_THERE_NOOBJECT,
	
	
	PE_THROW_BADERROR,
	
	
	PE_TOLOWER_BADPARAM,
	
	
	PE_TOPSTACK_BADPARAM,
	
	
	PE_TOUPPER_BADPARAM,
	
	
	PE_TRANSPOSE_BADPARAM,
	
	
	PE_TRUNC_BADPARAM,
	
	
	PE_TRY_BADSTATEMENT,
	
	
	PE_TRY_BADTYPE,
	
	
	PE_TRY_NOTCOMMAND,
	
	
	PE_TRY_WANTEDENDTRY,
	
	
	PE_TYPE_BADEXP,
	
	
	PE_UNGROUP_BADGROUP,
	
	
	PE_UNIDECODE_BADPARAM,
	
	
	PE_UNIENCODE_BADPARAM,
	
	
	PE_UNLOAD_BADURLEXP,
	
	
	PE_UNLOCK_BADEFFECT,
	
	
	PE_UNLOCK_NOTARGET,
	
	
	PE_UNLOCK_NOTTARGET,
	
	
	PE_URLDECODE_BADPARAM,
	
	
	PE_URLENCODE_BADPARAM,
	
	
	PE_URLSTATUS_BADPARAM,
	
	
	PE_VALUE_BADPARAM,
	
	
	PE_VALUE_BADOBJECT,
	
	
	PE_VARIABLE_BADINDEX,
	
	
	PE_VARIABLE_NORBRACE,
	
	
	PE_VISUAL_DUPDIRECTION,
	
	
	PE_VISUAL_DUPVISUAL,
	
	
	PE_VISUAL_NOTID,
	
	
	PE_VISUAL_NOTOKEN,
	
	
	PE_VISUAL_NOTVISUAL,
	
	
	PE_WAIT_BADCOND,
	
	
	PE_WAIT_NODURATION,
	
	
	PE_WITHIN_NOOBJECT,
	
	
	PE_WITHIN_BADPOINT,
	
	
	PE_WRITE_BADAT,
	
	
	PE_WRITE_BADEXP,
	
	
	PE_WRITE_BADNAME,
	
	
	PE_WRITE_BADTYPE,
	
	
	PE_WRITE_NOTO,
	
	
	PE_WRITE_NOTYPE,
	
	
	PE_ENCRYPTION_NOSOURCE,
	
	
	PE_ENCRYPTION_NOCIPHER,
	
	
	PE_ENCRYPTION_NOKEY,
	
	
	PE_ENCRYPTION_NOBIT,
	
	
	PE_ENCRYPTION_BADSOURCE,
	
	
	PE_ENCRYPTION_BADCIPHER,
	
	
	PE_ENCRYPTION_BADKEY,
	
	
	PE_ENCRYPTION_BADSALT,
	
	
	PE_ENCRYPTION_BADIV,
	
	
	PE_ENCRYPTION_BADBIT,
	
	
	PE_ENCRYPTION_BADPARAM,
	
	
	PE_VISUAL_NOPARAM,
	
	
	PE_VISUAL_BADPARAM,
	
	
	PE_PRIVATE_BADPASS,
	
	
	PE_BEGINSENDS_NOWITH,
	
	
	PE_NUMTOBYTE_BADPARAM,
	
	
	PE_BYTETONUM_BADPARAM,
	
	
	PE_ARRAYDECODE_BADPARAM,
	
	
	PE_ARRAYENCODE_BADPARAM,
	
	
	PE_DISPATCH_BADMESSAGE,
	
	
	PE_DISPATCH_BADPARAMS,
	
	
	PE_DISPATCH_BADTARGET,
	
	
	PE_DO_BADENV,
	
	
	PE_EXPORT_BADPALETTE,
	
	
	PE_OPENPRINTING_NODST,
	
	
	PE_OPENPRINTING_NOFILENAME,
	
	
	PE_OPENPRINTING_BADOPTIONS,
	
	
	PE_PRINTANCHOR_BADNAMEEXP,
	
	
	PE_PRINTANCHOR_NOATEXP,
	
	
	PE_PRINTANCHOR_BADTOEXP,
	
	
	PE_PRINTLINK_NOTOEXP,
	
	
	PE_PRINTLINK_BADTOEXP,
	
	
	PE_PRINTLINK_NOAREAEXP,
	
	
	PE_PRINTLINK_BADAREAEXP,
	
	
	PE_HANDLER_DUPPARAM,
	
	
	PE_INTERNAL_BADVERB,
	
	
	PE_INTERNAL_BADNOUN,
	
	
	PE_INTERNAL_BADEOS,
	
	
	PE_RANDOMBYTES_BADPARAM,
	
	
	PE_SHA1DIGEST_BADPARAM,
	
	
	PE_PRINTBOOKMARK_BADTITLEEXP,
	
	
	PE_PRINTBOOKMARK_NOLEVEL,
	
	
	PE_PRINTBOOKMARK_BADLEVELEXP,
	
	
	PE_PRINTBOOKMARK_BADATEXP,
	
	
	PE_PRINTBOOKMARK_NOBOOKMARK,
	
	
	PE_PRINTBOOKMARK_BADINITIALEXP,
	
	
	PE_SCRIPT_BADHANDLER,
	
	
	PE_SCRIPT_BADHANDLERTYPE,
	
	
	PE_SCRIPT_BADVAR,
	
	
	PE_SCRIPT_BADSTATEMENT,
	
	
	PE_SCRIPT_NOTSTATEMENT,
	
	
	PE_SCRIPT_BADCOMMAND,
	
	
	PE_SCRIPT_NOTCOMMAND,
	
	
	PE_SCRIPT_BADEOL,
	
	
	PE_SCRIPT_BADCHAR,
	
	
	PE_INCLUDE_BADFILENAME,
	
	
	PE_REQUIRE_BADFILENAME,
	
	
	PE_SCRIPT_BADECHO,
	
	
	PE_LOCK_BADRECT,
	
	
	PE_LOCK_NORECT,

	
	PE_RELAYER_BADCONTROL,

	
	PE_RELAYER_BADRELATION,

	
	PE_RELAYER_BADTARGET,
	
	
	PE_CONTROLATLOC_BADPARAM,
	
	
	PE_ARRAYOP_BADFORM,
	
	
	PE_UUID_BADPARAM,

	
	PE_AVGDEV_BADPARAM,

	
	PE_GEO_MEAN_BADPARAM,

	
	PE_HAR_MEAN_BADPARAM,

	
	PE_POP_STDDEV_BADPARAM,

	
	PE_POP_VARIANCE_BADPARAM,

	
	PE_VARIANCE_BADPARAM,
};

extern const char *MCparsingerrors;

#line 1626 "c:\\github\\livecode-runrev\\engine\\src\\parseerrors.h"


#line 2100 "c:\\github\\livecode-runrev\\engine\\src\\parsedef.h"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\executionerrors.h"


























enum Exec_errors
{
	
	EE_UNDEFINED,
	
	
	EE_NO_MEMORY,
	
	
	EE_RECURSION_LIMIT,
	
	
	EE_ABS_BADSOURCE,
	
	
	EE_ACCEPT_BADEXP,
	
	
	EE_ACLIP_LOUDNESSNAN,
	
	
	EE_ACOS_BADSOURCE,
	
	
	EE_ACOS_DOMAIN,
	
	
	EE_ADD_BADARRAY,
	
	
	EE_ADD_BADDEST,
	
	
	EE_ADD_BADSOURCE,
	
	
	EE_ADD_CANTSET,
	
	
	EE_ADD_MISMATCH,
	
	
	EE_ALIASREFERENCE_BADSOURCE,
	
	
	EE_AND_BADLEFT,
	
	
	EE_AND_BADRIGHT,
	
	
	EE_ANDBITS_BADLEFT,
	
	
	EE_ANDBITS_BADRIGHT,
	
	
	EE_ANNUITY_BADPERIODS,
	
	
	EE_ANNUITY_BADRATE,
	
	
	EE_ANSWER_BADQUESTION,
	
	
	EE_ANSWER_BADRESPONSE,
	
	
	EE_ANSWER_BADTITLE,
	
	
	EE_ARRAYOP_BADEXP,
	
	
	EE_ARROWKEY_BADEXP,
	
	
	EE_ARROWKEY_NOTAKEY,
	
	
	EE_ASIN_BADSOURCE,
	
	
	EE_ASIN_DOMAIN,
	
	
	EE_ASK_BADQUESTION,
	
	
	EE_ASK_BADREPLY,
	
	
	EE_ASK_BADTITLE,
	
	
	EE_ATAN2_BADS1,
	
	
	EE_ATAN2_BADS2,
	
	
	EE_ATAN2_DOMAIN,
	
	
	EE_ATAN_BADSOURCE,
	
	
	EE_ATAN_DOMAIN,
	
	
	EE_AVERAGE_BADSOURCE,
	
	
	EE_BASE64DECODE_BADSOURCE,
	
	
	EE_BASE64ENCODE_BADSOURCE,
	
	
	EE_BASECONVERT_BADDESTBASE,
	
	
	EE_BASECONVERT_BADSOURCE,
	
	
	EE_BASECONVERT_BADSOURCEBASE,
	
	
	EE_BASECONVERT_CANTCONVERT,
	
	
	EE_BASECONVERT_NOTBASE10,
	
	
	EE_BEEP_BADEXP,
	
	
	EE_BINARYD_BADDEST,
	
	
	EE_BINARYD_BADFORMAT,
	
	
	EE_BINARYD_BADPARAM,
	
	
	EE_BINARYD_BADSOURCE,
	
	
	EE_BINARYE_BADFORMAT,
	
	
	EE_BINARYE_BADPARAM,
	
	
	EE_BINARYE_BADSOURCE,
	
	
	EE_BUTTON_BADMODIFIER,
	
	
	EE_BUTTON_FAMILYNAN,
	
	
	EE_BUTTON_MENUBUTTONNAN,
	
	
	EE_BUTTON_MENUHISTORYNAN,
	
	
	EE_BUTTON_MENULINESNAN,
	
	
	EE_BUTTON_MNEMONICNAN,
	
	
	EE_CANCEL_IDNAN,
	
	
	EE_CHARTONUM_BADSOURCE,
	
	
	EE_CHOOSE_BADEXP,
	
	
	EE_CHOOSE_BADTOOL,
	
	
	EE_CHUNK_BADBACKGROUNDEXP,
	
	
	EE_CHUNK_BADCARDEXP,
	
	
	EE_CHUNK_BADCHARMARK,
	
	
	EE_CHUNK_BADCONTAINER,
	
	
	EE_CHUNK_BADEXPRESSION,
	
	
	EE_CHUNK_BADITEMMARK,
	
	
	EE_CHUNK_BADLINEMARK,
	
	
	EE_CHUNK_BADOBJECTEXP,
	
	
	EE_CHUNK_BADRANGEEND,
	
	
	EE_CHUNK_BADRANGESTART,
	
	
	EE_CHUNK_BADSTACKEXP,
	
	
	EE_CHUNK_BADTEXT,
	
	
	EE_CHUNK_BADTOKENMARK,
	
	
	EE_CHUNK_BADWORDMARK,
	
	
	EE_CHUNK_CANTDELETEOBJECT,
	
	
	EE_CHUNK_CANTFINDOBJECT,
	
	
	EE_CHUNK_CANTGETATTS,
	
	
	EE_CHUNK_CANTGETDEST,
	
	
	EE_CHUNK_CANTGETNUMBER,
	
	
	EE_CHUNK_CANTGETSOURCE,
	
	
	EE_CHUNK_CANTGETSUBSTRING,
	
	
	EE_CHUNK_CANTMARK,
	
	
	EE_CHUNK_CANTSETATTS,
	
	
	EE_CHUNK_CANTSETDEST,
	
	
	EE_CHUNK_CANTSETN,
	
	
	EE_CHUNK_NOBACKGROUND,
	
	
	EE_CHUNK_NOCARD,
	
	
	EE_CHUNK_NOOBJECT,
	
	
	EE_CHUNK_NOPROP,
	
	
	EE_CHUNK_NOSTACK,
	
	
	EE_CHUNK_NOTARGET,
	
	
	EE_CHUNK_NOTOPEN,
	
	
	EE_CHUNK_OBJECTNOTCONTAINER,
	
	
	EE_CHUNK_SETCANTGETBOJECT,
	
	
	EE_CHUNK_SETCANTGETDEST,
	
	
	EE_CHUNK_SETNOTACONTAINER,
	
	
	EE_CLICK_ABORT,
	
	
	EE_CLICK_BADBUTTON,
	
	
	EE_CLICK_BADLOCATION,
	
	
	EE_CLICK_NAP,
	
	
	EE_CLICK_STACKNOTOPEN,
	
	
	EE_CLONE_BADNAME,
	
	
	EE_CLONE_CANTCLONE,
	
	
	EE_CLONE_LOCKED,
	
	
	EE_CLONE_NOTARGET,
	
	
	EE_CLOSE_BADNAME,
	
	
	EE_CLOSE_NOOBJ,
	
	
	EE_COLOR_BADSELECTEDCOLOR,
	
	
	EE_COMPACT_NOTARGET,
	
	
	EE_COMPACT_NOTASTACK,
	
	
	EE_COMPOUND_BADPERIODS,
	
	
	EE_COMPOUND_BADRATE,
	
	
	EE_COMPRESS_BADSOURCE,
	
	
	EE_COMPRESS_ERROR,
	
	
	EE_CONCATSPACE_BADLEFT,
	
	
	EE_CONCATSPACE_BADRIGHT,
	
	
	EE_CONCAT_BADLEFT,
	
	
	EE_CONCAT_BADRIGHT,
	
	
	EE_CONTAINS_BADLEFT,
	
	
	EE_CONTAINS_BADRIGHT,
	
	
	EE_CONVERT_CANTGET,
	
	
	EE_CONVERT_CANTSET,
	
	
	EE_CLIPBOARD_BADDEST,
	
	
	EE_CLIPBOARD_NODEST,
	
	
	EE_CLIPBOARD_BADOBJ,
	
	
	EE_COPY_NOOBJ,
	
	
	EE_COPY_PASSWORD,
	
	
	EE_COS_BADSOURCE,
	
	
	EE_COS_DOMAIN,
	
	
	EE_CREATE_BADBGORCARD,
	
	
	EE_CREATE_BADEXP,
	
	
	EE_CREATE_BADFILEEXP,
	
	
	EE_CREATE_LOCKED,
	
	
	EE_CROP_NOIMAGE,
	
	
	EE_CROP_NOTIMAGE,
	
	
	EE_CROP_CANTGETRECT,
	
	
	EE_CROP_NAR,
	
	
	EE_CUT_NOOBJ,
	
	
	EE_DECOMPRESS_BADSOURCE,
	
	
	EE_DECOMPRESS_NOTCOMPRESSED,
	
	
	EE_DECOMPRESS_ERROR,
	
	
	EE_DELETE_BADFILEEXP,
	
	
	EE_DELETE_NOOBJ,
	
	
	EE_DISABLE_NOOBJ,
	
	
	EE_DISPATCH_NOFILEYET,
	
	
	EE_DIVIDE_BADARRAY,
	
	
	EE_DIVIDE_BADDEST,
	
	
	EE_DIVIDE_BADSOURCE,
	
	
	EE_DIVIDE_CANTSET,
	
	
	EE_DIVIDE_MISMATCH,
	
	
	EE_DIVIDE_RANGE,
	
	
	EE_DIVIDE_ZERO,
	
	
	EE_DIV_BADARRAY,
	
	
	EE_DIV_BADLEFT,
	
	
	EE_DIV_BADRIGHT,
	
	
	EE_DIV_MISMATCH,
	
	
	EE_DIV_RANGE,
	
	
	EE_DIV_ZERO,
	
	
	EE_DO_ABORT,
	
	
	EE_DO_BADCOMMAND,
	
	
	EE_DO_BADEXEC,
	
	
	EE_DO_BADEXP,
	
	
	EE_DO_BADLANG,
	
	
	EE_DO_BADLINE,
	
	
	EE_DO_NOCOMMAND,
	
	
	EE_DO_NOTCOMMAND,
	
	
	EE_DO_NOTLICENSED,
	
	
	EE_DOMENU_BADEXP,
	
	
	EE_DOMENU_DONTKNOW,
	
	
	EE_DRAG_ABORT,
	
	
	EE_DRAG_BADBUTTON,
	
	
	EE_DRAG_BADENDLOC,
	
	
	EE_DRAG_ENDNAP,
	
	
	EE_DRAG_BADSTARTLOC,
	
	
	EE_DRAG_STARTNAP,
	
	
	EE_DRIVERNAMES_BADTYPE,
	
	
	EE_DRIVES_BADTYPE,
	
	
	EE_EDIT_BADTARGET,
	
	
	EE_ENCRYPT_BADSOURCE,
	
	
	EE_EQUAL_OPS,
	
	
	EE_EXP10_BADSOURCE,
	
	
	EE_EXP10_DOMAIN,
	
	
	EE_EXP1_BADSOURCE,
	
	
	EE_EXP1_DOMAIN,
	
	
	EE_EXP2_BADSOURCE,
	
	
	EE_EXP2_DOMAIN,
	
	
	EE_EXPORT_BADNAME,
	
	
	EE_EXPORT_CANTOPEN,
	
	
	EE_EXPORT_CANTWRITE,
	
	
	EE_EXPORT_NOSELECTED,
	
	
	EE_EXPORT_NOTANIMAGE,
	
	
	EE_EXPRESSION_NFACTOR,
	
	
	EE_EXPRESSION_SFACTOR,
	
	
	EE_EXP_BADSOURCE,
	
	
	EE_EXP_DOMAIN,
	
	
	EE_EXTENTS_BADSOURCE,
	
	
	EE_FACTOR_BADLEFT,
	
	
	EE_FACTOR_BADRIGHT,
	
	
	EE_FIELD_BADTEXTATTS,
	
	
	EE_FIELD_HILITEDNAN,
	
	
	EE_FIELD_SCROLLBARWIDTHNAN,
	
	
	EE_FIELD_SHIFTNAN,
	
	
	EE_FIELD_TABSNAN,
	
	
	EE_FILES_NOPERM,
	
	
	EE_FILTER_CANTGET,
	
	
	EE_FILTER_CANTGETPATTERN,
	
	
	EE_FILTER_CANTSET,
	
	
	EE_FIND_BADSTRING,
	
	
	EE_FLIP_NOIMAGE,
	
	
	EE_FLIP_NOTIMAGE,
	
	
	EE_FLUSHEVENTS_BADTYPE,
	
	
	EE_FOCUS_BADOBJECT,
	
	
	EE_FONTNAMES_BADTYPE,
	
	
	EE_FONTSIZES_BADFONTNAME,
	
	
	EE_FONTSTYLES_BADFONTNAME,
	
	
	EE_FONTSTYLES_BADFONTSIZE,
	
	
	EE_FORMAT_BADSOURCE,
	
	
	EE_FUNCTION_BADFUNCTION,
	
	
	EE_FUNCTION_BADSOURCE,
	
	
	EE_FUNCTION_CANTEVALN,
	
	
	EE_FUNCTION_NAN,
	
	
	EE_GET_BADEXP,
	
	
	EE_GET_CANTSET,
	
	
	EE_GLOBALLOC_NAP,
	
	
	EE_GO_BADBACKGROUNDEXP,
	
	
	EE_GO_BADCARDEXP,
	
	
	EE_GO_BADSTACKEXP,
	
	
	EE_GO_BADWINDOWEXP,
	
	
	EE_GO_CANTATTACH,
	
	
	EE_GO_NODEST,
	
	
	EE_GRAB_NOOBJ,
	
	
	EE_GRAPHIC_NAN,
	
	
	EE_GREATERTHANEQUAL_OPS,
	
	
	EE_GREATERTHAN_OPS,
	
	
	EE_GROUP_BACKSIZENAP,
	
	
	EE_GROUP_HILITEDNAN,
	
	
	EE_GROUP_NOOBJ,
	
	
	EE_GROUPING_BADRIGHT,
	
	
	EE_HANDLER_ABORT,
	
	
	EE_HANDLER_BADSTATEMENT,
	
	
	EE_HANDLER_BADPARAM,
	
	
	EE_HANDLER_BADPARAMINDEX,
	
	
	EE_HASMEMORY_BADAMOUNT,
	
	
	EE_HIDE_BADEFFECT,
	
	
	EE_HIDE_NOOBJ,
	
	
	EE_HOSTADDRESS_BADSOCKET,
	
	
	EE_HOSTATON_BADADDRESS,
	
	
	EE_HOSTNAME_BADNAME,
	
	
	EE_HOSTNTOA_BADNAME,
	
	
	EE_IF_ABORT,
	
	
	EE_IF_BADCOND,
	
	
	EE_IF_BADSTATEMENT,
	
	
	EE_IMAGE_BADPIXMAP,
	
	
	EE_IMAGE_HOTNAP,
	
	
	EE_OBJECT_IDNAN,
	
	
	EE_OBJECT_IDINUSE,
	
	
	EE_IMAGE_NOTOPEN,
	
	
	EE_IMAGE_XHOTNAN,
	
	
	EE_IMAGE_YHOTNAN,
	
	
	EE_IMPORT_BADNAME,
	
	
	EE_IMPORT_CANTOPEN,
	
	
	EE_IMPORT_CANTREAD,
	
	
	EE_IMPORT_LOCKED,
	
	
	EE_INSERT_BADTARGET,
	
	
	EE_INSERT_NOTLICENSED,
	
	
	EE_INTERSECT_NOOBJECT,
	
	
	EE_IS_BADLEFT,
	
	
	EE_IS_BADRIGHT,
	
	
	EE_IS_BADOPS,
	
	
	EE_IS_WITHINNAP,
	
	
	EE_IS_WITHINNAR,
	
	
	EE_ISNUMBER_BADSOURCE,
	
	
	EE_ISOTOMAC_BADSOURCE,
	
	
	EE_ITEM_BADLEFT,
	
	
	EE_ITEM_BADRIGHT,
	
	
	EE_KEYS_BADSOURCE,
	
	
	EE_KILL_BADNAME,
	
	
	EE_KILL_BADNUMBER,
	
	
	EE_LAUNCH_BADAPPEXP,
	
	
	EE_LENGTH_BADSOURCE,
	
	
	EE_LESSTHANEQUAL_OPS,
	
	
	EE_LESSTHAN_OPS,
	
	
	EE_LN1_BADSOURCE,
	
	
	EE_LN1_DOMAIN,
	
	
	EE_LN_BADSOURCE,
	
	
	EE_LN_DOMAIN,
	
	
	EE_LOAD_BADURLEXP,
	
	
	EE_LOAD_BADMESSAGEEXP,
	
	
	EE_LOCALLOC_NAP,
	
	
	EE_LOG10_BADSOURCE,
	
	
	EE_LOG10_DOMAIN,
	
	
	EE_LOG2_BADSOURCE,
	
	
	EE_LOG2_DOMAIN,
	
	
	EE_LONGFILEPATH_BADSOURCE,
	
	
	EE_MACTOISO_BADSOURCE,
	
	
	EE_MARK_BADCARD,
	
	
	EE_MARK_BADSTRING,
	
	
	EE_MATCH_BADDEST,
	
	
	EE_MATCH_BADPARAM,
	
	
	EE_MATCH_BADPATTERN,
	
	
	EE_MATCH_BADSOURCE,
	
	
	EE_MATRIX_RANGE,
	
	
	EE_MATRIXMULT_BADSOURCE,
	
	
	EE_MATRIXMULT_MISMATCH,
	
	
	EE_MAX_BADSOURCE,
	
	
	EE_MCISENDSTRING_BADSOURCE,
	
	
	EE_MD5DIGEST_BADSOURCE,
	
	
	EE_MEDIAN_BADSOURCE,
	
	
	EE_MERGE_BADSOURCE,
	
	
	EE_MINUS_BADARRAY,
	
	
	EE_MINUS_BADLEFT,
	
	
	EE_MINUS_BADRIGHT,
	
	
	EE_MINUS_MISMATCH,
	
	
	EE_MINUS_RANGE,
	
	
	EE_MIN_BADSOURCE,
	
	
	EE_MOD_BADARRAY,
	
	
	EE_MOD_BADLEFT,
	
	
	EE_MOD_BADRIGHT,
	
	
	EE_MOD_MISMATCH,
	
	
	EE_MOD_RANGE,
	
	
	EE_MOD_ZERO,
	
	
	EE_MOUSE_BADSOURCE,
	
	
	EE_MOVE_ABORT,
	
	
	EE_MOVE_BADOBJECT,
	
	
	EE_MOVE_BADENDLOC,
	
	
	EE_MOVE_BADDURATION,
	
	
	EE_MOVE_DURATIONNAN,
	
	
	EE_MOVE_ENDNAP,
	
	
	EE_MOVE_BADSTARTLOC,
	
	
	EE_MOVE_STARTNAP,
	
	
	EE_MULTIPLY_BADARRAY,
	
	
	EE_MULTIPLY_BADDEST,
	
	
	EE_MULTIPLY_BADSOURCE,
	
	
	EE_MULTIPLY_CANTSET,
	
	
	EE_MULTIPLY_MISMATCH,
	
	
	EE_MULTIPLY_RANGE,
	
	
	EE_NOTEQUAL_OPS,
	
	
	EE_NOT_BADRIGHT,
	
	
	EE_NOTBITS_BADRIGHT,
	
	
	EE_NUMTOCHAR_BADSOURCE,
	
	
	EE_OBJECT_BADALIGN,
	
	
	EE_OBJECT_BADCOLOR,
	
	
	EE_OBJECT_BADCOLORS,
	
	
	EE_OBJECT_BADRELAYER,
	
	
	EE_OBJECT_BADSTYLE,
	
	
	EE_OBJECT_CANTREMOVE,
	
	
	EE_OBJECT_GETNOPROP,
	
	
	EE_OBJECT_LAYERNAN,
	
	
	EE_OBJECT_LOCNAP,
	
	
	EE_OBJECT_MARGINNAN,
	
	
	EE_OBJECT_NAB,
	
	
	EE_OBJECT_NAME,
	
	
	EE_OBJECT_NAN,
	
	
	EE_OBJECT_NAP,
	
	
	EE_OBJECT_NAR,
	
	
	EE_OBJECT_NOHOME,
	
	
	EE_OBJECT_PIXELNAN,
	
	
	EE_OBJECT_PIXMAPNAN,
	
	
	EE_OBJECT_SCRIPTEXECUTING,
	
	
	EE_OBJECT_SETNOPROP,
	
	
	EE_OBJECT_TEXTHEIGHTNAN,
	
	
	EE_OBJECT_TEXTSIZENAN,
	
	
	EE_OFFSET_BADOFFSET,
	
	
	EE_OFFSET_BADPART,
	
	
	EE_OFFSET_BADWHOLE,
	
	
	EE_OPEN_BADMESSAGE,
	
	
	EE_OPEN_BADNAME,
	
	
	EE_OPEN_NOPERM,
	
	
	EE_OR_BADLEFT,
	
	
	EE_OR_BADRIGHT,
	
	
	EE_ORBITS_BADLEFT,
	
	
	EE_ORBITS_BADRIGHT,
	
	
	EE_OVER_BADARRAY,
	
	
	EE_OVER_BADLEFT,
	
	
	EE_OVER_BADRIGHT,
	
	
	EE_OVER_MISMATCH,
	
	
	EE_OVER_RANGE,
	
	
	EE_OVER_ZERO,
	
	
	EE_PARAM_BADEXP,
	
	
	EE_PARAM_BADINDEX,
	
	
	EE_PARAM_BADSOURCE,
	
	
	EE_PARAM_NAN,
	
	
	EE_PASTE_LOCKED,
	
	
	EE_PEERADDRESS_BADSOCKET,
	
	
	EE_PLACE_ALREADY,
	
	
	EE_PLACE_NOBACKGROUND,
	
	
	EE_PLACE_NOCARD,
	
	
	EE_PLACE_NOTABACKGROUND,
	
	
	EE_PLACE_NOTACARD,
	
	
	EE_PLAY_BADCLIP,
	
	
	EE_PLAY_BADLOC,
	
	
	EE_PLAY_BADOPTIONS,
	
	
	EE_PLUS_BADLEFT,
	
	
	EE_PLUS_BADRIGHT,
	
	
	EE_PLUS_RANGE,
	
	
	EE_POP_CANTSET,
	
	
	EE_POST_BADDESTEXP,
	
	
	EE_POST_BADSOURCEEXP,
	
	
	EE_POW_BADLEFT,
	
	
	EE_POW_BADRIGHT,
	
	
	EE_POW_RANGE,
	
	
	EE_PRINT_CANTGETCOORD,
	
	
	EE_PRINT_CANTGETCOUNT,
	
	
	EE_PRINT_CANTGETRECT,
	
	
	EE_PRINT_ERROR,
	
	
	EE_PRINT_IOERROR,
	
	
	EE_PRINT_NAP,
	
	
	EE_PRINT_NAR,
	
	
	EE_PRINT_NOTACARD,
	
	
	EE_PRINT_NOTOPEN,
	
	
	EE_PRINT_NOTARGET,
	
	
	EE_PROPERTY_BADARCANGLE,
	
	
	EE_PROPERTY_BADBLINKRATE,
	
	
	EE_PROPERTY_BADCOLOR,
	
	
	EE_PROPERTY_BADCOLORS,
	
	
	EE_PROPERTY_BADCOUNTN,
	
	
	EE_PROPERTY_BADCOUNTS,
	
	
	EE_PROPERTY_BADDRAGSPEED,
	
	
	EE_PROPERTY_BADEFFECTRATE,
	
	
	EE_PROPERTY_BADEXTENDKEY,
	
	
	EE_PROPERTY_BADEXPRESSION,
	
	
	EE_PROPERTY_BADGRIDSIZE,
	
	
	EE_PROPERTY_BADIDLERATE,
	
	
	EE_PROPERTY_BADLINESIZE,
	
	
	EE_PROPERTY_BADMOVESPEED,
	
	
	EE_PROPERTY_BADMULTISPACE,
	
	
	EE_PROPERTY_BADPOLYSIDES,
	
	
	EE_PROPERTY_BADREPEATDELAY,
	
	
	EE_PROPERTY_BADREPEATRATE,
	
	
	EE_PROPERTY_BADDOUBLEDELTA,
	
	
	EE_PROPERTY_BADDOUBLETIME,
	
	
	EE_PROPERTY_BADROUNDRADIUS,
	
	
	EE_PROPERTY_BADSLICES,
	
	
	EE_PROPERTY_BADSTARTANGLE,
	
	
	EE_PROPERTY_BADTRACEDELAY,
	
	
	EE_PROPERTY_BADTRACESTACK,
	
	
	EE_PROPERTY_BADPRINTPROP,
	
	
	EE_PROPERTY_BADSYNCRATE,
	
	
	EE_PROPERTY_BADTOOLDELAY,
	
	
	EE_PROPERTY_BADTYPERATE,
	
	
	EE_PROPERTY_BADUSERLEVEL,
	
	
	EE_PROPERTY_BEEPNAN,
	
	
	EE_PROPERTY_BRUSHNAN,
	
	
	EE_PROPERTY_BRUSHNOIMAGE,
	
	
	EE_PROPERTY_BRUSHPATNAN,
	
	
	EE_PROPERTY_BRUSHPATNOIMAGE,
	
	
	EE_PROPERTY_CANTSET,
	
	
	EE_PROPERTY_CANTSETOBJECT,
	
	
	EE_PROPERTY_CURSORNAN,
	
	
	EE_PROPERTY_CURSORNOIMAGE,
	
	
	EE_PROPERTY_NAB,
	
	
	EE_PROPERTY_NAN,
	
	
	EE_PROPERTY_NODEFAULTSTACK,
	
	
	EE_PROPERTY_NODEFAULTMENUBAR,
	
	
	EE_PROPERTY_NOPROP,
	
	
	EE_PROPERTY_NOTANUM,
	
	
	EE_PROPERTY_PENPATNAN,
	
	
	EE_PROPERTY_PENPATNOIMAGE,
	
	
	EE_PROPERTY_RANDOMSEEDNAN,
	
	
	EE_PROPERTY_SOCKETTIMEOUTNAN,
	
	
	EE_PROPERTY_UMASKNAN,
	
	
	EE_PUSH_NOTACARD,
	
	
	EE_PUSH_NOTARGET,
	
	
	EE_PUT_BADEXP,
	
	
	EE_PUT_CANTSET,
	
	
	EE_PUT_CANTSETINTO,
	
	
	EE_QUERYREGISTRY_BADEXP,
	
	
	EE_RANDOM_BADSOURCE,
	
	
	EE_READ_ABORT,
	
	
	EE_READ_BADAT,
	
	
	EE_READ_BADCOND,
	
	
	EE_READ_BADEXP,
	
	
	EE_READ_ERROR,
	
	
	EE_READ_NAN,
	
	
	EE_READ_NOCHARACTER,
	
	
	EE_READ_NOFILE,
	
	
	EE_READ_NONUMBER,
	
	
	EE_READ_NOPROCESS,
	
	
	EE_RECORD_BADFILE,
	
	
	EE_RECORDCOMPRESSION_BADTYPE,
	
	
	EE_RECORDINPUT_BADTYPE,
	
	
	EE_REMOVE_NOOBJECT,
	
	
	EE_REMOVE_NOTABACKGROUND,
	
	
	EE_REMOVE_NOTACARD,
	
	
	EE_RENAME_BADSOURCE,
	
	
	EE_RENAME_BADDEST,
	
	
	EE_REPEAT_ABORT,
	
	
	EE_REPEAT_BADFORCOND,
	
	
	EE_REPEAT_BADSTATEMENT,
	
	
	EE_REPEAT_BADUNTILCOND,
	
	
	EE_REPEAT_BADWHILECOND,
	
	
	EE_REPEAT_BADWITHEND,
	
	
	EE_REPEAT_BADWITHSTART,
	
	
	EE_REPEAT_BADWITHSTEP,
	
	
	EE_REPEAT_BADWITHVAR,
	
	
	EE_REPLACE_CANTSET,
	
	
	EE_REPLACE_BADPATTERN,
	
	
	EE_REPLACE_BADREPLACEMENT,
	
	
	EE_REPLACE_BADCONTAINER,
	
	
	EE_REPLACETEXT_BADPATTERN,
	
	
	EE_REPLACETEXT_BADSOURCE,
	
	
	EE_REPLY_BADKEYWORDEXP,
	
	
	EE_REPLY_BADMESSAGEEXP,
	
	
	EE_REQUEST_BADKEYWORDEXP,
	
	
	EE_REQUEST_BADMESSAGEEXP,
	
	
	EE_REQUEST_BADPROGRAMEXP,
	
	
	EE_REQUEST_NOPERM,
	
	
	EE_RESOURCES_BADPARAM,
	
	
	EE_RESOURCES_NOPERM,
	
	
	EE_RETURN_BADEXP,
	
	
	EE_REVERT_HOME,
	
	
	EE_ROTATE_NOIMAGE,
	
	
	EE_ROTATE_NOTIMAGE,
	
	
	EE_ROTATE_BADANGLE,
	
	
	EE_ROUND_BADSOURCE,
	
	
	EE_SAVE_BADNOFILEEXP,
	
	
	EE_SAVE_NOPERM,
	
	
	EE_SAVE_NOTARGET,
	
	
	EE_SAVE_NOTASTACK,
	
	
	EE_SEEK_BADNAME,
	
	
	EE_SEEK_BADWHERE,
	
	
	EE_SEEK_NOFILE,
	
	
	EE_SELECT_BADTARGET,
	
	
	EE_SELECTED_BADSOURCE,
	
	
	EE_SELECTEDBUTTON_BADFAMILY,
	
	
	EE_SELECTEDBUTTON_BADPARENT,
	
	
	EE_SEND_BADEXEC,
	
	
	EE_SEND_BADEXP,
	
	
	EE_SEND_BADINEXP,
	
	
	EE_SEND_BADPROGRAMEXP,
	
	
	EE_SEND_BADTARGET,
	
	
	EE_SEND_NOPERM,
	
	
	EE_SET_BADEXP,
	
	
	EE_SET_BADSET,
	
	
	EE_SETREGISTRY_NOPERM,
	
	
	EE_SETREGISTRY_BADEXP,
	
	
	EE_SHELL_ABORT,
	
	
	EE_SHELL_BADCOMMAND,
	
	
	EE_SHELL_BADSOURCE,
	
	
	EE_SHELL_NOPERM,
	
	
	EE_SHORTFILEPATH_BADSOURCE,
	
	
	EE_SHOW_BADEFFECT,
	
	
	EE_SHOW_BADLOCATION,
	
	
	EE_SHOW_BADNUMBER,
	
	
	EE_SHOW_NOLOCATION,
	
	
	EE_SHOW_NOOBJ,
	
	
	EE_SIN_BADSOURCE,
	
	
	EE_SIN_DOMAIN,
	
	
	EE_SORT_BADTARGET,
	
	
	EE_SORT_CANTSORT,
	
	
	EE_SORT_NOTARGET,
	
	
	EE_SPECIALFOLDERPATH_BADPARAM,
	
	
	EE_SQRT_BADSOURCE,
	
	
	EE_SQRT_DOMAIN,
	
	
	EE_STACK_BADDECORATION,
	
	
	EE_STACK_BADID,
	
	
	EE_STACK_BADKEY,
	
	
	EE_STACK_BADSUBSTACK,
	
	
	EE_STACK_CANTSETMAINSTACK,
	
	
	EE_STACK_ICONNAN,
	
	
	EE_STACK_LEVELNAN,
	
	
	EE_STACK_MINMAXNAN,
	
	
	EE_STACK_NOKEY,
	
	
	EE_STACK_NOMAINSTACK,
	
	
	EE_STACK_NOTMAINSTACK,
	
	
	EE_STACK_EXTERNALERROR,
	
	
	EE_START_BADTARGET,
	
	
	EE_START_LOCKED,
	
	
	EE_START_NOTABACKGROUND,
	
	
	EE_START_NOTLICENSED,
	
	
	EE_STATEMENT_BADPARAM,
	
	
	EE_STATEMENT_BADCOMMAND,
	
	
	EE_STDDEV_BADSOURCE,
	
	
	EE_STOP_BADTARGET,
	
	
	EE_STOP_NOTABACKGROUND,
	
	
	EE_SUBTRACT_BADARRAY,
	
	
	EE_SUBTRACT_BADDEST,
	
	
	EE_SUBTRACT_BADSOURCE,
	
	
	EE_SUBTRACT_CANTSET,
	
	
	EE_SUBTRACT_MISMATCH,
	
	
	EE_SUBWINDOW_BADEXP,
	
	
	EE_SUBWINDOW_NOSTACK,
	
	
	EE_SUM_BADSOURCE,
	
	
	EE_SWITCH_BADCOND,
	
	
	EE_SWITCH_BADCASE,
	
	
	EE_SWITCH_BADSTATEMENT,
	
	
	EE_TAN_BADSOURCE,
	
	
	EE_TAN_DOMAIN,
	
	
	EE_TEXT_HEIGHT_SUM_NOOBJECT,
	
	
	EE_THERE_BADSOURCE,
	
	
	EE_THROW_BADERROR,
	
	
	EE_TIMES_BADARRAY,
	
	
	EE_TIMES_BADLEFT,
	
	
	EE_TIMES_BADRIGHT,
	
	
	EE_TIMES_RANGE,
	
	
	EE_TOLOWER_BADSOURCE,
	
	
	EE_TOPSTACK_BADSOURCE,
	
	
	EE_TOUPPER_BADSOURCE,
	
	
	EE_TRANSPOSE_BADSOURCE,
	
	
	EE_TRANSPOSE_MISMATCH,
	
	
	EE_TRUNC_BADSOURCE,
	
	
	EE_TRY_BADSTATEMENT,
	
	
	EE_TYPE_ABORT,
	
	
	EE_TYPE_BADSTRINGEXP,
	
	
	EE_UNGROUP_NOGROUP,
	
	
	EE_UNGROUP_NOTAGROUP,
	
	
	EE_UNIDECODE_BADLANGUAGE,
	
	
	EE_UNIDECODE_BADSOURCE,
	
	
	EE_UNIENCODE_BADLANGUAGE,
	
	
	EE_UNIENCODE_BADSOURCE,
	
	
	EE_UNLOAD_BADURLEXP,
	
	
	EE_UNLOCK_BADEFFECT,
	
	
	EE_URLDECODE_BADSOURCE,
	
	
	EE_URLENCODE_BADSOURCE,
	
	
	EE_URLSTATUS_BADSOURCE,
	
	
	EE_VALUE_BADSOURCE,
	
	
	EE_VALUE_ERROR,
	
	
	EE_VALUE_NOOBJ,
	
	
	EE_VARIABLE_BADINDEX,
	
	
	EE_VARIABLE_NAC,
	
	
	EE_VARIABLE_NAN,
	
	
	EE_VISUAL_BADEXP,
	
	
	EE_WAIT_ABORT,
	
	
	EE_WAIT_BADEXP,
	
	
	EE_WAIT_NAN,
	
	
	EE_WITHIN_NOCONTROL,
	
	
	EE_WITHIN_NAP,
	
	
	EE_WRITE_BADEXP,
	
	
	EE_XORBITS_BADLEFT,
	
	
	EE_XORBITS_BADRIGHT,
	
	
	EE_PROPERTY_STACKFILEBADVERSION,
	
	
	EE_PROPERTY_STACKFILEUNSUPPORTEDVERSION,
	
	
	EE_EXTERNAL_EXCEPTION,
	
	
	EE_EXPORT_EMPTYRECT,
	
	
	EE_IMPORT_EMPTYRECT,
	
	
	EE_DRAGDROP_BADACTION,
	
	
	EE_DRAGDROP_BADIMAGEOFFSET,
	
	
	EE_PROPERTY_BADDRAGDELTA,
	
	
	EE_CLIPBOARD_BADMIX,
	
	
	EE_CLIPBOARD_BADTEXT,
	
	
	EE_WRAP_BADARRAY,
	
	
	EE_WRAP_BADLEFT,
	
	
	EE_WRAP_BADRIGHT,
	
	
	EE_WRAP_MISMATCH,
	
	
	EE_WRAP_RANGE,
	
	
	EE_WRAP_ZERO,
	
	
	EE_BEGINSENDS_BADRIGHT,
	
	
	EE_BEGINSENDS_BADLEFT,
	
	
	EE_LOAD_BADEXTENSIONEXP,
	
	
	EE_UNUSED_0651,
	
	
	EE_UNUSED_0652,
	
	
	EE_UNUSED_0653,
	
	
	EE_UNUSED_0654,
	
	
	EE_UNUSED_0655,
	
	
	EE_UNUSED_0656,
	
	
	EE_UNUSED_0657,
	
	
	EE_GRAPHIC_NAB,
	
	
	EE_GRAPHIC_BADFILLRULE,
	
	
	EE_GRAPHIC_BADGRADIENTTYPE,
	
	
	EE_GRAPHIC_BADGRADIENTRAMP,
	
	
	EE_GRAPHIC_BADGRADIENTPOINT,
	
	
	EE_GRAPHIC_BADGRADIENTQUALITY,
	
	
	EE_GRAPHIC_BADGRADIENTKEY,
	
	
	EE_GRAPHIC_BADCAPSTYLE,
	
	
	EE_GRAPHIC_BADJOINSTYLE,
	
	
	EE_BYTETONUM_BADSOURCE,
	
	
	EE_NUMTOBYTE_BADSOURCE,
	
	
	EE_ARRAYENCODE_BADSOURCE,
	
	
	EE_ARRAYENCODE_FAILED,
	
	
	EE_ARRAYDECODE_BADSOURCE,
	
	
	EE_ARRAYDECODE_FAILED,
	
	
	EE_PARENTSCRIPT_BADOBJECT,
	
	
	EE_DISPATCH_BADMESSAGEEXP,
	
	
	EE_DISPATCH_BADCOMMAND,
	
	
	EE_DISPATCH_BADTARGET,
	
	
	EE_QUERYREGISTRY_BADDEST,
	
	
	EE_PARENTSCRIPT_CYCLICOBJECT,
	
	
	EE_DISK_NOPERM,
	
	
	EE_NETWORK_NOPERM,
	
	
	EE_PROCESS_NOPERM,
	
	
	EE_REGISTRY_NOPERM,
	
	
	NOTUSED__EE_STACK_NOPERM,
	
	
	EE_PRINT_NOPERM,
	
	
	EE_PRIVACY_NOPERM,
	
	
	EE_BITMAPEFFECT_BADKEY,
	
	
	EE_BITMAPEFFECT_BADKEYFORTYPE,
	
	
	EE_BITMAPEFFECT_BADNUMBER,
	
	
	EE_BITMAPEFFECT_BADCOLOR,
	
	
	EE_BITMAPEFFECT_BADBLENDMODE,
	
	
	EE_BITMAPEFFECT_BADFILTER,
	
	
	EE_BITMAPEFFECT_BADTECHNIQUE,
	
	
	EE_BITMAPEFFECT_BADSOURCE,
	
	
	EE_BITMAPEFFECT_BADBOOLEAN,
	
	
	EE_SECUREMODE_BADCATEGORY,
	
	
	EE_SECUREMODE_APPLESCRIPT_NOPERM,
	
	
	EE_SECUREMODE_DOALTERNATE_NOPERM,
	
	
	EE_PROCESSTYPE_BADVALUE,
	
	
	EE_PROCESSTYPE_NOTPOSSIBLE,
	
	
	NOTUSED__EE_VARIABLE_CONTENTS_LOCKED,
	
	
	NOTUSED__EE_VARIABLE_ELEMENT_LOCKED,
	
	
	EE_ENVDO_NOTSUPPORTED,
	
	
	EE_ENVDO_FAILED,
	
	
	EE_GRAPHIC_BADEDITMODE,
	
	
	EE_EXPORT_BADPALETTE,
	
	
	EE_EXPORT_BADPALETTESIZE,
	
	
	EE_OPEN_BADOPTIONS,
	
	
	NOTUSED__EE_FEATURE_NOTSUPPORTED,
	
	
	EE_PRINT_UNKNOWNDST,
	
	
	EE_OBJECT_NOTWHILEEDITING,
	
	
	EE_PRINTANCHOR_BADNAME,
	
	
	EE_PRINTANCHOR_BADLOCATION,
	
	
	EE_PRINTANCHOR_LOCATIONNAP,
	
	
	EE_PRINTLINK_BADDEST,
	
	
	EE_PRINTLINK_BADAREA,
	
	
	EE_PRINTLINK_AREANAR,
	
	
	EE_RANDOMBYTES_BADCOUNT,
	
	
	EE_SHA1DIGEST_BADSOURCE,
	
	
	EE_STACKLIMIT_NAN,
	
	
	EE_SECURITY_NOLIBRARY,
	
	
	EE_GROUP_CANNOTBEBGORSHARED,
	
	
	EE_GROUP_CANNOTBENONSHARED,
	
	
	EE_GROUP_NOBG,
	
	
	EE_PRINTBOOKMARK_BADTITLE,
	
	
	EE_PRINTBOOKMARK_BADLEVEL,
	
	
	EE_PRINTBOOKMARK_BADAT,
	
	
	EE_PRINTBOOKMARK_BADINITIAL,
	
	
	EE_SCRIPT_FILEINDEX,
	
	
	EE_INCLUDE_FILENOTFOUND,
	
	
	EE_SCRIPT_SYNTAXERROR,
	
	
	EE_SCRIPT_BADSTATEMENT,
	
	
	EE_INCLUDE_BADCONTEXT,
	
	
	EE_INCLUDE_BADFILENAME,
	
	
	EE_INCLUDE_TOOMANY,
	
	
	EE_REQUIRE_BADCONTEXT,
	
	
	NOTUSED__EE_REQUIRE_BADFILENAME,
	
	
	NOTUSED__EE_REQUIRE_BADSTACK,
	
	
	EE_ERRORMODE_BADVALUE,
	
	
	EE_OUTPUTENCODING_BADVALUE,
	
	
	EE_OUTPUTLINEENDING_BADVALUE,
	
	
	EE_SCRIPT_ERRORPOS,
	
	
	EE_PROPERTY_BADNETWORKINTERFACE,

	
	EE_CONTROL_BADLAYERMODE,
	
	
	EE_COMPOSITOR_INVALIDTILESIZE,
	
	
	EE_COMPOSITOR_UNKNOWNTYPE,
	
	
	EE_COMPOSITOR_NOTSUPPORTED,
	
	
	EE_INTERSECT_BADTHRESHOLD,
	
	
	EE_INTERSECT_ILLEGALTHRESHOLD,
	
	
	EE_SESSION_BADCONTEXT,
	
	
	EE_SESSION_SAVE_PATH_BADVALUE,
	
	
	EE_SESSION_LIFETIME_BADVALUE,
	
	
	EE_SESSION_NAME_BADVALUE,
	
	
	EE_SESSION_ID_BADVALUE,
	
	
	EE_LOCK_BADRECT,
	
	
	EE_LOCK_NAR,

	
	EE_FIELD_BADLISTSTYLE,

	
	EE_FIELD_FIRSTINDENTNAN,
	
	
	EE_FIELD_LEFTINDENTNAN,

	
	EE_FIELD_RIGHTINDENTNAN,

	
	EE_FIELD_SPACEBEFORENAN,

	
	EE_FIELD_SPACEAFTERNAN,

	
	EE_FIELD_BADLISTDEPTH,

	
	EE_FIELD_BORDERWIDTHNAN,

	
	EE_FIELD_LISTINDENTNAN,

	
	EE_FIELD_PADDINGNAN,
	
	
	EE_CHUNK_CANTSETUNICODEDEST,
	
	
	EE_RELAYER_TARGETNOTCONTAINER,
	
	
	EE_RELAYER_NOTARGET,

	
	EE_RELAYER_NOSOURCE,

	
	EE_RELAYER_SOURCENOTCONTROL,

	
	EE_RELAYER_CARDNOTSAME,

	
	EE_RELAYER_LAYERNAN,

	
	EE_RELAYER_BADLAYER,

	
	EE_RELAYER_TARGETNOTCONTROL,

	
	EE_RELAYER_ILLEGALMOVE,
	
	
	EE_RELAYER_OBJECTSVANISHED,
	
	
	EE_CONTROLATLOC_NAP,
	
	
	EE_DO_NOCALLER,
	
	
	EE_READ_NOTVALIDFORDATAGRAM,

	
	EE_FIELD_LISTINDEXNAN,
	
	
	EE_PROPERTY_BADIMAGECACHELIMIT,
    
    
	EE_GROUP_DIFFERENTPARENT,
	
	
	EE_UUID_BADTYPE,
	
	
	EE_UUID_TOOMANYPARAMS,
	
	
	EE_UUID_UNKNOWNTYPE,
	
	
	EE_UUID_BADNAMESPACEID,
	
	
	EE_UUID_NAMESPACENOTAUUID,
	
	
	EE_UUID_BADNAME,
	
	
	EE_UUID_NORANDOMNESS,
	
	
	EE_AVGDEV_BADSOURCE,
	
	
	EE_GEO_MEAN_BADSOURCE,

	
	EE_HAR_MEAN_BADSOURCE,

	
	EE_POP_STDDEV_BADSOURCE,
	
	
	EE_POP_VARIANCE_BADSOURCE,
	
	
	EE_VARIANCE_BADSOURCE,
	
	
	EE_GROUP_NOTGROUPABLE,
};

extern const char *MCexecutionerrors;

#line 2424 "c:\\github\\livecode-runrev\\engine\\src\\executionerrors.h"

#line 2101 "c:\\github\\livecode-runrev\\engine\\src\\parsedef.h"

#line 2103 "c:\\github\\livecode-runrev\\engine\\src\\parsedef.h"

#line 24 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\mcio.h"

























#line 27 "c:\\github\\livecode-runrev\\engine\\src\\mcio.h"





















#line 49 "c:\\github\\livecode-runrev\\engine\\src\\mcio.h"



extern void IO_set_stream(IO_handle stream, char *newptr);
extern Boolean IO_findstream(Streamnode *nodes, uint2 nitems, const char *name, uint2 &i);
extern Boolean IO_findfile(const char *name, uint2 &i);
extern Boolean IO_closefile(const char *name);
extern Boolean IO_findprocess(const char *name, uint2 &i);
extern void IO_cleanprocesses();
extern Boolean IO_findsocket(const char *name, uint2 &i);
extern real8 IO_cleansockets(real8 ctime);
extern void IO_freeobject(MCObject *o);
extern IO_stat IO_read(void *ptr, uint4 size, uint4 &n, IO_handle stream);
extern IO_stat IO_write(const void *ptr, uint4 s, uint4 n, IO_handle stream);
extern IO_stat IO_read_to_eof(IO_handle stream, MCExecPoint &ep);
extern IO_stat IO_fgets(char *ptr, uint4 length, IO_handle stream);

extern IO_stat IO_read_real8(real8 *dest, IO_handle stream);
extern IO_stat IO_read_real4(real4 *dest, IO_handle stream);
extern IO_stat IO_read_uint4(uint4 *dest, IO_handle stream);
extern IO_stat IO_read_int4(int4 *dest, IO_handle stream);
extern IO_stat IO_read_uint2(uint2 *dest, IO_handle stream);
extern IO_stat IO_read_int2(int2 *dest, IO_handle stream);
extern IO_stat IO_read_uint1(uint1 *dest, IO_handle stream);
extern IO_stat IO_read_int1(int1 *dest, IO_handle stream);

extern IO_stat IO_write_real8(real8 dest, IO_handle stream);
extern IO_stat IO_write_real4(real4 dest, IO_handle stream);
extern IO_stat IO_write_uint4(uint4 dest, IO_handle stream);
extern IO_stat IO_write_int4(int4 dest, IO_handle stream);
extern IO_stat IO_write_uint2(uint2 dest, IO_handle stream);
extern IO_stat IO_write_int2(int2 dest, IO_handle stream);
extern IO_stat IO_write_uint1(uint1 dest, IO_handle stream);
extern IO_stat IO_write_int1(int1 dest, IO_handle stream);



extern IO_stat IO_read_uint2or4(uint4 *dest, IO_handle stream);
extern IO_stat IO_write_uint2or4(uint4 dest, IO_handle stream);

extern void IO_iso_to_mac(char *sptr, uint4 len);
extern void IO_mac_to_iso(char *sptr, uint4 len);

extern IO_stat IO_read_string(char *&r_string, uint32_t &r_length, IO_handle p_stream, uint8_t p_size, bool p_includes_null, bool p_translate);
extern IO_stat IO_read_string(char *&string, IO_handle stream, uint1 size = 2);
extern IO_stat IO_read_string(char *&string, uint4 &outlen, IO_handle stream, bool isunicode, uint1 size = 2);
extern IO_stat IO_read_string_no_translate(char *&string, IO_handle stream, uint1 size = 2);
extern IO_stat IO_write_string(const MCString &string, IO_handle stream, uint1 size = 2, bool p_write_null = true);
extern IO_stat IO_write_string(const char *string, IO_handle stream, uint1 size = 2);
extern IO_stat IO_write_string(const char *string, uint4 outlen, IO_handle stream, Boolean isunicode, uint1 size = 2);

extern IO_stat IO_read_mccolor(MCColor& r_color, IO_handle stream);
extern IO_stat IO_write_mccolor(const MCColor& color, IO_handle stream);

extern IO_stat IO_read_nameref(MCNameRef& r_name, IO_handle stream, uint1 size = 2);
extern IO_stat IO_write_nameref(MCNameRef name, IO_handle stream, uint1 size = 2);



extern IO_stat IO_read_bytes(void *ptr, uint4 size, IO_handle stream);



struct MCFakeOpenCallbacks
{
	IO_stat (*read)(void *state, void *buffer, uint32_t size, uint32_t& r_filled);
	int64_t (*tell)(void *state);
	IO_stat (*seek_set)(void *state, int64_t offset);
	IO_stat (*seek_cur)(void *state, int64_t offset);
};
extern IO_handle MCS_fakeopencustom(struct MCFakeOpenCallbacks *callbacks, void *state);

extern IO_handle MCS_fakeopen(const MCString &data);
extern IO_handle MCS_fakeopenwrite(void);
extern IO_stat MCS_fakeclosewrite(IO_handle &stream, char*& r_buffer, uint4& r_length);

extern bool MCS_isfake(IO_handle stream);
extern uint4 MCS_faketell(IO_handle stream);
extern void MCS_fakewriteat(IO_handle stream, uint4 p_pos, const void *p_buffer, uint4 p_size);

extern IO_handle MCS_open(const char *path, const char *mode, Boolean map, Boolean driver, uint4 offset);
extern IO_stat MCS_close(IO_handle &stream);

extern IO_stat MCS_read(void *ptr, uint4 size, uint4 &n, IO_handle stream);
extern IO_stat MCS_write(const void *ptr, uint4 size, uint4 n, IO_handle stream);


extern IO_stat MCS_putback(char p_char, IO_handle stream);

extern IO_stat MCS_trunc(IO_handle stream);
extern IO_stat MCS_flush(IO_handle stream);
extern IO_stat MCS_sync(IO_handle stream);
extern Boolean MCS_eof(IO_handle stream);


extern IO_stat MCS_seek_cur(IO_handle stream, int64_t offset);
extern IO_stat MCS_seek_set(IO_handle stream, int64_t offset);
extern IO_stat MCS_seek_end(IO_handle stream, int64_t offset);
extern int64_t MCS_tell(IO_handle stream);
extern int64_t MCS_fsize(IO_handle stream);






int64_t MCS_fake_fsize(IO_handle stream);
int64_t MCS_fake_tell(IO_handle stream);
IO_stat MCS_fake_seek_cur(IO_handle stream, int64_t offset);
IO_stat MCS_fake_seek_set(IO_handle stream, int64_t offset);
IO_stat MCS_fake_read(void *ptr, uint4 size, uint4 &n, IO_handle stream);



#line 164 "c:\\github\\livecode-runrev\\engine\\src\\mcio.h"
#line 25 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"

#line 1 "c:\\github\\livecode-runrev\\engine\\src\\execpt.h"























#line 1 "c:\\github\\livecode-runrev\\engine\\src\\variable.h"




















#line 1 "c:\\github\\livecode-runrev\\engine\\src\\express.h"
























class MCExpression
{
protected:
	uint2 line;
	uint2 pos;
	Factor_rank rank;
	MCExpression *root;
	MCExpression *left;
	MCExpression *right;

public:
	MCExpression();
	virtual ~MCExpression();

	virtual Parse_stat parse(MCScriptPoint &, Boolean the);

	
	virtual Exec_stat eval(MCExecPoint &ep);

	
	
	virtual Exec_stat evalcontainer(MCExecPoint& ep, MCVariable*& r_var, MCVariableValue*& r_ref);

	
	
	virtual MCVariable *evalvar(MCExecPoint& ep);

	
	
	
	
	
	virtual MCVarref *getrootvarref(void);

	void setrank(Factor_rank newrank)
	{
		rank = newrank;
	}
	void setroot(MCExpression *newroot)
	{
		root = newroot;
	}
	void setleft(MCExpression *newleft)
	{
		left = newleft;
	}
	void setright(MCExpression *newright)
	{
		right = newright;
	}
	Factor_rank getrank()
	{
		return rank;
	}
	MCExpression *getroot()
	{
		return root;
	}
	MCExpression *getleft()
	{
		return left;
	}
	MCExpression *getright()
	{
		return right;
	}
	Parse_stat getexps(MCScriptPoint &sp, MCExpression *earray[], uint2 &ecount);
	void freeexps(MCExpression *earray[], uint2 ecount);
	Parse_stat get0params(MCScriptPoint &);
	Parse_stat get0or1param(MCScriptPoint &sp, MCExpression **exp, Boolean the);
	Parse_stat get1param(MCScriptPoint &, MCExpression **exp, Boolean the);
	Parse_stat get1or2params(MCScriptPoint &, MCExpression **e1,
	                         MCExpression **e2, Boolean the);
	Parse_stat get2params(MCScriptPoint &, MCExpression **e1, MCExpression **e2);
	Parse_stat get2or3params(MCScriptPoint &, MCExpression **exp1,
	                         MCExpression **exp2, MCExpression **exp3);
	Parse_stat get3params(MCScriptPoint &, MCExpression **exp1,
	                      MCExpression **exp2, MCExpression **exp3);
	Parse_stat get4or5params(MCScriptPoint &, MCExpression **exp1,
	                         MCExpression **exp2, MCExpression **exp3,
	                         MCExpression **exp4, MCExpression **exp5);
	Parse_stat get6params(MCScriptPoint &, MCExpression **exp1,
	                      MCExpression **exp2, MCExpression **exp3,
	                      MCExpression **exp4, MCExpression **exp5,
	                      MCExpression **exp6);
	Parse_stat getvariableparams(MCScriptPoint &sp, uint32_t p_min_params, uint32_t p_param_count, ...);
	Parse_stat getparams(MCScriptPoint &spt, MCParameter **params);
	void initpoint(MCScriptPoint &);
	Exec_stat compare(MCExecPoint &, int2 &i, bool p_compare_arrays = false);
	
	static int2 compare_arrays(MCExecPoint &ep1, MCExecPoint &ep2, MCExecPoint *p_context);
	static int2 compare_values(MCExecPoint &ep1, MCExecPoint &ep2, MCExecPoint *p_context, bool p_compare_arrays);
};

class MCFuncref : public MCExpression
{
	MCNameRef name;
		MCHandler *handler;
	MCObject *parent;
	MCParameter *params;
	bool resolved : 1;
public:
	MCFuncref(MCNameRef);
	virtual ~MCFuncref();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &ep);
};
#line 133 "c:\\github\\livecode-runrev\\engine\\src\\express.h"
#line 22 "c:\\github\\livecode-runrev\\engine\\src\\variable.h"
#line 23 "c:\\github\\livecode-runrev\\engine\\src\\variable.h"





















typedef struct
{
	uint4 min;
	uint4 max;
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
	
	void presethash(uint4 p_size);

	
	void resizehash(uint32_t p_new_size = 0);

	
	void freehash(void);

	

	
	uint32_t getnfilled(void) const;

	

	
	
	Exec_stat dofunc(MCExecPoint& ep, Functions func, uint4 &nparams, real8 &n, real8 oldn, void *titems);

	
	
	
	Exec_stat factorarray(MCExecPoint &ep, Operators op);

	
	
	Exec_stat unionarray(MCVariableArray& v);

	
	
	Exec_stat intersectarray(MCVariableArray& v);

	
	
	
	Exec_stat matrixmultiply(MCExecPoint& ep, MCVariableArray& va, MCVariableArray& vb);
	
	
	
	Exec_stat transpose(MCVariableArray& v);

	
	Boolean isnumeric(void);

	
	
	Boolean issequence(void);

	
	
	MCHashentry *lookuphash(const MCString &, Boolean cs, Boolean add);
	
	
	
	MCHashentry *lookupindex(uint32_t p_index, Boolean add);

	
	
	void removehash(const MCString&, Boolean cs);
	
	
	
	void removehash(MCHashentry *p_entry);
	
	
	
	void taketable(MCVariableArray *v);

	
	
	bool copytable(const MCVariableArray &v);

	
	
	void getextents(MCExecPoint& ep);

	
	
	void getkeys(char **keylist, uint4 kcount);

	
	
	void getkeys(MCExecPoint& ep);

	
	
	MCHashentry *getnextelement(uint4 &l, MCHashentry *e, Boolean donumeric, MCExecPoint &ep);

	
	
	MCHashentry *getnextkey(uint4& l, MCHashentry *e) const;

	
	
	
	MCHashentry *getnextkey(MCHashentry *e) const;

	
	
	void combine(MCExecPoint& ep, char e, char k, char*& r_buffer, uint32_t& r_length);

	
	
	void split(const MCString& s, char e, char k);

	
	
	void combine_column(MCExecPoint& ep, char r, char c, char*& r_buffer, uint32_t& r_length);

	
	
	void split_column(const MCString& s, char r, char c);

	
	
	void combine_as_set(MCExecPoint& ep, char e, char*& r_buffer, uint32_t& r_length);

	
	
	void split_as_set(const MCString& s, char e);
	
	
	
	IO_stat loadkeys(IO_handle stream, bool p_merge);

	
	
	IO_stat savekeys(IO_handle stream);

	
	
	Exec_stat setprops(uint4 parid, MCObject *optr);

	
	
	bool isnested(void);

	
	void listelements(MCHashentry** elements);
	
	
	uint4 measure(bool p_only_nested);

	IO_stat save(MCObjectOutputStream& p_stream, bool p_only_nested);
	IO_stat load(MCObjectInputStream& p_stream, bool p_merge);

private:
	
	uint4 computehash(const MCString &);

	
	void extentfromkey(char *skey);

	
	void calcextents(void);

	
	uint2 getextent(uint1 tdimension) const;

	
	Boolean ismissingelement(void) const;
};

inline uint32_t MCVariableArray::getnfilled(void) const
{
	return nfilled;
}






































class MCVariableValue
{
public:
	MCVariableValue(void);
	MCVariableValue(const MCVariableValue& p_other);

	~MCVariableValue(void);

	bool is_clear(void) const;
	bool is_undefined(void) const;
	bool is_empty(void) const;
	bool is_string(void) const;
	bool is_real(void) const;
	bool is_number(void) const;
	bool is_array(void) const;

	Value_format get_format(void) const;

	MCString get_string(void) const;
	real64_t get_real(void) const;
	MCVariableArray* get_array(void);

	void clear(void);

	bool assign(const MCVariableValue& v);
	void exchange(MCVariableValue& v);

	void assign_empty(void);
	void assign_new_array(uint32_t p_hash_size);
	void assign_constant_string(const MCString& s);
	bool assign_string(const MCString& s);
	void assign_real(real64_t r);
	bool assign_both(const MCString& s, real64_t n);
	void assign_constant_both(const MCString& s, real64_t n);
	void assign_buffer(char *p_buffer, uint32_t p_length);

	
	bool append_string(const MCString& s);

	
	
	
	bool assign_custom_both(const char *s, real64_t n);
	MCString get_custom_string(void) const;

	
	bool reserve(uint32_t required_length, void*& r_buffer, uint32_t& r_length);
	bool commit(uint32_t actual_length);

	
	
	
	
	Exec_stat fetch(MCExecPoint& ep, bool p_copy = false);
	
	
	
	Exec_stat store(MCExecPoint& ep);
	
	
	
	Exec_stat append(MCExecPoint& ep);

	bool has_element(MCExecPoint& ep, const MCString& key);

	
	
	bool fetch_element_if_exists(MCExecPoint& ep, const MCString& key, bool p_copy = false);

	Exec_stat fetch_element(MCExecPoint& ep, const MCString& key, bool p_copy = false);
	Exec_stat store_element(MCExecPoint& ep, const MCString& key);
	Exec_stat append_element(MCExecPoint& ep, const MCString& key);
	Exec_stat remove_element(MCExecPoint& ep, const MCString& key);
	Exec_stat lookup_index(MCExecPoint& ep, uindex_t index, bool p_add, MCVariableValue*& r_value);
	Exec_stat lookup_index(MCExecPoint& ep, uint32_t index, MCVariableValue*& r_value);
	Exec_stat lookup_element(MCExecPoint& ep, const MCString& key, MCVariableValue*& r_value);

	
	
	
	Exec_stat lookup_hash(MCExecPoint& ep, const MCString& p_key, bool p_add, MCHashentry*& r_hash);

	
	
	void remove_hash(MCHashentry *p_hash);

	bool get_as_real(MCExecPoint& ep, real64_t& r_value);

	bool ensure_real(MCExecPoint& ep);
	bool ensure_string(MCExecPoint& ep);

	
	bool ensure_cstring(void);

	Exec_stat combine(char e, char k, MCExecPoint& ep);
	Exec_stat split(char e, char k, MCExecPoint& ep);

	Exec_stat combine_column(char col, char row, MCExecPoint& ep);
	Exec_stat split_column(char col, char row, MCExecPoint& ep);

	Exec_stat combine_as_set(char e, MCExecPoint& ep);
	Exec_stat split_as_set(char e, MCExecPoint& ep);

	Exec_stat factorarray(MCExecPoint& ep, Operators op);
	Exec_stat unionarray(MCVariableValue& v);
	Exec_stat intersectarray(MCVariableValue& v);

	Exec_stat transpose(MCVariableValue& v);

	Exec_stat matrixmultiply(MCExecPoint& ep, MCVariableValue& va, MCVariableValue& vb);
	Exec_stat setprops(uint4 parid, MCObject* optr);

	void getextents(MCExecPoint& ep);
	void getkeys(MCExecPoint& ep);
	void getkeys(char **keys, uint32_t kcount);

	IO_stat loadkeys(IO_handle stream, bool p_merge);
	IO_stat savekeys(IO_handle stream);

	IO_stat loadarray(MCObjectInputStream& p_stream, bool p_merge);
	
	
	
	
	
	
	
	bool encode(void*& r_buffer, uint32_t& r_length);
	
	
	
	
	
	
	
	
	bool decode(const MCString& p_value);
	
	
	
	void set_dbg_notify(bool p_state);
	bool get_dbg_notify(void) const;
	void set_dbg_changed(bool p_state);
	bool get_dbg_changed(void) const;
	void set_dbg_mutated(bool p_state);
	bool get_dbg_mutated(void) const;

	void set_external(void);
	bool get_external(void) const;

	void set_temporary(void);
	bool get_temporary(void) const;

private:
	bool copy(const MCVariableValue& p_other);
	void destroy(void);

	bool coerce_to_real(MCExecPoint& ep);
	bool coerce_to_string(MCExecPoint& ep);

	Value_format get_type(void) const;
	void set_type(Value_format new_type);

	enum
	{
		
		
		kTemporaryBit = 1 << 0,

		
		
		kExternalBit = 1 << 1,

		
		kDebugNotifyBit = 1 << 2,
		kDebugChangedBit = 1 << 3,
		kDebugMutatedBit = 1 << 4,
	};

	uint8_t _type;
	uint8_t _flags;
	union
	{
		struct
		{
			struct
			{
				char *data;
				uint32_t size;
			} buffer;
			struct
			{
				const char *string;
				uint32_t length;
			} svalue;
			double nvalue;
		} strnum;
		MCVariableArray array;
	};
};



struct MCHashentry
{
	MCHashentry *next;
	uint32_t hash;
	MCVariableValue value;
	char string[4];

	MCHashentry(void);
	MCHashentry(const MCString& p_key, uint32_t p_hash);
	MCHashentry(const MCHashentry& e);

	MCHashentry *Clone(void) const;

	uint4 Measure(void);

	IO_stat Save(MCObjectOutputStream& p_stream);

	static IO_stat Load(MCObjectInputStream& p_stream, MCHashentry*& r_entry);

	static MCHashentry *Create(uint32_t p_key_length);
	static MCHashentry *Create(const MCString& p_key, uint32_t p_hash);
};

inline MCHashentry::MCHashentry(void)
	: next(0)
{
}

inline MCHashentry::MCHashentry(const MCString& p_key, uint32_t p_hash)
	: next(0),
	  hash(p_hash)
{
	strncpy(string, p_key . getstring(), p_key . getlength());
	string[p_key . getlength()] = '\0';
}

inline MCHashentry::MCHashentry(const MCHashentry& e)
	: next(0),
	  hash(e . hash),
	  value(e . value)
{
	strcpy(string, e . string);
}





#line 521 "c:\\github\\livecode-runrev\\engine\\src\\variable.h"
#line 522 "c:\\github\\livecode-runrev\\engine\\src\\variable.h"

inline MCHashentry *MCHashentry::Clone(void) const
{
	void *t_new_entry;
	t_new_entry = new char[sizeof(MCHashentry) + strlen(string) - 3];
	if (t_new_entry != 0)
		return new(t_new_entry) MCHashentry(*this);
	return 0;
}

inline MCHashentry *MCHashentry::Create(uint32_t p_key_length)
{
	void *t_new_entry;
	t_new_entry = new char[sizeof(MCHashentry) + p_key_length - 3];
	return new(t_new_entry) MCHashentry;
}

inline MCHashentry *MCHashentry::Create(const MCString& p_key, uint32_t p_hash)
{
	void *t_new_entry;
	t_new_entry = new char[sizeof(MCHashentry) + p_key . getlength() - 3];
	return new(t_new_entry) MCHashentry(p_key, p_hash);
}





#line 551 "c:\\github\\livecode-runrev\\engine\\src\\variable.h"
#line 552 "c:\\github\\livecode-runrev\\engine\\src\\variable.h"

class MCVariable
{
protected:
	MCNameRef name;
	MCVariable *next;
	MCVariableValue value;

	bool is_msg : 1;
	bool is_env : 1;
	bool is_global : 1;

	
	
	
	
	
	
	bool is_deferred : 1;

	
	
	bool is_uql : 1;

	
	
	MCVariable(void) {}
	MCVariable(const MCVariable& other) {}

public:
	
	
	~MCVariable(void);

	

	
	
	Boolean isclear(void) const;
	
	
	
	
	Boolean isfree(void) const;
	
	
	Boolean isarray(void) const;

	
	
	Boolean isempty(void) const;

	
	bool isuql(void) const;

	

	MCVariableValue& getvalue(void)
	{
		return value;
	}

	

	
	
	
	
	
	
	void clear(Boolean p_delete_buffer = 0);

	
	
	void clearuql(void);
	void doclearuql(void);

	

	
	
	
	
	
	
	Exec_stat sets(const MCString &s);
	Exec_stat setnameref_unsafe(MCNameRef name);

	
	void copysvalue(const MCString &s);

	
	void setnvalue(real8 n);

	
	
	
	void grab(char *p_buffer, uint4 p_length);

	

	
	
	Exec_stat store(MCExecPoint& ep, Boolean notify)
	{
		Exec_stat stat;
		stat = value . store(ep);
		synchronize(ep, notify);
		return stat;
	}

	
	
	Exec_stat store_element(MCExecPoint& ep, const MCString& k, Boolean notify)
	{
		Exec_stat stat;
		stat = value . store_element(ep, k);
		synchronize(ep, notify);
		return stat;
	}

	
	Exec_stat fetch(MCExecPoint& ep)
	{
		return value . fetch(ep);
	}

	
	
	Exec_stat fetch_element(MCExecPoint& ep, const MCString& k)
	{
		return value . fetch_element(ep, k);
	}

	
	Exec_stat append(MCExecPoint& ep, Boolean notify)
	{
		Exec_stat stat;
		stat = value . append(ep);
		synchronize(ep, notify);
		return stat;
	}

	
	Exec_stat append_element(MCExecPoint& ep, const MCString& k, Boolean notify)
	{
		Exec_stat stat;
		stat = value . append_element(ep, k);
		synchronize(ep, notify);
		return stat;
	}

	
	
	Exec_stat remove(MCExecPoint& ep, Boolean notify);

	
	Exec_stat remove_element(MCExecPoint& ep, const MCString& k, Boolean notify)
	{
		Exec_stat stat;
		stat = value . remove_element(ep, k);
		synchronize(ep, notify);
		return stat;
	}

	
	
	
	void synchronize(MCExecPoint& ep, Boolean notify = 0);

	void setnext(MCVariable *n)
	{
		next = n;
	}

	MCVariable *getnext()
	{
		return next;
	}

	

	MCNameRef getname(void)
	{
		return name;
	}

	bool hasname(MCNameRef other_name)
	{
		return MCNameIsEqualTo(name, other_name, kMCCompareCaseless);
	}

	

	
	void setmsg(void) { is_msg = true; }

	
	void setuql(void) { is_uql = true; }

	
	bool isdeferred(void) { return is_deferred; }
	
	
	bool isplain(void) { return !is_msg && !is_env; }

	
	MCVarref *newvarref(void);

	

	
	
	static MCVariable *lookupglobal(MCNameRef name);
	static MCVariable *lookupglobal_cstring(const char *name);

	
	
	 static bool ensureglobal(MCNameRef name, MCVariable*& r_var);
	 static bool ensureglobal_cstring(const char *name, MCVariable*& r_var);

	 static bool create(MCVariable*& r_var);
	 static bool createwithname(MCNameRef name, MCVariable*& r_var);
	 static bool createwithname_cstring(const char *name, MCVariable*& r_var);

	 static bool createcopy(MCVariable& other, MCVariable*& r_var);
};



inline Boolean MCVariable::isclear(void) const
{
	return value . is_clear();
}

inline Boolean MCVariable::isfree(void) const
{
	return value . is_clear();
}

inline Boolean MCVariable::isarray(void) const
{
	return value . is_array();
}

inline Boolean MCVariable::isempty(void) const
{
	return value . is_empty();
}

inline bool MCVariable::isuql(void) const
{
	return is_uql;
}



inline Exec_stat MCVariable::sets(const MCString& s)
{
	if (s.getlength() == 0)
		clear(1);
	else
		value . assign_constant_string(s);

	return ES_NORMAL;
}

inline Exec_stat MCVariable::setnameref_unsafe(MCNameRef p_name)
{
	return sets(MCNameGetOldString(p_name));
}


inline void MCVariable::copysvalue(const MCString& s)
{
	value . assign_string(s);
}

inline void MCVariable::setnvalue(real64_t n)
{
	value . assign_real(n);
}

inline void MCVariable::grab(char *p_buffer, uint4 p_length)
{
	value . assign_buffer(p_buffer, p_length);
}

inline void MCVariable::clear(Boolean p_delete_buffer)
{
	value . clear();
}

inline void MCVariable::clearuql(void)
{
	if (!is_uql)
		return;

	doclearuql();
}



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

	
	
	Boolean isscriptlocal : 1;
	
	
	
	bool isplain : 1;

public:
	MCVarref(MCVariable *var)
	{
		ref = var;
		exp = 0;
		dimensions = 0;
		index = 0;
		isparam = 0;
		isscriptlocal = 0;
		handler = 0;
		isplain = var -> isplain();
	}

	
	
	MCVarref(MCVariable *var, uint2 i)
	{
		ref = var;
		exp = 0;
		dimensions = 0;
		index = i;
		isparam = 0;
		isscriptlocal = 1;
		handler = 0;
		isplain = true;
	}

	MCVarref(MCHandler *hptr, uint2 i, Boolean param)
	{
		index = i;
		isparam = param;
		handler = hptr;
		ref = 0;
		exp = 0;
		dimensions = 0;
		isscriptlocal = 0;
		isplain = true;
	}
	virtual ~MCVarref();
	
	virtual Exec_stat eval(MCExecPoint &);
	virtual Exec_stat evalcontainer(MCExecPoint& ep, MCVariable*& r_var, MCVariableValue*& r_ref);
	virtual MCVariable *evalvar(MCExecPoint& ep);
	
	
	virtual MCVarref *getrootvarref(void);

	
	
	bool rootmatches(MCVarref *p_other) const;

	Boolean getisscriptlocal() { return isscriptlocal; };

	Exec_stat set(MCExecPoint &, Boolean append = 0);
	Parse_stat parsearray(MCScriptPoint &);
	Exec_stat sets(const MCString &s);
	void clear();
	void clearuql();
	Exec_stat dofree(MCExecPoint &);
	
	bool getisplain(void) const { return isplain; }
	
private:
	MCVariable *fetchvar(MCExecPoint& ep);

	Exec_stat resolve(MCExecPoint& ep, MCVariable*& r_var, MCVariableValue*& r_parent_ref, MCHashentry*& r_ref, bool p_add);
};




typedef Exec_stat (*MCDeferredVariableComputeCallback)(void *context, MCVariable *variable);

class MCDeferredVariable: public MCVariable
{
protected:
	MCDeferredVariableComputeCallback m_callback;
	void *m_context;

public:
	static bool createwithname_cstring(const char *name, MCDeferredVariableComputeCallback callback, void *context, MCVariable*& r_var);

	Exec_stat compute(void);
};




class MCDeferredVarref: public MCVarref
{
public:
	MCDeferredVarref(MCVariable *var)
		: MCVarref(var)
	{
	}

	
	
	
	virtual Exec_stat eval(MCExecPoint&);
	virtual Exec_stat evalcontainer(MCExecPoint& ep, MCVariable*& r_var, MCVariableValue*& r_ref);
	virtual MCVariable *evalvar(MCExecPoint& ep);
};



#line 984 "c:\\github\\livecode-runrev\\engine\\src\\variable.h"

#line 25 "c:\\github\\livecode-runrev\\engine\\src\\execpt.h"
#line 26 "c:\\github\\livecode-runrev\\engine\\src\\execpt.h"




class MCExecPoint
{
	MCObject *curobj;

	
	
	
	
	MCParentScriptUse *parentscript;

	MCHandlerlist *curhlist;
	MCHandler *curhandler;
	MCVariableValue *array;
	char *buffer;
	uint4 size;
	MCString svalue;
	real8 nvalue;
	Value_format format;
	uint2 nffw;
	uint2 nftrailing;
	uint2 nfforce;
	uint2 cutoff;
	uint2 line;
	Boolean convertoctals;
	Boolean casesensitive;
	Boolean wholematches;
	Boolean usesystemdate;
	Boolean useunicode;
	Boolean deletearray;
	char itemdel;
	char columndel;
	char linedel;
	char rowdel;

public:
	MCExecPoint()
	{
		memset(this, 0, sizeof(MCExecPoint));
		itemdel = ',';
		columndel = '\t';
		linedel = '\n';
		rowdel = '\n';
		nffw = 8;
		nftrailing = 6;
		cutoff = 35;
	}
	MCExecPoint(const MCExecPoint &ep)
	{
		*this = ep;
		array = 0;
		deletearray = 0;
		buffer = 0;
		size = 0;
	}
	MCExecPoint(MCObject *object, MCHandlerlist *hlist, MCHandler *handler)
	{
		memset(this, 0, sizeof(MCExecPoint));
		curobj = object;
		curhlist = hlist;
		curhandler = handler;
		itemdel = ',';
		columndel = '\t';
		linedel = '\n';
		rowdel = '\n';
		nffw = 8;
		nftrailing = 6;
		cutoff = 35;
	}
	void restore(const MCExecPoint &ep)
	{
		curobj = ep.curobj;
		curhlist = ep.curhlist;
		curhandler = ep.curhandler;
	}
	~MCExecPoint()
	{
		delete buffer;
		if (deletearray)
			delete array;
	}
	void sethlist(MCHandlerlist *hl)
	{
		curhlist = hl;
	}
	void sethandler(MCHandler *newhandler)
	{
		curhandler = newhandler;
	}

	
	MCVariableValue *getarray(void)
	{
		return array;
	}

	Boolean getdeletearray()
	{
		return deletearray;
	}

	
	void takearray(MCVariableValue*& r_array, Boolean& r_delete)
	{
		r_array = array;
		r_delete = deletearray;

		array = 0;
		deletearray = 0;
	}

	
	
	void setarray(MCVariableValue *a, Boolean d);

	Boolean isempty(void) const
	{
		return format == VF_UNDEFINED || format != VF_ARRAY && format != VF_NUMBER && svalue . getlength() == 0;
	}
	
	Value_format getformat()
	{
		return format;
	}
	void setsvalue(const MCString &s)
	{
		svalue = s;
		format = VF_STRING;
	}
	void setnvalue(const real8 &n)
	{
		nvalue = n;
		format = VF_NUMBER;
	}
	void setnvalue(uint4 n)
	{
		nvalue = (real8)n;
		format = VF_NUMBER;
	}
	void setnvalue(int4 n)
	{
		nvalue = (real8)n;
		format = VF_NUMBER;
	}
	void setnvalue(uint2 n)
	{
		nvalue = (real8)n;
		format = VF_NUMBER;
	}
	void setnvalue(int2 n)
	{
		nvalue = (real8)n;
		format = VF_NUMBER;
	}
	void setboth(const MCString &s, const real8 &n)
	{
		svalue = s;
		nvalue = n;
		format = VF_BOTH;
	}
	void clear();
	char *getbuffer(uint4 newsize);
	uint4 getbuffersize()
	{
		return size;
	}
	void setbuffer(char *s, uint4 l)
	{
		buffer = s;
		size = l;
	}
	Boolean changed()
	{
		return svalue.getstring() == buffer;
	}
	void copysvalue(const char *s, uint4 l);
	void copysvalue(const char *s) {copysvalue(s, strlen(s));}

	
	void grabsvalue();

	
	void grabarray();

	
	void grabbuffer(char *p_buffer, uint4 p_length);

	
	void grab(void);

	void setint(int4);
	void setuint(uint4);
    void setint64(int64_t);
    void setuint64(uint64_t);
	void setr8(real8 n, uint2 fw, uint2 trailing, uint2 force);
	Exec_stat getreal8(real8 &, uint2, uint2, Exec_errors);
	Exec_stat getuint2(uint2 &, uint2, uint2, Exec_errors);
	Exec_stat getint4(int4 &, uint2, uint2, Exec_errors);
	Exec_stat getuint4(uint4 &, uint2, uint2, Exec_errors);
	Exec_stat getboolean(Boolean &, uint2, uint2, Exec_errors);
	Exec_stat ntos();
	Exec_stat ston();
	void lower();
	void upper();
	
	
	Exec_stat tos()
	{
		return format == VF_NUMBER ? ntos() : (format != VF_ARRAY ? ES_NORMAL : ES_ERROR);
	}
	
	
	Exec_stat ton()
	{
		return format == VF_STRING ? ston() : (format != VF_ARRAY ? ES_NORMAL : ES_ERROR);
	}
	
	
	Exec_stat tona(void)
	{
		return format == VF_STRING ? ston() : ES_NORMAL;
	}

	
	
	const char *getcstring(void);

	
	
	const MCString& getsvalue0(void)
	{
		getcstring();
		return svalue;
	}

	const MCString &getsvalue()
	{
		if (format==VF_NUMBER)
			tos();
		return svalue;
	}
	real8 getnvalue() const
	{
		return nvalue;
	}
	uint4 getuint4() const
	{
		return (uint4)(nvalue < 0.0 ? 0 : nvalue + 0.5);
	}
	uint2 getuint2() const
	{
		return (uint2)(nvalue < 0.0 ? 0 : nvalue + 0.5);
	}
	int4 getint4() const
	{
		return (int4)(nvalue < 0.0?nvalue - 0.5:nvalue + 0.5);
	}
	int64_t getint8() const
	{
		return (int4)(nvalue < 0.0?nvalue - 0.5:nvalue + 0.5);
	}
	uint64_t getuint8() const
	{
		return (uint4)(nvalue < 0.0?nvalue - 0.5:nvalue + 0.5);
	}
	void setstrlen()
	{
		svalue.set(buffer, strlen(buffer));
		format = VF_STRING;
	}
	void setlength(uint4 l)
	{
		svalue.set(buffer, l);
		format = VF_STRING;
	}
	MCObject *getobj() const
	{
		return curobj;
	}
	MCHandlerlist *gethlist() const
	{
		return curhlist;
	}
	MCHandler *gethandler() const
	{
		return curhandler;
	}
	uint2 getnffw() const
	{
		return nffw;
	}
	uint2 getnftrailing() const
	{
		return nftrailing;
	}
	uint2 getnfforce() const
	{
		return nfforce;
	}
	Boolean getcasesensitive() const
	{
		return casesensitive;
	}
	uint2 getcutoff() const
	{
		return cutoff;
	}
	Boolean getconvertoctals() const
	{
		return convertoctals;
	}
	char getitemdel() const
	{
		return itemdel;
	}
	char getcolumndel() const
	{
		return columndel;
	}
	char getrowdel() const
	{
		return rowdel;
	}
	char getlinedel()
	{
		return linedel;
	}
	Boolean getwholematches() const
	{
		return wholematches;
	}
	Boolean getusesystemdate() const
	{
		return usesystemdate;
	}
	Boolean getuseunicode() const
	{
		return useunicode;
	}
	Exec_stat setcasesensitive(uint2 line, uint2 pos)
	{
		return getboolean(casesensitive, line, pos, EE_PROPERTY_NAB);
	}
	Exec_stat setcutoff(uint2 line, uint2 pos)
	{
		return getuint2(cutoff, line, pos, EE_PROPERTY_NOTANUM);
	}
	Exec_stat setconvertoctals(uint2 line, uint2 pos)
	{
		return getboolean(convertoctals, line, pos, EE_PROPERTY_NAB);
	}
	Exec_stat setusesystemdate(uint2 line, uint2 pos)
	{
		return getboolean(usesystemdate, line, pos, EE_PROPERTY_NAB);
	}
	Exec_stat setuseunicode(uint2 line, uint2 pos)
	{
		return getboolean(useunicode, line, pos, EE_PROPERTY_NAB);
	}
	Exec_stat setitemdel(uint2 line, uint2 pos);
	Exec_stat setcolumndel(uint2 line, uint2 pos);
	Exec_stat setlinedel(uint2 line, uint2 pos);
	Exec_stat setrowdel(uint2 line, uint2 pos);
	Exec_stat setwholematches(uint2 line, uint2 pos)
	{
		return getboolean(wholematches, line, pos, EE_PROPERTY_NAB);
	}

	void setobj(MCObject *p_object)
	{
		curobj = p_object;
	}

	void setline(uint2 l)
	{
		line = l;
	}
	uint2 getline()
	{
		return line;
	}

	MCParentScriptUse *getparentscript(void) const
	{
		return parentscript;
	}

	void setparentscript(MCParentScriptUse *obj)
	{
		parentscript = obj;
	}

	void setnumberformat();
	void insert(const MCString &, uint4 s, uint4 e);
	uint1 *pad(char value, uint4 count);
	void substring(uint4 s, uint4 e);
	void tail(uint4 s);
	Boolean usingbuffer();
	void fill(uint4 s, char c, uint4 n);
	void texttobinary();
	void binarytotext();
	void parseURL(Url_type &urltype, const char *&hostname, const char *&rname, uint4 &port, const char *&auth);

	void utf16toutf8(void);
	void utf8toutf16(void);

	void utf16tonative(void);
	void nativetoutf16(void);
	
	void utf8tonative(void);
	void nativetoutf8(void);

	
	
	
	void mapunicode(bool is_unicode, bool want_unicode);
	
	
	
	Parse_stat findvar(MCNameRef p_name, MCVarref** r_var);

	

	void setstaticcstring(const char *string);
	void setstaticchars(const char *string, uindex_t length);
	void setstaticbytes(const void *bytes, uindex_t length);
	void setstaticmcstring(const MCString& string);

	void setempty(void);
	void setcstring(const char *string);
	void setmcstring(const MCString& string);
	void setstringf(const char *spec, ...);
	void setchars(const char *string, uindex_t length);
	void setchar(char character);
	void setbytes(const void *bytes, uindex_t length);
	void setbyte(uint8_t byte);
	void setboolean(Boolean value);
	void setpoint(int16_t x, int16_t y);
	void setrectangle(int32_t left, int32_t top, int32_t right, int32_t bottom);
	void setrectangle(const MCRectangle& rect);
	void setrectangle(const MCRectangle32& rect);
	void setcolor(uint32_t r, uint32_t g, uint32_t b);
	void setcolor(const MCColor& color);
	void setcolor(const MCColor& color, const char *colorname);
	void setpixel(uint32_t pixel);
	void setnewline(void);

	void appendcstring(const char *string);
	void appendmcstring(const MCString& string);
	void appendstringf(const char *spec, ...);
	void appendchars(const char *string, uindex_t length);
	void appendchar(char character);
	void appendunichars(const uint2 *string, uindex_t length);
	void appendunichar(uint2 character);
	void appendbytes(const void *bytes, uindex_t length);
	void appendbyte(uint8_t byte);
	void appenduint(uint32_t integer);
	void appendint(int32_t integer);
	void appendreal(double real);
	void appendnewline(void);
	void appendnewline(bool unicode);

	void concatcstring(const char *string, Exec_concat sep, bool first);
	void concatchars(const char *chars, uindex_t count, Exec_concat sep, bool first);
	void concatmcstring(const MCString& string, Exec_concat sep, bool first);
	void concatuint(uint32_t value, Exec_concat sep, bool first);
	void concatint(int32_t value, Exec_concat sep, bool first);
	void concatreal(double real, Exec_concat sep, bool first);

	void replacechar(char from, char to);

	void setnameref_unsafe(MCNameRef name);
	void appendnameref(MCNameRef name);
	void concatnameref(MCNameRef name, Exec_concat sep, bool first);

	bool copyasnameref(MCNameRef& name);
    
	
	
    bool trytoconvertutf16tonative();

private:
	void dounicodetomultibyte(bool p_native, bool p_reverse);

	void concat(uint4 n, Exec_concat, Boolean);
	void concat(int4 n, Exec_concat, Boolean);
	void concat(const MCString &, Exec_concat, Boolean);
};

inline void MCExecPoint::utf16toutf8(void)
{
	dounicodetomultibyte(false, false);
}

inline void MCExecPoint::utf8toutf16(void)
{
	dounicodetomultibyte(false, true);
}

inline void MCExecPoint::utf16tonative(void)
{
	dounicodetomultibyte(true, false);
}

inline void MCExecPoint::nativetoutf16(void)
{
	dounicodetomultibyte(true, true);
}

inline void MCExecPoint::utf8tonative(void)
{
	utf8toutf16();
	utf16tonative();
}

inline void MCExecPoint::nativetoutf8(void)
{
	nativetoutf16();
	utf16toutf8();
}


#line 1 "c:\\github\\livecode-runrev\\engine\\src\\variable_impl.h"





















extern const char *MCnullstring;
extern uint4 MCU_r8tos(char *&sptr, uint4 &s, real8 n,uint2 fw, uint2 trailing, uint2 force);
extern Boolean MCU_stor8(const MCString&, real8& d, Boolean co);



inline Value_format MCVariableValue::get_type(void) const
{
	return (Value_format)_type;
}

inline void MCVariableValue::set_type(Value_format p_new_type)
{
	_type = (uint8_t)p_new_type;
}

inline bool MCVariableValue::get_external(void) const
{
	return (_flags & kExternalBit) != 0;
}

inline void MCVariableValue::set_external(void)
{
	_flags |= kExternalBit;
}

inline bool MCVariableValue::get_temporary(void) const
{
	return (_flags & kTemporaryBit) != 0;
}

inline void MCVariableValue::set_temporary(void)
{
	_flags |= kTemporaryBit;
}











































inline void MCVariableValue::set_dbg_notify(bool p_state)
{
}

inline bool MCVariableValue::get_dbg_notify(void) const
{
	return false;
}

inline void MCVariableValue::set_dbg_changed(bool p_state)
{
}

inline bool MCVariableValue::get_dbg_changed(void) const
{
	return false;
}

inline void MCVariableValue::set_dbg_mutated(bool p_state)
{
}

inline bool MCVariableValue::get_dbg_mutated(void) const
{
	return false;
}
#line 127 "c:\\github\\livecode-runrev\\engine\\src\\variable_impl.h"



inline MCVariableValue::MCVariableValue(void)
{
	set_type(VF_UNDEFINED);
	_flags = 0;

	strnum . buffer . data = 0;
	strnum . buffer . size = 0;
}

inline MCVariableValue::MCVariableValue(const MCVariableValue& p_other)
{
	copy(p_other);
	_flags = 0;
}

inline MCVariableValue::~MCVariableValue(void)
{
	destroy();






}



inline bool MCVariableValue::is_clear(void) const
{
	return get_type() == VF_UNDEFINED;
}

inline bool MCVariableValue::is_undefined(void) const
{
	return is_clear();
}

inline bool MCVariableValue::is_empty(void) const
{
	Value_format t_type;
	t_type = get_type();
	return t_type == VF_UNDEFINED || (t_type == VF_STRING && strnum . svalue . length == 0);
}

inline bool MCVariableValue::is_string(void) const
{
	Value_format t_type;
	t_type = get_type();
	return t_type == VF_STRING || t_type == VF_BOTH;
}

inline bool MCVariableValue::is_real(void) const
{
	
	Value_format t_type;
	t_type = get_type();
	return t_type == VF_NUMBER || t_type == VF_BOTH;
}

inline bool MCVariableValue::is_number(void) const
{
	return is_real();
}

inline bool MCVariableValue::is_array(void) const
{
	return get_type() == VF_ARRAY;
}



inline Value_format MCVariableValue::get_format(void) const
{
	return get_type();
}



inline MCString MCVariableValue::get_string(void) const
{
	(void)( (!!(is_string())) || (_wassert(L"is_string()", L"c:\\github\\livecode-runrev\\engine\\src\\variable_impl.h", 211), 0) );
	return MCString(strnum . svalue . string, strnum . svalue . length);
}

inline real64_t MCVariableValue::get_real(void) const
{
	(void)( (!!(is_real())) || (_wassert(L"is_real()", L"c:\\github\\livecode-runrev\\engine\\src\\variable_impl.h", 217), 0) );
	return strnum . nvalue;
}

inline MCVariableArray* MCVariableValue::get_array(void)
{
	(void)( (!!(is_array())) || (_wassert(L"is_array()", L"c:\\github\\livecode-runrev\\engine\\src\\variable_impl.h", 223), 0) );
	return &array;
}



inline MCString MCVariableValue::get_custom_string(void) const
{
	return MCString(strnum . svalue . string, strnum . svalue . length);
}



inline void MCVariableValue::clear(void)
{
	destroy();

	set_type(VF_UNDEFINED);

	strnum . buffer . data = 0;
	strnum . buffer . size = 0;
	
	set_dbg_changed(true);
}

inline bool MCVariableValue::assign(const MCVariableValue& v)
{
	destroy();
	return copy(v);
}



inline bool MCVariableValue::has_element(MCExecPoint& ep, const MCString& key)
{
	(void)( (!!(is_array())) || (_wassert(L"is_array()", L"c:\\github\\livecode-runrev\\engine\\src\\variable_impl.h", 258), 0) );
	return array . lookuphash(key, ep . getcasesensitive(), 0) != 0;
}

inline bool MCVariableValue::ensure_real(MCExecPoint& ep)
{
	if (is_number())
		return true;

	return coerce_to_real(ep);
}

inline bool MCVariableValue::ensure_string(MCExecPoint& ep)
{
	if (is_string())
		return true;

	return coerce_to_string(ep);
}



inline void MCVariableValue::getextents(MCExecPoint& ep)
{
	if (!is_array())
	{
		ep . clear();
		return;
	}

	array . getextents(ep);
}

inline void MCVariableValue::getkeys(MCExecPoint& ep)
{
	if (!is_array())
	{
		ep . clear();
		return;
	}

	array . getkeys(ep);
}

inline void MCVariableValue::getkeys(char **r_keys, uint32_t p_count)
{
	if (!is_array())
		return;

	array . getkeys(r_keys, p_count);
}



inline void MCVariableValue::destroy(void)
{
	if (get_type() != VF_ARRAY)
		_free_dbg(strnum . buffer . data, 1);
	else
		array . freehash();
}



inline Boolean MCVariableArray::issequence(void)
{
	return isnumeric() && dimensions == 1 && extents[0] . min == 1;
}

#line 328 "c:\\github\\livecode-runrev\\engine\\src\\variable_impl.h"
#line 552 "c:\\github\\livecode-runrev\\engine\\src\\execpt.h"
#line 553 "c:\\github\\livecode-runrev\\engine\\src\\execpt.h"

#line 555 "c:\\github\\livecode-runrev\\engine\\src\\execpt.h"
#line 27 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\util.h"























typedef struct
{
	uint2 lockscreen;
	uint2 errorlock;
	Boolean watchcursor;
	Boolean lockerrors;
	Boolean lockmessages;
	Boolean lockmoves;
	Boolean lockrecent;
	Boolean lockmenus;
	Boolean interrupt;
	uint2 dragspeed;
	MCCard *dynamiccard;
	MCObject *errorptr;
	MCObject *errorlockptr;
	Boolean exitall;
}
MCSaveprops;

typedef struct
{
	char *svalue;
	real8 nvalue;
	const void *data;
}
MCSortnode;

extern void MCU_play();
extern void MCU_play_stop();
extern void MCU_init();
extern void MCU_watchcursor(MCStack *sptr, Boolean force);
extern void MCU_unwatchcursor(MCStack *sptr, Boolean force);
extern void MCU_resetprops(Boolean update);
extern void MCU_saveprops(MCSaveprops &sp);
extern void MCU_restoreprops(MCSaveprops &sp);
extern int4 MCU_any(int4 max);
extern void MCU_getnumberformat(MCExecPoint &, uint2, uint2, uint2);
extern void MCU_setnumberformat(const MCString &, uint2 &, uint2 &, uint2 &);
extern real8 MCU_stoIEEE(const char *bytes);
extern real8 MCU_i4tor8(int4 in);
extern real8 MCU_fwrap(real8 p_x, real8 p_y);
extern int4 MCU_r8toi4(real8 in);
extern void MCU_srand();
extern real8 MCU_drand();
extern Boolean MCU_comparechar(const char *sptr, char target,
	                               Boolean isunicode = 0);
extern Boolean MCU_strchr(const char *&, uint4 &, char,
	                          Boolean isunicode = 0);
inline uint1 MCU_charsize(Boolean isunicode = 0);
extern char *MCU_strtok(char *, const char *);
extern int4 MCU_strtol(const char *&, uint4 &, int1, Boolean &done,
	                       Boolean reals = 0, Boolean octals = 0);
extern real8 MCU_strtor8(const char *&, uint4 &, int1, Boolean &r_done,
						Boolean convertoctals = 0);
extern void MCU_strip(char *sptr, uint2 trailing, uint2 force);
extern uint4 MCU_r8tos(char *&sptr, uint4 &s, real8 n,uint2 fw, uint2 trailing, uint2 force);
extern Boolean MCU_stor8(const MCString&, real8& d, Boolean co = 0);
extern Boolean MCU_stoi2(const MCString&, int2 &d);
extern Boolean MCU_stoui2(const MCString&, uint2 &d);
extern Boolean MCU_stoi2x2(const MCString&, int2 &d1, int2 &d2);
extern Boolean MCU_stoi2x4(const MCString&, int2 &d1, int2 &d2, int2 &d3, int2 &d4);
extern Boolean MCU_stoi4x4(const MCString&, int32_t &d1, int32_t &d2, int32_t &d3, int32_t &d4);
extern Boolean MCU_stobxb(const MCString& p_string, Boolean &r_left, Boolean& r_right);
extern Boolean MCU_stoi4(const MCString&, int4& d);
extern Boolean MCU_stoui4(const MCString&, uint4& d);
extern Boolean MCU_stob(const MCString&, Boolean& condition);
extern void MCU_lower(char *sptr, const MCString& s);
extern void MCU_upper(char *sptr, const MCString& s);
extern int4 MCU_strncasecmp(const char *one, const char *two, size_t n);
extern Boolean MCU_offset(const MCString &p, const MCString &w,
	                          uint4 &offset, Boolean casesensitive = 0);
void MCU_chunk_offset(MCExecPoint &ep, MCString &w,
                      Boolean whole, Chunk_term delimiter);
extern void MCU_additem(char *&dptr, const char *sptr, Boolean first);
extern void MCU_addline(char *&dptr, const char *sptr, Boolean first);
extern void MCU_break_string(const MCString &s, MCString *&ptrs, uint2 &nptrs,
	                             Boolean isunicode = 0);
extern void MCU_sort(MCSortnode *items, uint4 nitems,
	                     Sort_type dir, Sort_type form);



extern const char *MCU_ktos(Boolean condition);
extern Boolean MCU_matchflags(const MCString &, uint4 &, uint4, Boolean &);
extern Boolean MCU_matchname(const MCString &, Chunk_term type, MCNameRef name);
extern void MCU_snap(int2 &p);
extern void MCU_roundrect(MCPoint *&, uint2 &npoints,
	                          const MCRectangle &, uint2 radius);
extern void MCU_unparsepoints(MCPoint *points, uint2 npoints, MCExecPoint &);
extern Boolean MCU_parsepoints(MCPoint *&oldpoints, uint2 &n, const MCString &);
extern Boolean MCU_parsepoint(MCPoint &r_point, const MCString &);
extern void MCU_querymouse(int2 &x, int2 &y);
extern void MCU_resetcursors();
extern void MCU_set_rect(MCRectangle &rect, int2 x, int2 y, uint2 w, uint2 h);
extern void MCU_set_rect(MCRectangle32 &rect, int32_t x, int32_t y, int32_t w, int32_t h);
extern Boolean MCU_point_in_rect(const MCRectangle &srect, int2 x, int2 y);
extern Boolean MCU_rect_in_rect(const MCRectangle &p, const MCRectangle &w);
extern Boolean MCU_point_on_line(MCPoint *points, uint2 npoints,
	                                 int2 x, int2 y, uint2 linesize);
extern Boolean MCU_point_in_polygon(MCPoint *points, uint2 npoints,
	                                    int2 x, int2 y);
extern void MCU_offset_points(MCPoint *points, uint2 npoints,
	                              int2 xoff, int2 yoff);

extern MCRectangle MCU_compute_rect(int2 x1, int2 y1, int2 x2, int2 y2);
extern MCRectangle MCU_center_rect(const MCRectangle &, const MCRectangle &);
extern MCRectangle MCU_bound_rect(const MCRectangle &, int2, int2, uint2, uint2);
extern MCRectangle MCU_clip_rect(const MCRectangle &, int2, int2, uint2, uint2);
extern MCRectangle MCU_intersect_rect(const MCRectangle &one, const MCRectangle &two);
extern MCRectangle MCU_union_rect(const MCRectangle &one, const MCRectangle &two);
extern MCRectangle MCU_subtract_rect(const MCRectangle &one, const MCRectangle &two);
extern MCRectangle MCU_reduce_rect(const MCRectangle &rect, int2 amount);
extern MCRectangle MCU_scale_rect(const MCRectangle &rect, int2 factor);
extern MCRectangle MCU_offset_rect(const MCRectangle& r, int2 ox, int2 oy);

extern MCRectangle MCU_recttoroot(MCStack *sptr, const MCRectangle &o);
extern void MCU_getshift(uint4 mask, uint2 &shift, uint2 &outmask);
extern Exec_stat MCU_choose_tool(MCExecPoint &ep, Tool littool,
	                                 uint2 line, uint2 pos);
extern Exec_stat MCU_dofrontscripts(Handler_type htype, MCNameRef message, MCParameter *params);
extern void MCU_path2std(char *dptr);
extern void MCU_path2native(char *dptr);
extern void MCU_fix_path(char *cstr);
extern void MCU_base64encode(MCExecPoint &ep);
extern void MCU_base64decode(MCExecPoint &ep);
extern void MCU_urlencode(MCExecPoint &ep);
extern void MCU_urldecode(MCExecPoint &ep);
extern Boolean MCU_freeinserted(MCObjectList *&l);
extern void MCU_cleaninserted();
extern Exec_stat MCU_change_color(MCColor &c, char *&n, MCExecPoint &ep,
	                                  uint2 line, uint2 pos);
extern void MCU_get_color(MCExecPoint &ep, const char *name, MCColor &c);
extern void MCU_dofunc(Functions func, uint4 &nparams, real8 &n,
	                       real8 tn, real8 oldn, MCSortnode *titems);
extern void MCU_geturl(MCExecPoint &ep);
extern void MCU_puturl(MCExecPoint &ep, MCExecPoint &data);
extern uint1 MCU_unicodetocharset(uint2 uchar);
extern uint1 MCU_languagetocharset(const char *langname);
extern const char *MCU_charsettolanguage(uint1 charset);
extern uint1 MCU_wincharsettocharset(uint2 wincharset);
extern uint1 MCU_charsettowincharset(uint1 charset);
extern void MCU_multibytetounicode(const char *s, uint4 len, char *d,
	                                   uint4 destbufferlength, uint4 &destlen,
	                                   uint1 charset);
extern void MCU_unicodetomultibyte(const char *s, uint4 len, char *d,
	                                   uint4 destbufferlength, uint4 &destlen,
	                                   uint1 charset);
extern bool MCU_compare_strings_native(const char *p_a, bool p_a_isunicode, const char *p_b, bool p_b_isunicode);
extern double MCU_squared_distance_from_line(int4 sx, int4 sy, int4 ex, int4 ey, int4 x, int4 y);



extern bool MCU_random_bytes(size_t count, void *buffer);



struct MCRange
{
	int from;
	int to;
};

extern bool MCU_disjointrangeinclude(MCRange*& p_ranges, int& p_count, int p_from, int p_to);
extern bool MCU_disjointrangecontains(MCRange* p_ranges, int p_count, int p_element);





IO_stat MCU_dofakewrite(char*& x_buffer, size_t& x_length, const void *p_data, uint4 p_size, uint4 p_count);



inline bool MCU_empty_rect(const MCRectangle& a)
{
	return a . width == 0 || a . height == 0;
}

inline bool MCU_equal_rect(const MCRectangle& a, const MCRectangle& b)
{
	return a . x == b . x && a . y == b . y &&  a . width == b . width && a . height == b . height;
}

inline MCRectangle MCU_make_rect(int2 x, int2 y, uint2 w, uint2 h)
{
	MCRectangle r;
	r . x = x;
	r . y = y;
	r . width = w;
	r . height = h;
	return r;
}

#line 218 "c:\\github\\livecode-runrev\\engine\\src\\util.h"
#line 28 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\undolst.h"






















#line 1 "c:\\github\\livecode-runrev\\engine\\src\\dllst.h"






















class MCDLlist
{
protected:
	MCDLlist *nptr;
	MCDLlist *pptr;
public:
	MCDLlist()
	{
		pptr = nptr = this;
	}
	MCDLlist(const MCDLlist &dref)
	{
		nptr = pptr = this;
	}
	virtual ~MCDLlist();
	
	virtual void removelink(MCObject *optr);
	MCDLlist *next()
	{
		return nptr;
	}
	MCDLlist *prev()
	{
		return pptr;
	}
	const MCDLlist *next() const
	{
		return nptr;
	}
	const MCDLlist *prev() const
	{
		return pptr;
	}
	void totop(MCDLlist *&list);
	void insertto(MCDLlist *&list);
	void appendto(MCDLlist *&list);
	void append(MCDLlist *node);
	void splitat(MCDLlist *node);
	MCDLlist *remove(MCDLlist *&list);




};
#line 68 "c:\\github\\livecode-runrev\\engine\\src\\dllst.h"
#line 24 "c:\\github\\livecode-runrev\\engine\\src\\undolst.h"

enum Undo_type {
    UT_MOVE,
    UT_SIZE,
    UT_DELETE,
    UT_REPLACE,
    UT_DELETE_TEXT,
    UT_REPLACE_TEXT,
    UT_TYPE_TEXT,
	UT_MOVE_TEXT,
    UT_PAINT
};

struct DTstruct
{
	uint4 index;
	uint4 old_index;
	Boolean newline;
	uint4 newchars;
	MCParagraph *data;
};

struct Ustruct
{
	Undo_type type;
	union {
		MCRectangle rect;
		MCPoint deltas;
		uint2 layer;
		DTstruct text;
	} ud;
};

class MCUndonode : public MCDLlist
{
	MCObject *object;
	Ustruct *savedata;
public:
	MCUndonode(MCObject *objptr, Ustruct *data);
	~MCUndonode();
	void undo();
	MCObject *getobject()
	{
		return object;
	}
	Ustruct *getdata()
	{
		return savedata;
	}
	MCUndonode *next()
	{
		return (MCUndonode *)MCDLlist::next();
	}
	MCUndonode *prev()
	{
		return (MCUndonode *)MCDLlist::prev();
	}
	void totop(MCUndonode *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCUndonode *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCUndonode *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCUndonode *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCUndonode *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCUndonode *remove
	(MCUndonode *&list)
	{
		return (MCUndonode *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
};

class MCUndolist
{
	MCUndonode *nodes;
public:
	MCUndolist();
	~MCUndolist();
	void savestate(MCObject *objptr, Ustruct *data);
	void freestate();
	void freeobject(MCObject *objptr);
	MCObject *getobject();
	Ustruct *getstate();
	Boolean undo();
};
#line 123 "c:\\github\\livecode-runrev\\engine\\src\\undolst.h"
#line 29 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\sellst.h"






















#line 1 "c:\\github\\livecode-runrev\\engine\\src\\dllst.h"



































































#line 24 "c:\\github\\livecode-runrev\\engine\\src\\sellst.h"

class MCSelnode : public MCDLlist
{
public:
	MCObject *ref;
	MCSelnode(MCObject *object);
	~MCSelnode();
	MCSelnode *next()
	{
		return (MCSelnode *)MCDLlist::next();
	}
	MCSelnode *prev()
	{
		return (MCSelnode *)MCDLlist::prev();
	}
	void totop(MCSelnode *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCSelnode *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCSelnode *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCSelnode *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCSelnode *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCSelnode *remove(MCSelnode *&list)
	{
		return (MCSelnode *)MCDLlist::remove((MCDLlist *&)list);
	}
};

class MCSellist
{
	MCStack *owner;
	MCSelnode *objects;
	MCSelnode *curobject;
	int2 startx, starty;
	int2 lastx, lasty;
	Boolean locked;
	Boolean dropclone;
public:
	MCSellist();
	~MCSellist();
	MCObject *getfirst();
	void getids(MCExecPoint &);
	void clear(Boolean message);
	void top(MCObject *objptr);
	void replace(MCObject *objptr);
	void add(MCObject *objptr, bool p_sendmessage = true);
	void remove(MCObject *objptr, bool p_sendmessage = true);
	void sort();
	uint32_t count();
	MCControl *clone(MCObject *target);
	Exec_stat group(uint2 line, uint2 pos);
	Boolean copy();
	Boolean cut();
	Boolean del();
	void startmove(int2 x, int2 y, Boolean canclone);
	void continuemove(int2 x, int2 y);
	Boolean endmove();
	void lockclear()
	{
		locked = 1;
	}
	void unlockclear()
	{
		locked = 0;
	}
	void redraw();

private:
	bool clipboard(bool p_is_cut);
};
#line 108 "c:\\github\\livecode-runrev\\engine\\src\\sellst.h"
#line 30 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\image.h"






















#line 1 "c:\\github\\livecode-runrev\\engine\\src\\control.h"























#line 1 "c:\\github\\livecode-runrev\\engine\\src\\object.h"



























#line 1 "c:\\github\\livecode-runrev\\engine\\src\\imagebitmap.h"






















struct MCImageBitmap
{
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t *data;
	
	bool has_transparency;
	bool has_alpha;
};

struct MCImageIndexedBitmap
{
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint8_t *data;

	uint32_t palette_size;
	uint32_t transparent_index;
	MCColor *palette;
};



bool MCImageBitmapCreate(uindex_t p_width, uindex_t p_height, MCImageBitmap *&r_bitmap);
void MCImageFreeBitmap(MCImageBitmap *p_bitmap);
bool MCImageCopyBitmap(MCImageBitmap *p_bitmap, MCImageBitmap *&r_copy);
bool MCImageCopyBitmapRegion(MCImageBitmap *p_bitmap, MCRectangle &p_region, MCImageBitmap *&r_copy);

void MCImageBitmapClear(MCImageBitmap *p_bitmap);
void MCImageBitmapClearRegion(MCImageBitmap *p_bitmap, MCRectangle p_region);
void MCImageBitmapSet(MCImageBitmap *p_bitmap, uint32_t p_pixel_value);
void MCImageBitmapCopyRegionToBitmap(MCImageBitmap *p_src, MCImageBitmap *p_dst, int32_t sx, int32_t sy, int32_t dx, int32_t dy, uint32_t sw, uint32_t sh);
void MCImageBitmapCopyRegionToBitmap(MCImageBitmap *p_dst, MCImageBitmap *p_src, MCPoint p_dst_offset, MCRectangle p_src_rect);
void MCImageBitmapCopyRegionToBuffer(MCImageBitmap *p_bitmap, int32_t p_sx, int32_t p_sy, int32_t p_sw, int32_t p_sh, uindex_t p_buffer_stride, uint8_t *p_buffer);
void MCImageBitmapCopyRegionFromBuffer(MCImageBitmap *p_bitmap, MCRectangle &p_region, const uint8_t *p_buffer, uindex_t p_buffer_stride);

void MCImageBitmapCheckTransparency(MCImageBitmap *p_bitmap);
bool MCImageBitmapHasTransparency(MCImageBitmap *p_bitmap);
bool MCImageBitmapHasTransparency(MCImageBitmap *p_bitmap, bool &r_has_alpha);
uint32_t MCImageBitmapGetPixel(MCImageBitmap *p_bitmap, uindex_t x, uindex_t y);
void MCImageBitmapSetPixel(MCImageBitmap *p_bitmap, uindex_t x, uindex_t y, uint32_t p_pixel);

void MCImageBitmapPremultiply(MCImageBitmap *p_bitmap);
void MCImageBitmapPremultiplyRegion(MCImageBitmap *p_bitmap, int32_t p_sx, int32_t p_sy, int32_t p_sw, int32_t p_sh, uint32_t p_pixel_stride, uint32_t *p_pixel_ptr);
void MCImageBitmapUnpremultiply(MCImageBitmap *p_bitmap);
void MCImageBitmapUnpremultiplyChecking(MCImageBitmap *p_bitmap);
void MCImageBitmapFixPremultiplied(MCImageBitmap *p_bitmap);



bool MCImageCreateIndexedBitmap(uindex_t p_width, uindex_t p_height, MCImageIndexedBitmap *&r_indexed);
void MCImageFreeIndexedBitmap(MCImageIndexedBitmap *p_bitmap);

bool MCImageIndexedBitmapHasTransparency(MCImageIndexedBitmap *p_bitmap);
bool MCImageIndexedBitmapAddTransparency(MCImageIndexedBitmap *p_bitmap);



bool MCImageConvertIndexedToBitmap(MCImageIndexedBitmap *p_indexed, MCImageBitmap *&r_bitmap);
bool MCImageConvertBitmapToIndexed(MCImageBitmap *p_bitmap, bool p_ignore_transparent, MCImageIndexedBitmap *&r_indexed);
bool MCImageForceBitmapToIndexed(MCImageBitmap *p_bitmap, bool p_dither, MCImageIndexedBitmap *&r_indexed);



void MCImageBitmapExtractMask(MCImageBitmap *p_bitmap, void *p_mask, uint32_t p_mask_stride, uint8_t p_threshold);



struct MCImageFrame
{
	MCImageBitmap *image;
	uint32_t duration;
};

void MCImageFreeFrames(MCImageFrame *p_frames, uindex_t p_count);




bool MCImageDataIsJPEG(const MCString& data);
bool MCImageDataIsPNG(const MCString& data);
bool MCImageDataIsGIF(const MCString& data);



#line 111 "c:\\github\\livecode-runrev\\engine\\src\\imagebitmap.h"
#line 29 "c:\\github\\livecode-runrev\\engine\\src\\object.h"
#line 30 "c:\\github\\livecode-runrev\\engine\\src\\object.h"

#line 1 "c:\\github\\livecode-runrev\\engine\\src\\globals.h"






















#line 1 "c:\\github\\livecode-runrev\\engine\\src\\mcstring.h"
















extern const uint1 MCisotranslations[256];
extern const uint1 MCmactranslations[256];

extern const char *MCtoolnames[];

extern const uint4 MCbuildnumber;
extern const char *MCversionstring;
extern const char *MCcopystring;
extern const char *MCstandardstring;
extern const char *MCdialogstring;
extern const char *MCmovablestring;
extern const char *MCpalettestring;
extern const char *MCsheetstring;
extern const char *MCdrawerstring;
extern const char *MCmodalstring;
extern const char *MCmodelessstring;
extern const char *MCtoplevelstring;
extern const char *MCtransparentstring;
extern const char *MCopaquestring;
extern const char *MCrectanglestring;
extern const char *MCshadowstring;
extern const char *MCscrollingstring;
extern const char *MCroundrectstring;
extern const char *MCcheckboxstring;
extern const char *MCradiobuttonstring;
extern const char *MClinestring;
extern const char *MCpolygonstring;
extern const char *MCarcstring;
extern const char *MCovalstring;
extern const char *MCregularstring;
extern const char *MCcurvestring;
extern const char *MCtextstring;
extern const char *MCleftstring;
extern const char *MCcenterstring;
extern const char *MCrightstring;
extern const char *MCjustifystring;
extern const char *MCplainstring;
extern const char *MCmixedstring;
extern const char *MCboxstring;
extern const char *MCthreedboxstring;
extern const char *MCunderlinestring;
extern const char *MCstrikeoutstring;
extern const char *MCgroupstring;
extern const char *MClinkstring;
extern const char *MCtruestring;
extern const char *MCfalsestring;
extern const char *MCdownstring;
extern const char *MCupstring;
extern const char *MCshiftstring;
extern const char *MCcommandstring;
extern const char *MCcontrolstring;
extern const char *MCmod1string;
extern const char *MCpulldownstring;
extern const char *MCpopupstring;
extern const char *MCoptionstring;
extern const char *MCcascadestring;
extern const char *MCcombostring;
extern const char *MCtabstring;
extern const char *MCstackstring;
extern const char *MCaudiostring;
extern const char *MCvideostring;
extern const char *MCdefaultstring;
extern const char *MCtitlestring;
extern const char *MCmenustring;
extern const char *MCminimizestring;
extern const char *MCmaximizestring;
extern const char *MCclosestring;
extern const char *MCmetalstring;
extern const char *MCutilitystring;
extern const char *MCnoshadowstring;
extern const char *MCforcetaskbarstring;
extern const char *MCunicodestring;
extern const char *MCnativestring;

extern const char *MCbackgroundstring;
extern const char *MCcardstring;
extern const char *MCbuttonstring;
extern const char *MCgraphicstring;
extern const char *MCepsstring;
extern const char *MCscrollbarstring;
extern const char *MCplayerstring;
extern const char *MCscalestring;
extern const char *MCprogressstring;
extern const char *MCimagestring;
extern const char *MCfieldstring;
extern const char *MCcolorstring;
extern const char *MCmagnifierstring;

extern const char *MCnotfoundstring;
extern const char *MCplatformstring;
extern const char *MClnfamstring;
extern const char *MClnfmacstring;
extern const char *MClnfmotifstring;
extern const char *MClnfwinstring;
extern const char *MCuntitledstring;
extern const char *MCapplicationstring;
extern const char *MCanswernamestring;
extern const char *MCasknamestring;
extern const char *MCfsnamestring;
extern const char *MCcsnamestring;
extern const char *MChelpnamestring;
extern const char *MChomenamestring;
extern const char *MChcstatnamestring;
extern const char *MCmessagenamestring;
extern const char *MCdonestring;
extern const char *MCnullstring;
extern const char *MCintersectstring;
extern const char *MCsurroundstring;
extern const char *MCtopstring;
extern const char *MCbottomstring;
extern const char *MCcancelstring;

extern const char *MCliststylestrings[];
extern const char *MCtextalignstrings[];

extern MCNameRef MCN_msg;
extern MCNameRef MCN_each;
extern MCNameRef MCN_it;

extern MCNameRef MCN_default_text_font;

extern MCNameRef MCM_apple_event;
extern MCNameRef MCM_arrow_key;
extern MCNameRef MCM_backspace_key;
extern MCNameRef MCM_close_background;
extern MCNameRef MCM_close_card;
extern MCNameRef MCM_close_control;
extern MCNameRef MCM_close_field;
extern MCNameRef MCM_close_stack;
extern MCNameRef MCM_close_stack_request;
extern MCNameRef MCM_color_changed;
extern MCNameRef MCM_command_key_down;
extern MCNameRef MCM_control_key_down;
extern MCNameRef MCM_copy_key;
extern MCNameRef MCM_current_time_changed;
extern MCNameRef MCM_cut_key;
extern MCNameRef MCM_debug_str;

extern MCNameRef MCM_delete_background;
extern MCNameRef MCM_delete_button;
extern MCNameRef MCM_delete_card;
extern MCNameRef MCM_delete_eps;
extern MCNameRef MCM_delete_field;
extern MCNameRef MCM_delete_graphic;
extern MCNameRef MCM_delete_group;
extern MCNameRef MCM_delete_image;
extern MCNameRef MCM_delete_scrollbar;
extern MCNameRef MCM_delete_player;
extern MCNameRef MCM_delete_stack;

extern MCNameRef MCM_delete_key;
extern MCNameRef MCM_delete_url;
extern MCNameRef MCM_desktop_changed;
extern MCNameRef MCM_drag_drop;
extern MCNameRef MCM_drag_end;
extern MCNameRef MCM_drag_enter;
extern MCNameRef MCM_drag_leave;
extern MCNameRef MCM_drag_move;
extern MCNameRef MCM_drag_start;
extern MCNameRef MCM_edit_script;
extern MCNameRef MCM_enter_in_field;
extern MCNameRef MCM_enter_key;
extern MCNameRef MCM_error_dialog;
extern MCNameRef MCM_escape_key;
extern MCNameRef MCM_eval;
extern MCNameRef MCM_exit_field;
extern MCNameRef MCM_focus_in;
extern MCNameRef MCM_focus_out;
extern MCNameRef MCM_function_key;
extern MCNameRef MCM_get_cached_urls;
extern MCNameRef MCM_get_url;
extern MCNameRef MCM_get_url_status;


extern MCNameRef MCM_gradient_edit_ended;
extern MCNameRef MCM_gradient_edit_started;

extern MCNameRef MCM_help;
extern MCNameRef MCM_hot_spot_clicked;
extern MCNameRef MCM_icon_menu_pick;
extern MCNameRef MCM_icon_menu_opening;
extern MCNameRef MCM_status_icon_menu_pick;
extern MCNameRef MCM_status_icon_menu_opening;
extern MCNameRef MCM_status_icon_click;
extern MCNameRef MCM_status_icon_double_click;
extern MCNameRef MCM_iconify_stack;
extern MCNameRef MCM_id_changed;
extern MCNameRef MCM_idle;
extern MCNameRef MCM_license;
extern MCNameRef MCM_internal;
extern MCNameRef MCM_internal2;
extern MCNameRef MCM_internal3;
extern MCNameRef MCM_key_down;
extern MCNameRef MCM_key_up;
extern MCNameRef MCM_keyboard_activated;
extern MCNameRef MCM_keyboard_deactivated;
extern MCNameRef MCM_library_stack;
extern MCNameRef MCM_link_clicked;
extern MCNameRef MCM_load_url;
extern MCNameRef MCM_main_stack_changed;


extern MCNameRef MCM_main_stacks_changed;

extern MCNameRef MCM_menu_pick;
extern MCNameRef MCM_message;
extern MCNameRef MCM_message_handled;
extern MCNameRef MCM_message_not_handled;
extern MCNameRef MCM_mouse_double_down;
extern MCNameRef MCM_mouse_double_up;
extern MCNameRef MCM_mouse_down;
extern MCNameRef MCM_mouse_down_in_backdrop;
extern MCNameRef MCM_mouse_enter;
extern MCNameRef MCM_mouse_leave;
extern MCNameRef MCM_mouse_move;
extern MCNameRef MCM_mouse_release;
extern MCNameRef MCM_mouse_still_down;
extern MCNameRef MCM_mouse_up;
extern MCNameRef MCM_mouse_up_in_backdrop;
extern MCNameRef MCM_mouse_within;
extern MCNameRef MCM_move_control;
extern MCNameRef MCM_move_stack;
extern MCNameRef MCM_move_stopped;
extern MCNameRef MCM_movie_touched;
extern MCNameRef MCM_name_changed;
extern MCNameRef MCM_new_background;
extern MCNameRef MCM_new_card;
extern MCNameRef MCM_new_stack;
extern MCNameRef MCM_new_tool;
extern MCNameRef MCM_node_changed;


extern MCNameRef MCM_object_selection_ended;
extern MCNameRef MCM_object_selection_started;

extern MCNameRef MCM_open_background;
extern MCNameRef MCM_open_control;
extern MCNameRef MCM_open_card;
extern MCNameRef MCM_open_field;
extern MCNameRef MCM_open_stack;
extern MCNameRef MCM_option_key_down;
extern MCNameRef MCM_paste_key;
extern MCNameRef MCM_play_paused;
extern MCNameRef MCM_play_started;
extern MCNameRef MCM_play_stopped;
extern MCNameRef MCM_post_url;
extern MCNameRef MCM_preopen_background;
extern MCNameRef MCM_preopen_card;
extern MCNameRef MCM_preopen_control;
extern MCNameRef MCM_preopen_stack;


extern MCNameRef MCM_property_changed;

extern MCNameRef MCM_put_url;
extern MCNameRef MCM_qtdebugstr;
extern MCNameRef MCM_raw_key_down;
extern MCNameRef MCM_raw_key_up;

extern MCNameRef MCM_relaunch;
#line 278 "c:\\github\\livecode-runrev\\engine\\src\\mcstring.h"
extern MCNameRef MCM_release_stack;
extern MCNameRef MCM_reload_stack;
extern MCNameRef MCM_resize_control;


extern MCNameRef MCM_resize_control_ended;
extern MCNameRef MCM_resize_control_started;

extern MCNameRef MCM_resize_stack;
extern MCNameRef MCM_resolution_error;
extern MCNameRef MCM_resume;
extern MCNameRef MCM_resume_stack;
extern MCNameRef MCM_return_in_field;
extern MCNameRef MCM_return_key;
extern MCNameRef MCM_save_stack_request;
extern MCNameRef MCM_script_error;
extern MCNameRef MCM_script_execution_error;
extern MCNameRef MCM_scrollbar_beginning;
extern MCNameRef MCM_scrollbar_drag;
extern MCNameRef MCM_scrollbar_end;
extern MCNameRef MCM_scrollbar_line_dec;
extern MCNameRef MCM_scrollbar_line_inc;
extern MCNameRef MCM_scrollbar_page_dec;
extern MCNameRef MCM_scrollbar_page_inc;
extern MCNameRef MCM_selected_object_changed;
extern MCNameRef MCM_selection_changed;
extern MCNameRef MCM_signal;
extern MCNameRef MCM_shut_down;
extern MCNameRef MCM_shut_down_request;
extern MCNameRef MCM_socket_error;
extern MCNameRef MCM_socket_closed;
extern MCNameRef MCM_socket_timeout;
extern MCNameRef MCM_start_up;
extern MCNameRef MCM_suspend;
extern MCNameRef MCM_suspend_stack;
extern MCNameRef MCM_tab_key;
extern MCNameRef MCM_text_changed;
extern MCNameRef MCM_trace;
extern MCNameRef MCM_trace_break;
extern MCNameRef MCM_trace_done;
extern MCNameRef MCM_trace_error;

extern MCNameRef MCM_undo_changed;
extern MCNameRef MCM_undo_key;
extern MCNameRef MCM_uniconify_stack;
extern MCNameRef MCM_unload_url;
extern MCNameRef MCM_update_var;
























































































































#line 24 "c:\\github\\livecode-runrev\\engine\\src\\globals.h"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\imagelist.h"















#line 1 "c:\\github\\livecode-runrev\\engine\\src\\dllst.h"



































































#line 17 "c:\\github\\livecode-runrev\\engine\\src\\imagelist.h"
#line 1 "c:\\github\\livecode-runrev\\libgraphics\\include\\graphics.h"



#line 1 "c:\\github\\livecode-runrev\\libcore\\include\\core.h"
















































































































































































































































































































































































































































































































































































































































#line 5 "c:\\github\\livecode-runrev\\libgraphics\\include\\graphics.h"



typedef struct __MCGContext *MCGContextRef;
typedef struct __MCGPath *MCGPathRef;
typedef struct __MCGImage *MCGImageRef;
typedef struct __MCGMask *MCGMaskRef;

typedef struct __MCGDashes *MCGDashesRef;
















typedef uint32_t MCGPixelFormat;



#line 35 "c:\\github\\livecode-runrev\\libgraphics\\include\\graphics.h"

#line 37 "c:\\github\\livecode-runrev\\libgraphics\\include\\graphics.h"

static inline uint32_t __MCGPixelPackComponents(uint8_t p_1, uint8_t p_2, uint8_t p_3, uint8_t p_4)
{
	return p_1 | (p_2 << 8) | (p_3 << 16) | (p_4 << 24);
}

static inline uint32_t MCGPixelPack(MCGPixelFormat p_format, uint8_t p_red, uint8_t p_green, uint8_t p_blue, uint8_t p_alpha)
{
	switch (p_format)
	{
		case ((1 << 0)):
			return __MCGPixelPackComponents(p_red, p_green, p_blue, p_alpha);
			
		case (0):
			return __MCGPixelPackComponents(p_blue, p_green, p_red, p_alpha);
			
		case ((1 << 1)):
			return __MCGPixelPackComponents(p_alpha, p_blue, p_green, p_red);
			
		case ((1 << 1) | (1 << 0)):
			return __MCGPixelPackComponents(p_alpha, p_red, p_green, p_blue);
	}
}

static inline void __MCGPixelUnpackComponents(uint32_t p_pixel, uint8_t &r_1, uint8_t &r_2, uint8_t &r_3, uint8_t &r_4)
{
	r_1 = p_pixel & 0xFF;
	r_2 = (p_pixel >> 8) & 0xFF;
	r_3 = (p_pixel >> 16) & 0xFF;
	r_4 = (p_pixel >> 24) & 0xFF;
}

static inline void MCGPixelUnpack(MCGPixelFormat p_format, uint32_t p_pixel, uint8_t &r_red, uint8_t &r_green, uint8_t &r_blue, uint8_t &r_alpha)
{
	switch (p_format)
	{
		case ((1 << 0)):
			__MCGPixelUnpackComponents(p_pixel, r_red, r_green, r_blue, r_alpha);
			break;
			
		case (0):
			__MCGPixelUnpackComponents(p_pixel, r_blue, r_green, r_red, r_alpha);
			break;
			
		case ((1 << 1)):
			__MCGPixelUnpackComponents(p_pixel, r_alpha, r_blue, r_green, r_red);
			break;
			
		case ((1 << 1) | (1 << 0)):
			__MCGPixelUnpackComponents(p_pixel, r_alpha, r_red, r_green, r_blue);
			break;
	}
}



typedef float MCGFloat;
typedef uint32_t MCGColor;

struct MCGPoint
{
	MCGFloat x, y;
};

struct MCGSize
{
	MCGFloat width, height;
};

struct MCGRectangle
{
	MCGPoint origin;
	MCGSize size;
};

struct MCGAffineTransform
{
	MCGFloat a, b, c, d;
	MCGFloat tx, ty;
};



enum MCGFillRule
{
	kMCGFillRuleNonZero,
	kMCGFillRuleEvenOdd
};

enum MCGBlendMode
{
	kMCGBlendModeClear,
	kMCGBlendModeCopy,
	kMCGBlendModeSourceOver,
	kMCGBlendModeSourceIn,
	kMCGBlendModeSourceOut,
	kMCGBlendModeSourceAtop,
	kMCGBlendModeDestinationOver,
	kMCGBlendModeDestinationIn,
	kMCGBlendModeDestinationOut,
	kMCGBlendModeDestinationAtop,
	kMCGBlendModeXor,
	kMCGBlendModePlusDarker,
	kMCGBlendModePlusLighter,
	kMCGBlendModeMultiply,
	kMCGBlendModeScreen,
	kMCGBlendModeOverlay,
	kMCGBlendModeDarken,
	kMCGBlendModeLighten,
	kMCGBlendModeColorDodge,
	kMCGBlendModeColorBurn,
	kMCGBlendModeSoftLight,
	kMCGBlendModeHardLight,
	kMCGBlendModeDifference,
	kMCGBlendModeExclusion,
	kMCGBlendModeHue,
	kMCGBlendModeSaturation,
	kMCGBlendModeColor,
	kMCGBlendModeLuminosity,
	
	kMCGBlendModeCount,
};

enum MCGJoinStyle
{
	kMCGJoinStyleBevel,
	kMCGJoinStyleRound,
	kMCGJoinStyleMiter,
};

enum MCGCapStyle
{
	kMCGCapStyleButt,
	kMCGCapStyleRound,
	kMCGCapStyleSquare
};

enum MCGRasterFormat
{
	kMCGRasterFormat_xRGB, 
	kMCGRasterFormat_ARGB, 
	kMCGRasterFormat_U_ARGB, 
	kMCGRasterFormat_A, 
};

enum MCGImageFilter
{
	kMCGImageFilterNearest,
	kMCGImageFilterBilinear,
	kMCGImageFilterBicubic,
};

enum MCGGradientFunction
{
	kMCGGradientFunctionLinear,
	kMCGGradientFunctionRadial,
	kMCGGradientFunctionConical,
	kMCGGradientFunctionSweep,
};

enum MCGGradientTileMode
{
	kMCGGradientTileModeClamp,
	kMCGGradientTileModeRepeat,
	kMCGGradientTileModeMirror,	
};

enum MCGMaskFormat
{
	kMCGMaskFormat_A1,
	kMCGMaskFormat_A8,
	kMCGMaskFormat_LCD32,
};

struct MCGRaster
{
	MCGRasterFormat format;
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	void *pixels;
};

struct MCGStrokeAttr
{
	MCGFloat width;
	MCGJoinStyle join_style;
	MCGCapStyle cap_style;
	MCGFloat miter_limit;
	MCGDashesRef dashes;
};

struct MCGLayerEffect
{
	MCGColor color;
	MCGBlendMode blend_mode;
};

struct MCGShadowEffect
{
	MCGColor color;
	MCGBlendMode blend_mode;
	MCGFloat size;
	MCGFloat x_offset;
	MCGFloat y_offset;
	bool knockout : 1;
};

struct MCGGlowEffect
{
	MCGColor color;
	MCGBlendMode blend_mode;
	MCGFloat size;
	bool inverted : 1;
};

struct MCGBitmapEffects
{
	bool has_color_overlay : 1;
	bool has_inner_glow : 1;
	bool has_inner_shadow : 1;
	bool has_outer_glow : 1;
	bool has_drop_shadow : 1;
	
	MCGLayerEffect color_overlay;
	MCGGlowEffect inner_glow;
	MCGShadowEffect inner_shadow;
	MCGGlowEffect outer_glow;
	MCGShadowEffect drop_shadow;
};

struct MCGDeviceMaskInfo
{
	MCGMaskFormat format;
	int32_t x, y, width, height;
	void *data;
};



inline MCGRectangle MCGRectangleMake(MCGFloat p_x, MCGFloat p_y, MCGFloat p_width, MCGFloat p_height)
{
	MCGRectangle t_rect;
	t_rect . origin . x = p_x;
	t_rect . origin . y = p_y;
	t_rect . size . width = p_width;
	t_rect . size . height = p_height;
	return t_rect;
}

inline MCGRectangle MCGRectangleTranslate(MCGRectangle p_rect, MCGFloat p_dx, MCGFloat p_dy)
{
	MCGRectangle t_rect = p_rect;
	t_rect.origin.x += p_dx;
	t_rect.origin.y += p_dy;
	
	return t_rect;
}

static inline MCGRectangle MCGRectangleScale(MCGRectangle p_rect, MCGFloat p_scale)
{
	MCGRectangle t_rect;
	t_rect.origin.x = p_rect.origin.x * p_scale;
	t_rect.origin.y = p_rect.origin.y * p_scale;
	t_rect.size.width = p_rect.size.width * p_scale;
	t_rect.size.height = p_rect.size.height * p_scale;
	
	return t_rect;
}

MCGRectangle MCGRectangleIntersection(MCGRectangle rect_1, MCGRectangle rect_2);

inline MCGPoint MCGPointMake(MCGFloat p_x, MCGFloat p_y)
{
	MCGPoint t_point;
	t_point . x = p_x;
	t_point . y = p_y;
	return t_point;
}




bool MCGImageCreateWithRaster(const MCGRaster& raster, MCGImageRef& r_image);


bool MCGImageCreateWithRasterNoCopy(const MCGRaster &raster, MCGImageRef &r_image);


bool MCGImageCreateWithRasterAndRelease(const MCGRaster &raster, MCGImageRef &r_image);

bool MCGImageCreateWithData(const void *bytes, uindex_t byte_count, MCGImageRef& r_image);
bool MCGImageCreateWithFilename(const char *filename, MCGImageRef& r_image);

bool MCGImageGetRaster(MCGImageRef image, MCGRaster &r_raster);

MCGImageRef MCGImageRetain(MCGImageRef image);
void MCGImageRelease(MCGImageRef image);

bool MCGImageIsValid(MCGImageRef image);

int32_t MCGImageGetWidth(MCGImageRef image);
int32_t MCGImageGetHeight(MCGImageRef image);

MCGSize MCImageGetSize(MCGImageRef image);



bool MCGMaskCreateWithInfoAndRelease(const MCGDeviceMaskInfo& info, MCGMaskRef& r_mask);
void MCGMaskRelease(MCGMaskRef mask);



typedef uint8_t MCGPathCommand;
enum
{
	kMCGPathCommandEnd,
	kMCGPathCommandMoveTo,
	kMCGPathCommandLineTo,
	kMCGPathCommandCurveTo,
	kMCGPathCommandQuadCurveTo,
	kMCGPathCommandCloseSubpath,
};

bool MCGPathCreate(const MCGPathCommand *commands, const MCGFloat *parameters, MCGPathRef& r_path);
bool MCGPathCreateMutable(MCGPathRef& r_path);

MCGPathRef MCGPathRetain(MCGPathRef path);
void MCGPathRelease(MCGPathRef path);

bool MCGPathIsValid(MCGPathRef path);

void MCGPathCopy(MCGPathRef path, MCGPathRef& r_new_path);
void MCGPathCopyAndRelease(MCGPathRef path, MCGPathRef& r_new_path);
void MCGPathMutableCopy(MCGPathRef path, MCGPathRef& r_new_path);
void MCGPathMutableCopyAndRelease(MCGPathRef path, MCGPathRef& r_new_path);

void MCGPathAddRectangle(MCGPathRef path, MCGRectangle bounds);
void MCGPathAddRoundedRectangle(MCGPathRef path, MCGRectangle bounds, MCGSize corner_radii);
void MCGPathAddEllipse(MCGPathRef path, MCGPoint center, MCGSize radii, MCGFloat rotation);
void MCGPathAddArc(MCGPathRef path, MCGPoint center, MCGSize radii, MCGFloat rotation, MCGFloat start_angle, MCGFloat finish_angle);
void MCGPathAddSector(MCGPathRef path, MCGPoint center, MCGSize radii, MCGFloat rotation, MCGFloat start_angle, MCGFloat finish_angle);
void MCGPathAddSegment(MCGPathRef path, MCGPoint center, MCGSize radii, MCGFloat rotation, MCGFloat start_angle, MCGFloat finish_angle);
void MCGPathAddLine(MCGPathRef path, MCGPoint start, MCGPoint finish);
void MCGPathAddPolygon(MCGPathRef path, const MCGPoint *points, uindex_t arity);
void MCGPathAddPolyline(MCGPathRef path, const MCGPoint *points, uindex_t arity);
void MCGPathAddPath(MCGPathRef self, MCGPathRef path);

void MCGPathMoveTo(MCGPathRef path, MCGPoint end_point);
void MCGPathLineTo(MCGPathRef path, MCGPoint end_point);
void MCGPathQuadraticTo(MCGPathRef path, MCGPoint control_point, MCGPoint end_point);
void MCGPathCubicTo(MCGPathRef path, MCGPoint first_control_point, MCGPoint second_control_point, MCGPoint end_point);
void MCGPathArcTo(MCGPathRef path, MCGSize radii, MCGFloat rotation, bool large_arc, bool sweep, MCGPoint end_point);
void MCGPathCloseSubpath(MCGPathRef path);

void MCGPathThicken(MCGPathRef path, const MCGStrokeAttr& attr, MCGPathRef& r_thick_path);
void MCGPathFlatten(MCGPathRef path, MCGFloat flatness, MCGPathRef& r_flat_path);
void MCGPathSimplify(MCGPathRef path, MCGPathRef& r_simple_path);



bool MCGContextCreate(uint32_t width, uint32_t height, bool alpha, MCGContextRef& r_context);
bool MCGContextCreateWithPixels(uint32_t width, uint32_t height, uint32_t stride, void *pixels, bool alpha, MCGContextRef& r_context);
bool MCGContextCreateWithRaster(const MCGRaster& raster, MCGContextRef& r_context);

MCGContextRef MCGContextRetain(MCGContextRef context);
void MCGContextRelease(MCGContextRef context);




bool MCGContextIsValid(MCGContextRef context);


void MCGContextSave(MCGContextRef context);
void MCGContextRestore(MCGContextRef context);


void MCGContextSetFlatness(MCGContextRef context, MCGFloat flatness);
void MCGContextSetShouldAntialias(MCGContextRef context, bool should_antialias);


void MCGContextSetOpacity(MCGContextRef context, MCGFloat opacity);
void MCGContextSetBlendMode(MCGContextRef context, MCGBlendMode mode);

void MCGContextBegin(MCGContextRef context);
void MCGContextBeginWithEffects(MCGContextRef context, const MCGBitmapEffects &effects);
void MCGContextEnd(MCGContextRef context);

void MCGContextClipToRect(MCGContextRef context, MCGRectangle rect);
MCGRectangle MCGContextGetDeviceClipBounds(MCGContextRef context);


void MCGContextSetFillRule(MCGContextRef context, MCGFillRule rule);
void MCGContextSetFillOpacity(MCGContextRef context, MCGFloat opacity);
void MCGContextSetFillRGBAColor(MCGContextRef context, MCGFloat red, MCGFloat green, MCGFloat blue, MCGFloat alpha);
void MCGContextSetFillPattern(MCGContextRef context, MCGImageRef image, MCGAffineTransform transform, MCGImageFilter filter);
void MCGContextSetFillGradient(MCGContextRef context, MCGGradientFunction function, const MCGFloat* stops, const MCGColor* colors, uindex_t ramp_length, bool mirror, bool wrap, uint32_t repeats, MCGAffineTransform transform, MCGImageFilter filter);


void MCGContextSetStrokeOpacity(MCGContextRef context, MCGFloat opacity);
void MCGContextSetStrokeRGBAColor(MCGContextRef context, MCGFloat red, MCGFloat green, MCGFloat blue, MCGFloat alpha);
void MCGContextSetStrokePattern(MCGContextRef context, MCGImageRef image, MCGAffineTransform transform, MCGImageFilter filter);
void MCGContextSetStrokeGradient(MCGContextRef context, MCGGradientFunction function, const MCGFloat* stops, const MCGColor* colors, uindex_t ramp_length, bool mirror, bool wrap, uint32_t repeats, MCGAffineTransform transform, MCGImageFilter filter);
void MCGContextSetStrokeWidth(MCGContextRef context, MCGFloat width);
void MCGContextSetStrokeMiterLimit(MCGContextRef context, MCGFloat limit);
void MCGContextSetStrokeJoinStyle(MCGContextRef context, MCGJoinStyle style);
void MCGContextSetStrokeCapStyle(MCGContextRef context, MCGCapStyle style);
void MCGContextSetStrokeDashes(MCGContextRef context, MCGFloat phase, const MCGFloat *lengths, uindex_t arity);


void MCGContextConcatCTM(MCGContextRef context, MCGAffineTransform transform);
void MCGContextRotateCTM(MCGContextRef context, MCGFloat angle);
void MCGContextTranslateCTM(MCGContextRef context, MCGFloat xoffset, MCGFloat yoffset);
void MCGContextScaleCTM(MCGContextRef context, MCGFloat xscale, MCGFloat yscale);
void MCGContextResetCTM(MCGContextRef context);
MCGAffineTransform MCGContextGetDeviceTransform(MCGContextRef context);


void MCGContextAddRectangle(MCGContextRef context, MCGRectangle bounds);
void MCGContextAddRoundedRectangle(MCGContextRef context, MCGRectangle bounds, MCGSize corner_radii);
void MCGContextAddEllipse(MCGContextRef context, MCGPoint center, MCGSize radii, MCGFloat rotation);
void MCGContextAddArc(MCGContextRef context, MCGPoint center, MCGSize radii, MCGFloat p_rotation, MCGFloat start_angle, MCGFloat finish_angle);
void MCGContextAddSector(MCGContextRef context, MCGPoint center, MCGSize radii, MCGFloat rotation, MCGFloat start_angle, MCGFloat finish_angle);
void MCGContextAddSegment(MCGContextRef context, MCGPoint center, MCGSize radii, MCGFloat rotation, MCGFloat start_angle, MCGFloat finish_angle);
void MCGContextAddLine(MCGContextRef context, MCGPoint start, MCGPoint finish);
void MCGContextAddPolygon(MCGContextRef context, const MCGPoint *points, uindex_t arity);
void MCGContextAddPolyline(MCGContextRef context, const MCGPoint *points, uindex_t arity);
void MCGContextAddRoundedPolygon(MCGContextRef context, const MCGPoint *points, uindex_t arity);
void MCGContextAddRoundedPolyline(MCGContextRef context, const MCGPoint *points, uindex_t arity);
void MCGContextAddDot(MCGContextRef context, MCGPoint location);
void MCGContextAddPath(MCGContextRef context, MCGPathRef path);


void MCGContextBeginPath(MCGContextRef context);
void MCGContextMoveTo(MCGContextRef context, MCGPoint end_point);
void MCGContextLineTo(MCGContextRef context, MCGPoint end_point);
void MCGContextQuadraticTo(MCGContextRef context, MCGPoint control_point, MCGPoint end_point);
void MCGContextCubicTo(MCGContextRef context, MCGPoint first_control_point, MCGPoint second_control_point, MCGPoint end_point);
void MCGContextArcTo(MCGContextRef context, MCGSize radii, MCGFloat rotation, bool large_arc, bool sweep, MCGPoint end_point);
void MCGContextCloseSubpath(MCGContextRef context);







void MCGContextCopyPath(MCGContextRef context, MCGPathRef& r_path);


void MCGContextFill(MCGContextRef context);


void MCGContextStroke(MCGContextRef context);



void MCGContextFillAndStroke(MCGContextRef context);


void MCGContextClip(MCGContextRef context);

void MCGContextThicken(MCGContextRef context);

void MCGContextFlatten(MCGContextRef context);


void MCGContextSimplify(MCGContextRef context);

void MCGContextDrawPixels(MCGContextRef context, const MCGRaster& raster, MCGRectangle dst_rect, MCGImageFilter filter);
void MCGContextDrawImage(MCGContextRef context, MCGImageRef image, MCGRectangle dst_rect, MCGImageFilter filter);
void MCGContextDrawRectOfImage(MCGContextRef self, MCGImageRef p_image, MCGRectangle p_src, MCGRectangle p_dst, MCGImageFilter p_filter);

void MCGContextDrawText(MCGContextRef context, const char* text, uindex_t length, MCGPoint location, uint32_t font_size);
MCGFloat MCGContextMeasureText(MCGContextRef context, const char *text, uindex_t length, uint32_t font_size);

void MCGContextDrawDeviceMask(MCGContextRef context, MCGMaskRef mask, int32_t tx, int32_t ty);

bool MCGContextCopyImage(MCGContextRef context, MCGImageRef &r_image);




MCGAffineTransform MCGAffineTransformMakeIdentity(void);
MCGAffineTransform MCGAffineTransformMakeRotation(MCGFloat p_angle);
MCGAffineTransform MCGAffineTransformMakeTranslation(MCGFloat p_xoffset, MCGFloat p_yoffset);
MCGAffineTransform MCGAffineTransformMakeScale(MCGFloat p_xscale, MCGFloat p_yscale);

MCGAffineTransform MCGAffineTransformConcat(const MCGAffineTransform& transform_1, const MCGAffineTransform& transform_2);
MCGAffineTransform MCGAffineTransformRotate(const MCGAffineTransform& transform, MCGFloat angle);
MCGAffineTransform MCGAffineTransformTranslate(const MCGAffineTransform& transform, MCGFloat xoffset, MCGFloat yoffset);
MCGAffineTransform MCGAffineTransformScale(const MCGAffineTransform& transform, MCGFloat xscale, MCGFloat yscale);
MCGAffineTransform MCGAffineTransformInvert(const MCGAffineTransform& transform);

MCGPoint MCGPointApplyAffineTransform(const MCGPoint& p_point, const MCGAffineTransform& p_transform);
MCGRectangle MCGRectangleApplyAffineTransform(const MCGRectangle& p_rect, const MCGAffineTransform& p_transform);



#line 538 "c:\\github\\livecode-runrev\\libgraphics\\include\\graphics.h"
#line 18 "c:\\github\\livecode-runrev\\engine\\src\\imagelist.h"

class MCImageListNode : public MCDLlist
{
	MCImage *source;
	MCGImageRef image;
	uint32_t refcount;

public:
	MCImageListNode(MCImage *isource);
	~MCImageListNode();
	
	
	
	
	bool allocimage(MCImage* source, MCGImageRef &r_image);
	bool freeimage(MCGImageRef image);
	bool unreferenced();
	
	MCImageListNode *next()
	{
		return (MCImageListNode *)MCDLlist::next();
	}
	MCImageListNode *prev()
	{
		return (MCImageListNode *)MCDLlist::prev();
	}
	void totop(MCImageListNode *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCImageListNode *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCImageListNode *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCImageListNode *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCImageListNode *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCImageListNode *remove(MCImageListNode *&list)
	{
		return (MCImageListNode *)MCDLlist::remove((MCDLlist *&)list);
	}
};

class MCImageList
{
	MCImageListNode *images;
public:
	MCImageList();
	~MCImageList();
	MCGImageRef allocpat(uint4 id, MCObject *optr);
	void freepat(MCGImageRef &pat);
};
#line 80 "c:\\github\\livecode-runrev\\engine\\src\\imagelist.h"
#line 25 "c:\\github\\livecode-runrev\\engine\\src\\globals.h"

typedef struct _Streamnode Streamnode;
typedef struct _Linkatts Linkatts;

extern int MCquit;




extern int MCquitisexplicit;

extern int MCidleRate;


extern Boolean MCaqua;
extern char *MCcmd;
extern char *MCfiletype;
extern char *MCstackfiletype;

extern Boolean MCuseXft ;
extern Boolean MCuselibgnome ;
extern Boolean MCuseESD ;











extern char **MCstacknames;

extern int2 MCnstacks;
extern Boolean MCnofiles;
extern Boolean MCmmap;
extern Boolean MCnopixmaps;
extern Boolean MCpointerfocus;
extern Boolean MCemacskeys;
extern Boolean MClowrestimers;
extern Boolean MCraisemenus;
extern Boolean MCsystemmodals;
extern Boolean MCactivatepalettes;
extern Boolean MChidepalettes;
extern Boolean MCraisepalettes;
extern Boolean MCproportionalthumbs;
extern Boolean MCdontuseNS;
extern Boolean MCdontuseQT;
extern Boolean MCdontuseQTeffects;
extern Boolean MCfreescripts;
extern uint4 MCeventtime;
extern uint2 MCbuttonstate;
extern uint2 MCmodifierstate;
extern uint2 MCextendkey;
extern int2 MCclicklocx;
extern int2 MCclicklocy;
extern int2 MCmousex;
extern int2 MCmousey;
extern uint2 MCsiguser1;
extern uint2 MCsiguser2;
extern int4 MCinputfd;
extern int4 MCshellfd;

extern MCCursorRef MCcursors[];
extern Boolean MCshm;
extern Boolean MCshmpix ;
extern Boolean MCvcshm;
extern Boolean MCmmap;
extern Boolean MCnoui;
extern char *MCdisplayname;
extern Boolean MCshmoff;
extern Boolean MCshmon;
extern uint4 MCvisualid;
extern real8 MCgamma;

extern MCColor MCzerocolor;
extern MCColor MConecolor;
extern MCColor MCpencolor;
extern char *MCpencolorname;
extern MCColor MCbrushcolor;
extern MCColor MChilitecolor;
extern MCColor MCgraycolor;
extern char *MCbrushcolorname;
extern uint4 MCpenpmid;
extern MCGImageRef MCpenpattern;
extern uint4 MCbrushpmid;
extern MCGImageRef MCbrushpattern;
extern uint4 MCbackdroppmid;
extern MCGImageRef MCbackdroppattern;
extern MCImageList *MCpatternlist;
extern MCColor MCaccentcolor;
extern char *MCaccentcolorname;
extern MCColor MChilitecolor;
extern char *MChilitecolorname;
extern MCColor MCselectioncolor;
extern char *MCselectioncolorname;
extern Linkatts MClinkatts;
extern Boolean MCrelayergrouped;
extern Boolean MCselectgrouped;
extern Boolean MCselectintersect;
extern MCRectangle MCwbr;
extern uint2 MCjpegquality;
extern uint2 MCpaintcompression;
extern uint2 MCrecordformat;
extern uint2 MCsoundchannel;
extern uint2 MCrecordsamplesize;
extern uint2 MCrecordchannels;
extern real8 MCrecordrate;
extern char MCrecordcompression[5];
extern char MCrecordinput[5];
extern Boolean MCuselzw;

extern real8 MCinfinity;
extern char *MCstackbottom;
extern Boolean MCcheckstack;
extern Boolean MCswapbytes;
extern Boolean MCtranslatechars;
extern Boolean MCdragging;
extern Streamnode *MCfiles;
extern Streamnode *MCprocesses;
extern MCSocket **MCsockets;
extern real8 MCsockettimeout;
extern real8 MCmaxwait;
extern uint2 MCnfiles;
extern uint2 MCnprocesses;
extern uint2 MCnsockets;
extern MCStack **MCusing;
extern uint2 MCnusing;
extern uint2 MCiconicstacks;
extern uint2 MCwaitdepth;
extern uint4 MCrecursionlimit;

extern Boolean MCownselection;
extern MCUndolist *MCundos;
extern MCSellist *MCselected;
extern MCStacklist *MCstacks;
extern MCStacklist *MCtodestroy;
extern MCObject *MCtodelete;
extern MCCardlist *MCrecent;
extern MCCardlist *MCcstack;
extern MCDispatch *MCdispatcher;
extern MCStack *MCtopstackptr;
extern MCStack *MCdefaultstackptr;
extern MCStack *MCstaticdefaultstackptr;
extern MCStack *MCmousestackptr;
extern MCStack *MCclickstackptr;
extern MCStack *MCfocusedstackptr;
extern MCObject *MCtargetptr;
extern MCObject *MCmenuobjectptr;
extern MCCard *MCdynamiccard;
extern Boolean MCdynamicpath;
extern MCObject *MCerrorptr;
extern MCObject *MCerrorlockptr;
extern MCGroup *MCsavegroupptr;
extern MCGroup *MCdefaultmenubar;
extern MCGroup *MCmenubar;
extern MCAudioClip *MCacptr;
extern MCPlayer *MCplayers;

extern MCStack *MCtemplatestack;
extern MCAudioClip *MCtemplateaudio;
extern MCVideoClip *MCtemplatevideo;
extern MCGroup *MCtemplategroup;
extern MCCard *MCtemplatecard;
extern MCButton *MCtemplatebutton;
extern MCGraphic *MCtemplategraphic;
extern MCEPS *MCtemplateeps;
extern MCScrollbar *MCtemplatescrollbar;
extern MCPlayer *MCtemplateplayer;
extern MCImage *MCtemplateimage;
extern MCField *MCtemplatefield;

extern MCImage *MCmagimage;
extern MCMagnify *MCmagnifier;
extern MCObject *MCdragsource;
extern MCObject *MCdragdest;
extern MCField *MCactivefield;
extern MCField *MCclickfield;
extern MCField *MCfoundfield;
extern MCField *MCdropfield;
extern int4 MCdropchar;
extern MCImage *MCactiveimage;
extern MCImage *MCeditingimage;
extern MCTooltip *MCtooltip;
extern MCStack *MCmbstackptr;

extern MCUIDC *MCscreen;
extern MCPrinter *MCprinter;
extern MCPrinter *MCsystemprinter;

extern char *MCscriptfont;
extern uint2 MCscriptsize;
extern uint2 MCfocuswidth;
extern uint2 MCsizewidth;
extern uint2 MCminsize;
extern uint2 MCcloneoffset;
extern uint2 MCtitlebarheight;
extern uint2 MCdoubledelta;
extern uint2 MCdoubletime;
extern uint2 MCdragdelta;
extern uint2 MCblinkrate;
extern uint2 MCrepeatrate;
extern uint2 MCrepeatdelay;
extern uint2 MCtyperate;
extern uint2 MCrefreshrate;
extern uint2 MCsyncrate;
extern uint2 MCeffectrate;
extern uint2 MCminstackwidth;
extern uint2 MCminstackheight;
extern uint2 MCerrorlimit;
extern uint2 MCscrollbarwidth;
extern uint2 MCwmwidth;
extern uint2 MCwmheight;
extern uint2 MCcharset;
extern uint2 MCet;
extern Boolean MCabortscript;
extern Boolean MCalarm;
extern Boolean MCallowinterrupts;
extern Boolean MCinterrupt;
extern Boolean MCexplicitvariables;
extern Boolean MCpreservevariables;
extern Boolean MCsystemFS;
extern Boolean MCsystemCS;
extern Boolean MCsystemPS;
extern Boolean MChidewindows;
extern Boolean MCbufferimages;
extern char *MCserialcontrolsettings;
extern char *MCshellcmd;
extern char *MCvcplayer;
extern char *MCbackdropcolor;

extern char *MCftpproxyhost;
extern uint2 MCftpproxyport;

extern char *MChttpproxy;

extern char *MClongdateformat;
extern char *MCshortdateformat;
extern char *MChttpheaders;
extern int4 MCrandomseed;
extern Boolean MCshowinvisibles;
extern MCObjectList *MCbackscripts;
extern MCObjectList *MCfrontscripts;


extern MCRectangle MCcur_effects_rect;
extern MCEffectList *MCcur_effects;
extern MCError *MCperror;
extern MCError *MCeerror;
extern MCVariable *MCmb;
extern MCVariable *MCeach;
extern MCVariable *MCresult;
extern MCVariable *MCurlresult;
extern MCVariable *MCglobals;
extern MCVariable *MCdialogdata;
extern char *MChcstat;
extern char *MCcurdir;
extern Boolean MCexitall;
extern int4 MCretcode;
extern Boolean MCrecording;



extern Boolean MCobjectpropertieschanged;

extern uint32_t MCpropertylistenerthrottletime;


extern Boolean MCmainstackschanged;



extern uint2 MClook;
extern const char *MCttbgcolor;
extern const char *MCttfont;
extern uint2 MCttsize;
extern uint2 MCtrylock;
extern uint2 MCerrorlock;
extern Boolean MCwatchcursor;
extern Boolean MClockcursor;
extern MCCursorRef MCcursor;
extern uint4 MCcursorid;
extern MCCursorRef MCdefaultcursor;
extern uint4 MCdefaultcursorid;
extern uint2 MCbusycount;
extern uint2 MClockscreen;
extern Boolean MClockcolormap;
extern Boolean MClockerrors;
extern Boolean MClockmenus;
extern Boolean MClockmessages;
extern Boolean MClockmoves;
extern Boolean MClockrecent;
extern Boolean MCtwelvetime;
extern Boolean MCuseprivatecmap;
extern Tool MCcurtool;
extern Tool MColdtool;
extern uint4 MCbrush;
extern uint4 MCspray;
extern uint4 MCeraser;
extern Boolean MCcentered;
extern Boolean MCfilled;
extern Boolean MCgrid;
extern uint2 MCgridsize;
extern uint2 MClinesize;
extern uint2 MCstartangle;
extern uint2 MCarcangle;
extern uint1 *MCdashes;
extern uint2 MCndashes;
extern uint2 MCroundradius;
extern Boolean MCmultiple;
extern uint2 MCmultispace;
extern uint4 MCpattern;
extern uint2 MCpolysides;
extern Boolean MCroundends;
extern uint2 MCslices;
extern uint2 MCmagnification;
extern uint2 MCdragspeed;
extern uint2 MCmovespeed;
extern uint2 MCtooltipdelay;
extern uint2 MCtooltime;
extern Boolean MClongwindowtitles;
extern Boolean MCblindtyping;
extern Boolean MCpowerkeys;
extern Boolean MCnavigationarrows;
extern Boolean MCtextarrows;
extern uint2 MCuserlevel;
extern Boolean MCusermodify;
extern Boolean MCinlineinput;
extern MCTheme *MCcurtheme;
extern Boolean MChidebackdrop;
extern Boolean MCraisewindows;
extern char *MCsslcertificates;
extern char *MCdefaultnetworkinterface;
extern uint4 MCstackfileversion;
extern uint4 MCmajorosversion;
extern Boolean MCantialiasedtextworkaround;
extern uint4 MCqtidlerate;

extern uint4 MCiconid;
extern char *MCiconmenu;
extern uint4 MCstatusiconid;
extern char *MCstatusiconmenu;
extern char *MCstatusicontooltip;








extern uint4 MCruntimebehaviour;

extern MCDragData *MCdragdata;
extern MCDragAction MCdragaction;
extern MCObject *MCdragtargetptr;
extern MCDragActionSet MCallowabledragactions;
extern uint4 MCdragimageid;
extern MCPoint MCdragimageoffset;

extern MCClipboardData *MCclipboarddata;
extern MCSelectionData *MCselectiondata;

extern uint4 MCsecuremode;




extern Boolean MCcursorcanbealpha;
extern Boolean MCcursorcanbecolor;
extern Boolean MCcursorbwonly;
extern int32_t MCcursormaxsize;



extern uint32_t MCpendingstacklimit;
extern uint32_t MCstacklimit;

extern Boolean MCappisactive;



extern MCPoint MCgroupedobjectoffset;



extern Boolean MCallowdatagrambroadcasts;



#line 418 "c:\\github\\livecode-runrev\\engine\\src\\globals.h"
#line 32 "c:\\github\\livecode-runrev\\engine\\src\\object.h"

enum {
    MAC_SHADOW,
    MAC_THUMB_TOP,
    MAC_THUMB_BACK,
    MAC_THUMB_BOTTOM,
    MAC_THUMB_GRIP,
    MAC_THUMB_HILITE,
    MAC_DISABLED,
    MAC_NCOLORS
};

typedef bool (*MCObjectListFontsCallback)(void *p_context, unsigned int p_index);
typedef bool (*MCObjectResetFontsCallback)(void *p_context, unsigned int p_index, unsigned int p_new_index);

enum MCVisitStyle
{
	VISIT_STYLE_DEPTH_FIRST,
	VISIT_STYLE_DEPTH_LAST
};

enum MCObjectIntersectType
{
	kMCObjectIntersectBounds,
	kMCObjectIntersectPixels,
	kMCObjectIntersectPixelsWithEffects,
};

enum MCObjectShapeType
{
	kMCObjectShapeEmpty,
	kMCObjectShapeRectangle,
	kMCObjectShapeMask,
	kMCObjectShapeComplex
};

enum MCPropertyChangedMessageType 
{
	kMCPropertyChangedMessageTypeNone = 0,
	kMCPropertyChangedMessageTypePropertyChanged = 1 << 0,
	kMCPropertyChangedMessageTypeResizeControlStarted = 1 << 1,
	kMCPropertyChangedMessageTypeResizeControlEnded = 1 << 2,
	kMCPropertyChangedMessageTypeGradientEditStarted = 1 << 3,
	kMCPropertyChangedMessageTypeGradientEditEnded = 1 << 4
};

struct MCObjectShape
{
	
	MCObjectShapeType type;
	
	MCRectangle bounds;
	struct
	{
		
		MCRectangle rectangle;
		
		
		struct
		{
			MCPoint origin;
			MCImageBitmap *bits;
		} mask;
	};
};

struct MCObjectRef
{
	MCObject *object;
	uint4 part;
};

struct MCObjectVisitor
{
	virtual ~MCObjectVisitor(void);
	
	virtual bool OnObject(MCObject *p_object);
	virtual bool OnControl(MCControl *p_control);

	virtual bool OnStack(MCStack *p_stack);
	virtual bool OnAudioClip(MCAudioClip *p_audio_clip);
	virtual bool OnVideoClip(MCVideoClip *p_video_clip);
	virtual bool OnCard(MCCard *p_card);
	virtual bool OnGroup(MCGroup *p_group);
	virtual bool OnField(MCField *p_field);
	virtual bool OnButton(MCButton *p_button);
	virtual bool OnImage(MCImage *p_image);
	virtual bool OnScrollbar(MCScrollbar *p_scrollbar);
	virtual bool OnPlayer(MCPlayer *p_player);
	virtual bool OnParagraph(MCParagraph *p_paragraph);
	virtual bool OnBlock(MCBlock *p_block);
	virtual bool OnStyledText(MCStyledText *p_styled_text);
};









class MCObjectHandle
{
public:
	MCObjectHandle(MCObject *p_object);
	~MCObjectHandle(void);

	
	void Retain(void);

	
	void Release(void);

	MCObject *Get(void);

	
	void Clear(void);

	
	bool Exists(void);

private:
	uint32_t m_references;
	MCObject *m_object;
};


struct MCObjectFontAttrs
{
	MCNameRef name;
	uint2 style;
	uint2 size;
};

class MCObject : public MCDLlist
{
protected:
	uint4 obj_id;
	MCObject *parent;
	MCNameRef _name;
	uint4 flags;
	MCRectangle rect;
	MCColor *colors;
	char **colornames;
	uint4 *patternids;
	MCGImageRef *patterns;
	char *script;
	MCHandlerlist *hlist;
	MCObjectPropertySet *props;
	uint4 state;
	uint2 fontheight;
	uint2 dflags;
	uint2 ncolors;
	uint2 npatterns;
	uint2 altid;
	uint1 hashandlers;
	uint1 scriptdepth;
	uint1 borderwidth;
	int1 shadowoffset;
	uint1 ink;
	uint1 extraflags;

	
	
	uint1 blendlevel;
	
	
	
	uint1 m_font_flags;
	
	
	bool m_listening : 1;
	uint8_t m_properties_changed : 6;
	
	
	bool m_in_id_cache : 1;
	
	
	bool m_script_encrypted : 1;
	
	char *tooltip;
	
	
	MCParentScriptUse *parent_script;

	
	MCObjectHandle *m_weak_handle;

	
	
	uint32_t opened;

	
	MCFontRef m_font;

	
	MCObjectFontAttrs *m_font_attrs;

	static uint1 dashlist[2];
	static uint1 dotlist[2];
	static int1 dashoffset;
	static MCRectangle selrect;
	static int2 startx;
	static int2 starty;
	static uint1 menudepth;
	static MCStack *attachedmenu;
	static MCColor maccolors[MAC_NCOLORS];

	
	
	
	static uint2 s_last_font_index;

	
	
	
	static bool s_loaded_parent_script_reference;
	
public:
	MCObject();
	MCObject(const MCObject &oref);
	virtual ~MCObject();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual bool visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor *p_visitor);

	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	virtual IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part);

	virtual IO_stat load(IO_handle stream, const char *version);
	virtual IO_stat extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length);
	
	virtual void open();
	virtual void close();
	virtual void kfocus();
	virtual Boolean kfocusnext(Boolean top);
	virtual Boolean kfocusprev(Boolean bottom);
	virtual void kunfocus();
	virtual Boolean kdown(const char *string, KeySym key);
	virtual Boolean kup(const char *string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void mfocustake(MCControl *target);
	virtual void munfocus();
	virtual void mdrag(void);
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	
	virtual void timer(MCNameRef mptr, MCParameter *params);
	virtual uint2 gettransient() const;
	virtual void setrect(const MCRectangle &nrect);

	
	virtual Exec_stat getprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat getarrayprop(uint4 parid, Properties which, MCExecPoint &, MCNameRef key, Boolean effective);
	virtual Exec_stat setprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat setarrayprop(uint4 parid, Properties which, MCExecPoint&, MCNameRef key, Boolean effective);

	virtual void select();
	virtual void deselect();
	virtual Boolean del();
	virtual void paste(void);
	virtual void undo(Ustruct *us);
	virtual void freeundo(Ustruct *us);
	virtual MCStack *getstack();

	
    
    
	virtual Exec_stat handle(Handler_type, MCNameRef, MCParameter *, MCObject *pass_from);
	virtual void closemenu(Boolean kfocus, Boolean disarm);
	virtual void recompute();
	
	
	virtual bool lockshape(MCObjectShape& r_shape);
	virtual void unlockshape(MCObjectShape& shape);

	
	virtual bool recomputefonts(MCFontRef parent_font);

	
	MCFontRef getfontref(void) const { return m_font; }

	const MCRectangle &getrect() const;
    virtual MCRectangle getrectangle(bool p_effective) const;

	
	
	virtual void relayercontrol(MCControl *source, MCControl *target);
	virtual void relayercontrol_remove(MCControl *control);
	virtual void relayercontrol_insert(MCControl *control, MCControl *target);

	MCNameRef getdefaultpropsetname(void);

	Exec_stat sendgetprop(MCExecPoint& ep, MCNameRef set_name, MCNameRef prop_name);
	Exec_stat getcustomprop(MCExecPoint& ep, MCNameRef set_name, MCNameRef prop_name);
	Exec_stat sendsetprop(MCExecPoint& ep, MCNameRef set_name, MCNameRef prop_name);
	Exec_stat setcustomprop(MCExecPoint& ep, MCNameRef set_name, MCNameRef prop_name);

	Exec_stat changeid(uint32_t new_id);

	uint4 getid() const
	{
		return obj_id;
	}
	uint4 getaltid() const
	{
		return altid;
	}
	void setid(uint4 inid)
	{
		obj_id = inid;
	}

	
	bool isunnamed(void) const
	{
		return MCNameIsEmpty(_name);
	}

	
	MCNameRef getname(void) const
	{
		return _name;
	}

	const char *getname_cstring(void) const
	{
		return MCNameGetCString(_name);
	}

	MCString getname_oldstring(void) const
	{
		return MCNameGetOldString(_name);
	}

	
	
	bool hasname(MCNameRef p_other_name);

	
	void setname(MCNameRef new_name);
	void setname_cstring(const char *p_new_name);
	void setname_oldstring(const MCString& p_new_name);

	uint1 getopened() const
	{
		return opened;
	}
	uint1 gethashandlers() const
	{
		return hashandlers;
	}
	Boolean getflag(uint4 flag) const
	{
		return (flags & flag) != 0;
	}
	Boolean getextraflag(uint4 flag) const
	{
		return (extraflags & flag) != 0;
	}
	uint4 getflags(void) const
	{
		return flags;
	}
	char *getscript(void)
	{
		return script;
	}
	MCHandlerlist *gethandlers(void)
	{
		return hlist;
	}

	uint32_t getopacity(void) { return blendlevel * 255 / 100; }
	uint32_t getink(void) { return ink; }

	void setflag(uint4 on, uint4 flag);
	void setextraflag(uint4 on, uint4 flag);

	Boolean getstate(uint4 flag) const
	{
		return (state & flag) != 0;
	}
	void setstate(Boolean on, uint4 newstate);

	Boolean isdisabled() const
	{
		return (flags & (1UL << 12)) != 0;
	}

	Exec_stat setsprop(Properties which, const MCString &);
	void help();

	Boolean getselected() const
	{
		return (state & (1UL << 3)) != 0;
	}

	bool isselectable(bool p_only_object = false) const;
	uint1 getborderwidth(void) {return borderwidth;}

	MCObject *getparent() const
	{
		return parent;
	}
	uint1 getscriptdepth() const
	{
		return scriptdepth;
	}
	void setparent(MCObject *newparent)
	{
		parent = newparent;
	}
	MCCard *getcard(uint4 cid = 0);
	Window getw();

	
	uint16_t getdashes(uint8_t *&r_dashlist, int8_t &r_dashoffset)
	{
		r_dashlist = dashlist;
		r_dashoffset = dashoffset;
		return 2;
	}
	
	
	
	
	MCParentScript *getparentscript(void) const;

	
	
	
	virtual bool resolveparentscript(void);
	
	
	
	
	MCImage *resolveimageid(uint4 image_id);
	MCImage *resolveimagename(const MCString& name);
	
	Boolean isvisible();
	Boolean resizeparent();
	Boolean getforecolor(uint2 di, Boolean reversed, Boolean hilite, MCColor &c,
	                     MCGImageRef &r_pattern, int2 &x, int2 &y, MCDC *dc, MCObject *o);
	void setforeground(MCDC *dc, uint2 di, Boolean rev, Boolean hilite = 0);
	Boolean setcolor(uint2 index, const MCString &eptr);
	Boolean setcolors(const MCString &data);
	Boolean setpattern(uint2 newpixmap, const MCString &);
	Boolean setpatterns(const MCString &data);
	Boolean getcindex(uint2 di, uint2 &i);
	uint2 createcindex(uint2 di);
	void destroycindex(uint2 di, uint2 i);
	Boolean getpindex(uint2 di, uint2 &i);
	uint2 createpindex(uint2 di);
	void destroypindex(uint2 di, uint2 i);
	MCColor *getcolors(void) {return colors;}
	void allowmessages(Boolean allow);

	
	
	uint32_t getcoloraspixel(uint2 di);
	
	
	
	void setfontattrs(const char *textfont, uint2 textsize, uint2 textstyle);

	
	
	void getfontattsnew(MCNameRef& fname, uint2& fsize, uint2& fstyle);
	void getfontattsnew(const char *& fname, uint2& fsize, uint2& fstyle);

	
	MCNameRef gettextfont(void);
	
	uint2 gettextheight(void);
	
	uint2 gettextsize(void);
	
	uint2 gettextstyle(void);

	
	bool hasunicode(void) const { return (m_font_flags & (1UL << 6)) != 0; }

	
	
	
	Exec_stat conditionalmessage(uint32_t p_flag, MCNameRef name);
	
	
	Exec_stat dispatch(Handler_type type, MCNameRef name, MCParameter *params);

	Exec_stat message(MCNameRef name, MCParameter *p = 0, Boolean changedefault = 1, Boolean send = 0, Boolean p_force = 0);
	Exec_stat message_with_args(MCNameRef name, const MCString &v1);
	Exec_stat message_with_args(MCNameRef name, const MCString &v1, const MCString &v2);
	Exec_stat message_with_args(MCNameRef name, const MCString &v1, const MCString &v2, const MCString& v3);
	Exec_stat message_with_args(MCNameRef name, const MCString &v1, const MCString &v2, const MCString& v3, const MCString& v4);
	Exec_stat message_with_args(MCNameRef name, MCNameRef v1);
	Exec_stat message_with_args(MCNameRef name, MCNameRef v1, MCNameRef v2);
	Exec_stat message_with_args(MCNameRef name, int4 v1);
	Exec_stat message_with_args(MCNameRef name, int4 v1, int4 v2);
	Exec_stat message_with_args(MCNameRef name, int4 v1, int4 v2, int4 v3);
	Exec_stat message_with_args(MCNameRef name, int4 v1, int4 v2, int4 v3, int4 v4);
	
	void senderror();
	void sendmessage(Handler_type htype, MCNameRef mess, Boolean handled);
	Exec_stat names(Properties which, MCExecPoint &ep, uint4 parid);
	Boolean parsescript(Boolean report, Boolean force = 0);
	void drawshadow(MCDC *dc, const MCRectangle &drect, int2 soffset);
	void draw3d(MCDC *dc, const MCRectangle &drect,
	            Etch style, uint2 bwidth);
	void drawborder(MCDC *dc, const MCRectangle &drect, uint2 bwidth);
	void positionrel(const MCRectangle &dptr, Object_pos xpos, Object_pos ypos);
	Exec_stat domess(const char *sptr);
	Exec_stat eval(const char *sptr, MCExecPoint &ep);
	void editscript();
	void removefrom(MCObjectList *l);
	Boolean attachmenu(MCStack *sptr);
	void alloccolors();

	int hashandler(Handler_type p_type, MCNameRef name);
	MCHandler *findhandler(Handler_type p_type, MCNameRef name);

	Exec_stat exechandler(MCHandler *p_handler, MCParameter *p_params);

	
	
	
	
	Exec_stat execparenthandler(MCHandler *p_handler, MCParameter *p_params, MCParentScriptUse *p_parentscript);

	
	
	
	Exec_stat handleself(Handler_type type, MCNameRef message, MCParameter* parameters);

	
	
	Exec_stat handleparent(Handler_type type, MCNameRef message, MCParameter* parameters);

	MCImageBitmap *snapshot(const MCRectangle *rect, const MCPoint *size, bool with_effects);

	
	
	
	
	Exec_stat getproparray(MCExecPoint &ep, uint4 parid, bool effective);

	MCObjectHandle *gethandle(void);

	
	bool intersects(MCObject *other, uint32_t threshold);
	
	
	
	bool needtosavefontrecord(void) const;

	
	
	static MCPickleContext *startpickling(bool include_2700);
	static void continuepickling(MCPickleContext *p_context, MCObject *p_object, uint4 p_part);
	static MCSharedString *stoppickling(MCPickleContext *p_context);

	static MCSharedString *pickle(MCObject *p_object, uint4 p_part);

	static MCObject *unpickle(MCSharedString *p_object, MCStack *p_stack);

	
	MCObject *next()
	{
		return (MCObject *)MCDLlist::next();
	}
	MCObject *prev()
	{
		return (MCObject *)MCDLlist::prev();
	}
	void totop(MCObject *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCObject *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCObject *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCObject *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCObject *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCObject *remove(MCObject *&list)
	{
		return (MCObject *)MCDLlist::remove((MCDLlist *&)list);
	}
	
	
	void listen(void)
	{
		m_listening = true;
	}
	
	void unlisten(void)
	{
		m_listening = false;
		m_properties_changed = kMCPropertyChangedMessageTypeNone;
	}
	
	inline void signallisteners(Properties which)
	{
		if (m_listening && which != P_CUSTOM_PROPERTY_SET)
		{
			m_properties_changed |= kMCPropertyChangedMessageTypePropertyChanged;
			MCobjectpropertieschanged = 1;
		}
	}
	
	inline void signallistenerswithmessage(uint8_t p_message)
	{
		if (m_listening)
		{
			m_properties_changed |= p_message;
			MCobjectpropertieschanged = 1;
		}
	}	
	
	uint8_t propertieschanged(void)
	{
		if (m_properties_changed != kMCPropertyChangedMessageTypeNone)
		{
			uint8_t t_properties_changed;
			t_properties_changed = m_properties_changed;
			m_properties_changed = kMCPropertyChangedMessageTypeNone;
			return t_properties_changed;
		}
		return kMCPropertyChangedMessageTypeNone;
	}	

	void scheduledelete(void);
	
	
	void setinidcache(bool p_value)
	{
		m_in_id_cache = p_value;
	}
	
	bool getinidcache(void)
	{
		return m_in_id_cache;
	}

	
	
	virtual bool imagechanged(MCImage *p_image, bool p_deleting)
	{
		return false;
	}

protected:
	IO_stat defaultextendedsave(MCObjectOutputStream& p_stream, uint4 p_part);
	IO_stat defaultextendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_remaining);

	IO_stat loadpropsets(IO_handle stream);
	IO_stat savepropsets(IO_handle stream);

	
	
	
	void loadfontattrs(uint2 index);
	
	
	void mapfont(void);
	void unmapfont(void);
	
private:
	Exec_stat getrectprop(Properties which, MCExecPoint& ep, Boolean effective);

	Exec_stat setrectprop(Properties which, MCExecPoint& ep, Boolean effective);
	Exec_stat setscriptprop(MCExecPoint& ep);
	Exec_stat setparentscriptprop(MCExecPoint& ep);
	Exec_stat setvisibleprop(uint4 parid, Properties which, MCExecPoint& ep);
	Exec_stat setshowfocusborderprop(MCExecPoint& ep);

	bool clonepropsets(MCObjectPropertySet*& r_new_props) const;
	void deletepropsets(void);

	
	bool findpropset(MCNameRef name, bool p_empty_is_default, MCVariableValue*& r_value);
	
	 bool ensurepropset(MCNameRef name, bool p_empty_is_default, MCVariableValue*& r_value);
	
	
	 bool setpropset(MCNameRef name);

	
	void listpropsets(MCExecPoint& ep);
	
	 bool changepropsets(MCExecPoint& ep);

	bool hasarraypropsets(void);
	uint32_t measurearraypropsets(void);
	IO_stat loadunnamedpropset(IO_handle stream);
	IO_stat saveunnamedpropset(IO_handle stream);
	IO_stat loadarraypropsets(MCObjectInputStream& stream);
	IO_stat savearraypropsets(MCObjectOutputStream& stream);

	
	void copyfontattrs(const MCObject& other);
	
	void clearfontattrs(void);
	
	uint2 savefontattrs(void);
	
	void setfontattrs(uint32_t which, MCNameRef textfont, uint2 textsize, uint2 textstyle);

	
	
	bool hasfontattrs(void) const;
	
	
	bool needtosavefontflags(void) const;

	
	MCImage *resolveimage(const MCString& name, uint4 image_id);
	
	Exec_stat mode_getprop(uint4 parid, Properties which, MCExecPoint &, const MCString &carray, Boolean effective);

	friend class MCObjectHandle;
	friend class MCEncryptedStack;
};

class MCObjectList : public MCDLlist
{
protected:
	MCObject *object;
	Boolean removed;
public:
	MCObjectList(MCObject *optr)
	{
		object = optr;
		removed = 0;
	}
	MCObject *getobject()
	{
		return object;
	}
	Boolean getremoved()
	{
		return removed;
	}
	void setremoved(Boolean r)
	{
		removed = r;
	}
	MCObjectList *next()
	{
		return (MCObjectList *)MCDLlist::next();
	}
	MCObjectList *prev()
	{
		return (MCObjectList *)MCDLlist::prev();
	}
	void insertto(MCObjectList *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCObjectList *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	MCObjectList *remove(MCObjectList *&list)
	{
		return (MCObjectList *)MCDLlist::remove((MCDLlist *&)list);
	}
};
#line 816 "c:\\github\\livecode-runrev\\engine\\src\\object.h"
#line 25 "c:\\github\\livecode-runrev\\engine\\src\\control.h"




enum MCLayerModeHint
{
	
	
	kMCLayerModeHintStatic,
	
	
	kMCLayerModeHintDynamic,
	
	
	kMCLayerModeHintScrolling,
	
	
	kMCLayerModeHintContainer
};

class MCControl : public MCObject
{
protected:
	int2 mx;
	int2 my;
	int2 leftmargin;
	int2 rightmargin;
	int2 topmargin;
	int2 bottommargin;
	
	MCBitmapEffectsRef m_bitmap_effects;

	
	uint32_t m_layer_id;
	
	
	
	bool m_layer_attr_changed : 1;
	
	MCLayerModeHint m_layer_mode_hint : 3;
	
	
	MCLayerModeHint m_layer_mode : 3;
	
	
	
	bool m_layer_is_toplevel : 1;
	
	
	bool m_layer_is_opaque : 1;
	
	
	bool m_layer_is_direct : 1;
	
	
	bool m_layer_is_unadorned : 1;
	
	
	bool m_layer_is_sprite : 1;

	static int2 defaultmargin;
	static int2 xoffset;
	static int2 yoffset;
	static MCControl *focused;
	static double aspect;

public:
	MCControl();
	MCControl(const MCControl &cref);
	~MCControl();

	
	virtual void open();
	virtual void close();
	virtual void kfocus();
	virtual void kunfocus();
	virtual Boolean kdown(const char *string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void munfocus();
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void timer(MCNameRef mptr, MCParameter *params);
	virtual uint2 gettransient() const;

	
	virtual Exec_stat getprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat getarrayprop(uint4 parid, Properties which, MCExecPoint &, MCNameRef key, Boolean effective);
	virtual Exec_stat setprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat setarrayprop(uint4 parid, Properties which, MCExecPoint&, MCNameRef key, Boolean effective);

	virtual void select();
	virtual void deselect();
	virtual Boolean del();
	virtual void paste(void);

	virtual void undo(Ustruct *us);

	
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite) = 0;

	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	virtual Boolean kfocusset(MCControl *target);
	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);
	virtual MCControl *findnum(Chunk_term type, uint2 &num);
	virtual MCControl *findname(Chunk_term type, const MCString &);
	virtual MCControl *findid(Chunk_term type, uint4 inid, Boolean alt);
	virtual Boolean count(Chunk_term otype, MCObject *stop, uint2 &num);
	virtual Boolean maskrect(const MCRectangle &srect);
	virtual void installaccels(MCStack *stack);
	virtual void removeaccels(MCStack *stack);
	virtual MCCdata *getdata(uint4 cardid, Boolean clone);
	virtual void replacedata(MCCdata *&data, uint4 newid);
	virtual void compactdata();
	virtual void resetfontindex(MCStack *oldstack);
	virtual Exec_stat hscroll(int4 offset, Boolean doredraw);
	virtual Exec_stat vscroll(int4 offset, Boolean doredraw);
	virtual void readscrollbars();
	virtual void setsbrects();
	virtual void resetscrollbars(Boolean move);
	virtual void fliph();
	virtual void flipv();
	virtual void getwidgetthemeinfo(MCWidgetInfo &widgetinfo);
	virtual void unlink(MCControl *p_control);
	
	virtual MCObject *hittest(int32_t x, int32_t y);

	IO_stat save_extended(IO_handle p_stream, const MCString& p_data, uint4 p_part);

	
	void attach(Object_pos p, bool invisible);

	void redraw(MCDC *dc, const MCRectangle &dirty);

	void sizerects(MCRectangle *rects);
	void drawselected(MCDC *dc);
	void drawarrow(MCDC *dc, int2 x, int2 y, uint2 size,
	               Arrow_direction dir, Boolean border, Boolean hilite);
	void continuesize(int2 x, int2 y);
	uint2 sizehandles();
	void start(Boolean canclone);
	void end(bool p_send_mouse_up = true);
	void create(int2 x, int2 y);
	Boolean moveable();
	void newmessage();
	void enter();
	void leave();
	void hblt(MCRectangle &drect, int4 offset);
	void vblt(MCRectangle &drect, int4 offset);
	Boolean sbfocus(int2 x, int2 y, MCScrollbar *hsb, MCScrollbar *vsb);
	Boolean sbdown(uint2 which, MCScrollbar *hsb, MCScrollbar *vsb);
	Boolean sbup(uint2 which, MCScrollbar *hsb, MCScrollbar *vsb);
	Boolean sbdoubledown(uint2 which, MCScrollbar *hsb, MCScrollbar *vsb);
	Boolean sbdoubleup(uint2 which, MCScrollbar *hsb, MCScrollbar *vsb);
	Exec_stat setsbprop(Properties which, const MCString &data, int4 tx, int4 ty,
	                    uint2 &sbw, MCScrollbar *&hsb, MCScrollbar *&vsb,
	                    Boolean &dirty);

	void drawfocus(MCDC* p_dc, const MCRectangle& p_dirty);

	void grab();
	void ungrab(uint2 which);

	
	
	
	virtual MCRectangle geteffectiverect(void) const;

	
	MCBitmapEffectsRef getbitmapeffects(void);
	void setbitmapeffects(MCBitmapEffectsRef bitmap_effects);

	
	
	MCRectangle computeeffectsrect(const MCRectangle& area) const;

	
	void layer_redrawall(void);
	
	void layer_redrawrect(const MCRectangle& rect);
	
	void layer_transientchangedandredrawall(int32_t old_transient);
	
	void layer_setrect(const MCRectangle& new_rect, bool redrawall);
	
	void layer_rectchanged(const MCRectangle& old_rect, bool redrawall);
	
	void layer_effectiverectchangedandredrawall(const MCRectangle& old_effective_rect);
	
	void layer_effectschanged(const MCRectangle& old_effective_rect);
	
	void layer_contentoriginchanged(int32_t dx, int32_t dy);
	
	void layer_visibilitychanged(const MCRectangle& old_effective_rect);
	
	void layer_scrolled(void);
	
	
	
	void layer_dirtyeffectiverect(const MCRectangle& effective_rect, bool update_card);
	
	
	void layer_changeeffectiverect(const MCRectangle& old_effective_rect, bool force_update, bool update_card);
	
	
	void layer_dirtycontentrect(const MCRectangle& content_rect, bool update_card);

	
	uint32_t layer_getid(void) { return m_layer_id; }
	
	void layer_setid(uint32_t p_id) { m_layer_id = p_id; }

	
	MCLayerModeHint layer_getmodehint(void) { return m_layer_mode_hint; }
	
	void layer_setmodehint(MCLayerModeHint p_mode) { m_layer_mode_hint = p_mode; m_layer_attr_changed = true; }
	
	MCLayerModeHint layer_geteffectivemode(void) { return layer_computeattrs(false); }
	
	MCRectangle layer_getcontentrect(void);

	
	bool layer_issprite(void) { return m_layer_is_sprite; }
	
	bool layer_isscrolling(void) { return m_layer_mode == kMCLayerModeHintScrolling; }
	
	bool layer_isopaque(void) { return m_layer_is_opaque; }

	
	MCLayerModeHint layer_computeattrs(bool commit);
	
	void layer_resetattrs(void);

	static MCControl *getfocused()
	{
		return focused;
	}

	uint2 getstyle()
	{
		return getstyleint(flags);
	}

	uint2 getleftmargin() const
	{
		return leftmargin;
	}

	uint2 getrightmargin() const
	{
		return rightmargin;
	}

	MCControl *next()
	{
		return (MCControl *)MCDLlist::next();
	}

	MCControl *prev()
	{
		return (MCControl *)MCDLlist::prev();
	}

	void totop(MCControl *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}

	void insertto(MCControl *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}

	void appendto(MCControl *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}

	void append(MCControl *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}

	void splitat(MCControl *node)
	{
		MCDLlist::splitat((MCDLlist *)node);
	}

	MCControl *remove(MCControl *&list)
	{
		return (MCControl *)MCDLlist::remove((MCDLlist *&)list);
	}
};
#line 318 "c:\\github\\livecode-runrev\\engine\\src\\control.h"
#line 24 "c:\\github\\livecode-runrev\\engine\\src\\image.h"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\imagebitmap.h"














































































































#line 25 "c:\\github\\livecode-runrev\\engine\\src\\image.h"
#line 1 "c:\\github\\livecode-runrev\\libgraphics\\include\\graphics.h"

























































































































































































































































































































































































































































































































































#line 26 "c:\\github\\livecode-runrev\\engine\\src\\image.h"





typedef struct
{
	int2 y;
	int2 lx;
	int2 rx;
	int2 direction;
} MCstacktype;

typedef struct
{
	MCGImageRef image;
	int32_t xhot;
	int32_t yhot;
} MCBrush;



bool MCImageBitmapApplyColorTransform(MCImageBitmap *p_bitmap, MCColorTransformRef p_transform);

bool MCImageQuantizeImageBitmap(MCImageBitmap *p_bitmap, MCColor *p_colors, uindex_t p_color_count, bool p_dither, bool p_add_transparency_index, MCImageIndexedBitmap *&r_indexed);

bool MCImageQuantizeColors(MCImageBitmap *p_bitmap, MCImagePaletteSettings *p_palette_settings, bool p_dither, bool p_transparency_index, MCImageIndexedBitmap *&r_indexed);

bool MCImageScaleBitmap(MCImageBitmap *p_src_bitmap, uindex_t p_width, uindex_t p_height, uint8_t p_quality, MCImageBitmap *&r_scaled);
bool MCImageRotateBitmap(MCImageBitmap *p_src, real64_t p_angle, uint8_t p_quality, uint32_t p_backing_color, MCImageBitmap *&r_rotated);


bool MCImageEncodeGIF(MCImageBitmap *p_image, IO_handle p_stream, bool p_dither, uindex_t &r_bytes_written);
bool MCImageEncodeGIF(MCImageIndexedBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written);
bool MCImageDecodeGIF(IO_handle p_stream, MCImageFrame *&r_frames, uindex_t &r_frame_count);

bool MCImageEncodeJPEG(MCImageBitmap *p_image, IO_handle p_stream, uindex_t &r_bytes_written);
bool MCImageDecodeJPEG(IO_handle p_stream, MCImageBitmap *&r_image);

bool MCImageEncodePNG(MCImageBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written);
bool MCImageEncodePNG(MCImageIndexedBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written);
bool MCImageDecodePNG(IO_handle p_stream, MCImageBitmap *&r_bitmap);

bool MCImageEncodeBMP(MCImageBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written);
bool MCImageDecodeBMPStruct(IO_handle p_stream, uindex_t &x_bytes_read, MCImageBitmap *&r_bitmap);
bool MCImageDecodeBMP(IO_handle p_stream, MCPoint &r_hotspot, MCImageBitmap *&r_bitmap);

bool MCImageEncodeRawTrueColor(MCImageBitmap *p_bitmap, IO_handle p_stream, Export_format p_format, uindex_t &r_bytes_written);
bool MCImageEncodeRawIndexed(MCImageBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written);
bool MCImageEncodeRawIndexed(MCImageIndexedBitmap *p_indexed, IO_handle p_stream, uindex_t &r_bytes_written);

bool MCImageEncodePBM(MCImageBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written);
bool MCImageEncodePPM(MCImageBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written);
bool MCImageDecodeNetPBM(IO_handle p_stream, MCImageBitmap *&r_bitmap);

bool MCImageDecodeXBM(IO_handle p_stream, MCPoint &r_hotspot, char *&r_name, MCImageBitmap *&r_bitmap);
bool MCImageDecodeXPM(IO_handle p_stream, MCImageBitmap *&r_bitmap);
bool MCImageDecodeXWD(IO_handle stream, char *&r_name, MCImageBitmap *&r_bitmap);


void MCImageBitmapSetAlphaValue(MCImageBitmap *p_bitmap, uint8_t p_alpha);



struct MCImageCompressedBitmap
{
	uint32_t compression;
	uint8_t *data;
	uindex_t size;

	
	uindex_t width, height, depth;
	MCColor *colors;
	uindex_t color_count;
	uint8_t *mask;
	uindex_t mask_size;

	uint8_t **planes;
	uindex_t *plane_sizes;
};

bool MCImageCreateCompressedBitmap(uint32_t p_compression, MCImageCompressedBitmap *&r_compressed);
bool MCImageCopyCompressedBitmap(MCImageCompressedBitmap *p_src, MCImageCompressedBitmap *&r_dst);
void MCImageFreeCompressedBitmap(MCImageCompressedBitmap *p_compressed);

bool MCImageCompressRLE(MCImageBitmap *p_bitmap, MCImageCompressedBitmap *&r_compressed);
bool MCImageCompressRLE(MCImageIndexedBitmap *p_indexed, MCImageCompressedBitmap *&r_compressed);
bool MCImageDecompressRLE(MCImageCompressedBitmap *p_compressed, MCImageBitmap *&r_bitmap);

bool MCImageCompress(MCImageBitmap *p_bitmap, bool p_dither, MCImageCompressedBitmap *&r_compressed);
bool MCImageDecompress(MCImageCompressedBitmap *p_compressed, MCImageFrame *&r_frames, uindex_t &r_frame_count);

bool MCImageGetMetafileGeometry(IO_handle p_stream, uindex_t &r_width, uindex_t &r_height);
bool MCImageImport(IO_handle p_stream, IO_handle p_mask_stream, MCPoint &r_hotspot, char *&r_name, MCImageCompressedBitmap *&r_compressed, MCImageBitmap *&r_bitmap);
bool MCImageExport(MCImageBitmap *p_bitmap, Export_format p_format, MCImagePaletteSettings *p_palette_settings, bool p_dither, IO_handle p_stream, IO_handle p_mask_stream);

bool MCImageDecode(IO_handle p_stream, MCImageFrame *&r_frames, uindex_t &r_frame_count);
bool MCImageDecode(const uint8_t *p_data, uindex_t p_size, MCImageFrame *&r_frames, uindex_t &r_frame_count);

bool MCImageCreateClipboardData(MCImageBitmap *p_bitmap, MCSharedString *&r_data);




bool MCImageBitmapToDIB(MCImageBitmap *p_bitmap, MCWinSysHandle &r_dib);
bool MCImageBitmapToV5DIB(MCImageBitmap *p_bitmap, MCWinSysHandle &r_dib);
bool MCImageBitmapToMetafile(MCImageBitmap *p_bitmap, MCWinSysMetafileHandle &r_metafile);
bool MCImageBitmapToEnhancedMetafile(MCImageBitmap *p_bitmap, MCWinSysEnhMetafileHandle &r_metafile);


#line 137 "c:\\github\\livecode-runrev\\engine\\src\\image.h"

#line 1 "c:\\github\\livecode-runrev\\engine\\src\\image_rep.h"



















typedef enum
{
	kMCImageRepUnknown,

	kMCImageRepMutable,
	kMCImageRepReferenced,
	kMCImageRepResident,
	kMCImageRepVector,
	kMCImageRepCompressed,
	
	kMCImageRepTransformed,
} MCImageRepType;




class MCImageRep
{
public:
	MCImageRep();
	virtual ~MCImageRep();

	virtual MCImageRepType GetType() = 0;
	virtual uindex_t GetFrameCount() = 0;
	virtual bool LockImageFrame(uindex_t p_index, bool p_premultiplied, MCImageFrame *&r_frame) = 0;
	virtual void UnlockImageFrame(uindex_t p_index, MCImageFrame *p_frame) = 0;
	virtual bool GetGeometry(uindex_t &r_width, uindex_t &r_height) = 0;

	

	MCImageRep *Retain();
	void Release();

private:
	uindex_t m_reference_count;
};




class MCCachedImageRep : public MCImageRep
{
public:
	MCCachedImageRep();
	virtual ~MCCachedImageRep();

	virtual MCImageRepType GetType() = 0;

	virtual uindex_t GetFrameCount();
	virtual bool LockImageFrame(uindex_t p_index, bool p_premultiplied, MCImageFrame *&r_frame);
	virtual void UnlockImageFrame(uindex_t p_index, MCImageFrame *p_frame);

	virtual bool GetGeometry(uindex_t &r_width, uindex_t &r_height);

	

	uint32_t GetFrameByteCount();
	void ReleaseFrames();

	

	static void init();
	static void AddRep(MCCachedImageRep *p_rep);
	static void RemoveRep(MCCachedImageRep *p_rep);
	static void MoveRepToHead(MCCachedImageRep *p_rep);

	static bool FindReferencedWithFilename(const char *p_filename, MCCachedImageRep *&r_rep);

	static void FlushCache();
	static void FlushCacheToLimit();
	
	static uint32_t GetCacheUsage() { return s_cache_size; }
	static void SetCacheLimit(uint32_t p_limit)	{ s_cache_limit = p_limit; }
	static uint32_t GetCacheLimit() { return s_cache_limit; }
	

protected:
	virtual bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height) = 0;
	virtual bool LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count) = 0;

	bool m_have_geometry;
	uindex_t m_width, m_height;

	bool m_premultiplied;

private:
	bool EnsureImageFrames(bool p_premultiplied);
	void PremultiplyFrames();
	
	uindex_t m_lock_count;

	MCImageFrame *m_frames;
	uindex_t m_frame_count;

	

	MCCachedImageRep *m_next;
	MCCachedImageRep *m_prev;

	static MCCachedImageRep *s_head;
	static MCCachedImageRep *s_tail;

	

	static uint32_t s_cache_size;
	static uint32_t s_cache_limit;
};




class MCEncodedImageRep : public MCCachedImageRep
{
public:
	MCEncodedImageRep()
	{
		m_compression = (0UL << 17);
	}

	virtual ~MCEncodedImageRep();

	uint32_t GetDataCompression();

protected:
	
	bool LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count);
	bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height);

	

	
	virtual bool GetDataStream(IO_handle &r_stream) = 0;

	

	uint32_t m_compression;
};



class MCReferencedImageRep : public MCEncodedImageRep
{
public:
	MCReferencedImageRep(const char *p_filename);
	~MCReferencedImageRep();

	MCImageRepType GetType() { return kMCImageRepReferenced; }

	

	const char *GetFilename()
	{
		return m_file_name;
	}

	

protected:
	
	bool GetDataStream(IO_handle &r_stream);

	char *m_file_name;

	
	void *m_url_data;
	uindex_t m_url_data_size;
	
	
	
	bool m_url_load_attempted : 1;
};



class MCResidentImageRep : public MCEncodedImageRep
{
public:
	MCResidentImageRep(const void *p_data, uindex_t p_size);
	~MCResidentImageRep();

	MCImageRepType GetType() { return kMCImageRepResident; }

	

	void GetData(void *&r_data, uindex_t &r_size)
	{
		r_data = m_data;
		r_size = m_size;
	}

protected:
	
	bool GetDataStream(IO_handle &r_stream);

	void *m_data;
	uindex_t m_size;
};



class MCVectorImageRep : public MCCachedImageRep
{
public:
	MCVectorImageRep(const void *p_data, uindex_t p_size);
	~MCVectorImageRep();

	MCImageRepType GetType() { return kMCImageRepVector; }

	uindex_t GetFrameCount() { return 1; }

	

	void GetData(void *&r_data, uindex_t &r_size)
	{
		r_data = m_data, r_size = m_size;
	}

	bool Render(MCDC *p_context, bool p_embed, MCRectangle &p_image_rect, MCRectangle &p_clip_rect);

protected:
	bool LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count);
	bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height);

	

	void *m_data;
	uindex_t m_size;
};



class MCCompressedImageRep : public MCCachedImageRep
{
public:
	MCCompressedImageRep(MCImageCompressedBitmap *p_bitmap);
	~MCCompressedImageRep();

	MCImageRepType GetType() { return kMCImageRepCompressed; }

	uindex_t GetFrameCount() { return 1; }

	

	MCImageCompressedBitmap *GetCompressed()
	{
		return m_compressed;
	}

protected:
	bool LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count);
	bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height);

	

	MCImageCompressedBitmap *m_compressed;
};



bool MCImageRepGetReferenced(const char *p_filename, MCImageRep *&r_rep);
bool MCImageRepGetResident(void *p_data, uindex_t p_size, MCImageRep *&r_rep);
bool MCImageRepGetVector(void *p_data, uindex_t p_size, MCImageRep *&r_rep);
bool MCImageRepGetCompressed(MCImageCompressedBitmap *p_compressed, MCImageRep *&r_rep);



#line 287 "c:\\github\\livecode-runrev\\engine\\src\\image_rep.h"
#line 139 "c:\\github\\livecode-runrev\\engine\\src\\image.h"

class MCMutableImageRep : public MCImageRep
{
public:
	MCMutableImageRep(MCImage *p_owner, MCImageBitmap *p_bitmap);
	~MCMutableImageRep();

	
	virtual MCImageRepType GetType() { return kMCImageRepMutable; }
	virtual uindex_t GetFrameCount();
	virtual bool LockImageFrame(uindex_t p_index, bool p_premultiplied, MCImageFrame *&r_frame);
	virtual void UnlockImageFrame(uindex_t p_index, MCImageFrame *p_frame);
	virtual bool GetGeometry(uindex_t &r_width, uindex_t &r_height);

	
	bool copy_selection(MCImageBitmap *&r_bitmap);

	Boolean image_mfocus(int2 x, int2 y);
	Boolean image_mdown(uint2 which);
	Boolean image_mup(uint2 which);
	Boolean image_doubledown(uint2 which);
	Boolean image_doubleup(uint2 which);

	bool has_selection();

	void drawsel(MCDC *dc);
	void drawselrect(MCDC *dc);

	void image_undo(Ustruct *p_undo);
	void image_freeundo(Ustruct *p_undo);

	void startdraw();
	void continuedraw();
	void enddraw();

	void canceldraw();

	void startrub();
	MCRectangle continuerub(Boolean line);
	void endrub();

	void startseldrag();
	void endsel();

	void put_brush(int2 x, int2 y, MCBrush *which);
	void fill_line(MCGRaster &plane, int2 left, int2 right, int2 y);
	bool bucket_line(MCImageBitmap *simage, uint4 color,
	                    int2 x, int2 y, int2 &l, int2 &r);
	bool bucket_point(MCImageBitmap *simage, uint4 color, MCGRaster &dimage,
	                     MCstacktype pstack[], uint2 &pstacktop,
	                     uint2 &pstackptr, int2 xin, int2 yin, int2 direction,
	                     int2 &xleftout, int2 &xrightout, bool &collide);
	void bucket_fill(MCImageBitmap *simage, uint4 scolor, MCGRaster &dimage,
	                 int2 xleft, int2 oldy);

	MCRectangle drawbrush(Tool which);
	void drawbucket();
	MCRectangle drawline(Boolean cancenter);
	MCRectangle drawreg();
	MCRectangle drawroundrect();
	MCRectangle drawoval();
	MCRectangle drawpencil();
	MCRectangle drawrectangle();

	void fill_path(MCGPathRef p_path);
	void stroke_path(MCGPathRef p_path);
	void draw_path(MCGPathRef p_path);
	void apply_stroke_style(MCGContextRef p_context, bool p_miter);
	void apply_fill_paint(MCGContextRef p_context, MCGImageRef p_pattern, const MCColor &p_color);
	void apply_stroke_paint(MCGContextRef p_context, MCGImageRef p_pattern, const MCColor &p_color);

	void battson(MCContext *ctxt, uint2 depth);

	void fillimage(const MCRectangle &drect);
	void eraseimage(const MCRectangle &drect);

	MCRectangle getopaqueregion(uint1 p_threshold = 0);
	void croptoopaque();

	void selimage();

	void getsel(Boolean cut);
	void cutoutsel();
	void stampsel();
	void rotatesel(int2 angle);
	void flipsel(Boolean ishorizontal);

	void pasteimage(MCImageBitmap *p_bitmap);

	static void init();
	static void shutdown();

private:
	MCImage *m_owner;
	MCImageFrame m_frame;

	MCImageBitmap *m_bitmap;
	MCImageBitmap *m_unpre_bitmap;
	MCImageBitmap *m_selection_image;
	MCImageBitmap *m_undo_image;
	MCImageBitmap *m_rub_image;
	MCGRaster m_draw_mask;

	 MCRectangle rect;

	MCRectangle selrect;

	uint32_t state;

	int16_t mx, my;
	int16_t startx, starty;

	static Boolean erasing;

	static Tool oldtool;
	static MCRectangle newrect;

	static MCPoint *points;
	static uint2 npoints;
	static uint2 polypoints;
};

class MCImageNeed
{
public:
	MCImageNeed(MCObject *p_object);
	~MCImageNeed();

	void Add(MCImageNeed *&p_list);
	void Remove(MCImageNeed *&p_list);

	MCObject *GetObject();
	MCImageNeed *GetNext();

private:
	MCObjectHandle *m_object;
	MCImageNeed *m_prev;
	MCImageNeed *m_next;
};

class MCImage : public MCControl
{
	friend class MCHcbmap;
	
	MCImageRep *m_rep;
	MCImageFrame *m_locked_frame;
	MCImageBitmap *m_transformed_bitmap;
	uint32_t m_image_opened;

	bool m_has_transform;
	MCGAffineTransform m_transform;

	MCImageNeed *m_needs;

	bool m_have_control_colors;
	MCColor *m_control_colors;
	char **m_control_color_names;
	uint16_t m_control_color_count;
	uint16_t m_control_color_flags;
	
	uint32_t m_current_width;
	uint32_t m_current_height;

	int2 xhot;
	int2 yhot;
	uint2 angle;
	int2 currentframe;
	int2 repeatcount;
	int2 irepeatcount;
	uint1 resizequality;
	char *filename;
	static int2 magmx;
	static int2 magmy;
	static MCRectangle magrect;
	static MCObject *magtoredraw;

	static Boolean filledborder;
	static MCBrush brush;
	static MCBrush spray;
	static MCBrush eraser;
	static MCCursorRef cursor;
	static MCCursorRef defaultcursor;
	static uint2 cmasks[8];
	
public:
	
	bool setbitmap(MCImageBitmap *p_bitmap, bool p_update_geometry = false);

private:
	void setrep(MCImageRep *p_rep);
	
	bool setcompressedbitmap(MCImageCompressedBitmap *p_compressed);
	bool setfilename(const char *p_filename);
	bool setdata(void *p_data, uindex_t p_size);
	
	void notifyneeds(bool p_deleting);
	
public:
	MCImage();
	MCImage(const MCImage &iref);
	
	virtual ~MCImage();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();
	virtual void open();
	virtual void close();
	virtual Boolean mfocus(int2 x, int2 y);
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void timer(MCNameRef mptr, MCParameter *params);
	virtual void setrect(const MCRectangle &nrect);
	virtual Exec_stat getprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat setprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual void select();
	virtual void deselect();
	virtual void undo(Ustruct *us);
	virtual void freeundo(Ustruct *us);
	
	
	virtual bool lockshape(MCObjectShape& r_shape);
	virtual void unlockshape(MCObjectShape& shape);

	
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite);
	
	
	virtual bool recomputefonts(MCFontRef parent_font);

	
	IO_stat load(IO_handle stream, const char *version);
	IO_stat extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part);

	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);
	virtual Boolean maskrect(const MCRectangle &srect);

	bool isediting() const;
	void startediting(uint16_t p_which);
	void finishediting();
	void sourcerectchanged(MCRectangle p_newrect);

	void invalidate_rep(MCRectangle &p_rect);

	bool convert_to_mutable();

	void resetimage();

	void rotate_transform(int32_t p_angle);
	void resize_transform();

	void apply_transform();

	uint8_t getresizequality()
	{
		return resizequality;
	}

	void setframe(int32_t p_newframe);
	void advanceframe();

	uint32_t getcompression();

	
	bool lockbitmap(MCImageBitmap *&r_bitmap, bool p_premultiplied, bool p_update_transform = true);
	void unlockbitmap(MCImageBitmap *p_bitmap);

	void getgeometry(uint32_t &r_pixwidth, uint32_t &r_pixheight);

	void addneed(MCObject *p_object);

	void endsel();

	
	void drawme(MCDC *dc, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy);
	void drawcentered(MCDC *dc, int2 x, int2 y, Boolean reverse);

	void canceldraw(void);
	void startmag(int2 x, int2 y);
	void endmag(Boolean close);
	void drawmagrect(MCDC *dc);
	void magredrawdest(const MCRectangle &brect);
	
	void magredrawrect(MCContext *context, const MCRectangle &dirty);
	Boolean magmfocus(int2 x, int2 y);
	Boolean magmdown(uint2 which);
	Boolean magmup(uint2 which);
	Boolean magdoubledown(uint2 which);
	Boolean magdoubleup(uint2 which);
	
	static void init();
	static void shutdown();
	
	void cutimage();
	void copyimage();
	void selimage();
	void delimage();
	void pasteimage(MCImage *clipimage);

	void rotatesel(int2 angle);
	void flipsel(Boolean ishorizontal);

	void compute_gravity(MCRectangle &trect, int2 &xorigin, int2 &yorigin);
	void compute_offset(MCRectangle &p_rect, int16_t &r_xoffset, int16_t &r_yoffset);
	void crop(MCRectangle *newrect);
	void createbrush(Properties which);

	static MCBrush *getbrush(Tool p_which);

	MCCursorRef createcursor();
	MCCursorRef getcursor(bool p_is_default = false);
	bool createpattern(MCGImageRef &r_pattern);
	
	Boolean noblack();
	void recompress();
	bool decompressbrush(MCGImageRef &r_brush);
	void openimage();
	void closeimage();
	void prepareimage();
	void reopen(bool p_newfile, bool p_lock_size = false);
	
	IO_stat import(const char *newname, IO_handle stream, IO_handle mstream);

	
	
	
	
	MCSharedString *getclipboardtext(void);

	
	MCWindowShape *makewindowshape(void);
	



#line 477 "c:\\github\\livecode-runrev\\engine\\src\\image.h"
	MCWinSysIconHandle makeicon(uint4 p_width, uint4 p_height);
	MCSharedString *converttodragimage(void);
#line 480 "c:\\github\\livecode-runrev\\engine\\src\\image.h"

	void set_gif(uint1 *data, uint4 length);

	MCString getrawdata(void);
	
	MCImage *next()
	{
		return (MCImage *)MCDLlist::next();
	}

	MCImage *prev()
	{
		return (MCImage *)MCDLlist::prev();
	}

	void totop(MCImage *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}

	void insertto(MCImage *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}

	void appendto(MCImage *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}

	void append(MCImage *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}

	void splitat(MCImage *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}

	MCImage *remove(MCImage *&list)
	{
		return (MCImage *)MCDLlist::remove((MCDLlist *&)list);
	}
};

extern bool MCU_israwimageformat(Export_format p_format);

#line 529 "c:\\github\\livecode-runrev\\engine\\src\\image.h"
#line 31 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\button.h"



















#line 1 "c:\\github\\livecode-runrev\\engine\\src\\control.h"





























































































































































































































































































































#line 21 "c:\\github\\livecode-runrev\\engine\\src\\button.h"










































typedef struct
{
	MCButton *parent;
	MCButton *buttons;
	uint2 f;
	uint2 maxwidth;
	uint2 maxaccelwidth;
}
sublist;

enum Current_icon {
    CI_ARMED,
    CI_DISABLED,
    CI_HILITED,
    CI_DEFAULT,
    CI_VISITED,
		CI_FILE_NICONS,
		CI_HOVER = CI_FILE_NICONS,
    CI_NICONS
};

typedef struct
{
	MCImage *curicon;
	uint4 iconids[CI_NICONS];
}
iconlist;





class ButtonMenuCallback;

class MCButton : public MCControl
{
	friend class MCHcbutton;
	MCCdata *bdata;
	iconlist *icons;
	char *label;
	uint2 labelsize;
	uint2 menusize;
	char *menuname;
	char *menustring;
	MCField *entry;
	MCStack *menu;
	char *acceltext;
	uint2 acceltextsize;
	char *seltext;
	MCString *tabs;
	uint2 ntabs;
	uint2 menuhistory;
	uint2 menulines;
	uint2 accelkey;
	uint2 labelwidth;
	uint2 family;




	uint1 mymenudepth;
	uint1 menubutton;
	uint1 menumode;
	uint1 accelmods;
	uint1 mnemonic;
	uint1 menucontrol;
	bool menuhasitemtags;

	Boolean ishovering;
	static uint2 focusedtab;
	static uint2 mnemonicoffset;
	static MCRectangle optionrect;
	static Keynames button_keys[];
	static uint4 clicktime;
	static uint2 menubuttonheight;
	static Boolean starthilite;
	static uint2 starttab;
	static MCImage *macrb;
	static MCImage *macrbtrack;
	static MCImage *macrbhilite;
	static MCImage *macrbhilitetrack;
public:
	MCButton();
	MCButton(const MCButton &bref);
	
	void removelink(MCObject *optr);
	bool imagechanged(MCImage *p_image, bool p_deleting);

	
	virtual ~MCButton();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual bool visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor* p_visitor);

	virtual void open();
	virtual void close();
	virtual void kfocus();
	virtual Boolean kfocusnext(Boolean top);
	virtual Boolean kfocusprev(Boolean bottom);
	virtual void kunfocus();
	virtual Boolean kdown(const char *string, KeySym key);
	virtual Boolean kup(const char *string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void munfocus();
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);




	virtual uint2 gettransient() const;
	virtual void setrect(const MCRectangle &nrect);
	virtual Exec_stat getprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat setprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual void closemenu(Boolean kfocus, Boolean disarm);
	
	
	virtual bool lockshape(MCObjectShape& r_shape);
	virtual void unlockshape(MCObjectShape& shape);

	
	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	virtual IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part);
	virtual IO_stat load(IO_handle stream, const char *version);
	virtual IO_stat extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length);

	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);
	virtual MCControl *findnum(Chunk_term type, uint2 &num);
	virtual MCControl *findname(Chunk_term type, const MCString &);
	virtual Boolean count(Chunk_term type, MCObject *stop, uint2 &num);
	virtual Boolean maskrect(const MCRectangle &srect);
	virtual void installaccels(MCStack *stack);
	virtual void removeaccels(MCStack *stack);
	virtual MCCdata *getdata(uint4 cardid, Boolean clone);
	virtual void replacedata(MCCdata *&data, uint4 newid);
	virtual void compactdata();
	virtual void resetfontindex(MCStack *oldstack);
	virtual void getwidgetthemeinfo(MCWidgetInfo &widgetinfo);
	
	
	void activate(Boolean notify, uint2 key);
	void clearmnemonic();
	void setupmnemonic();
	MCCdata *getbptr(uint4 cardid);
	uint2 getfamily();
	Boolean gethilite(uint4 parid);
	void setdefault(Boolean def);
	Boolean sethilite(uint4 parid, Boolean hilite);
	void resethilite(uint4 parid, Boolean hilite);

	
	
	void mayberedrawall(void);

	bool gethovering(void)
	{
		return ishovering == 1;
	}

	int32_t getmenucontrol(void)
	{
		return menucontrol;
	}

	MCField *getentry()
	{
		return entry;
	}
	
	
	
	void getlabeltext(MCString &s, bool& r_unicode);

	void getmenustring(MCString &s)
	{
		s.set(menustring, menusize);
	}
	uint1 getmenumode()
	{
		return menumode;
	}
	void setmenumode(uint1 newmode)
	{
		menumode = newmode;
	}
	bool getmenuhastags()
	{
		return menuhasitemtags;
	}
	void setmenuhasitemtags(bool p_hastags)
	{
		menuhasitemtags = p_hastags;
	}
	MCStack *getmenu()
	{
		return menu;
	}
	uint2 getaccelkey()
	{
		return accelkey;
	}
	uint1 getaccelmods()
	{
		return accelmods;
	}
	void getentrytext();
	void createentry();
	void deleteentry();
	void makemenu(sublist *bstack, int2 &stackdepth, uint2 menuflags, MCFontRef fontref);
	
	
	
	Boolean findmenu(bool p_just_for_accel = false);
	
	void openmenu(Boolean grab);
	void freemenu(Boolean force);
	void docascade(MCString &pick);
	void getmenuptrs(const char *&sptr, const char *&eptr);
	void setupmenu();
	void selectedchunk(MCExecPoint &);
	void selectedline(MCExecPoint &);
	void selectedtext(MCExecPoint &);
	Boolean resetlabel();
	void reseticon();
	void radio();
	void setmenuhistory(int2 newline);
	void setmenuhistoryprop(int2 newline);
	uint2 getmousetab(int2 &curx);
	void allocicons();
	void freeicons();
	bool tabselectonmouseup();
	
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite);
	void drawlabel(MCDC *dc, int2 sx, int sy, uint2 t, const MCRectangle &srect, const MCString &lptr, bool isunicode, uint2 fstyle);
	void drawcheck(MCDC *dc, MCRectangle &srect, Boolean white);
	void drawradio(MCDC *dc, MCRectangle &srect, Boolean white);
	void drawoption(MCDC *dc, MCRectangle &srect, MCRectangle& r_content_rect);
	void drawpulldown(MCDC *dc, MCRectangle &srect);
	void drawcascade(MCDC *dc, MCRectangle &srect);
	void drawcombo(MCDC *dc, MCRectangle &srect);
	void drawtabs(MCDC *dc, MCRectangle &srect);
	void drawstandardbutton(MCDC *dc, MCRectangle &srect);
	void drawmacdefault(MCDC *dc, const MCRectangle &srect);
	void drawmacborder(MCDC *dc, MCRectangle &srect);
	void drawmacpopup(MCDC *dc, MCRectangle &srect);








	MCCdata *getcdata(void) {return bdata;}

	MCButton *next()
	{
		return (MCButton *)MCDLlist::next();
	}
	MCButton *prev()
	{
		return (MCButton *)MCDLlist::prev();
	}
	void totop(MCButton *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCButton *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCButton *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCButton *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCButton *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCButton *remove(MCButton *&list)
	{
		return (MCButton *)MCDLlist::remove((MCDLlist *&)list);
	}

private:
	int4 formattedtabwidth(void);

	
	void switchunicode(bool p_to_unicode);
	
	void trytochangetonative(void);

	friend class ButtonMenuCallback;
};
#line 365 "c:\\github\\livecode-runrev\\engine\\src\\button.h"
#line 32 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\stack.h"




















#line 1 "c:\\github\\livecode-runrev\\engine\\src\\object.h"















































































































































































































































































































































































































































































































































































































































































































































































































































#line 22 "c:\\github\\livecode-runrev\\engine\\src\\stack.h"
#line 23 "c:\\github\\livecode-runrev\\engine\\src\\stack.h"


#line 1 "c:\\github\\livecode-runrev\\engine\\src\\uidc.h"























#line 1 "c:\\github\\livecode-runrev\\engine\\src\\dllst.h"



































































#line 25 "c:\\github\\livecode-runrev\\engine\\src\\uidc.h"
#line 26 "c:\\github\\livecode-runrev\\engine\\src\\uidc.h"






#line 1 "c:\\github\\livecode-runrev\\engine\\src\\transfer.h"







































































class MCParagraph;
class MCField;

































class MCSharedString
{
public:
	
	void Retain(void);

	
	void Release(void);

	
	MCString Get(void) const;

	
	const void *GetBuffer(void) const;

	
	uint4 GetLength(void) const;


	
	static MCSharedString *Create(const MCString& p_string);

	
	static MCSharedString *Create(const void *p_data, uint4 p_length);

	
	
	
	static MCSharedString *CreateNoCopy(const MCString& p_string);

	
	
	
	static MCSharedString *CreateNoCopy(void *p_data, uint4 p_length);

private:
	
	
	MCSharedString(void);

	
	
	
	~MCSharedString(void);

	
	uint4 m_references;

	
	char *m_data;

	
	uint4 m_length;
};

inline MCSharedString::MCSharedString(void)
{
	m_references = 0;
	m_data = 0;
	m_length = 0;
}

inline MCSharedString::~MCSharedString(void)
{
	delete m_data;
}

inline void MCSharedString::Retain(void)
{
	m_references += 1;
}

inline void MCSharedString::Release(void)
{
	if (--m_references == 0)
		delete this;
}

inline MCString MCSharedString::Get(void) const
{
	return MCString(m_data, m_length);
}

inline const void *MCSharedString::GetBuffer(void) const
{
	return m_data;
}

inline uint4 MCSharedString::GetLength(void) const
{
	return m_length;
}

inline MCSharedString *MCSharedString::Create(const MCString& p_string)
{
	MCSharedString *t_string;
	t_string = new("c:\\github\\livecode-runrev\\engine\\src\\transfer.h", 203) MCSharedString;
	if (t_string != 0)
	{
		void *t_new_data;
		t_new_data = memdup(p_string . getstring(), p_string . getlength());
		if (t_new_data == 0)
		{
			delete t_string;
			return false;
		}

		t_string -> m_data = (char *)t_new_data;
		t_string -> m_length = p_string . getlength();
		t_string -> Retain();
	}

	return t_string;
}

inline MCSharedString *MCSharedString::Create(const void *p_buffer, uint4 p_length)
{
	return Create(MCString((char *)p_buffer, p_length));
}

inline MCSharedString *MCSharedString::CreateNoCopy(const MCString& p_string)
{
	MCSharedString *t_string;
	t_string = new("c:\\github\\livecode-runrev\\engine\\src\\transfer.h", 230) MCSharedString;
	if (t_string != 0)
	{
		t_string -> m_data = (char *)p_string . getstring();
		t_string -> m_length = p_string . getlength();
		t_string -> Retain();
	}
	return t_string;
}

inline MCSharedString *MCSharedString::CreateNoCopy(void *p_buffer, uint4 p_length)
{
	return CreateNoCopy(MCString((char *)p_buffer, p_length));
}














































enum MCTransferType
{
	TRANSFER_TYPE_NULL,
	TRANSFER_TYPE_TEXT,
	
	TRANSFER_TYPE_TEXT__FIRST = TRANSFER_TYPE_TEXT,
	TRANSFER_TYPE_UNICODE_TEXT,
	TRANSFER_TYPE_STYLED_TEXT,
	TRANSFER_TYPE_RTF_TEXT,
	TRANSFER_TYPE_HTML_TEXT,
	TRANSFER_TYPE_TEXT__LAST = TRANSFER_TYPE_HTML_TEXT,

	TRANSFER_TYPE_IMAGE,
	TRANSFER_TYPE_FILES,
	TRANSFER_TYPE_PRIVATE,
	TRANSFER_TYPE_OBJECTS

};



























class MCPasteboard
{
public:
	
	virtual void Retain(void) = 0;

	
	
	
	virtual void Release(void) = 0;

	
	
	
	
	
	
	
	
	
	virtual bool Query(MCTransferType*& r_types, unsigned int& r_type_count) = 0;

	
	
	
	
	
	
	
	
	
	
	virtual bool Fetch(MCTransferType p_type, MCSharedString*& r_data) = 0;

	
	
	
	
};




















class MCLocalPasteboard: public MCPasteboard
{
public:
	
	MCLocalPasteboard(void);
	
	
	void Retain(void);

	
	void Release(void);

	
	
	bool Query(MCTransferType*& r_types, unsigned int& r_type_count);

	
	
	bool Fetch(MCTransferType p_type, MCSharedString*& r_data);

	
	
	bool Store(MCTransferType p_type, MCSharedString* p_data);

private:
	
	~MCLocalPasteboard(void);

	
	
	bool Find(MCTransferType p_type, uint4& r_index);

	
	
	
	
	
	bool Normalize(MCTransferType p_type, MCSharedString *p_string, MCTransferType& r_normal_type, MCSharedString*& r_normal_data);

	
	uint4 m_references;

	
	uint4 m_count;

	
	MCTransferType *m_types;

	
	MCSharedString **m_datas;
};






























class MCTransferData
{
public:
	MCTransferData(void);
	virtual ~MCTransferData(void);

	
	bool HasText(void);

	
	bool HasImage(void);

	
	bool HasFiles(void);

	
	bool HasPrivate(void);

	
	bool HasObjects(void);


	
	
	
	
	
	bool Lock(void);

	
	
	
	bool Query(MCTransferType*& r_types, uint4& r_type_count);

	
	
	
	bool Contains(MCTransferType p_type, bool p_with_conversion = false);

	
	
	
	MCSharedString *Fetch(MCTransferType p_type);

	
	void Unlock(void);


	
	
	MCParagraph *FetchParagraphs(MCField *p_field);


	
	
	
	void Open(void);

	
	
	bool Store(MCTransferType p_type, MCSharedString *p_string);

	
	
	bool Close(void);

	
	static MCTransferType StringToType(const MCString& p_string);

	
	static const char *TypeToString(MCTransferType p_type);

protected:
	
	
	
	
	
	
	
	
	virtual MCPasteboard *Get(void) = 0;

	
	
	
	
	
	
	
	virtual bool Set(MCPasteboard *p_pasteboard) = 0;

private:
	
	uint4 m_lock_count;

	
	
	MCPasteboard *m_pasteboard;

	
	uint4 m_open_count;

	
	
	MCLocalPasteboard *m_open_pasteboard;

	
	
	bool HasTypeConversion(MCTransferType p_source, MCTransferType p_target);
};




















class MCSelectionData: public MCTransferData
{
public:
	MCSelectionData(void);
	~MCSelectionData(void);

protected:
	MCPasteboard *Get(void);
	bool Set(MCPasteboard *p_pasteboard);

private:
	
	MCPasteboard *m_local_pasteboard;
};
































class MCDragData: public MCTransferData
{
public:
	MCDragData(void);
	~MCDragData(void);

	
	void SetTarget(MCPasteboard *p_pasteboard);

	
	void ResetTarget(void);

	
	
	
	
	MCPasteboard *GetSource(void);

	
	void ResetSource(void);

protected:
	
	
	MCPasteboard *Get(void);
	
	
	
	
	bool Set(MCPasteboard *p_pasteboard);

private:
	
	
	
	
	MCPasteboard *m_target_pasteboard;

	
	
	
	MCPasteboard *m_source_pasteboard;
};













class MCClipboardData: public MCTransferData
{
public:
	MCClipboardData(void);
	~MCClipboardData(void);

protected:
	MCPasteboard *Get(void);
	bool Set(MCPasteboard *p_pasteboard);

private:
	
	MCPasteboard *m_local_pasteboard;
};





enum
{
	DRAG_ACTION_NONE = 0,
	DRAG_ACTION_MOVE = 1 << 0,
	DRAG_ACTION_COPY = 1 << 1,
	DRAG_ACTION_LINK = 1 << 2
};
typedef uint4 MCDragAction;
typedef uint4 MCDragActionSet;

typedef MCSharedString* (*MCTransferConverter)(MCSharedString *p_string);

MCSharedString *MCConvertTextToUnicode(MCSharedString *p_string);
MCSharedString *MCConvertUnicodeToText(MCSharedString *p_string);

MCSharedString *MCConvertStyledTextToText(MCSharedString *p_string);
MCSharedString *MCConvertStyledTextToUnicode(MCSharedString *p_string);
MCSharedString *MCConvertStyledTextToRTF(MCSharedString *p_string);
MCSharedString *MCConvertStyledTextToHTML(MCSharedString *p_string);

MCSharedString *MCConvertTextToStyledText(MCSharedString *p_string);
MCSharedString *MCConvertUnicodeToStyledText(MCSharedString *p_string);
MCSharedString *MCConvertUnicodeToStyledText(const MCString& p_string);
MCSharedString *MCConvertRTFToStyledText(MCSharedString *p_string);
MCSharedString *MCConvertRTFToStyledText(const MCString& p_string);
MCSharedString *MCConvertHTMLToStyledText(MCSharedString *p_string);



enum MCImageFormat
{
	IMAGE_FORMAT_UNKNOWN,
	IMAGE_FORMAT_PNG,
	IMAGE_FORMAT_GIF,
	IMAGE_FORMAT_JFIF,
	IMAGE_FORMAT_DIB
};

struct MCImageData
{
	uint4 width;
	uint4 height;
	uint4 *pixels;
};

MCImageFormat MCImageDataIdentify(MCSharedString *p_string);
MCImageFormat MCImageDataIdentify(const MCString& p_string);
MCImageFormat MCImageDataIdentify(IO_handle p_stream);

MCImageData *MCImageDataDecompress(const MCString& p_string);
MCSharedString *MCImageDataCompress(MCImageData *p_data, MCImageFormat p_as_format);

MCImage *MCImageDataToObject(MCImageData *p_data);
MCImageData *MCImageDataFromObject(MCImage *p_object);



bool MCFormatImageIsPNG(MCSharedString *p_string);
bool MCFormatImageIsGIF(MCSharedString *p_string);
bool MCFormatImageIsJPEG(MCSharedString *p_string);

bool MCFormatStyledTextIsUnicode(MCSharedString *p_string);

#line 791 "c:\\github\\livecode-runrev\\engine\\src\\transfer.h"
#line 33 "c:\\github\\livecode-runrev\\engine\\src\\uidc.h"
#line 34 "c:\\github\\livecode-runrev\\engine\\src\\uidc.h"

#line 1 "c:\\github\\livecode-runrev\\libgraphics\\include\\graphics.h"

























































































































































































































































































































































































































































































































































#line 36 "c:\\github\\livecode-runrev\\engine\\src\\uidc.h"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\imagebitmap.h"














































































































#line 37 "c:\\github\\livecode-runrev\\engine\\src\\uidc.h"

enum Flush_events {
    FE_ALL,
    FE_MOUSEDOWN,
    FE_MOUSEUP,
    FE_KEYDOWN,
    FE_KEYUP,
    FE_AUTOKEY,
    FE_DISK,
    FE_ACTIVATE,
    FE_HIGHLEVEL,
    FE_SYSTEM,
    FE_LAST
};

enum Window_position
{ 
    
    WP_DEFAULT,
    WP_CENTERMAINSCREEN,
    WP_CENTERPARENT,
    WP_CENTERPARENTSCREEN,
    WP_PARENTRIGHT,
    WP_PARENTLEFT,
    WP_PARENTTOP,
    WP_PARENTBOTTOM
};

enum Window_mode {
    WM_CLOSED,
    WM_TOP_LEVEL,  
    WM_TOP_LEVEL_LOCKED,
    WM_MODELESS,
    WM_PALETTE,
    WM_MODAL,
    WM_SHEET,
    WM_PULLDOWN,
    WM_POPUP,
    WM_OPTION,     
    WM_CASCADE,
    WM_COMBO,
    WM_ICONIC,
    WM_DRAWER,
    WM_TOOLTIP,
    WM_LICENSE,
    WM_LAST
};

enum Transfer_type {
    TRT_UNDEFINED,
    TRT_HTML,
    TRT_IMAGE,
    TRT_OBJECT,
    TRT_TEXT,
    TRT_RTF,
    TRT_UNICODE,
    TRT_MAC_STYLED_TEXT,
    TRT_FILES
};

enum Transfer_mode {
    TRM_UNDEFINED,
    TRM_CLIPBOARD,
    TRM_DRAGDROP
};

typedef struct
{
	MCObject *object;
	MCNameRef message;
	real8 time;
	MCParameter *params;
	uint4 id;
}
MCMessageList;

struct MCDisplay
{
	uint4 index;
	MCRectangle viewport;
	MCRectangle workarea;
};

enum MCColorSpaceType
{
	kMCColorSpaceNone,
	kMCColorSpaceCalibratedRGB,
	kMCColorSpaceStandardRGB,
	kMCColorSpaceEmbedded
};

enum MCColorSpaceIntent
{
	kMCColorSpaceIntentPerceptual,
	kMCColorSpaceIntentRelativeColorimetric,
	kMCColorSpaceIntentSaturation,
	kMCColorSpaceIntentAbsoluteColorimetric
};

struct MCColorSpaceInfo
{
	MCColorSpaceType type;
	union
	{
		struct
		{
			double white_x, white_y;
			double red_x, red_y;
			double green_x, green_y;
			double blue_x, blue_y;
			double gamma;
		} calibrated;
		struct
		{
			MCColorSpaceIntent intent;
		} standard;
		struct
		{
			void *data;
			uint32_t data_size;
		} embedded;
	};
};

typedef void *MCColorTransformRef;

class MCMovingList : public MCDLlist
{
public:
	MCObject *object;
	MCPoint *pts;
	uint2 lastpt;
	uint2 curpt;
	int2 donex, doney;
	real8 dx, dy;
	real8 starttime;
	real8 duration;
	real8 speed;
	Boolean waiting;
	MCMovingList()
	{
		pts = 0;
	}
	~MCMovingList();
	MCMovingList *next()
	{
		return (MCMovingList *)MCDLlist::next();
	}
	MCMovingList *prev()
	{
		return (MCMovingList *)MCDLlist::prev();
	}
	void appendto(MCMovingList *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	MCMovingList *remove(MCMovingList *&list)
	{
		return (MCMovingList *)MCDLlist::remove((MCDLlist *&)list);
	}
};

struct MCImageBuffer;



enum
{
	kMCAnswerDialogButtonOk,
	kMCAnswerDialogButtonCancel,
	kMCAnswerDialogButtonRetry,
	kMCAnswerDialogButtonYes,
	kMCAnswerDialogButtonNo,
	kMCAnswerDialogButtonAbort,
	kMCAnswerDialogButtonIgnore
};

enum
{
	kMCAnswerDialogTypeInformation,
	kMCAnswerDialogTypeQuestion,
	kMCAnswerDialogTypeWarning,
	kMCAnswerDialogTypeError
};



enum MCPlatformFeature
{
	PLATFORM_FEATURE_WINDOW_TRANSPARENCY,
	PLATFORM_FEATURE_OS_FILE_DIALOGS,
	PLATFORM_FEATURE_OS_COLOR_DIALOGS,
	PLATFORM_FEATURE_OS_PRINT_DIALOGS,
	PLATFORM_FEATURE_NATIVE_THEMES,
	PLATFORM_FEATURE_TRANSIENT_SELECTION
};

class MCUIDC
{
protected:
	MCMessageList *messages;
	MCMovingList *moving;
	uint4 messageid;
	uint2 nmessages;
	uint2 maxmessages;
	MCColor *colors;
	char **colornames;
	int2 *allocs;
	int2 ncolors;
	Boolean modalclosed;
	Boolean lockmods;
	uint2 redshift;
	uint2 greenshift;
	uint2 blueshift;
	uint2 redbits;
	uint2 greenbits;
	uint2 bluebits;
	const char *  m_sound_internal ;
public:
	MCColor white_pixel;
	MCColor black_pixel;
	MCColor gray_pixel;
	MCColor background_pixel;

	MCUIDC();
	virtual ~MCUIDC();
	
	virtual bool setbeepsound ( const char * p_internal) ;
	virtual const char * getbeepsound ( void );

	virtual bool hasfeature(MCPlatformFeature p_feature);

	virtual void setstatus(const char *status);

	virtual int4 textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override = false);
	
	virtual bool textmask(MCFontStruct *f, const char *s, uint2 len, bool p_unicode_override, MCRectangle clip, MCGAffineTransform transform, MCGMaskRef& r_mask);


	virtual Boolean open();
	virtual Boolean close(Boolean force);

	virtual const char *getdisplayname();
	
	virtual void resetcursors();
	virtual void setcursor(Window w, MCCursorRef c);
	
	virtual void grabpointer(Window w);
	virtual void ungrabpointer();
	
	virtual uint2 getwidth();
	virtual uint2 getheight();
	virtual uint2 getwidthmm();
	virtual uint2 getheightmm();
	virtual uint2 getmaxpoints();
	virtual uint2 getvclass();
	virtual uint2 getdepth();
	virtual uint2 getrealdepth(void);

	virtual uint4 getdisplays(MCDisplay const *& p_displays, bool effective);
	virtual const MCDisplay *getnearestdisplay(const MCRectangle& p_rectangle);

	virtual void openwindow(Window w, Boolean override);
	virtual void closewindow(Window window);
	virtual void destroywindow(Window &window);
	virtual void raisewindow(Window window);
	virtual void iconifywindow(Window window);
	virtual void uniconifywindow(Window window);

	
	virtual void setname(Window window, const char *newname);
	virtual void setcmap(MCStack *sptr);

	virtual void sync(Window w);

	virtual void flush(Window w);

	virtual void beep();

	virtual void setinputfocus(Window window);

	virtual Boolean getwindowgeometry(Window w, MCRectangle &drect);

	virtual void setgraphicsexposures(Boolean on, MCStack *sptr);
	virtual void copyarea(Drawable source, Drawable dest, int2 depth,
	                      int2 sx, int2 sy, uint2 sw, uint2 sh,
	                      int2 dx, int2 dy, uint4 rop);

	virtual MCColorTransformRef createcolortransform(const MCColorSpaceInfo& info);
	virtual void destroycolortransform(MCColorTransformRef transform);
	virtual bool transformimagecolors(MCColorTransformRef transform, MCImageBitmap *image);

	virtual MCCursorRef createcursor(MCImageBitmap *image, int2 xhot, int2 yhot);
	virtual void freecursor(MCCursorRef c);

	virtual uint4 dtouint4(Drawable d);
	virtual Boolean uint4towindow(uint4, Window &w);

	virtual void getbeep(uint4 property, MCExecPoint &ep);
	virtual void setbeep(uint4 property, int4 beep);
	virtual void getvendorstring(MCExecPoint &ep);
	virtual uint2 getpad();
	virtual Window getroot();
	virtual MCImageBitmap *snapshot(MCRectangle &r, uint4 window, const char *displayname);

	virtual void enablebackdrop(bool p_hard = false);
	virtual void disablebackdrop(bool p_hard = false);
	virtual void configurebackdrop(const MCColor& p_colour, MCGImageRef p_pattern, MCImage *p_badge);
	virtual void assignbackdrop(Window_mode p_mode, Window p_window);

	virtual void hidemenu();
	virtual void hidetaskbar();
	virtual void showmenu();
	virtual void showtaskbar();

	virtual MCColor *getaccentcolors();

	virtual void boundrect(MCRectangle &rect, Boolean title, Window_mode m);

	virtual void expose();
	virtual Boolean abortkey();
	virtual void waitconfigure(Window w);
	virtual void waitreparent(Window w);
	virtual void waitfocus();
	virtual void querymouse(int2 &x, int2 &y);
	virtual uint2 querymods();
	virtual void setmouse(int2 x, int2 y);
	virtual Boolean getmouse(uint2 button, Boolean& r_abort);
	virtual Boolean getmouseclick(uint2 button, Boolean& r_abort);
	virtual void addmessage(MCObject *optr, MCNameRef name, real8 time, MCParameter *params);
	virtual void delaymessage(MCObject *optr, MCNameRef name, char *p1 = 0, char *p2 = 0);
	
	
	
	
	
	virtual Boolean wait(real8 duration, Boolean dispatch, Boolean anyevent);
	
	
	
	
	virtual void pingwait(void);

	virtual void flushevents(uint2 e);
	virtual void updatemenubar(Boolean force);
	virtual Boolean istripleclick();
	virtual void getkeysdown(MCExecPoint &ep);
	
	virtual uint1 fontnametocharset(const char *oldfontname);
	virtual char *charsettofontname(uint1 charset, const char *oldfontname);
	
	virtual void clearIME(Window w);
	virtual void openIME();
	virtual void activateIME(Boolean activate);
	virtual void closeIME();

	virtual void seticon(uint4 p_icon);
	virtual void seticonmenu(const char *p_menu);
	virtual void configurestatusicon(uint32_t icon_id, const char *menu, const char *tooltip);
	virtual void enactraisewindows(void);

	

	virtual MCPrinter *createprinter(void);
	virtual void listprinters(MCExecPoint& ep);

	

	virtual int4 getsoundvolume(void);
	virtual void setsoundvolume(int4 p_volume);
	virtual void startplayingsound(IO_handle p_stream, MCObject *p_callback, bool p_next, int p_volume);
	virtual void stopplayingsound(void);

	

	
	
	virtual bool ownsselection(void);

	
	
	
	
	
	
	
	
	
	virtual bool setselection(MCPasteboard *p_pasteboard);

	
	
	
	
	
	
	virtual MCPasteboard *getselection(void);

	

	
	
	
	virtual void flushclipboard(void);

	
	
	virtual bool ownsclipboard(void);

	
	
	
	
	
	
	
	
	
	virtual bool setclipboard(MCPasteboard *p_pasteboard);

	
	
	
	
	
	
	
	
	
	
	virtual MCPasteboard *getclipboard(void);

	
	
	
	
	
	
	
	
	
	
	
	
	
	
	virtual MCDragAction dodragdrop(MCPasteboard *p_pasteboard, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint *p_image_offset);
	
	

	virtual MCScriptEnvironment *createscriptenvironment(const char *p_language);

	

	virtual int32_t popupanswerdialog(const char **p_buttons, uint32_t p_button_count, uint32_t p_type, const char *p_title, const char *p_message);
	virtual char *popupaskdialog(uint32_t p_type, const char *p_title, const char *p_message, const char *p_initial, bool p_hint);
	
	
	
	
	virtual MCStack *getstackatpoint(int32_t x, int32_t y);
	
	

	void addtimer(MCObject *optr, MCNameRef name, uint4 delay);
	void cancelmessageindex(uint2 i, Boolean dodelete);
	void cancelmessageid(uint4 id);
	void cancelmessageobject(MCObject *optr, MCNameRef name);
	void listmessages(MCExecPoint &ep);
	Boolean handlepending(real8 &curtime, real8 &eventtime, Boolean dispatch);
	void addmove(MCObject *optr, MCPoint *pts, uint2 npts,
	             real8 &duration, Boolean waiting);
	void listmoves(MCExecPoint &ep);
	void stopmove(MCObject *optr, Boolean finish);
	void handlemoves(real8 &curtime, real8 &eventtime);
	void siguser();
	Boolean lookupcolor(const MCString &s, MCColor *color);
	void dropper(Drawable d, int2 mx, int2 my, MCColor *cptr);
	Boolean parsecolor(const MCString &s, MCColor *color, char **cname);
	Boolean parsecolors(const MCString &values, MCColor *colors,
	                    char *cnames[], uint2 ncolors);
	void alloccolor(MCColor &color);
	void querycolor(MCColor &color);
	Boolean getcolors(MCExecPoint &);
	Boolean setcolors(const MCString &);
	void getcolornames(MCExecPoint &);
	void getpaletteentry(uint4 n, MCColor &c);

	Boolean position(const char *geom, MCRectangle &rect);
	Boolean hasmessages()
	{
		return nmessages != 0;
	}
	void closemodal()
	{
		modalclosed = 1;
	}
	void setlockmods(Boolean l)
	{
		lockmods = l;
	}
	Boolean getlockmods()
	{
		return lockmods;
	}
	void dodrop(MCStack *dropstack);

	const MCColor& getblack(void) const
	{
		return black_pixel;
	}

	const MCColor& getwhite(void) const
	{
		return white_pixel;
	}

	const MCColor& getgray(void) const
	{
		return gray_pixel;
	}
};

#line 561 "c:\\github\\livecode-runrev\\engine\\src\\uidc.h"
#line 26 "c:\\github\\livecode-runrev\\engine\\src\\stack.h"
#line 27 "c:\\github\\livecode-runrev\\engine\\src\\stack.h"





typedef struct
{
	MCFontStruct *font;
	char *name;
	uint2 size;
	uint2 style;
}
Fontcache;

typedef struct
{
	char *stackname;
	char *filename;
}
MCStackfile;

typedef struct _Mnemonic Mnemonic;

struct MCStackModeData;

class MCStackIdCache;




enum MCStackSurfaceTargetType
{
	kMCStackSurfaceTargetNone,
	kMCStackSurfaceTargetWindowsDC,
	kMCStackSurfaceTargetQuickDraw,
	kMCStackSurfaceTargetCoreGraphics,
	kMCStackSurfaceTargetEAGLContext,
	kMCStackSurfaceTargetPixmap,
};

class MCStackSurface
{
public:
	
	virtual bool LockGraphics(MCRegionRef area, MCGContextRef& r_context) = 0;
	
	virtual void UnlockGraphics(void) = 0;
	
	
	
	virtual bool LockPixels(MCRegionRef area, MCGRaster& r_raster) = 0;
	
	virtual void UnlockPixels(void) = 0;
	
	
	virtual bool LockTarget(MCStackSurfaceTargetType type, void*& r_context) = 0;
	
	virtual void UnlockTarget(void) = 0;
	
	
	virtual bool Composite(MCGRectangle p_dst_rect, MCGImageRef p_source, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend) = 0;

	
	
	virtual bool Lock(void) = 0;
	
	virtual void Unlock(void) = 0;
};

typedef bool (*MCStackUpdateCallback)(MCStackSurface *p_surface, MCRegionRef p_region, void *p_context);

class MCStack : public MCObject
{
	friend class MCHcstak;
	friend class MCHccard;

protected:
	Window window;
	MCCursorRef cursor;
	MCStack *substacks;
	MCCard *cards;
	MCCard *curcard;
	MCControl *controls;
	MCGroup *editing;
	MCCard *savecard;
	MCCard *savecards;
	MCControl *savecontrols;
	MCAudioClip *aclips;
	MCVideoClip *vclips;
	uint4 backgroundid;
	Window_mode mode;
	Window_position wposition;
	Object_pos walignment;
	Mnemonic *mnemonics;
	MCButton **needs;

	
	char *title;
	char *titlestring;
	
	uint4 iconid;
	uint4 windowshapeid;
	uint2 minwidth;
	uint2 minheight;
	uint2 maxwidth;
	uint2 maxheight;
	uint2 decorations;
	uint2 nfuncs;
	uint2 nmnemonics;
	int2 lasty;
	uint2 nneeds;
	uint2 nstackfiles;
	int2 menuy;
	uint2 menuheight;
	uint1 scrollmode;
	uint1 old_blendlevel;
	MCStackfile *stackfiles;
	Linkatts *linkatts;
	char *externalfiles;
	char *filename;
	MCNameRef _menubar;
	void (*idlefunc)();
	
	uint4 f_extended_state;


	CDropTarget *droptarget;
#line 155 "c:\\github\\livecode-runrev\\engine\\src\\stack.h"

	Window parentwindow;
	
	MCExternalHandlerList *m_externals;

	
	
	MCRegionRef m_update_region;

	
	MCTileCacheRef m_tilecache;

	
	int32_t m_scroll;
	
	
	
	MCGImageRef m_snapshot;
	
	
	MCWindowShape *m_window_shape;
	
	
	MCStackIdCache *m_id_cache;
	
	
	
	bool m_defer_updates : 1;
	
	MCRectangle old_rect ; 	
	
	static uint2 ibeam;

public:
	Boolean menuwindow;

	MCStack(void);
	MCStack(const MCStack &sref);
	
	virtual ~MCStack();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();
	
	virtual bool visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor* p_visitor);
	
	virtual void open();
	virtual void close();
	virtual void kfocus();
	virtual Boolean kfocusnext(Boolean top);
	virtual Boolean kfocusprev(Boolean bottom);
	virtual void kunfocus();
	virtual Boolean kdown(const char *string, KeySym key);
	virtual Boolean kup(const char *string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void mfocustake(MCControl *target);
	virtual void munfocus(void);
	virtual void mdrag(void);
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void timer(MCNameRef mptr, MCParameter *params);
	virtual void setrect(const MCRectangle &nrect);
	virtual Exec_stat getprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat setprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);

	virtual Boolean del();
	virtual void paste(void);

	virtual MCStack *getstack();
	virtual Exec_stat handle(Handler_type, MCNameRef, MCParameter *, MCObject *pass_from);
	virtual void recompute();
	
	
	virtual bool lockshape(MCObjectShape& r_shape);
	virtual void unlockshape(MCObjectShape& shape);
	
	
	
    virtual void render(MCContext *dc, const MCRectangle& dirty);

	
	virtual bool recomputefonts(MCFontRef parent_font);
	
    
    MCRectangle getwindowrect() const;
    virtual MCRectangle getrectangle(bool p_effective) const;
    
	void external_idle();
	void loadwindowshape();
	void setidlefunc(void (*newfunc)());
	Boolean setscript(char *newscript);
	void checkdestroy();
	IO_stat print(Print_mode mode, uint2 num, MCCard *card,
	              const MCRectangle *srect, const MCRectangle *drect);
	void resize(uint2 oldw, uint2 oldh);
	void configure(Boolean user);
	void iconify();
	void uniconify();
	void position(const char *geometry);
	Window_mode getmode();
	Window_mode getrealmode()
	{
		return mode;
	}
	uint2 userlevel();
	Boolean hcaddress();
	Boolean hcstack();
	
	virtual bool iskeyed() { return true; }
	virtual void securescript(MCObject *) { }
	virtual void unsecurescript(MCObject *) { }
	
	Boolean islocked();
	Boolean isiconic();
	Boolean isediting();
	Tool gettool(MCObject *optr) const;
	void hidecursor();
	void setcursor(MCCursorRef newcursor, Boolean force);
	MCCursorRef getcursor();
	void resetcursor(Boolean force);
	void clearcursor(void);
	void setibeam();
	void clearibeam();
	void extraopen(bool p_force);
	void extraclose(bool p_force);

	char *resolve_filename(const char *filename);

	void setopacity(uint1 p_value);
	
	void updatemodifiedmark(void);

    
	
	
	
	
	
	
	
	bool resolveparentscripts(void);

	
	
	void checksharedgroups(void);
	void checksharedgroups_slow(void);

	Window getwindow();
	Window getparentwindow();

	void redrawicon();

	uint2 getdecorations()
	{
		return decorations;
	}

	Boolean is_fullscreen (void)
	{
		return ( getextendedstate((1UL << 24)) ) ;
	}

	Boolean takewindow(MCStack *sptr);
	Boolean setwindow(Window w);
	void setparentwindow(Window w);

	void kfocusset(MCControl *target);
	MCStack *clone();
	void compact();
	Boolean checkid(uint4 cardid, uint4 controlid);
	IO_stat saveas(const MCString &);
	MCStack *findname(Chunk_term type, const MCString &);
	MCStack *findid(Chunk_term type, uint4 inid, Boolean alt);
	void setmark();
	void clearmark();
	void setbackground(MCControl *bptr);
	void clearbackground();
	void ungroup(MCGroup *source);
	void startedit(MCGroup *group);
	void stopedit();
	void updatemenubar();
	
	
	
	int32_t getnextscroll(void);
	
	int32_t getscroll(void) const;
	
	
	void applyscroll(void);
	
	
	void clearscroll(void);
	
	
	void syncscroll(void);
	
	void scrollintoview();
	void scrollmenu(int2 offset, Boolean draw);
	void clipmenu(MCContext *context, MCRectangle &crect);
	Boolean count(Chunk_term otype, Chunk_term ptype, MCObject *, uint2 &num);
	void renumber(MCCard *card, uint4 newnumber);
	MCObject *getAV(Chunk_term etype, const MCString &, Chunk_term otype);
	MCCard *getchild(Chunk_term etype, const MCString &, Chunk_term otype);
	MCGroup *getbackground(Chunk_term etype, const MCString &, Chunk_term otype);
	void addneed(MCButton *bptr);
	void removeneed(MCButton *bptr);
	void addmnemonic(MCButton *button, uint1 key);
	void deletemnemonic(MCButton *button);
	MCButton *findmnemonic(char which);
	void installaccels(MCStack *stack);
	void removeaccels(MCStack *stack);
	void setwindowname();
	void openwindow(Boolean override);
	void reopenwindow();
	Exec_stat openrect(const MCRectangle &rel, Window_mode wm, MCStack *parentwindow,
	                   Window_position wpos,  Object_pos walign);
	void getstackfiles(MCExecPoint &);
	void stringtostackfiles(char *d, MCStackfile **sf, uint2 &nf);
	void setstackfiles(const MCString &);
	char *getstackfile(const MCString &);
	void setfilename(char *f);

	virtual IO_stat load(IO_handle stream, const char *version, uint1 type);
	IO_stat load_stack(IO_handle stream, const char *version);
	IO_stat extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length);
	virtual IO_stat load_substacks(IO_handle stream, const char *version);
	
	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	IO_stat save_stack(IO_handle stream, uint4 p_part, bool p_force_ext);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part);

	Exec_stat resubstack(char *data);
	MCCard *getcardid(uint4 inid);
	MCCard *findcardbyid(uint4 p_id);

	MCControl *getcontrolid(Chunk_term type, uint4 inid, bool p_recurse = false);
	MCControl *getcontrolname(Chunk_term type, const MCString &);
	MCObject *getAVid(Chunk_term type, uint4 inid);
	MCObject *getAVname(Chunk_term type, const MCString &);
	Exec_stat setcard(MCCard *card, Boolean recent, Boolean dynamic);
	MCStack *findstackfile(const MCString &s);
	MCStack *findstackname(const MCString &);
	MCStack *findsubstackname(const MCString &);
	MCStack *findstackid(uint4 fid);
	MCStack *findsubstackid(uint4 fid);
	void translatecoords(MCStack *dest, int2 &x, int2 &y);
	uint4 newid();
	void appendaclip(MCAudioClip *aptr);
	void removeaclip(MCAudioClip *aptr);
	void appendvclip(MCVideoClip *vptr);
	void removevclip(MCVideoClip *vptr);
	void appendcontrol(MCControl *cptr);
	void removecontrol(MCControl *cptr);
	void appendcard(MCCard *cptr);
	void removecard(MCCard *cptr);
	MCObject *getsubstackobjid(Chunk_term type, uint4 inid);
	MCObject *getobjid(Chunk_term type, uint4 inid);
	MCObject *getsubstackobjname(Chunk_term type, const MCString &);
	MCObject *getobjname(Chunk_term type, const MCString &);
	void createmenu(MCControl *nc, uint2 width, uint2 height);
	void menuset(uint2 button, uint2 defy);
	void menumup(uint2 which, MCString &s, uint2 &selline);
	void menukdown(const char *string, KeySym key,
	               MCString &s, uint2 &selline);
	void findaccel(uint2 key, MCString &tpick, bool &r_disabled);
	void raise();
	void enter();
	void flip(uint2 count);
	Exec_stat sort(MCExecPoint &ep, Sort_type dir, Sort_type form,
	               MCExpression *by, Boolean marked);
	void breakstring(const MCString &, MCString **dest, uint2 &nstrings,
	                 Find_mode fmode);
	Boolean findone(MCExecPoint &ep, Find_mode mode, const MCString *strings,
	                uint2 nstrings, MCChunk *field, Boolean firstcard);
	void find(MCExecPoint &ep, Find_mode mode, const MCString &, MCChunk *field);
	void markfind(MCExecPoint &ep, Find_mode mode, const MCString &,
	              MCChunk *, Boolean mark);
	void mark(MCExecPoint &ep, MCExpression *where, Boolean mark);
	Linkatts *getlinkatts();
	Boolean cantabort()
	{
		return (flags & (1UL << 16)) != 0;
	}
	const char *getfilename()
	{
		return filename;
	}
	const char *gettitletext()
	{
		return title != 0 ? title : MCNameGetCString(_name);
	}
	MCControl *getcontrols()
	{
		return controls;
	}
	MCCard *getcurcard()
	{
		return curcard;
	}
	MCCard *getcards()
	{
		return cards;
	}

	bool hasmenubar(void)
	{
		return !MCNameIsEmpty(_menubar);
	}
	MCNameRef getmenubar(void)
	{
		return _menubar;
	}

	MCGroup *getediting(void)
	{
		return editing;
	}

	MCStack *getsubstacks(void)
	{
		return substacks;
	}

	MCStackModeData *getmodedata(void)
	{
		return m_mode_data;
	}

	void effectrect(const MCRectangle &drect, Boolean &abort);
	
	MCStack *findstackd(Window w);
	MCStack *findchildstackd(Window w,uint2 &ccount, uint2 cindex);
	void realize();
	void sethints();
	void setgeom();
	void destroywindowshape();

	
	void dirtyall(void);
	
	void dirtyrect(const MCRectangle& rect);
	
	void dirtywindowname(void);

	
	void applyupdates(void);
	
	
	
	
	void updatewindow(MCRegionRef region);
	
	
	
	
	
	
	
	
	
	void updatewindowwithcallback(MCRegionRef p_region, MCStackUpdateCallback p_callback, void *p_context);
	
	
	
	void updatetilecache(void);
	
	bool snapshottilecache(MCRectangle area, MCGImageRef& r_image);
	
    void deactivatetilecache(void);
    
	
	
	void redrawwindow(MCStackSurface *surface, MCRegionRef region);
	
	
	
	void snapshotwindow(const MCRectangle& rect);
	
	
	void takewindowsnapshot(MCStack *other);
	
	
	
	void preservescreenforvisualeffect(const MCRectangle& p_rect);

	
	MCTileCacheRef gettilecache(void) { return m_tilecache; }

	
	bool getacceleratedrendering(void);
	void setacceleratedrendering(bool value);
	
	
	void cacheobjectbyid(MCObject *object);
	void uncacheobjectbyid(MCObject *object);
	MCObject *findobjectbyid(uint32_t object);
	void freeobjectidcache(void);

	inline bool getextendedstate(uint4 flag) const
	{
		return (f_extended_state & flag) != 0;
	}
	
	inline void setextendedstate(bool value, uint4 flag)
	{
		if (value)
			f_extended_state |= flag;
		else
			f_extended_state &= ~flag;
	}
	
	MCExternalHandlerList *getexternalhandlers(void)
	{
		return m_externals;
	}

	void purgefonts();
	
	bool ismetal(void)
	{
		return getflag((1UL << 21)) && (getdecorations() & (1UL << 6)) != 0;
	}
	
	MCWindowShape *getwindowshape(void) { return m_window_shape; }


	MCSysWindowHandle getrealwindow();
	MCSysWindowHandle getqtwindow(void);

	
	
	void onpaint(void);

	
	
	void composite(void);
	
	void getstyle(uint32_t &wstyle, uint32_t &exstyle);
	void constrain(intptr_t lp);
















#line 613 "c:\\github\\livecode-runrev\\engine\\src\\stack.h"
	
	bool cursoroverride ;

	void start_externals();
	void stop_externals();

	MCStack *next()
	{
		return (MCStack *)MCDLlist::next();
	}
	MCStack *prev()
	{
		return (MCStack *)MCDLlist::prev();
	}
	void totop(MCStack *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCStack *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCStack *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCStack *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCStack *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCStack *remove(MCStack *&list)
	{
		return (MCStack *)MCDLlist::remove((MCDLlist *&)list);
	}
	
	MCRectangle recttoroot(const MCRectangle& orect);
	MCRectangle rectfromroot(const MCRectangle& rrect);
	
	void enablewindow(bool p_enable);
	bool mode_haswindow(void);

	void mode_openasmenu(MCStack *grab);
	void mode_closeasmenu(void);

	void mode_constrain(MCRectangle& rect);
	bool mode_needstoopen(void);
	
private:
	void loadexternals(void);
	void unloadexternals(void);

	
	MCStackModeData *m_mode_data;

	void mode_create(void);
	void mode_copy(const MCStack& other);
	void mode_destroy(void);

	void mode_load(void);

	Exec_stat mode_getprop(uint4 parid, Properties which, MCExecPoint &, const MCString &carray, Boolean effective);
	Exec_stat mode_setprop(uint4 parid, Properties which, MCExecPoint &, const MCString &cprop, const MCString &carray, Boolean effective);

	char *mode_resolve_filename(const char *filename);
	void mode_getrealrect(MCRectangle& r_rect);
	void mode_takewindow(MCStack *other);
	void mode_takefocus(void);
	void mode_setgeom(void);
	void mode_setcursor(void);
	
	bool mode_openasdialog(void);
	void mode_closeasdialog(void);
};
#line 691 "c:\\github\\livecode-runrev\\engine\\src\\stack.h"
#line 33 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\card.h"



















#line 1 "c:\\github\\livecode-runrev\\engine\\src\\object.h"















































































































































































































































































































































































































































































































































































































































































































































































































































#line 21 "c:\\github\\livecode-runrev\\engine\\src\\card.h"

class MCCard : public MCObject
{
	friend class MCHccard;
protected:
	MCObjptr *objptrs;
	MCObjptr *kfocused;
	MCObjptr *oldkfocused;
	MCObjptr *mfocused;
	MCButton *defbutton;
	MCButton *odefbutton;
	Boolean mgrabbed;
	MCCdata *savedata;

	
	uint32_t m_bg_layer_id;
	
	uint32_t m_fg_layer_id;
	
	
	bool m_selecting_objects : 1;

	static MCRectangle selrect;
	static int2 startx;
	static int2 starty;
	static MCObjptr *removedcontrol;
public:
	MCCard();
	MCCard(const MCCard &cref);
	
	virtual ~MCCard();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual bool visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor *p_visitor);

	virtual void open();
	virtual void close();
	virtual void kfocus();
	virtual Boolean kfocusnext(Boolean top);
	virtual Boolean kfocusprev(Boolean bottom);
	virtual void kunfocus();
	virtual Boolean kdown(const char *string, KeySym key);
	virtual Boolean kup(const char *string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void mfocustake(MCControl *target);
	virtual void munfocus();
	virtual void mdrag(void);
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void timer(MCNameRef mptr, MCParameter *params);
	virtual Exec_stat getprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat setprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);

	virtual Boolean del();
	virtual void paste(void);

	virtual Exec_stat handle(Handler_type, MCNameRef, MCParameter *, MCObject *pass_from);
	virtual void recompute();
	
	
	virtual bool lockshape(MCObjectShape& r_shape);
	virtual void unlockshape(MCObjectShape& shape);

	
	virtual bool recomputefonts(MCFontRef parent_font);

	
	virtual void relayercontrol(MCControl *p_source, MCControl *p_target);
	virtual void relayercontrol_remove(MCControl *control);
	virtual void relayercontrol_insert(MCControl *control, MCControl *target);

	void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated);

	MCObject *hittest(int32_t x, int32_t y);
	
	
	IO_stat load(IO_handle stream, const char *version);
	IO_stat extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part);

	IO_stat saveobjects(IO_handle stream, uint4 p_part);
	IO_stat loadobjects(IO_handle stream, const char *version);

	void kfocusset(MCControl *target);
	MCCard *clone(Boolean attach, Boolean controls);
	void clonedata(MCCard *source);
	void replacedata(MCStack *source);
	Exec_stat relayer(MCControl *optr, uint2 newlayer);
	MCCard *findname(Chunk_term type, const MCString &);
	MCCard *findid(Chunk_term type, uint4 inid, Boolean alt);
	Boolean countme(uint4 groupid, Boolean marked);
	Boolean count(Chunk_term otype, Chunk_term ptype, MCObject *stop,
	              uint2 &n, Boolean dohc);
	MCControl *getchild(Chunk_term e, const MCString &,
	                    Chunk_term o, Chunk_term p);
	Boolean getchildid(uint4 inid);
	Exec_stat groupmessage(MCNameRef message, MCCard *other);
	void installaccels(MCStack *stack);
	void removeaccels(MCStack *stack);
	void resize(uint2 width, uint2 height);
	MCImage *createimage();
	Boolean removecontrol(MCControl *cptr, Boolean needredraw, Boolean cf);
	void clearfocus(MCObjptr *oldptr, MCObjptr *newptr);
	void erasefocus(MCObject *p_object);
	MCObjptr *newcontrol(MCControl *cptr, Boolean needredraw);
	void resetid(uint4 oldid, uint4 newid);
	Boolean checkid(uint4 controlid);
	Boolean find(MCExecPoint &ep, Find_mode mode, const MCString &,
	             Boolean firstcard, Boolean firstword);
	MCObjptr *getrefs();
	void clean();
	void clear();
	void setmark(Boolean newstate);
	Boolean getmark()
	{
		return (flags & (1UL << 15)) != 0;
	}
	MCControl *getkfocused();
	MCControl *getmfocused();

	MCControl *getmousecontrol(void);
	
	MCObjptr *getobjptrs(void) { return objptrs; }
	MCObjptr *getobjptrforcontrol(MCControl *control);

	void selectedbutton(uint2 n, Boolean bg, MCExecPoint &ep);
	void grab()
	{
		mgrabbed = 1;
	}
	void ungrab()
	{
		mgrabbed = 0;
	}
	Boolean getgrab()
	{
		return mgrabbed;
	}
	void setdefbutton(MCButton *btn)
	{
		defbutton = btn;
	}
	MCButton *getodefbutton()
	{
		return odefbutton;
	}
	MCButton *getdefbutton()
	{
		return defbutton == 0 ? odefbutton : defbutton;
	}
	void freedefbutton(MCButton *btn);
	MCRectangle computecrect();
	void updateselection(MCControl *cptr, const MCRectangle &oldrect, const MCRectangle &selrect, MCRectangle &drect);

	int2 getborderwidth(void);
	void drawcardborder(MCDC *dc, const MCRectangle &dirty);
	
	Exec_stat openbackgrounds(bool p_is_preopen, MCCard *p_other);
	Exec_stat closebackgrounds(MCCard *p_other);
	
	Exec_stat opencontrols(bool p_is_preopen);
	Exec_stat closecontrols(void);

	
	void layer_dirtyrect(const MCRectangle& dirty_rect);
	
	void layer_added(MCControl *control, MCObjptr *previous, MCObjptr *next);
	
	void layer_removed(MCControl *control, MCObjptr *previous, MCObjptr *next);
	
	void layer_setviewport(int32_t x, int32_t y, int32_t width, int32_t height);
	
	void layer_selectedrectchanged(const MCRectangle& old_rect, const MCRectangle& new_rect);

	
	void render(void);

	
	static bool render_background(void *context, MCContext *target, const MCRectangle& dirty);
	
	static bool render_foreground(void *context, MCContext *target, const MCRectangle& dirty);

	
	
	MCObject *getobjbylayer(uint32_t layer);

	MCCard *next()
	{
		return (MCCard *)MCDLlist::next();
	}
	MCCard *prev()
	{
		return (MCCard *)MCDLlist::prev();
	}
	void totop(MCCard *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCCard *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCCard *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCCard *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCCard *node)
	{
		MCDLlist::splitat((MCDLlist *)node);
	}
	MCCard *remove(MCCard *&list)
	{
		return (MCCard *)MCDLlist::remove((MCDLlist *&)list);
	}
};

#line 246 "c:\\github\\livecode-runrev\\engine\\src\\card.h"
#line 34 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\mcerror.h"






















class MCScriptPoint;
class MCExecPoint;

class MCError
{
	MCString svalue;
	char *buffer;
	uint2 errorline;
	uint2 errorpos;
	uint2 depth;
	Boolean thrown;
public:
	MCError()
	{
		buffer = MCU_empty();
		errorline = errorpos = 0;
		depth = 0;
		thrown = 0;
	}
	~MCError()
	{
		delete buffer;
	}
	void add(uint2 id, MCScriptPoint &);
	void add(uint2 id, uint2 line, uint2 pos);
	void add(uint2 id, uint2 line, uint2 pos, uint32_t);
	void add(uint2 id, uint2 line, uint2 pos, const MCString &);
	void add(uint2 id, uint2 line, uint2 pos, MCNameRef);
	void append(MCError& string);
	const MCString &getsvalue();
	void copysvalue(const MCString &s, Boolean t);
	void clear();
	Boolean isempty()
	{
		return strlen(buffer) == 0;
	}
	void geterrorloc(uint2 &line, uint2 &pos);
};
#line 62 "c:\\github\\livecode-runrev\\engine\\src\\mcerror.h"

#line 35 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\objectstream.h"





















class MCObjectInputStream
{
public:
	MCObjectInputStream(IO_handle p_stream, uint32_t p_remaining);
	virtual ~MCObjectInputStream(void);

	IO_stat ReadTag(uint32_t& r_flags, uint32_t& r_length, uint32_t& r_header_length);

	IO_stat ReadU8(uint8_t& r_value);
	IO_stat ReadU16(uint16_t& r_value);
	IO_stat ReadU32(uint32_t& r_value);
	IO_stat ReadU64(uint64_t& r_value);
	IO_stat ReadS8(int8_t& r_value);
	IO_stat ReadS16(int16_t& r_value);
	IO_stat ReadS32(int32_t& r_value);
	IO_stat ReadFloat32(float& r_value);
	IO_stat ReadFloat64(double& r_value);
	IO_stat ReadCString(char*& r_value);
	IO_stat ReadNameRef(MCNameRef& r_value);
	IO_stat ReadColor(MCColor &r_color);

	IO_stat Read(void *p_buffer, uint32_t p_amount);

	IO_stat Mark(void);
	IO_stat Skip(uint32_t p_amount);
	IO_stat Flush(void);

protected:
	virtual IO_stat Fill(void);
	
	IO_handle m_stream;

	int32_t m_mark;

	
	void *m_buffer;

	
	uint32_t m_frontier;

	
	uint32_t m_limit;

	
	uint32_t m_bound;

	
	uint32_t m_remaining;
};



class MCObjectOutputStream
{
public:
	MCObjectOutputStream(IO_handle p_stream);
	virtual ~MCObjectOutputStream(void);

	IO_stat WriteTag(uint32_t p_flags, uint32_t p_length);

	IO_stat WriteFloat32(float p_value);
	IO_stat WriteFloat64(double p_value);
	IO_stat WriteU8(uint8_t p_value);
	IO_stat WriteU16(uint16_t p_value);
	IO_stat WriteU32(uint32_t p_value);
	IO_stat WriteU64(uint64_t p_value);
	IO_stat WriteCString(const char *p_value);
	IO_stat WriteNameRef(MCNameRef p_value);
	IO_stat WriteColor(const MCColor &p_value);

	IO_stat WriteS8(int8_t p_value)
	{
		return WriteU8((unsigned)p_value);
	}

	IO_stat WriteS16(int16_t p_value)
	{
		return WriteU16((unsigned)p_value);
	}

	IO_stat WriteS32(int32_t p_value)
	{
		return WriteU32((unsigned)p_value);
	}

	IO_stat Write(const void *p_buffer, uint32_t p_amount);

	virtual IO_stat Flush(bool p_end);

protected:
	IO_handle m_stream;

	void *m_buffer;
	uint32_t m_mark;
	uint32_t m_frontier;
};



#line 122 "c:\\github\\livecode-runrev\\engine\\src\\objectstream.h"
#line 36 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
#line 1 "c:\\github\\livecode-runrev\\engine\\src\\osspec.h"



















extern void MCS_init();
extern void MCS_shutdown();
extern void MCS_seterrno(int value);
extern int MCS_geterrno(void);
extern uint32_t MCS_getsyserror(void);
extern void MCS_alarm(real8 secs);
extern void MCS_launch_document(char *docname);
extern void MCS_launch_url(char *url);
extern void MCS_startprocess(char *appname, char *docname, Open_mode mode, Boolean elevated);
extern void MCS_checkprocesses();
extern void MCS_closeprocess(uint2 index);
extern void MCS_kill(int4 pid, int4 sig);
extern void MCS_killall();
extern real8 MCS_time();
extern void MCS_reset_time();
extern void MCS_sleep(real8);
extern char *MCS_getenv(const char *name);
extern void MCS_setenv(const char *name, const char *value);
extern void MCS_unsetenv(const char *name);
extern int4 MCS_rawopen(const char *path, int4 flags);
extern int4 MCS_rawclose(int4 fd);
extern Boolean MCS_rename(const char *oname, const char *nname);
extern Boolean MCS_backup(const char *oname, const char *nname);
extern Boolean MCS_unbackup(const char *oname, const char *nname);
extern Boolean MCS_unlink(const char *path);
extern const char *MCS_tmpnam();
extern char *MCS_resolvepath(const char *path);
extern char *MCS_get_canonical_path(const char *path);
extern char *MCS_getcurdir();
extern Boolean MCS_setcurdir(const char *path);
extern void MCS_getentries(MCExecPoint &p_context, bool p_files, bool p_detailed);
extern void MCS_getDNSservers(MCExecPoint &);
extern Boolean MCS_getdevices(MCExecPoint &);
extern Boolean MCS_getdrives(MCExecPoint &);
extern Boolean MCS_noperm(const char *path);
extern Boolean MCS_exists(const char *path, Boolean file);
extern Boolean MCS_nodelay(int4 fd);

extern IO_stat MCS_runcmd(MCExecPoint &);
extern uint2 MCS_umask(uint2 mask);
extern IO_stat MCS_chmod(const char *path, uint2 mask);
extern int4 MCS_getumask();
extern void MCS_setumask(int4 newmask);
extern Boolean MCS_mkdir(const char *path);
extern Boolean MCS_rmdir(const char *path);

extern uint4 MCS_getpid();
extern const char *MCS_getaddress();
extern const char *MCS_getmachine();
extern const char *MCS_getprocessor();
extern real8 MCS_getfreediskspace(void);
extern const char *MCS_getsystemversion();
extern void MCS_loadfile(MCExecPoint &ep, Boolean binary);
extern void MCS_loadresfile(MCExecPoint &ep);
extern void MCS_savefile(const MCString &f, MCExecPoint &data, Boolean b);
extern void MCS_saveresfile(const MCString &s, const MCString data);

extern void MCS_query_registry(MCExecPoint &dest, const char** type);
extern void MCS_delete_registry(const char *key, MCExecPoint &dest);
extern void MCS_list_registry(MCExecPoint& dest);
extern void MCS_set_registry(const char *key, MCExecPoint &dest, char *type);

extern Boolean MCS_poll(real8 delay, int fd);

extern void MCS_send(const MCString &mess, const char *program,
	                     const char *eventtype, Boolean reply);
extern void MCS_reply(const MCString &mess, const char *keyword, Boolean err);
extern char *MCS_request_ae(const MCString &message, uint2 ae);
extern char *MCS_request_program(const MCString &message, const char *program);
extern void MCS_copyresourcefork(const char *source, const char *dest);
extern void MCS_copyresource(const char *source, const char *dest,
	                             const char *type, const char *name,
	                             const char *newid);
extern void MCS_deleteresource(const char *source, const char *type,
	                               const char *name);
extern void MCS_getresource(const char *source, const char *type,
	                            const char *name, MCExecPoint &ep);
extern char *MCS_getresources(const char *source, const char *type);
extern void MCS_setresource(const char *source, const char *type,
	                            const char *name, const char *id, const char *flags,
	                            const MCString &s);
extern void MCS_getspecialfolder(MCExecPoint &ep);
extern void MCS_shortfilepath(MCExecPoint &ep);
extern void MCS_longfilepath(MCExecPoint &ep);
extern Boolean MCS_createalias(char *srcpath, char *dstpath);
extern void MCS_resolvealias(MCExecPoint &ep);
extern void MCS_doalternatelanguage(MCString &s, const char *langname);
extern void MCS_alternatelanguages(MCExecPoint &ep);
extern uint1 MCS_langidtocharset(uint2 langid);
extern uint2 MCS_charsettolangid(uint1 charset);

extern void MCS_multibytetounicode(const char *s, uint4 len, char *d, uint4 destbufferl, uint4 &destlen, uint1 charset);
extern void MCS_unicodetomultibyte(const char *s, uint4 len, char *d, uint4 destbufferl, uint4 &destlen, uint1 charset);

extern void MCS_nativetoutf16(const char *p_native, uint4 p_native_length, unsigned short *p_utf16, uint4& p_utf16_length);
extern void MCS_utf16tonative(const unsigned short *p_utf16, uint4 p_utf16_length, char *p_native, uint4& p_native_length);

extern void MCS_nativetoutf8(const char *p_native, uint4 p_native_length, char *p_utf8, uint4& p_utf16_length);
extern void MCS_utf8tonative(const char *p_utf8, uint4 p_uitf8_length, char *p_native, uint4& p_native_length);

extern Boolean MCS_isleadbyte(uint1 charset, char *s);

extern MCSysModuleHandle MCS_loadmodule(const char *p_filename);
extern void *MCS_resolvemodulesymbol(MCSysModuleHandle p_module, const char *p_symbol);
extern void MCS_unloadmodule(MCSysModuleHandle p_module);

extern void MCS_getlocaldatetime(MCDateTime& x_datetime);
extern bool MCS_datetimetouniversal(MCDateTime& x_datetime);
extern bool MCS_datetimetolocal(MCDateTime& x_datetime);
extern bool MCS_datetimetoseconds(const MCDateTime& p_datetime, double& r_seconds);
extern bool MCS_secondstodatetime(double p_seconds, MCDateTime& r_datetime);
extern const MCDateTimeLocale *MCS_getdatetimelocale(void);

extern char *MCS_dnsresolve(const char *p_hostname);
extern char *MCS_hostaddress(void);

extern bool MCS_processtypeisforeground(void);
extern bool MCS_changeprocesstype(bool to_foreground);

extern bool MCS_isatty(int);
extern bool MCS_isnan(double value);

extern bool MCS_mcisendstring(const char *command, char buffer[256]);


void MCS_system_alert(const char *title, const char *message);

bool MCS_generate_uuid(char buffer[128]);

void MCS_getnetworkinterfaces(MCExecPoint& ep);




bool MCS_random_bytes(size_t p_count, void *p_buffer);



void MCS_deleteurl(MCObject *p_target, const char *p_url);
void MCS_loadurl(MCObject *p_target, const char *p_url, const char *p_message);
void MCS_unloadurl(MCObject *p_target, const char *p_url);
void MCS_posttourl(MCObject *p_target, const MCString& p_data, const char *p_url);
void MCS_putintourl(MCObject *p_target, const MCString& p_data, const char *p_url);
void MCS_geturl(MCObject *p_target, const char *p_url);



enum MCSPutKind
{
	kMCSPutNone,
	kMCSPutIntoMessage,
	kMCSPutAfterMessage,
	kMCSPutBeforeMessage,
	
	kMCSPutOutput,
	kMCSPutUnicodeOutput,
	kMCSPutBinaryOutput,
	kMCSPutHeader,
	kMCSPutNewHeader,
	kMCSPutContent,
	kMCSPutUnicodeContent,
	kMCSPutMarkup,
	kMCSPutUnicodeMarkup
};

bool MCS_put(MCExecPoint& ep, MCSPutKind kind, const MCString& data);



enum MCSErrorMode
{
	kMCSErrorModeNone,
	kMCSErrorModeQuiet,
	kMCSErrorModeInline,
	kMCSErrorModeStderr,
	kMCSErrorModeDebugger,
};

void MCS_set_errormode(MCSErrorMode mode);
MCSErrorMode MCS_get_errormode(void);

enum MCSOutputTextEncoding
{
	kMCSOutputTextEncodingUndefined,
	
	kMCSOutputTextEncodingWindows1252,
	kMCSOutputTextEncodingMacRoman,
	kMCSOutputTextEncodingISO8859_1,
	kMCSOutputTextEncodingUTF8,
	


#line 213 "c:\\github\\livecode-runrev\\engine\\src\\osspec.h"
	kMCSOutputTextEncodingNative = kMCSOutputTextEncodingWindows1252,




#line 219 "c:\\github\\livecode-runrev\\engine\\src\\osspec.h"
};

void MCS_set_outputtextencoding(MCSOutputTextEncoding encoding);
MCSOutputTextEncoding MCS_get_outputtextencoding(void);

enum MCSOutputLineEndings
{
	kMCSOutputLineEndingsLF,
	kMCSOutputLineEndingsCR,
	kMCSOutputLineEndingsCRLF,
	


#line 233 "c:\\github\\livecode-runrev\\engine\\src\\osspec.h"

#line 235 "c:\\github\\livecode-runrev\\engine\\src\\osspec.h"
	kMCSOutputLineEndingsNative = kMCSOutputLineEndingsCRLF,


#line 239 "c:\\github\\livecode-runrev\\engine\\src\\osspec.h"

};

void MCS_set_outputlineendings(MCSOutputLineEndings line_endings);
MCSOutputLineEndings MCS_get_outputlineendings(void);

bool MCS_set_session_save_path(const char *p_path);
const char *MCS_get_session_save_path(void);
bool MCS_set_session_lifetime(uint32_t p_lifetime);
uint32_t MCS_get_session_lifetime(void);
bool MCS_set_session_name(const char *p_name);
const char *MCS_get_session_name(void);
bool MCS_set_session_id(const char *p_id);
const char *MCS_get_session_id(void);



#line 257 "c:\\github\\livecode-runrev\\engine\\src\\osspec.h"
#line 37 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"

#line 1 "c:\\github\\livecode-runrev\\engine\\src\\context.h"



















#line 1 "c:\\github\\livecode-runrev\\engine\\src\\imagebitmap.h"














































































































#line 21 "c:\\github\\livecode-runrev\\engine\\src\\context.h"
#line 1 "c:\\github\\livecode-runrev\\libgraphics\\include\\graphics.h"

























































































































































































































































































































































































































































































































































#line 22 "c:\\github\\livecode-runrev\\engine\\src\\context.h"

typedef real4 MCScalar;

struct MCTransform
{
	MCScalar sx, ry;
	MCScalar rx, sy;
	MCScalar tx, ty;
};

enum MCQuality
{
	QUALITY_DEFAULT,
	QUALITY_SMOOTH
};

enum MCContextType
{
	CONTEXT_TYPE_SCREEN,
	CONTEXT_TYPE_OFFSCREEN,
	CONTEXT_TYPE_PRINTER
};

struct MCStrokeStyle
{
	uint2 style;
	uint2 width;
	uint2 cap;
	uint2 join;
	real8 miter_limit;
	struct
	{
		uint2 length;
		uint4 *data;
	} dash;
};

class MCPath;
struct MCGradientFill;

struct MCBitmapEffects;
typedef MCBitmapEffects *MCBitmapEffectsRef;

enum MCImageDataType
{
	kMCImageDataNone,
	kMCImageDataPNG,
	kMCImageDataGIF,
	kMCImageDataJPEG
};

struct MCImageDescriptor
{
	bool has_transform;
	MCGAffineTransform transform;
	MCGImageFilter filter;

	
	MCImageBitmap *bitmap;

	
	MCImageDataType data_type;
	void *data_bits;
	uint32_t data_size;
};

enum MCThemeDrawType
{
	THEME_DRAW_TYPE_SLIDER,
	THEME_DRAW_TYPE_SCROLLBAR,
	THEME_DRAW_TYPE_PROGRESS,
	THEME_DRAW_TYPE_BUTTON,
	THEME_DRAW_TYPE_FRAME,
	THEME_DRAW_TYPE_GROUP,
	THEME_DRAW_TYPE_TAB,
	THEME_DRAW_TYPE_TAB_PANE,
	THEME_DRAW_TYPE_FOCUS_RECT,
	THEME_DRAW_TYPE_BACKGROUND,
	THEME_DRAW_TYPE_MENU,
	THEME_DRAW_TYPE_GTK
};

bool MCThemeDraw(MCGContextRef p_context, MCThemeDrawType p_type, MCThemeDrawInfo *p_info_ptr);

class MCContext
{
public:
	virtual ~MCContext(void) { };

	
	
	
	
	
	
	virtual void begin(bool p_group) = 0;
	virtual bool begin_with_effects(MCBitmapEffectsRef effects, const MCRectangle& shape) = 0;
	virtual void end(void) = 0;

	virtual MCContextType gettype(void) const = 0;

	
	
	
	
	
	virtual bool changeopaque(bool p_value) = 0;

	virtual void setprintmode(void) = 0;

	virtual void setclip(const MCRectangle& rect) = 0;
	virtual const MCRectangle& getclip(void) const = 0;
	virtual void clearclip(void) = 0;

	virtual void setorigin(int2 x, int2 y) = 0;
	virtual void clearorigin(void) = 0;

	virtual void setquality(uint1 quality) = 0;
	virtual void setfunction(uint1 function) = 0;
	virtual uint1 getfunction(void) = 0;
	virtual void setopacity(uint1 opacity) = 0;
	virtual uint1 getopacity(void) = 0;
	virtual void setforeground(const MCColor& c) = 0;
	virtual void setbackground(const MCColor& c) = 0;
	virtual void setdashes(uint2 offset, const uint1 *dashes, uint2 ndashes) = 0;
	virtual void setfillstyle(uint2 style, MCGImageRef p, int2 x, int2 y) = 0;
	virtual void getfillstyle(uint2& style, MCGImageRef& p, int2& x, int2& y) = 0;
	virtual void setlineatts(uint2 linesize, uint2 linestyle, uint2 capstyle, uint2 joinstyle) = 0;
	virtual void setmiterlimit(real8 p_limit) = 0;
	virtual void setgradient(MCGradientFill *p_gradient) = 0;

	virtual void drawline(int2 x1, int2 y1, int2 x2, int2 y2) = 0;
	virtual void drawlines(MCPoint *points, uint2 npoints, bool p_closed = false) = 0;
	virtual void drawsegments(MCSegment *segments, uint2 nsegs) = 0;
	virtual void drawtext(int2 x, int2 y, const char *s, uint2 length, MCFontStruct *f, Boolean image, bool p_unicode_override = false) = 0;
	virtual void drawrect(const MCRectangle& rect) = 0;
	virtual void fillrect(const MCRectangle& rect) = 0;
	virtual void fillrects(MCRectangle *rects, uint2 nrects) = 0;
	virtual void fillpolygon(MCPoint *points, uint2 npoints) = 0;
	virtual void drawroundrect(const MCRectangle& rect, uint2 radius) = 0;
	virtual void fillroundrect(const MCRectangle& rect, uint2 radius) = 0;
	virtual void drawarc(const MCRectangle& rect, uint2 start, uint2 angle) = 0;
	virtual void drawsegment(const MCRectangle& rect, uint2 start, uint2 angle) = 0;
	virtual void fillarc(const MCRectangle& rect, uint2 start, uint2 angle) = 0;

	virtual void drawpath(MCPath *path) = 0;
	virtual void fillpath(MCPath *path, bool p_evenodd = true) = 0;

	virtual void drawpict(uint1 *data, uint4 length, bool embed, const MCRectangle& drect, const MCRectangle& crect) = 0;
	virtual void draweps(real8 sx, real8 sy, int2 angle, real8 xscale, real8 yscale, int2 tx, int2 ty,
		                   const char *prolog, const char *psprolog, uint4 psprologlength, const char *ps, uint4 length,
											 const char *fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font, const MCRectangle& trect) = 0;
	
	virtual void drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy) = 0;

	virtual void drawlink(const char *link, const MCRectangle& region) = 0;

	virtual int4 textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override = false) = 0;

	virtual void applywindowshape(MCWindowShape *p_mask, uint4 p_u_width, uint4 p_u_height) = 0;

	virtual void drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo* p_parameters) = 0;

	
	virtual MCRegionRef computemaskregion(void) = 0;
	virtual void clear(const MCRectangle* rect) = 0;

	virtual uint2 getdepth(void) const = 0;

	virtual const MCColor& getblack(void) const = 0;
	virtual const MCColor& getwhite(void) const = 0;
	virtual const MCColor& getgray(void) const = 0;
	virtual const MCColor& getbg(void) const = 0;
};

#line 198 "c:\\github\\livecode-runrev\\engine\\src\\context.h"
#line 39 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"

#line 1 "c:\\github\\livecode-runrev\\engine\\src\\globals.h"

































































































































































































































































































































































































































#line 41 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"







int2 MCImage::magmx;
int2 MCImage::magmy;
MCRectangle MCImage::magrect;
MCObject *MCImage::magtoredraw;
Boolean MCImage::filledborder;
MCBrush MCImage::brush;
MCBrush MCImage::spray;
MCBrush MCImage::eraser;
MCCursorRef MCImage::cursor;
MCCursorRef MCImage::defaultcursor;
uint2 MCImage::cmasks[7 + 1] = {0x00, 0x01, 0x03, 0x07,
                                        0x0F, 0x1F, 0x3F, 0x7F};

MCImage::MCImage()
{
	angle = 0;
	flags &= ~((1UL << 5) | (1UL << 13));

	m_rep = 0;
	m_transformed_bitmap = 0;
	m_image_opened = false;
	m_has_transform = false;

	m_have_control_colors = false;
	m_control_colors = 0;
	m_control_color_names = 0;
	m_control_color_count = 0;

	m_needs = 0;

	filename = 0;

	xhot = yhot = 1;
	currentframe = 0;
	repeatcount = 0;
	resizequality = 0;
}

MCImage::MCImage(const MCImage &iref) : MCControl(iref)
{
	m_rep = 0;
	m_transformed_bitmap = 0;
	m_image_opened = false;
	m_has_transform = false;

	m_have_control_colors = false;
	m_control_colors = 0;
	m_control_color_names = 0;
	m_control_color_count = 0;

	m_needs = 0;

	filename = 0;

	if (iref.isediting())
	{
		MCImageBitmap *t_bitmap = 0;
		static_cast<MCMutableImageRep*>(iref.m_rep)->copy_selection(t_bitmap);
		setbitmap(t_bitmap);
		MCImageFreeBitmap(t_bitmap);
		if (static_cast<MCMutableImageRep*>(iref.m_rep)->has_selection())
		{
			xhot = t_bitmap->width >> 1;
			yhot = t_bitmap->height >> 1;
		}
	}
	else
	{
		xhot = iref.xhot;
		yhot = iref.yhot;
		if (iref . m_rep != 0)
			m_rep = iref . m_rep->Retain();
	}

	if (iref.flags & (1UL << 16))
		 MCCStringClone(iref.filename, filename);

	angle = iref.angle;
	currentframe = 0;
	repeatcount = iref.repeatcount;
	resizequality = iref.resizequality;
}

MCImage::~MCImage()
{
	while (opened)
		close();

	if (m_needs != 0)
		notifyneeds(true);

	if (m_rep != 0)
	{
		m_rep->Release();
		m_rep = 0;
	}

	if (filename != 0)
		MCCStringFree(filename);
}

Chunk_term MCImage::gettype() const
{
	return CT_IMAGE;
}

const char *MCImage::gettypestring()
{
	return MCimagestring;
}

void MCImage::open()
{
	MCControl::open();
	
	
	
	if ((opened == 1) && (MCbufferimages || flags & (1UL << 24)))
		openimage();
}

void MCImage::close()
{
	if (state & (1UL << 18))
		endsel();
	if (state & (1UL << 14))
		endmag(1);
	if (opened == 1 && m_image_opened)
		closeimage();
	MCControl::close();
}

Boolean MCImage::mfocus(int2 x, int2 y)
{
	if (!(flags & (1UL << 11) || MCshowinvisibles)
	        || flags & (1UL << 12) && getstack()->gettool(this) == T_BROWSE)
		return 0;
	
	mx = x;
	my = y;
	
	if (state & (1UL << 14) && state & (1UL << 16))
	{
		MCRectangle brect;
		brect = MCU_reduce_rect(magrect, -8);
		magrect.x += mx - startx;
		magrect.y += my - starty;
		int2 oldx = magrect.x;
		int2 oldy = magrect.y;
		MCRectangle trect = MCU_intersect_rect(rect, getcard()->getrect());
		magrect = MCU_bound_rect(magrect, trect.x - rect.x, trect.y - rect.y,
		                         trect.width, trect.height);
		brect = MCU_union_rect(brect, MCU_reduce_rect(magrect, -8));
		startx = mx + (magrect.x - oldx);
		starty = my + (magrect.y - oldy);
		brect.x += rect.x;
		brect.y += rect.y;

		layer_redrawrect(brect);
		magredrawdest(brect);

		return 1;
	}

	if (isediting() &&
		static_cast<MCMutableImageRep *>(m_rep) -> image_mfocus(x, y))
		return 1;
	

	switch(getstack() -> gettool(this))
	{
	case T_BRUSH:
	case T_BUCKET:
	case T_CURVE:
	case T_ERASER:
	case T_LASSO:
	case T_LINE:
	case T_OVAL:
	case T_PENCIL:
	case T_POLYGON:
	case T_RECTANGLE:
	case T_REGULAR_POLYGON:
	case T_ROUND_RECT:
	case T_SELECT:
	case T_SPRAY:
	case T_TEXT:
		if (flags & (1UL << 16) || !MCU_point_in_rect(rect, x, y))
			return 0;
		message_with_args(MCM_mouse_move, x, y);
		break;
	case T_BROWSE:
	case T_POINTER:
	case T_IMAGE:
	case T_HELP:
		return MCControl::mfocus(x, y);
	default:
		return 0;
	}
	return 1;

}

Boolean MCImage::mdown(uint2 which)
{
	if (state & (1UL << 5))
		return 0;
		
	if (state & (1UL << 31))
		return MCObject::mdown(which);
	
	state |= (1UL << 5);
	
	switch (which)
	{
	case 3:
	case 1:
			switch (getstack()->gettool(this))
			{
			case T_BROWSE:
				message_with_args(MCM_mouse_down, which);
				break;
			case T_POINTER:
			case T_IMAGE:
				if (which != 1)
				{
					message_with_args(MCM_mouse_down, which);
					break;
				}
				if (state & (1UL << 14))
					endmag(1);
				finishediting();
				start(1);
				if (state & ((1UL << 8) | (1UL << 9) | (1UL << 10) | (1UL << 11)))
				{
					if (MCmodifierstate & 0x02)
					{ 
						state |= (1UL << 22);
						m_current_width = rect.width;
						m_current_height = rect.height;
					}
					if (state & (1UL << 21))
					{
						state &= ~((1UL << 8) | (1UL << 9) | (1UL << 10) | (1UL << 11));
						state |= (1UL << 7);
						MCselected->startmove(mx, my, 0);
					}
				}
				break;
			case T_HELP:
				break;
			default:
				if (state & (1UL << 14)
						&& !MCU_point_in_rect(magrect, mx - rect.x, my - rect.y)
						&& MCU_point_in_rect(MCU_reduce_rect(magrect, -8),
											 mx - rect.x, my - rect.y))
				{
					startx = mx;
					starty = my;
					MCRectangle brect;
					state &= ~(1UL << 14);
					brect = MCU_reduce_rect(magrect, -8);
					brect.x += rect.x;
					brect.y += rect.y;
					brect.width = (brect.width + (brect.x & 0x07) + 0x07) & ~0x07;
					brect.x &= ~0x07;
					
					layer_redrawrect(brect);
					state |= (1UL << 14) | (1UL << 16);

					if (state & (1UL << 14))
						magredrawdest(brect);
				}
				else
				{
					switch (getstack()->gettool(this))
					{
					case T_BRUSH:
					case T_SPRAY:
					case T_ERASER:
					case T_PENCIL:
						if (MCmodifierstate & 0x02)
						{
							if (state & (1UL << 14))
								endmag(1);
							else
								startmag(mx - rect.x, my - rect.y);

							return 1;
						}
					}

					if (isediting() &&
						static_cast<MCMutableImageRep *>(m_rep) -> image_mdown(which))
						return 1;

					startediting(which);
				}

				break;
			}
		break;
	case 2:
		message_with_args(MCM_mouse_down, "2");
		break;
	}
	return 1;
}

Boolean MCImage::mup(uint2 which)
{
	if (!(state & (1UL << 5)))
		return 0;

	if (state & (1UL << 31))
		return MCObject::mup(which);
		
	state &= ~(1UL << 5);
	if (state & (1UL << 30))
	{
		ungrab(which);
		return 1;
	}
	
	if (state & (1UL << 14) && state & (1UL << 16))
	{
		state &= ~(1UL << 16);
		return 1;
	}

	if (isediting() &&
		static_cast<MCMutableImageRep *>(m_rep) -> image_mup(which))
		return 1;

	switch(getstack() -> gettool(this))
	{
	case T_BROWSE:
		MCRectangle srect;
		MCU_set_rect(srect, mx, my, 1, 1);
		if (maskrect(srect))
			message_with_args(MCM_mouse_up, which);
		else
			message_with_args(MCM_mouse_release, which);
		break;
	case T_IMAGE:
	case T_POINTER:
		{
			if (which != 1)
			{
				message_with_args(MCM_mouse_up, which);
				break;
			}
			uint32_t t_pixwidth, t_pixheight;
			getgeometry(t_pixwidth, t_pixheight);

			if ((t_pixwidth != rect.width || t_pixheight != rect.height)
					&& state & ((1UL << 8) | (1UL << 9) | (1UL << 10) | (1UL << 11)) && state & (1UL << 22))
			{
				crop(0);
			}
			state &= ~(((1UL << 8) | (1UL << 9) | (1UL << 10) | (1UL << 11)) | (1UL << 22));
			end(false);
			if (state & (1UL << 14))
				magredrawdest(rect);
			if (maskrect(srect))
				message_with_args(MCM_mouse_up, which);
		}
		break;
	case T_HELP:
		help();
		break;
	default:
		break;
	}
	return 1;
}

Boolean MCImage::doubledown(uint2 which)
{
	if (isediting() && static_cast<MCMutableImageRep*>(m_rep)->image_doubledown(which) == 1)
		return 1;
	return MCControl::doubledown(which);
}

Boolean MCImage::doubleup(uint2 which)
{
	if (isediting() && static_cast<MCMutableImageRep*>(m_rep)->image_doubleup(which) == 1)
		return 1;
	return MCControl::doubleup(which);
}

void MCImage::timer(MCNameRef mptr, MCParameter *params)
{
	if (MCNameIsEqualTo(mptr, MCM_internal, kMCCompareCaseless))
	{
		if (state & (1UL << 18))
		{
			dashoffset++;
			dashoffset &= 0x07;
			MCRectangle trect = selrect;
			trect.x += rect.x;
			trect.y += rect.y;
			
			layer_redrawrect(trect);
			if (state & (1UL << 14))
				magredrawdest(trect);
			MCscreen->addtimer(this, MCM_internal, MCmovespeed);
		}
		else
			if ((isvisible() || m_needs) && irepeatcount && m_rep != 0 && m_rep->GetFrameCount() > 1)
			{
				advanceframe();
				if (irepeatcount)
				{
					MCImageFrame *t_frame = 0;
					if (m_rep->LockImageFrame(currentframe, true, t_frame))
					{
						MCscreen->addtimer(this, MCM_internal, t_frame->duration);
						m_rep->UnlockImageFrame(currentframe, t_frame);
					}
				}
			}
	}
	else if (MCNameIsEqualTo(mptr, MCM_internal2, kMCCompareCaseless))
		{
			if (state & (1UL << 14))
			{
				switch (getstack()->gettool(this))
				{
				case T_BROWSE:
				case T_POINTER:
				case T_IMAGE:
				case T_BRUSH:
				case T_BUCKET:
				case T_CURVE:
				case T_DROPPER:
				case T_ERASER:
				case T_LASSO:
				case T_LINE:
				case T_OVAL:
				case T_PENCIL:
				case T_POLYGON:
				case T_RECTANGLE:
				case T_REGULAR_POLYGON:
				case T_ROUND_RECT:
				case T_SELECT:
				case T_SPRAY:
				case T_TEXT:
					{
						if (!(state & (1UL << 18)))
						{
							dashoffset++;
							dashoffset &= 0x07;
						}
						MCRectangle trect = MCU_reduce_rect(magrect, -8);
						trect.x += rect.x;
						trect.y += rect.y;
						
						layer_redrawrect(trect);
						MCscreen->addtimer(this, MCM_internal2, MCmovespeed);
					}
					break;
				default:
					endmag(1);
					break;
				}
			}
		}
		else
			MCControl::timer(mptr, params);
}

void MCImage::setrect(const MCRectangle &nrect)
{
	MCRectangle orect = rect;
	rect = nrect;

	if (!(state & ((1UL << 8) | (1UL << 9) | (1UL << 10) | (1UL << 11))) || !(state & (1UL << 22)))
	{
		
		
		
		if (angle != 0)
			apply_transform();
		if ((rect.width != orect.width || rect.height != orect.height) && m_rep != 0)
		{
			layer_rectchanged(orect, true);
			notifyneeds(false);
		}
	}
}

void MCImageSetMask(MCImageBitmap *p_bitmap, uint8_t *p_mask_data, uindex_t p_mask_size, bool p_is_alpha)
{
	uint32_t t_mask_stride = MCMin(p_mask_size / p_bitmap->height, p_bitmap->width);
	uint32_t t_width = t_mask_stride;

	uint8_t *t_src_ptr = p_mask_data;
	uint8_t *t_dst_ptr = (uint8_t*)p_bitmap->data;
	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		uint8_t *t_src_row = t_src_ptr;
		uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
		for (uindex_t x = 0; x < t_width; x++)
		{
			uint32_t t_alpha = *t_src_row++;
			
			if (!p_is_alpha && t_alpha > 0)
				t_alpha = 0xFF;
			uint32_t t_pixel = (*t_dst_row & 0x00FFFFFF) | (t_alpha << 24);
			*t_dst_row++ = t_pixel;
		}
		t_src_ptr += t_mask_stride;
		t_dst_ptr += p_bitmap->stride;
	}

	MCImageBitmapCheckTransparency(p_bitmap);
}

Exec_stat MCImage::getprop(uint4 parid, Properties which, MCExecPoint& ep, Boolean effective)
{
	uint2 i;
	uint4 size = 0;

	switch (which)
	{
	case P_XHOT:
		ep.setint(xhot);
		break;
	case P_YHOT:
		ep.setint(yhot);
		break;
	case P_HOT_SPOT:
		ep.setpoint(xhot, yhot);
		break;
	case P_FILE_NAME:
		if (m_rep == 0 || m_rep->GetType() != kMCImageRepReferenced)
			ep.clear();
		else
			ep.setcstring(filename);
		break;
	case P_ALWAYS_BUFFER:
		ep.setboolean(getflag((1UL << 24)));
		break;
	case P_IMAGE_PIXMAP_ID:
		ep.clear();
	case P_MASK_PIXMAP_ID:
		ep.clear();
		break;
	case P_DONT_DITHER:
		ep.setboolean(getflag((1UL << 14)));
		break;
	case P_MAGNIFY:
		ep.setboolean(getstate((1UL << 14)));
		break;
	case P_SIZE:
		{
			void *t_data = 0;
			uindex_t t_size = 0;
			MCImageCompressedBitmap *t_compressed = 0;

			if (m_rep != 0)
			{
				if (m_rep->GetType() == kMCImageRepResident)
					static_cast<MCResidentImageRep*>(m_rep)->GetData(t_data, t_size);
				else if (m_rep->GetType() == kMCImageRepVector)
					static_cast<MCVectorImageRep*>(m_rep)->GetData(t_data, t_size);
				else if (m_rep->GetType() == kMCImageRepCompressed)
				{
					t_compressed = static_cast<MCCompressedImageRep*>(m_rep)->GetCompressed();
					if (t_compressed->size != 0)
						t_size = t_compressed->size;
					else
					{
						i = t_compressed->color_count;
						while (i--)
							t_size += t_compressed->plane_sizes[i];
					}
				}
			}

			ep.setuint(t_size);
		}
		break;
	case P_CURRENT_FRAME:
		ep.setint(currentframe + 1);
		break;
	case P_FRAME_COUNT:
		if (m_rep == 0 || m_rep->GetFrameCount() <= 1)
			ep.setuint(0);
		else
			ep.setuint(m_rep->GetFrameCount());
		break;
	case P_PALINDROME_FRAMES:
		ep.setboolean(getflag((1UL << 21)));
		break;
	case P_CONSTANT_MASK:
		ep.setboolean(getflag((1UL << 25)));
		break;
	case P_REPEAT_COUNT:
		ep.setint(repeatcount);
		break;
	case P_FORMATTED_HEIGHT:
		{
			uindex_t t_width = 0, t_height = 0;
			if (m_rep != 0)
				m_rep->GetGeometry(t_width, t_height);

			ep.setint(t_height);
		}
		break;
	case P_FORMATTED_WIDTH:
		{
			uindex_t t_width = 0, t_height = 0;
			if (m_rep != 0)
				m_rep->GetGeometry(t_width, t_height);

			ep.setint(t_width);
		}
		break;
	case P_TEXT:
		recompress();
		if (m_rep == 0 || m_rep->GetType() == kMCImageRepReferenced)
			ep.clear();
		else
		{
			void *t_data = 0;
			uindex_t t_size = 0;
			if (m_rep->GetType() == kMCImageRepResident)
				static_cast<MCResidentImageRep*>(m_rep)->GetData(t_data, t_size);
			else if (m_rep->GetType() == kMCImageRepVector)
				static_cast<MCVectorImageRep*>(m_rep)->GetData(t_data, t_size);
			else if (m_rep->GetType() == kMCImageRepCompressed)
			{
				MCImageCompressedBitmap *t_compressed = 0;
				t_compressed = static_cast<MCCompressedImageRep*>(m_rep)->GetCompressed();
				if (t_compressed->data != 0)
				{
					t_data = t_compressed->data;
					t_size = t_compressed->size;
				}
				else
				{
					t_data = t_compressed->planes[0];
					t_size = t_compressed->plane_sizes[0];
				}
			}
			ep.copysvalue((char*)t_data, t_size);
		}
		break;
	case P_IMAGE_DATA:
		{
			
			
			uint32_t t_pixel_count = rect.width * rect.height;
			uint32_t t_data_size = t_pixel_count * sizeof(uint32_t);
			
			bool t_success = true;
			
			uint32_t *t_data_ptr = 0;
			t_success = 0 != (t_data_ptr = (uint32_t*)ep.getbuffer(t_data_size));
			
			if (t_success)
			{
				if (m_rep == 0)
					MCMemoryClear(t_data_ptr, t_data_size);
				else
				{
					openimage();
					
					MCImageBitmap *t_bitmap = 0;
					
					t_success = lockbitmap(t_bitmap, false);
					if (t_success)
					{
						MCMemoryCopy(t_data_ptr, t_bitmap->data, t_data_size);







#line 731 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
					}
					unlockbitmap(t_bitmap);
					
					closeimage();
				}
			}
			if (t_success)
				ep.setlength(t_data_size);
		}
		break;
	case P_MASK_DATA:
	case P_ALPHA_DATA:
		{
			uint32_t t_pixel_count = rect.width * rect.height;
			uint32_t t_data_size = t_pixel_count;
			
			bool t_success = true;
			
			uint8_t *t_data_ptr = 0;
			t_success = 0 != (t_data_ptr = (uint8_t*)ep.getbuffer(t_data_size));
			
			if (t_success)
			{
				if (m_rep == 0)
					MCMemoryClear(t_data_ptr, t_data_size);
				else
				{
					openimage();
					
					MCImageBitmap *t_bitmap = 0;
					
					t_success = lockbitmap(t_bitmap, false);
					if (t_success)
					{
						uint8_t *t_src_ptr = (uint8_t*)t_bitmap->data;
						for (uindex_t y = 0; y < t_bitmap->height; y++)
						{
							uint32_t *t_src_row = (uint32_t*)t_src_ptr;
							for (uindex_t x = 0; x < t_bitmap->width; x++)
							{
								uint8_t t_alpha = *t_src_row++ >> 24;
								if (which == P_MASK_DATA && t_alpha > 0)
									*t_data_ptr++ = 0xFF;
								else
									*t_data_ptr++ = t_alpha;
							}
							t_src_ptr += t_bitmap->stride;
						}
					}
					unlockbitmap(t_bitmap);
					
					closeimage();
				}
			}
			if (t_success)
				ep.setlength(t_data_size);
		}
		break;
	case P_RESIZE_QUALITY:
		if (resizequality == 0)
			ep.setstaticcstring("normal");
		else if (resizequality == 1)
			ep.setstaticcstring("good");
		else if (resizequality == 2)
			ep.setstaticcstring("best");
		break;
	case P_PAINT_COMPRESSION:
		switch (getcompression())
		{
		case (3UL << 17):
			ep.setstaticcstring("png");
			break;
		case (2UL << 17):
			ep.setstaticcstring("jpeg");
			break;
		case (1UL << 17):
			ep.setstaticcstring("gif");
			break;
		case (1UL << 22):
			ep.setstaticcstring("pict");
			break;
		default:
			ep.setstaticcstring("rle");
			break;
		}
		break;
	case P_ANGLE:
		ep.setint(angle);
		break;
	default:
		return MCControl::getprop(parid, which, ep, effective);
	}
	return ES_NORMAL;
}

Exec_stat MCImage::setprop(uint4 parid, Properties p, MCExecPoint &ep, Boolean effective)
{
	Boolean dirty = 0;
	uint2 i;
	MCString data = ep.getsvalue();
	uint4 newstate = state;

	switch (p)
	{
	case P_INVISIBLE:
	case P_VISIBLE:
		{
			Boolean wasvisible = isvisible();
			Exec_stat stat = MCControl::setprop(parid, p, ep, effective);
			if (!(MCbufferimages || flags & (1UL << 24))
			        && !isvisible() && m_rep != 0)
				closeimage();
			if (state & (1UL << 21) && opened > 0)
			{
				
				layer_redrawall();
			}
			if (isvisible() && !wasvisible && m_rep != 0 && m_rep->GetFrameCount() > 1)
			{
				MCImageFrame *t_frame = 0;
				if (m_rep->LockImageFrame(currentframe, true, t_frame))
				{
					MCscreen->addtimer(this, MCM_internal, t_frame->duration);
					m_rep->UnlockImageFrame(currentframe, t_frame);
				}
			}
			return stat;
		}
	case P_XHOT:
		{
			if (!MCU_stoi2(data, xhot))
			{
				MCeerror->add(EE_IMAGE_XHOTNAN, 0, 0, data);
				return ES_ERROR;
			}
			uint32_t t_pixwidth, t_pixheight;
			getgeometry(t_pixwidth, t_pixheight);
			xhot = MCMax(1, MCMin(xhot, (int32_t)t_pixwidth));
		}
		break;
	case P_YHOT:
		{
			if (!MCU_stoi2(data, yhot))
			{
				MCeerror->add(EE_IMAGE_YHOTNAN, 0, 0, data);
				return ES_ERROR;
			}
			uint32_t t_pixwidth, t_pixheight;
			getgeometry(t_pixwidth, t_pixheight);
			yhot = MCMax(1, MCMin(yhot, (int32_t)t_pixheight));
		}
		break;
	case P_HOT_SPOT:
		{
			if (!MCU_stoi2x2(data, xhot, yhot))
			{
				MCeerror->add(EE_IMAGE_HOTNAP, 0, 0, data);
				return ES_ERROR;
			}
			uint32_t t_pixwidth, t_pixheight;
			getgeometry(t_pixwidth, t_pixheight);
			xhot = MCMax(1, MCMin(xhot, (int32_t)t_pixwidth));
			yhot = MCMax(1, MCMin(yhot, (int32_t)t_pixheight));
		}
		break;
	case P_FILE_NAME:
		





		if (m_rep == 0 || m_rep->GetType() != kMCImageRepReferenced ||
			data != filename)
		{
			char *t_filename = 0;
			if (data != MCnullmcstring)
				 t_filename = data.clone();

			setfilename(t_filename);
                
			MCCStringFree(t_filename);

			resetimage();

			if (m_rep != 0)
				MCresult->clear(0);
			else
				MCresult->sets("could not open image");
		}
		break;
	case P_ALWAYS_BUFFER:
		if (!MCU_matchflags(data, flags, (1UL << 24), dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_IMAGE_PIXMAP_ID:
		break;
	case P_MASK_PIXMAP_ID:
		break;
	case P_DONT_DITHER:
		if (!MCU_matchflags(data, flags, (1UL << 14), dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty)
			dirty = 0;
		break;
	case P_MAGNIFY:
		if (!MCU_matchflags(data, newstate, (1UL << 14), dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty)
		{
			if (newstate & (1UL << 14))
				startmag(rect.width >> 1, rect.height >> 1);
			else
				endmag(1);
		}
		break;
	case P_CURRENT_FRAME:
		if (!MCU_stoui2(data, i))
		{
			MCeerror->add
			(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		setframe(i - 1);
		break;
	case P_PALINDROME_FRAMES:
		if (!MCU_matchflags(data, flags, (1UL << 21), dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_CONSTANT_MASK:
		if (!MCU_matchflags(data, flags, (1UL << 25), dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_REPEAT_COUNT:
		{
			int2 rc;
			if (!MCU_stoi2(data, rc))
			{
				MCeerror->add
				(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
			if (rc < 0)
				flags &= ~(1UL << 20);
			else
				flags |= (1UL << 20);
			irepeatcount = repeatcount = rc;
			if (opened && m_rep != 0 && m_rep->GetFrameCount() > 1 && repeatcount != 0)
			{
				setframe(currentframe == m_rep->GetFrameCount() - 1 ? 0 : currentframe + 1);
				MCImageFrame *t_frame = 0;
				if (m_rep->LockImageFrame(currentframe, true, t_frame))
				{
					MCscreen->addtimer(this, MCM_internal, t_frame->duration);
					m_rep->UnlockImageFrame(currentframe, t_frame);
				}
			}
		}
		break;
	case P_TEXT:
		{
			bool t_success = true;

			MCImageBitmap *t_bitmap = 0;
			MCImageCompressedBitmap *t_compressed = 0;
			MCPoint t_hotspot;
			char *t_name = 0;
			IO_handle t_stream = 0;

			if (data.getlength() == 0)
			{
				










				
				flags &= ~((3UL << 17 | 1UL << 22) | (1UL << 15) | (1UL << 16));
				setrep(0);
			}
			else
			{
				if (t_success)
					t_success = 0 != (t_stream = MCS_fakeopen(data));
				if (t_success)
					t_success = MCImageImport(t_stream, 0, t_hotspot, t_name, t_compressed, t_bitmap);
				if (t_success)
				{
					if (t_compressed != 0)
						t_success = setcompressedbitmap(t_compressed);
					else if (t_bitmap != 0)
						t_success = setbitmap(t_bitmap);
				}

				MCImageFreeBitmap(t_bitmap);
				MCImageFreeCompressedBitmap(t_compressed);
				MCCStringFree(t_name);
				if (t_stream != 0)
					MCS_close(t_stream);
			}

			if (t_success)
			{
				resetimage();
				dirty = 0;
			}
		}
		break;
	case P_IMAGE_DATA:
		if (data.getlength())
		{
			bool t_success = true;

			MCImageBitmap *t_copy = 0;
			if (m_rep != 0)
			{
				MCImageBitmap *t_bitmap = 0;
				t_success = lockbitmap(t_bitmap, false);
				if (t_success)
					t_success = MCImageCopyBitmap(t_bitmap, t_copy);
				unlockbitmap(t_bitmap);
			}
			else
			{
				t_success = MCImageBitmapCreate(rect.width, rect.height, t_copy);
				if (t_success)
					MCImageBitmapSet(t_copy, MCGPixelPack((0), 0, 0, 0, 255)); 
			}

			if (t_success)
			{
				uint32_t t_stride = MCMin(data.getlength() / t_copy->height, t_copy->width * 4);
				uint32_t t_width = t_stride / 4;

				uint8_t *t_src_ptr = (uint8_t*)data.getstring();
				uint8_t *t_dst_ptr = (uint8_t*)t_copy->data;
				for (uindex_t y = 0; y < t_copy->height; y++)
				{
					uint8_t *t_src_row = t_src_ptr;
					uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
					for (uindex_t x = 0; x < t_width; x++)
					{
						uint8_t a, r, g, b;
						a = *t_src_row++;
						r = *t_src_row++;
						g = *t_src_row++;
						b = *t_src_row++;

						*t_dst_row++ = MCGPixelPack((0), r, g, b, 255);
					}
					t_src_ptr += t_stride;
					t_dst_ptr += t_copy->stride;
				}

				setbitmap(t_copy);
			}

			MCImageFreeBitmap(t_copy);

			resetimage();
			dirty = 0;
		}
		break;
	case P_MASK_DATA:
		if (data.getlength())
		{
			bool t_success = true;

			MCImageBitmap *t_copy = 0;
			if (m_rep != 0)
			{
				MCImageBitmap *t_bitmap = 0;
				t_success = lockbitmap(t_bitmap, false);
				if (t_success)
					t_success = MCImageCopyBitmap(t_bitmap, t_copy);
				unlockbitmap(t_bitmap);
			}
			else
			{
				t_success = MCImageBitmapCreate(rect.width, rect.height, t_copy);
				if (t_success)
					MCImageBitmapSet(t_copy, 0xFF000000); 
			}

			if (t_success)
			{
				MCImageSetMask(t_copy, (uint8_t*)data.getstring(), data.getlength(), false);
				setbitmap(t_copy);
			}

			MCImageFreeBitmap(t_copy);

			resetimage();
			dirty = 0;
		}
		break;
	case P_ALPHA_DATA:
		if (data.getlength())
		{
			bool t_success = true;

			MCImageBitmap *t_copy = 0;
			if (m_rep != 0)
			{
				MCImageBitmap *t_bitmap = 0;
				t_success = lockbitmap(t_bitmap, false);
				if (t_success)
					t_success = MCImageCopyBitmap(t_bitmap, t_copy);
				unlockbitmap(t_bitmap);
			}
			else
			{
				t_success = MCImageBitmapCreate(rect.width, rect.height, t_copy);
				if (t_success)
					MCImageBitmapSet(t_copy, 0xFF000000); 
			}

			if (t_success)
			{
				MCImageSetMask(t_copy, (uint8_t*)data.getstring(), data.getlength(), true);
				setbitmap(t_copy);
			}

			MCImageFreeBitmap(t_copy);

			resetimage();
			dirty = 1;
		}
		break;
	case P_RESIZE_QUALITY:
		if (data == "best")
			resizequality = 2;
		else if (data == "good")
			resizequality = 1;
		else if (data == "normal")
			resizequality = 0;
		break;
	case P_ANGLE:
		{
			int2 i1;
			if (!MCU_stoi2(data, i1))
			{
				MCeerror->add(EE_GRAPHIC_NAN, 0, 0, data);
				return ES_ERROR;
			}
			while (i1 < 0)
				i1 += 360;
			i1 %= 360;

			if (i1 != angle)
			{
				
				
				MCRectangle oldrect = rect;
				rotate_transform(i1);

				angle = i1;

				if (angle)
					flags |= (1UL << 26);
				else
					flags &= ~(1UL << 26);

				
				layer_rectchanged(oldrect, true);
				dirty = 0;

				notifyneeds(false);
			}
		}
		break;
	case P_INK:
	case P_BLEND_LEVEL:
		{
			Exec_stat t_stat = MCControl::setprop(parid, p, ep, effective);
			if (t_stat == ES_NORMAL)
				notifyneeds(false);
			return t_stat;
		}
		break;
	default:
		return MCControl::setprop(parid, p, ep, effective);
	}
	if (dirty && opened)
	{
		
		layer_redrawall();
	}
	return ES_NORMAL;
}

void MCImage::select()
{
	MCControl::select();
	if (state & (1UL << 14))
		magredrawdest(rect);
}

void MCImage::deselect()
{
	MCControl::deselect();
	if (state & (1UL << 14))
		magredrawdest(rect);
}

void MCImage::undo(Ustruct *us)
{
	switch (us->type)
	{
	case UT_PAINT:
		static_cast<MCMutableImageRep*>(m_rep)->image_undo(us);
		break;
	default:
		MCControl::undo(us);
	}
}

void MCImage::freeundo(Ustruct *us)
{
	switch (us->type)
	{
	case UT_PAINT:
		if (m_rep != 0 && m_rep->GetType() == kMCImageRepMutable)
			static_cast<MCMutableImageRep*>(m_rep)->image_freeundo(us);
		break;
	default:
		MCControl::freeundo(us);
		break;
	}
}

MCControl *MCImage::clone(Boolean attach, Object_pos p, bool invisible)
{
	recompress();
	MCImage *newiptr = new("c:\\github\\livecode-runrev\\engine\\src\\image.cpp", 1291) MCImage(*this);
	if (attach)
		newiptr->attach(p, invisible);
	return newiptr;
}

Boolean MCImage::maskrect(const MCRectangle &srect)
{
	if (!(flags & (1UL << 11) || MCshowinvisibles))
		return 0;
	MCRectangle drect = MCU_intersect_rect(srect, rect);
	if (drect.width == 0 || drect.height == 0)
		return 0;
	if (srect.width != 1 || srect.height != 1)
		return 1; 
	if (state & (1UL << 21) && !(state & (1UL << 20)))
		return 1;

	
	MCImageFrame *t_frame = 0;
	if (!getstate((1UL << 3)) && m_rep != 0 && m_rep->LockImageFrame(currentframe, true, t_frame))
	{
		int32_t t_x = srect.x - rect.x;
		int32_t t_y = srect.y - rect.y;
		if (m_has_transform)
		{
			MCGAffineTransform t_inverted = MCGAffineTransformInvert(m_transform);
			MCGPoint t_src_point = MCGPointApplyAffineTransform(MCGPointMake(t_x, t_y), t_inverted);
			t_x = t_src_point.x;
			t_y = t_src_point.y;
		}
		uint32_t t_pixel = 0;
		if (t_x >= 0 && t_y >= 0 && t_x <t_frame->image->width && t_y < t_frame->image->height)
			t_pixel = MCImageBitmapGetPixel(t_frame->image, t_x, t_y);

		m_rep->UnlockImageFrame(currentframe, t_frame);
		return (t_pixel >> 24) != 0;
	}
	else
		return 1;
}


bool MCImage::lockshape(MCObjectShape& r_shape)
{
	
	if (getflag((1UL << 5)) || getstate((1UL << 4)) && (extraflags & (1UL << 2)) == 0 ||
		getcompression() == (1UL << 22) || m_rep == 0)
	{
		r_shape . type = kMCObjectShapeComplex;
		r_shape . bounds = getrect();
		return true;
	}
	
	bool t_mask, t_alpha;
	MCImageBitmap *t_bitmap = 0;

	 lockbitmap(t_bitmap, true);
	t_mask = MCImageBitmapHasTransparency(t_bitmap, t_alpha);

	
	if (!t_mask)
	{
		r_shape . type = kMCObjectShapeRectangle;
		r_shape . bounds = getrect();
		r_shape . rectangle = r_shape . bounds;
		unlockbitmap(t_bitmap);
		return true;
	}
	else
	{
		
		r_shape . type = kMCObjectShapeMask;
		r_shape . bounds = getrect();
		r_shape . mask . origin . x = r_shape . bounds . x;
		r_shape . mask . origin . y = r_shape . bounds . y;
		r_shape . mask . bits = t_bitmap;
		return true;
	}
}

void MCImage::unlockshape(MCObjectShape& p_shape)
{
	if (p_shape . type == kMCObjectShapeMask)
		unlockbitmap(p_shape . mask . bits);
}





void MCImage::draw(MCDC *dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite)
{
	MCRectangle dirty;
	dirty = p_dirty;

	
	
	
	
	
	
	if (!p_isolated)
	{
		
		if (!p_sprite)
		{
			dc -> setfunction(ink);
			dc -> setopacity(blendlevel * 255 / 100);
		}
	}

	bool t_need_group;
	if (!p_isolated)
	{
		
		t_need_group = getflag((1UL << 5)) || getstate((1UL << 4)) || getcompression() == (1UL << 22) || m_bitmap_effects != 0;
		if (t_need_group)
		{
			if (m_bitmap_effects == 0)
				dc -> begin(false);
			else
			{
				if (!dc -> begin_with_effects(m_bitmap_effects, rect))
					return;
				dirty = dc -> getclip();
			}
		}
	}

	int2 xorigin;
	int2 yorigin;

	MCRectangle trect;
	trect = MCU_intersect_rect(dirty, getrect());
	uint32_t t_pixwidth, t_pixheight;
	getgeometry(t_pixwidth, t_pixheight);
	if (state & ((1UL << 8) | (1UL << 9) | (1UL << 10) | (1UL << 11)) && state & (1UL << 22) && (t_pixwidth != rect.width || t_pixheight != rect.height))
		compute_gravity(trect, xorigin, yorigin);
	else
	{
		xorigin = trect.x - rect.x;
		yorigin = trect.y - rect.y;
	}
	drawme(dc, xorigin, yorigin, trect.width, trect.height, trect.x, trect.y);

	if (getflag((1UL << 5)))
	{
		if (getflag((1UL << 8)))
			draw3d(dc, rect, ETCH_RAISED, borderwidth);
		else
			drawborder(dc, rect, borderwidth);
	}

	if (getstate((1UL << 4)))
		drawfocus(dc, p_dirty);

	if (!p_isolated)
	{
		if (t_need_group)
			dc -> end();

		if (isediting())
			static_cast<MCMutableImageRep*>(m_rep)->drawsel(dc);

		if (getstate((1UL << 14)))
			drawmagrect(dc);

		if (getstate((1UL << 3)))
			drawselected(dc);
	}
}









IO_stat MCImage::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part)
{
	
	
	
	
	
	
	
	
	
	
	
	IO_stat t_stat;
	t_stat = p_stream . WriteU8(resizequality);

	uint4 t_flags;
	t_flags = 0;

	uint4 t_length;
	t_length = 0;

	if (m_have_control_colors)
	{
		t_flags |= (1 << 0);
		t_length += sizeof(uint16_t) + sizeof(uint16_t);
		t_length += m_control_color_count * 3 * sizeof(uint16_t);
		for (uint16_t i = 0; i < m_control_color_count; i++)
			t_length += MCCStringLength(m_control_color_names[i]) + 1;
	}

	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteTag(t_flags, t_length);
	
	if (t_stat == IO_NORMAL && m_have_control_colors)
	{
		t_stat = p_stream . WriteU16(m_control_color_count);
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . WriteU16(m_control_color_flags);

		for (uint16_t i = 0; t_stat == IO_NORMAL && i < m_control_color_count; i++)
			t_stat = p_stream . WriteColor(m_control_colors[i]);
		for (uint16_t i = 0; t_stat == IO_NORMAL && i < m_control_color_count; i++)
			t_stat = p_stream . WriteCString(m_control_color_names[i]);
	}

	if (t_stat == IO_NORMAL)
		t_stat = MCObject::extendedsave(p_stream, p_part);

	return t_stat;
}

IO_stat MCImage::extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_remaining)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	if (p_remaining >= 1)
	{
		t_stat = p_stream . ReadU8(resizequality);
		
		if (t_stat == IO_NORMAL)
			p_remaining -= 1;
	}

	if (p_remaining > 0)
	{
		uint4 t_flags, t_length, t_header_length;
		t_stat = p_stream . ReadTag(t_flags, t_length, t_header_length);

		if (t_stat == IO_NORMAL)
			t_stat = p_stream . Mark();

		if (t_stat == IO_NORMAL && (t_flags & (1 << 0)))
		{
			m_have_control_colors = true;
			t_stat = p_stream . ReadU16(m_control_color_count);
			t_stat = p_stream . ReadU16(m_control_color_flags);

			if (t_stat == IO_NORMAL)
			{
				 MCMemoryNewArray(m_control_color_count, m_control_colors);
				 MCMemoryNewArray(m_control_color_count, m_control_color_names);
			}

			for (uint32_t i = 0; t_stat == IO_NORMAL && i < m_control_color_count; i++)
				t_stat = p_stream . ReadColor(m_control_colors[i]);
			for (uint32_t i = 0; t_stat == IO_NORMAL && i < m_control_color_count; i++)
				t_stat = p_stream . ReadCString(m_control_color_names[i]);
		}

		if (t_stat == IO_NORMAL)
			t_stat = p_stream . Skip(t_length);

		if (t_stat == IO_NORMAL)
			p_remaining -= t_length + t_header_length;
	}

	if (t_stat == IO_NORMAL)
		t_stat = MCObject::extendedload(p_stream, p_version, p_remaining);

	return t_stat;
}

IO_stat MCImage::save(IO_handle stream, uint4 p_part, bool p_force_ext)
{
	IO_stat stat;

	recompress();
	if ((stat = IO_write_uint1(OT_IMAGE, stream)) != IO_NORMAL)
		return stat;

	m_have_control_colors = false;
	if (m_rep != 0)
	{
		if (m_rep->GetType() == kMCImageRepCompressed)
		{
			MCImageCompressedBitmap *t_compressed;
			t_compressed = static_cast<MCCompressedImageRep*>(m_rep)->GetCompressed();
			if (t_compressed->colors != 0)
			{
				m_have_control_colors = true;

				m_control_color_count = ncolors;
				m_control_colors = colors;
				m_control_color_names = colornames;
				m_control_color_flags = dflags;

				ncolors = t_compressed->color_count;
				colors = t_compressed->colors;
				dflags = MCImage::cmasks[MCMin(ncolors, 7)];
				 MCMemoryNewArray(ncolors, colornames);
			}
		}
	}
	uint32_t t_pixwidth, t_pixheight;
	getgeometry(t_pixwidth, t_pixheight);

	if (getcompression() == 0
	        && (rect.width != t_pixwidth || rect.height != t_pixheight))
		flags |= (1UL << 23);

	uint1 t_old_ink = ink;


	if (MCstackfileversion < 2700)
	{
		if (ink == 0x1d)
			if (blendlevel != 50)
				ink = (100 - blendlevel) | 0x80;
			else
				ink = 0x12;
	}


	bool t_has_extension = false;
	if (resizequality != 0)
		t_has_extension = true;
	if (m_have_control_colors)
		t_has_extension = true;

	uint4 oldflags = flags;
	if (flags & (1UL << 16))
		flags &= ~((1UL << 15) | (3UL << 17 | 1UL << 22) | (1UL << 19));
	stat = MCControl::save(stream, p_part, t_has_extension || p_force_ext);

	flags = oldflags;

	ink = t_old_ink;

	if (m_have_control_colors)
	{
		MCMemoryDeleteArray(colornames);

		ncolors = m_control_color_count;
		colors = m_control_colors;
		colornames = m_control_color_names;
		dflags = m_control_color_flags;

		m_control_colors = 0;
		m_control_color_names = 0;
		m_control_color_count = 0;

		m_have_control_colors = false;
	}

	if (stat != IO_NORMAL)
		return stat;


	if (flags & (1UL << 16))
	{
		if ((stat = IO_write_string(filename, stream)) != IO_NORMAL)
			return stat;
	}
	else
	{
		if (m_rep != 0)
		{
			MCImageRepType t_type = m_rep->GetType();
			void *t_data = 0;
			uindex_t t_size = 0;
			void *t_mask_data = 0;
			uindex_t t_mask_size = 0;
			MCImageCompressedBitmap *t_compressed = 0;

			if (t_type == kMCImageRepResident)
				static_cast<MCResidentImageRep*>(m_rep)->GetData(t_data, t_size);
			else if (t_type == kMCImageRepVector)
				static_cast<MCVectorImageRep*>(m_rep)->GetData(t_data, t_size);
			else if (t_type == kMCImageRepCompressed)
			{
				t_compressed = static_cast<MCCompressedImageRep*>(m_rep)->GetCompressed();
				if (t_compressed->size > 0)
				{
					t_data = t_compressed->data;
					t_size = t_compressed->size;
				}
			}

			if (t_size > 0)
			{
				if (flags & (1UL << 20))
					if ((stat = IO_write_int2(repeatcount, stream)) != IO_NORMAL)
						return stat;
				if ((stat = IO_write_uint4(t_size, stream)) != IO_NORMAL)
					return stat;
				if ((stat = IO_write(t_data, sizeof(uint1), t_size, stream)) != IO_NORMAL)
					return stat;
			}
			else if (t_compressed != 0)
			{
				uint2 i;
				for (i = 0 ; i < t_compressed->color_count ; i++)
				{
					if ((stat = IO_write_uint4(t_compressed->plane_sizes[i], stream)) != IO_NORMAL)
						return stat;
					if ((stat = IO_write(t_compressed->planes[i], sizeof(uint1),
					                     t_compressed->plane_sizes[i], stream)) != IO_NORMAL)
						return stat;
				}

				t_mask_data = t_compressed->mask;
				t_mask_size = t_compressed->mask_size;
			}
			if ((stat = IO_write_uint4(t_mask_size, stream)) != IO_NORMAL)
				return stat;

			if (t_mask_size != 0)
				if ((stat = IO_write(t_mask_data, sizeof(uint1), t_mask_size, stream)) != IO_NORMAL)
					return stat;
			if (flags & (1UL << 23))
			{
				if ((stat = IO_write_uint2(t_pixwidth, stream)) != IO_NORMAL)
					return stat;
				if ((stat = IO_write_uint2(t_pixheight, stream)) != IO_NORMAL)
					return stat;
			}
		}
	}
	if ((stat = IO_write_uint2(xhot, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(yhot, stream)) != IO_NORMAL)
		return stat;
	if (flags & (1UL << 26))
		if ((stat = IO_write_uint2(angle, stream)) != IO_NORMAL)
			return stat;
	return savepropsets(stream);
}

IO_stat MCImage::load(IO_handle stream, const char *version)
{
	IO_stat stat;

	resizequality = 0;

	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return stat;


	if (ink & 0x80 && strncmp(version, "2.7", 3) < 0)
	{
		blendlevel = 100 - (ink & 0x7F);
		ink = 0x1d;
	}

	if (flags & (1UL << 16))
	{
		char *t_filename = 0;
		if ((stat = IO_read_string(t_filename, stream)) != IO_NORMAL)
			return stat;

		 setfilename(t_filename);
		MCCStringFree(t_filename);
	}
	else
		if (ncolors || flags & (3UL << 17 | 1UL << 22) || flags & (1UL << 15))
		{
			MCImageCompressedBitmap *t_compressed = 0;
			 MCImageCreateCompressedBitmap(flags & (3UL << 17 | 1UL << 22), t_compressed);
			if (ncolors > 8 || flags & (3UL << 17 | 1UL << 22)
			        || flags & (1UL << 15))
			{
				
				repeatcount = -1;
				if (flags & (1UL << 20))
					if ((stat = IO_read_int2(&repeatcount, stream)) != IO_NORMAL)
						return stat;

				if ((stat = IO_read_uint4(&t_compressed->size, stream)) != IO_NORMAL)
					return stat;
				 MCMemoryAllocate(t_compressed->size, t_compressed->data);
				if (IO_read(t_compressed->data, sizeof(uint1),
				            t_compressed->size, stream) != IO_NORMAL)
					return IO_ERROR;
				if (strncmp(version, "1.4", 3) == 0)
				{
					if ((ncolors == 16 || ncolors == 256) && noblack())
						flags |= (1UL << 19);
					_dbg_MCU_realloc((char **)&colors, ncolors, ncolors + 1, sizeof(MCColor), "c:\\github\\livecode-runrev\\engine\\src\\image.cpp", 1791);
					colors[ncolors].pixel = 0;
				}
			}
			else
			{
				t_compressed->color_count = ncolors;
				 MCMemoryNewArray(ncolors, t_compressed->planes);
				 MCMemoryNewArray(ncolors, t_compressed->plane_sizes);

				uint2 i;
				for (i = 0 ; i < ncolors ; i++)
				{
					if ((stat = IO_read_uint4(&t_compressed->plane_sizes[i], stream)) != IO_NORMAL)
					{
						MCImageFreeCompressedBitmap(t_compressed);
						return stat;
					}
					if (t_compressed->plane_sizes[i] != 0)
					{
						 MCMemoryAllocate(t_compressed->plane_sizes[i], t_compressed->planes[i]);
						if (IO_read(t_compressed->planes[i], sizeof(uint1),
						            t_compressed->plane_sizes[i], stream) != IO_NORMAL)
						{
							MCImageFreeCompressedBitmap(t_compressed);
							return IO_ERROR;
						}
					}
				}
			}
			if (t_compressed->compression == (0UL << 17) && ncolors != 0 && (flags & (1UL << 15)) == 0)
			{
				t_compressed->color_count = ncolors;
				 MCMemoryAllocateCopy(colors, sizeof(MCColor) * ncolors, t_compressed->colors);
			}

			if ((stat = IO_read_uint4(&t_compressed->mask_size, stream)) != IO_NORMAL)
				return stat;
			if (t_compressed->mask_size != 0)
			{
				 MCMemoryAllocate(t_compressed->mask_size, t_compressed->mask);
				if (IO_read(t_compressed->mask, sizeof(uint1), t_compressed->mask_size, stream) != IO_NORMAL)
					return IO_ERROR;
			}

			uint16_t t_pixwidth, t_pixheight;
			t_pixwidth = rect.width;
			t_pixheight = rect.height;
			if (flags & (1UL << 23))
			{
				if ((stat = IO_read_uint2(&t_pixwidth, stream)) != IO_NORMAL)
					return stat;
				if ((stat = IO_read_uint2(&t_pixheight, stream)) != IO_NORMAL)
					return stat;
			}
			t_compressed->width = t_pixwidth;
			t_compressed->height = t_pixheight;

			 setcompressedbitmap(t_compressed);
			MCImageFreeCompressedBitmap(t_compressed);
		}
	if ((stat = IO_read_int2(&xhot, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_int2(&yhot, stream)) != IO_NORMAL)
		return stat;
	if (flags & (1UL << 26))
		if ((stat = IO_read_uint2(&angle, stream)) != IO_NORMAL)
			return stat;

	if (m_have_control_colors)
	{
		MCMemoryDeallocate(colors);
		for (uint32_t i = 0; i < ncolors; i++)
			MCCStringFree(colornames[i]);
		MCMemoryDeallocate(colornames);

		colors = m_control_colors;
		colornames = m_control_color_names;
		ncolors = m_control_color_count;
		dflags = m_control_color_flags;

		m_control_colors = 0;
		m_control_color_names = 0;
		m_control_color_count = 0;
		m_have_control_colors = false;
	}

	return loadpropsets(stream);
}



bool MCImage::recomputefonts(MCFontRef p_parent_font)
{
	return false;
}



MCSharedString *MCImage::getclipboardtext(void)
{
	MCSharedString *t_data = 0;
	recompress();
	if (getcompression() == (0UL << 17))
	{
		bool t_success = true;

		MCImageBitmap *t_bitmap = 0;

		t_success = lockbitmap(t_bitmap, false);
		if (t_success)
			t_success = MCImageCreateClipboardData(t_bitmap, t_data);
		unlockbitmap(t_bitmap);
	}
	else if (m_rep != 0)
	{
		MCImageRepType t_type = m_rep->GetType();
		void *t_bytes = 0;
		uindex_t t_size = 0;
		if (t_type == kMCImageRepResident)
			static_cast<MCResidentImageRep*>(m_rep)->GetData(t_bytes, t_size);
		else if (t_type == kMCImageRepVector)
			static_cast<MCVectorImageRep*>(m_rep)->GetData(t_bytes, t_size);

		t_data = MCSharedString::Create(t_bytes, t_size);
	}

	return t_data;
}



void MCImage::apply_transform()
{
	uindex_t t_width = rect.width;
	uindex_t t_height = rect.height;
	if (m_rep != 0)
		m_rep->GetGeometry(t_width, t_height);

	if (angle != 0)
		rotate_transform(angle);
	else if (rect.width != t_width || rect.height != t_height)
		resize_transform();
	else
		m_has_transform = false;
}



void MCImage::sourcerectchanged(MCRectangle p_new_rect)
{
	MCRectangle t_old_rect = rect;
	rect = p_new_rect;
	layer_rectchanged(t_old_rect, true);
}

void MCImage::invalidate_rep(MCRectangle &p_rect)
{
	layer_redrawrect(p_rect);

	if (state & (1UL << 14))
		magredrawdest(p_rect);
}

bool MCImage::isediting() const
{
	return m_rep != 0 && m_rep->GetType() == kMCImageRepMutable;
}

bool MCImage::convert_to_mutable()
{
	
	if (m_rep != 0 && m_rep->GetType() == kMCImageRepReferenced)
		return false;

	if (m_rep != 0 && m_rep->GetType() == kMCImageRepMutable)
		return true;

	bool t_success = true;

	MCMutableImageRep *t_rep = 0;
	MCImageBitmap *t_bitmap = 0;
	if (m_rep != 0)
	{
		t_success = lockbitmap(t_bitmap, true);
		if (t_success)
			t_success = 0 != (t_rep = new("c:\\github\\livecode-runrev\\engine\\src\\image.cpp", 1977) MCMutableImageRep(this, t_bitmap));
		unlockbitmap(t_bitmap);
	}
	else
	{
		t_success = MCImageBitmapCreate(rect.width, rect.height, t_bitmap);
		if (t_success)
		{
			MCImageBitmapClear(t_bitmap);
			t_success = 0 != (t_rep = new("c:\\github\\livecode-runrev\\engine\\src\\image.cpp", 1986) MCMutableImageRep(this, t_bitmap));
		}
		MCImageFreeBitmap(t_bitmap);
	}

	if (t_success)
	{
		setrep(t_rep);
		
		
		angle = 0;
	}

	return t_success;
}

void MCImage::startediting(uint16_t p_which)
{
	bool t_success = convert_to_mutable();

	if (t_success)
	{
		static_cast<MCMutableImageRep*>(m_rep)->image_mfocus(mx, my);
		static_cast<MCMutableImageRep*>(m_rep)->image_mdown(p_which);
	}

	 (void)( (!!(t_success)) || (__MCAssert("c:\\github\\livecode-runrev\\engine\\src\\image.cpp", 2012, "t_success"), 0) );
}

void MCImage::finishediting()
{
	if (!isediting())
		return;

	if (MCeditingimage == this)
		MCeditingimage = 0;

	if (MCactiveimage == this)
		MCactiveimage = 0;

	bool t_success = true;

	MCImageRep *t_rep = m_rep;
	MCImageFrame *t_frame = 0;

	t_success = t_rep->LockImageFrame(0, false, t_frame);
	if (t_success)
		t_success = setbitmap(t_frame->image);
	t_rep->UnlockImageFrame(0, t_frame);

	 (void)( (!!(t_success)) || (__MCAssert("c:\\github\\livecode-runrev\\engine\\src\\image.cpp", 2036, "t_success"), 0) );
}



void MCImage::setrep(MCImageRep *p_rep)
{
	if (p_rep == m_rep)
		return;

	MCImageRep *t_rep = 0;
	if (p_rep != 0)
		t_rep = p_rep->Retain();

	if (m_rep != 0)
		m_rep->Release();

	m_rep = t_rep;

	m_has_transform = false;

	
	
	if (currentframe != 0)
		setframe(currentframe);

	notifyneeds(false);
}

bool MCImage::setfilename(const char *p_filename)
{
	bool t_success = true;

	if (p_filename == 0)
	{
		setrep(0);
		flags &= ~((3UL << 17 | 1UL << 22) | (1UL << 15) | (1UL << 19));
		flags &= ~(1UL << 16);
		return true;
	}

	char *t_filename = 0;
	char *t_resolved = 0;
	MCImageRep *t_rep = 0;

	t_success = MCCStringClone(p_filename, t_filename);
	if (t_success)
		t_success = 0 != (t_resolved = getstack() -> resolve_filename(p_filename));
	
	
	










	if (t_success)
		t_success = MCImageRepGetReferenced(t_resolved, t_rep);

	MCCStringFree(t_resolved);

	if (t_success)
	{
		setrep(t_rep);
		t_rep->Release();
		flags &= ~((3UL << 17 | 1UL << 22) | (1UL << 15) | (1UL << 19));
		flags |= (1UL << 16);

		if (filename != 0)
			MCCStringFree(filename);
		filename = t_filename;
	}
	else
		MCCStringFree(t_filename);

	return t_success;
}


bool MCImage::setdata(void *p_data, uindex_t p_size)
{
	bool t_success = true;

	MCImageRep *t_rep = 0;

	t_success = MCImageRepGetResident(p_data, p_size, t_rep);

	if (t_success)
	{
		setrep(t_rep);
		t_rep->Release();
	}

	return t_success;
}

bool MCImage::setbitmap(MCImageBitmap *p_bitmap, bool p_update_geometry)
{
	bool t_success = true;

	MCImageCompressedBitmap *t_compressed = 0;

	t_success = MCImageCompress(p_bitmap, (flags & (1UL << 14)) == 0, t_compressed);

	if (t_success)
		t_success = setcompressedbitmap(t_compressed);

	MCImageFreeCompressedBitmap(t_compressed);

	if (t_success)
	{
		if (p_update_geometry)
		{
	


			if (!(flags & (1UL << 27)))
	#line 2159 "c:\\github\\livecode-runrev\\engine\\src\\image.cpp"
			{
				rect . width = p_bitmap->width;
				rect . height = p_bitmap->height;
			}
		}

		angle = 0;
	}

	return t_success;
}

bool MCImage::setcompressedbitmap(MCImageCompressedBitmap *p_compressed)
{
	bool t_success = true;

	MCImageRep *t_rep = 0;

	switch (p_compressed->compression)
	{
	case (1UL << 17):
	case (3UL << 17):
	case (2UL << 17):
		t_success = MCImageRepGetResident(p_compressed->data, p_compressed->size, t_rep);
		break;
	case (1UL << 22):
		t_success = MCImageRepGetVector(p_compressed->data, p_compressed->size, t_rep);
		break;
	case (0UL << 17):
		t_success = MCImageRepGetCompressed(p_compressed, t_rep);
		break;
	default:
		t_success = false;
	}

	if (t_success)
	{
		setrep(t_rep);
		t_rep->Release();
		flags &= ~((1UL << 16) | (3UL << 17 | 1UL << 22) | (1UL << 15) | (1UL << 19));
		flags |= p_compressed->compression;

		if (p_compressed->compression == (0UL << 17))
		{
			if (p_compressed->color_count == 0)
				flags |= (1UL << 15);
		}
	}

	return t_success;
}



bool MCImage::lockbitmap(MCImageBitmap *&r_bitmap, bool p_premultiplied, bool p_update_transform)
{
	if (p_update_transform)
		apply_transform();

	if (m_rep != 0)
	{
		if (!m_rep->LockImageFrame(currentframe, p_premultiplied || m_has_transform, m_locked_frame))
			return false;

		if (!m_has_transform)
		{
			r_bitmap = m_locked_frame->image;
			return true;
		}

		
		bool t_success = true;

		MCGContextRef t_context = 0;
		MCGRaster t_raster;

		uint32_t t_trans_width = rect.width;
		uint32_t t_trans_height = rect.height;

		
		if ((state & ((1UL << 8) | (1UL << 9) | (1UL << 10) | (1UL << 11))) && (state & (1UL << 22)))
		{
			
			t_trans_width = m_current_width;
			t_trans_height = m_current_height;
		}

		__MCLog("c:\\github\\livecode-runrev\\engine\\src\\image.cpp", 2246, "locking transformed image: (%d,%d) -> (%d,%d)", m_locked_frame->image->width, m_locked_frame->image->height, t_trans_width, t_trans_height);

		t_success = MCImageBitmapCreate(t_trans_width, t_trans_height, m_transformed_bitmap);
		MCImageBitmapClear(m_transformed_bitmap);

		if (t_success)
			t_success = MCGContextCreateWithPixels(t_trans_width, t_trans_height, m_transformed_bitmap->stride, m_transformed_bitmap->data, true, t_context);

		if (t_success)
		{
			t_raster.width = m_locked_frame->image->width;
			t_raster.height = m_locked_frame->image->height;
			t_raster.stride = m_locked_frame->image->stride;
			t_raster.pixels = m_locked_frame->image->data;
			t_raster.format = kMCGRasterFormat_ARGB;

			MCGRectangle t_dst = MCGRectangleMake(0, 0, m_locked_frame->image->width, m_locked_frame->image->height);
			MCGImageFilter t_filter = resizequality == 2 ? kMCGImageFilterBicubic : (resizequality == 1 ? kMCGImageFilterBilinear : kMCGImageFilterNearest);
			MCGContextConcatCTM(t_context, m_transform);
			MCGContextDrawPixels(t_context, t_raster, t_dst, t_filter);
		}

		MCGContextRelease(t_context);

		if (t_success)
		{
			MCImageBitmapCheckTransparency(m_transformed_bitmap);
			if (!p_premultiplied)
				MCImageBitmapUnpremultiply(m_transformed_bitmap);
			r_bitmap = m_transformed_bitmap;
			return true;
		}

		MCImageFreeBitmap(m_transformed_bitmap);
		m_transformed_bitmap = 0;
	}

	return false;
}

void MCImage::unlockbitmap(MCImageBitmap *p_bitmap)
{
	if (p_bitmap == 0 || m_locked_frame == 0)
		return;

	MCImageFreeBitmap(m_transformed_bitmap);
	m_transformed_bitmap = 0;

	if (m_rep != 0)
	{
		m_rep->UnlockImageFrame(currentframe, m_locked_frame);
		m_locked_frame = 0;
	}
}



uint32_t MCImage::getcompression()
{
	uint32_t t_compression = (0UL << 17);

	if (m_rep != 0)
	{
		switch (m_rep->GetType())
		{
		case kMCImageRepCompressed:
			t_compression = (0UL << 17);
			break;
		case kMCImageRepVector:
			t_compression = (1UL << 22);
			break;
		case kMCImageRepReferenced:
		case kMCImageRepResident:
			t_compression = static_cast<MCEncodedImageRep*>(m_rep)->GetDataCompression();
			break;
		}
	}

	return t_compression;
}

MCString MCImage::getrawdata()
{
	if (m_rep == 0 || m_rep->GetType() != kMCImageRepResident)
		return MCString(0, 0);
	
	void *t_data;
	uindex_t t_size;
	static_cast<MCResidentImageRep*>(m_rep)->GetData(t_data, t_size);
	
	return MCString((char*)t_data, t_size);
}

void MCImage::getgeometry(uint32_t &r_pixwidth, uint32_t &r_pixheight)
{
	
	if ((state & (1UL << 22)) && (state & ((1UL << 8) | (1UL << 9) | (1UL << 10) | (1UL << 11))))
	{
		r_pixwidth = m_current_width;
		r_pixheight = m_current_height;
		return;
	}

	if (m_rep != 0 && m_rep->GetGeometry(r_pixwidth, r_pixheight))
		return;

	r_pixwidth = rect.width;
	r_pixheight = rect.height;
}



