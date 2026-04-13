SUMMARY = "Aux FPGA Setup & dtbo overlay insertion"
SECTION = "PETALINUX/apps"
LICENSE = "ZILLNK"

LIC_FILES_CHKSUM = " \
	"

PR = "r01"

SRC_URI = " \
          file://aux-dtbo.sh \
          "

SRC_URI:append:auxs = " \
                       file://interlink_setup.sh \
                       "
S = "${WORKDIR}"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

inherit update-rc.d

INITSCRIPT_NAME = "aux-dtbo.sh"

INITSCRIPT_PARAMS = "start 41 S ."

do_install() {
  install -d ${D}${INIT_D_DIR}
  install -m 0755 ${S}/${INITSCRIPT_NAME} ${D}${sysconfdir}/init.d/${INITSCRIPT_NAME}
}

do_install:append:auxs() {
  install -m 0755 ${S}/interlink_setup.sh ${D}${sysconfdir}/init.d/interlink_setup.sh
}

FILES:${PN} += "${sysconfdir}/*"
