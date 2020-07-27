AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4 --install

SUBDIRS = \
  include \
  src \
  test

bin_SCRIPTS = \
  mutexgear-config

release: dist-gzip dist-bzip2
	@echo Release packages for ${PACKAGE}-${VERSION} have been created.

EXTRA_DIST = \
  bmpgen \
  License-MutexGear.txt \
  README.md

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mutexgear.pc
