# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.61])
AC_INIT(i4oled, 1.0-rc1, przemo@firszt.eu)
AM_INIT_AUTOMAKE([-Wall -Werror])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES([DEPS], [pangocairo >= 1.29.0])
PKG_CHECK_MODULES([DEPS], [pango >= 1.29.0])
PKG_CHECK_MODULES([DEPS], [cairo >= 1.10.0])

# Checks for header files.
AC_CHECK_HEADER(getopt.h, [], [a=0])
AC_CHECK_HEADER(png.h, [], [a=0])
AC_CHECK_HEADER(stdio.h, [], [a=0])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_LIB([png], [png_read_image],,[])
AC_CHECK_LIB([m], [trunc],,[])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
])
AC_OUTPUT
