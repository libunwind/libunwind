AC_DEFUN([LIBUNWIND___THREAD],
[dnl Check whether the compiler supports the __thread keyword.
if test "x$enable___thread" != xno; then
  AC_CACHE_CHECK([for __thread], libc_cv_gcc___thread,
		 [cat > conftest.c <<\EOF
    __thread int a = 42;
EOF
  if AC_TRY_COMMAND([${CC-cc} $CFLAGS -c conftest.c >&AS_MESSAGE_LOG_FD]); then
    libc_cv_gcc___thread=yes
  else
    libc_cv_gcc___thread=no
  fi
  rm -f conftest*])
  if test "$libc_cv_gcc___thread" = yes; then
    AC_DEFINE(HAVE___THREAD, 1,
	[Define to 1 if __thread keyword is supported by the C compiler.])
  fi
else
  libc_cv_gcc___thread=no
fi])
