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
 * eu-unstrip -n --core COREDUMP
 *   figure out which virtual addresses in COREDUMP correspond to which mapped executable files
 *   (binary and libraries), then supply them like this:
 * ./example-core-unwind COREDUMP 0x400000:/bin/crashed_program 0x3458600000:/lib/libc.so.6 [...]
 *
 * Note: Program eu-unstrip is part of elfutils, virtual addresses of shared
 * libraries can be determined by ldd (at least on linux).
 */

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>

#include "compiler.h"
#include <libunwind-coredump.h>

/* Utility logging functions */

enum {
    LOGMODE_NONE = 0,
    LOGMODE_STDIO = (1 << 0),
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
      ssize_t written UNUSED = write(STDERR_FILENO, msg, used + msgeol_len);
    }
  msg[used] = '\0'; /* remove msg_eol (usually "\n") */
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

void error_msg_and_die(const char *s, ...)
{
  va_list p;
  va_start(p, s);
  verror_msg_helper(s, p, NULL, logmode);
  va_end(p);
  xfunc_die();
}

/* End of utility logging functions */


int
main(int argc UNUSED, char **argv)
{
  unw_addr_space_t as;
  struct UCD_info *ui;
  unw_cursor_t c;
  int ret;

#define TEST_FRAMES 4
#define TEST_NAME_LEN 32
  int testcase = 0;
  int test_cur = 0;
  long test_start_ips[TEST_FRAMES];
  char test_names[TEST_FRAMES][TEST_NAME_LEN];

  const char *progname = strrchr(argv[0], '/');
  if (progname)
    progname++;
  else
    progname = argv[0];

  if (!argv[1])
    error_msg_and_die("Usage: %s COREDUMP [VADDR:BINARY_FILE]...", progname);

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

  /* Enable checks for the crasher test program? */
  if (*argv && !strcmp(*argv, "-testcase"))
  {
    testcase = 1;
    logmode = LOGMODE_NONE;
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

      if (!testcase)
        {
          char proc_name[128];
          unw_word_t off;
          unw_get_proc_name(&c, proc_name, sizeof(proc_name), &off);

          printf("\tip=0x%08lx proc=%08lx-%08lx handler=0x%08lx lsda=0x%08lx %s\n",
                 (long) ip,
                 (long) pi.start_ip, (long) pi.end_ip,
                 (long) pi.handler, (long) pi.lsda, proc_name);

          char filename[PATH_MAX];
          unw_word_t file_offset;
          ret = unw_get_elf_filename (&c, filename, sizeof (filename), &file_offset);
          if (ret == UNW_ESUCCESS)
              printf ("\t[%s+0x%lx]\n", filename, (long) file_offset);
        }

      if (testcase && test_cur < TEST_FRAMES)
        {
           unw_word_t off;

           test_start_ips[test_cur] = (long) pi.start_ip;
           if (unw_get_proc_name(&c, test_names[test_cur], sizeof(test_names[0]), &off) != 0)
           {
             test_names[test_cur][0] = '\0';
           }
           test_cur++;
        }

      log("step");
      ret = unw_step(&c);
      log("step done:%d", ret);
      if (ret < 0)
    	error_msg_and_die("FAILURE: unw_step() returned %d", ret);
      if (ret == 0)
        break;
    }
  log("stepping ended");

  /* Check that the second and third frames are equal, but distinct of the
   * others */
  if (testcase &&
       (test_cur != 4
       || test_start_ips[1] != test_start_ips[2]
       || test_start_ips[0] == test_start_ips[1]
       || test_start_ips[2] == test_start_ips[3]
       )
     )
    {
      fprintf(stderr, "FAILURE: start IPs incorrect\n");
      return -1;
    }

  if (testcase &&
       (  strcmp(test_names[0], "a")
       || strcmp(test_names[1], "b")
       || strcmp(test_names[2], "b")
       || strcmp(test_names[3], "main")
       )
     )
    {
      fprintf(stderr, "FAILURE: procedure names are missing/incorrect\n");
      return -1;
    }

  _UCD_destroy(ui);
  unw_destroy_addr_space(as);

  return 0;
}
