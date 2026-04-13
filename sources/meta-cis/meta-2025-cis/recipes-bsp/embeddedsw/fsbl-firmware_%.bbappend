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

#YAML_COMPILER_FLAGS:append = " -DFSBL_DEBUG_DETAILED"
