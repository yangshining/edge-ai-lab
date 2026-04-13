#
# xi.he@zillnk.com
#
SUMMARY = "C utility functions"
DESCRIPTION = "C utility functions for OpenWrt"
HOMEPAGE = "https://git.openwrt.org/project/libubox.git"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
SECTION = "PETALINUX/bsp"
DEPENDS += "json-c"

SRC_URI = "\
          git://git.openwrt.org/project/libubox.git;branch=master; \
		  file://0001-not-build-lua-examples-by-default.patch \
          "

SRCREV = "d2223ef9da7172a84d1508733dc58840e1381e3c"
PV = "master+git${SRCPV}"

S = "${WORKDIR}/git"

inherit cmake pkgconfig

PACKAGECONFIG_CONFARGS = "-DABIVERSION=2.0.0"

