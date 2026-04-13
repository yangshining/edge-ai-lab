FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

do_configure_prepend_fara() {
    if [ -d "${S}/patches" ]; then
       rm -rf ${S}/patches
    fi
 
    if [ -d "${S}/.pc" ]; then
       rm -rf ${S}/.pc
    fi
}

do_configure_prepend_fara2() {
    if [ -d "${S}/patches" ]; then
       rm -rf ${S}/patches
    fi

    if [ -d "${S}/.pc" ]; then
       rm -rf ${S}/.pc
    fi
}

do_configure_prepend_mega() {
    if [ -d "${S}/patches" ]; then
       rm -rf ${S}/patches
    fi

    if [ -d "${S}/.pc" ]; then
       rm -rf ${S}/.pc
    fi
}

SRC_URI_append_fara = " \ 
						file://0001-add-gigadevice-qspi24-support-for-zynqmp.patch \
						file://0002-enable-fsbl-log.patch \
					  "

SRC_URI_append_fara2 = " \
						file://0001-add-gigadevice-qspi24-support-for-zynqmp.patch \
						file://0002-enable-fsbl-log.patch \
						file://0013-add-new-prod-sup.patch \
						file://0014-skip-wdt.patch \
						file://1003-test-issi-qspi32-4b.patch \
						"
SRC_URI_append_mega = " \
						file://0001-add-gigadevice-qspi24-support-for-zynqmp.patch \
						file://0002-enable-fsbl-log.patch \
						file://0013-add-new-prod-sup.patch \
						file://0014-skip-wdt.patch \
						"
						
SRC_URI_append_zu67se = " \
						file://0005-disable-uboot-authentication.patch \
						"

#YAML_COMPILER_FLAGS_append = " -DFSBL_DEBUG_DETAILED"
