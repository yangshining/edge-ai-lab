#
# This file is the spidev recipe.
#

SUMMARY = "Simple spidev application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://spidev_fdx.c \
	file://spidev_test.c \
	file://Makefile \
	"

S = "${WORKDIR}"

DEPENDS = " libgpiod "

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}/${bindir}
	     install -m 0755 ${S}/spidev_fdx ${D}/${bindir}
	     install -m 0755 ${S}/spidev_test ${D}/${bindir}
}

FILES_${PN} = "${bindir}/*"