SUBDIRS = data

# This is used in GNOMECC_CAPPLETS_CFLAGS
cappletname = appearance

bin_PROGRAMS = gnome-appearance-properties

gnome_appearance_properties_SOURCES = \
	appearance.h \
	appearance-desktop.c \
	appearance-desktop.h \
	appearance-font.c \
	appearance-font.h \
	appearance-main.c \
	appearance-themes.c \
	appearance-themes.h \
	appearance-style.c \
	appearance-style.h \
	gnome-wp-info.c \
	gnome-wp-info.h \
	gnome-wp-item.c \
	gnome-wp-item.h \
	gnome-wp-xml.c \
	gnome-wp-xml.h \
	theme-installer.c \
	theme-installer.h \
	theme-save.c \
	theme-save.h \
	theme-util.c \
	theme-util.h


ccmodulesdir = $(EXTENSIONSDIR)
ccmodules_LTLIBRARIES = libappearance.la

libappearance_la_SOURCES =		\
	appearance.h \
	appearance-desktop.c \
	appearance-desktop.h \
	appearance-font.c \
	appearance-font.h \
	appearance-main.c \
	appearance-themes.c \
	appearance-themes.h \
	appearance-style.c \
	appearance-style.h \
	gnome-wp-info.c \
	gnome-wp-info.h \
	gnome-wp-item.c \
	gnome-wp-item.h \
	gnome-wp-xml.c \
	gnome-wp-xml.h \
	theme-installer.c \
	theme-installer.h \
	theme-save.c \
	theme-save.h \
	theme-util.c \
	theme-util.h \
	appearance-module.c		\
	cc-appearance-panel.h		\
	cc-appearance-panel.c \
	$(NULL)

libappearance_la_LDFLAGS =		\
	$(EXTENSION_LIBTOOL_FLAGS)	\
	$(NULL)

libappearance_la_LIBADD =		\
	$(EXTENSION_LIBS)		\
	$(FONT_CAPPLET_LIBS)		\
	$(EXTENSION_COMMON_LIBS)	\
	$(NULL)

libappearance_la_CFLAGS =		\
	$(EXTENSION_CFLAGS)		\
	$(EXTENSION_COMMON_CFLAGS)	\
	$(NULL)

libappearance_la_LIBTOOLFLAGS = --tag=disable-static


gnome_appearance_properties_LDADD = \
	$(top_builddir)/libwindow-settings/libgnome-window-settings.la \
	$(GNOMECC_CAPPLETS_LIBS) \
	$(FONT_CAPPLET_LIBS) \
	$(METACITY_LIBS)
gnome_appearance_properties_LDFLAGS = -export-dynamic

gtkbuilderdir = $(pkgdatadir)/ui
pixmapdir = $(pkgdatadir)/pixmaps
wallpaperdir = $(datadir)/gnome-background-properties

INCLUDES = \
	$(METACITY_CFLAGS) \
	$(GNOMECC_CAPPLETS_CFLAGS) \
	$(FONT_CAPPLET_CFLAGS) \
	-DGNOMELOCALEDIR="\"$(datadir)/locale\"" \
	-DGNOMECC_DATA_DIR="\"$(pkgdatadir)\"" \
	-DGNOMECC_GTKBUILDER_DIR="\"$(gtkbuilderdir)\"" \
	-DGNOMECC_PIXMAP_DIR="\"$(pixmapdir)\"" \
	-DWALLPAPER_DATADIR="\"$(wallpaperdir)\"" \
	-I$(top_srcdir)/libgnome-control-center-extension/ \
	-DGNOME_DESKTOP_USE_UNSTABLE_API

CLEANFILES = $(GNOMECC_CAPPLETS_CLEANFILES)

-include $(top_srcdir)/git.mk
