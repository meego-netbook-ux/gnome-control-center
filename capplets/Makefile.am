SUBDIRS = \
	common			\
	accessibility	\
	appearance		\
	default-applications	\
	display			\
	keybindings		\
	keyboard		\
	mouse			\
	network			\
	windows			\
	date
	language

DIST_SUBDIRS = \
	common			\
	accessibility		\
	appearance		\
	default-applications	\
	keybindings		\
	keyboard		\
	mouse			\
	network			\
	security		\
	windows			\
	display 		\
	date			\
	language		\
	about-me


if BUILD_ABOUTME
SUBDIRS += about-me
endif

if BUILD_SECURITY
SUBDIRS += security
endif

-include $(top_srcdir)/git.mk
