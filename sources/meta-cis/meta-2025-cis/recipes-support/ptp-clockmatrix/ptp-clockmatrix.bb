#
#compile ptp clockmatrix Linux module.
#hexi <xi.he@zillnk.com>
#
SUMMARY = "Recipe for  build an external ptp clockmatrix Linux kernel module"
SECTION = "PETALINUX/modules"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"

inherit module

INHIBIT_PACKAGE_STRIP = "1"

SRC_URI = "file://Makefile \
           file://ptp_clockmatrix.c \
           file://ptp_clockmatrix.h \
           file://ptp_private.h \
           file://COPYING \
           file://ptp-clockmatrix.sh \
           file://0001-optimize-dpll-mode-setting-strategy.patch \
          "

S = "${WORKDIR}"

inherit update-rc.d

INITSCRIPT_NAME = "ptp-clockmatrix.sh"

INITSCRIPT_PARAMS = "start 40 S ."
INITSCRIPT_PARAMS:emimom = "start 98 S ."

do_install() {
	install -d ${D}${nonarch_base_libdir}/modules/extra
	install -m 655 ${S}/ptp_clockmatrix.ko ${D}${nonarch_base_libdir}/modules/extra
	install -d ${D}${INIT_D_DIR}
	install -m 0755 ${S}/${INITSCRIPT_NAME} ${D}${sysconfdir}/init.d/${INITSCRIPT_NAME}
}

# The inherit of module.bbclass will automatically name module packages with
# "kernel-module-" prefix as required by the oe-core build environment.
FILES:${PN} += "${sysconfdir}/*"
