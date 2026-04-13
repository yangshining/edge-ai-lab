#
# This file is the fw-printenv recipe.
#

SUMMARY = "Simple fw-printenv application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "	file://re-boot.c \
			file://Makefile \
	"

S = "${WORKDIR}"

DEPENDS = " libgpiod ubootenv zlib rlog"

INSANE_SKIP_fw-printenv = "ldflags"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"

CFLAGS_prepend = "-I ${S}/include"
do_compile() {
        oe_runmake
}

do_install() {
		 install -d ${D}${bindir}
		 install -m 0755 ${S}/re_boot ${D}${bindir}
}
