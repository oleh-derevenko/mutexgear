AM_CXXFLAGS = \
  -I $(top_srcdir)/include \
  -I $(top_builddir)/include

check_PROGRAMS = \
  test

TESTS = \
  test

test_SOURCES = \
  mgtest.cpp \
  pch.cpp pch.h

test_LDFLAGS= \
  @MUTEXGEAR_TEST_LIBS@ \
  @MUTEXGEAR_PRIVATE_LIBS@

test_LDADD = \
  $(top_builddir)/src/libmutexgear.la
