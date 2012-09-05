# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.61])
AC_INIT(i4oled, 0.1, przemo@firszt.eu)
AM_INIT_AUTOMAKE([-Wall -Werror])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADER(fnctl.h, [], [a=0])
AC_CHECK_HEADER(getopt.h, [], [a=0])
AC_CHECK_HEADER(png.h, [], [a=0])
AC_CHECK_HEADER(stdio.h, [], [a=0])


# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_LIB([png], [png_read_image],,[])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
])
AC_OUTPUT
