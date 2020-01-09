Name: ding-libs
Version: 0.6.1
Release: 32%{?dist}
Summary: "Ding is not GLib" assorted utility libraries
Group: Development/Libraries
License: LGPLv3+
URL: https://pagure.io/SSSD/ding-libs
Source0: https://releases.pagure.org/SSSD/ding-libs/%{name}-%{version}.tar.gz
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

# If a new upstream release changes some, but not all of these
# version numbers, remember to keep the Release tag in order to
# allow clean upgrades!
%global path_utils_version 0.2.1
%global dhash_version 0.5.0
%global collection_version 0.7.0
%global ref_array_version 0.1.5
%global basicobjects_version 0.1.1
%global ini_config_version 1.3.1

### Patches ###
Patch0001: 0001-INI-Silence-ini_augment-match-failures.patch
Patch0002: 0002-INI-Remove-definiton-of-TRACE_LEVEL.patch


### Downstream only ###
Patch999: ding-libs-fake-soname.patch

### Dependencies ###
# ding-libs is a meta-package that will pull in all of its own
# sub-packages
Requires: libpath_utils = %{path_utils_version}-%{release}
Requires: libdhash = %{dhash_version}-%{release}
Requires: libcollection = %{collection_version}-%{release}
Requires: libref_array = %{ref_array_version}-%{release}
Requires: libbasicobjects = %{basicobjects_version}-%{release}
Requires: libini_config = %{ini_config_version}-%{release}

### Build Dependencies ###

BuildRequires: autoconf
BuildRequires: automake
BuildRequires: libtool
BuildRequires: m4
BuildRequires: doxygen
BuildRequires: check-devel

%description
A set of helpful libraries used by projects such as SSSD.

%package devel
Summary: Development packages for ding-libs
Group: Development/Libraries
License: LGPLv3+

# ding-libs is a meta-package that will pull in all of its own
# sub-packages
Requires: libpath_utils-devel = %{path_utils_version}-%{release}
Requires: libdhash-devel = %{dhash_version}-%{release}
Requires: libcollection-devel = %{collection_version}-%{release}
Requires: libref_array-devel = %{ref_array_version}-%{release}
Requires: libbasicobjects-devel = %{basicobjects_version}-%{release}
Requires: libini_config-devel = %{ini_config_version}-%{release}

%description devel
A meta-package that pulls in development libraries for libcollection,
libdhash, libini_config, librefarray and libpath_utils.

##############################################################################
# Path Utils
##############################################################################

%package -n libpath_utils
Summary: Filesystem Path Utilities
Group: Development/Libraries
License: LGPLv3+
Version: %{path_utils_version}

%description -n libpath_utils
Utility functions to manipulate filesystem pathnames

%package -n libpath_utils-devel
Summary: Development files for libpath_utils
Group: Development/Libraries
Requires: libpath_utils = %{path_utils_version}-%{release}
License: LGPLv3+
Version: %{path_utils_version}

%description -n libpath_utils-devel
Utility functions to manipulate filesystem pathnames

%post -n libpath_utils -p /sbin/ldconfig
%postun -n libpath_utils -p /sbin/ldconfig

%files -n libpath_utils
%defattr(-,root,root,-)
%doc COPYING COPYING.LESSER
%{_libdir}/libpath_utils.so.*

%files -n libpath_utils-devel
%defattr(-,root,root,-)
%{_includedir}/path_utils.h
%{_libdir}/libpath_utils.so
%{_libdir}/pkgconfig/path_utils.pc
%doc path_utils/README.path_utils
%doc path_utils/doc/html/


##############################################################################
# dhash
##############################################################################

%package -n libdhash
Group: Development/Libraries
Summary: Dynamic hash table
License: LGPLv3+
Version: %{dhash_version}

%description -n libdhash
A hash table which will dynamically resize to achieve optimal storage & access
time properties

%package -n libdhash-devel
Summary: Development files for libdhash
Group: Development/Libraries
Requires: libdhash = %{dhash_version}-%{release}
License: LGPLv3+
Version: %{dhash_version}

%description -n libdhash-devel
A hash table which will dynamically resize to achieve optimal storage & access
time properties

%post -n libdhash -p /sbin/ldconfig
%postun -n libdhash -p /sbin/ldconfig

