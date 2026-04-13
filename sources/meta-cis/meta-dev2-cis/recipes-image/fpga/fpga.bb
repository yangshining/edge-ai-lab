#
# This file is the fpga ku3p/ku5p install loading recipe.
#

SUMMARY = "fpga bit stream"
SECTION = "PETALINUX/bsp"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://ku5p_feynman_fpga_top.bit \
		"
S = "${WORKDIR}"

#DEPENDS += " ubootenv zlib"

#do_compile() {
#	     oe_runmake
#}

do_install() {
	     install -d ${D}${base_libdir}/firmware
	     install -m 0755 ku5p_feynman_fpga_top.bit ${D}${base_libdir}/firmware/feynman_fpga_top.bit
}
FILES:${PN} = "${base_libdir}/firmware"
