
FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
KERNELURI = "git://172.16.1.21:29418/3pp/bsp2/linux-xlnx;protocol=ssh;user=${USER};name=machine"

###for zulu2
SRC_URI_append_zulu2 = " \
			file://0001-zulu-sport-uio-driver.patch \
			file://0002-zulu-add-rsmu-driver-and-adjust-ptp-timer.patch \
			file://0003-zulu-support-switch-chip-mv88e6185.patch \
			file://0004-zulu-spi-modify-maybe-used-for-debug.patch \
                        file://0020-add-rmdom-func.patch \
			file://zu19_linux_config.cfg \
			file://zu19_linux_base_defconfig \
                       "

#KBRANCH_zulu2 = "xlnx_rebase_v5.10"
#SRCREV_zulu2 = "c830a552a6c34931352afd41415a2e02cca3310d"

do_kernel_metadata_prepend_zulu2 () {
        #bbwarn "add config to ${TMPDIR}/work-shared/${MACHINE}/kernel-source/arch/arm64/configs/"
	cp -f ${WORKDIR}/zu19_linux_base_defconfig ${TMPDIR}/work-shared/${MACHINE}/kernel-source/arch/arm64/configs/
}

###for fara

#KBRANCH_fara = "xlnx_rebase_v5.10"
#SRCREV_fara = "568989d44176ae0a38ea78c16d0590c726d3b60a"

fara_patch_list = " \
                   file://0001-fara-support-uio-driver.patch \
                   file://0002-fara-support-centec-phy.patch \
                   file://0003-fara-modify-axienet-driver.patch \
                   file://0004-fara-support-chip-mx66l2g45g.patch \
                   file://0005-fara-support-rs485-in-uart.patch \
                   file://0011-FDD-port-code-from-mega.patch \
                   file://0012-FDD-supports-3types-Gigadevice-norflash-in-kernel.patch \
                   file://0013-xilinx_axienet-use-multi-channel-mcdma.patch \
                   file://0014-add-irq-xilinx-intc-interlink-support.patch \
                   file://0015-add-remaining-memory-domains-supported.patch \
                   file://0016-fix-auto-negotiation-gigabit-of-the-debug-port.patch \
                   file://CBRS_linux_config.cfg \
                  "

SRC_URI_append_fara = "${fara_patch_list}"
SRC_URI_append_fara2 = "${fara_patch_list}"
SRC_URI_append_mega = "${fara_patch_list}"
#SRC_URI_remove_mega = " file://CBRS_linux_config.cfg"
#SRC_URI_append_mega = " file://Mega_linux_config.cfg"
SRC_URI_append_mega = " file://mega_ptp_enable.cfg"

SRC_URI_append_emimoa = " \
			file://0100-emimoa-support-internal-pcspma.patch \
			"

SRC_URI_append_kili2 = " \
                      file://mu-support-multi-uartlite.cfg \
                    "

SRC_URI_append_mu = " \
                      file://0017-mu-add-multi-spidev.patch \
                      file://0018-add-motorcomm-phy-driver.patch \
                      file://0019-config-phy-led-behavior.patch \
                      file://mu-support-multi-uartlite.cfg \
                    "
KERNEL_FEATURES_append_fara = " CBRS_linux_config.cfg"
KERNEL_FEATURES_append_fara2 = " CBRS_linux_config.cfg"
#KERNEL_FEATURES_append_mega = " Mega_linux_config.cfg"
