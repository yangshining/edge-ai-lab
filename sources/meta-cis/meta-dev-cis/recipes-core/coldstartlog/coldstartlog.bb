SUMMARY = "HW WD & Cold restart record in rlog "
SECTION = "PETALINUX/apps"
LICENSE = "ZILLNK"

LIC_FILES_CHKSUM = " \
	"

PR = "r01"

SRC_URI = " file://pmu_power_record \
		  "

S = "${WORKDIR}"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

inherit update-rc.d

INITSCRIPT_NAME = "pmu_power_record"

#INITSCRIPT_PARAMS = "start 99 S ."
INITSCRIPT_PARAMS = "start 81 5 ."

do_install() {
  install -d ${D}${INIT_D_DIR}
  install -m 0755 ${S}/${INITSCRIPT_NAME} ${D}${sysconfdir}/init.d/${INITSCRIPT_NAME}
}
FILES_${PN} += "${sysconfdir}/*"
