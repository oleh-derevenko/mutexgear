AM_CPPFLAGS = \
  @MUTEXGEAR_D__EXT@

AM_CXXFLAGS = \
  -I $(top_srcdir)/include \
  -I $(top_builddir)/include

check_PROGRAMS = \
  mgtest

if SAME_HOST_BUILD
TESTS = \
  mgtest
endif

mgtest_SOURCES = \
  mgtest.cpp \
  pch.cpp pch.h

mgtest_LDFLAGS= \
  @MUTEXGEAR_TEST_LIBS@ \
  @MUTEXGEAR_PRIVATE_LIBS@

mgtest_LDADD = \
  $(top_builddir)/src/libmutexgear.la
