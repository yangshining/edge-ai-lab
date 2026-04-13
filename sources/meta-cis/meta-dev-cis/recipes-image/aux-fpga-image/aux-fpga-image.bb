#
# This file is the aux fpga image install loading recipe.
#

SUMMARY = "aux fpga image"
SECTION = "PETALINUX/bsp"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://aux-fpga.bin \
		"
S = "${WORKDIR}"

do_install() {
	     install -d ${D}${base_libdir}/firmware
	     install -m 0755 aux-fpga.bin ${D}${base_libdir}/firmware/aux-fpga.bin
}
FILES_${PN} = "${base_libdir}/firmware"
