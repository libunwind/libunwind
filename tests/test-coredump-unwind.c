/*
 * Example program for unwinding core dumps.
 *
 * Compile a-la:
 * gcc -Os -Wall \
 *    -Wl,--start-group \
 *        -lunwind -lunwind-x86 -lunwind-coredump \
 *        example-core-unwind.c \
 *    -Wl,--end-group \
 *    -oexample-core-unwind
 *
 * Run:
 * objdump -sx COREDUMP
 * eu-unstrip -n --core COREDUMP
 *   figure out which segments in COREDUMP correspond to which mapped executable files
 *   (binary and libraries), then supply them like this:
 * ./example-core-unwind COREDUMP 3:/bin/crashed_program 6:/lib/libc.so.6 [...]
 */

#undef _GNU_SOURCE
#define _GNU_SOURCE 1
#undef __USE_GNU
#define __USE_GNU 1

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <syslog.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>

/* For SIGSEGV handler code */
#include <execinfo.h>
#include <sys/ucontext.h>

#include <libunwind-coredump.h>


/* Utility logging functions */

enum {
    LOGMODE_NONE = 0,
    LOGMODE_STDIO = (1 << 0),
    LOGMODE_SYSLOG = (1 << 1),
    LOGMODE_BOTH = LOGMODE_SYSLOG + LOGMODE_STDIO,
};
const char *msg_prefix = "";
const char *msg_eol = "\n";
int logmode = LOGMODE_STDIO;
int xfunc_error_retval = EXIT_FAILURE;

void xfunc_die(void)
{
  exit(xfunc_error_retval);
}

static void verror_msg_helper(const char *s,
                              va_list p,
                              const char* strerr,
                              int flags)
{
  char *msg;
  int prefix_len, strerr_len, msgeol_len, used;

  if (!logmode)
    return;

  used = vasprintf(&msg, s, p);
  if (used < 0)
    return;

  /* This is ugly and costs +60 bytes compared to multiple
   * fprintf's, but is guaranteed to do a single write.
   * This is needed for e.g. when multiple children
   * can produce log messages simultaneously. */

  prefix_len = msg_prefix[0] ? strlen(msg_prefix) + 2 : 0;
  strerr_len = strerr ? strlen(strerr) : 0;
  msgeol_len = strlen(msg_eol);
  /* +3 is for ": " before strerr and for terminating NUL */
  char *msg1 = (char*) realloc(msg, prefix_len + used + strerr_len + msgeol_len + 3);
  if (!msg1)
    {
      free(msg);
      return;
    }
  msg = msg1;
  /* TODO: maybe use writev instead of memmoving? Need full_writev? */
  if (prefix_len)
    {
      char *p;
      memmove(msg + prefix_len, msg, used);
      used += prefix_len;
      p = stpcpy(msg, msg_prefix);
      p[0] = ':';
      p[1] = ' ';
    }
  if (strerr)
    {
      if (s[0])
        {
          msg[used++] = ':';
          msg[used++] = ' ';
        }
      strcpy(&msg[used], strerr);
      used += strerr_len;
    }
  strcpy(&msg[used], msg_eol);

  if (flags & LOGMODE_STDIO)
    {
      fflush(stdout);
      write(STDERR_FILENO, msg, used + msgeol_len);
    }
  msg[used] = '\0'; /* remove msg_eol (usually "\n") */
  if (flags & LOGMODE_SYSLOG)
    {
      syslog(LOG_ERR, "%s", msg + prefix_len);
    }
  free(msg);
}

void log_msg(const char *s, ...)
{
  va_list p;
  va_start(p, s);
  verror_msg_helper(s, p, NULL, logmode);
  va_end(p);
}
/* It's a macro, not function, since it collides with log() from math.h */
#undef log
#define log(...) log_msg(__VA_ARGS__)

void error_msg(const char *s, ...)
{
  va_list p;
  va_start(p, s);
  verror_msg_helper(s, p, NULL, logmode);
  va_end(p);
}

void error_msg_and_die(const char *s, ...)
{
  va_list p;
  va_start(p, s);
  verror_msg_helper(s, p, NULL, logmode);
  va_end(p);
  xfunc_die();
}

