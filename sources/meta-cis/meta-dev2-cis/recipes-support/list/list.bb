#
# This file is the list recipe.
#

SUMMARY = "Simple list application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://list.c \
		file://Makefile \
		file://list.h \
		"
S = "${WORKDIR}"

DEPENDS += "json-c libubox ubus zlib"

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 list ${D}${bindir}
}
