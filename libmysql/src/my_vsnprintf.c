/* Copyright (C) 2000 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include <my_global.h>
#include <m_string.h>
#include <stdarg.h>
#include <m_ctype.h>

/*
  Limited snprintf() implementations

  exported to plugins as a service, see the detailed documentation
  around my_snprintf_service_st
*/

size_t my_vsnprintf(char *to, size_t n, const char* fmt, va_list ap)
{
  char *start=to, *end=to+n-1;
  size_t length, width;
  uint pre_zero, have_longlong;

  for (; *fmt ; fmt++)
  {
    if (*fmt != '%')
    {
      if (to == end)                            /* End of buffer */
	break;
      *to++= *fmt;                            /* Copy ordinary char */
      continue;
    }
    fmt++;					/* skip '%' */
    /* Read max fill size (only used with %d and %u) */
    if (*fmt == '-')
      fmt++;
    length= width= 0;
    pre_zero= have_longlong= 0;
    if (*fmt == '*')
    {
      fmt++;
      length= va_arg(ap, int);
    }
    else
      for (; my_isdigit(&my_charset_latin1, *fmt); fmt++)
      {
        length= length * 10 + (uint)(*fmt - '0');
        if (!length)
          pre_zero= 1;                         /* first digit was 0 */
      }
    if (*fmt == '.')
    {
      fmt++;
      if (*fmt == '*')
      {
        fmt++;
        width= va_arg(ap, int);
      }
      else
      {
        for (; my_isdigit(&my_charset_latin1, *fmt); fmt++)
          width= width * 10 + (uint)(*fmt - '0');
      }
    }
    else
      width= SIZE_T_MAX;
    if (*fmt == 'l')
    {
      fmt++;
      if (*fmt != 'l')
        have_longlong= (sizeof(long) == sizeof(longlong));
      else
      {
        fmt++;
        have_longlong= 1;
      }
    }
    else if (*fmt == 'z')
    {
      fmt++;
      have_longlong= (sizeof(size_t) == sizeof(longlong));
    }
    if (*fmt == 's')				/* String parameter */
    {
      reg2 char	*par= va_arg(ap, char *);
      size_t plen, left_len= (size_t) (end - to) + 1;
      if (!par)
        par = (char*) "(null)";
      plen= strnlen(par, width);
      if (left_len <= plen)
	plen = left_len - 1;
      to= strnmov(to,par,plen);
      continue;
    }
    else if (*fmt == 'b')				/* Buffer parameter */
    {
      char *par = va_arg(ap, char *);
      DBUG_ASSERT(to <= end);
      if (to + width + 1 > end)
        width= end - to - 1;  /* sign doesn't matter */
      memmove(to, par, width);
      to+= width;
      continue;
    }
    else if (*fmt == 'f' || *fmt == 'g')
    {
      double d= va_arg(ap, double);
      if (width == SIZE_T_MAX)
        width= FLT_DIG; /* width not set, use default */
      else if (width >= NOT_FIXED_DEC)
        width= NOT_FIXED_DEC - 1; /* max.precision for my_fcvt() */
      width= min(width, (size_t)(end-to) - 1);

      if (*fmt == 'f')
        to+= my_fcvt(d, (int)width , to, NULL);
      else
        to+= my_gcvt(d, MY_GCVT_ARG_DOUBLE, (int) width , to, NULL);
    }
    else if (*fmt == 'd' || *fmt == 'u' || *fmt == 'x' || *fmt == 'X' ||
             *fmt == 'p')
    {
      /* Integer parameter */
      longlong larg;
      size_t res_length, to_length;
      char *store_start= to, *store_end;
      char buff[32];
      if (*fmt == 'p')
      {
        have_longlong= (sizeof(void *) == sizeof(longlong));
      }

      if ((to_length= (size_t) (end-to)) < 16 || length)
	store_start= buff;
      if (have_longlong)
        larg = va_arg(ap,longlong);
      else if (*fmt == 'd')
        larg = va_arg(ap, int);
      else
        larg= va_arg(ap, uint);
      if (*fmt == 'd')
	store_end= longlong10_to_str(larg, store_start, -10);
      else if (*fmt == 'u')
        store_end= longlong10_to_str(larg, store_start, 10);
      else if (*fmt == 'p')
      {
        store_start[0]= '0';
        store_start[1]= 'x';
        store_end= ll2str(larg, store_start + 2, 16, 0);
      }
      else
      {
        DBUG_ASSERT(*fmt == 'X' || *fmt =='x');
        store_end= ll2str(larg, store_start, 16, (*fmt == 'X'));
      }

      if ((res_length= (size_t) (store_end - store_start)) > to_length)
	break;					/* num doesn't fit in output */
      /* If %#d syntax was used, we have to pre-zero/pre-space the string */
      if (store_start == buff)
      {
	length= min(length, to_length);
	if (res_length < length)
	{
	  size_t diff= (length- res_length);
	  bfill(to, diff, pre_zero ? '0' : ' ');
	  to+= diff;
	}
	bmove(to, store_start, res_length);
      }
      to+= res_length;
      continue;
    }
    else if (*fmt == 'c')                       /* Character parameter */
    {
      register int larg;
      if (to == end)
        break;
      larg = va_arg(ap, int);
      *to++= (char) larg;
      continue;
    }

    /* We come here on '%%', unknown code or too long parameter */
    if (to == end)
      break;
    *to++='%';				/* % used as % or unknown code */
  }
  DBUG_ASSERT(to <= end);
  *to='\0';				/* End of errmessage */
  return (size_t) (to - start);
}


size_t my_snprintf(char* to, size_t n, const char* fmt, ...)
{
  size_t result;
  va_list args;
  va_start(args,fmt);
  result= my_vsnprintf(to, n, fmt, args);
  va_end(args);
  return result;
}

#ifdef MAIN
#define OVERRUN_SENTRY  250
static void my_printf(const char * fmt, ...)
{
  char buf[33];
  int n;
  va_list ar;
  va_start(ar, fmt);
  buf[sizeof(buf)-1]=OVERRUN_SENTRY;
  n = my_vsnprintf(buf, sizeof(buf)-1,fmt, ar);
  printf(buf);
  printf("n=%d, strlen=%d\n", n, strlen(buf));
  if ((uchar) buf[sizeof(buf)-1] != OVERRUN_SENTRY)
  {
    fprintf(stderr, "Buffer overrun\n");
    abort();
  }
  va_end(ar);
}


int main()
{

  my_printf("Hello\n");
  my_printf("Hello int, %d\n", 1);
  my_printf("Hello string '%s'\n", "I am a string");
  my_printf("Hello hack hack hack hack hack hack hack %d\n", 1);
  my_printf("Hello %d hack  %d\n", 1, 4);
  my_printf("Hello %d hack hack hack hack hack %d\n", 1, 4);
  my_printf("Hello '%s' hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh\n", "hack");
  my_printf("Hello hhhhhhhhhhhhhh %d sssssssssssssss\n", 1);
  my_printf("Hello  %u\n", 1);
  my_printf("Hex:   %lx  '%6lx'\n", 32, 65);
  my_printf("conn %ld to: '%-.64s' user: '%-.32s' host:\
 `%-.64s' (%-.64s)", 1, 0,0,0,0);
  return 0;
}
#endif
