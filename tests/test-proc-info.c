/* This test program checks whether proc_info lookup failures are
   cached.  They must NOT be cached because it could otherwise turn
   temporary failures into permanent ones.  Furthermore, we allow apps
   to return -UNW_ESTOPUNWIND to terminate unwinding (though this
   feature is deprecated and dynamic unwind info should be used
   instead).  */

#include <stdio.h>
#include <string.h>

#include <libunwind.h>

int errors;

#define panic(args...)					\
	{ ++errors; fprintf (stderr, args); return -1; }

static int
find_proc_info (unw_addr_space_t as, unw_word_t ip, unw_proc_info_t *pip,
		int need_unwind_info, void *arg)
{
  return -UNW_ESTOPUNWIND;
}

static int
access_mem (unw_addr_space_t as, unw_word_t addr, unw_word_t *valp,
	    int write, void *arg)
{
  if (!write)
    *valp = 0;
  return 0;
}

static int
access_reg (unw_addr_space_t as, unw_regnum_t regnum, unw_word_t *valp,
	    int write, void *arg)
{
  if (!write)
    *valp = 32;
  return 0;
}

static int
access_fpreg (unw_addr_space_t as, unw_regnum_t regnum, unw_fpreg_t *valp,
	      int write, void *arg)
{
  if (!write)
    memset (valp, 0, sizeof (valp));
  return 0;
}

static int
get_dyn_info_list_addr (unw_addr_space_t as, unw_word_t *dilap, void *arg)
{
  return -UNW_ENOINFO;
}

static int
nop (void)
{
  panic ("nop() got called!\n");
  return -1;
}

int
main (int argc, char **argv)
{
  unw_accessors_t acc;
  unw_addr_space_t as;
  int ret, verbose = 0;
  unw_cursor_t c;

  if (argc > 1 && strcmp (argv[0], "-v") == 0)
    verbose = 1;

  memset (&acc, 0, sizeof (acc));
  acc.find_proc_info = find_proc_info;
  acc.put_unwind_info = (void *) nop;
  acc.get_dyn_info_list_addr = get_dyn_info_list_addr;
  acc.access_mem = access_mem;
  acc.access_reg = access_reg;
  acc.access_fpreg = access_fpreg;
  acc.resume = (void *) nop;
  acc.get_proc_name = (void *) nop;

  as = unw_create_addr_space (&acc, 0);
  if (!as)
    panic ("unw_create_addr_space() failed\n");

  unw_set_caching_policy (as, UNW_CACHE_GLOBAL);

  ret = unw_init_remote (&c, as, NULL);
  if (ret < 0)
    panic ("unw_init_remote() returned %d instead of 0\n", ret);

  ret = unw_step (&c);
  if (ret != -UNW_ESTOPUNWIND)
    panic ("First call to unw_step() returned %d instead of %d\n",
	   ret, -UNW_ESTOPUNWIND);

  ret = unw_step (&c);
  if (ret != -UNW_ESTOPUNWIND)
    panic ("Second call to unw_step() returned %d instead of %d\n",
	   ret, -UNW_ESTOPUNWIND);

  if (verbose)
    printf ("SUCCESS\n");
  return 0;
}
