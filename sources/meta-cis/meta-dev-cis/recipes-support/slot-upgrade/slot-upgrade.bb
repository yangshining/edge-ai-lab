#
# This file is the slot-upgrade recipe.
#

SUMMARY = "Simple slot-upgrade application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI ="git://172.16.1.21:29418/cis/support/slot-upgrade;protocol=ssh;user=${USER};branch=master;"

SRCREV = "6e04949a79709fccfe4d082c13225757e091db27"

SRC_URI_append = "\
			file://slot-upgrade.sh \
			file://slot-upgrade_fara2.sh \
			file://unpack.sh \
		  "

SRC_URI_append_auxs = "\
			file://0001-add-tool-package-update.patch \
			"

S = "${WORKDIR}/git"
PACKAGE_ARCH = "${MACHINE_ARCH}"
TARGET_CC_ARCH += "${LDFLAGS}"

DEPENDS = "json-c libubox ubus ubootenv zlib"
PROVIDES = "slot-upgrade"
EXTRA_OECMAKE += "-DCMAKE_SKIP_RPATH=TRUE"

inherit cmake pkgconfig

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 ${S}/slot-upgrade ${D}${bindir}
	     install -m 0755 ${WORKDIR}/slot-upgrade.sh ${D}${bindir}
		 install -m 0755 ${WORKDIR}/unpack.sh ${D}${bindir}
		 install -m 0755 ${WORKDIR}/slot-upgrade_fara2.sh ${D}${bindir}
}

FILES_${PN} = " ${bindir}/*"
INSANE_SKIP_${PN} += "file-rdeps"

BBCLASSEXTEND = "native nativesdk"
