dnl Process this file with autoconf to produce a configure script.
#AC_INIT(apps/tm.c)
AC_INIT(TMTK, 0.8.04, algermissen@acm.org, apps/tm.c)
AC_CONFIG_HEADER([src/config.h:src/config.h.in] )

dnl Checks for programs.
AC_PROG_CC

dnl Checks for libraries.

AC_CHECK_LIB(expat, XML_ParserCreate)
AC_CHECK_LIB(m, ceil)
AC_CHECK_LIB(raptor, raptor_new_parser)




dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h limits.h unistd.h)

#AC_DEFINE(HELLO)


dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_STRUCT_TM

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(getcwd mkdir rmdir strerror strstr)
AC_CHECK_FUNCS(snprintf vsnprintf bzero)

#AC_TRY_RUN([main() { if(sizeof(unsigned long) <= sizeof(char*)) return (0); else return (1);}],
#             worked=yes, worked=no, worked=no)
#if test $worked = yes; then
#  echo "pointer size can store longs"
#else
#  echo "unsigned long does not fit into a pointer" 
#  exit 1
#fi


AC_OUTPUT(Makefile)
