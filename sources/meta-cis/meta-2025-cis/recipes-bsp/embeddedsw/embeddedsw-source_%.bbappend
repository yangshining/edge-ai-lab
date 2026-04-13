FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

COMMON_FSBL_PATCH_LIST = " \ 
                    file://0001-add-gigadevice-qspi24-support-for-zynqmp-2025p1.patch \
                    file://0001-PMUFW-modification-pm-access-tabe.patch \
                    file://0002-enable-fsbl-log-2025p1.patch\
                    file://0003-skip-wdt-2025p1.patch \
                    file://1003-test-issi-qspi32-4b.patch \
                    file://0001-plm-debug-qspi-flash-2025p1.patch \
                    file://0006-versal-set-fpdwdt-reset-2025p1.patch \
					"

SRC_URI:append:fara = "${COMMON_FSBL_PATCH_LIST}"
SRC_URI:append:fara2 = "${COMMON_FSBL_PATCH_LIST}"

SRC_URI:append:mega = "${COMMON_FSBL_PATCH_LIST}"
SRC_URI:append:zu67se = " \
					file://0100-disable-uboot-authentication-2023p2.patch \
					file://0102-disable-eFuse-checks-in-bitstream-validation-path.patch \
					"

