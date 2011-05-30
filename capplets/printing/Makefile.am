INCLUDES = 						\
	$(GNOMECC_CAPPLETS_CFLAGS)			\
	-I$(top_srcdir)/libgnome-control-center-extension	\
	-DGNOMELOCALEDIR="\"$(datadir)/locale\""	\
	-DGNOMECC_DATA_DIR="\"$(pkgdatadir)\""		\
	-DGNOMECC_UI_DIR="\"$(uidir)\""

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" $(PANEL_CFLAGS)

AM_CFLAGS = $(EXTENSION_CFLAGS) $(EXTENSION_COMMON_CFLAGS)

libdir = $(EXTENSIONSDIR)
lib_LTLIBRARIES = libprinting.la


libprinting_la_SOURCES =     \
	lib.c                \
	cc-printing-panel.h  \
	cc-printing-panel.c

libprinting_la_LDFLAGS = $(EXTENSION_LIBTOOL_FLAGS)

libprinting_la_LIBADD = $(EXTENSION_LIBS) $(EXTENSION_COMMON_LIBS)


-include $(top_srcdir)/git.mk
