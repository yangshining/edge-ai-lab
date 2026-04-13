FILESEXTRAPATHS:prepend := "${THISDIR}/files:"


SRC_URI = "git://172.16.1.21:29418/3pp/bsp2/u-boot-xlnx;protocol=ssh;user=${USER};branch=${UBRANCH}; \
           "
S = "${WORKDIR}/git"

UBOOT_IMAGES:append = " bootenv.bin:bootenv.bin "

DEPENDS += "dtc-native u-boot-mkimage-native u-boot-mkenvimage-native "
SRC_URI:append:zulu2 = " \ 
		file://platform-top.h \
		file://0001-zulu-support-zboot-cmd-2023.1.patch \
		file://0002-zulu-support-ipconfig-cmd-2023.1.patch \
		file://0003-zulu-support-driver-for-mv88e6185-2023.1.patch \
		file://zboot_info.h \
		file://zu19_boot_config.cfg \
		file://zu19_boot_base_defconfig \
		"

do_configure:prepend:zulu2 () {
	bbwarn "add config to ${WORKDIR}/git/configs"
	cp -f ${WORKDIR}/zboot_info.h ${S}/include/
	cp -f ${WORKDIR}/zu19_boot_base_defconfig ${WORKDIR}/git/configs
}

do_configure:append:zulu2 () {
	if [ "${U_BOOT_AUTO_CONFIG}" = "1" ]; then
		install ${WORKDIR}/platform-auto.h ${S}/include/configs/
		install ${WORKDIR}/platform-top.h ${S}/include/configs/
	fi
}

###for fara
#SRCREV_fara = "63b6d260dbe64a005407439e2caeb32da9025954"

fara_uboot_patch_list = " \
						file://0001-fara-adapt-bootenv-for-zillnk-2025.1.patch \
						file://0002-fara-add-zboot-cmd-2025.1.patch \
						file://0003-fara-add-centec-phy-driver-suport-2025.1.patch \
						file://0004-fara-sf-erase-add-ctl-wd-2025.1.patch \
						file://0005-fara-u-boot-xlnx-add-clu-reset-2025.1.patch \
						file://0006-fara-add-loads-rsa-bitstream-sup-for-zboot-2025.1.patch \
						file://0007-fara-add-mars-loopback-assert-support-2025.1.patch \
						file://0008-fara-add-motorcomm-phy-YT8531S-sup-2025.1.patch \
						file://0009-fara-fix-leds-status-of-mars-phy-2025.1.patch \
						file://0010-fara-remove-auto-update-zboot-2025.1.patch \
						file://0011-fara-support-multi-fpga-bit-loading-2025.1.patch \
						file://0012-fara-add-zu67-ver-sel-support-for-zboot-2025.1.patch \
						file://0013-fara-sup-zboot-info-file-2025.1.patch \
						file://0014-fara-add-zboot-env-2025.1.patch \
						file://0015-fara-zboot-zu67-se-use-unencrypted-bit-2025.1.patch \
						file://0016-versal-by-pass-fpga-loading-in-uboot-2025.1.patch \
						file://0017-versal-set-specific-boot-env-2025.1.patch \
						file://0018-versal-support-reset-reason-2025.1.patch \
						file://0019-add-loopback-for-yt8521s-2025.1.patch \
						file://0020-u-boot-add-switch-mv88e6185-sup-2025.1.patch \
						file://0021-sup-lookback-mode-exit-2025.1.patch \
						file://platform-top.h \
						file://zboot_info.h \
						file://CBRS_uboot_config.cfg \
						"

SRC_URI:append:fara = "${fara_uboot_patch_list}"
SRC_URI:append:fara2 = "${fara_uboot_patch_list}"

SRC_URI:append:mega = "${fara_uboot_patch_list}"
#SRC_URI:remove:mega = " file://CBRS_uboot_config.cfg"
#SRC_URI:append:mega = " file://Mega_uboot_config.cfg"

SRC_URI:append:saku2 = " \
						file://0100-fara-saku2-modify-boot-env-mem-layout-2025.1.patch \
					  "

SRC_URI:append:fmav = " \
						${fara_uboot_patch_list} \
						file://fmav_uboot_config.cfg \
						file://0028-modify-zynqmp-log.patch \
					  "

SRC_URI:append:zu67se = " \
						 file://zu67se_uboot_config.cfg \
						"

SRC_URI:append:emimom = " \
						file://0100-emimo-adpat-yt-phy-hw-config-2025.1.patch \
						file://0101-emimo-sup-rst-reason-2025.1.patch \
						file://0102-sup-switch-yt-phy-2025.1.patch \
					  "

SRC_URI:remove:versal = "file://CBRS_uboot_config.cfg"
SRC_URI:append:versal = "file://versal_uboot_config.cfg"


do_configure:append:fara () {
	install ${WORKDIR}/platform-top.h ${S}/include/configs/
	cp -f ${WORKDIR}/zboot_info.h ${S}/include/
}

do_configure:append:mega () {
	install ${WORKDIR}/platform-top.h ${S}/include/configs/
	cp -f ${WORKDIR}/zboot_info.h ${S}/include/
}

do_configure:append:fara2 () {
	install ${WORKDIR}/platform-top.h ${S}/include/configs/
	cp -f ${WORKDIR}/zboot_info.h ${S}/include/
}

do_configure:append:microblaze () {
	if [ "${U_BOOT_AUTO_CONFIG}" = "1" ]; then
		install ${WORKDIR}/platform-auto.h ${S}/include/configs/
		install -d ${B}/source/board/xilinx/microblaze-generic/
		install ${WORKDIR}/config.mk ${B}/source/board/xilinx/microblaze-generic/
	fi
}

do_deploy:append () {
	mkenvimage -o ${DEPLOYDIR}/bootenv.bin ${DEPLOYDIR}/${UBOOT_INITIAL_ENV} -s 0x20000
}
