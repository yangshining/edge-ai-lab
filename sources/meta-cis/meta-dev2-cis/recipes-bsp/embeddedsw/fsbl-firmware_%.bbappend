FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

do_configure:prepend:fara() {
    if [ -d "${S}/patches" ]; then
       rm -rf ${S}/patches
    fi
 
    if [ -d "${S}/.pc" ]; then
       rm -rf ${S}/.pc
    fi
}

do_configure:prepend:fara2() {
    if [ -d "${S}/patches" ]; then
       rm -rf ${S}/patches
    fi

    if [ -d "${S}/.pc" ]; then
       rm -rf ${S}/.pc
    fi
}

do_configure:prepend:mega() {
    if [ -d "${S}/patches" ]; then
       rm -rf ${S}/patches
    fi

    if [ -d "${S}/.pc" ]; then
       rm -rf ${S}/.pc
    fi
}

COMMON_FSBL_PATCH_LIST = " \ 
						file://0001-add-gigadevice-qspi24-support-for-zynqmp-2023p2.patch \
						file://0002-enable-fsbl-log-2023p2.patch\
						file://0004-add-new-prod-sup.patch \
						file://0005-skip-wdt.patch \
						file://1003-test-issi-qspi32-4b.patch \
					  "

SRC_URI:append:fara = "${COMMON_FSBL_PATCH_LIST}"
SRC_URI:append:fara2 = "${COMMON_FSBL_PATCH_LIST}"

SRC_URI:append:mega = "${COMMON_FSBL_PATCH_LIST}"
SRC_URI:append:zu67se = " \
						file://0100-disable-uboot-authentication-2023p2.patch \
						"

#YAML_COMPILER_FLAGS:append = " -DFSBL_DEBUG_DETAILED"
