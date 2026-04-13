#
# This file is the aux-fpga recipe.
#

SUMMARY = "Simple auxfpgaload application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit deploy nopackages image-wic-utils

SRC_URI = " \
		file://gen_aux_fpga_pack.sh \
		file://pmufw_slave.elf \
		file://slave_fpga.bit \
		file://slave_output_zboot.bif \
		file://version.txt \
		file://zynqmp_fsbl_slave.elf \
		"

SRC_URI_append_apple = " file://zynqmp_fsbl_slave_apple.elf "

S = "${WORKDIR}"

DEPENDS_append = " auxfpgaload-native "

PACKAGES_LIST_append = " aux-fpga "
PACKAGES_LIST[aux-fpga] = "aux_pack_handler:aux_pack_handler gen_aux_fpga_pack.sh:gen_aux_fpga_pack.sh \
			pmufw_slave.elf:pmufw_slave.elf slave_fpga.bit:slave_fpga.bit slave_output_zboot.bif:slave_output_zboot.bif \
			zynqmp_fsbl_slave.elf:zynqmp_fsbl_slave.elf version.txt:version.txt"

do_configure_append () {
    if [ -f ${WORKDIR}/zynqmp_fsbl_slave_*.elf ];then
        cp -f ${WORKDIR}/zynqmp_fsbl_slave_*.elf  ${WORKDIR}/zynqmp_fsbl_slave.elf
    fi

}

do_deploy() {
    install -d ${DEPLOYDIR}
    install -m 0755 ${S}/gen_aux_fpga_pack.sh ${DEPLOYDIR}/gen_aux_fpga_pack.sh
    install -m 0644 ${S}/pmufw_slave.elf ${DEPLOYDIR}/pmufw_slave.elf
    install -m 0644 ${S}/zynqmp_fsbl_slave.elf ${DEPLOYDIR}/zynqmp_fsbl_slave.elf
    install -m 0644 ${S}/slave_fpga.bit ${DEPLOYDIR}/slave_fpga.bit
    install -m 0644 ${S}/slave_output_zboot.bif ${DEPLOYDIR}/slave_output_zboot.bif
    install -m 0644 ${S}/version.txt ${DEPLOYDIR}/version.txt
	install -m 0755 ${RECIPE_SYSROOT_NATIVE}/${bindir}/auxfpgaload ${DEPLOYDIR}/aux_pack_handler
}

addtask do_deploy after do_compile before do_build

