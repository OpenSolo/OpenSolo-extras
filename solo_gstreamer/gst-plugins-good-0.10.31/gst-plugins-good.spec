%define majorminor  0.10
%define gstreamer   gstreamer

%define gst_minver   0.10.0

Name: 		%{gstreamer}-plugins-good
Version: 	0.10.31
Release: 	1.gst
Summary: 	GStreamer plug-ins with good code and licensing

Group: 		Applications/Multimedia
License: 	LGPL
URL:		http://gstreamer.freedesktop.org/
Vendor:         GStreamer Backpackers Team <package@gstreamer.freedesktop.org>
Source:         http://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Requires: 	  %{gstreamer} >= %{gst_minver}
BuildRequires: 	  %{gstreamer}-devel >= %{gst_minver}

BuildRequires:  gcc-c++

BuildRequires: esound-devel >= 0.2.8
Obsoletes:     gstreamer-esd

Provides:		gstreamer-audiosrc
Provides:		gstreamer-audiosink
BuildRequires: flac-devel >= 1.0.3
BuildRequires: 	GConf2-devel
BuildRequires: libjpeg-devel
BuildRequires: libcaca-devel
BuildRequires: libdv-devel
BuildRequires: libpng-devel >= 1.2.0
BuildRequires: glibc-devel
BuildRequires:	speex-devel
BuildRequires: hal-devel
BuildRequires: libshout-devel >= 2.0
BuildRequires:  aalib-devel >= 1.3
Provides:       gstreamer-aasink = %{version}-%{release}
BuildRequires: pulseaudio-libs-devel
BuildRequires: libavc1394-devel
BuildRequires: libdc1394-devel
BuildRequires: libiec61883-devel  
BuildRequires: libraw1394-devel

%description
GStreamer is a streaming media framework, based on graphs of filters which
operate on media data. Applications using this library can do anything
from real-time sound processing to playing videos, and just about anything
else media-related.  Its plugin-based architecture means that new data
types or processing capabilities can be added simply by installing new
plug-ins.

%prep
%setup -q -n gst-plugins-good-%{version}
%build
%configure \
  --enable-debug \
  --enable-DEBUG 

make %{?_smp_mflags}
                                                                                
%install
rm -rf $RPM_BUILD_ROOT

export GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1
%makeinstall
unset GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL
                                                                                
