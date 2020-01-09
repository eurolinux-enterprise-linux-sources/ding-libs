Name: ding-libs
Version: 0.4.0
Release: 13%{?dist}
Summary: "Ding is not GLib" assorted utility libraries
Group: Development/Libraries
License: LGPLv3+
URL: http://fedorahosted.org/sssd/
Source0: http://fedorahosted.org/releases/d/i/ding-libs/%{name}-%{version}.tar.gz
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

# If a new upstream release changes some, but not all of these
# version numbers, remember to keep the Release tag in order to
# allow clean upgrades!
%global path_utils_version 0.2.1
%global dhash_version 0.4.3
%global collection_version 0.6.2
%global ref_array_version 0.1.4
%global basicobjects_version 0.1.1
%global ini_config_version 1.1.0

### Patches ###
Patch0001: 0001-ini_parse-Add-missing-TRACE_FLOW_EXIT.patch
Patch0002: 0002-ini-Add-INI_PARSE_IGNORE_NON_KVP-flag.patch
Patch0003: 0003-INI-Allow-longer-values-then-PATH_MAX.patch

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
%doc dhash/examples/dhash_example.c
%doc dhash/examples/dhash_test.c


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
%doc COPYING
%doc COPYING.LESSER
%{_libdir}/libbasicobjects.so.*

%files -n libbasicobjects-devel
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
Requires: libcollection = %{collection_version}-%{release}
Requires: libpath_utils = %{path_utils_version}-%{release}
Requires: libref_array = %{ref_array_version}-%{release}

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
* Thu Feb 08 2018 Michal Zidek <mzidek@redhat.com> - 0.4.0-13
- Resolves: rhbz#1538061 - sssd/libini_config" cannot parse configuration file
                           with line longer than 5102

* Tue Sep 27 2016 Jakub Hrozek <jhrozek@redhat.com> - 0.4.0-12
- Related: rhbz#1377213 - ding-libs don't parse lines without an equal sign

* Thu Jun 03 2014 Jakub Hrozek <jhrozek@redhat.com> - 0.4.0-11
- Do not package built objects in dhash-devel
- Fix NVR in the previous changelog entry
- Related: rhbz#1069287 - Rebase ding-libs to latest version in 6.6

* Thu May 29 2014 Jakub Hrozek <jhrozek@redhat.com> - 0.4.0-10
- Resolves: rhbz#1069287 - Rebase ding-libs to latest version in 6.6

* Thu Mar 29 2012 Stephen Gallagher <sgallagh@redhat.com> - 0.1.2-9
- Resolves: rhbz#801393 - Off-by-one error causing stack smashing in path_utils

* Thu Feb 10 2012 Stephen Gallagher <sgallagh@redhat.com> - 0.1.2-8
- Fix bad patch
- Related:  rhbz#736074 - dhash can corrupt memory if initial table size is
                          large

* Thu Feb 09 2012 Stephen Gallagher <sgallagh@redhat.com> - 0.1.2-7
- Resolves: rhbz#736074 - dhash can corrupt memory if initial table size is
                          large

* Thu Jan 20 2011 Stephen Gallagher <sgallagh@redhat.com> - 0.1.2-6
- Resolves: rhbz#668586 - ding-libs subpackages should have explicit version
-                         requirements
* Thu Jan 13 2011 Stephen Gallagher <sgallagh@redhat.com> - 0.1.2-5
- Fix mislabled licenses (not a re-licensing)
- Related: rhbz#644073 - Add ding-libs to RHEL

* Fri Dec 17 2010 Stephen Gallagher <sgallagh@redhat.com> - 0.1.2-4
- Related: rhbz#644073 - Add ding-libs to RHEL

* Fri Oct 15 2010 Stephen Gallagher <sgallagh@redhat.com> - 0.1.2-3
- New upstream release 0.1.2
- Fixes a serious issue with libdhash where hash_enter() would never update
- existing entries for a key.

* Fri Sep 23 2010 Stephen Gallagher <sgallagh@redhat.com> - 0.1.1-2
- Fix invalid source URL

* Thu Sep 23 2010 Stephen Gallagher <sgallagh@redhat.com> - 0.1.1-1
- Initial release of ding-libs
