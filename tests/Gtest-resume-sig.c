/* libunwind - a platform-independent unwind library
   Copyright (C) 2003 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

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

/*  Verify that unw_resume() restores the signal mask at proper time.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libunwind.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef HAVE_IA64INTRIN_H
# include <ia64intrin.h>
#endif

#define panic(args...)						\
	do { fprintf (stderr, args); ++nerrors; } while (0)

int verbose;
int nerrors;
int got_usr1, got_usr2;
char *sigusr1_sp;

void
handler (int sig)
{
  unw_word_t ip;
  sigset_t mask;
  unw_context_t uc;
  unw_cursor_t c;
  char foo;

#if UNW_TARGET_IA64
# ifdef __ECC
  void *bsp = (void *) __getReg(_IA64_REG_AR_BSP);
# else
  void *bsp = __builtin_ia64_bsp ();
#endif
  if (verbose)
    printf ("bsp = %p\n", bsp);
#endif

  if (verbose)
    printf ("got signal %d\n", sig);

  if (sig == SIGUSR1)
    {
      ++got_usr1;
      sigusr1_sp = &foo;

      sigemptyset (&mask);
      sigaddset (&mask, SIGUSR2);
      sigprocmask (SIG_BLOCK, &mask, NULL);
      kill (getpid (), SIGUSR2);	/* pend SIGUSR2 */

      signal (SIGUSR1, SIG_IGN);
      signal (SIGUSR2, handler);

      unw_getcontext(&uc);
      unw_init_local(&c, &uc);
      unw_step(&c);		/* step to signal trampoline */
      unw_step(&c);		/* step to signaller frame (main ()) */
      unw_get_reg(&c, UNW_REG_IP, &ip);
      if (verbose)
	printf ("resuming at 0x%lx, with SIGUSR2 pending\n",
		(unsigned long) ip);
      unw_resume(&c);
    }
  else if (sig == SIGUSR2)
    {
      ++got_usr2;
      if (got_usr1)
	{
	  if (sigusr1_sp != &foo)
	    panic ("Stack pointer changed from %p to %p between signals\n",
		   sigusr1_sp, &foo);
	  else if (verbose)
	    printf ("OK: stack still at %p\n", &foo);
	}
      signal (SIGUSR2, SIG_IGN);
    }
  else
    panic ("Got unexpected signal %d\n", sig);
}

int
main (int argc, char **argv)
{
  if (argc > 1)
    verbose = 1;

  signal (SIGUSR1, handler);

  if (verbose)
    printf ("sending SIGUSR1\n");
  kill (getpid (), SIGUSR1);

  if (nerrors)
    fprintf (stderr, "FAILURE: detected %d errors\n", nerrors);

  if (verbose)
    printf ("SUCCESS\n");
  return 0;
}
