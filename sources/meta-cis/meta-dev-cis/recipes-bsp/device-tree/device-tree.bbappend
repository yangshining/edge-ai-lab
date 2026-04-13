FILESEXTRAPATHS_prepend := "${THISDIR}/files:${SYSCONFIG_PATH}:"

##for zulu2
SRC_URI_append = " file://config \
                   file://reserved-memory.dtsi \
                   file://system-user.dtsi \
                 "

##for fara
SRC_URI_remove_fara = " file://system-user.dtsi"
SRC_URI_append_fara = " file://system-user_fara.dtsi"
SRC_URI_append_fara = " file://system-conf_fara.dtsi"

##for mega
SRC_URI_remove_mega = " file://system-user.dtsi"
#SRC_URI_remove_mega = " file://reserved-memory.dtsi"
#SRC_URI_append_mega = " file://reserved-memory_mega.dtsi"
SRC_URI_append_mega = " file://system-user_mega.dtsi"
SRC_URI_append_mega = " file://system-conf_mega.dtsi"

##for soga
SRC_URI_remove_soga = " file://system-user.dtsi"
SRC_URI_append_soga = " file://system-user_soga.dtsi"
SRC_URI_append_soga = " file://system-conf_fara2.dtsi"

##for fara2
SRC_URI_remove_fara2 = " file://system-user.dtsi"
SRC_URI_append_fara2 = " file://system-user_fara2.dtsi"
SRC_URI_remove_fara2_saku = " file://system-user_fara2.dtsi"
SRC_URI_append_fara2_saku = " file://system-user_saku.dtsi"
SRC_URI_append_fara2 = " file://system-conf_fara2.dtsi"

##for ulak series products (b1b3-matterhorn, b7-Weisshorn, b8b20b28-Breithorn)
SRC_URI_remove_ulak = " file://system-conf_fara2.dtsi"
SRC_URI_append_ulak = " file://system-conf_ulak.dtsi"

##for fara2-saku-saku2-apple
SRC_URI_remove_apple = "  file://system-user_saku.dtsi"
SRC_URI_append_apple = "  file://system-user_apple.dtsi"

SRC_URI_remove_mru = " file://system-user_saku.dtsi"
SRC_URI_append_mru = " file://system-user_mru.dtsi"

SRC_URI_remove_mu = " file://system-user_saku.dtsi"
SRC_URI_append_mu = " file://system-user_mu.dtsi"

SRC_URI_remove_hu = " file://system-user_saku.dtsi"
SRC_URI_append_hu = " file://system-user_hu.dtsi"

SRC_URI_append_fara2_saku = " file://reserved-memory_saku.dtsi"
SRC_URI_remove_fara2_saku_saku2 = " file://reserved-memory_saku.dtsi"
SRC_URI_append_fara2_saku_saku2 = " file://reserved-memory_saku2.dtsi"

SRC_URI_remove_phoenix = " file://system-user_saku.dtsi"
SRC_URI_append_phoenix = " file://system-user_phoenix.dtsi"

SRC_URI_remove_kili = " file://reserved-memory_saku.dtsi"
SRC_URI_append_kili = " file://reserved-memory_kili.dtsi"

##for gemini
SRC_URI_remove_gemini = " file://system-user.dtsi"
SRC_URI_append_gemini = " file://system-user_fara2.dtsi"
SRC_URI_append_gemini = " file://system-conf_fara2.dtsi"

##for versal 1202
SRC_URI_remove_vpk120 = " file://system-conf_fara2.dtsi"
SRC_URI_append_vpk120 = " file://system-conf_vpk120.dtsi"
SRC_URI_remove_vpk120 = " file://system-user_saku.dtsi"
SRC_URI_append_vpk120 = " file://system-user_vpk120.dtsi"
##for versal 1802
SRC_URI_remove_emimom = " file://system-conf_fara2.dtsi"
SRC_URI_append_emimom = " file://system-conf_doara.dtsi"
SRC_URI_remove_emimom = " file://system-user_saku.dtsi"
SRC_URI_append_emimom = " file://system-user_doara.dtsi"
##for doara auxiliary zynqmp chipsets
SRC_URI_remove_emimoa = " file://system-conf_fara2.dtsi"
SRC_URI_append_emimoa = " file://system-conf_emimoa.dtsi"
SRC_URI_remove_emimoa = " file://system-user_saku.dtsi"
SRC_URI_append_emimoa = " file://system-user_emimoa.dtsi"


python () {
    if d.getVar("CONFIG_DISABLE"):
        d.setVarFlag("do_configure", "noexec", "1")
}

export PETALINUX
do_configure_append () {
    script="${PETALINUX}/etc/hsm/scripts/petalinux_hsm_bridge.tcl"
    data=${PETALINUX}/etc/hsm/data/
    eval xsct -sdx -nodisp ${script} -c ${WORKDIR}/config \
    -hdf ${DT_FILES_PATH}/hardware_description.${HDF_EXT} -repo ${S} \
    -data ${data} -sw ${DT_FILES_PATH} -o ${DT_FILES_PATH} -a "soc_mapping"

    bbnote "to use ${PN} specific dtsi file"
    if [ -f ${WORKDIR}/system-user_*.dtsi ];then
        cp ${WORKDIR}/system-user_*.dtsi  ${WORKDIR}/system-user.dtsi
    fi

    if [ -f ${WORKDIR}/reserved-memory_*.dtsi ];then
        cp ${WORKDIR}/reserved-memory_*.dtsi  ${WORKDIR}/reserved-memory.dtsi
    fi

    echo '#include "reserved-memory.dtsi"' >> ${WORKDIR}/system-user.dtsi

    bbnote "replace system-conf.dtsi to adapt customized requirement."
    if [ -f ${DT_FILES_PATH}/system-conf.dtsi ];then
        if [ -f ${WORKDIR}/system-conf_*.dtsi ];then
            cp -f ${WORKDIR}/system-conf_*.dtsi ${DT_FILES_PATH}/system-conf.dtsi
        fi
    else
        bbfatal "system-conf.dtsi doesn't exist, please check!"
    fi
}
