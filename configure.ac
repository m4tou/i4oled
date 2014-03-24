# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.61])
AC_INIT(i4oled, 1.2, przemo@firszt.eu)
AM_INIT_AUTOMAKE([-Wall -Werror])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES([DEPS], [pangocairo >= 1.29.0 pango >= 1.29.0 cairo >= 1.10.0])

# Checks for header files.
AC_CHECK_HEADER(getopt.h, [], [])
AC_CHECK_HEADER(png.h, [], [])
AC_CHECK_HEADER(stdio.h, [], [])
AC_CHECK_HEADER(string.h, [], [])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_LIB([glib-2.0], [g_base64_encode],,[])
AC_CHECK_LIB([png], [png_read_image],,[])
AC_CHECK_LIB([m], [trunc],,[])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
])
AC_OUTPUT
