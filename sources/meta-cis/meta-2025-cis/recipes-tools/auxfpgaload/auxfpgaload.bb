#
# This file is the auxfpgaload recipe.
#

SUMMARY = "Simple auxfpgaload application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
		file://package_handle.c \
		file://package_handle.h \
		file://aux_fpga_load.h \
		file://Makefile \
		"

S = "${WORKDIR}"

PROVIDES = "libauxfpgaload"

DEPENDS:append= " libgpiod "
DEPENDS:remove:class-native = "libgpiod"

LDFLAGS:append = " -lgpiod "
LDFLAGS:remove:class-native = "-lgpiod"

CFLAGS:append = " -DDEBUG_LEVEL=1 -DCRC_ENABLE=1 -DGPIO_ENABLE=1"

CFLAGS:remove:class-native = " -DGPIO_ENABLE=1 "
CFLAGS:remove:class-native = " -DDEBUG_LEVEL=1 "
CFLAGS:append:class-native = " -DGPIO_ENABLE=0 -DDEBUG_LEVEL=3"

CFLAGS:append:apple = " -DPROD_APPLE "
CFLAGS:append:grape = " -DPROD_GRAPE "

do_install() {
		install -d ${D}/${bindir}
		install -d ${D}/${libdir}
		install -d ${D}${includedir}

		install -m 0644 ${S}/aux_fpga_load.h ${D}${includedir}
		oe_soinstall ${S}/libauxfpgaload.so.0.0.1 ${D}${libdir}
		install -m 0755 ${S}/package_handle ${D}/${bindir}/auxfpgaload

		ln -s ${bindir}/auxfpgaload ${D}/${bindir}/ls-aux
}

do_install:append:class-native() {
		rm -rf ${D}/${bindir}/ls-aux
}

BBCLASSEXTEND += "native"
FILES:${PN} += "${libdir}/* ${bindir}/* ${includedir}/*"
