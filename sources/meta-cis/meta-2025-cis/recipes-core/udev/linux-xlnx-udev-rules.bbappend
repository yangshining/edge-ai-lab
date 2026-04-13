FILESEXTRAPATHS:prepend := "${THISDIR}/eudev:"

##for fara
SRC_URI:append = " \
         file://79-net-name-config.rules \
"

##for zulu2
SRC_URI:append:zulu2 = " \
         file://79-net-name-config.rules.zulu2 \
"

##for saku running on 2 net ports
SRC_URI:append:saku = " \
         file://79-net-name-config.rules.saku \
"

##for mega running on gem3?
SRC_URI:append:mega = " \
         file://79-net-name-config.rules.mega \
"

##for Versal-SoC running on 1 net port by gem1
SRC_URI:append:vpk120 = " \
         file://79-net-name-config.rules.vpk120 \
"

#for das hu/ru only 1 debug port
SRC_URI:remove:hu = " file://79-net-name-config.rules.saku "
SRC_URI:remove:mru = " file://79-net-name-config.rules.saku "
SRC_URI:remove:vpk120 = " file://79-net-name-config.rules.saku "
SRC_URI:remove:emimom = " file://79-net-name-config.rules.saku "
SRC_URI:append:emimom = " file://79-net-name-config.rules.emimom "

do_install:append(){
	#rm ${D}${sysconfdir}/udev/rules.d/11-sd-cards-auto-mount.rules

    if [ -f ${WORKDIR}/79-net-name-config.rules.* ];then
      cp ${WORKDIR}/79-net-name-config.rules.*  ${WORKDIR}/79-net-name-config.rules
    fi

	install -d ${D}${sysconfdir}/udev/rules.d
	install -m 0644 ${WORKDIR}/79-net-name-config.rules ${D}${sysconfdir}/udev/rules.d/79-net-name-config.rules
}

