#!/bin/sh

#gettextize --force --copy --intl --no-changelog
autopoint --force
aclocal -I m4
autoheader
automake --add-missing --force-missing --gnu --copy
autoconf
