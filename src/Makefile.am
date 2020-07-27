AM_CPPFLAGS = \
  -D_MUTEXGEAR_DLL

AM_CFLAGS = \
  -I $(top_srcdir)/include \
  -I $(top_builddir)/include \
  -DNDEBUG

lib_LTLIBRARIES = \
  libmutexgear.la

libmutexgear_la_LDFLAGS = \
  @MUTEXGEAR_VERSION_INFO@ \
  @MUTEXGEAR_PRIVATE_LIBS@

libmutexgear_la_SOURCES = \
  completion.c completion.h \
  dlralist.h \
  rwlock.c \
  toggle.c \
  utility.c utility.h \
  wheel.c wheel.h
