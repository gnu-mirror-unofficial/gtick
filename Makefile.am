SUBDIRS = doc intl po src testsuite

EXTRA_DIST = config.rpath \
	     autogen.sh \
	     gtick.appdata.xml \
	     gtick.desktop \
	     gtick.spec \
	     ABOUT-NLS

appdatadir = $(datadir)/appdata
appdata_DATA = gtick.appdata.xml

desktopdir = $(datadir)/applications
desktop_DATA = gtick.desktop

#AUTOMAKE_OPTIONS = dist-bzip2
AUTOMAKE_OPTIONS = subdir-objects
