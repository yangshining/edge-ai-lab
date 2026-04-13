do_configure:prepend() {
    if [ -d "${S}/patches" ]; then
       rm -rf ${S}/patches
    fi
 
    if [ -d "${S}/.pc" ]; then
       rm -rf ${S}/.pc
    fi
}

YAML_COMPILER_FLAGS:append = " -DENABLE_EM -DENABLE_SCHEDULER -DENABLE_WDT"