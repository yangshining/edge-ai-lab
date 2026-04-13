SUMMARY = "Versal FPGA loading & dtbo overlay insertion"
SECTION = "PETALINUX/apps"
LICENSE = "ZILLNK"

LIC_FILES_CHKSUM = " \
	"

PR = "r01"

SRC_URI = " file://install-versal-pld.sh \
		  "

S = "${WORKDIR}"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

inherit update-rc.d

INITSCRIPT_NAME = "install-versal-pld.sh"

INITSCRIPT_PARAMS = "start 30 S ."

do_install() {
  install -d ${D}${INIT_D_DIR}
  install -m 0755 ${S}/${INITSCRIPT_NAME} ${D}${sysconfdir}/init.d/${INITSCRIPT_NAME}
}
FILES:${PN} += "${sysconfdir}/*"
