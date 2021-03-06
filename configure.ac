AC_INIT([gnome-control-center], [2.30.1],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=gnome-control-center])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([shell])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.9 no-dist-gzip dist-bzip2 tar-ustar])
AM_MAINTAINER_MODE
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AC_STDC_HEADERS
AM_PROG_LIBTOOL

# Use the GNOME documentation framework
GNOME_DOC_INIT

changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

# Internationalization support

IT_PROG_INTLTOOL([0.37.1])

GETTEXT_PACKAGE=gnome-control-center-2.0
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package])

AM_GLIB_GNU_GETTEXT

GNOME_COMMON_INIT
GNOME_DEBUG_CHECK
GNOME_COMPILE_WARNINGS([maximum])
GNOME_MAINTAINER_MODE_DEFINES

AC_PATH_XTRA
x_libs="$X_PRE_LIBS $X_LIBS -lX11 $X_EXTRA_LIBS"

AC_CHECK_FUNC(XScreenSaverQueryInfo, [
  TYPING_BREAK="typing-break"], [
  save_LIBS=$LIBS
  LIBS="$LIBS $X_PRE_LIBS $X_LIBS -lXss -lXext -lX11"
  AC_CHECK_LIB(Xss, XScreenSaverQueryInfo, [
    TYPING_BREAK="typing-break"
    SCREENSAVER_LIBS="$X_PRE_LIBS $X_LIBS -lXss -lXext -lX11"], [
    LIBS=$save_LIBS
    LIBS="$LIBS $X_PRE_LIBS $X_LIBS -lXext -lX11"
    AC_CHECK_LIB(Xext, XScreenSaverQueryInfo, [
      TYPING_BREAK="typing-break"
      SCREENSAVER_LIBS="$X_PRE_LIBS $X_LIBS -lXext -lX11"],[])])
  LIBS=$save_LIBS])
AC_SUBST(TYPING_BREAK)
AC_SUBST(SCREENSAVER_LIBS)

PKG_CHECK_MODULES(LIBCANBERRA_GTK, libcanberra-gtk, [
  CANBERRA_GTK=1
  AC_SUBST(LIBCANBERRA_GTK_CFLAGS)
  AC_SUBST(LIBCANBERRA_GTK_LIBS)
  ], [:])
AM_CONDITIONAL(HAVE_LIBCANBERRA_GTK, test "x$CANBERRA_GTK" = "x1")

AM_CONDITIONAL(HAVE_TYPING_BREAK, test "x$TYPING_BREAK" = xtyping-break)

dnl keyboard-properties-capplet
savecppflags=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $X_CFLAGS"
AC_CHECK_HEADERS([X11/Xlib.h])
AC_CHECK_LIB(Xxf86misc, XF86MiscQueryExtension, [
  AC_CHECK_HEADERS([X11/extensions/xf86misc.h], [XF86MISC_LIBS="-lXxf86misc"],[],
[#if HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif
])])
AC_SUBST(XF86MISC_LIBS)
AC_CHECK_HEADERS(X11/extensions/XKB.h)
CPPFLAGS=$savecppflags

AC_CHECK_LIB(m, floor)

dnl ==============================================
dnl Check that we meet the  dependencies
dnl ==============================================

COMMON_MODULES="gtk+-2.0 >= 2.18.0 dnl
 glib-2.0 >= 2.17.4 dnl
 gthread-2.0 dnl
 gio-2.0 dnl
 gconf-2.0 dnl
 libxml-2.0 dnl
 gnome-desktop-2.0 >= 2.29.4"
PKG_CHECK_MODULES(CAPPLET, $COMMON_MODULES)
PKG_CHECK_MODULES(GNOMECC, $COMMON_MODULES libgnome-menu >= 2.10.1)
PKG_CHECK_MODULES(GNOMECC_SHELL, $COMMON_MODULES libgnome-menu unique-1.0 gio-unix-2.0)
PKG_CHECK_MODULES(DBUS, dbus-1 dbus-glib-1)
PKG_CHECK_MODULES(GNOME_DESKTOP, gnome-desktop-2.0)
PKG_CHECK_MODULES(DEFAULT_APPLICATIONS_CAPPLET, libxml-2.0)
PKG_CHECK_MODULES(TYPING, glib-2.0 > 2.11 gconf-2.0 gtk+-2.0)
PKG_CHECK_MODULES(GSD_DBUS, gnome-settings-daemon)
PKG_CHECK_MODULES(GIO, gio-2.0)

