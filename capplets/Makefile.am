SUBDIRS = \
	common			\
	appearance		\
	display			\
	email			\
	keyboard		\
	mouse			\
	network			\
	network-setup		\
	power			\
	printing		\
	date			\
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
	network-setup		\
	power			\
	printing		\
	security		\
	windows			\
	display 		\
	email	 		\
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
