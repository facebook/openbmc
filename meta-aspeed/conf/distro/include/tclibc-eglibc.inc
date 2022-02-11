#
# eglibc specific configuration
#

LIBCEXTENSION = "${@['', '-gnu'][(d.getVar('ABIEXTENSION', True) or '') != '']}"

# Add glibc overrides to the overrides for eglibc.
LIBCOVERRIDE = ":libc-glibc"
OVERRIDES .= "${LIBCOVERRIDE}"

PREFERRED_PROVIDER_virtual/libiconv ?= "eglibc"
PREFERRED_PROVIDER_virtual/nativesdk-libiconv ?= "nativesdk-eglibc"
PREFERRED_PROVIDER_virtual/nativesdk-libintl ?= "nativesdk-eglibc"
PREFERRED_PROVIDER_virtual/libintl ?= "eglibc"
PREFERRED_PROVIDER_virtual/libc ?= "eglibc"
PREFERRED_PROVIDER_virtual/nativesdk-libc ?= "nativesdk-eglibc"
PREFERRED_PROVIDER_virtual/libc-locale ?= "eglibc-locale"

CXXFLAGS += "-fvisibility-inlines-hidden"

LIBC_DEPENDENCIES = "libsegfault \
		     eglibc \
		     eglibc-dbg \
		     eglibc-dev \
		     eglibc-utils \
		     eglibc-thread-db \
		     ${@get_libc_locales_dependencies(d)}"

LIBC_LOCALE_DEPENDENCIES = "\
	eglibc-localedata-i18n \
	eglibc-gconv-ibm850 \
	eglibc-gconv-cp1252 \
	eglibc-gconv-iso8859-1 \
	eglibc-gconv-iso8859-15"

def get_libc_locales_dependencies(d):
    if 'libc-locales' in (d.getVar('DISTRO_FEATURES', True) or '').split() :
        return d.getVar('LIBC_LOCALE_DEPENDENCIES', True) or ''
    else:
        return ''