gtk_lib_dir=`$PKG_CONFIG --variable libdir gtk+-2.0`
gtk_binary_version=`$PKG_CONFIG --variable gtk_binary_version gtk+-2.0`
GTK_ENGINE_DIR="$gtk_lib_dir/gtk-2.0/$gtk_binary_version/engines"
AC_SUBST(GTK_ENGINE_DIR)

dnl
dnl Check dependencies of libslab
dnl
PKG_CHECK_MODULES(EXTERNAL_LIBSLAB, [libslab], have_libslab=yes, have_libslab=no)

if test $have_libslab = no; then
	PKG_CHECK_MODULES(LIBSLAB, [
				   gio-2.0 gnome-desktop-2.0 librsvg-2.0 libgnome-menu pango gconf-2.0
				   ],
				   have_libslab_deps=yes,
				   have_libslab_deps=no)
	WARN_CFLAGS="-Wall"
else
	have_libslab_deps=no
fi

AC_SUBST(EXTERNAL_LIBSLAB_CFLAGS)
AC_SUBST(EXTERNAL_LIBSLAB_LIBS)
AC_SUBST(LIBSLAB_CFLAGS)
AC_SUBST(LIBSLAB_LIBS)
AC_SUBST(WARN_CFLAGS)

AM_CONDITIONAL(HAVE_LIBSLAB, [test $have_libslab = yes -o $have_libslab_deps = yes])
AM_CONDITIONAL(HAVE_LIBSLAB_DEPS, [test $have_libslab_deps = yes])
AM_CONDITIONAL(LIBSLAB_FOR_INTERNAL_USE, test "yes" = "yes")

dnl ================================================================
dnl Extensions
dnl ================================================================

PKG_CHECK_MODULES(EXTENSION_COMMON, $COMMON_MODULES)

EXTENSION_LIBTOOL_FLAGS="-export_dynamic -avoid-version -module -no-undefined -export-symbols-regex '^g_io_module_(load|unload)'"
AC_SUBST(EXTENSION_LIBTOOL_FLAGS)

EXTENSIONSDIR="${libdir}/control-center-1/extensions"
AC_SUBST(EXTENSIONSDIR)

EXTENSION_CFLAGS="-I\$(top_srcdir)/capplets/common"
AC_SUBST(EXTENSION_CFLAGS)

EXTENSION_LIBS="\$(top_builddir)/capplets/common/libgnome-control-center-extension.la"
AC_SUBST(EXTENSION_LIBS)

dnl
dnl Check for Xft version 2; we build in extra functionality to the font capplet
dnl when we have it.
dnl
xft_modules=
if $PKG_CONFIG --exists xft ; then
  xft_modules="xft"
  AC_DEFINE(HAVE_XFT2,,[Define if Xft functionality is available])
fi

PKG_CHECK_MODULES(FONT_CAPPLET, $COMMON_MODULES $xft_modules)
PKG_CHECK_MODULES(FONT_VIEWER, $COMMON_MODULES $xft_modules)

PKG_CHECK_MODULES(AT_CAPPLET, $COMMON_MODULES)

dnl
dnl Check for XRandR, needed for display capplet
dnl

