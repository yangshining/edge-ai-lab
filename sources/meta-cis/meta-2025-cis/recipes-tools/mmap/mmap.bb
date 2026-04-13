#
# This file is the direct memory access recipe.
#

SUMMARY = "Simple memeory access application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://mmap.c \
		file://Makefile \
		"
S = "${WORKDIR}"

#DEPENDS += " ubootenv zlib"

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 mmap ${D}${bindir}
}

do_install:append:emimom() {
             install -d ${D}/${bindir}
             ln -s mmap ${D}${bindir}/dirmem-access
}

