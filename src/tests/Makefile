CPPFLAGS = -I../../include
CFLAGS = -g -O -Wall

local_PROGS = bt exc

ifeq ($(ARCH),$(HOSTARCH))
  PROGS += $(local_PROGS)
endif

all: $(PROGS)

clean:
	rm -f *.o $(PROGS)

distclean: clean
	rm -f *~

.PHONY: clean distclean

verify: verify.o ../libunwind.a
exc: exc.o ../libunwind.a
sig: sig.o ../libunwind.a
bt: bt.o ../libunwind.a
