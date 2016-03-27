dnl Functions for libfuse
dnl
dnl Version: 20111009

dnl Function to detect if libfuse is available
dnl ac_libfuse_dummy is used to prevent AC_CHECK_LIB adding unnecessary -l<library> arguments
AC_DEFUN([AX_LIBFUSE_CHECK_LIB],
 [dnl Check if parameters were provided
 AS_IF(
  [test "x$ac_cv_with_libfuse" != x && test "x$ac_cv_with_libfuse" != xno && test "x$ac_cv_with_libfuse" != xauto-detect],
  [AS_IF(
   [test -d "$ac_cv_with_libfuse"],
   [CFLAGS="$CFLAGS -I${ac_cv_with_libfuse}/include"
   LDFLAGS="$LDFLAGS -L${ac_cv_with_libfuse}/lib"],
   [AC_MSG_WARN([no such directory: $ac_cv_with_libfuse])
   ])
  ])

 dnl Check for headers
 ac_cv_libfuse=no

 AS_IF(
  [test "x$ac_cv_with_libfuse" != xno],
  [AC_CHECK_HEADERS([fuse.h])

  dnl libfuse sometimes requires -D_FILE_OFFSET_BITS=64 to be set
  AS_IF(
   [test "x$ac_cv_header_fuse_h" = xno],
   [AS_UNSET([ac_cv_header_fuse_h])
   CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
   AC_CHECK_HEADERS([fuse.h])
  ])

  AS_IF(
   [test "x$ac_cv_header_fuse_h" = xno],
   [ac_cv_libfuse=no],
   [ac_cv_libfuse=yes

   AC_CHECK_LIB(
    fuse,
    fuse_daemonize,
    [ac_cv_libfuse_dummy=yes],
    [ac_cv_libfuse=no])
   AC_CHECK_LIB(
    fuse,
    fuse_destroy,
    [ac_cv_libfuse_dummy=yes],
    [ac_cv_libfuse=no])
   AC_CHECK_LIB(
    fuse,
    fuse_mount,
    [ac_cv_libfuse_dummy=yes],
    [ac_cv_libfuse=no])
   AC_CHECK_LIB(
    fuse,
    fuse_new,
    [ac_cv_libfuse_dummy=yes],
    [ac_cv_libfuse=no])
   ])
  ])

 AS_IF(
  [test "x$ac_cv_libfuse" = xyes],
  [AC_SUBST(
   [LIBFUSE_LIBADD],
   ["-lfuse"])
  AC_DEFINE(
   [HAVE_LIBFUSE],
   [1],
   [Define to 1 if you have the 'fuse' library (-lfuse).])
  ])

 AS_IF(
  [test "x$ac_cv_libfuse" = xyes],
  [AC_SUBST(
   [HAVE_LIBFUSE],
   [1]) ],
  [AC_SUBST(
   [HAVE_LIBFUSE],
   [0])
  ])
 ])

dnl Function to detect how to enable libfuse
AC_DEFUN([AX_LIBFUSE_CHECK_ENABLE],
 [AX_COMMON_ARG_WITH(
  [libfuse],
  [libfuse],
  [search for libfuse in includedir and libdir or in the specified DIR, or no if not to use libfuse],
  [auto-detect],
  [DIR])

 AX_LIBFUSE_CHECK_LIB

 AS_IF(
  [test "x$ac_cv_libfuse" = xyes],
  [AC_SUBST(
   [ax_libfuse_pc_libs_private],
   [-lfuse])
  ])

 AS_IF(
  [test "x$ac_cv_libfuse" = xyes],
  [AC_SUBST(
   [ax_libfuse_spec_requires],
   [fuse-libs])
  AC_SUBST(
   [ax_libfuse_spec_build_requires],
   [fuse-devel])
  ])
 ])