%files -n libdhash
%defattr(-,root,root,-)
%doc COPYING COPYING.LESSER
%{_libdir}/libdhash.so.*

%files -n libdhash-devel
%defattr(-,root,root,-)
%{_includedir}/dhash.h
%{_libdir}/libdhash.so
%{_libdir}/pkgconfig/dhash.pc
%doc dhash/README.dhash
%doc dhash/examples/*.c


##############################################################################
# collection
##############################################################################
%package -n libcollection
Summary: Collection data-type for C
Group: Development/Libraries
License: LGPLv3+
Version: %{collection_version}

%description -n libcollection
A data-type to collect data in a hierarchical structure for easy iteration
and serialization

%package -n libcollection-devel
Summary: Development files for libcollection
Group: Development/Libraries
License: LGPLv3+
Requires: libcollection = %{collection_version}-%{release}
Version: %{collection_version}

%description -n libcollection-devel
A data-type to collect data in a hierarchical structure for easy iteration
and serialization

%post -n libcollection -p /sbin/ldconfig
%postun -n libcollection -p /sbin/ldconfig


%files -n libcollection
%defattr(-,root,root,-)
%doc COPYING
%doc COPYING.LESSER
%{_libdir}/libcollection.so.*

%files -n libcollection-devel
%defattr(-,root,root,-)
%{_includedir}/collection.h
%{_includedir}/collection_tools.h
%{_includedir}/collection_queue.h
%{_includedir}/collection_stack.h
%{_libdir}/libcollection.so
%{_libdir}/pkgconfig/collection.pc
%doc collection/doc/html/


##############################################################################
# ref_array
##############################################################################

%package -n libref_array
Summary: A refcounted array for C
Group: Development/Libraries
License: LGPLv3+
Version: %{ref_array_version}

%description -n libref_array
A dynamically-growing, reference-counted array

%package -n libref_array-devel
Summary: Development files for libref_array
Group: Development/Libraries
Requires: libref_array = %{ref_array_version}-%{release}
License: LGPLv3+
Version: %{ref_array_version}

%description -n libref_array-devel
A dynamically-growing, reference-counted array

%post -n libref_array -p /sbin/ldconfig
%postun -n libref_array -p /sbin/ldconfig

%files -n libref_array
%defattr(-,root,root,-)
%doc COPYING
%doc COPYING.LESSER
%{_libdir}/libref_array.so.*

%files -n libref_array-devel
%defattr(-,root,root,-)
%{_includedir}/ref_array.h
%{_libdir}/libref_array.so
%{_libdir}/pkgconfig/ref_array.pc
%doc refarray/README.ref_array
%doc refarray/doc/html/


##############################################################################
# basicobjects
##############################################################################

%package -n libbasicobjects
Summary: Basic object types for C
Group: Development/Libraries
License: GPLv3+
Version: %{basicobjects_version}

%description -n libbasicobjects
Basic object types

%package -n libbasicobjects-devel
Summary: Development files for libbasicobjects
Group: Development/Libraries
License: GPLv3+
Version: %{basicobjects_version}
Requires: libbasicobjects = %{basicobjects_version}-%{release}

%description -n libbasicobjects-devel
Basic object types

%post -n libbasicobjects -p /sbin/ldconfig
%postun -n libbasicobjects -p /sbin/ldconfig

%files -n libbasicobjects
%defattr(-,root,root,-)
%doc COPYING
%doc COPYING.LESSER
%{_libdir}/libbasicobjects.so.*

%files -n libbasicobjects-devel
%defattr(-,root,root,-)
%{_includedir}/simplebuffer.h
%{_libdir}/libbasicobjects.so
%{_libdir}/pkgconfig/basicobjects.pc

##############################################################################
# ini_config
##############################################################################

%package -n libini_config
Summary: INI file parser for C
Group: Development/Libraries
License: LGPLv3+
Version: %{ini_config_version}

%description -n libini_config
Library to process config files in INI format into a libcollection data
structure

%package -n libini_config-devel
Summary: Development files for libini_config
Group: Development/Libraries
License: LGPLv3+
Requires: libini_config = %{ini_config_version}-%{release}
Requires: libcollection-devel = %{collection_version}-%{release}
Requires: libref_array-devel = %{ref_array_version}-%{release}
Requires: libbasicobjects-devel = %{basicobjects_version}-%{release}
Version: %{ini_config_version}

%description -n libini_config-devel
Library to process config files in INI format into a libcollection data
structure

%post -n libini_config -p /sbin/ldconfig
%postun -n libini_config -p /sbin/ldconfig

%files -n libini_config
%defattr(-,root,root,-)
%doc COPYING
%doc COPYING.LESSER
%{_libdir}/libini_config.so.*

%files -n libini_config-devel
%defattr(-,root,root,-)
%{_includedir}/ini_config.h
%{_includedir}/ini_comment.h
%{_includedir}/ini_configmod.h
%{_includedir}/ini_configobj.h
%{_includedir}/ini_valueobj.h
%{_libdir}/libini_config.so
%{_libdir}/pkgconfig/ini_config.pc
%doc ini/doc/html/


##############################################################################
# Build steps
##############################################################################

%prep
%setup -q

for p in %patches ; do
    %__patch -p1 -i $p
done

%build
autoreconf -ivf
%configure \
    --disable-static

make %{?_smp_mflags} all docs

%check
make %{?_smp_mflags} check

%install
rm -rf $RPM_BUILD_ROOT

make install DESTDIR=$RPM_BUILD_ROOT

# Remove .la files created by libtool
rm -f $RPM_BUILD_ROOT/%{_libdir}/*.la

# Remove the example files from the output directory
# We will copy them directly from the source directory
# for packaging
rm -f \
    $RPM_BUILD_ROOT/usr/share/doc/ding-libs/README.* \
    $RPM_BUILD_ROOT/usr/share/doc/ding-libs/examples/dhash_example.c \
    $RPM_BUILD_ROOT/usr/share/doc/ding-libs/examples/dhash_test.c

# Remove document install script. RPM is handling this
rm -f */doc/html/installdox

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Thu Jun  7 2018 Michal Židek <mzidek@redhat.com> - 0.6.1-32
- Previous change in spec file broke the application of downstream only patch
- Related: rhbz#1507607 - ding-libs: libini_config: `ini_config_augment` reports errors on match failure

