
SUMMARY = "Mount Virtual non-volatile RAM as ramdisk for log system"
SECTION = "PETALINUX/apps"
LICENSE = "ZILLNK"
LIC_FILES_CHKSUM = " \
	"
PR = "r01"
SRC_URI = " file://mtvram.sh"
S = "${WORKDIR}"
inherit update-rc.d
INITSCRIPT_PACKAGES = "${PN}"
INITSCRIPT_NAME = "mtvram.sh"
	
INITSCRIPT_PARAMS = "start 38 S ."
do_install() {
  install -d ${D}${INIT_D_DIR}
  install -m 0755 ${S}/${INITSCRIPT_NAME} ${D}${sysconfdir}/init.d/${INITSCRIPT_NAME}
}
FILES_${PN} += "${sysconfdir}/*"

