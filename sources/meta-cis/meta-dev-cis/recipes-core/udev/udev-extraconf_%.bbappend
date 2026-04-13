FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

do_install_append_zulu2() {
	rm ${D}${sysconfdir}/udev/rules.d/automount.rules
}