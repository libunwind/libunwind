prefix = /usr
libdir = $(prefix)/lib
includedir = $(prefix)/include

# XXX fix for cross-compilation:
ARCH = $(shell uname -m)
HOSTARCH = $(shell uname -m)
CPPFLAGS = -I$(ARCH) -I../include -D_GNU_SOURCE
CFLAGS = -Wall -g -O
SUBDIRS = tests

ia64_OBJS =						\
	__ia64_install_context.o			\
	init.o parser.o regs.o script.o tables-glibc.o	\
	unw_get_reg.o unw_resume.o unw_set_reg.o	\
	unw_init_local.o unw_init_remote.o unw_step.o

local_OBJS = backtrace.o

OBJS = $(patsubst %,$(ARCH)/%,$($(ARCH)_OBJS))

ifeq ($(ARCH),$(HOSTARCH))
	OBJS += $(local_OBJS)
else
	CPPFLAGS += -DUNW_REMOTE_ONLY
endif

LIBUNWIND = libunwind.a

all: $(LIBUNWIND) all-recursive

clean: clean-recursive
	rm -f $(LIBUNWIND) *.o $(ARCH)/*.o

distclean: clean distclean-recursive
	rm -f *.bak *~ $(ARCH)/*~ ../include/*~

install:
	install -m 644 libunwind.a $(libdir)
	install -m 644 ../include/libunwind-config.h $(includedir)
	install -m 644 ../include/libunwind.h $(includedir)
	install -m 644 ../include/libunwind-$(ARCH).h $(includedir)

%-recursive:
	@for subdir in $(SUBDIRS); do					\
		make ARCH=$(ARCH) HOSTARCH=$(HOSTARCH) -C $$subdir $*;	\
	done

libunwind.a: libunwind.a($(OBJS))

$(ARCH)/%.o: $(ARCH)/%.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(ARCH)/%.o: $(ARCH)/%.S
	$(COMPILE.S) $(OUTPUT_OPTION) $<

dep:
	makedepend $(CPPFLAGS) *.c $(ARCH)/*.c

.PHONY: clean distclean SUBDIRS

# DO NOT DELETE

backtrace.o: ../include/libunwind.h /usr/include/stdint.h
backtrace.o: /usr/include/features.h /usr/include/sys/cdefs.h
backtrace.o: /usr/include/gnu/stubs.h /usr/include/bits/wchar.h
backtrace.o: /usr/include/bits/wordsize.h ../include/libunwind-config.h
backtrace.o: ../include/libunwind-ia64.h /usr/include/ucontext.h
backtrace.o: /usr/include/sys/ucontext.h /usr/include/signal.h
backtrace.o: /usr/include/bits/sigset.h /usr/include/bits/types.h
backtrace.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h
backtrace.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
backtrace.o: /usr/include/bits/signum.h /usr/include/time.h
backtrace.o: /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h
backtrace.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
backtrace.o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
ia64/init.o: ia64/unwind_i.h /usr/include/memory.h /usr/include/features.h
ia64/init.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
ia64/init.o: /usr/include/string.h
ia64/init.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h
ia64/init.o: /usr/include/xlocale.h /usr/include/stdint.h
ia64/init.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
ia64/init.o: ../include/libunwind.h ../include/libunwind-config.h
ia64/init.o: ../include/libunwind-ia64.h /usr/include/ucontext.h
ia64/init.o: /usr/include/sys/ucontext.h /usr/include/signal.h
ia64/init.o: /usr/include/bits/sigset.h /usr/include/bits/types.h
ia64/init.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
ia64/init.o: /usr/include/bits/signum.h /usr/include/time.h
ia64/init.o: /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h
ia64/init.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
ia64/init.o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
ia64/init.o: /usr/include/stdio.h /usr/include/libio.h
ia64/init.o: /usr/include/_G_config.h /usr/include/wchar.h
ia64/init.o: /usr/include/gconv.h
ia64/init.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stdarg.h
ia64/init.o: /usr/include/bits/stdio_lim.h ia64/script.h
ia64/parser.o: /usr/include/assert.h /usr/include/features.h
ia64/parser.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
ia64/parser.o: /usr/include/stdio.h
ia64/parser.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h
ia64/parser.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
ia64/parser.o: /usr/include/bits/sched.h /usr/include/libio.h
ia64/parser.o: /usr/include/_G_config.h /usr/include/wchar.h
ia64/parser.o: /usr/include/bits/wchar.h /usr/include/gconv.h
ia64/parser.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stdarg.h
ia64/parser.o: /usr/include/bits/stdio_lim.h /usr/include/stdlib.h
ia64/parser.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
ia64/parser.o: /usr/include/endian.h /usr/include/bits/endian.h
ia64/parser.o: /usr/include/xlocale.h /usr/include/sys/types.h
ia64/parser.o: /usr/include/time.h /usr/include/sys/select.h
ia64/parser.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
ia64/parser.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
ia64/parser.o: /usr/include/alloca.h /usr/include/string.h ia64/unwind_i.h
ia64/parser.o: /usr/include/memory.h /usr/include/stdint.h
ia64/parser.o: /usr/include/bits/wordsize.h ../include/libunwind.h
ia64/parser.o: ../include/libunwind-config.h ../include/libunwind-ia64.h
ia64/parser.o: /usr/include/ucontext.h /usr/include/sys/ucontext.h
ia64/parser.o: /usr/include/signal.h /usr/include/bits/signum.h
ia64/parser.o: /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h
ia64/parser.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
ia64/parser.o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
ia64/parser.o: ia64/script.h ia64/unwind_decoder.c
ia64/regs.o: /usr/include/assert.h /usr/include/features.h
ia64/regs.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h ia64/offsets.h
ia64/regs.o: ia64/rse.h /usr/include/stdint.h /usr/include/bits/wchar.h
ia64/regs.o: /usr/include/bits/wordsize.h ia64/unwind_i.h
ia64/regs.o: /usr/include/memory.h /usr/include/string.h
ia64/regs.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h
ia64/regs.o: /usr/include/xlocale.h ../include/libunwind.h
ia64/regs.o: ../include/libunwind-config.h ../include/libunwind-ia64.h
ia64/regs.o: /usr/include/ucontext.h /usr/include/sys/ucontext.h
ia64/regs.o: /usr/include/signal.h /usr/include/bits/sigset.h
ia64/regs.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
ia64/regs.o: /usr/include/bits/sched.h /usr/include/bits/signum.h
ia64/regs.o: /usr/include/time.h /usr/include/bits/siginfo.h
ia64/regs.o: /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h
ia64/regs.o: /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h
ia64/regs.o: /usr/include/bits/sigthread.h /usr/include/stdio.h
ia64/regs.o: /usr/include/libio.h /usr/include/_G_config.h
ia64/regs.o: /usr/include/wchar.h /usr/include/gconv.h
ia64/regs.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stdarg.h
ia64/regs.o: /usr/include/bits/stdio_lim.h ia64/script.h
ia64/script.o: ia64/offsets.h ia64/rse.h /usr/include/stdint.h
ia64/script.o: /usr/include/features.h /usr/include/sys/cdefs.h
ia64/script.o: /usr/include/gnu/stubs.h /usr/include/bits/wchar.h
ia64/script.o: /usr/include/bits/wordsize.h ia64/unwind_i.h
ia64/script.o: /usr/include/memory.h /usr/include/string.h
ia64/script.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h
ia64/script.o: /usr/include/xlocale.h ../include/libunwind.h
ia64/script.o: ../include/libunwind-config.h ../include/libunwind-ia64.h
ia64/script.o: /usr/include/ucontext.h /usr/include/sys/ucontext.h
ia64/script.o: /usr/include/signal.h /usr/include/bits/sigset.h
ia64/script.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
ia64/script.o: /usr/include/bits/sched.h /usr/include/bits/signum.h
ia64/script.o: /usr/include/time.h /usr/include/bits/siginfo.h
ia64/script.o: /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h
ia64/script.o: /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h
ia64/script.o: /usr/include/bits/sigthread.h /usr/include/stdio.h
ia64/script.o: /usr/include/libio.h /usr/include/_G_config.h
ia64/script.o: /usr/include/wchar.h /usr/include/gconv.h
ia64/script.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stdarg.h
ia64/script.o: /usr/include/bits/stdio_lim.h ia64/script.h
ia64/unw_get_reg.o: ia64/unwind_i.h /usr/include/memory.h
ia64/unw_get_reg.o: /usr/include/features.h /usr/include/sys/cdefs.h
ia64/unw_get_reg.o: /usr/include/gnu/stubs.h /usr/include/string.h
ia64/unw_get_reg.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h
ia64/unw_get_reg.o: /usr/include/xlocale.h /usr/include/stdint.h
ia64/unw_get_reg.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
ia64/unw_get_reg.o: ../include/libunwind.h ../include/libunwind-config.h
ia64/unw_get_reg.o: ../include/libunwind-ia64.h /usr/include/ucontext.h
ia64/unw_get_reg.o: /usr/include/sys/ucontext.h /usr/include/signal.h
ia64/unw_get_reg.o: /usr/include/bits/sigset.h /usr/include/bits/types.h
ia64/unw_get_reg.o: /usr/include/bits/pthreadtypes.h
ia64/unw_get_reg.o: /usr/include/bits/sched.h /usr/include/bits/signum.h
ia64/unw_get_reg.o: /usr/include/time.h /usr/include/bits/siginfo.h
ia64/unw_get_reg.o: /usr/include/bits/sigaction.h
ia64/unw_get_reg.o: /usr/include/bits/sigcontext.h
ia64/unw_get_reg.o: /usr/include/asm/sigcontext.h
ia64/unw_get_reg.o: /usr/include/bits/sigstack.h
ia64/unw_get_reg.o: /usr/include/bits/sigthread.h /usr/include/stdio.h
ia64/unw_get_reg.o: /usr/include/libio.h /usr/include/_G_config.h
ia64/unw_get_reg.o: /usr/include/wchar.h /usr/include/gconv.h
ia64/unw_get_reg.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stdarg.h
ia64/unw_get_reg.o: /usr/include/bits/stdio_lim.h ia64/script.h
ia64/unw_init_local.o: /usr/include/string.h /usr/include/features.h
ia64/unw_init_local.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
ia64/unw_init_local.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h
ia64/unw_init_local.o: /usr/include/xlocale.h ia64/rse.h
ia64/unw_init_local.o: /usr/include/stdint.h /usr/include/bits/wchar.h
ia64/unw_init_local.o: /usr/include/bits/wordsize.h ia64/unwind_i.h
ia64/unw_init_local.o: /usr/include/memory.h ../include/libunwind.h
ia64/unw_init_local.o: ../include/libunwind-config.h
ia64/unw_init_local.o: ../include/libunwind-ia64.h /usr/include/ucontext.h
ia64/unw_init_local.o: /usr/include/sys/ucontext.h /usr/include/signal.h
ia64/unw_init_local.o: /usr/include/bits/sigset.h /usr/include/bits/types.h
ia64/unw_init_local.o: /usr/include/bits/pthreadtypes.h
ia64/unw_init_local.o: /usr/include/bits/sched.h /usr/include/bits/signum.h
ia64/unw_init_local.o: /usr/include/time.h /usr/include/bits/siginfo.h
ia64/unw_init_local.o: /usr/include/bits/sigaction.h
ia64/unw_init_local.o: /usr/include/bits/sigcontext.h
ia64/unw_init_local.o: /usr/include/asm/sigcontext.h
ia64/unw_init_local.o: /usr/include/bits/sigstack.h
ia64/unw_init_local.o: /usr/include/bits/sigthread.h /usr/include/stdio.h
ia64/unw_init_local.o: /usr/include/libio.h /usr/include/_G_config.h
ia64/unw_init_local.o: /usr/include/wchar.h /usr/include/gconv.h
ia64/unw_init_local.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stdarg.h
ia64/unw_init_local.o: /usr/include/bits/stdio_lim.h ia64/script.h
ia64/unw_init_remote.o: ia64/rse.h /usr/include/stdint.h
ia64/unw_init_remote.o: /usr/include/features.h /usr/include/sys/cdefs.h
ia64/unw_init_remote.o: /usr/include/gnu/stubs.h /usr/include/bits/wchar.h
ia64/unw_init_remote.o: /usr/include/bits/wordsize.h ia64/unwind_i.h
ia64/unw_init_remote.o: /usr/include/memory.h /usr/include/string.h
ia64/unw_init_remote.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h
ia64/unw_init_remote.o: /usr/include/xlocale.h ../include/libunwind.h
ia64/unw_init_remote.o: ../include/libunwind-config.h
ia64/unw_init_remote.o: ../include/libunwind-ia64.h /usr/include/ucontext.h
ia64/unw_init_remote.o: /usr/include/sys/ucontext.h /usr/include/signal.h
ia64/unw_init_remote.o: /usr/include/bits/sigset.h /usr/include/bits/types.h
ia64/unw_init_remote.o: /usr/include/bits/pthreadtypes.h
ia64/unw_init_remote.o: /usr/include/bits/sched.h /usr/include/bits/signum.h
ia64/unw_init_remote.o: /usr/include/time.h /usr/include/bits/siginfo.h
ia64/unw_init_remote.o: /usr/include/bits/sigaction.h
ia64/unw_init_remote.o: /usr/include/bits/sigcontext.h
ia64/unw_init_remote.o: /usr/include/asm/sigcontext.h
ia64/unw_init_remote.o: /usr/include/bits/sigstack.h
ia64/unw_init_remote.o: /usr/include/bits/sigthread.h /usr/include/stdio.h
ia64/unw_init_remote.o: /usr/include/libio.h /usr/include/_G_config.h
ia64/unw_init_remote.o: /usr/include/wchar.h /usr/include/gconv.h
ia64/unw_init_remote.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stdarg.h
ia64/unw_init_remote.o: /usr/include/bits/stdio_lim.h ia64/script.h
ia64/unw_is_signal_frame.o: ia64/unwind_i.h /usr/include/memory.h
ia64/unw_is_signal_frame.o: /usr/include/features.h /usr/include/sys/cdefs.h
ia64/unw_is_signal_frame.o: /usr/include/gnu/stubs.h /usr/include/string.h
ia64/unw_is_signal_frame.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h
ia64/unw_is_signal_frame.o: /usr/include/xlocale.h /usr/include/stdint.h
ia64/unw_is_signal_frame.o: /usr/include/bits/wchar.h
ia64/unw_is_signal_frame.o: /usr/include/bits/wordsize.h
ia64/unw_is_signal_frame.o: ../include/libunwind.h
ia64/unw_is_signal_frame.o: ../include/libunwind-config.h
ia64/unw_is_signal_frame.o: ../include/libunwind-ia64.h
ia64/unw_is_signal_frame.o: /usr/include/ucontext.h
ia64/unw_is_signal_frame.o: /usr/include/sys/ucontext.h /usr/include/signal.h
ia64/unw_is_signal_frame.o: /usr/include/bits/sigset.h
ia64/unw_is_signal_frame.o: /usr/include/bits/types.h
ia64/unw_is_signal_frame.o: /usr/include/bits/pthreadtypes.h
ia64/unw_is_signal_frame.o: /usr/include/bits/sched.h
ia64/unw_is_signal_frame.o: /usr/include/bits/signum.h /usr/include/time.h
ia64/unw_is_signal_frame.o: /usr/include/bits/siginfo.h
ia64/unw_is_signal_frame.o: /usr/include/bits/sigaction.h
ia64/unw_is_signal_frame.o: /usr/include/bits/sigcontext.h
ia64/unw_is_signal_frame.o: /usr/include/asm/sigcontext.h
ia64/unw_is_signal_frame.o: /usr/include/bits/sigstack.h
ia64/unw_is_signal_frame.o: /usr/include/bits/sigthread.h
ia64/unw_is_signal_frame.o: /usr/include/stdio.h /usr/include/libio.h
ia64/unw_is_signal_frame.o: /usr/include/_G_config.h /usr/include/wchar.h
ia64/unw_is_signal_frame.o: /usr/include/gconv.h
ia64/unw_is_signal_frame.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stdarg.h
ia64/unw_is_signal_frame.o: /usr/include/bits/stdio_lim.h ia64/script.h
ia64/unw_resume.o: ia64/unwind_i.h /usr/include/memory.h
ia64/unw_resume.o: /usr/include/features.h /usr/include/sys/cdefs.h
ia64/unw_resume.o: /usr/include/gnu/stubs.h /usr/include/string.h
ia64/unw_resume.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h
ia64/unw_resume.o: /usr/include/xlocale.h /usr/include/stdint.h
ia64/unw_resume.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
ia64/unw_resume.o: ../include/libunwind.h ../include/libunwind-config.h
ia64/unw_resume.o: ../include/libunwind-ia64.h /usr/include/ucontext.h
ia64/unw_resume.o: /usr/include/sys/ucontext.h /usr/include/signal.h
ia64/unw_resume.o: /usr/include/bits/sigset.h /usr/include/bits/types.h
ia64/unw_resume.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
ia64/unw_resume.o: /usr/include/bits/signum.h /usr/include/time.h
ia64/unw_resume.o: /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h
ia64/unw_resume.o: /usr/include/bits/sigcontext.h
ia64/unw_resume.o: /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h
ia64/unw_resume.o: /usr/include/bits/sigthread.h /usr/include/stdio.h
ia64/unw_resume.o: /usr/include/libio.h /usr/include/_G_config.h
ia64/unw_resume.o: /usr/include/wchar.h /usr/include/gconv.h
ia64/unw_resume.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stdarg.h
ia64/unw_resume.o: /usr/include/bits/stdio_lim.h ia64/script.h
ia64/unw_set_reg.o: ia64/unwind_i.h /usr/include/memory.h
ia64/unw_set_reg.o: /usr/include/features.h /usr/include/sys/cdefs.h
ia64/unw_set_reg.o: /usr/include/gnu/stubs.h /usr/include/string.h
ia64/unw_set_reg.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h
ia64/unw_set_reg.o: /usr/include/xlocale.h /usr/include/stdint.h
ia64/unw_set_reg.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
ia64/unw_set_reg.o: ../include/libunwind.h ../include/libunwind-config.h
ia64/unw_set_reg.o: ../include/libunwind-ia64.h /usr/include/ucontext.h
ia64/unw_set_reg.o: /usr/include/sys/ucontext.h /usr/include/signal.h
ia64/unw_set_reg.o: /usr/include/bits/sigset.h /usr/include/bits/types.h
ia64/unw_set_reg.o: /usr/include/bits/pthreadtypes.h
ia64/unw_set_reg.o: /usr/include/bits/sched.h /usr/include/bits/signum.h
ia64/unw_set_reg.o: /usr/include/time.h /usr/include/bits/siginfo.h
ia64/unw_set_reg.o: /usr/include/bits/sigaction.h
ia64/unw_set_reg.o: /usr/include/bits/sigcontext.h
ia64/unw_set_reg.o: /usr/include/asm/sigcontext.h
ia64/unw_set_reg.o: /usr/include/bits/sigstack.h
ia64/unw_set_reg.o: /usr/include/bits/sigthread.h /usr/include/stdio.h
ia64/unw_set_reg.o: /usr/include/libio.h /usr/include/_G_config.h
ia64/unw_set_reg.o: /usr/include/wchar.h /usr/include/gconv.h
ia64/unw_set_reg.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stdarg.h
ia64/unw_set_reg.o: /usr/include/bits/stdio_lim.h ia64/script.h
ia64/unw_step.o: ia64/offsets.h ia64/rse.h /usr/include/stdint.h
ia64/unw_step.o: /usr/include/features.h /usr/include/sys/cdefs.h
ia64/unw_step.o: /usr/include/gnu/stubs.h /usr/include/bits/wchar.h
ia64/unw_step.o: /usr/include/bits/wordsize.h ia64/unwind_i.h
ia64/unw_step.o: /usr/include/memory.h /usr/include/string.h
ia64/unw_step.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h
ia64/unw_step.o: /usr/include/xlocale.h ../include/libunwind.h
ia64/unw_step.o: ../include/libunwind-config.h ../include/libunwind-ia64.h
ia64/unw_step.o: /usr/include/ucontext.h /usr/include/sys/ucontext.h
ia64/unw_step.o: /usr/include/signal.h /usr/include/bits/sigset.h
ia64/unw_step.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
ia64/unw_step.o: /usr/include/bits/sched.h /usr/include/bits/signum.h
ia64/unw_step.o: /usr/include/time.h /usr/include/bits/siginfo.h
ia64/unw_step.o: /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h
ia64/unw_step.o: /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h
ia64/unw_step.o: /usr/include/bits/sigthread.h /usr/include/stdio.h
ia64/unw_step.o: /usr/include/libio.h /usr/include/_G_config.h
ia64/unw_step.o: /usr/include/wchar.h /usr/include/gconv.h
ia64/unw_step.o: /usr/lib/gcc-lib/i386-linux/2.95.4/include/stdarg.h
ia64/unw_step.o: /usr/include/bits/stdio_lim.h ia64/script.h