# Clean out files that should not be part of the rpm.
rm -f $RPM_BUILD_ROOT%{_libdir}/gstreamer-%{majorminor}/*.la
rm -f $RPM_BUILD_ROOT%{_libdir}/gstreamer-%{majorminor}/*.a
rm -f $RPM_BUILD_ROOT%{_libdir}/*.a
rm -f $RPM_BUILD_ROOT%{_libdir}/*.la

%find_lang gst-plugins-good-%{majorminor}

%clean
rm -rf $RPM_BUILD_ROOT

%post
export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
gconftool-2 --makefile-install-rule %{_sysconfdir}/gconf/schemas/gstreamer-%{majorminor}.schemas > /dev/null

%files -f gst-plugins-good-%{majorminor}.lang
%defattr(-, root, root)
%doc AUTHORS COPYING README REQUIREMENTS gst-plugins-good.doap
%{_datadir}/gstreamer-%{majorminor}/presets/GstIirEqualizer10Bands.prs
%{_datadir}/gstreamer-%{majorminor}/presets/GstIirEqualizer3Bands.prs

# non-core plugins without external dependencies
%{_libdir}/gstreamer-%{majorminor}/libgstalaw.so
%{_libdir}/gstreamer-%{majorminor}/libgstalpha.so
%{_libdir}/gstreamer-%{majorminor}/libgstautodetect.so
%{_libdir}/gstreamer-%{majorminor}/libgstavi.so
%{_libdir}/gstreamer-%{majorminor}/libgsteffectv.so
%{_libdir}/gstreamer-%{majorminor}/libgstgoom.so
%{_libdir}/gstreamer-%{majorminor}/libgstlevel.so
%{_libdir}/gstreamer-%{majorminor}/libgstefence.so
%{_libdir}/gstreamer-%{majorminor}/libgstmulaw.so
%{_libdir}/gstreamer-%{majorminor}/libgstisomp4.so
%{_libdir}/gstreamer-%{majorminor}/libgstrtp.so
%{_libdir}/gstreamer-%{majorminor}/libgstrtpmanager.so
%{_libdir}/gstreamer-%{majorminor}/libgstrtsp.so
%{_libdir}/gstreamer-%{majorminor}/libgstsmpte.so
%{_libdir}/gstreamer-%{majorminor}/libgstudp.so
%{_libdir}/gstreamer-%{majorminor}/libgstvideobox.so
%{_libdir}/gstreamer-%{majorminor}/libgstwavenc.so
%{_libdir}/gstreamer-%{majorminor}/libgstwavparse.so
%{_libdir}/gstreamer-%{majorminor}/libgstauparse.so
%{_libdir}/gstreamer-%{majorminor}/libgstdebug.so
%{_libdir}/gstreamer-%{majorminor}/libgstnavigationtest.so
%{_libdir}/gstreamer-%{majorminor}/libgstalphacolor.so
%{_libdir}/gstreamer-%{majorminor}/libgstcairo.so
%{_libdir}/gstreamer-%{majorminor}/libgstflxdec.so
%{_libdir}/gstreamer-%{majorminor}/libgstmatroska.so
%{_libdir}/gstreamer-%{majorminor}/libgstvideomixer.so
%{_libdir}/gstreamer-%{majorminor}/libgstcutter.so
%{_libdir}/gstreamer-%{majorminor}/libgstmultipart.so
%{_libdir}/gstreamer-%{majorminor}/libgstid3demux.so
%{_libdir}/gstreamer-%{majorminor}/libgstgdkpixbuf.so
%{_libdir}/gstreamer-%{majorminor}/libgstapetag.so
%{_libdir}/gstreamer-%{majorminor}/libgstannodex.so
%{_libdir}/gstreamer-%{majorminor}/libgstvideocrop.so
%{_libdir}/gstreamer-%{majorminor}/libgsticydemux.so
%{_libdir}/gstreamer-%{majorminor}/libgsttaglib.so
%{_libdir}/gstreamer-%{majorminor}/libgstximagesrc.so
%{_libdir}/gstreamer-%{majorminor}/libgstaudiofx.so
%{_libdir}/gstreamer-%{majorminor}/libgstequalizer.so
%{_libdir}/gstreamer-%{majorminor}/libgstmultifile.so
%{_libdir}/gstreamer-%{majorminor}/libgstspectrum.so
%{_libdir}/gstreamer-%{majorminor}/libgstgoom2k1.so
%{_libdir}/gstreamer-%{majorminor}/libgstinterleave.so
%{_libdir}/gstreamer-%{majorminor}/libgstreplaygain.so
%{_libdir}/gstreamer-%{majorminor}/libgstdeinterlace.so
%{_libdir}/gstreamer-%{majorminor}/libgstflv.so
%{_libdir}/gstreamer-%{majorminor}/libgsty4menc.so
%{_libdir}/gstreamer-%{majorminor}/libgstoss4audio.so
%{_libdir}/gstreamer-%{majorminor}/libgstimagefreeze.so
%{_libdir}/gstreamer-%{majorminor}/libgstshapewipe.so
%{_libdir}/gstreamer-%{majorminor}/libgstvideofilter.so
%{_libdir}/gstreamer-%{majorminor}/libgstaudioparsers.so

# sys plugins
%{_libdir}/gstreamer-%{majorminor}/libgstvideo4linux2.so

# gstreamer-plugins with external dependencies but in the main package
%{_libdir}/gstreamer-%{majorminor}/libgstcacasink.so
%{_libdir}/gstreamer-%{majorminor}/libgstesd.so
%{_libdir}/gstreamer-%{majorminor}/libgstflac.so
%{_libdir}/gstreamer-%{majorminor}/libgstjack.so
%{_libdir}/gstreamer-%{majorminor}/libgstjpeg.so
%{_libdir}/gstreamer-%{majorminor}/libgstpng.so
%{_libdir}/gstreamer-%{majorminor}/libgstossaudio.so
%{_libdir}/gstreamer-%{majorminor}/libgstspeex.so
%{_libdir}/gstreamer-%{majorminor}/libgstgconfelements.so
%{_libdir}/gstreamer-%{majorminor}/libgsthalelements.so
%{_libdir}/gstreamer-%{majorminor}/libgstshout2.so
%{_libdir}/gstreamer-%{majorminor}/libgstaasink.so
%{_libdir}/gstreamer-%{majorminor}/libgstdv.so
%{_libdir}/gstreamer-%{majorminor}/libgst1394.so
%{_libdir}/gstreamer-%{majorminor}/libgstwavpack.so
%{_libdir}/gstreamer-%{majorminor}/libgstsouphttpsrc.so
%{_libdir}/gstreamer-%{majorminor}/libgstpulse.so

# schema files
%{_sysconfdir}/gconf/schemas/gstreamer-%{majorminor}.schemas

%changelog
* Tue Jun 12 2007 Jan Schmidt <jan at fluendo dot com>
- wavpack and qtdemux have moved from bad

* Fri Sep 02 2005 Thomas Vander Stichele <thomas at apestaart dot org>
- clean up for splitup
