#
# This file is the rsmu_ctl recipe.
#

SUMMARY = "Simple rsmu application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
			file://incdefs.sh \
			file://makefile \
			file://pdt.h \
			file://print.c \
			file://print.h \
			file://rsmu_ctl.c \
			file://util.c \
			file://util.h \
			file://version.c \
			file://version.h \
			file://version.sh \
			file://rsmu.h \
			"

S = "${WORKDIR}"

do_install() {
	     install -d ${D}/${bindir}
	     install -m 0755 ${S}/rsmu_ctl ${D}/${bindir}/rsmu-ctl
}

do_install:append:emimom() {
             install -d ${D}/${bindir}
             ln -s rsmu-ctl ${D}/${bindir}/clu-access
}