* Thu Jun  7 2018 Michal Židek <mzidek@redhat.com> - 0.6.1-31
- Resolves: rhbz#1507607 - ding-libs: libini_config: `ini_config_augment` reports errors on match failure

* Thu Jun 07 2018 Michal Židek  <mzidek@redhat.com> - 0.6.1-30
- Adopting to use Downstream Helper for bellow BZ backport
- Related: rhbz#1507607 - ding-libs: libini_config: `ini_config_augment` reports errors on match failure

* Mon Jun 27 2016 Michal Židek  <mzidek@redhat.com> - 0.6.1-29
- Fix rpmdiff failure for 'Binary stripping'
- Resolves: rhbz#1480270 - Rebase ding-libs to the latest upstream release

* Mon Jun 27 2016 Michal Židek  <mzidek@redhat.com> - 0.6.1-28
- Re-add the soname hack
- Resolves: rhbz#1480270 - Rebase ding-libs to the latest upstream release

* Mon Jun 27 2016 Jakub Hrozek <jhrozek@redhat.com> - 0.5.0.1-27
- Re-add the soname hack
- Resolves: rhbz#1290382 - Rebase ding-libs to the latest available upstream

* Wed Jun 22 2016 Jakub Hrozek <jhrozek@redhat.com> - 0.5.0.1-26
- Resolves: rhbz#1290382 - Rebase ding-libs to the latest available upstream

* Tue Jun 23 2015 Jakub Hrozek <jhrozek@redhat.com> - 0.5.0.1-25
- Resolves: rhbz#1205557 - Include the latest upstream ding-libs release in 7.2
- Port the SONAME hack

* Thu Sep 11 2014 Jakub Hrozek <jhrozek@redhat.com> - 0.4.0.1-24
- Fix ABI breakage that affected gssproxy
- Related: rhbz#1109856 - Rebase ding-libs to 0.4.0

* Thu Jul 24 2014 Jakub Hrozek <jhrozek@redhat.com> - 0.4.0.1-23
- Resolves: rhbz#1109856 - Rebase ding-libs to 0.4.0

