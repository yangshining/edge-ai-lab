FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI = "git://172.16.1.21:29418/3pp/bsp2/u-boot-xlnx;protocol=ssh;user=${USER};branch=${UBRANCH}; \
           "

UBOOT_IMAGES_append = " bootenv.bin:bootenv.bin "

DEPENDS += "dtc-native u-boot-mkimage-native u-boot-mkenvimage-native "

###for zulu2
##SRCREV_zulu = "41fc08b3fe2d78b00fa2ad4438a39e9164fde3bb"
SRC_URI_append_zulu2 = " \
						 file://0002-zulu-support-zboot-cmd.patch \
						 file://0003-zulu-support-ipconfig-cmd.patch \
						 file://0004-zulu-support-driver-for-mv88e6185.patch \
						 file://0005-zulu-opt-boot-env-save.patch \
						 file://zboot_info.h \
						 file://platform-top.h \
                         file://zu19_boot_config.cfg \
                         file://zu19_boot_base_defconfig \
                       "
do_configure_prepend_zulu2 () {
    bbwarn "add config to ${WORKDIR}/git/configs"
	cp -f ${WORKDIR}/zboot_info.h ${S}/include/
	cp -f ${WORKDIR}/zu19_boot_base_defconfig ${WORKDIR}/git/configs
}

do_configure_append_zulu2 () {
	if [ "${U_BOOT_AUTO_CONFIG}" = "1" ]; then
		install ${WORKDIR}/platform-auto.h ${S}/include/configs/
		install ${WORKDIR}/platform-top.h ${S}/include/configs/
	fi
}

###for fara
#SRCREV_fara = "63b6d260dbe64a005407439e2caeb32da9025954"

fara_uboot_patch_list = " \
						  file://0001-fara-adapt-bootenv-for-zillnk.patch \
						  file://0002-fara-add-zboot-cmd.patch \
						  file://0003-fara-add-centec-phy-driver-suport.patch \
						  file://0010-FDD-support-3types-GigaDevice-norflash-in-uboot.patch \
						  file://0011-sf_erase_add_control_watchdog.patch \
						  file://0012-zboot-modify-info.patch \
						  file://0013-u-boot-xlnx-add-clu-reset.patch \
						  file://0014-add-loads-rsa-bitstream-support-for-zboot.patch \
						  file://0015-add-mars-loopback-assert-support.patch \
						  file://0016-add-motorcomm-phy-YT8531S-support.patch \
						  file://0017-fix-leds-status-of-mars-phy.patch \
						  file://0018-remove-auto-update-zboot.patch \
						  file://0019-support-multi-fpga-bit-loading.patch \
						  file://0020-add_zu67_version_selection_support_for_zboot.patch \
						  file://0021-sup-zboot-info-file.patch \
						  file://zboot_info.h \
						  file://0025-device-detection.patch \
						  file://0031-add-zboot-env.patch \
						  file://0032-zboot-zu67-se-does-not-use-encrypted-bit-files.patch \
						  file://0034-add-loopback-for-yt8521s.patch \
						  file://platform-top.h \
						  file://CBRS_uboot_config.cfg \
						"

SRC_URI_append_fara = "${fara_uboot_patch_list}"
SRC_URI_append_fara2 = "${fara_uboot_patch_list}"

SRC_URI_append_mega = "${fara_uboot_patch_list}"
SRC_URI_remove_mega = " file://CBRS_uboot_config.cfg"
SRC_URI_append_mega = " file://Mega_uboot_config.cfg"

SRC_URI_append_saku2 = " \
						file://0100-modify-boot-env-mem-layout.patch \
					  "

SRC_URI_append_bailong = " \
						file://0101-adapt-bailong-eth-set.patch \
					  "

SRC_URI_append_fmav = " \
						${fara_uboot_patch_list} \
						file://fmav_uboot_config.cfg \
						file://0028-modify-zynqmp-log.patch \
					  "

SRC_URI_append_zu67se = " \
						 file://zu67se_uboot_config.cfg \
						"

SRC_URI_remove_emimoa = " file://CBRS_uboot_config.cfg"
SRC_URI_append_emimoa = " file://0102-set-ip-mac-for-auxiliary.patch \
                          file://emimoa_uboot_config.cfg"

do_configure_append_fara () {
	install ${WORKDIR}/platform-top.h ${S}/include/configs/
	cp -f ${WORKDIR}/zboot_info.h ${S}/include/
}

do_configure_append_mega () {
	install ${WORKDIR}/platform-top.h ${S}/include/configs/
	cp -f ${WORKDIR}/zboot_info.h ${S}/include/
}

do_configure_append_fara2 () {
	install ${WORKDIR}/platform-top.h ${S}/include/configs/
	cp -f ${WORKDIR}/zboot_info.h ${S}/include/
}

do_configure_append_microblaze () {
	if [ "${U_BOOT_AUTO_CONFIG}" = "1" ]; then
		install ${WORKDIR}/platform-auto.h ${S}/include/configs/
		install -d ${B}/source/board/xilinx/microblaze-generic/
		install ${WORKDIR}/config.mk ${B}/source/board/xilinx/microblaze-generic/
	fi
}

do_deploy_append () {
	mkenvimage -o ${DEPLOYDIR}/bootenv.bin ${DEPLOYDIR}/${UBOOT_INITIAL_ENV} -s 0x20000
}
