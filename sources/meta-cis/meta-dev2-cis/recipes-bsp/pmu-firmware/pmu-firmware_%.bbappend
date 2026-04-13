do_configure:prepend() {
    if [ -d "${S}/patches" ]; then
       rm -rf ${S}/patches
    fi
 
    if [ -d "${S}/.pc" ]; then
       rm -rf ${S}/.pc
    fi
}

SRC_URI:append = " \ 
        file://0001-PMUFW-modification-pm-access-tabe.patch \
        "

SRC_URI:append:zu67se = " \
        file://0002-disable-eFuse-checks-in-bitstream-validation-path.patch \
        "

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

YAML_COMPILER_FLAGS:append = " -DENABLE_EM -DENABLE_SCHEDULER -DENABLE_WDT"