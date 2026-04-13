#
# This file is the RU test recipe.
#
SUMMARY = "dhcp application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

INSANE_SKIP:${PN} = "ldflags"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
DEPENDS:append:fara = "libcap"

SRC_URI += " file://dhclient \
             file://libdhcp.so.0.0.0 \
	         file://libdns.so.1104.0.2 \
             file://libirs.so.161.0.0 \
             file://libisc.so.1100.0.0 \
             file://libisccfg.so.163.0.0 \
             file://libomapi.so.0.0.0 \
             file://libcrypto_1022.so.1.1 \
             file://libgnutls.so.30.24.0 \
             file://libhogweed.so.5.0 \
             file://libnettle.so.7.0 \
             file://libcurl_1022.so.4.6.0 \
"

S = "${WORKDIR}"

do_install() {
            install -d ${D}/usr/lib
            install -d ${D}/usr/bin
            install -m 0755 ${S}/dhclient ${D}/usr/bin
            install -m 0644 ${S}/libdhcp.so.0.0.0 ${D}/usr/lib
            install -m 0644 ${S}/libdns.so.1104.0.2 ${D}/usr/lib
            install -m 0644 ${S}/libirs.so.161.0.0 ${D}/usr/lib
            install -m 0644 ${S}/libisc.so.1100.0.0 ${D}/usr/lib
            install -m 0644 ${S}/libisccfg.so.163.0.0 ${D}/usr/lib
            install -m 0644 ${S}/libomapi.so.0.0.0 ${D}/usr/lib
            install -m 0644 ${S}/libgnutls.so.30.24.0 ${D}/usr/lib
            install -m 0644 ${S}/libhogweed.so.5.0 ${D}/usr/lib
            install -m 0644 ${S}/libnettle.so.7.0 ${D}/usr/lib
            install -m 0644 ${S}/libcurl_1022.so.4.6.0 ${D}/usr/lib
            install -m 0644 ${S}/libcrypto_1022.so.1.1 ${D}/usr/lib
}

do_install:append:fara () {
            ln -s -r ${D}/usr/lib/libdhcp.so.0.0.0 ${D}/usr/lib/libdhcp.so.0
            ln -s -r ${D}/usr/lib/libdns.so.1104.0.2 ${D}/usr/lib/libdns.so.1104
            ln -s -r ${D}/usr/lib/libirs.so.161.0.0 ${D}/usr/lib/libirs.so.161
            ln -s -r ${D}/usr/lib/libisc.so.1100.0.0 ${D}/usr/lib/libisc.so.1100
            ln -s -r ${D}/usr/lib/libisccfg.so.163.0.0 ${D}/usr/lib/libisccfg.so.163
            ln -s -r ${D}/usr/lib/libomapi.so.0.0.0 ${D}/usr/lib/libomapi.so.0
            #ln -s -r ${D}/usr/lib/libgnutls.so.30.24.0 ${D}/usr/lib/
            ln -s -r ${D}/usr/lib/libhogweed.so.5.0 ${D}/usr/lib/libhogweed.so.5
            ln -s -r ${D}/usr/lib/libnettle.so.7.0 ${D}/usr/lib/libnettle.so.7
            #ln -s -r ${D}/usr/lib/libcurl_1022.so.4.6.0 ${D}/usr/lib/libcurl.so.4
            #ln -s -r ${D}/usr/lib/libcrypto_1022.so.1.1 ${D}/usr/lib/
}

do_install:append:mega () {
            ln -s -r ${D}/usr/lib/libdhcp.so.0.0.0 ${D}/usr/lib/libdhcp.so.0
            ln -s -r ${D}/usr/lib/libdns.so.1104.0.2 ${D}/usr/lib/libdns.so.1104
            ln -s -r ${D}/usr/lib/libirs.so.161.0.0 ${D}/usr/lib/libirs.so.161
            ln -s -r ${D}/usr/lib/libisc.so.1100.0.0 ${D}/usr/lib/libisc.so.1100
            ln -s -r ${D}/usr/lib/libisccfg.so.163.0.0 ${D}/usr/lib/libisccfg.so.163
            ln -s -r ${D}/usr/lib/libomapi.so.0.0.0 ${D}/usr/lib/libomapi.so.0
            #ln -s -r ${D}/usr/lib/libgnutls.so.30.24.0 ${D}/usr/lib/
            ln -s -r ${D}/usr/lib/libhogweed.so.5.0 ${D}/usr/lib/libhogweed.so.5
            ln -s -r ${D}/usr/lib/libnettle.so.7.0 ${D}/usr/lib/libnettle.so.7
            #ln -s -r ${D}/usr/lib/libcurl_1022.so.4.6.0 ${D}/usr/lib/libcurl.so.4
            #ln -s -r ${D}/usr/lib/libcrypto_1022.so.1.1 ${D}/usr/lib/
}

do_install:append:fara2 () {
            ln -s -r ${D}/usr/lib/libdhcp.so.0.0.0 ${D}/usr/lib/libdhcp.so.0
            ln -s -r ${D}/usr/lib/libdns.so.1104.0.2 ${D}/usr/lib/libdns.so.1104
            ln -s -r ${D}/usr/lib/libirs.so.161.0.0 ${D}/usr/lib/libirs.so.161
            ln -s -r ${D}/usr/lib/libisc.so.1100.0.0 ${D}/usr/lib/libisc.so.1100
            ln -s -r ${D}/usr/lib/libisccfg.so.163.0.0 ${D}/usr/lib/libisccfg.so.163
            ln -s -r ${D}/usr/lib/libomapi.so.0.0.0 ${D}/usr/lib/libomapi.so.0
            #ln -s -r ${D}/usr/lib/libgnutls.so.30.24.0 ${D}/usr/lib/
            ln -s -r ${D}/usr/lib/libhogweed.so.5.0 ${D}/usr/lib/libhogweed.so.5
            ln -s -r ${D}/usr/lib/libnettle.so.7.0 ${D}/usr/lib/libnettle.so.7
            #ln -s -r ${D}/usr/lib/libcurl_1022.so.4.6.0 ${D}/usr/lib/libcurl.so.4
            #ln -s -r ${D}/usr/lib/libcrypto_1022.so.1.1 ${D}/usr/lib/
}

FILES:${PN} += "/usr/bin/*"
FILES:${PN} += "/usr/lib/*"