* Fri Jan 24 2014 Daniel Mach <dmach@redhat.com> - 0.3.0.1-22
- Mass rebuild 2014-01-24

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com> - 0.3.0.1-21
- Mass rebuild 2013-12-27

* Fri Sep 27 2013 Jakub Hrozek <jhrozek@redhat.com> - 0.3.0.1-20
- Apply a patch by Dmitri Pal to strip trailing whitespace
- Resolves: rhbz#1012434 - libiniconfig doesn't trim trailing whitespace

* Tue Jul 16 2013 Ondrej Kos <okos@redhat.com> - 0.3.0.1-18
- Apply patch to stop generating doxygen timestamps
- Apply patch to include correct libini_config file

* Thu May 23 2013 Jakub Hrozek <jhrozek@redhat.com> - 0.3.0.1-17
- Apply patch by Ondrej Kos to bump libtool version info

* Fri Apr 05 2013 Jakub Hrozek <jhrozek@redhat.com> - 0.3.0.1-16
- Fix libiniconfig_devel Requires

* Thu Apr 04 2013 Jakub Hrozek <jhrozek@redhat.com> - 0.3.0.1-15
- Increase the release number to -15 to allow clean upgrade path from
  0.2 since some of the components kept their version the same in 0.3

* Mon Apr 01 2013 Stephen Gallagher <sgallagh@redhat.com> - 0.3.0.1-3
- Bumping revision to fix build glitch

* Fri Mar 29 2013 Jakub Hrozek <jhrozek@redhat.com> - 0.3.0.1-1
- New upstream release 0.3.0.1
- https://fedorahosted.org/sssd/wiki/Releases/DingNotes-0.3.0.1
- obsoletes patch0001

* Thu Mar 28 2013 Jakub Hrozek <jhrozek@redhat.com> - 0.3.0-2
- Remove cast to allow INI to work on 32bits

* Thu Mar 28 2013 Jakub Hrozek <jhrozek@redhat.com> - 0.3.0-1
- New upstream release 0.3.0
- https://fedorahosted.org/sssd/wiki/Releases/DingNotes-0.3.0

* Mon Mar 25 2013 Jakub Hrozek <jhrozek@redhat.com> - 0.2.91-14
- include a patch to get rid of autoreconf warnings
- run autoreconf before configure
- Resolves: #925258

* Wed Feb 13 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.2.91-13
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Wed Oct 31 2012 Ondrej Kos <okos@redhat.com> - 0.2.91-12
- Fixes missing devel dependency

* Wed Jul 18 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.2.91-11
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Mon May 28 2012 Jan Zeleny <jzeleny@redhat.com> - 0.2.91-10
- a bunch of fixes in spec file

* Fri May 25 2012 Jan Zeleny <jzeleny@redhat.com> - 0.2.91-9
- Bumped the release number to 9 for smooth upgrade

* Fri May 25 2012 Jan Zeleny <jzeleny@redhat.com> - 0.2.91-1
- Rebase to 0.3.0beta1, changelog available at
  https://fedorahosted.org/sssd/wiki/Releases/DingNotes-0.2.91

* Tue Mar 06 2012 Stephen Gallagher <sgallagh@redhat.com> - 0.1.3-8
- Make path_concat return empty string on ENOBUFS

* Tue Mar 06 2012 Stephen Gallagher <sgallagh@redhat.com> - 0.1.3-7
- Fix off-by-one bug in path_concat()
- Resolves: rhbz#799347 - path_utils:test_path_concat_neg fails on 64-bit big
                          endians

* Fri Jan 13 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.1.3-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Thu Sep 15 2011 Stephen Gallagher <sgallagh@redhat.com> - 0.1.3-5
- New upstream release 0.1.3
- Fixes a serious issue with libdhash and large initial hash sizes

* Tue Feb 08 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.1.2-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Fri Oct 15 2010 Stephen Gallagher <sgallagh@redhat.com> - 0.1.2-3
- New upsteam release 0.1.2
- Fixes a serious issue with libdhash where hash_enter() would never update
- existing entries for a key.

* Fri Sep 24 2010 Stephen Gallagher <sgallagh@redhat.com> - 0.1.1-2
- Fix invalid source URL

* Thu Sep 23 2010 Stephen Gallagher <sgallagh@redhat.com> - 0.1.1-1
- Initial release of ding-libs