have_randr=no
AC_CHECK_LIB(Xrandr, XRRUpdateConfiguration,
  [AC_CHECK_HEADER(X11/extensions/Xrandr.h,
     have_randr=yes
     RANDR_LIBS="-lXrandr -lXrender"
     AC_DEFINE(HAVE_RANDR, 1, [Define if Xrandr extension is available]),
	  :, [#include <X11/Xlib.h>])], : ,
       -lXrandr -lXrender $x_libs)
AM_CONDITIONAL(HAVE_RANDR, [test $have_randr = yes])

PKG_CHECK_MODULES(DISPLAY_CAPPLET, $COMMON_MODULES mx-gtk-1.0)

DISPLAY_CAPPLET_LIBS="$DISPLAY_CAPPLET_LIBS $RANDR_LIBS $DBUS_LIBS"

CAPPLET_LIBS="$CAPPLET_LIBS $x_libs"
GNOMECC_LIBS="$GNOMECC_LIBS $x_libs"

dnl
dnl Check for XCursor support.  If it exists, then we compile the
dnl mouse capplet with support for it turned on
dnl
have_xcursor=no
PKG_CHECK_MODULES(XCURSOR, [
	xcursor
	],
	have_xcursor=yes
	AC_DEFINE(HAVE_XCURSOR, 1, [Define if the Xcursor extension is available]),
	have_xcursor=no)
AM_CONDITIONAL(HAVE_XCURSOR, [test $have_xcursor=yes])
AC_SUBST(XCURSOR_CFLAGS)
AC_SUBST(XCURSOR_LIBS)

if test x$have_xcursor = xyes; then
  AC_MSG_CHECKING([for XCURSOR cursor location])
  XCURSOR_ICONDIR=$($PKG_CONFIG --variable=icondir xcursor)
  if test "x$XCURSOR_ICONDIR" != x; then
    AC_DEFINE_UNQUOTED(XCURSOR_ICONDIR, "${XCURSOR_ICONDIR}", [Define to the xcursor icon path])
  else
    XCURSOR_ICONDIR=none
  fi
  AC_MSG_RESULT([${XCURSOR_ICONDIR}])
fi

CAPPLET_LIBS="$CAPPLET_LIBS $XCURSOR_LIBS"

dnl =============================================
dnl X Input library >= 1.2 with property support
dnl =============================================
have_xinput=no
PKG_CHECK_MODULES(XINPUT, [xi >= 1.2],
	have_xinput=yes
	AC_DEFINE(HAVE_XINPUT, 1, [Define if the XInput extension is available]),
	have_xinput=no)
AM_CONDITIONAL(HAVE_XINPUT, [test $have_xinput=yes])
AC_SUBST(XINPUT_CFLAGS)
AC_SUBST(XINPUT_LIBS)

CAPPLET_LIBS="$CAPPLET_LIBS $XINPUT_LIBS"

dnl ==============
dnl gswitchit
dnl ==============
PKG_CHECK_MODULES(LIBGNOMEKBD, [libgnomekbd >= 2.27.4 libxklavier >= 4.0])
AC_SUBST(LIBGNOMEKBD_CFLAGS)
AC_SUBST(LIBGNOMEKBD_LIBS)

PKG_CHECK_MODULES(LIBGNOMEKBDUI, [libgnomekbdui >= 2.21])
AC_SUBST(LIBGNOMEKBDUI_CFLAGS)
AC_SUBST(LIBGNOMEKBDUI_LIBS)

dnl ==============================================
dnl About-me
dnl ==============================================

AC_MSG_CHECKING([whether to enable About Me])
AC_ARG_ENABLE([aboutme],
   AC_HELP_STRING([--enable-aboutme],
                  [enable about-me capplet]),,
   [enable_aboutme=no])
AC_MSG_RESULT([$enable_aboutme])

if test "x$enable_aboutme" = "xyes"; then
  PKG_CHECK_MODULES(LIBEBOOK, [libebook-1.2 >= 1.7.90],
                    [AC_DEFINE([HAVE_LIBEBOOK], 1,
                    [Define if evolution-data-server libebook-1.2 is available])])
fi

AM_CONDITIONAL(BUILD_ABOUTME, test "x$enable_aboutme" = "xyes")

dnl ==============================================
dnl Security
dnl ==============================================

AC_MSG_CHECKING([whether to enable Security])
AC_ARG_ENABLE([security],
   AC_HELP_STRING([--enable-security],
                  [enable security capplet]),,
   [enable_security=no])
AC_MSG_RESULT([$enable_security])

if test "x$enable_security" = "xyes"; then
  PKG_CHECK_MODULES(MX_GTK, [mx-gtk-1.0])
fi
AC_SUBST(MX_GTK_CFLAGS)
AC_SUBST(MX_GTK_LIBS)

AM_CONDITIONAL(BUILD_SECURITY, test "x$enable_security" = "xyes")

dnl ==============================================
dnl Check the window manager we need to work with
dnl ==============================================

AC_ARG_WITH([window_manager],
            [AS_HELP_STRING([--with-window-manager],
                            [specify which window manager to support (metacity, mutter)])],
            [with_window_manager=${withval}],
            [with_window_manager=metacity])

AS_IF([test "x$with_window_manager" == "xmetacity"],
      [WM_REQUIRES="libmetacity-private >= 2.23.1"
       AC_DEFINE(HAVE_METACITY, 1, [defined if using metacity])],
      [test "x$with_window_manager" == "xmutter"],
      [WM_REQUIRES="libmutter-private >= 2.23.1"
       AC_DEFINE(HAVE_MUTTER, 1, [defined if using mutter])],
      [AC_MSG_FAILURE([Unsuported window manager: "$with_window_manager"])])

PKG_CHECK_MODULES(WINDOW_MANAGER, $WM_REQUIRES)

CAPPLET_LIBS="$CAPPLET_LIBS $WINDOW_MANAGER_LIBS"

dnl End: Check that we meet the  dependencies
dnl ==============================================

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal, no)

if test x"$GLIB_GENMARSHAL" = xno; then
  AC_MSG_ERROR([glib-genmarshal executable not found in your path - should be installed with glib])
fi

AC_SUBST(GLIB_GENMARSHAL)


dnl ==============================================
dnl Moblin specific tweaks
dnl ==============================================
AC_ARG_ENABLE(moblin, AC_HELP_STRING([--enable-moblin],[Build Moblin version]))
if test x"$enable_moblin" = xyes; then
  AC_DEFINE(HAVE_MOBLIN, 1, [Define if we are building for Moblin])
fi

AM_CONDITIONAL(HAVE_MOBLIN, test "x$enable_moblin" = "xyes")

dnl ==============================================
dnl Special GConf section
dnl ==============================================

AC_PATH_PROG(GCONFTOOL, gconftool-2, no)

if test x"$GCONFTOOL" = xno; then
  AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
fi

AM_GCONF_SOURCE_2

dnl ==============================================
dnl Define the main variables
dnl ==============================================
COMMON_CFLAGS="-I\$(top_srcdir)/capplets/common"
COMMON_LIBS="\$(top_builddir)/capplets/common/libgnome-control-center-extension.la"

EXTRA_CFLAGS="-I\$(top_srcdir)/ -DG_LOG_DOMAIN=\"\\\"\$(cappletname)-properties\\\"\""

GNOMECC_CAPPLETS_CFLAGS="${COMMON_CFLAGS} ${CAPPLET_CFLAGS} ${EXTRA_CFLAGS} ${DBUS_CFLAGS}"
GNOMECC_CAPPLETS_LIBS="${COMMON_LIBS} ${CAPPLET_LIBS} ${DBUS_LIBS}"

GNOMECC_CAPPLETS_CLEANFILES="\$(desktop) \$(desktop).in"
GNOMECC_CAPPLETS_EXTRA_DIST="ChangeLog \$(desktop).in.in \$(cappletname)-capplet.png \$(pixmaps_DATA)"

AC_SUBST(GNOMECC_CAPPLETS_EXTRA_DIST)
AC_SUBST(GNOMECC_CAPPLETS_CLEANFILES)
AC_SUBST(GNOMECC_CAPPLETS_CFLAGS)
AC_SUBST(GNOMECC_CAPPLETS_LIBS)

dnl =======================================
dnl Update Mime Database
dnl =======================================

AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database, no)

AC_ARG_ENABLE(update-mimedb,
   AC_HELP_STRING([--disable-update-mimedb],
                   [do not update mime database after installation]),,
                   enable_update_mimedb=yes)
AM_CONDITIONAL(ENABLE_UPDATE_MIMEDB, test x$enable_update_mimedb = xyes)


AC_CONFIG_FILES([
Makefile
capplets/Makefile
capplets/about-me/Makefile
capplets/about-me/gnome-about-me.desktop.in
capplets/about-me/icons/Makefile
capplets/accessibility/Makefile
capplets/accessibility/at-properties/at-properties.desktop.in
capplets/accessibility/at-properties/Makefile
capplets/appearance/Makefile
capplets/appearance/data/Makefile
capplets/appearance/data/gnome-appearance-properties.desktop.in
capplets/appearance/data/gnome-theme-installer.desktop.in
capplets/common/Makefile
capplets/common/libgnome-control-center-extension.pc
capplets/common/libgnome-control-center-extension-uninstalled.pc
capplets/default-applications/Makefile
capplets/default-applications/default-applications.desktop.in
capplets/default-applications/gnome-at-commandline.in
capplets/default-applications/gnome-at-session.desktop.in
capplets/default-applications/gnome-default-applications.pc
capplets/display/Makefile
capplets/display/display-properties.desktop.in
capplets/email/Makefile
capplets/keybindings/Makefile
capplets/keybindings/gnome-keybindings.pc
capplets/keybindings/keybinding.desktop.in
capplets/keyboard/Makefile
capplets/keyboard/keyboard.desktop.in
capplets/keyboard/gnome-keybindings.pc
capplets/mouse/Makefile
capplets/mouse/gnome-settings-mouse.desktop.in
capplets/network/Makefile
capplets/network/gnome-network-properties.desktop.in
capplets/network-setup/Makefile
capplets/power/Makefile
capplets/power/power-properties.desktop.in
capplets/printing/Makefile
capplets/windows/Makefile
capplets/windows/window-properties.desktop.in
capplets/security/Makefile
capplets/security/security.desktop.in
capplets/date/Makefile
capplets/language/Makefile
font-viewer/Makefile
font-viewer/gnome-font-viewer.desktop.in
help/Makefile
libwindow-settings/Makefile
libwindow-settings/gnome-window-settings-2.0.pc
po/Makefile.in
libslab/Makefile
shell/Makefile
shell/gnomecc.desktop.in
typing-break/Makefile
])

dnl due to a bug in intltool we need to expand something from the root last control-center.spec
AC_OUTPUT
