#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <execinfo.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <unistd.h>
#include <sys/ptrace.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

/*
 * unwind in the signal handler checking the backtrace is correct
 * after a bad jump.
 */
void handle_sigsegv(int signal, siginfo_t *info, void *ucontext)
{
	/*
	 * 0 = success
	 * !0 = general failure
	 * 77 = test skipped
	 * 99 = complete failure
	 */
	int test_status = 0; 

	unw_cursor_t cursor; unw_context_t uc;
	unw_word_t ip, sp, offset;
	char name[1000];
	int found_signal_frame = 0;
	int i = 0;
	char *names[] = {
		"",
		"main",
	};
	int names_count = sizeof(names) / sizeof(*names);

	unw_getcontext(&uc);
	unw_init_local(&cursor, &uc);

	while (unw_step(&cursor) > 0 && !test_status) {
		if(unw_is_signal_frame(&cursor)) {
			found_signal_frame = 1;
		}
		if(found_signal_frame) {
			unw_get_reg(&cursor, UNW_REG_IP, &ip);
			unw_get_reg(&cursor, UNW_REG_SP, &sp);
			memset(name, 0, sizeof(char) * 1000);
			unw_get_proc_name(&cursor, name, sizeof(char) * 1000, &offset);
			printf("ip = %lx, sp = %lx offset = %lx name = %s\n", (long) ip, (long) sp, (long) offset, name);
			if(i < names_count) {
				if(strcmp(names[i], name) != 0) {
					test_status = 1;
					printf("frame %s doesn't match expected frame %s\n", name, names[i]);
				} else {
					i += 1;
				}
			}
		}
	}

	if(i != names_count) { //Make sure we found all the frames!
		printf("Failed to find all frames i:%d != names_count:%d\n", i, names_count);
		test_status = 1;
	}

	/*return test_status to test harness*/
	exit(test_status);
}


void (*invalid_function)() = (void*)1;

int main(int argc, char *argv[])
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = handle_sigsegv;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &sa, NULL);

	invalid_function();

	/*
	 * 99 is the hard error exit status for automake tests:
	 * https://www.gnu.org/software/automake/manual/html_node/Scripts_002dbased-Testsuites.html#Scripts_002dbased-Testsuites
	 * If we dont end up in the signal handler something went horribly wrong.
	 */
	return 99;
}
