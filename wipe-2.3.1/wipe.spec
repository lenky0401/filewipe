%define ver	2.3
%define rel	1
%define prefix	/usr
%define docs	LICENSE copyright README CHANGES TODO INSTALL TESTING

Prefix: %{prefix}
BuildRoot: %{_tmppath}/%{name}-buildroot
Docdir: %{prefix}/doc

Summary: secure file wiper
Name: wipe
Version: %ver
Release: %rel
License: GPL
Group: Utilities/File
Source: metalab.unc.edu:/pub/Linux/utils/file/wipe-%{ver}.tar.bz2
URL: http://wipe.sf.net/
Packager: Tom Vier <nester@users.sf.net>

%description
Wipe is a tool that attemps to effectively degauses the surface of
a hard disk, making it virtually impossible to retrieve the data
that was stored on it. This tool is designed to make sure secure
data that is erased from a hard drive is unrecoverable.

%prep
rm -rf $RPM_BUILD_DIR/wipe-%{ver}
bunzip2 < $RPM_SOURCE_DIR/wipe-%{ver}.tar.bz2 | tar xv

%build
cd wipe-%{ver}
./configure --prefix=$RPM_BUILD_ROOT/%{prefix}
make

%install
cd wipe-%{ver}
#make install
cp %{docs} $RPM_BUILD_DIR
install -d $RPM_BUILD_ROOT%{_bindir}
install -s ./wipe $RPM_BUILD_ROOT%{_bindir}
install -d $RPM_BUILD_ROOT%{_mandir}/man1/
install -m 0644 wipe.1 $RPM_BUILD_ROOT%{_mandir}/man1/
rm -rf $RPM_BUILD_ROOT/%{_prefix}/doc/wipe/
install -d $RPM_BUILD_ROOT%{_prefix}/doc/wipe/
for file in %{docs}
do
       install -m 0644 $file $RPM_BUILD_ROOT/%{_prefix}/doc/wipe/
done

%files
%defattr(-,root,root)
%doc %{docs}
/usr/bin/wipe
#/usr/man/man1/wipe.1
%{_mandir}/man1/wipe.1*

%clean
rm -rf $RPM_BUILD_ROOT
rm -rf $RPM_BUILD_DIR/wipe-%{ver}
rm -f %{docs}
