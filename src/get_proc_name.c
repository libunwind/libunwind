/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#include <endian.h>

#include "internal.h"

static int
intern_string (unw_addr_space_t as, unw_accessors_t *a,
	       unw_word_t addr, char *buf, size_t buf_len, void *arg)
{
  size_t i;
  int ret;

  for (i = 0; i < buf_len; ++i)
    {
      if ((ret = fetch8 (as, a, &addr, buf + i, arg)) < 0)
	return ret;

      if (buf[i] == '\0')
	return 0;		/* copied full string; return success */
    }
  buf[buf_len - 1] = '\0';	/* ensure string is NUL terminated */
  return -UNW_ENOMEM;
}

HIDDEN int
unwi_get_proc_name (unw_addr_space_t as, unw_word_t ip, int is_local,
		    char *buf, size_t buf_len, void *arg)
{
  unw_accessors_t *a = unw_get_accessors (as);
  unw_proc_info_t pi;
  int ret;

  buf[0] = '\0';	/* always return a valid string, even if it's empty */

  ret = unwi_find_dynamic_proc_info (as, ip, &pi, 1, arg);
  if (ret == 0)
    {
      unw_dyn_info_t *di = pi.unwind_info;

      switch (di->format)
	{
	case UNW_INFO_FORMAT_DYNAMIC:
	  ret = intern_string (as, a, di->u.pi.name_ptr, buf, buf_len, arg);
	  break;

	case UNW_INFO_FORMAT_TABLE:
	  /* XXX should we create a fake name, e.g.: "tablenameN",
	     where N is the index of the function in the table??? */
	  ret = -UNW_ENOINFO;
	  break;

	default:
	  ret = -UNW_EINVAL;
	  break;
	}
      unwi_put_dynamic_unwind_info (as, &pi, arg);
      return ret;
    }

  if (ret != -UNW_ENOINFO)
    return ret;

  /* not a dynamic procedure */

  if (!is_local)
    /* It makes no sense to implement get_proc_name() for remote
       address spaces because that would require a callback and in
       that case, the application using libunwind needs to know how to
       look up a procedure name anyhow.  */
    return -UNW_ENOINFO;

  /* XXX implement me: look up ELF executable covering IP, then check
     dynamic and regular symbol tables for a good name.  */
  return -UNW_ENOINFO;
}
