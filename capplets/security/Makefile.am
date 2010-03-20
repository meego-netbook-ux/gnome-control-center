cappletname = security

NULL =

security_SOURCES = \
	security.c \
	run-passwd.h \
	run-passwd.c \
	$(NULL)

ui_files = \
	security.ui \
	$(NULL)


if BUILD_SECURITY

bin_PROGRAMS = security

security_LDADD = \
	$(GNOMECC_CAPPLETS_LIBS) \
	$(MX_GTK_LIBS) \
	$(top_builddir)/libgnome-control-center-extension/libcommon.la \
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

endif # BUILD_SECURITY

CLEANFILES = \
	$(GNOMECC_CAPPLETS_CLEANFILES) \
	$(desktop_in_files) \
	$(desktop_DATA)
	$(NULL)

EXTRA_DIST = \
	$(ui_DATA) \
	$(NULL)

-include $(top_srcdir)/git.mk
