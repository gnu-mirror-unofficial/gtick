Summary: A metronome application
Name: gtick
Version: 0.3.9
Release: 1
License: GPL
Group: Applications/Sound
URL: http://www.antcom.de/gtick/
Source: http://www.antcom.de/download/gtick/gtick-%{version}.tar.gz
Packager: Kev Green <kyrian@ore.org>
Buildroot: %{_tmppath}/%{name}-%{version}-buildroot

%description
gtick is a small metronome application written for Linux and UN*X supporting 
different meters (2/4, 3/4, 4/4) and speeds ranging from 30 to 250 bpm. It
utilizes GTK+ and OSS (ALSA compatible).

This program has been originally written Alex Roberts, but since his email 
address doesn't seem to be working any more and the package needed help, I
[ Roland Stigge, stigge@antcom.de ] took it over (initially to package it
for Debian, but there were too many "upstream" issues, so I decided to
maintain the whole package).

%prep
%setup -q
%configure

%build
%make

%install
%makeinstall

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc README COPYING TODO ChangeLog 
%doc doc/NOTES
%doc NEWS THANKS INSTALL AUTHORS ABOUT-NLS
%doc %{_mandir}/man1/gtick.1.bz2
%{_bindir}/gtick
%{_datadir}/locale/*

%changelog
* Wed May 31 2006 Lamarque Eric <eric_lamarque@yahoo.fr> 0.3.9-1
- Build on Mandrake 9.2/Mandriva 2006
- Added all locales

* Tue Jun 24 2003 Kev Green <kyrian@ore.org>
- 0.2.1
- Added .de locale.

* Thu Apr 17 2003 Kev Green <kyrian@ore.org>
- Initial version, 0.2.0, built against RedHat 9.

