NULL =

uidir = $(pkgdatadir)/ui

INCLUDES =						\
	-DEXTENSIONSDIR="\"$(EXTENSIONSDIR)\""			\
	-DGNOMECC_DATA_DIR="\"$(pkgdatadir)\""				\
	-DGNOMELOCALEDIR="\"$(datadir)/locale\""			\
	-DLIBEXECDIR="\"$(libexecdir)\""				\
	-DGTK_ENGINE_DIR="\"$(GTK_ENGINE_DIR)\"" 			\
	-DINSTALL_PREFIX=\"$(prefix)\"					\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/libwindow-settings				\
	-DPIXMAP_DIR=\""$(datadir)/gnome-control-center/pixmaps"\"	\
	-DUIDIR="\"$(uidir)\""					\
	$(CAPPLET_CFLAGS)						\
	$(DBUS_CFLAGS)							\
	$(GNOME_DESKTOP_CFLAGS)						\
	$(WINDOW_MANAGER_CFLAGS)						\
	$(GSD_DBUS_CFLAGS)						\
	$(GIO_CFLAGS)
	$(EXTENSION_COMMON_CFLAGS)			\
	$(NULL)



libexec_PROGRAMS = cc-theme-thumbnailer-helper

cc_theme_thumbnailer_helper_DEPENDENCIES = libgnome-control-center-extension.la

cc_theme_thumbnailer_helper_SOURCES =	\
	cc-theme-thumbnailer-slave.c	\
	cc-theme-thumbnailer-slave.h	\
	cc-theme-thumbnailer-helper.c	\
	$(NULL)

cc_theme_thumbnailer_helper_LDADD = 	\
	$(WINDOW_MANAGER_LIBS)		\
	$(GNOMECC_CAPPLETS_LIBS)	\
	$(NULL)

#noinst_LTLIBRARIES = libcommon.la

libcommon_la_sources = \
	activate-settings-daemon.c	\
	activate-settings-daemon.h	\
	capplet-stock-icons.c		\
	capplet-stock-icons.h		\
	capplet-util.c			\
	capplet-util.h			\
	file-transfer-dialog.c		\
	file-transfer-dialog.h		\
	gconf-property-editor.c		\
	gconf-property-editor.h		\
	gconf-property-editor-marshal.c	\
	gconf-property-editor-marshal.h \
	gnome-theme-apply.c		\
	gnome-theme-apply.h 		\
	gnome-theme-info.c		\
	gnome-theme-info.h		\
	gtkrc-utils.c			\
	gtkrc-utils.h			\
	cc-theme-thumbnailer.c		\
	cc-theme-thumbnailer.h		\
	theme-thumbnail.c		\
	theme-thumbnail.h		\
	wm-common.c			\
	wm-common.h

#libcommon_la_LIBADD =							\
#	$(top_builddir)/libwindow-settings/libgnome-window-settings.la  \
#	$(DBUS_LIBS)							\
#	$(GNOME_DESKTOP_LIBS)						\
#	$(GIO_LIBS)

#libcommon_la_LDFLAGS = -export-dynamic -no-undefined -avoid-version

lib_LTLIBRARIES = libgnome-control-center-extension.la

libgnome_control_center_extension_include_HEADERS =	\
	cc-page.h					\
	cc-panel.h					\
	cc-shell.h					\
	mux-label.h					\
	$(NULL)

libgnome_control_center_extension_la_SOURCES =		\
	$(libcommon_la_sources)				\
	cc-page.h					\
	cc-page.c					\
	cc-panel.h					\
	cc-panel.c					\
	cc-shell.c					\
	cc-shell.h					\
	mux-label.h					\
	mux-label.c					\
	$(NULL)

#        -version-info @GNOME_CONTROL_CENTER_EXTENSION_VERSION_INFO@
libgnome_control_center_extension_la_LDFLAGS =		\
        -no-undefined					\
        $(NULL)

libgnome_control_center_extension_la_LIBADD = 		\
	$(top_builddir)/libwindow-settings/libgnome-window-settings.la  \
	$(WINDOW_MANAGER_LIBS)				\
	$(DBUS_LIBS)					\
	$(GNOME_DESKTOP_LIBS)				\
	$(GIO_LIBS)					\
	$(EXTENSION_COMMON_CFLAGS)			\
	$(NULL)

libgnome_control_center_extension_la_LIBTOOLFLAGS = --tag=disable-static

libgnome_control_center_extension_includedir = $(includedir)/gnome-control-center-1/libgnome-control-center-extension



gnome_theme_test_SOURCES = \
	gnome-theme-test.c

gnome_theme_test_LDADD = 						\
	$(GNOMECC_CAPPLETS_LIBS)

gnome_theme_test_DEPENDENCIES = libgnome-control-center-extension.la

noinst_PROGRAMS = \
	gnome-theme-test

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=libgnome-control-center-extension.pc

EXTRA_DIST =					\
        libgnome-control-center-extension.pc.in	\
        $(NULL)

AM_CPPFLAGS =							\
	-DGNOMELOCALEDIR="\"$(datadir)/locale\""		\
	-DEXTENSION_DIR="\"$(libdir)/control-center-1/extensions\""					\
	-DUIDIR="\"$(uidir)\""					\
	-DMENUDIR="\"$(menudir)\""

-include $(top_srcdir)/git.mk
