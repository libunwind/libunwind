AC_DEFUN([CHECK_ATOMIC_OPS],
[dnl Check whether the system has the atomic_ops package installed.
  AC_CHECK_HEADERS(atomic_ops.h)
#
# Don't link against libatomic_ops for now.  We don't want libunwind
# to depend on libatomic_ops.so.  Fortunately, none of the platforms
# we care about so far need libatomic_ops.a (everything is done via
# inline macros).
#
#  AC_CHECK_LIB(atomic_ops, main)
])
