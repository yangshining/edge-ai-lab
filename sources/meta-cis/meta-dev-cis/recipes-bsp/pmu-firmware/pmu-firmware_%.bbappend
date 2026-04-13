do_configure_prepend() {
    if [ -d "${S}/patches" ]; then
       rm -rf ${S}/patches
    fi
 
    if [ -d "${S}/.pc" ]; then
       rm -rf ${S}/.pc
    fi
}

SRC_URI_append = " \ 
        file://0001-PMUFW-modification-pm-access-tabe.patch \
        "

SRC_URI_append_zu67se = " \
        file://0002-disable-eFuse-checks-in-bitstream-validation-path.patch \
        "

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

YAML_COMPILER_FLAGS_append = " -DENABLE_EM -DENABLE_SCHEDULER -DENABLE_WDT"