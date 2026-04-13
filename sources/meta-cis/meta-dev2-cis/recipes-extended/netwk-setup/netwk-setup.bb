SUMMARY = "Network configuration, rename eth adapters, set debug ip, set kernel network config"
SECTION = "PETALINUX/apps"
LICENSE = "ZILLNK"

LIC_FILES_CHKSUM = " \
	"

PR = "r01"

SRC_URI = " file://netwk-setup.sh \
		  "

S = "${WORKDIR}"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

inherit update-rc.d

INITSCRIPT_NAME = "netwk-setup.sh"

INITSCRIPT_PARAMS = "start 42 S ."

do_install() {
  install -d ${D}${INIT_D_DIR}
  install -m 0755 ${S}/${INITSCRIPT_NAME} ${D}${sysconfdir}/init.d/${INITSCRIPT_NAME}
}
FILES:${PN} += "${sysconfdir}/*"
