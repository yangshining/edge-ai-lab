FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

do_install:append:zulu2() {
	rm ${D}${sysconfdir}/udev/rules.d/automount.rules
}

do_install:append:saku () {
	rm -f ${D}${sysconfdir}/udev/rules.d/automount.rules
}

