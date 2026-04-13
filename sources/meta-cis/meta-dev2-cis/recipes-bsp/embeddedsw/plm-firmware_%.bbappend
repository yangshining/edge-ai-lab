FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

COMMON_PLM_PATCH_LIST = " \ 
						file://0001-plm-debug-qspi-flash.patch \
						file://0006-versal-set-fpdwdt-reset.patch \
					  "

SRC_URI:append = "${COMMON_PLM_PATCH_LIST}"

#YAML_COMPILER_FLAGS:append = " -DPLM_DEBUG_DETAILED"
