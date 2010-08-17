cappletname = security

NULL =

libsecurity_la_SOURCES = \
	security-module.c \
	cc-security-panel.c \
	cc-security-panel.h \
	run-passwd.h \
	run-passwd.c \
	$(NULL)

ccmodulesdir = $(PANELS_DIR)
ccmodules_LTLIBRARIES = libsecurity.la

libsecurity_la_CFLAGS = $(PANEL_CFLAGS)

libsecurity_la_LDFLAGS = $(PANEL_LDFLAGS)

libsecurity_la_LIBADD = \
	$(PANEL_LIBS) \
	$(MX_GTK_LIBS) \
	$(NULL)

@INTLTOOL_DESKTOP_RULE@

ui_files = security.ui
uidir    = $(pkgdatadir)/ui
ui_DATA  = $(ui_files)



desktopdir = $(datadir)/applications
desktop_in_files = security.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

INCLUDES = \
	$(GNOMECC_CAPPLETS_CFLAGS) \
	$(MX_GTK_CFLAGS) \
	-I$(top_srcdir)/libgnome-control-center \
	-DUIDIR=\""$(uidir)"\" \
	-DGNOMELOCALEDIR="\"$(datadir)/locale\"" \
	$(NULL)


CLEANFILES = \
	$(GNOMECC_CAPPLETS_CLEANFILES) \
	$(desktop_in_files) \
	$(desktop_DATA) \
	$(NULL)

EXTRA_DIST = \
	$(ui_DATA) \
	$(NULL)

-include $(top_srcdir)/git.mk
