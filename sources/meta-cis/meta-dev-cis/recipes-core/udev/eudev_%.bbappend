FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

##for fara
SRC_URI_append = " \
         file://79-net-name-config.rules \
"

##for zulu2
SRC_URI_append_zulu2 = " \
         file://79-net-name-config.rules.zulu2 \
"

##for saku running on 2 net ports
SRC_URI_append_saku = " \
         file://79-net-name-config.rules.saku \
"

##for mega running on gem3?
SRC_URI_append_mega = " \
         file://79-net-name-config.rules.mega \
"

#for das hu/ru only 1 debug port
SRC_URI_remove_hu = " file://79-net-name-config.rules.saku "
SRC_URI_remove_mru = " file://79-net-name-config.rules.saku "

do_install_append(){
	rm ${D}${sysconfdir}/udev/rules.d/11-sd-cards-auto-mount.rules

    if [ -f ${WORKDIR}/79-net-name-config.rules.* ];then
      cp ${WORKDIR}/79-net-name-config.rules.*  ${WORKDIR}/79-net-name-config.rules
    fi

	install -d ${D}${sysconfdir}/udev/rules.d
	install -m 0644 ${WORKDIR}/79-net-name-config.rules ${D}${sysconfdir}/udev/rules.d/79-net-name-config.rules
}

