#include <libunwind.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

#define panic(args...)				\
	{ fprintf (stderr, args); exit (-1); }

typedef void (*template_t) (int, void (*)(),
			    int (*)(const char *, ...), const char *,
			    const char **);

static const char *strarr[] =
  {
    "i", "ii", "iii", "iv", "v", "vi", "vii", "viii", "ix", "x", NULL
  };

#ifdef __ia64__
struct fdesc
  {
    long code;
    long gp;
  };
# define get_fdesc(fdesc,func)	(fdesc = *(struct fdesc *) &(func))
# define get_funcp(fdesc)	((template_t) &(fdesc))
# define get_gp(fdesc)		((fdesc).gp)
#else
struct fdesc
  {
    long code;
  };
# define get_fdesc(fdesc,func)	(fdesc.code = (long) &(func))
# define get_funcp(fdesc)	((template_t) (fdesc).code)
# define get_gp(fdesc)		(0)
#endif

static void
flush_cache (void *addr, unsigned long len)
{
#ifdef __ia64__
  void *end = (char *) addr + len;

  while (addr < end)
    {
      asm volatile ("fc %0" :: "r"(addr));
      addr = (char *) addr + 32;
    }
  asm volatile (";;sync.i;;srlz.i;;");
#endif
}

void
template (int i, template_t self,
	  int (*printer)(const char *, ...), const char *fmt, const char **arr)
{
  (*printer) (fmt, arr[11 - i][0], arr[11 - i] + 1);
  if (i > 0)
    (*self) (i - 1, self, printer, fmt, arr);
}

static void
sighandler (int signal)
{
  unw_cursor_t cursor;
  unw_word_t ip;
  unw_context_t uc;
  int count = 0;

  printf ("caught signal %d\n", signal);

  unw_getcontext (&uc);
  unw_init_local (&cursor, &uc);

  while (!unw_is_signal_frame (&cursor))
    if (unw_step (&cursor) < 0)
      panic ("failed to find signal frame!\n");
  unw_step (&cursor);

  do
    {
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      printf ("ip = %lx\n", (long) ip);
      ++count;
    }
  while (unw_step (&cursor) > 0);

  if (count != 13)
    panic ("FAILURE: expected 13, not %d frames below signal frame\n", count);

  printf ("SUCCESS\n");
  exit (0);
}

int
main (int argc, char *argv[])
{
  unw_dyn_region_info_t *region;
  unw_dyn_info_t di;
  struct fdesc fdesc;
  template_t funcp;
  void *mem;

  mem = malloc (getpagesize ());

  get_fdesc (fdesc, template);

  printf ("old code @ %p, new code @ %p\n", (void *) fdesc.code, mem);

  memcpy (mem, (void *) fdesc.code, 256);
  mprotect ((void *) ((long) mem & ~(getpagesize () - 1)),
	    2*getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC);
  flush_cache (mem, 256);

  signal (SIGSEGV, sighandler);

  /* register the new function: */
  region = alloca (_U_dyn_region_info_size (2));
  region->next = NULL;
  region->insn_count = 256;
  region->op_count = 2;
  _U_dyn_op_alias (&region->op[0], 0, -1, fdesc.code);
  _U_dyn_op_stop (&region->op[1]);

  memset (&di, 0, sizeof (di));
  di.start_ip = (long) mem;
  di.end_ip = (long) mem + 256;
  di.gp = get_gp (fdesc);
  di.format = UNW_INFO_FORMAT_DYNAMIC;
  di.u.pi.name_ptr = (unw_word_t) "copy_of_template";
  di.u.pi.regions = region;

  _U_dyn_register (&di);

  /* call new function: */
  fdesc.code = (long) mem;
  funcp = get_funcp (fdesc);

  (*funcp) (10, funcp, printf, "iteration %c%s\n", strarr);

  _U_dyn_cancel (&di);
  return -1;
}
