SUMMARY = "System device startup, configure date, time, watchdog"
SECTION = "PETALINUX/apps"
LICENSE = "ZILLNK"

LIC_FILES_CHKSUM = " \
	"

PR = "r01"

SRC_URI = " file://sdev-setup.sh \
		  "

S = "${WORKDIR}"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

inherit update-rc.d

INITSCRIPT_NAME = "sdev-setup.sh"

INITSCRIPT_PARAMS = "start 40 S ."
INITSCRIPT_PARAMS:emimom = "start 20 S ."

do_install() {
  install -d ${D}${INIT_D_DIR}
  install -m 0755 ${S}/${INITSCRIPT_NAME} ${D}${sysconfdir}/init.d/${INITSCRIPT_NAME}
}
FILES:${PN} += "${sysconfdir}/*"
