#include <libunwind.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/mman.h>

#ifdef __ia64__
# define GET_ENTRY(fdesc)	(((uintptr_t *) (fdesc))[0])
# define GET_GP(fdesc)		(((uintptr_t *) (fdesc))[0])
# define EXTRA			16
#else
# define GET_ENTRY(fdesc)	((uintptr_t ) (fdesc))
# define GET_GP(fdesc)		(0)
# define EXTRA			0
#endif

static void
flush_cache (void *addr, size_t len)
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

int
make_executable (void *addr, size_t len)
{
  if (mprotect ((void *) (((long) addr) & -getpagesize ()), len,
		PROT_READ | PROT_WRITE | PROT_EXEC) < 0)
    {
      perror ("mprotect");
      return -1;
    }
  flush_cache (addr, len);
  return 0;
}

void *
create_func (unw_dyn_info_t *di, const char *name, long (*func) (),
	     void *end, unw_dyn_region_info_t *region)
{
  void *mem, *addr, *fptr;
  unw_word_t gp = 0;
  size_t len;

  len = (uintptr_t) end - GET_ENTRY (func) + EXTRA;
  mem = malloc (len);

#ifdef __ia64__
  addr = (void *) GET_ENTRY (func);

  /* build function descriptor: */
  ((long *) mem)[0] = (long) mem + 16;		/* entry point */
  ((long *) mem)[1] = GET_GP (func);		/* global-pointer */
  fptr = mem;
  mem = (void *) ((long) mem + 16);
#else
  fptr = mem;
#endif

  memcpy (mem, addr, len);

  if (make_executable (mem, len) < 0)
    return NULL;

  if (di)
    {
      memset (di, 0, sizeof (*di));
      di->start_ip = (unw_word_t) mem;
      di->end_ip = (unw_word_t) mem + len;
      di->gp = gp;
      di->format = UNW_INFO_FORMAT_DYNAMIC;
      di->u.pi.name_ptr = (unw_word_t) name;
      di->u.pi.regions = region;
    }
  return fptr;
}

int
main (int argc, char **argv)
{
  extern long func_add1 (long);
  extern char func_add1_end[];
  extern long func_add3 (long, long (*[])());
  extern char func_add3_end[];
  extern char _U_dyn_info_list[];
  unw_dyn_region_info_t *r_pro, *r_epi;
  unw_dyn_info_t di0, di1;
  long (*add1) (long);
  long (*add3_0) (long);
  long (*add3_1) (long, long (*[])());
  void *flist[2];
  long ret;

  add1 = create_func (NULL, "func_add1", func_add1, func_add1_end, NULL);

  /* Describe the epilogue of func_add3: */
  r_epi = alloca (_U_dyn_region_info_size (2));
  r_epi->next = NULL;
  r_epi->insn_count = -4;
  r_epi->op_count = 2;
  _U_dyn_op_pop_frames (&r_epi->op[0],
			/* qp=*/ 1, /* when=*/ 0, /* num_frames=*/ 1);
  _U_dyn_op_stop (&r_epi->op[1]);

  /* Describe the prologue of func_add3: */
  r_pro = alloca (_U_dyn_region_info_size (2));
  r_pro->next = r_epi;
  r_pro->insn_count = 3;
  r_pro->op_count = 3;
  _U_dyn_op_save_reg (&r_pro->op[0], /* qp=*/ 1, /* when=*/ 0,
		      /* reg=*/ UNW_IA64_AR_PFS, /* dst=*/ UNW_IA64_GR + 35);
  _U_dyn_op_save_reg (&r_pro->op[1], /* qp=*/ 1, /* when=*/ 2,
		      /* reg=*/ UNW_IA64_BR + 0, /* dst=*/ UNW_IA64_GR + 34);
  _U_dyn_op_stop (&r_pro->op[2]);

  /* Create two functions which can share the region-list:  */
  add3_0 = create_func (&di0, "func_add3/0", func_add3, func_add3_end, r_pro);
  add3_1 = create_func (&di1, "func_add3/1", func_add3, func_add3_end, r_pro);

  _U_dyn_register (&di0);
  _U_dyn_register (&di1);

  flist[0] = add3_0;
  flist[1] = add1;

  syscall (-1);		/* do something ptmon can latch onto */
  ret = (*add3_1) (13, flist);

  _U_dyn_cancel (&di0);
  _U_dyn_cancel (&di1);

  if (ret != 18)
    {
      fprintf (stderr, "FAILURE: (*add3_1)(13) returned %ld\n", ret);
      exit (-1);
    }
  return 0;
}
