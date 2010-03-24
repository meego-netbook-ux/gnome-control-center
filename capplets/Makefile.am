NULL =

SUBDIRS = \
	appearance		\
	keyboard		\
	mouse			\
	network			\
	power			\
	$(NULL)

DIST_SUBDIRS = \
	accessibility		\
	appearance		\
	default-applications	\
	keyboard		\
	mouse			\
	network			\
	security		\
	power		\
	windows			\
	display 		\
	about-me


if HAVE_RANDR
SUBDIRS += display
endif

if BUILD_ABOUTME
SUBDIRS += about-me
endif

if BUILD_SECURITY
SUBDIRS += security
endif

-include $(top_srcdir)/git.mk
