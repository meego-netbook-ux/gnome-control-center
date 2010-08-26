cappletname = security

NULL =

libsecurity_la_SOURCES = \
	security-module.c \
	cc-security-panel.c \
	cc-security-panel.h \
	run-passwd.h \
	run-passwd.c \
	$(NULL)

ui_files = \
	security.ui \
	$(NULL)


libsecurity_la_CFLAGS = \
	$(EXTENSION_CFLAGS) \
	$(EXTENSION_COMMON_CFLAGS)

libsecurity_la_LDFLAGS = \
	$(EXTENSION_LIBTOOL_FLAGS)

libsecurity_la_LIBADD = \
	$(EXTENSION_LIBS) \
	$(EXTENSION_COMMON_LIBS) \
	$(MX_GTK_LIBS) \
	$(NULL)

@INTLTOOL_DESKTOP_RULE@

uidir   = $(pkgdatadir)/ui
ui_DATA = $(ui_files)

desktopdir = $(datadir)/applications
desktop_in_files = security.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

INCLUDES = \
	$(GNOMECC_CAPPLETS_CFLAGS) \
	$(MX_GTK_CFLAGS) \
	-I$(top_srcdir)/libgnome-control-center-extension \
	-DUIDIR=\""$(uidir)"\" \
	-DGNOMELOCALEDIR="\"$(datadir)/locale\"" \
	$(NULL)

ccmodulesdir = $(EXTENSIONSDIR)
ccmodules_LTLIBRARIES = libsecurity.la

CLEANFILES = \
	$(GNOMECC_CAPPLETS_CLEANFILES) \
	$(desktop_in_files) \
	$(desktop_DATA) \
	$(NULL)

EXTRA_DIST = \
	$(ui_DATA) \
	$(NULL)

-include $(top_srcdir)/git.mk
