FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
KERNELURI = "git://172.16.1.21:29418/3pp/bsp2/linux-xlnx;protocol=ssh;user=${USER};name=machine"

SRC_URI:append = " file://bsp.cfg"
KERNEL_FEATURES:append = " bsp.cfg"

fara_patch_list = " \
                   file://0001-fara-support-uio-driver.patch \
                   file://0002-fara-support-centec-phy-2023.2.patch \
                   file://0003-fara-modify-axienet-driver.patch \
                   file://0004-fara-support-rs485-in-uart-2023.2.patch \
                   file://0005-fara-supports-3types-gd-norflash-in-kernel-2023.2.patch \
                   file://0006-fara-xilinx_axienet-use-multi-channel-mcdma-2023.2.patch \
                   file://0007-fara-add-irq-xilinx-intc-interlink-support.patch \
                   file://0008-fara-add-remaining-memory-domains-supported-2023.1.patch \
                   file://0009-fix-auto-negotiation-gigabit-of-the-debug-port.patch \
                   file://0010-Support-rsmu-cdev-in-2023.1.patch \
                   file://0011-skip-axienet-print.patch \
                   file://0012-Kernel-add-YT-phy-driver.patch \
                   file://0013-sup-yt-phy-10-100-1000m-led-blink.patch \
                   file://CBRS_linux_config.cfg \
                  "

SRC_URI:append:fara = "${fara_patch_list}"
SRC_URI:append:fara2 = "${fara_patch_list}"
SRC_URI:append:mega = "${fara_patch_list}"
#SRC_URI:remove:mega = " file://CBRS_linux_config.cfg"
#SRC_URI:append:mega = " file://Mega_linux_config.cfg"
#SRC_URI:append:mega = " file://mega_ptp_enable.cfg"

KERNEL_FEATURES:append:fara = " CBRS_linux_config.cfg"
KERNEL_FEATURES:append:fara2 = " CBRS_linux_config.cfg"
#KERNEL_FEATURES:append:mega = " Mega_linux_config.cfg"
