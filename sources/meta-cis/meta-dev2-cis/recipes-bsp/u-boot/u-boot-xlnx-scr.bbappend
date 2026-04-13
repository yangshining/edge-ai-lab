FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append:zulu2 = " file://boot.script \
                       "
###for fara
SRC_URI:append:fara = " \
             		file://boot_fara.script \
             		file://boot_fara2.script \
             		file://boot_fara3.script \
                      "
SRC_URI:append:mega = " file://boot_mega.script \
                       "

FIT_IMAGE_LOAD_ADDRESS = "0x8000000"

SRC_URI:append:fara2 = " file://boot_fara2.script \
			"

SRC_URI:remove:saku2 = " file://boot_fara2.script "
SRC_URI:append:saku2 = " file://boot_saku2.script "

do_compile:append:fara () {
	bbnote "to use fara specific boot script file"
	cp ${WORKDIR}/boot_fara.script  ${WORKDIR}/boot.script
}

do_compile:append:mega () {
	bbnote "to use mega specific boot script file"
	cp ${WORKDIR}/boot_mega.script  ${WORKDIR}/boot.script
}

do_compile:append:fara2 () {
	bbnote "to use fara2 specific boot script file"
	if [ -f ${WORKDIR}/boot_*.script ]; then
		cp ${WORKDIR}/boot_*.script  ${WORKDIR}/boot.script
	fi
}

do_compile:append:soga () {
	bbnote "to use soga specific boot script file"
	cp ${WORKDIR}/boot_fara2.script  ${WORKDIR}/boot.script
}

do_compile:append:gemini () {
	bbnote "to use soga specific boot script file"
	cp ${WORKDIR}/boot_fara2.script  ${WORKDIR}/boot.script
}

do_compile:append:fara3 () {
	bbnote "to use soga specific boot script file"
	cp ${WORKDIR}/boot_fara3.script  ${WORKDIR}/boot.script
}

do_compile:append() {
	bbnote "do compile boot script file"
    sed -e 's/@@KERNEL_IMAGETYPE@@/${KERNEL_IMAGETYPE}/' \
        -e 's/@@KERNEL_LOAD_ADDRESS@@/${KERNEL_LOAD_ADDRESS}/' \
        -e 's/@@DEVICE_TREE_NAME@@/${DEVICE_TREE_NAME}/' \
        -e 's/@@DEVICETREE_ADDRESS@@/${DEVICETREE_ADDRESS}/' \
        -e 's/@@RAMDISK_IMAGE@@/${RAMDISK_IMAGE}/' \
        -e 's/@@RAMDISK_IMAGE_ADDRESS@@/${RAMDISK_IMAGE_ADDRESS}/' \
        -e 's/@@KERNEL_BOOTCMD@@/${KERNEL_BOOTCMD}/' \
        -e 's/@@SDBOOTDEV@@/${SDBOOTDEV}/' \	
        -e 's/@@BITSTREAM@@/${@boot_files_bitstream(d)[0]}/g' \	
        -e 's/@@BITSTREAM_LOAD_ADDRESS@@/${BITSTREAM_LOAD_ADDRESS}/g' \	
        -e 's/@@BITSTREAM_IMAGE@@/${@boot_files_bitstream(d)[0]}/g' \	
        -e 's/@@BITSTREAM_LOAD_TYPE@@/${@get_bitstream_load_type(d)}/g' \	
	-e 's/@@QSPI_KERNEL_OFFSET@@/${QSPI_KERNEL_OFFSET}/' \
	-e 's/@@NAND_KERNEL_OFFSET@@/${NAND_KERNEL_OFFSET}/' \
	-e 's/@@QSPI_KERNEL_SIZE@@/${QSPI_KERNEL_SIZE}/' \
	-e 's/@@NAND_KERNEL_SIZE@@/${NAND_KERNEL_SIZE}/' \
	-e 's/@@QSPI_RAMDISK_OFFSET@@/${QSPI_RAMDISK_OFFSET}/' \
	-e 's/@@NAND_RAMDISK_OFFSET@@/${NAND_RAMDISK_OFFSET}/' \
	-e 's/@@QSPI_RAMDISK_SIZE@@/${QSPI_RAMDISK_SIZE}/' \
	-e 's/@@NAND_RAMDISK_SIZE@@/${NAND_RAMDISK_SIZE}/' \
	-e 's/@@KERNEL_IMAGE@@/${KERNEL_IMAGE}/' \
	-e 's/@@QSPI_KERNEL_IMAGE@@/${QSPI_KERNEL_IMAGE}/' \
	-e 's/@@NAND_KERNEL_IMAGE@@/${NAND_KERNEL_IMAGE}/' \
	-e 's/@@FIT_IMAGE_LOAD_ADDRESS@@/${FIT_IMAGE_LOAD_ADDRESS}/' \
	-e 's/@@QSPI_FIT_IMAGE_OFFSET@@/${QSPI_FIT_IMAGE_OFFSET}/' \
	-e 's/@@QSPI_FIT_IMAGE_SIZE@@/${QSPI_FIT_IMAGE_SIZE}/' \
	-e 's/@@NAND_FIT_IMAGE_OFFSET@@/${NAND_FIT_IMAGE_OFFSET}/' \
	-e 's/@@NAND_FIT_IMAGE_SIZE@@/${NAND_FIT_IMAGE_SIZE}/' \
	-e 's/@@FIT_IMAGE@@/${FIT_IMAGE}/' \
	-e 's/@@PRE_BOOTENV@@/${PRE_BOOTENV}/' \
	-e 's/@@UENV_MMC_LOAD_ADDRESS@@/${UENV_MMC_LOAD_ADDRESS}/' \
	-e 's/@@UENV_TEXTFILE@@/${UENV_TEXTFILE}/' \
	-e 's/@@RAMDISK_IMAGE1@@/${RAMDISK_IMAGE1}/' \
        "${WORKDIR}/boot.script" > "${WORKDIR}/boot.cmd"
    mkimage -A arm -T script -C none -n "Boot script" -d "${WORKDIR}/boot.cmd" boot.scr
    sed -e 's/@@KERNEL_IMAGETYPE@@/${KERNEL_IMAGETYPE}/' \
        -e 's/@@DEVICE_TREE_NAME@@/${DEVICE_TREE_NAME}/' \
	-e 's/@@RAMDISK_IMAGE@@/${PXERAMDISK_IMAGE}/' \
	"${WORKDIR}/pxeboot.pxe" > "pxeboot.pxe"
}
