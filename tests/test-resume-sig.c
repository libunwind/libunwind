/*  Verify that unw_resume() restores the signal mask at proper time.  */

#include <libunwind.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define panic(args...)				\
	{ fprintf (stderr, args); exit (-1); }

int got_usr1, got_usr2;
char *sigusr1_sp;

void
handler (int sig)
{
  sigset_t mask;
  unw_context_t uc;
  unw_cursor_t c;
  unw_word_t ip;
  char foo;

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
      printf ("resuming at 0x%lx\n", ip);
      unw_resume(&c);
    }
  else if (sig == SIGUSR2)
    {
      ++got_usr2;
      if (got_usr1)
	if (sigusr1_sp != &foo)
	  panic ("Stack pointer changed from %p to %p between signals\n",
		 sigusr1_sp, &foo);
      signal (SIGUSR2, SIG_IGN);
    } else
      panic ("Got unexpected signal %d\n", sig);
}

int
main (int argc, char **argv)
{
  signal (SIGUSR1, handler);
  kill (getpid (), SIGUSR1);
  return 0;
}
