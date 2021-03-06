NULL =
INCLUDES =					\
	-I$(top_srcdir)				\
	$(EXTENSION_CFLAGS)			\
	$(EXTENSION_COMMON_CFLAGS)		\
	$(GNOMECC_SHELL_CFLAGS)			\
	$(NULL)

bin_PROGRAMS = gnome-control-center

MARSHAL_FILES = cc-shell-marshal.c cc-shell-marshal.h
BUILT_SOURCES = $(MARSHAL_FILES)

cc-shell-marshal.h: cc-shell-marshal.list
	@GLIB_GENMARSHAL@ --prefix=cc_shell_marshal $< --header > $@

cc-shell-marshal.c: cc-shell-marshal.list
	@GLIB_GENMARSHAL@ --prefix=cc_shell_marshal $< --body --header > $@

gnome_control_center_SOURCES =			\
	gnome-control-center.c			\
	control-center.c			\
	control-center.h			\
	shell-search-renderer.c			\
	shell-search-renderer.h			\
	cc-shell-category-view.c		\
	cc-shell-category-view.h		\
	cc-shell-item-view.c			\
	cc-shell-item-view.h			\
	cc-shell-model.c			\
	cc-shell-model.h			\
	$(MARSHAL_FILES)			\
	$(NULL)

gnome_control_center_LDADD =			\
	$(EXTENSION_LIBS)			\
	$(EXTENSION_COMMON_LIBS)		\
	$(GNOMECC_SHELL_LIBS)			\
	$(NULL)

gnome_control_center_LDFLAGS = -export-dynamic

AM_CPPFLAGS =							\
	-DGNOMELOCALEDIR="\"$(datadir)/locale\""		\
	-DEXTENSIONSDIR="\"$(EXTENSIONSDIR)\""			\
	-DUIDIR="\"$(uidir)\""					\
	-DMENUDIR="\"$(menudir)\""				\
	$(NULL)

menudir = $(sysconfdir)/xdg/menus
menu_DATA = gnomecc.menu

uidir = $(pkgdatadir)/ui
ui_DATA = shell.ui

sysdir = $(datadir)/applications
sys_in_files = gnomecc.desktop.in
sys_DATA = $(sys_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

directorydir = $(datadir)/desktop-directories
directory_in_files = gnomecc.directory.in
directory_DATA = $(directory_in_files:.directory.in=.directory)
@INTLTOOL_DIRECTORY_RULE@

schemasdir   = @GCONF_SCHEMA_FILE_DIR@
schemas_DATA = control-center.schemas
@INTLTOOL_SCHEMAS_RULE@

install-data-local: $(schema_DATA)
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
		for p in $^ ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p 2>&1 > /dev/null; \
		done \
	fi
endif

EXTRA_DIST = 					\
	$(ui_DATA)				\
	gnomecc.directory.in			\
	gnomecc.menu				\
	cc-shell-marshal.list			\
	$(schemas_DATA).in			\
	cc-shell-marshal.list			\
	$(NULL)

DISTCLEANFILES = gnomecc.desktop gnomecc.desktop.in gnomecc.directory $(schemas_DATA)

-include $(top_srcdir)/git.mk
