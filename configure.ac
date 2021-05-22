
AC_INIT([ancient], [2.0.0-pre.2], [https://github.com/temisu/ancient/issues], [ancient], [https://github.com/temisu/ancient/])
#                   ^^^^^^^^^^^
# Package Version
#
# MAJOR.MINOR.PATCH[-PRE][+META]
#
# *  increment MAJOR and set MINOR=0 and PATCH=0 for incompatible ABI/API changes (changed/removed API elements)
# *  increment MINOR and set PATCH=0 for compatible ABI/API changes (added API elements)
# *  increment PATCH and set PRE=-pre.0 immediately after every release, always
# *  increment PRE at free will
# *  for a release, set PRE="", i.e. the version number should look like "2.32.42" or maybe "2.23.42+autotools"
# *  eveything appended after "+" is considered build/variant metadata is not part of version ordering
# For details, see <https://semver.org/spec/v2.0.0.html>.
#

#
# Libtool version-info (used to determine soname)
#
# *  increment REVISION immediately after every release, always
# *  increment CURRENT when the next release contains any ABI changes (added/changed/removed API elements), and set REVISION=0
# *  increment AGE when the next release stays compatible with the previous release (only added API elements)
# *  set AGE=0 when the next release is incompatible with the previous release (changed/removed API elements)
# For details, see <https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html>.
LIBANCIENT_LTVER_CURRENT=2
LIBANCIENT_LTVER_REVISION=0
LIBANCIENT_LTVER_AGE=0
#

#
# Dependencies (Debian/Ubuntu package names):
#
# *  autoconf
# *  autoconf-archive
# *  automake
# *  libtool
# *  make
# *  pkg-config
#

#
# Release procedure:
#
#     ./autogen.sh
#     ./configure
#     make distcheck
#     make distclean
#

#
# Verbose make output:
#     ./configure --disable-silent-rules
#

AC_PREREQ([2.68])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([Makefile libancient.pc])

AM_INIT_AUTOMAKE([1.11 -Wall -Werror foreign subdir-objects])

AM_PROG_AR

LT_INIT

AC_SYS_LARGEFILE

PKG_PROG_PKG_CONFIG([0.24])

AM_SILENT_RULES([yes])

# C
#AC_PROG_CC
#AM_PROG_CC_C_O

# C++
AC_PROG_CXX

AC_PROG_INSTALL

AC_SUBST([LIBANCIENT_LTVER_CURRENT])
AC_SUBST([LIBANCIENT_LTVER_REVISION])
AC_SUBST([LIBANCIENT_LTVER_AGE])

# C
#AC_PROG_CC_STDC
#AC_PROG_CC_C99

# C++
AX_CXX_COMPILE_STDCXX(17, [noext], [mandatory])

# internal (non-exposed) dependencies (pkg-config) for .pc file
LIBANCIENT_PC_REQUIRES_PRIVATE=
# internal (non-exposed) dependencies (plain linker) for .pc file
LIBANCIENT_PC_LIBS_PRIVATE=
# CFLAGS required to use the library for .pc file
LIBANCIENT_PC_CFLAGS=
# ANCIENT_API macro for .pc file
LIBANCIENT_PC_ANCIENT_API=

# C
#AC_LANG_PUSH([C])
#AX_CHECK_COMPILE_FLAG([-fvisibility=hidden], [CFLAGS="$CFLAGS -fvisibility=hidden"])
#AX_CFLAGS_WARN_ALL
#AC_LANG_POP([C])

# C++
AC_LANG_PUSH([C++])
AX_CHECK_COMPILE_FLAG([-fvisibility=hidden], [CXXFLAGS="$CXXFLAGS -fvisibility=hidden"])
AX_CHECK_COMPILE_FLAG([-fvisibility=default], [AC_DEFINE([ANCIENT_API], [__attribute__((visibility("default")))])])
AX_CHECK_COMPILE_FLAG([-fvisibility=default], [AC_SUBST([LIBANCIENT_PC_ANCIENT_API], [-DANCIENT_API_VISIBILITY_DEFAULT])])
AX_CXXFLAGS_WARN_ALL
AC_LANG_POP([C++])

AC_SUBST([LIBANCIENT_PC_REQUIRES_PRIVATE])
AC_SUBST([LIBANCIENT_PC_LIBS_PRIVATE])
AC_SUBST([LIBANCIENT_PC_CFLAGS])
AC_SUBST([LIBANCIENT_PC_ANCIENT_API])

AC_OUTPUT
