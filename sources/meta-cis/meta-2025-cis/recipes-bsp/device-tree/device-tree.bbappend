FILESEXTRAPATHS:prepend := "${THISDIR}/files:${SYSCONFIG_PATH}:"

SRC_URI:append = " \
		file://reserved-memory.dtsi \ 
		file://system-user.dtsi \
		"

##for fara
SRC_URI:remove:fara = " file://system-user.dtsi"
SRC_URI:append:fara = " file://system-user_fara.dtsi"
SRC_URI:append:fara = " file://system-conf_fara.dtsi"

##for mega
SRC_URI:remove:mega = " file://system-user.dtsi"
#SRC_URI:remove:mega = " file://reserved-memory.dtsi"
#SRC_URI:append:mega = " file://reserved-memory_mega.dtsi"
SRC_URI:append:mega = " file://system-user_mega.dtsi"
SRC_URI:append:mega = " file://system-conf_mega.dtsi"

##for soga
SRC_URI:remove:soga = " file://system-user.dtsi"
SRC_URI:append:soga = " file://system-user_soga.dtsi"
SRC_URI:append:soga = " file://system-conf_fara2.dtsi"

##for fara2
SRC_URI:remove:fara2 = " file://system-user.dtsi"
SRC_URI:append:fara2 = " file://system-user_fara2.dtsi"
SRC_URI:remove:saku = " file://system-user_fara2.dtsi"
SRC_URI:append:saku = " file://system-user_saku.dtsi"
SRC_URI:append:fara2 = " file://system-conf_fara2.dtsi"

##for fara2-saku-saku2-apple
SRC_URI:remove:apple = "  file://system-user_saku.dtsi"
SRC_URI:append:apple = "  file://system-user_apple.dtsi"

SRC_URI:remove:apricot = "  file://system-user_saku.dtsi"
SRC_URI:append:apricot = "  file://system-user_apricot.dtsi"

SRC_URI:remove:bberry = "  file://system-user_saku.dtsi"
SRC_URI:append:bberry = "  file://system-user_bberry.dtsi"

SRC_URI:remove:grape = "  file://system-user_saku.dtsi"
SRC_URI:append:grape = "  file://system-user_grape.dtsi"

SRC_URI:remove:mru = " file://system-user_saku.dtsi"
SRC_URI:append:mru = " file://system-user_mru.dtsi"

SRC_URI:remove:mu = " file://system-user_saku.dtsi"
SRC_URI:append:mu = " file://system-user_mu.dtsi"

SRC_URI:remove:hu = " file://system-user_saku.dtsi"
SRC_URI:append:hu = " file://system-user_hu.dtsi"
SRC_URI:append:saku = " file://reserved-memory_saku.dtsi"
SRC_URI:remove:saku2 = " file://reserved-memory_saku.dtsi"
SRC_URI:append:saku2 = " file://reserved-memory_saku2.dtsi"

SRC_URI:remove:phoenix = " file://system-user_saku.dtsi"
SRC_URI:append:phoenix = " file://system-user_phoenix.dtsi"
##for gemini
SRC_URI:remove:gemini = " file://system-user.dtsi"
SRC_URI:append:gemini = " file://system-user_fara2.dtsi"
SRC_URI:append:gemini = " file://system-conf_fara2.dtsi"

##for versal 1202
SRC_URI:remove:vpk120 = " file://system-conf_fara2.dtsi"
SRC_URI:append:vpk120 = " file://system-conf_vpk120.dtsi"
SRC_URI:remove:vpk120 = " file://system-user_saku.dtsi"
SRC_URI:append:vpk120 = " file://system-user_vpk120.dtsi"

##for versal 1802
SRC_URI:remove:emimom = " file://system-conf_fara2.dtsi"
SRC_URI:append:emimom = " file://system-conf_doara.dtsi"
SRC_URI:remove:emimom = " file://system-user_saku.dtsi"
SRC_URI:append:emimom = " file://system-user_doara.dtsi"
SRC_URI:remove:emimom = " file://reserved-memory_saku.dtsi"
SRC_URI:append:emimom = " file://reserved-memory_doara.dtsi"

do_configure:append () {
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

require ${@'device-tree-sdt.inc' if d.getVar('SYSTEM_DTFILE') != '' else ''}
