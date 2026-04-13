#
# compile device tree overlay.
# xi.he@zillnk.com
#

SUMMARY = "device tree overlay"
SECTION = "PETALINUX/bsp"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS += "dtc-native"

SRC_URI = " \
           file://overlay.dts \
	   "

SRC_URI:append:auxs = " file://overlay_aux.dts "
SRC_URI:append:emimom = " file://overlay_emimom.dts "

S="${WORKDIR}"

PL_DTSI ??= "overlay.dts"
PL_DTBO ?= "overlay.dtbo"

PL_DTSI:auxs = "overlay_aux.dts"
PL_DTSI:emimom = "overlay_emimom.dts"

do_compile() {
	bbnote "compile PL_DTSI[${PL_DTSI}] to PL_DTBO[${PL_DTBO}]."
	dtc -I dts -O dtb -@ ${WORKDIR}/${PL_DTSI} -o ${S}/${PL_DTBO}
}


do_install() {

	install -d ${D}/lib/firmware
	install -m 655 ${S}/${PL_DTBO} ${D}/lib/firmware/${PL_DTBO}

}

FILES:${PN} += "/lib/firmware/overlay.dtbo"
