/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2003 Hewlett-Packard Co
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

#ifndef internal_h
#define internal_h

/* Platform-independent libunwind-internal declarations.  */

#include <assert.h>
#include <pthread.h>
#include <libunwind.h>

#ifdef __GNUC__
# define HIDDEN		__attribute__((visibility ("hidden")))
# define likely(x)	__builtin_expect ((x), 1)
# define unlikely(x)	__builtin_expect ((x), 0)
#else
# define HIDDEN
# define likely(x)
# define unlikely(x)
#endif

#ifdef DEBUG
# define UNW_DEBUG	1
#endif

#if UNW_DEBUG
# include <stdio.h>
# define debug(level,format...)	\
	do { if (tdep_debug_level > level) printf (format); } while (0)
# define dprintf(format...) \
	printf (format)
# ifdef __GNUC__
#  define inline	__attribute__ ((unused))
# endif
#else
# define debug(level,format...)
# define dprintf(format...)
#endif

#define NELEMS(a)	(sizeof (a) / sizeof ((a)[0]))

/* Make it easy to write thread-safe code which may or may not be
   linked against libpthread.  The macros below can be used
   unconditionally and if -lpthread is around, they'll call the
   corresponding routines otherwise, they do nothing.  */

#pragma weak pthread_mutex_lock
#pragma weak pthread_mutex_unlock

#define mutex_lock(l)	(pthread_mutex_lock ? pthread_mutex_lock (l) : 0)
#define mutex_unlock(l)	(pthread_mutex_unlock ? pthread_mutex_unlock (l) : 0)

#define UNWI_OBJ(fn)	  UNW_PASTE(UNW_PREFIX,UNW_PASTE(I,fn))
#define UNWI_ARCH_OBJ(fn) UNW_PASTE(UNW_PASTE(UNW_PASTE(_UI,UNW_TARGET),_), fn)

extern int UNWI_OBJ(find_dynamic_proc_info) (unw_addr_space_t as,
					     unw_word_t ip,
					     unw_proc_info_t *pi,
					     int need_unwind_info, void *arg);
extern int UNWI_ARCH_OBJ(extract_dynamic_proc_info) (unw_addr_space_t as,
						     unw_word_t ip,
						     unw_proc_info_t *pi,
						     unw_dyn_info_t *di,
						     int need_unwind_info,
						     void *arg);
extern void UNWI_OBJ(put_dynamic_unwind_info) (unw_addr_space_t as,
					       unw_proc_info_t *pi, void *arg);
extern int UNWI_ARCH_OBJ(dyn_remote_find_proc_info) (unw_addr_space_t as,
						     unw_word_t ip,
						     unw_proc_info_t *pi,
						     unw_word_t *generation,
						     int need_unwind_info,
						     void *arg);
extern void UNWI_ARCH_OBJ(dyn_remote_put_unwind_info) (unw_addr_space_t as,
						       unw_proc_info_t *pi,
						       void *arg);
extern int UNWI_ARCH_OBJ(get_proc_name) (unw_addr_space_t as,
					 unw_word_t ip, int is_local,
					 char *buf, size_t buf_len, void *arg);

#define unwi_find_dynamic_proc_info(as,ip,pi,n,arg)			\
	UNWI_OBJ(find_dynamic_proc_info)(as, ip, pi, n, arg)

#define unwi_extract_dynamic_proc_info(as,ip,pi,di,n,arg)		\
	UNWI_ARCH_OBJ(extract_dynamic_proc_info)(as, ip, pi, di, n, arg)

#define unwi_put_dynamic_unwind_info(as,pi,arg)				\
	UNWI_OBJ(put_dynamic_unwind_info)(as, pi, arg)

/* These handle the remote (cross-address-space) case of accessing
   dynamic unwind info. */

#define unwi_dyn_remote_find_proc_info(as,i,p,g,n,arg)			\
	UNWI_ARCH_OBJ(dyn_remote_find_proc_info)(as, i, p, g, n, arg)

#define unwi_dyn_remote_put_unwind_info(as,p,arg)			\
	UNWI_ARCH_OBJ(dyn_remote_put_unwind_info)(as, p, arg)

#define unwi_get_proc_name(as,ip,l,b,s,arg)				\
	UNWI_ARCH_OBJ(get_proc_name)(as, ip, l, b, s, arg)

extern unw_dyn_info_list_t _U_dyn_info_list;
extern pthread_mutex_t _U_dyn_info_list_lock;

#define WSIZE	(sizeof (unw_word_t))

static inline int
fetch8 (unw_addr_space_t as, unw_accessors_t *a,
	unw_word_t *addr, int8_t *valp, void *arg)
{
  unw_word_t val, aligned_addr = *addr & -WSIZE, off = *addr - aligned_addr;
  int ret;

  *addr += 1;

  ret = (*a->access_mem) (as, aligned_addr, &val, 0, arg);

#if __BYTE_ORDER == __LITTLE_ENDIAN
  val >>= 8*off;
#else
  val >>= 8*(WSIZE - 1 - off);
#endif
  *valp = val & 0xff;
  return ret;
}

static inline int
fetch16 (unw_addr_space_t as, unw_accessors_t *a,
	 unw_word_t *addr, int16_t *valp, void *arg)
{
  unw_word_t val, aligned_addr = *addr & -WSIZE, off = *addr - aligned_addr;
  int ret;

  assert ((off & 0x1) == 0);

  *addr += 2;

  ret = (*a->access_mem) (as, aligned_addr, &val, 0, arg);

#if __BYTE_ORDER == __LITTLE_ENDIAN
  val >>= 8*off;
#else
  val >>= 8*(WSIZE - 2 - off);
#endif
  *valp = val & 0xffff;
  return ret;
}

static inline int
fetch32 (unw_addr_space_t as, unw_accessors_t *a,
	 unw_word_t *addr, int32_t *valp, void *arg)
{
  unw_word_t val, aligned_addr = *addr & -WSIZE, off = *addr - aligned_addr;
  int ret;

  assert ((off & 0x3) == 0);

  *addr += 4;

  ret = (*a->access_mem) (as, aligned_addr, &val, 0, arg);

#if __BYTE_ORDER == __LITTLE_ENDIAN
  val >>= 8*off;
#else
  val >>= 8*(WSIZE - 4 - off);
#endif
  *valp = val & 0xffffffff;
  return ret;
}

static inline int
fetchw (unw_addr_space_t as, unw_accessors_t *a,
	unw_word_t *addr, unw_word_t *valp, void *arg)
{
  int ret;

  ret = (*a->access_mem) (as, *addr, valp, 0, arg);
  *addr += WSIZE;
  return ret;
}

extern void mi_init (void);	/* machine-independent initializations */

#include <tdep.h>

#endif /* internal_h */