void perror_msg(const char *s, ...)
{
  va_list p;
  va_start(p, s);
  /* Guard against "<error message>: Success" */
  verror_msg_helper(s, p, errno ? strerror(errno) : NULL, logmode);
  va_end(p);
}

void perror_msg_and_die(const char *s, ...)
{
  va_list p;
  va_start(p, s);
  /* Guard against "<error message>: Success" */
  verror_msg_helper(s, p, errno ? strerror(errno) : NULL, logmode);
  va_end(p);
  xfunc_die();
}

void die_out_of_memory(void)
{
  error_msg_and_die("Out of memory, exiting");
}

/* End of utility logging functions */



static
void handle_sigsegv(int sig, siginfo_t *info, void *ucontext)
{
  long ip;
  ucontext_t *uc;

  uc = ucontext;
#if defined(__linux__)
#ifdef TARGET_X86
	ip = uc->uc_mcontext.gregs[REG_EIP];
#elif defined(TARGET_X86_64)
	ip = uc->uc_mcontext.gregs[REG_RIP];
#elif defined(TARGET_ARM)
	ip = uc->uc_mcontext.arm_ip;
#endif
#else
#error Port me
#endif
  dprintf(2, "signal:%d address:0x%lx ip:0x%lx\n",
			sig,
			/* this is void*, but using %p would print "(null)"
			 * even for ptrs which are not exactly 0, but, say, 0x123:
			 */
			(long)info->si_addr,
			ip);

  {
    /* glibc extension */
    void *array[50];
    int size;
    size = backtrace(array, 50);
    backtrace_symbols_fd(array, size, 2);
  }

  _exit(1);
}

static void install_sigsegv_handler(void)
{
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = handle_sigsegv;
  sa.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGILL, &sa, NULL);
  sigaction(SIGFPE, &sa, NULL);
  sigaction(SIGBUS, &sa, NULL);
}

int
main(int argc, char **argv)
{
  unw_addr_space_t as;
  struct UCD_info *ui;
  unw_cursor_t c;
  int ret;

  install_sigsegv_handler();

  const char *progname = strrchr(argv[0], '/');
  if (progname)
    progname++;
  else
    progname = argv[0];

  if (!argv[1])
    error_msg_and_die("Usage: %s COREDUMP [SEGMENT_NO:BINARY_FILE]...", progname);

  msg_prefix = progname;

  as = unw_create_addr_space(&_UCD_accessors, 0);
  if (!as)
    error_msg_and_die("unw_create_addr_space() failed");

  ui = _UCD_create(argv[1]);
  if (!ui)
    error_msg_and_die("_UCD_create('%s') failed", argv[1]);
  ret = unw_init_remote(&c, as, ui);
  if (ret < 0)
    error_msg_and_die("unw_init_remote() failed: ret=%d\n", ret);

  argv += 2;
  while (*argv)
    {
      char *colon = strchr(*argv, ':');
      if (!colon)
        error_msg_and_die("Bad format: '%s'", *argv);
      *colon = '\0';
      unsigned n = atoi(*argv);
      *colon = ':';
      if (_UCD_add_backing_file_at_segment(ui, n, colon + 1) < 0)
        error_msg_and_die("Can't add backing file '%s'", *argv);
      argv++;
    }

  for (;;)
    {
      unw_word_t ip;
      ret = unw_get_reg(&c, UNW_REG_IP, &ip);
      if (ret < 0)
        error_msg_and_die("unw_get_reg(UNW_REG_IP) failed: ret=%d\n", ret);

      unw_proc_info_t pi;
      ret = unw_get_proc_info(&c, &pi);
      if (ret < 0)
        error_msg_and_die("unw_get_proc_info(ip=0x%lx) failed: ret=%d\n", (long) ip, ret);
      printf("\tip=0x%08lx proc=%08lx-%08lx handler=0x%08lx lsda=0x%08lx\n",
				(long) ip,
				(long) pi.start_ip, (long) pi.end_ip,
				(long) pi.handler, (long) pi.lsda);

      log("step");
      ret = unw_step(&c);
      log("step done:%d", ret);
      if (ret < 0)
    	error_msg_and_die("FAILURE: unw_step() returned %d", ret);
      if (ret == 0)
        break;
    }
  log("stepping ended");

  _UCD_destroy(ui);

  return 0;
}
