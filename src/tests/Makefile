CPPFLAGS = -I../../include
CFLAGS = -g -O -Wall

PROGS = bt exc

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
