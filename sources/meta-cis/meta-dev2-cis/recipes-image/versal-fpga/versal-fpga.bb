#
# This file is the fpga pld pdi install loading recipe.
#

SUMMARY = "veral fpga pld pdi stream"
SECTION = "PETALINUX/bsp"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://titan_fh_fpga_xsa_pld.pdi \
		"
S = "${WORKDIR}"

#DEPENDS += " ubootenv zlib"

#do_compile() {
#	     oe_runmake
#}

do_install() {
	     install -d ${D}${base_libdir}/firmware
	     install -m 0755 titan_fh_fpga_xsa_pld.pdi  ${D}${base_libdir}/firmware/titan_fh_fpga_xsa_pld.pdi
}
FILES:${PN} = "${base_libdir}/firmware"
